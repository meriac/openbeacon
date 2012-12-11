/***************************************************************
 *
 * OpenBeacon.org - main file for OpenPCD2 basic demo
 *
 * Copyright 2010 Milosch Meriac <meriac@openbeacon.de>
 *
 ***************************************************************

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

 */
#include <openbeacon.h>
#include "main.h"
#include "usbserial.h"

#define ISO14443A_CARRIER 13560000UL
#define ISO14443A_ETU(etu) ((uint32_t)((128ULL*SYSTEM_CORE_CLOCK*etu)/ISO14443A_CARRIER))
#define ETU_TOLERANCE 10
#define MAX_EDGES 1024
#define MAX_PACKET 32

#define STATE_WAITREQA 0

#define ERR_EOF 0x80
#define ERR_SHORT_PULSE 0x81
#define ERR_INVALID_SEQUENCE 0x82
#define ERR_INVALID_BIN 0x83

static uint16_t g_counter_prev = 0;
static uint8_t g_counter_overflow = 0;

static uint8_t g_buffer[MAX_EDGES];
static int g_buffer_pos;

static void
rfid_hexdump (const void *buffer, int size)
{
	int i;
	const unsigned char *p = (unsigned char *) buffer;

	for (i = 0; i < size; i++)
	{
		if (i && ((i & 3) == 0))
			debug_printf (" ");
		debug_printf (" %02X", *p++);
	}
	debug_printf (" [size=%02i]\n", size);
}

uint16_t rfid_crc16 (const uint8_t* data, uint16_t length)
{
	uint8_t u;
	uint16_t crc = 0x6363;

	while(length--)
	{
		u = *data++;
		u ^= (uint8_t)crc;
		u ^= u << 4;
		crc = (crc>>8)^(((uint16_t)u)<<8)^(((uint16_t)u)<<3)^(u>>4);
	}

	return crc;
}

static void rfid_decode_packet(const uint8_t *data, uint8_t length)
{
	GPIOSetValue (LED_PORT, LED_BIT, LED_ON);
#if 0
	switch(*data)
	{
		0x26: /* REQA */
			if(length==1)
			{
			}
			break;
		0x52: /* WUPA */
			if(length==1)
			{
			}
			break;
	}
#endif
	if((length+g_buffer_pos)<MAX_EDGES)
	{
		memcpy(&g_buffer[g_buffer_pos], data, length);
		g_buffer_pos = (g_buffer_pos+0x10)&~0xFUL;
	}
}

static void rfid_decode_bit(uint8_t bit)
{
	static uint8_t buffer[MAX_PACKET];
	static uint8_t bitpos = 0, data = 0, buffer_pos = 0;
	uint16_t crc;
	uint8_t parity;

	if(bit & 0x80)
	{
		if(bit == ERR_EOF)
		{
			if(bitpos==7)
			{
				buffer[0] = data;
				rfid_decode_packet(buffer, 1);
			}
			else
				if(buffer_pos>2)
				{
					crc = rfid_crc16(buffer, buffer_pos-2);
					if((buffer[buffer_pos-2] == (uint8_t)crc) && (buffer[buffer_pos-1] == (uint8_t)(crc>>8)))
						rfid_decode_packet(buffer, buffer_pos);
				}
				else
					if(buffer_pos)
						rfid_decode_packet(buffer, buffer_pos);
		}
		data = bitpos = buffer_pos = 0;
	}
	else
	{
		/* gathered a full byte */
		if(bitpos == 8)
		{
			/*parity calculation */
			parity = data ^ (data>>4);
			parity ^= parity>>2;
			parity ^= parity>>1;

			if((parity & 1)!=bit)
			{
				if(buffer_pos<MAX_PACKET)
					buffer[buffer_pos++] = data;
			}
			else
				buffer_pos = 0;

			data = bitpos = 0;
		}
		else
		{
			data >>= 1;
			if(bit)
				data|=0x80;
			bitpos++;
		}
	}
}

static void rfid_decode_miller(uint8_t etu)
{
	static uint8_t bit = 0;

	switch(etu)
	{
		/* filter spurious signals */
		case 0:
			rfid_decode_bit (ERR_SHORT_PULSE);
			bit = 0;
			break;

		case 4:
			rfid_decode_bit (bit);
			break;

		case 6:
			if(bit)
			{
				rfid_decode_bit (0);
				rfid_decode_bit (0);
				bit = 0;
			}
			else
			{
				rfid_decode_bit (1);
				bit = 1;
			}
			break;

		case 8:
			if(bit)
			{
				rfid_decode_bit (0);
				rfid_decode_bit (1);
			}
			else
				rfid_decode_bit (ERR_EOF);
			break;

		default:
			rfid_decode_bit((etu>8) ? ERR_EOF : ERR_INVALID_BIN);
			bit = 0;
			break;
	}
}

