/*
 * pn532_libnfc_reader.c
 *
 *  Created on: 03.04.2009
 *      Author: henryk
 */

#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <beacontypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <task.h>
#include <USB-CDC.h>

#include <board.h>
#include <led.h>

#include "pn532.h"

#define LIBNFC_INTERNALS
#include "libnfc/libnfc.h"

/* Static declaration to avoid malloc related troubles and because we really only
 * support one instance anyway
 */
static dev_info pn532_dev_info;
static int already_open = 0;
static struct pn532_wait_queue *wait_queue = NULL;

//#define DEBUG_LIBNFC_GLUE

static dev_info* pn532_connect(const libnfc_driver_info_t driver_info, const ui32 device_index)
{
	(void)driver_info; (void)device_index;

	taskENTER_CRITICAL();

	if(already_open) return INVALID_DEVICE_INFO;

	if(pn532_get_wait_queue(&wait_queue, PN532_WAIT_CATCHALL, 0, NULL) != 0) {
#ifdef DEBUG_LIBNFC_GLUE
		printf("Error: Couldn't get wait queue\n");
#endif
		taskEXIT_CRITICAL();
		return INVALID_DEVICE_INFO;
	}

	/* We'll assume the PN532 code already got initialized in main and just set up pn532_dev_info */
	strncpy(pn532_dev_info.acName, driver_info->driver_name, sizeof(pn532_dev_info.acName));
	pn532_dev_info.acName[sizeof(pn532_dev_info.acName)-1] = 0;
	pn532_dev_info.ct = CT_PN532;
	pn532_dev_info.ds = 0;
	pn532_dev_info.bActive = true;
	pn532_dev_info.bCrc = true;
	pn532_dev_info.bPar = true;
	pn532_dev_info.ui8TxBits = 0;

	already_open = 1;
	taskEXIT_CRITICAL();

	return &pn532_dev_info;
}

static struct pn532_message_buffer *msg;

static const unsigned char hextable[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
void hexdump(const unsigned char *data, size_t length){ u_int32_t i=0;
	for(i=0;i<length; i++) { vUSBSendByte(hextable[data[i] >> 4]); vUSBSendByte(hextable[data[i] & 15]); vUSBSendByte(' '); }
}

static bool pn532_transceive(const dev_spec ds, const byte* tx, const ui32 tx_len, byte* rx, ui32* rx_len)
{
	(void)ds; (void)tx; (void)tx_len; (void)rx; (void)rx_len;

	/* Receive and discard all pending messages */
	while(pn532_recv_frame_queue(&msg, wait_queue, 0) == 0) {
		pn532_put_message_buffer(&msg);
#ifdef DEBUG_LIBNFC_GLUE
		printf("Warning: Discarded spurious message\n");
#endif
	}

	if(pn532_get_message_buffer(&msg) == 0) {
		pn532_prepare_frame(msg, tx, tx_len);
		if(pn532_send_frame(msg)==0) {
#ifdef DEBUG_LIBNFC_GLUE
			printf("\n\n");fflush(stdout);
			hexdump(tx, tx_len);
			printf("\nInfo: Command sent and ack'ed\n");
#endif
		} else {
#ifdef DEBUG_LIBNFC_GLUE
			printf("Error: Command not ack'ed\n");
			pn532_put_message_buffer(&msg);
			return false;
#endif
		}
		pn532_put_message_buffer(&msg);
	} else {
#ifdef DEBUG_LIBNFC_GLUE
		printf("Error: Couldn't get message buffer\n");
#endif
		return false;
	}

	size_t orig_rx_len;
	if(rx_len == NULL) orig_rx_len = 0; else orig_rx_len = *rx_len;

	/* Receive response */
	if(pn532_recv_frame_queue(&msg, wait_queue, portMAX_DELAY) == 0) {
		if( rx != NULL && rx_len != NULL && orig_rx_len >= msg->payload_len ) {
			if(msg->payload_len >= 2 && (msg->type == MESSAGE_TYPE_NORMAL || msg->type == MESSAGE_TYPE_EXTENDED) ) {
				memcpy(rx, msg->message.data+2, msg->payload_len-2);
				*rx_len = msg->payload_len-2;
			} else {
				memcpy(rx, msg->message.data, msg->payload_len);
				*rx_len = msg->payload_len;
			}
		}
#ifdef DEBUG_LIBNFC_GLUE
		hexdump(&msg->type, 1); vUSBSendByte(' ');
		hexdump(msg->message.data, msg->payload_len);
		vUSBSendByte('\n'); vUSBSendByte('\r');
#endif
		pn532_put_message_buffer(&msg);
	}

	return true;
}

static void pn532_disconnect(dev_info* pdi)
{
	(void)pdi;

	taskENTER_CRITICAL();
	pn532_put_wait_queue(&wait_queue);
	wait_queue = NULL;
	already_open = 0;
	taskEXIT_CRITICAL();
}

const struct libnfc_driver_info FIRMWARE_DRIVER_INFO = {
		.connect = pn532_connect,
		.transceive = pn532_transceive,
		.disconnect = pn532_disconnect,
		.driver_name = "Internal PN532 SPI connection",
};
