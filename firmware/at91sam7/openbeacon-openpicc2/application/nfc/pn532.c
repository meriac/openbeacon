#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <beacontypes.h>

#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <USB-CDC.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "spi.h"
#include "led.h"
#include "pio_irq.h"
#include "pn532.h"

static const int SCBR = ((int) (MCK / 5e6) + 1) & 0xFF;
static spi_device pn532_spi;
static xSemaphoreHandle pn532_rx_semaphore;
static int pn532_rx_enabled = 0;

//#define PN532_EMULATE_FAKE_INTERRUPTS
//#define DEBUG_MESSAGE_FLOW
//#define DEBUG_QUEUE_POSTING

#define PN532_NR_MESSAGE_BUFFER 3
#define PN532_NR_WAIT_QUEUES 4

#define PN532_WAIT_PRIORITY_MAX 255
#define PN532_WAIT_PRIORITY_RENUMBER_THRESHOLD 200

struct pn532_wait_queue {
	unsigned int wait_mask; /* == 0 if this queue is unused */
	xQueueHandle message_queue;
	unsigned char wait_priority;
	unsigned char match_length;
	unsigned char match_data[2];
} __attribute__((packed));

static struct pn532_message_buffer *last_received;
static struct pn532_message_buffer pn532_message_buffers[PN532_NR_MESSAGE_BUFFER];
static struct pn532_wait_queue pn532_wait_queues[PN532_NR_WAIT_QUEUES];

/* Utilities to handle message buffer structs (from the static field pn532_message_buffers) */
static int _pn532_get_message_buffer(struct pn532_message_buffer **msg, int in_irq); /* Strictly internal for use in IRQ handlers */
static int _pn532_put_message_buffer(struct pn532_message_buffer **msg, int skip_clean); /* Strictly internal for use in IRQ handlers */

/* Internal */
static void reverse_bytes(u_int8_t *data, const unsigned int len);
static void pn532_enable_rx(u_int8_t enable, u_int8_t from_irq);
static void pn532_reset(void);

#ifndef __GNUC__
/* Evaluates to 18 instructions */
#define REVERSE_TWOBIT(a) ( ((a)&1)<<1 | ((a)>>1) )
#define REVERSE_NIBBLE(a) (REVERSE_TWOBIT((a)&0x3)<<2 | REVERSE_TWOBIT((a)>>2) )
#define REVERSE_BYTE(a) ( REVERSE_NIBBLE((a)&0xF)<<4 | REVERSE_NIBBLE((a)>>4) )
#else
/* Evaluates to 10 instructions */
#define REVERSE_BYTE2(c) ({u_int8_t _c = (c); \
	((_c&0xAA)>>1) | ((_c<<1) & 0xAA); })
#define REVERSE_BYTE1(b) ({u_int8_t _b = (b); _b=REVERSE_BYTE2(_b); \
	((_b&0xCC)>>2) | ((_b<<2) & 0xCC); })
#define REVERSE_BYTE(a) ({u_int8_t _a = (a); _a=REVERSE_BYTE1(_a); \
	((_a&0xF0)>>4) | ((_a<<4) & 0xF0); })

/* Reverse 4 bytes at once */
#define REVERSE_BYTE_W2(c) ({u_int32_t _c = (c); \
	((_c&0xAAAAAAAA)>>1) | ((_c<<1) & 0xAAAAAAAA); })
#define REVERSE_BYTE_W1(b) ({u_int32_t _b = (b); _b=REVERSE_BYTE_W2(_b); \
	((_b&0xCCCCCCCC)>>2) | ((_b<<2) & 0xCCCCCCCC); })
#define REVERSE_BYTE_W(a) ({u_int32_t _a = (a); _a=REVERSE_BYTE_W1(_a); \
	((_a&0xF0F0F0F0)>>4) | ((_a<<4) & 0xF0F0F0F0); })
#endif

#define PN532_SPI_COMMAND_READ_SPI_STATUS 0x02
#define PN532_SPI_COMMAND_READ_FIFO 0x03
#define PN532_SPI_COMMAND_WRITE_FIFO 0x01

