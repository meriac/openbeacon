#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <task.h>

#include <led.h>
#include <USB-CDC.h>
#include <stdio.h>
#include <errno.h>

#include "pio_irq.h"
#include "adc.h"
#include "power.h"

adc_device batcheck_adc;

static void power_pressed_default_callback(void)
{
	power_set_bus_current(POWER_BUS_SUSPEND);
	power_off();
	while(1) {vTaskDelay(1000/portTICK_RATE_MS);}
}
static power_pressed_callback_t *power_pressed_callback = power_pressed_default_callback;

void power_on(void)
{
    AT91F_PIO_SetOutput(POWER_ON_PIO, POWER_ON_PIN);
}

void power_off(void)
{
    AT91F_PIO_ClearOutput(POWER_ON_PIO, POWER_ON_PIN);
}

unsigned char power_is_pressed(void)
{
	return !AT91F_PIO_IsInputSet(BUTTON_PIO, BUTTON_PIN);
}

unsigned char power_is_usb_connected(void)
{
	return !!AT91F_PIO_IsInputSet(USB_DETECT_PIO, USB_DETECT_PIN);
}

void power_set_bus_current(enum power_bus_current current)
{
	switch(current) {
	case POWER_BUS_SUSPEND:
		AT91F_PIO_SetOutput(POWER_MODE_PIO, POWER_MODE_0_PIN | POWER_MODE_1_PIN);
		break;
	case POWER_BUS_1:
		AT91F_PIO_ClearOutput(POWER_MODE_PIO, POWER_MODE_0_PIN | POWER_MODE_1_PIN);
		break;
	case POWER_BUS_5:
		AT91F_PIO_ClearOutput(POWER_MODE_PIO, POWER_MODE_0_PIN);
		AT91F_PIO_SetOutput(POWER_MODE_PIO, POWER_MODE_1_PIN);
		break;
	case POWER_BUS_10:
		AT91F_PIO_ClearOutput(POWER_MODE_PIO, POWER_MODE_1_PIN);
		AT91F_PIO_SetOutput(POWER_MODE_PIO, POWER_MODE_0_PIN);
		break;
	}
}

/* Get batcheck voltage as 10-bit integer, relative to AD_VREF (3.3V). 
 * Battery full seems to be 0x37F-0x380. Batcheck is the battery on a 27/12 voltage divider */
int power_get_battery_voltage(void)
{
	adc_convert(&batcheck_adc);
	return batcheck_adc.results[POWER_BATTERY_CHANNEL];
}


void power_monitor_task(void *parameter)
{
	(void)parameter;
	int state = 0;
	int wait_for_usb = 0;
	
	power_set_bus_current(POWER_BUS_1);
	while(1) {
		switch(state) { /* debounce the power switch */
		case 0:
			if(!power_is_pressed())	state = 1;
			break;
		case 1:
			if(power_is_pressed()) state = 2;
			break;
		case 2:
			if(!power_is_pressed()) { /* Power switch pressed and released. Switch off power */
				power_pressed_callback();
				state = 0;
			}
			break;
		}
		vTaskDelay(100/portTICK_RATE_MS);
		
		/* FIXME: Add interrupt handler for USB_DETECT */
		if(!power_is_usb_connected()) wait_for_usb = 0;
		else if(wait_for_usb < 10) ++wait_for_usb;
		
		/* Assume that the high-power configuration has CbConfigurationValue == 1 */
		if( (iUSBGetChosenConfiguration() == 1) && wait_for_usb >= 10) {
			power_set_bus_current(POWER_BUS_5);
		} else {
			power_set_bus_current(POWER_BUS_1);
		}
	}
}

int power_set_pressed_callback(power_pressed_callback_t *callback)
{
	if(callback == NULL) {
		power_pressed_callback = power_pressed_default_callback;
		return 0;
	}
	if(power_pressed_callback != power_pressed_default_callback)
		return -EBUSY;
	power_pressed_callback = callback;
	return 0;
}


unsigned char power_init(void)
{
	pio_irq_init_once();
	
	adc_open(&batcheck_adc, 1<<POWER_BATTERY_CHANNEL);
	
	AT91F_PIO_SetOutput(POWER_MODE_PIO, POWER_MODE_0_PIN | POWER_MODE_1_PIN);
	AT91F_PIO_CfgOutput(POWER_MODE_PIO, POWER_MODE_0_PIN | POWER_MODE_1_PIN);
	
	AT91F_PIO_CfgInput(BUTTON_PIO, BUTTON_PIN);
	AT91F_PIO_CfgInput(USB_DETECT_PIO, USB_DETECT_PIN);
	
	AT91F_PIO_ClearOutput(POWER_ON_PIO, POWER_ON_PIN);
	AT91F_PIO_CfgOutput(POWER_ON_PIO, POWER_ON_PIN);
	
	return xTaskCreate (power_monitor_task, (signed portCHAR *) "POWER_MONITOR", TASK_POWER_STACK,
			NULL, TASK_POWER_PRIORITY, NULL) == pdPASS;
}