void TIMER16_1_IRQHandler(void)
{
	uint8_t reason;
	uint16_t counter;
	uint32_t diff;

	/* get interrupt reason */
	reason = (uint8_t)LPC_TMR16B1->IR;

	GPIOSetValue (LED_PORT, LED_BIT, LED_OFF);

	/* if overflow - reset pulse length detection */
	if(reason & 0x01)
	{
		rfid_decode_miller(0xFF);
		LPC_TMR16B1->MCR = 0;
		g_counter_overflow=TRUE;
	}
	else
		/* we captured an edge */
		if(reason & 0x10)
		{
			/* get captured value */
			counter = LPC_TMR16B1->CR0;
			/* update overflow register */
			LPC_TMR16B1->MR0 = counter+ISO14443A_ETU(3);

			/* ignore first value after overflow */
			if(g_counter_overflow)
				g_counter_overflow=FALSE;
			else
			{
				/* calculate difference */
				diff = (4UL*(counter-g_counter_prev)+ISO14443A_ETU(0.5))/ISO14443A_ETU(1);
				rfid_decode_miller((diff>0xFF)?0xFF:(uint8_t)diff);
			}
			/* remember current counter value */
			g_counter_prev=counter;

			LPC_TMR16B1->MCR = 1;
		}

	/* acknowledge IRQ reason */
	LPC_TMR16B1->IR = reason;
}

static inline void
rfid_init_emulator (void)
{
	/* enable timer 32B0 & 16B1 */
	LPC_SYSCON->SYSAHBCLKCTRL |= EN_CT32B0|EN_CT16B1;

	/* reset and stop counter 16B0 */
	LPC_TMR16B1->TCR = 0x02;
	LPC_TMR16B1->TCR = 0;
	LPC_TMR16B1->TC = 0;
	LPC_TMR16B1->CTCR = 0;
	/* match on MR0 for overflow detection */
	LPC_TMR16B1->MR0 = 0;
	LPC_TMR16B1->MCR = 0;
	LPC_TMR16B1->EMR = 0;
	LPC_TMR16B1->PWMC = 0;
	/* Capture rising edge & trigger interrupt */
	LPC_TMR16B1->CCR = 5;
	/* disable prescaling */
	LPC_TMR16B1->PR = 0;
	/* enable CT16B1_CAP0 for signal duration capture */
	LPC_IOCON->PIO1_8 = 0x1;
	/* enable timer capture interrupt */
	NVIC_EnableIRQ(TIMER_16_1_IRQn);
	NVIC_SetPriority(TIMER_16_1_IRQn, 1);
	/* reset previous counter value */
	g_counter_prev = g_counter_overflow = 0;
	/* run counter */
	LPC_TMR16B1->TCR = 0x01;

	/* reset and stop counter 32B0 */
	LPC_TMR32B0->TCR = 0x02;
	LPC_TMR32B0->TCR = 0x00;
	/* enable CT32B0_CAP0 timer clock */
	LPC_IOCON->PIO1_5 = 0x02;
	/* no capturing */
	LPC_TMR32B0->CCR = 0x00;
	/* no match */
	LPC_TMR32B0->MCR = 0;
	LPC_TMR32B0->EMR = 0;
	LPC_TMR32B0->PWMC = 0;
	/* incremented upon CAP0 input rising edge */
	LPC_TMR32B0->CTCR = 0x01;
	/* disable prescaling */
	LPC_TMR32B0->PR = 0;
	/* run counter */
	LPC_TMR32B0->TCR = 0x01;

	/* enable RxMultiple mode */
	rfid_write_register (0x6303, 0x0C);
	/* disable parity for TX/RX */
	rfid_write_register (0x630D, 0x10);

	/* enable SVDD switch */
	rfid_write_register (0x6306, 0x8F);
	rfid_write_register (0x6307, 0x03);
	rfid_write_register (0x6106, 0x10);
	/* enable secure clock */
	rfid_write_register (0x6330, 0x80);
	rfid_write_register (0x6104, 0x00);

	/* select test bus signal */
	rfid_write_register (0x6321, 0x30|5);
	/* select test bus type */
	rfid_write_register (0x6322, 27);
	rfid_write_register (0x6328, 0xFC);

	/* WTF? FIXME !!! */
	LPC_SYSCON->SYSAHBCLKCTRL |= EN_CT32B0|EN_CT16B1;
}

