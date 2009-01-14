#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <beacontypes.h>

#include <task.h>

#include <stdio.h>

#include "pn532.h"

void pn532_demo_task(void *parameter)
{
	(void)parameter;
	vTaskDelay(3000/portTICK_RATE_MS);
	printf("Here\n");
	vTaskDelay(1000/portTICK_RATE_MS);
	
	struct pn532_message_buffer *msg;
	if( pn532_get_message_buffer(&msg) != 0) {
		printf("Couldn't get message buffer\n");
		while(1) vTaskDelay(1000);
	}
	
	{
#if 0
		const char cmd[] = {0xd4, 0x8c, 
				0x04,
				0x80, 0x00,
				0x12, 0x34, 0x56,
				0x60,
				0x01, 0xfe, 0xa2, 0xa3,
				0xa4, 0xa5, 0xa6, 0xa7,
				0xc0, 0xc1, 0xc2, 0xc3,
				0xc4, 0xc5, 0xc6, 0xc7,
				0xff, 0xff,
				0xaa, 0x99, 0x88,
				0x77, 0x66, 0x55, 0x44,
				0x33, 0x22, 0x11,
				0x00,
				0x00}; // TgInitAsTarget
#elif 0
		const char cmd[] = { 0xd4, 0x4a, 0x02, 0x00}; // InListPassiveTarget
#elif 0
		const char cmd[] = { 0xd4, 0x60, 0xff, 0x01, 0x10}; // InAutoPoll
#elif 1
		const char cmd[] = {0xd4, 0x14, 0x02, 0x00 }; // SAMconfiguration: virtual card
#elif 0
		const char cmd[] = {0xd4, 0x04}; // GetGeneralStatus
#else
		const char cmd[] = {0xD4, 0x02}; // GetFirmwareVersion
#endif
		pn532_prepare_frame(msg, cmd, sizeof(cmd));
	}
	pn532_send_frame(msg);
	pn532_put_message_buffer(&msg);
	
	vTaskDelay(1000/portTICK_RATE_MS);
	pn532_write_register(0x6330, 0x80); vTaskDelay(150);
	pn532_write_register(0x6306, (2<<6) | (0<<4) | (0xf)); vTaskDelay(150);
	
	while(1) {
#if 0
		vTaskDelay(1000/portTICK_RATE_MS);
		pn532_read_register(0x6306);
		vTaskDelay(150);
		pn532_write_register(0x6330, 0x80);
		vTaskDelay(150);
		pn532_write_register(0x6306, 0x2E);
		vTaskDelay(150);
		pn532_write_register(0x6303, (5<<4) | (1<<3) | (1<<2) | 0);
		vTaskDelay(150);
		pn532_write_register(0x630D, (1<<2) | 0);
		vTaskDelay(150);
		pn532_write_register(0x6321, 2<<4);
		vTaskDelay(150);
		pn532_write_register(0x6308, (1<<4) | (4<<0));
		vTaskDelay(150);
		pn532_write_register(0x6328, 5);
#endif
		vTaskDelay(1500);
	}
}
