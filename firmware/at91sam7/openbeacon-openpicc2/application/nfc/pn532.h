#ifndef PN532_H_
#define PN532_H_

extern int pn532_init(void);

/* Higher level functions (public) */
extern int pn532_read_spi_status_register(void);
extern int pn532_read_register(u_int16_t addr);
extern int pn532_write_register(u_int16_t addr, u_int8_t val);


/* FIXME: Maybe make the definitions below non-public as soon as a proper higher level abstraction is there */

#define PN532_MAX_PAYLOAD_LEN 265
#define PN532_MAX_FRAME_LEN (PN532_MAX_PAYLOAD_LEN+10)

enum pn532_message_type {
	MESSAGE_TYPE_UNKNOWN,
	MESSAGE_TYPE_NORMAL,
	MESSAGE_TYPE_EXTENDED,
	MESSAGE_TYPE_ACK,
	MESSAGE_TYPE_NACK,
	MESSAGE_TYPE_ERROR,
};

struct pn532_message_buffer {
	enum { DIRECTION_INCOMING, DIRECTION_OUTGOING} direction;
	u_int16_t bufsize; /* The raw size of the allocated data buffer, without SPI header */
	u_int16_t received_len; /* How many bytes have been received (without the one byte that is the SPI header) */
	u_int16_t additional_receive_len; /* How many bytes of the raw frame should be received additionally in order to form a complete frame */
	u_int16_t payload_len; /* How many bytes the payload is supposed to have */
	u_int16_t decoded_len; /* How may bytes of the payload have been decoded */
	u_int16_t decoded_raw_len; /* How may bytes of the raw frame have been decoded */
	enum pn532_message_type type;
	enum {
		MESSAGE_STATE_FREE=0,
		MESSAGE_STATE_RESERVED,
		MESSAGE_STATE_IN_PREAMBLE,
		MESSAGE_STATE_IN_LEN,
		MESSAGE_STATE_IN_BODY,
		MESSAGE_STATE_IN_CHECKSUM,
		MESSAGE_STATE_IN_POSTAMBLE,
		MESSAGE_STATE_EXCEPTION, /* Exception while receiving */
		MESSAGE_STATE_ABORTED, /* Parse error */
	} state;
	struct __attribute__((packed)) {
		u_int8_t checksum;
		u_int8_t filler[2];
		u_int8_t spi_header; /* aligned directly before a 4-byte boundary */
		u_int8_t data[PN532_MAX_FRAME_LEN]; /* 4 byte aligned */
	} message;
};

/* Wait for ACK or NACK */
#define PN532_WAIT_ACK 1
/* Wait for an error indication */
#define PN532_WAIT_ERROR 2
/* Wait for a content frame (normal or extended) */
#define PN532_WAIT_CONTENT 4
/* Wait for a specific match */
#define PN532_WAIT_MATCH 8
/* Catch the rest */
#define PN532_WAIT_CATCHALL 16

/* Utilities to handle message buffer structs (from the static field pn532_message_buffers) */
extern int pn532_get_message_buffer(struct pn532_message_buffer **msg);
extern int pn532_put_message_buffer(struct pn532_message_buffer **msg);

/* Utilities to handle message buffer contents */
extern int pn532_parse_frame(struct pn532_message_buffer *msg); /* (partially) parse from PN532 to host */
extern int pn532_unparse_frame(struct pn532_message_buffer *msg); /* Construct from host to PN532 (input data expected in msg->message.data) */
extern int pn532_prepare_frame(struct pn532_message_buffer *msg, const unsigned char *payload, unsigned int plen); /* Similar to pn532_unparse_frame, but doesn't need payload to be already stored in the buffer (e.g. from ROM) */

extern int pn532_send_frame(struct pn532_message_buffer *msg); /* Blockingly send a fully constructed message */
extern int pn532_recv_frame(struct pn532_message_buffer **msg, unsigned int wait_mask); /* Blockingly receive a message, caller is responsible for freeing the message buffer structure */
extern int pn532_recv_frame_match(struct pn532_message_buffer **msg, unsigned int wait_mask, unsigned char match_length, const unsigned char * const match_data); /* ditto, but with content match */

struct pn532_wait_queue;
extern int pn532_get_wait_queue(struct pn532_wait_queue **queue, unsigned int wait_mask, unsigned char match_length, const unsigned char * const match_data);
extern int pn532_put_wait_queue(struct pn532_wait_queue **queue);
extern int pn532_recv_frame_queue(struct pn532_message_buffer **msg, struct pn532_wait_queue *queue, portTickType wait_time);


#endif /*PN532_H_*/
