/***************************************************************
 *
 * OpenBeacon.org - main entry for 2.4GHz RFID USB reader
 *
 * Copyright 2007 Milosch Meriac <meriac@openbeacon.de>
 *
 * basically starts the USB task, initializes all IO ports
 * and introduces idle application hook to handle the HF traffic
 * from the nRF24L01 chip
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
/* Library includes. */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <USB-CDC.h>
#include <task.h>
#include <beacontypes.h>
#include <board.h>
#include <dbgu.h>
#include <led.h>
#include <task.h>

#include "proto.h"
#include "power.h"
#include "sdram.h"
#include "eink/eink.h"
#include "eink/eink_flash.h"
#include "eink/eink_lowlevel.h"
#include "touch/ad7147.h"
#include "touch/slider.h"
#include "accelerometer.h"
#include "dosfs/dosfs.h"
#include "nfc/pn532.h"
#include "nfc/pn532_demo.h"
#include "nfc/picc_emu.h"
#include "nfc/libnfc_demo.h"
#include "ebook/ebook.h"
#include "paint/paint.h"

static uint8_t sector[SECTOR_SIZE],sector1[SECTOR_SIZE],buffer[SECTOR_SIZE];

/**********************************************************************/
static inline void
prvSetupHardware (void)
{
  /*  When using the JTAG debugger the hardware is not always initialised to
     the correct default state.  This line just ensures that this does not
     cause all interrupts to be masked at the start. */
  AT91C_BASE_AIC->AIC_EOICR = 0;

  /*  Enable the peripheral clock. */
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOA;
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOB;
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOC;
}

/**********************************************************************/
void
vApplicationIdleHook (void)
{
#if 1
	/* Disable SDRAM clock and send SDRAM to self-refresh. The SDRAM will be
	 * automatically reenabled on the next access.
	 */
	AT91C_BASE_SDRC->SDRC_SRR = 1;
	/* Disable processor clock to set the core into Idle Mode. The clock will 
	 * automatically be reenabled by any interrupt. */
	AT91C_BASE_PMC->PMC_SCDR = 1;
#endif
}

void
watchdog_restart_task (void *parameter)
{
	(void) parameter;
	while (1) {
		/* Restart watchdog, has been enabled in Cstartup_SAM7.c */
		AT91F_WDTRestart (AT91C_BASE_WDTC);
		vTaskDelay (500 / portTICK_RATE_MS);
	}
}

void
demo_task (void *parameter)
{
	(void) parameter;
	
	led_set_red (1);
	vTaskDelay (500 / portTICK_RATE_MS);
	led_set_red (0);
	
	while (1) {
		vTaskDelay (1000 / portTICK_RATE_MS);
	}
}

/**********************************************************************/

static inline unsigned char
HexChar (unsigned char nibble)
{
	return nibble + ((nibble < 0x0A) ? '0' : ('A' - 0xA));
}

/**********************************************************************/

static void do_sdram_read_test(volatile unsigned int *base, unsigned int size)
{
	long start, stop;

	int ok = 1;
	unsigned int i;
	
	start = xTaskGetTickCount();
	for (i = 0; i < (size/4) / 8; i++)
	{
		int j = rand() % ((size/4) - 8);
		register int i0 asm("r0"),
			i1 asm("r1"),
			i2 asm("r2"),
			i3 asm("r3"),
			i4 asm("r4"),
			i5 asm("r5"),
			i6 asm("r6"),
			i7 asm("r7");

		asm volatile("LDM %[base], {%0-%7}\n\t"
				"MOV r9, %[index]\n\t"
				"EOR %0, %0, %[beef]\n\t"
				"SUB %0, %0, r9\n\r"
				"ADD r9, r9, #1\n\t"
				"EOR %1, %1, %[beef]\n\t"
				"SUB %1, %1, r9\n\r"
				"ADD r9, r9, #1\n\t"
				"EOR %2, %2, %[beef]\n\t"
				"SUB %2, %2, r9\n\r"
				"ADD r9, r9, #1\n\t"
				"EOR %3, %3, %[beef]\n\t"
				"SUB %3, %3, r9\n\r"
				"ADD r9, r9, #1\n\t"
				"EOR %4, %4, %[beef]\n\t"
				"SUB %4, %4, r9\n\r"
				"ADD r9, r9, #1\n\t"
				"EOR %5, %5, %[beef]\n\t"
				"SUB %5, %5, r9\n\r"
				"ADD r9, r9, #1\n\t"
				"EOR %6, %6, %[beef]\n\t"
				"SUB %6, %6, r9\n\r"
				"ADD r9, r9, #1\n\t"
				"EOR %7, %7, %[beef]\n\t"
				"SUB %7, %7, r9\n\r"
				
				: "=&r" (i0), "=&r" (i1), "=&r" (i2), "=&r" (i3),
					"=&r" (i4), "=&r" (i5), "=&r" (i6), "=&r" (i7)
				: [base] "r" (base + j),
					[beef] "r" (0xbeef0000), 
					[index] "r" (j)
				: "r9");
		if(i0 != 0 || i1 != 0 || i2 != 0 || i3 != 0 || i4 != 0 || i5 != 0 || i6 != 0 || i7 != 0)
		{
			if(ok) printf("Error at %i: %08X %08X %08X %08X %08X %08X %08X %08X\n", j, 
				i0, i1, i2, i3, i4, i5, i6, i7);
			ok = 0;
		}
	}
	stop = xTaskGetTickCount();
	led_set_red (0);
	led_set_green (0);
	vTaskDelay (100 / portTICK_RATE_MS);
	led_set_red (!ok);
	led_set_green (ok);
	if (ok)
		printf ("SDRAM ok (%li ticks)\n", (long)(stop-start));

}

