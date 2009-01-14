#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <beacontypes.h>

#include <stdio.h>
#include <string.h>

#include <task.h>

#include "led.h"
#include "eink/eink.h"
#include "eink/eink_lowlevel.h"

/* eInk controller SPI flash primitives
 * You MUST have called eink_interface_init() before using these functions
 * You MUST wait 4ms between eink_interface_init() and eink_flash_acquire()
 * You MUST NOT use any other eink_* function between calls to eink_flash_acquire()
 * and eink_flash_release().
 * eink_flash_release() will reset the controller, so you'll need to call eink_controller_init()
 * after eink_flash_release() before using the other eink_* functions.
 */

static
#ifdef HAVE_FLASH_CONTENT
#include "eink/eink_flash_content.h"
#else
const unsigned char eink_flash_content[] = {};
#endif


int eink_flash_acquire(void)
{
	eink_controller_reset();
	
	u_int16_t product = eink_read_register(2);
	if(product != 0x0047) return -EINK_ERROR_NOT_DETECTED;
	
	u_int16_t revision = eink_read_register(0);
	if(revision != 0x0100) return -EINK_ERROR_NOT_SUPPORTED;
	
	eink_write_register(0x10, 0x0004);
	eink_write_register(0x12, 0x5949);
	eink_write_register(0x14, 0x0040);
	
	eink_write_register(0x16, 0x000);
	while((eink_read_register(0xa) & 1) != 1);
	
	eink_write_register(0x6, 0x0);
	
	/* Enable power-on sequence */
	
	eink_write_register(0x234, 0x1c);
	eink_write_register(0x236, 0x09);
	eink_write_register(0x230, 1);
	while(eink_read_register(0x230) & 0x80) ;

	eink_write_register(0x204, EINK_FLASH_CONTROL_VALUE & ~(0x80));
	eink_write_register(0x208, 0);
	
	return 0;
}

#define SPI_FLASH_BUSY 0x08
#define SPI_FLASH_WRITE_EMPTY 0x04
#define SPI_FLASH_READ_READY 0x01
static u_int16_t eink_flash_wait(u_int8_t mask, u_int8_t val)
{
	u_int16_t status;
	while( ((status=eink_read_register(0x206)) & mask) != val) ;
	return status;
}

static int eink_flash_read_words(u_int32_t *dst, size_t len)
{
	unsigned int i,j;
	u_int32_t val=0;
	for(i=0; i<len/4; i++) {
		for(j=0; j<4; j++) {
			eink_flash_wait(SPI_FLASH_BUSY, 0);
			eink_flash_wait(SPI_FLASH_WRITE_EMPTY, SPI_FLASH_WRITE_EMPTY);
			eink_write_register(0x202, 0x0);
			eink_flash_wait(SPI_FLASH_BUSY, 0);
			eink_flash_wait(SPI_FLASH_READ_READY, SPI_FLASH_READ_READY);
			val = val | ( (eink_read_register(0x200)&0xff) << (j*8));
		}
		dst[i] = val;
		val = 0;
	}
	return 0;
}

static int eink_flash_read_bytes(u_int8_t *dst, size_t len)
{
	unsigned int i;
	for(i=0; i<len; i++) {
		eink_flash_wait(SPI_FLASH_BUSY, 0);
		eink_flash_wait(SPI_FLASH_WRITE_EMPTY, SPI_FLASH_WRITE_EMPTY);
		eink_write_register(0x202, 0x0);
		eink_flash_wait(SPI_FLASH_BUSY, 0);
		eink_flash_wait(SPI_FLASH_READ_READY, SPI_FLASH_READ_READY);
		dst[i] = eink_read_register(0x200) & 0xff;
	}
	return 0;
}