/* Receive methodology:
 * The data sent from the PN532 may be of arbitrary size. In order not to have to schedule an
 * SPI receive job for the full maximum message length each time we'll do the following:
 *  + Start the receive process by scheduling a receive job for the first 8 bytes
 *  + In the SPI completion callback, look at and decode those first 8 bytes to determine the frame
 *    type and frame size (6 bytes should be enough to determine wether it's ACK/NACK (6 bytes), a
 *    normal frame header (5 bytes including the length) or an extended frame header (8 bytes including
 *    the length). In the case of an ACK or NACK packet we will receive some additional bytes of postamble,
 *    in the case of a normal frame we will receive some content bytes that need to be decoded and stored.
 *  + If necessary (extended frame, normal frame with more than two payload bytes), schedule an SPI
 *    receive job for the remainder of the frame to be executed *right*now*. The chip select line will
 *    not be released in between these two jobs (SPI_FLAG_DELAYED_CS_INACTIVE).
 * +  In the case of completion of an additional receive job give the partially decoded frame to the
 *    rx task for further decoding (no need to do this in interrupt context).
 */
#define INITIAL_RECEIVE_LEN 8

static void pn532_callback(void *buf, unsigned int len, portBASE_TYPE *xTaskWoken)
{
	(void)len;
	int i, full_frame = 0;
	struct pn532_message_buffer *msg = NULL;
	for(i=0; i<PN532_NR_MESSAGE_BUFFER; i++) {
		if ((buf == &(pn532_message_buffers[i].message.spi_header))  ||
			(buf == ((void*)(&(pn532_message_buffers[i].message.data)))
					+ pn532_message_buffers[i].received_len
					- pn532_message_buffers[i].additional_receive_len) ) {
			msg = &(pn532_message_buffers[i]);
			break;
		}
	}
	if(msg == NULL) return;
	if(msg->direction == DIRECTION_OUTGOING) return;

	pn532_parse_frame(msg);

	switch(msg->type) {
	case MESSAGE_TYPE_NORMAL: /* Fall-through */
	case MESSAGE_TYPE_EXTENDED:
		/* Schedule new job for the remaining len, if necessary */
		if(msg->additional_receive_len > 0) {
			unsigned int oldlen = msg->received_len;
			msg->received_len += msg->additional_receive_len;
			if(spi_transceive_from_irq(&pn532_spi, ((void*)(&(msg->message.data)))+oldlen,
					msg->additional_receive_len, 1, xTaskWoken) < 0) {
				msg->state = MESSAGE_STATE_EXCEPTION; /* Scheduling the second SPI job failed, give something to the rx task so that it can recover */
				full_frame = 1;
			}
		} else {
			full_frame = 1;
		}
		break;
	case MESSAGE_TYPE_ACK: /* Fall-through */
	case MESSAGE_TYPE_NACK:
	case MESSAGE_TYPE_ERROR:
	case MESSAGE_TYPE_UNKNOWN: /* This one is so that the rx task may reenable the interrupt */
		full_frame = 1;
		break;
	}

	if(full_frame) {
		last_received = msg;
		xSemaphoreGiveFromISR(pn532_rx_semaphore, xTaskWoken);
	}
}

static void pn532_prepare_receive(struct pn532_message_buffer *msg)
{
	msg->bufsize = sizeof(msg->message.data);
	msg->message.checksum = 0;
	int toclean = 6;
	if(msg->received_len > toclean) toclean = msg->received_len;
	if(msg->bufsize < toclean) toclean = msg->bufsize;
	memset(msg->message.data, 0x0, toclean);
	msg->direction = DIRECTION_INCOMING;

	msg->decoded_len = 0;
	msg->payload_len = 0;
	msg->received_len = 0;
	msg->additional_receive_len = 0;
	msg->decoded_raw_len = 0;

	msg->type = MESSAGE_TYPE_UNKNOWN;
}

static portBASE_TYPE pn532_trigger_recv(AT91PS_PIO pio, u_int32_t pin, portBASE_TYPE xTaskWoken)
{
	(void)pio; (void)pin;
	struct pn532_message_buffer *msg;
	if(_pn532_get_message_buffer(&msg, 1) != 0) return xTaskWoken;
	pn532_enable_rx(0, 1);
	msg->direction = DIRECTION_INCOMING;
	msg->state = MESSAGE_STATE_IN_PREAMBLE;
	msg->message.spi_header = REVERSE_BYTE(PN532_SPI_COMMAND_READ_FIFO);
	msg->received_len = INITIAL_RECEIVE_LEN;
	spi_transceive_from_irq(&pn532_spi, &(msg->message.spi_header), msg->received_len+1, 0, &xTaskWoken);
	return xTaskWoken;
}

static portBASE_TYPE pn532_irq(AT91PS_PIO pio, u_int32_t pin, portBASE_TYPE xTaskWoken)
{
	if(AT91F_PIO_IsInputSet(pio, pin)) return pdFALSE;
	return pn532_trigger_recv(pio, pin, xTaskWoken);
}