static void
loop_rfid (void)
{
	int res, line;
	uint8_t test_signal, old_test_signal, signal, bus;
	static unsigned char data[80];

	/* fully initialized */
	GPIOSetValue (LED_PORT, LED_BIT, LED_ON);

	/* read firmware revision */
	debug_printf ("\nreading firmware version...\n");
	data[0] = PN532_CMD_GetFirmwareVersion;
	while ((res = rfid_execute (&data, 1, sizeof (data))) < 0)
	{
		debug_printf ("Reading Firmware Version Error [%i]\n", res);
		pmu_wait_ms (450);
		GPIOSetValue (LED_PORT, LED_BIT, LED_ON);
		pmu_wait_ms (10);
		GPIOSetValue (LED_PORT, LED_BIT, LED_OFF);
	}

	debug_printf ("PN532 Firmware Version: ");
	if (data[1] != 0x32)
		rfid_hexdump (&data[1], data[0]);
	else
		debug_printf ("v%i.%i\n", data[2], data[3]);

	/* Set PN532 to virtual card */
	data[0] = PN532_CMD_SAMConfiguration;
	data[1] = 0x02;																/* Virtual Card Mode */
	data[2] = 0x00;																/* No Timeout Control */
	data[3] = 0x00;																/* No IRQ */
	if ((res = rfid_execute (&data, 4, sizeof (data))) > 0)
	{
		debug_printf ("SAMConfiguration: ");
		rfid_hexdump (&data, res);
	}

	/* init RFID emulator */
	rfid_init_emulator ();

	/* enable debug output */
	GPIOSetValue (LED_PORT, LED_BIT, LED_ON);
	line = 0;

	signal = bus = old_test_signal = 0;
	test_signal = (27<<3)|5;

	while (1)
	{
		if (fifo_in_count>0)
		{
			switch (fifo_in[0])
			{
				case 'm':
					debug_printf("Modulation=%c\n",(line&1)?'1':'0');
					GPIOSetValue (0, 11, line&1);
					line++;
					break;
				case 'D':
				case 'd':
					debug_printf("Dumping %u bytes.\n", g_buffer_pos);
					hex_dump (g_buffer, 0, g_buffer_pos);
					g_buffer_pos = 0;
					memset(g_buffer,0,sizeof(g_buffer));
					break;
				case '+':
					test_signal++;
					break;
				case '-':
					test_signal--;
					break;
				case 'B':
					/* increment U.FL test bus number */
					test_signal = (test_signal & ~0x7) + (1 << 3);
					break;
				case 'b':
					/* decrement U.FL test bus number */
					test_signal = (test_signal & ~0x7) - (1 << 3);
					break;
				case '0':
					test_signal = 0;
					break;
			}
			UARTCount = 0;
		}

		if (test_signal != old_test_signal)
		{
			old_test_signal = test_signal;

			/* enable test singal output on U.FL sockets */
			signal = test_signal & 0x7;
			bus = (test_signal >> 3) & 0x1F;

			rfid_write_register (0x6328, 0xF9);
			/* select test bus signal */
			rfid_write_register (0x6321, 0x30|signal);
			/* select test bus type */
			rfid_write_register (0x6322, bus);

			/* display current test signal ID */
			debug_printf ("TEST_SIGNAL_ID: %02i.%i\n", bus, signal);
		}
	}
}

int
main (void)
{
	g_buffer_pos = 0;

	/* Initialize GPIO (sets up clock) */
	GPIOInit ();

	/* Set LED port pin to output */
	GPIOSetDir (LED_PORT, LED_BIT, 1);
	GPIOSetValue (LED_PORT, LED_BIT, LED_OFF);

	LPC_IOCON->JTAG_TDI_PIO0_11 = 0x81;
	GPIOSetDir (0, 11, 1);
	GPIOSetValue (0, 11, 0);

	/* Init Power Management Routines */
	pmu_init ();

	/* Init USB serial port */
	init_usbserial ();

	/* Init RFID */
	rfid_init ();

	/* RUN RFID loop */
	loop_rfid ();

	return 0;
}
