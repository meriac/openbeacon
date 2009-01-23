/*
 * picc_emu.c
 *
 *  Created on: 23.01.2009
 *      Author: henryk
 */

#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <beacontypes.h>

#include <task.h>
#include <USB-CDC.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "led.h"

#include "pn532.h"
#include "picc_emu.h"

#define DBG 0
#define DIE(args...) { if(DBG) printf(args); led_halt_blinking(7); }

const char target_init_cmd[] = {0xd4, 0x8c,
		0x04, /* PICC only */
		0x08, 0x00, /* ATQA */
		0x12, 0x34, 0x56, /* UID */
		0x20, /* SAK */
		0x01, 0xfe, 0xa2, 0xa3,
		0xa4, 0xa5, 0xa6, 0xa7,
		0xc0, 0xc1, 0xc2, 0xc3,
		0xc4, 0xc5, 0xc6, 0xc7,
		0xff, 0xff,
		0xaa, 0x99, 0x88,
		0x77, 0x66, 0x55, 0x44,
		0x33, 0x22, 0x11,
		0x00,
		0x02, 0x12, 0x34 /* 2 historical bytes of ATS: 12 34 */
}; // TgInitAsTarget

const char get_data_cmd[] = {0xd4, 0x86};

const char set_data_cmd[] = {0xd4, 0x8e};

const char get_status_cmd[] = {0xd4, 0x8a};

const unsigned char fixed_resp[] = {0x90, 0x00};

struct pn532_wait_queue *queue;

#define RETRIES 30

static int picc_init_target(void)
{
	int tries = 0;
	struct pn532_message_buffer *msg;

	if(DBG) printf("INF: Trying init\n");

	/* FIXME HACK HACK HACK */
	AT91C_BASE_RSTC->RSTC_RCR = 0xA5000008;
	vTaskDelay(10/portTICK_RATE_MS);

init_target_again:
	if( pn532_get_message_buffer(&msg) != 0) {
		DIE("ERR: Couldn't get message buffer\n");
	}

	pn532_prepare_frame(msg, target_init_cmd, sizeof(target_init_cmd));

	if(pn532_send_frame(msg)!=0) {
		if(tries++ < RETRIES) {
			pn532_put_message_buffer(&msg);
			goto init_target_again;
		}
		DIE("ERR: Couldn't TgInitAsTarget\n");
	}
	pn532_put_message_buffer(&msg);

	if(DBG) printf("INF: Boot ok\n");

	if(pn532_recv_frame_queue(&msg, queue, portMAX_DELAY) == 0) {
		if(DBG) {int i; for(i=0; i<msg->payload_len; i++) printf("%02X ", msg->message.data[i]); printf("\n");}
		if(DBG) printf("INF: Connection open\n");
		pn532_put_message_buffer(&msg);
	}

	return 0;
}

static int picc_get_data(unsigned char *data, unsigned int *length, portTickType wait_ticks)
{
	struct pn532_message_buffer *msg;
	int result;

	if(pn532_get_message_buffer(&msg) != 0) {
		DIE("ERR: Couldn't get message buffer (2)\n");
	}

	pn532_prepare_frame(msg, get_data_cmd, sizeof(get_data_cmd));
	if(pn532_send_frame(msg)!=0){
		DIE("ERR: Couldn't GetData\n");
	}
	if(DBG) printf("INF: Sent GetData\n");
	pn532_put_message_buffer(&msg);

	if(pn532_recv_frame_queue(&msg, queue, wait_ticks) == 0) {
		if(DBG) {int i; for(i=0; i<msg->payload_len; i++) printf("%02X ", msg->message.data[i]); printf("\n");}

		if(msg->message.data[1] != 0x87) {
			DIE("ERR: Strange response to GetData\n");
		}

		result = msg->message.data[2];
		unsigned int len = msg->payload_len - 3;
		if(len <= *length) {
			memcpy(data, msg->message.data + 3, len);
			*length = len;
		} else *length = 0;

		pn532_put_message_buffer(&msg);
	} else return -ETIMEDOUT;

	return result;
}

static int picc_put_data(const unsigned char * const data, unsigned int len, portTickType wait_ticks)
{
	struct pn532_message_buffer *msg;
	int result;

	if(pn532_get_message_buffer(&msg) != 0) {
		if(DBG) printf("ERR: Couldn't get message buffer (3)\n");
		return -ENOMEM;
	}


	memcpy(msg->message.data, set_data_cmd, sizeof(set_data_cmd));
	msg->payload_len = sizeof(set_data_cmd);

	memcpy(msg->message.data + msg->payload_len, data, len);
	msg->payload_len += len;

	pn532_prepare_frame(msg, (char*)(msg->message.data), msg->payload_len);

	if(pn532_send_frame(msg)!=0) {
		DIE("ERR: Couldn't SetData\n");
	}

	if(DBG) printf("INF: Sent response\n");

	pn532_put_message_buffer(&msg);

	if(pn532_recv_frame_queue(&msg, queue, wait_ticks) == 0) {
		if(msg->message.data[1] == 0x8F) {
			if(DBG) printf("INF: Response send ok\n");
			result = msg->message.data[2];
		} else {
			if(DBG) {int i; for(i=0; i<msg->payload_len; i++) printf("%02X ", msg->message.data[i]); printf("\n");}
			if(DBG) printf("ERR: Strange response to SetData\n");
			result = -EIO;
		}
		pn532_put_message_buffer(&msg);
	} else return -ETIMEDOUT;

	return 0;
}