static void eink_flash_send_address(u_int32_t flash_address)
{
	eink_flash_wait(SPI_FLASH_BUSY, 0);
	eink_flash_wait(SPI_FLASH_WRITE_EMPTY, SPI_FLASH_WRITE_EMPTY);
	eink_write_register(0x202, 0x0100 | ((flash_address >> 16) & 0xff) ); /* Address[23:16] */
	
	eink_flash_wait(SPI_FLASH_BUSY, 0);
	eink_flash_wait(SPI_FLASH_WRITE_EMPTY, SPI_FLASH_WRITE_EMPTY);
	eink_write_register(0x202, 0x0100 | ((flash_address >> 8) & 0xff) ); /* Address[15:8] */
	
	eink_flash_wait(SPI_FLASH_BUSY, 0);
	eink_flash_wait(SPI_FLASH_WRITE_EMPTY, SPI_FLASH_WRITE_EMPTY);
	eink_write_register(0x202, 0x0100 | ((flash_address >> 0) & 0xff) ); /* Address[7:0] */
}

int eink_flash_read(u_int32_t flash_address, char *dst, size_t len)
{
	int r;
	eink_write_register(0x208, 0x1); /* Enable Chip Select */
	
	eink_flash_wait(SPI_FLASH_BUSY, 0);
	eink_flash_wait(SPI_FLASH_WRITE_EMPTY, SPI_FLASH_WRITE_EMPTY);
	eink_write_register(0x202, 0x103); /* Read command */
	
	eink_flash_send_address(flash_address);
	
	if(((unsigned int)dst & 0x3) == 0) {
		r = eink_flash_read_words((u_int32_t*)dst, len - (len%4));
		if(r>=0 && (len%4 != 0)) 
			r = eink_flash_read_bytes((u_int8_t*)(dst + (len - (len%4))), len%4);
	} else {
		r = eink_flash_read_bytes((u_int8_t*)dst, len);
	}
	eink_write_register(0x208, 0x0); /* Disable Chip Select */
	return r;
}

static int eink_flash_wait_status_register(u_int8_t mask, u_int8_t value)
{
	u_int8_t status;
	eink_write_register(0x208, 0x1); /* Enable Chip Select */

	eink_flash_wait(SPI_FLASH_BUSY, 0);
	eink_flash_wait(SPI_FLASH_WRITE_EMPTY, SPI_FLASH_WRITE_EMPTY);
	eink_write_register(0x202, 0x105); /* Read flash status register command */
	
	while(1) {
		eink_flash_read_bytes(&status, 1);
		if( (status & mask) == value )
			break;
	}
	
	eink_write_register(0x208, 0x0); /* Disable Chip Select */
	return 0;
}

int eink_flash_write_enable(void)
{
	eink_write_register(0x208, 0x1); /* Enable Chip Select */
	
	eink_flash_wait(SPI_FLASH_BUSY, 0);
	eink_flash_wait(SPI_FLASH_WRITE_EMPTY, SPI_FLASH_WRITE_EMPTY);
	eink_write_register(0x202, 0x106); /* Write enable command */
	
	eink_flash_wait(SPI_FLASH_BUSY, 0);
	
	eink_write_register(0x208, 0x0); /* Disable Chip Select */
	return 0;
}

int eink_flash_bulk_erase(void)
{
	eink_write_register(0x208, 0x1); /* Enable Chip Select */

	eink_flash_wait(SPI_FLASH_BUSY, 0);
	eink_flash_wait(SPI_FLASH_WRITE_EMPTY, SPI_FLASH_WRITE_EMPTY);
	eink_write_register(0x202, 0x1C7); /* Bulk erase command */

	eink_flash_wait(SPI_FLASH_BUSY, 0);
	
	eink_write_register(0x208, 0x0); /* Disable Chip Select */
	
	eink_flash_wait_status_register(1, 0); /* Wait for Write-In-Progress to go low */
	
	return 0;
}

int eink_flash_read_identification(void)
{
	u_int8_t id[3];
	eink_write_register(0x208, 0x1); /* Enable Chip Select */

	eink_flash_wait(SPI_FLASH_BUSY, 0);
	eink_flash_wait(SPI_FLASH_WRITE_EMPTY, SPI_FLASH_WRITE_EMPTY);
	eink_write_register(0x202, 0x19F); /* Read identification command */

	eink_flash_read_bytes(id, 3);

	eink_write_register(0x208, 0x0); /* Disable Chip Select */
	return (id[0]<<16) | (id[1]<<8) | id[2];
}