static void do_sdram_write_test(volatile unsigned int *base, unsigned int size)
{
	long start = xTaskGetTickCount(), stop;
	volatile unsigned int *tmp = base;
	unsigned int i;
	
	for (i = 0; i < (size/4) / 8; i++) {
		asm volatile("MOV r9, %[index]\n\t"
				"EOR r0, r9, %[beef]\n\t"
				"ADD r9, r9, #1\n\t"
				"EOR r1, r9, %[beef]\n\t"
				"ADD r9, r9, #1\n\t"
				"EOR r2, r9, %[beef]\n\t"
				"ADD r9, r9, #1\n\t"
				"EOR r3, r9, %[beef]\n\t"
				"ADD r9, r9, #1\n\t"
				"EOR r4, r9, %[beef]\n\t"
				"ADD r9, r9, #1\n\t"
				"EOR r5, r9, %[beef]\n\t"
				"ADD r9, r9, #1\n\t"
				"EOR r6, r9, %[beef]\n\t"
				"ADD r9, r9, #1\n\t"
				"EOR r7, r9, %[beef]\n\t"
				"STM %[base]!, {r0-r7}\n\t"
				: [base] "+r" (tmp)
				: [beef] "r" (0xbeef0000), 
					[index] "r" (i*8)
				: "memory", "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r9");
		
	}
	stop = xTaskGetTickCount();
	printf("Loaded test pattern in %li ticks\n", (long)(stop-start));
	
	start = xTaskGetTickCount();
	tmp = base;
	for (i = 0; i < (size/4) / 8; i++) {
		asm volatile( "LDM %0!, {r0-r7}\n\t"
				: "+r" (tmp)
				:
				: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7");
		
	}
	stop = xTaskGetTickCount();
	printf("Read test pattern in %li ticks\n", (long)(stop-start));
}


void
sdram_test_task (void *parameter)
{
	int extra_wait = (int)parameter;
	volatile int i;
	int offset = 1024*1024; /* Leave some room for malloc, e.g. the Task Control Blocks */
	volatile unsigned int *pSdram = (unsigned int *) (SDRAM_BASE + offset);
	
	if(extra_wait == 0) {
		vTaskDelay (1000 / portTICK_RATE_MS);
		printf("Starting SDRAM test for %s\n", BOARD->friendly_name);
		
		for(i=0; i<4; i++) {
			SDRAM_BASE[offset+i] = i;
		}
		
		if(pSdram[0] == 0x03020100) {
			printf("Byte-wise SDRAM access OK\n");
		} else {
			printf("Byte-wise SDRAM access not OK\n");
		}
		
		vTaskDelay(100 / portTICK_RATE_MS);
		
		do_sdram_write_test(pSdram, SDRAM_SIZE - offset);
	} else {
		vTaskDelay(extra_wait/portTICK_RATE_MS);
	}
	
	while (1) {
		do_sdram_read_test(pSdram, SDRAM_SIZE - offset);
	}
}

void eink_traffic_test_task (void *parameter)
{
	(void)parameter;
	vTaskDelay(10000/portTICK_RATE_MS);
	printf("Fire in the hole\n");
	
	while(1) {
		/* Don't try this at home, kids! */
		eink_display_streamed_image_update((unsigned char*)SDRAM_BASE, SDRAM_SIZE);
		printf("... Ok\n");
		vTaskDelay(5000/portTICK_RATE_MS);
	}
}

void
hex_dump (const unsigned char *buf, unsigned int addr, unsigned int len)
{
	unsigned int start, i, j;
	char c;
	
	start = addr & ~0xf;
	
	for (j = 0; j < len; j += 16) {
		printf ("%08x:", start + j);
		
		for (i = 0; i < 16; i++) {
			if (start + i + j >= addr && start + i + j < addr + len) {
				printf (" %02x", buf[start + i + j]);
			} else {
				printf ("   ");
			}
		}
		printf ("  |");
		for (i = 0; i < 16; i++) {
			if (start + i + j >= addr && start + i + j < addr + len) {
				c = buf[start + i + j];
				if (c >= ' ' && c < 127) {
					printf ("%c", c);
				} else {
					printf (".");
				}
			} else {
				printf (" ");
			}
		}
		printf ("|\n");
	}
}

void sdcard_test_task (void *parameter)
{
	u_int32_t pstart, psize, i,t;
	u_int8_t pactive, ptype;
	VOLINFO vi;
	DIRINFO di;
	DIRENT de;
	FILEINFO fi;
	//  u_int32_t cache;
	int res;
	
	(void) parameter;
	
	
	vTaskDelay (5000 / portTICK_RATE_MS);
	printf ("Here we go\n");
	
	if ((res = DFS_OpenCard ()) != 0) {
		printf ("Can't open SDCARD [%i]\n", res);
		while (1)
			vTaskDelay (100 / portTICK_RATE_MS);
	} else {
		// Obtain pointer to first partition on first (only) unit
		pstart = DFS_GetPtnStart (0, sector, 0, &pactive, &ptype, &psize);
		if (pstart == 0xffffffff) {
			printf ("Cannot find first partition\n");
			hex_dump(sector,0,sizeof(sector));
			while(1) vTaskDelay (100 / portTICK_RATE_MS);
		}
		
		printf ("Partition 0 start sector 0x%08X active 0x%02X type 0x%02X size 0x%08X\n",
				(unsigned int) pstart, (unsigned int) pactive, (unsigned int) ptype,
				(unsigned int) psize);
		
		if (DFS_GetVolInfo (0, sector, pstart, &vi)) {
			printf ("Error getting volume information\n");
			while(1) vTaskDelay (100 / portTICK_RATE_MS);
		}
		printf ("Volume label '%-11.11s'\n", vi.label);
		printf ("%d sector/s per cluster, %d reserved sector/s, volume total %d sectors.\n",
				(int) vi.secperclus, (int) vi.reservedsecs, (int) vi.numsecs);
		printf ("%d sectors per FAT, first FAT at sector #%d, root dir at #%d.\n",
				(int) vi.secperfat, (int) vi.fat1, (int) vi.rootdir);
		printf ("(For FAT32, the root dir is a CLUSTER number, FAT12/16 it is a SECTOR number)\n");
		printf ("%d root dir entries, data area commences at sector #%d.\n",
				(int) vi.rootentries, (int) vi.dataarea);
		printf ("%d clusters (%d bytes) in data area, filesystem IDd as ",
				(int) vi.numclusters,
				(int) vi.numclusters * vi.secperclus * SECTOR_SIZE);
		
		if (vi.filesystem == FAT12) {
			printf ("FAT12.\n");
		} else if (vi.filesystem == FAT16) {
			printf ("FAT16.\n");
		} else if (vi.filesystem == FAT32) {
			printf ("FAT32.\n");
		} else {
			printf ("[unknown]\n");
		}
		
		di.scratch = sector1;
		if (DFS_OpenDir (&vi, (uint8_t *) "", &di)) {
			printf ("Error opening root directory\n");
			while(1) vTaskDelay (100 / portTICK_RATE_MS);
		}
		
		while (!DFS_GetNext (&vi, &di, &de)) {
			if (de.name[0]) {
				printf ("file: '%-11.11s'\n", de.name);
				printf ("read test:\n");
				if (DFS_OpenFile (&vi, de.name, DFS_READ, sector, &fi)) {
					printf ("error opening file\n");
				} else {
					t = fi.filelen;
					if(t>SECTOR_SIZE)
						t=SECTOR_SIZE;
					DFS_ReadFile (&fi, sector, buffer, &i, t);
					hex_dump(buffer,0,i);
				}
			}
		}
		
		
		printf("DONE\n");
		while(1) vTaskDelay (100 / portTICK_RATE_MS);
	}
}

extern int eink_comm_test (u_int16_t reg1, u_int16_t reg2);
extern u_int16_t eink_read_register (u_int16_t reg);
#define READ_SIZE (256*1024)
void
flash_demo_task (void *parameter)
{
	(void) parameter;
	vTaskDelay (3000 / portTICK_RATE_MS);
	printf ("Here\n");
	
	int r = eink_flash_acquire ();
	if (r >= 0) {
		unsigned int ident = eink_flash_read_identification ();
		printf ("Device identification: %06X\n", ident);
		
		printf ("Starting read\n");
		eink_flash_read (0, (char *) SDRAM_BASE, READ_SIZE);
		eink_flash_release ();
		printf ("Dunnit\n");
		
	} else {
		switch ((enum eink_error) -r) {
		case EINK_ERROR_NOT_DETECTED:
			printf ("eInk controller not detected\n");
			break;
		case EINK_ERROR_NOT_SUPPORTED:
			printf ("eInk controller not supported (unknown revision)\n");
			break;
		case EINK_ERROR_COMMUNICATIONS_FAILURE:
			printf
			("eInk controller communications failure, check FPC cable and connector\n");
			break;
		case EINK_ERROR_CHECKSUM_ERROR:
			printf ("eInk controller waveform flash checksum failure\n");
			break;
		default:
			printf ("eInk controller initialization: unknown error\n");
			break;
		}
		if (eink_comm_test (0x148, 0x156)) {
			printf ("Comm test ok\n");
		} else {
			printf ("Comm test not ok\n");
			printf ("%04X\n", eink_read_register (0x148));
		}
	}
	
	while (1) {
		vTaskDelay (1000 / portTICK_RATE_MS);
	}
}


const struct board_layout BOARD_LAYOUTS[] = {
		[BOARD_V0_1] = { BOARD_V0_1,
				"Test release v0.1",
				{AT91C_BASE_PIOA, 1L<<1}, // AD7147 INT
				{AT91C_BASE_PIOA, 1L<<0}, // PN532 INT
		},
		[BOARD_V0_2] = { BOARD_V0_2,
				"Release v0.2",
				{AT91C_BASE_PIOA, 1L<<19},
				{AT91C_BASE_PIOA, 1L<<17},
		},
};
const struct board_layout * BOARD;
static void detect_board(void)
{
	/* Detect board v0.1 vs. v0.2 by looking at the POWER_MODE_0 and _1 pulldowns */
	AT91F_PIO_CfgInput(POWER_MODE_PIO, POWER_MODE_0_PIN | POWER_MODE_1_PIN);
	POWER_MODE_PIO->PIO_PPUER = POWER_MODE_0_PIN | POWER_MODE_1_PIN;

	volatile int i; for(i=0; i<100; i++) ;

	if( (!!AT91F_PIO_IsInputSet(POWER_MODE_PIO, POWER_MODE_0_PIN)) != (!!AT91F_PIO_IsInputSet(POWER_MODE_PIO, POWER_MODE_1_PIN)) ){
		/* Safety check: in the current hardware these two must always be set to the same level */
		led_halt_blinking(0);
	}

	if( AT91F_PIO_IsInputSet(POWER_MODE_PIO, POWER_MODE_0_PIN) ) {
		BOARD = &(BOARD_LAYOUTS[BOARD_V0_1]);
	} else {
		BOARD = &(BOARD_LAYOUTS[BOARD_V0_2]);
	}

	POWER_MODE_PIO->PIO_PPUDR = POWER_MODE_0_PIN | POWER_MODE_1_PIN;
}

static long last_transfer = 0, count_old, count = 0;
static void __attribute__((unused)) slider_update_cb(struct slider_state *state)
{
	/* This prints the slider report frequency */
	(void)state;
	long now = xTaskGetTickCount();
	if( now - last_transfer > (long)(1000/portTICK_RATE_MS) ) {
		count_old = count;
		count = 0;
		last_transfer = now;
		printf("%li\n", count_old);
	}
	count++;
}

void sdram_clear(void)
{
	register int r0 asm("r0") = 0xdeadbeef, 
		r1 asm("r1") = 0xcafebabe,
		r2 asm("r2") = 0xc0ded00d,
		r3 asm("r3") = 0xb0bcad00,
		r4 asm("r4") = 0,
		r5 asm("r5") = 0,
		r6 asm("r6") = 0,
		r7 asm("r7") = SDRAM_SIZE / (8*4);
	register volatile void *base asm("r8") = SDRAM_BASE;
	while(r7-->0) {
		asm volatile(
				"STM %8!, {%0-%7}"
				: "+r" (r0), "+r" (r1), "+r" (r2), "+r" (r3), 
					"+r" (r4), "+r" (r5), "+r" (r6), "+r" (r7)
				: "r" (base)
				: "memory"
		);
	}
}

/**********************************************************************/
void __attribute__((noreturn)) mainloop (void)
{
	prvSetupHardware ();
	AT91F_RSTSetMode(AT91C_BASE_RSTC, AT91F_RSTGetMode(AT91C_BASE_RSTC) | 0x11 );
	
	led_init ();
	
	detect_board();
	
	sdram_init ();
	if(0) { /* This takes a lot of power and should only be done with a connected battery.
			   Note: Since it writes to SDRAM and the tasks stack etc. are in SDRAM, it
			   MUST be done directly after sdram_init() or not at all. The FreeRTOS code
			   does not take kindly to messing around with the SDRAM contents after it
			   has used it. */
		led_set_red(1);
		sdram_clear();
		led_set_red(0);
	}

	if (!power_init ()) {
		led_halt_blinking (1);
	}
	power_on ();
	
	
	xTaskCreate (watchdog_restart_task, (signed portCHAR *) "WATCHDOG",
			TASK_WATCHDOG_STACK, NULL, TASK_WATCHDOG_PRIORITY, NULL);
	
	xTaskCreate (vUSBCDCTask, (signed portCHAR *) "USB        ", TASK_USB_STACK,
			NULL, TASK_USB_PRIORITY, NULL);
	
	/* vInitProtocolLayer (); */
	
	//xTaskCreate (sdram_test_task, (signed portCHAR *) "SDRAM_DEMO 0", 512, (void*)0, NEAR_IDLE_PRIORITY, NULL);
	//xTaskCreate (sdram_test_task, (signed portCHAR *) "SDRAM_DEMO 1", 512, (void*)3000, NEAR_IDLE_PRIORITY, NULL);
	//xTaskCreate (sdram_test_task, (signed portCHAR *) "SDRAM_DEMO 2", 512, (void*)4000, NEAR_IDLE_PRIORITY, NULL);
	//xTaskCreate (sdram_test_task, (signed portCHAR *) "SDRAM_DEMO 2", 512, (void*)5000, NEAR_IDLE_PRIORITY, NULL);
	//xTaskCreate(eink_traffic_test_task, (signed portCHAR *) "EINK TRAFFIC", 512, NULL, NEAR_IDLE_PRIORITY, NULL);
	
	/*xTaskCreate (demo_task, (signed portCHAR *) "LED_DEMO", 512,
     NULL, NEAR_IDLE_PRIORITY, NULL); */
	/*xTaskCreate (sdcard_test_task, (signed portCHAR *) "SDCARD_DEMO", 1024,
			NULL, NEAR_IDLE_PRIORITY, NULL);*/
	/*xTaskCreate (flash_demo_task, (signed portCHAR *) "FLASH DEMO", 512,
     NULL, NEAR_IDLE_PRIORITY, NULL); */
	/*xTaskCreate (pn532_demo_task, (signed portCHAR *) "PN532 DEMO TASK", TASK_PN532_STACK,
     NULL, TASK_PN532_PRIORITY, NULL);*/
	
	
	eink_interface_init();
	ad7147_init();
	//accelerometer_init();
	DFS_Init ();
	//pn532_init();
	//libnfc_demo_init();
	//picc_emu_init();
	//ebook_init();
	paint_init();
	
	//slider_register_slider_update_callback(slider_update_cb);
	
	led_set_green (1);
	
	vTaskStartScheduler ();
	
	while(1);
}