static int _pn532_get_message_buffer(struct pn532_message_buffer **msg, int in_irq)
{
	if(msg == NULL) return -EINVAL;
	*msg = NULL;
	if(!in_irq) taskENTER_CRITICAL();
	int i;
	for(i=0; i<PN532_NR_MESSAGE_BUFFER; i++) {
		if(pn532_message_buffers[i].state == MESSAGE_STATE_FREE) {
			*msg = &(pn532_message_buffers[i]);
			(*msg)->state = MESSAGE_STATE_RESERVED;
			break;
		}
	}
	if(!in_irq) taskEXIT_CRITICAL();
	if(*msg == NULL) return -EBUSY;
	return 0;
}

static int _pn532_put_message_buffer(struct pn532_message_buffer **msg, int skip_clean)
{
	if(msg == NULL) return -EINVAL;
	if(*msg == NULL) return -EINVAL;
	if(!skip_clean) pn532_prepare_receive(*msg);
	(*msg)->state = MESSAGE_STATE_FREE;
	return 0;
}

int pn532_get_message_buffer(struct pn532_message_buffer **msg)
{
	return _pn532_get_message_buffer(msg, 0);
}

int pn532_put_message_buffer(struct pn532_message_buffer **msg)
{
	return _pn532_put_message_buffer(msg, 0);
}

static int _pn532_get_wait_queue(struct pn532_wait_queue **queue, unsigned int wait_mask, unsigned char match_length, const unsigned char * const match_data, int in_irq)
{
	int i;
	int min_priority=-1, max_priority=-1;
	if(queue == NULL) return -EINVAL;
	if(match_length > sizeof((*queue)->match_data)) return -EINVAL;
	*queue = NULL;
	if(!in_irq) taskENTER_CRITICAL();

#ifdef DEBUG_QUEUE_MANAGEMENT
	printf("<"); for(i=0; i<PN532_NR_WAIT_QUEUES; i++) {
		if(pn532_wait_queues[i].wait_mask == 0) printf("!");
		else printf("%i", pn532_wait_queues[i].wait_priority);
	} printf(">");
#endif

	for(i=0; i<PN532_NR_WAIT_QUEUES; i++) {
		if(pn532_wait_queues[i].wait_mask == 0) {
			if(*queue == NULL) {
				*queue = &(pn532_wait_queues[i]);
#ifdef DEBUG_QUEUE_MANAGEMENT
				printf("[%i]", i);
#endif
			}
		} else {
			if(min_priority == -1) min_priority = pn532_wait_queues[i].wait_priority;
			else if(pn532_wait_queues[i].wait_priority < min_priority) min_priority = pn532_wait_queues[i].wait_priority;
			if(max_priority == -1) max_priority = pn532_wait_queues[i].wait_priority;
			else if(pn532_wait_queues[i].wait_priority > max_priority) max_priority = pn532_wait_queues[i].wait_priority;
		}
	}

	if(*queue != NULL) {
		if(min_priority > PN532_WAIT_PRIORITY_RENUMBER_THRESHOLD) {
			for(i=0; i<PN532_NR_WAIT_QUEUES; i++)
				pn532_wait_queues[i].wait_priority -= min_priority;
			max_priority -= min_priority;
			min_priority -= min_priority;
		}

		(*queue)->wait_mask = wait_mask;
		(*queue)->wait_priority = max_priority+1;
		(*queue)->match_length = match_length;
		for(i=0; i<match_length; i++) {
			(*queue)->match_data[i] = match_data[i];
		}
#ifdef DEBUG_QUEUE_MANAGEMENT
		printf("(%i,%i)", (*queue)->wait_priority, ((*queue)-pn532_wait_queues)/sizeof(pn532_wait_queues[0]));
#endif
	}

	if(!in_irq) taskEXIT_CRITICAL();
	if(*queue == NULL) return -EBUSY;
#ifdef DEBUG_QUEUE_MANAGEMENT
	printf("{");
#endif
	return 0;
}

static int _pn532_put_wait_queue(struct pn532_wait_queue **queue)
{
	if(queue == NULL) return -EINVAL;
	if(*queue == NULL) return -EINVAL;
	(*queue)->wait_mask = 0;
#ifdef DEBUG_QUEUE_MANAGEMENT
	printf("}");
#endif
	return 0;
}