static int __attribute__((unused)) picc_get_status(portTickType wait_ticks)
{
	struct pn532_message_buffer *msg;
	int result;

	if(pn532_get_message_buffer(&msg) != 0) {
		DIE("ERR: Couldn't get message buffer (4)\n");
	}

	pn532_prepare_frame(msg, get_status_cmd, sizeof(get_status_cmd));
	if(pn532_send_frame(msg)!=0){
		DIE("ERR: Couldn't GetTargetStatus\n");
	}
	pn532_put_message_buffer(&msg);

	if(pn532_recv_frame_queue(&msg, queue, wait_ticks) == 0) {
		if(DBG) {int i; for(i=0; i<msg->payload_len; i++) printf("%02X ", msg->message.data[i]); printf("\n");}

		if(msg->message.data[1] != 0x8B) {
			DIE("ERR: Strange response to GetTargetStatus\n");
		}

		result = msg->message.data[2];

		pn532_put_message_buffer(&msg);
	} else return -ETIMEDOUT;

	return result;
}

extern inline void
DumpUIntToUSB (unsigned int data);

static int readline(unsigned char * const line, unsigned int *linelen)
{
	unsigned int temp = 0;
	char b;
	unsigned int pos = 0;
	unsigned int len=0, havelen = 0, havebytes = 0;

	while(1) {
		vUSBRecvByte(&b, 1, portMAX_DELAY);
		//DumpUIntToUSB(b); printf("\n");
		switch(b) {
		case '0'...'9':
			temp <<=4;
			temp |= (b-'0');
			havebytes++;
			break;
		case 'a'...'f':
			temp <<=4;
			temp |= (b-'a'+10);
			havebytes++;
			break;
		case 'A'...'F':
			temp <<=4;
			temp |= (b-'A'+10);
			havebytes++;
			break;
		case ':':
			if(!havelen) {
				if(havebytes > 0) {
					len = temp;
					havelen = 1;
					temp = 0;
					havebytes = 0;
				}
			} else goto treat_colon_as_whitespace;
			break;
		case ' ': case '\t': case '\v': case '\r': case '\n':
treat_colon_as_whitespace:
			if(havelen) {
				if(havebytes > 0) {
					line[pos++] = temp;
					temp = 0;
					havebytes = 0;
				}
				if(pos > *linelen)
					goto readline_end;
			}
			if(b == '\r')
				goto readline_end;
			break;
		}
	}

readline_end:
	*linelen = pos;
	return 0;
}

static int handle_apdu(const unsigned char * const command, unsigned int cmdlen,
		unsigned char * const response, unsigned int *resplen)
{
	unsigned int i;
	printf("%04X:", cmdlen);
	for(i=0; i<cmdlen; i++) {
		printf(" %02X", command[i]);
	}
	printf("\n");

#if 0
	memcpy(response, fixed_resp, sizeof(fixed_resp));
	*resplen = sizeof(fixed_resp);
#else
	readline(response, resplen);
#endif
	return 0;
}

static int reset_state(void)
{
	printf("RESET\n");
	return 0;
}

unsigned char buffer[270];

void picc_emu_task(void *parameter)
{
	(void)parameter;
	vTaskDelay(500/portTICK_RATE_MS);

	if(pn532_get_wait_queue(&queue, PN532_WAIT_CONTENT, 0, NULL) != 0) {
		DIE("ERR: Couldn't get wait queue\n");
	}

	reset_state();
	picc_init_target();

	while(1) {
		unsigned int buflen = sizeof(buffer);
		int r = picc_get_data(buffer, &buflen, portMAX_DELAY);

		if(r == 0) {
			if(DBG) printf("INF: Received APDU\n");

			unsigned int cmdlen = buflen, resplen = sizeof(buffer);
			handle_apdu(buffer, cmdlen, buffer, &resplen);

			r = picc_put_data(buffer, resplen, portMAX_DELAY);
			if(r == 0) {
				if(DBG) printf("INF: Sent APDU\n");
			} else if(r == 0x29) {
				if(DBG) printf("INF: Target lost (2)\n");
				reset_state();
				picc_init_target();
			} else {
				DIE("ERR: Unknown error (2) %i\n", r);
			}
		} else if(r==0x29) {
			if(DBG) printf("INF: Target lost\n");
			reset_state();
			picc_init_target();
		} else if(r == -ETIMEDOUT) {
			/* Ignore, retry */
		} else {
			DIE("ERR: Unknown error %i\n", r);
		}

	}

}

void picc_emu_init(void)
{
	xTaskCreate (picc_emu_task, (signed portCHAR *) "PICC EMUTASK", TASK_PN532_STACK, NULL, TASK_PN532_PRIORITY, NULL);
}
