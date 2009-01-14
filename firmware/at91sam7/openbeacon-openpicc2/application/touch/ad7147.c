#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <beacontypes.h>
#include <task.h>
#include <semphr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "touch/ad7147.h"
#include "touch/ad7147_registers.h"
#include "touch/slider.h"
#include "spi.h"
#include "led.h"
#include "pio_irq.h"

static const int SCBR = ((int) (MCK / 5e6) + 1) & 0xFF;
static spi_device ad7147_spi;
static xSemaphoreHandle ad7147_irq_semaphore;

#define TRANSMIT_CHIP_SELECT ((~(1L<<(AD7147_CS+16))) & 0x000F0000)
#define AD7147_COMMAND_WORD_READ 0xE400
#define AD7147_COMMAND_WORD_WRITE 0xE000
#define SIZED_COMMAND_BUFFER(size) struct { \
	u_int16_t command; \
	u_int16_t buffer[size]; \
} __attribute__((packed))

static portBASE_TYPE ad7147_irq(AT91PS_PIO pio, u_int32_t pin, portBASE_TYPE xTaskWoken)
{
	(void)pio; (void)pin;
	/* Wake up the main thread, then disable IRQ */
	xSemaphoreGiveFromISR(ad7147_irq_semaphore, &xTaskWoken);
	pio_irq_disable(AD7147_INT_PIO, AD7147_INT_PIN);
	return xTaskWoken;
}

static void ad7147_wait_for_irq(void)
{
	pio_irq_enable(AD7147_INT_PIO, AD7147_INT_PIN);
	xSemaphoreTake(ad7147_irq_semaphore, 10000/portTICK_RATE_MS);
}

static int ad7147_load_initial_configuration(void);
static int ad7147_detect(void);

static SIZED_COMMAND_BUFFER(3+12) interrupt_status;
static struct slider_state slider_state;
//#define PRINT_INTERRUPT_STATE
//#define PRINT_SLIDER_STATE
static void ad7147_main_task(void *parameter)
{
	(void)parameter;
	if(!ad7147_detect())
		led_halt_blinking(4);
	ad7147_registers_create_initial_configuration();
	if(ad7147_load_initial_configuration() != 0)
		led_halt_blinking(5);
	int do_extra_poll = 0;
	while(1) {
		/* Wait for an IRQ, but only when we didn't just previously change the IRQ settings */
		if(!do_extra_poll) 
			ad7147_wait_for_irq();
		do_extra_poll = 0;
		/* Handle IRQ */
		memset(&interrupt_status, 0xff, sizeof(interrupt_status));
		interrupt_status.command = AD7147_COMMAND_WORD_READ | 0x08;
		spi_transceive(&ad7147_spi, &interrupt_status, 1+3+12);
#ifdef PRINT_INTERRUPT_STATE
		printf("AD7147 IRQ");
#endif
		spi_wait_for_completion(&ad7147_spi);
		
#ifdef PRINT_INTERRUPT_STATE
		/* Print raw register values */
		printf(" %04X %04X %04X  ", interrupt_status.buffer[0], interrupt_status.buffer[1], interrupt_status.buffer[2]);
		int i;
		for(i=0; i<12; i++) 
			printf("  %04x ", interrupt_status.buffer[3+i]);
		printf("\n");
#endif
		
		/* Print nice bargraph */
#if 0
		int j;
		for(i=0; i<12; i++) {
			for(j=0; j<6; j++)
				if(j*0x1000 > (interrupt_status.buffer[3+i]-0xa000))
					printf(" ");
				else
					printf("#");
			printf(" ");
		}
		printf("\n");
#else
		slider_update(interrupt_status.buffer, &slider_state);
#ifdef PRINT_SLIDER_STATE
		slider_print(&slider_state);
#endif
		int update_irq = 0;
		if(slider_state.buttons.button1 || slider_state.buttons.button2 || slider_state.buttons.xtouching || slider_state.buttons.ytouching) {
			/* User touches the slider, enable regular interrupts */
			if(ad7147_registers.banks.bank1.reg[7] != 1L<<10) {
				ad7147_registers.banks.bank1.reg[7] = 1L<<10;
				update_irq = 1;
			}
		} else {
			/* User doesn't touch the slider, disable regular interrupts */
			if(ad7147_registers.banks.bank1.reg[7] != 0) {
				ad7147_registers.banks.bank1.reg[7] = 0;
				update_irq = 1;
			}
		}
		if(update_irq) {
			u_int16_t buf[2] = {AD7147_COMMAND_WORD_WRITE | 7, ad7147_registers.banks.bank1.reg[7]};
			spi_transceive_automatic_retry(&ad7147_spi, &buf, 1+1);
			spi_wait_for_completion(&ad7147_spi);
			do_extra_poll = 1;
		}
#endif
#if defined(PRINT_INTERRUPT_STATE) || defined(PRINT_SLIDER_STATE)
		vTaskDelay(100/portTICK_RATE_MS); /* Give the USB time to send all the data */
#endif
	}
}

static SIZED_COMMAND_BUFFER(8) bank1_init;
static SIZED_COMMAND_BUFFER(96) bank2_init;

static int ad7147_load_initial_configuration(void)
{
	int r;
	/* Write Bank 2 */
	bank2_init.command = AD7147_COMMAND_WORD_WRITE | 0x80;
	memcpy(bank2_init.buffer, &ad7147_registers.banks.bank2, 96*2);
	r=spi_transceive(&ad7147_spi, &bank2_init, 1+96);
	if(r != 0) return r;
	
	/* Write Bank 1 (first 8 registers) */
	bank1_init.command = AD7147_COMMAND_WORD_WRITE | 0x00;
	memcpy(bank1_init.buffer, &ad7147_registers.banks.bank1, 8*2);
	r=spi_transceive(&ad7147_spi, &bank1_init, 1+8);
	if(r != 0) return r;
	
	/* Write Bank 1, address 1 */
	u_int16_t buf[2] = {AD7147_COMMAND_WORD_WRITE | 1, 0x07FF};
	r=spi_transceive(&ad7147_spi, &buf, 1+1);
	if(r != 0) return r;
	
	/* Wait for everything to complete */
	spi_wait_for_completion(&ad7147_spi);
	return 0;
}

int ad7147_detect(void)
{
	u_int16_t buf[2] = {AD7147_COMMAND_WORD_READ | 0x17, 0};
	spi_transceive(&ad7147_spi, buf, 2);
	spi_wait_for_completion(&ad7147_spi);
	int revision_code = buf[1] & 0xF;
	int devid = buf[1] >> 4;
	
	(void)revision_code;
	return devid == 0x147;
}

int ad7147_init(void)
{
	vSemaphoreCreateBinary(ad7147_irq_semaphore);
	AT91F_PIO_CfgPeriph(AD7147_CS_PIO, AD7147_CS_PIN, 0);
	AT91F_PIO_CfgInput(AD7147_INT_PIO, AD7147_INT_PIN);
	pio_irq_register(AD7147_INT_PIO, AD7147_INT_PIN, ad7147_irq);
	
	spi_open(&ad7147_spi, AD7147_CS, AT91C_SPI_BITS_16 | AT91C_SPI_CPOL | 
			(SCBR << 8) | (1L << 16) | (0L << 24) );
	
	xTaskCreate (ad7147_main_task, (signed portCHAR *) "AD7147 MAIN TASK", TASK_AD7147_STACK,
			NULL, TASK_AD7147_PRIORITY, NULL);
	
	return 0;
}