int pn532_get_wait_queue(struct pn532_wait_queue **queue, unsigned int wait_mask, unsigned char match_length, const unsigned char * const match_data)
{
	return _pn532_get_wait_queue(queue, wait_mask, match_length, match_data, 0);
}
int pn532_put_wait_queue(struct pn532_wait_queue **queue)
{
	return _pn532_put_wait_queue(queue);
}

static const char PN532_CMD_GetGeneralStatus[] = {0xD4, 0x04};
static const char PN532_CMD_GetFirmwareVersion[] = {0xD4, 0x02};

inline void
DumpUIntToUSB (unsigned int data)
{
  int i = 0;
  unsigned char buffer[10], *p = &buffer[sizeof (buffer)];

  do
    {
      *--p = '0' + (unsigned char) (data % 10);
      data /= 10;
      i++;
    }
  while (data);

  while (i--)
    vUSBSendByte (*p++);
}


static void pn532_rx_task(void *parameter)
{
	(void)parameter;
	pn532_reset();
	vTaskDelay(10/portTICK_RATE_MS);
	while(1) {
		pn532_enable_rx(1, 0);
		xSemaphoreTake(pn532_rx_semaphore, portMAX_DELAY);

		if(last_received == NULL) {
			printf("Whee, got a null pointer\n");
			pn532_put_message_buffer(&last_received);
			continue;
		}

		if(last_received->state == MESSAGE_STATE_EXCEPTION) {
			printf("Receive exception occured\n");
			pn532_put_message_buffer(&last_received);
			continue;
		}

		unsigned int i;
		int r = pn532_parse_frame(last_received);
#ifdef DEBUG_MESSAGE_FLOW
		printf("M\n");
#endif

#if 0
		if(r >= 0) {
			switch(last_received->type) {
			case MESSAGE_TYPE_NORMAL: printf("normal: "); break;
			case MESSAGE_TYPE_EXTENDED: printf("extd.:  "); break;
			case MESSAGE_TYPE_ACK: printf("ack"); break;
			case MESSAGE_TYPE_NACK: printf("nack"); break;
			case MESSAGE_TYPE_ERROR: printf("error: "); break;
			case MESSAGE_TYPE_UNKNOWN: printf("UNKNOWN: "); break;
			}
			if(0) for(i=0; i<last_received->payload_len; i++)
				printf("%02X ", last_received->message.data[i]);
			printf("\n");
		} else {
			if(0) printf("RX error: %i\n", r);
			else printf("RX error\n");
		}
#else
		(void)r; (void)i;
#endif

		struct pn532_wait_queue *queue = NULL;
		unsigned int max_priority=0;

		if(last_received->state == MESSAGE_STATE_IN_POSTAMBLE) {
			taskENTER_CRITICAL();
			for(i=0; i<PN532_NR_WAIT_QUEUES; i++) {
				const struct pn532_wait_queue * const current = &(pn532_wait_queues[i]);
				int matches = 0;

				if(current->wait_mask == 0) continue;
				if(current->wait_mask & PN532_WAIT_ACK) {
					matches = matches || ((last_received->type == MESSAGE_TYPE_ACK) || (last_received->type == MESSAGE_TYPE_NACK));
				}
				if(current->wait_mask & PN532_WAIT_ERROR) {
					matches = matches || (last_received->type == MESSAGE_TYPE_ERROR);
				}
				if(current->wait_mask & PN532_WAIT_CONTENT) {
					matches = matches || ((last_received->type == MESSAGE_TYPE_NORMAL) || (last_received->type == MESSAGE_TYPE_EXTENDED));
				}
				if(current->wait_mask & PN532_WAIT_MATCH) {
					if(current->match_length <= last_received->payload_len) {
						int j, m=1;
						for(j=0; j<current->match_length; j++)
							if(current->match_data[j] != last_received->message.data[j])
								m = 0;
						matches = matches || m;
					}
				}
				if(current->wait_mask & PN532_WAIT_CATCHALL) {
					matches = matches || 1;
				}

				if(matches && (current->wait_priority >= max_priority)) {
					queue = (struct pn532_wait_queue*)current;
					max_priority = current->wait_priority;
				}
			}
			taskEXIT_CRITICAL();
		}

		if(queue == NULL) {
			pn532_put_message_buffer(&last_received);
		} else {
			if(!xQueueSend(queue->message_queue, &last_received, 0)) {
				printf("Queue full\n");
				pn532_put_message_buffer(&last_received);
			} else {
#ifdef DEBUG_QUEUE_POSTING
				DumpUIntToUSB(queue->wait_priority);
				printf("P\n");
#endif
			}
		}
	}
}