int eink_flash_program_page(u_int32_t flash_address, const unsigned char *data, unsigned int len)
{
	unsigned int i;
	eink_write_register(0x208, 0x1); /* Enable Chip Select */
	
	eink_flash_wait(SPI_FLASH_BUSY, 0);
	eink_flash_wait(SPI_FLASH_WRITE_EMPTY, SPI_FLASH_WRITE_EMPTY);
	eink_write_register(0x202, 0x102); /* Program Page command */
	
	eink_flash_send_address(flash_address);

	for(i=0; i<len; i++) {
		eink_flash_wait(SPI_FLASH_BUSY, 0);
		eink_flash_wait(SPI_FLASH_WRITE_EMPTY, SPI_FLASH_WRITE_EMPTY);
		eink_write_register(0x202, 0x100 | data[i]); /* data */
	}
	
	eink_write_register(0x208, 0x0); /* Disable Chip Select */

	eink_flash_wait_status_register(1, 0); /* Wait for Write-In-Progress to go low */
	return 0;
}

#define PAGE_LEN 256
int eink_flash_program(const unsigned char *data, unsigned int len)
{
	unsigned int offset = 0;
	while(offset<len) {
		unsigned int length = 256;
		if(offset+length >= len)
			length = len-offset;
		eink_flash_write_enable();
		eink_flash_program_page(offset, data+offset, length);
		offset += length;
	}
	return 0;
}

void eink_flash_release(void)
{
	eink_controller_reset();
}

static char flash_tmpbuf[4];
//#define DEBUG_REFLASH

static int end_led = 0;
static void reflash_led_blinker(void *parameters)
{
	(void)parameters;
	while(!end_led) {
		led_set_red(0); led_set_green(1);
		vTaskDelay(100/portTICK_RATE_MS);
		led_set_red(1); led_set_green(0);
		vTaskDelay(100/portTICK_RATE_MS);
	}
	led_set_red(0); led_set_green(1);
	while(1) { vTaskDelay(1000/portTICK_RATE_MS); }
}

/* Reflash the controller flash if all of the following conditions are satisfied:
 *  + We have new flash content in eink_flash_content
 *  + The flash identification indicates our custom built controller and not the stock Epson one
 *  + The first 4 bytes of the existing flash content do not match eink_flash_content
 * 
 * If this function returns a non-null value then the controller was reset and the application
 * should wait 5ms before proceeding;
 */
int eink_flash_conditional_reflash(void)
{
	if(sizeof(eink_flash_content) == 0) return 0;
	int r = eink_flash_acquire();
	if(r>=0) {
		unsigned int ident = eink_flash_read_identification();
		xTaskHandle led_task_handle;
#ifdef DEBUG_REFLASH
		printf("Device identification: %06X\n", ident);
#endif
		if( ident != 0x202016 ) { /* Only for our own flash, not the pre-assembled */
			goto out_done;
		}
		
		eink_flash_read(0, flash_tmpbuf, sizeof(flash_tmpbuf));
#ifdef DEBUG_REFLASH
		printf("%02X %02X %02X %02X\n", flash_tmpbuf[0], flash_tmpbuf[1], flash_tmpbuf[2], flash_tmpbuf[3]);
#endif
		if(memcmp(flash_tmpbuf, eink_flash_content, sizeof(flash_tmpbuf)) != 0) {
			printf("Flash content not correct, starting reflash cycle\nDO NOT POWER OFF\n");
			xTaskCreate( reflash_led_blinker, (signed char*)"LED BLINKER", 128, NULL, tskIDLE_PRIORITY+1, &led_task_handle);
			printf("Starting bulk erase\n");
			eink_flash_write_enable();
			eink_flash_bulk_erase();
			printf("Done bulk erase\n");
			printf("Starting programming (%i bytes)\n", sizeof(eink_flash_content));
			eink_flash_program(eink_flash_content, sizeof(eink_flash_content));
			printf("Done\n");
			end_led = 1;
			vTaskDelay(150/portTICK_RATE_MS);
			vTaskDelete(led_task_handle);
			led_set_red(0); led_set_green(1);
		}
	}
out_done:
	eink_flash_release();
	return 1;
}