#ifdef PN532_EMULATE_FAKE_INTERRUPTS
static void pn532_fakeirq_task(void *parameter)
{
	(void)parameter;
	while(1) {
		vTaskDelay(10);
		if(!pn532_rx_enabled) continue;
		u_int8_t status = pn532_read_spi_status_register();
		if(status & 1) {
			portBASE_TYPE xTaskWoken = pdFALSE;
			taskENTER_CRITICAL();
			xTaskWoken = pn532_trigger_recv(PN532_CS_PIO, PN532_CS_PIN, xTaskWoken);
			taskEXIT_CRITICAL();
			if(xTaskWoken)
				taskYIELD();
		}
	}
}
#endif

/* Hard reset the PN532 through the reset controller.
 * Note: wait up to 7 ms after return from this function before using the PN532 */
static void pn532_reset(void)
{
	AT91C_BASE_RSTC->RSTC_RCR = 0xA5000008;
}

int pn532_read_spi_status_register(void)
{
	u_int8_t buf[] = { REVERSE_BYTE(PN532_SPI_COMMAND_READ_SPI_STATUS), 0x00};
	int r = spi_transceive_automatic_retry(&pn532_spi, buf, 2);
	if(r<0) return r;
	spi_wait_for_completion(&pn532_spi);
	return REVERSE_BYTE(buf[1]);
}

int pn532_send_frame(struct pn532_message_buffer *msg)
{
	msg->message.spi_header = REVERSE_BYTE(PN532_SPI_COMMAND_WRITE_FIFO);
	struct pn532_wait_queue *queue;
	struct pn532_message_buffer *ack;
	int r = _pn532_get_wait_queue(&queue, PN532_WAIT_ACK, 0, NULL, 0);
	if(r<0) return r;
	r = spi_transceive_automatic_retry(&pn532_spi, &(msg->message.spi_header), msg->received_len+1);
	if(r<0) {_pn532_put_wait_queue(&queue); return r;}
	spi_wait_for_completion(&pn532_spi);
	if(pn532_recv_frame_queue(&ack, queue) == 0) {
		_pn532_put_wait_queue(&queue);
#ifdef DEBUG_MESSAGE_FLOW
		printf("A\n");
#endif
		if(ack->type == MESSAGE_TYPE_ACK) {
			pn532_put_message_buffer(&ack);
			return 0;
		} else {
			pn532_put_message_buffer(&ack);
			return -EIO;
		}
	} else {
		_pn532_put_wait_queue(&queue);
		return -ECANCELED;
	}
}

int pn532_recv_frame(struct pn532_message_buffer **msg, unsigned int wait_mask)
{
	return pn532_recv_frame_match(msg, wait_mask, 0, NULL);
}

int pn532_recv_frame_match(struct pn532_message_buffer **msg, unsigned int wait_mask, unsigned char match_length, const unsigned char * const match_data)
{
	struct pn532_wait_queue *queue;
	int r = _pn532_get_wait_queue(&queue, wait_mask, match_length, match_data, 0);
	if(r < 0) return r;
#ifdef DEBUG_QUEUE_POSTING
	DumpUIntToUSB(queue->wait_priority);
	printf("W\n");
#endif
	xQueueReceive(queue->message_queue, msg, portMAX_DELAY);
	_pn532_put_wait_queue(&queue);
	return 0;
}

int pn532_recv_frame_queue(struct pn532_message_buffer **msg, struct pn532_wait_queue *queue)
{
#ifdef DEBUG_QUEUE_POSTING
	DumpUIntToUSB(queue->wait_priority);
	printf("W\n");
#endif
	xQueueReceive(queue->message_queue, msg, portMAX_DELAY);
	return 0;
}

/* Fill a frame with payload and construct the surrounding frame structure. Works from end to start
 * in order not to overwrite payload that is stored in the same spot as the frame structure.
 * E.g. you can use a struct pn532_message_buffer, store your payload in message.data and then call
 * this function with a pointer to the message buffer, to message.data and the length and it will
 * construct a proper frame in message.data.
 */
int pn532_prepare_frame(struct pn532_message_buffer *msg, const char *payload, unsigned int plen)
{
	if(plen > PN532_MAX_PAYLOAD_LEN) return -EMSGSIZE;
	int extended_frame = !!(plen > 255);
	int offset = 1 + 2 + 2 + (extended_frame ? 3 : 0);
	u_int8_t checksum = 0;
	unsigned int pos = 0;

	for(pos = 0; pos < plen; pos++) {
		checksum += payload[plen-1-pos];
		msg->message.data[offset+plen-1-pos] = payload[plen-1-pos];
	}
	msg->message.data[offset+plen] = (-checksum) & 0xff;
	msg->message.data[offset+plen+1] = 0;
	msg->message.data[0] = 0;
	msg->message.data[1] = 0x00;
	msg->message.data[2] = 0xff;
	if(!extended_frame) {
		msg->message.data[3] = plen;
		msg->message.data[4] = (-plen) & 0xff;
	} else {
		msg->message.data[3] = 0xff;
		msg->message.data[4] = 0xff;
		msg->message.data[5] = (plen >> 8) & 0xff;
		msg->message.data[6] = (plen >> 0) & 0xff;
		msg->message.data[7] = (-( ((plen >> 8) & 0xff) + ((plen >> 0) & 0xff) )) & 0xff;
	}

	msg->received_len = offset + plen + 1 + 1;
	msg->direction = DIRECTION_OUTGOING;

	reverse_bytes(msg->message.data, msg->received_len);

	return 0;
}

/* Unparse a frame, the perfect counterpart to pn532_parse_frame. Main difference: Is not iterative, only
 * accepts full frames.
 * For standard or extended frames with content it will call pn532_prepare_frame with the right
 * payload and plen values. See description there.
 */
static const u_int8_t NACK_FRAME[] = {0, 0, 0xff, 0xff, 0, 0};
static const u_int8_t ACK_FRAME[] = {0, 0, 0xff, 0, 0xff, 0};
int pn532_unparse_frame(struct pn532_message_buffer *msg)
{
	if(msg == NULL) return -EINVAL;
	if(msg->type == MESSAGE_TYPE_EXTENDED || msg->type == MESSAGE_TYPE_NORMAL || msg->type == MESSAGE_TYPE_ERROR) {
		return pn532_prepare_frame(msg, (char*)&(msg->message.data), msg->payload_len);
	} else if(msg->type == MESSAGE_TYPE_ACK) {
		memcpy(msg->message.data, ACK_FRAME, sizeof(ACK_FRAME));
		msg->received_len = sizeof(ACK_FRAME);
	} else if(msg->type == MESSAGE_TYPE_NACK) {
		memcpy(msg->message.data, NACK_FRAME, sizeof(NACK_FRAME));
		msg->received_len = sizeof(NACK_FRAME);
	} else return -EBADMSG;

	msg->direction = DIRECTION_OUTGOING;
	reverse_bytes(msg->message.data, msg->received_len);
	return 0;
}

void reverse_bytes(u_int8_t *data, const unsigned int len)
{
	unsigned int pos = 0;
	while( (((unsigned int)data) & 0x3) != 0  && pos < len) {
		data[0] = REVERSE_BYTE(data[0]);
		data++;
		pos++;
	}

	if(len >= 4)
		while( pos < len-3 ) {
			((u_int32_t*)data)[0] = REVERSE_BYTE_W( ((u_int32_t*)data)[0] );
			data+=4;
			pos+=4;
		}

	while(pos < len) {
		data[0] = REVERSE_BYTE(data[0]);
		data++;
		pos++;
	}
}

int pn532_parse_frame(struct pn532_message_buffer *msg)
{
	if(msg == NULL) return -EINVAL;
	u_int8_t *inp = &(msg->message.data[msg->decoded_raw_len]);
	u_int8_t *outp = &(msg->message.data[msg->decoded_len]);
	const u_int8_t *endp = &(msg->message.data[msg->received_len]);
	u_int8_t *oendp = &(msg->message.data[msg->payload_len]);

	reverse_bytes(inp, msg->received_len - msg->decoded_raw_len);

#define NEED_BYTES(a) {if(inp+a >= endp) {msg->type = MESSAGE_TYPE_UNKNOWN; msg->state = MESSAGE_STATE_ABORTED; goto out; } }
#define GET_OFFSET(a) ( ((unsigned long)a)-((unsigned long)(&msg->message.data)) )

	int lastwaszero = 0;
	msg->additional_receive_len = 0;
	while(inp < endp) {
		switch(msg->state) {
		case MESSAGE_STATE_IN_PREAMBLE:
			if(*inp == 0) lastwaszero = 1;
			if(*inp == 0xff && lastwaszero) {
				msg->state = MESSAGE_STATE_IN_LEN;
			}
			break;
		case MESSAGE_STATE_IN_LEN:
			NEED_BYTES(1);
			if(inp[0] == 0 && inp[1] == 0xff) {
				msg->type = MESSAGE_TYPE_ACK;
				msg->payload_len = 0;
				msg->state = MESSAGE_STATE_IN_POSTAMBLE;
				inp++;
			} else if(inp[0] == 0xff && inp[1] == 0x00) {
				msg->type = MESSAGE_TYPE_NACK;
				msg->payload_len = 0;
				msg->state = MESSAGE_STATE_IN_POSTAMBLE;
				inp++;
			} else if(inp[0] == 0xff && inp[1] == 0xff) {
				msg->type = MESSAGE_TYPE_EXTENDED;
				inp+=2;
				NEED_BYTES(3);
				msg->payload_len = ((unsigned int)(inp[0])) * 256 + ((unsigned int)(inp[1]));
				if( ((inp[0] + inp[1] + inp[2]) & 0xff) != 0 ) {
					msg->type = MESSAGE_TYPE_UNKNOWN;
					msg->payload_len = 0;
					msg->state = MESSAGE_STATE_ABORTED;
				} else {
					msg->state = MESSAGE_STATE_IN_BODY;
					oendp = &(msg->message.data[msg->payload_len]);
				}
				inp+=2; /* Additional increment taking place below */
			} else {
				if(inp[0] == 0x01) {
					msg->type = MESSAGE_TYPE_ERROR;
				} else {
					msg->type = MESSAGE_TYPE_NORMAL;
				}
				msg->payload_len = inp[0];
				if( ((inp[0] + inp[1]) & 0xff) != 0 ) {
					msg->type = MESSAGE_TYPE_UNKNOWN;
					msg->payload_len = 0;
					msg->state = MESSAGE_STATE_ABORTED;
				} else {
					msg->state = MESSAGE_STATE_IN_BODY;
					oendp = &(msg->message.data[msg->payload_len]);
				}
				inp++;
			}
			break;
		case MESSAGE_STATE_IN_BODY:
			if(outp < oendp) {
				*(outp++) = inp[0];
				msg->message.checksum += inp[0];
			} else {
				msg->state = MESSAGE_STATE_IN_CHECKSUM;
				msg->message.checksum += inp[0];
				if(msg->message.checksum != 0) {
					msg->state = MESSAGE_STATE_ABORTED;
				} else {
					msg->state = MESSAGE_STATE_IN_POSTAMBLE;
				}
			}
			break;
		case MESSAGE_STATE_FREE: /* Can't happen */
		case MESSAGE_STATE_RESERVED: /* Can't happen */
		case MESSAGE_STATE_IN_CHECKSUM: /* Can't happen */
		case MESSAGE_STATE_ABORTED: /* Ignore further data */
		case MESSAGE_STATE_EXCEPTION: /* Ignore further data */
		case MESSAGE_STATE_IN_POSTAMBLE: /* After frame end, ignore further data */
			break;
		}
		inp++;
	}
out:
	msg->decoded_len = GET_OFFSET(outp);
	msg->decoded_raw_len = GET_OFFSET(inp);

	msg->additional_receive_len = 0;
	switch(msg->state) { /* All lengths are cumulative */
	case MESSAGE_STATE_IN_PREAMBLE:
		msg->additional_receive_len += 2;
	case MESSAGE_STATE_IN_LEN:
		msg->additional_receive_len += 5;
	case MESSAGE_STATE_IN_BODY:
		msg->additional_receive_len += msg->payload_len - msg->decoded_len;
	case MESSAGE_STATE_IN_CHECKSUM:
		msg->additional_receive_len += 1+1; /* One byte checksum, one byte postamble */
	default:
		break;
	}

	if(msg->state != MESSAGE_STATE_ABORTED) {
		return 0;
	} else {
		return -EBADMSG;
	}
}

int pn532_read_register(u_int16_t addr)
{
	const char cmd[] = {0xD4, 0x06, (addr>>8)&0xff, addr & 0xff};
	struct pn532_message_buffer *msg;
	int r = pn532_get_message_buffer(&msg);
	if(r != 0) return r;
	pn532_prepare_frame(msg, cmd, sizeof(cmd));
	pn532_send_frame(msg);
	pn532_put_message_buffer(&msg);
	return 0;
}

int pn532_write_register(u_int16_t addr, u_int8_t val)
{
	const char cmd[] = {0xD4, 0x08, (addr>>8)&0xff, addr & 0xff, val};
	const unsigned char match[] = {0xD5, 0x09};

	struct pn532_message_buffer *msg;
	struct pn532_wait_queue *queue;

	int r = _pn532_get_wait_queue(&queue, PN532_WAIT_MATCH, sizeof(match), match, 0);
	if(r != 0) return r;

	r = pn532_get_message_buffer(&msg);
	if(r != 0) { pn532_put_wait_queue(&queue); return r;}

	pn532_prepare_frame(msg, cmd, sizeof(cmd));
	pn532_send_frame(msg);
	pn532_put_message_buffer(&msg);

	if(pn532_recv_frame_queue(&msg, queue) == 0) {
#ifdef DEBUG_MESSAGE_FLOW
		printf("O\n");
#endif
		pn532_put_message_buffer(&msg);
		_pn532_put_wait_queue(&queue);
		return 0;
	} else {
		_pn532_put_wait_queue(&queue);
		return -EIO;
	}
}

static void pn532_enable_rx(u_int8_t enable, u_int8_t from_irq) {
	if(!from_irq) taskENTER_CRITICAL();
	pn532_rx_enabled = enable;
#ifndef PN532_EMULATE_FAKE_INTERRUPTS
	if(enable) pio_irq_enable(PN532_INT_PIO, PN532_INT_PIN);
	else pio_irq_disable(PN532_INT_PIO, PN532_INT_PIN);
#endif
	if(!from_irq) taskEXIT_CRITICAL();
}

/* A note on the PN532 operating mode: The PN532 offers three modes, the selection
 * is managed through P70_IRQ (pin 25 on the PN532) and P35 (pin 19 on the PN532)
 * on startup:
 *   P70_IRQ   P35   mode
 *   0         0     PN532
 *   1         1     PN532
 *   0         1     RFfieldON mode
 *   1         0     PN512 emulation
 *
 * On the OpenPICC2/txtr.mini P35 is NC (but can be connected to ground through R43),
 * P70_IRQ is connected to PA0, thereby pulled high by the automatic internal pullup.
 *
 * In order for the PN532 to enter the true PN532 mode both pins must have the same
 * level, this is (through sheer luck) the case by default: P35 is pulled high by
 * an internal pullup in the PN532 (this is only documented in AN10609: PN532 application
 * note, C106 appendix, page 8).
 * Alternatively it would be possible to connect P35 on the P532 to GND through R43, but
 * then it would enter PN512 emulation mode on power-on. In order to get it to switch
 * to true PN532 mode one would need to enable the pull-down on PA0 in the AT91SAM7, then
 * issue a reset to the PN532 through the NRST line of the AT91SAM's reset manager.
 */

int pn532_init(void)
{
	AT91F_PIO_CfgInput(PN532_INT_PIO, PN532_INT_PIN);
	AT91F_PIO_CfgPeriph(PN532_CS_PIO, PN532_CS_PIN, 0);
	if(spi_open(&pn532_spi, PN532_CS, AT91C_SPI_BITS_8 | AT91C_SPI_NCPHA |
			(SCBR << 8) | (1L << 16) | (0L << 24) ) < 0) {
		led_halt_blinking(6);
	}
	spi_set_flag(&pn532_spi, SPI_FLAG_DELAYED_CS_INACTIVE, 1);

	int i;
	for(i=0; i<PN532_NR_WAIT_QUEUES; i++) {
		pn532_wait_queues[i].message_queue = xQueueCreate(PN532_NR_MESSAGE_BUFFER, sizeof(struct pn532_message_buffer*));
	}

	vSemaphoreCreateBinary(pn532_rx_semaphore);
	xSemaphoreTake(pn532_rx_semaphore, 0);
	spi_set_callback(&pn532_spi, pn532_callback, 1);

	pio_irq_init_once();
#ifndef PN532_EMULATE_FAKE_INTERRUPTS
	pio_irq_register(PN532_INT_PIO, PN532_INT_PIN, pn532_irq);
#else
	(void)pn532_irq;
#endif

	pn532_enable_rx(0, 0);

	xTaskCreate (pn532_rx_task, (signed portCHAR *) "PN532 RX TASK", TASK_PN532_STACK,
			NULL, TASK_PN532_PRIORITY, NULL);
#ifdef PN532_EMULATE_FAKE_INTERRUPTS
	xTaskCreate (pn532_fakeirq_task, (signed portCHAR *) "PN532 FAKEIRQ TASK", 128,
			NULL, TASK_PN532_PRIORITY, NULL);
#endif

	return 0;
}
