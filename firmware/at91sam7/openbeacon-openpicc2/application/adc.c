#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <beacontypes.h>
#include <errno.h>

#include <semphr.h>

#include "adc.h"

static u_int32_t open_devices = 0;
static int initialized = 0;
xSemaphoreHandle adc_busy_semaphore;

static int adc_init(void)
{
	// PRESCAL = 4 -> ADC clock = MCLK / 10 = 4.8 MHz (max is 5 MHz at 10 bit)
	// Start time <20 us -> STARTUP=11
	// Track and hold time >600 ns -> SHTIM=2
	AT91C_BASE_ADC->ADC_MR = AT91C_ADC_TRGEN_DIS | AT91C_ADC_LOWRES_10_BIT |
		AT91C_ADC_SLEEP_MODE | (4 << 8) | (11 << 16) | (2 << 24);
	AT91C_BASE_ADC->ADC_IDR = 0xfffff;
	
	adc_busy_semaphore = xSemaphoreCreateMutex();
	if(adc_busy_semaphore == NULL)
		return -ENOMEM;
	xSemaphoreGive(adc_busy_semaphore);
	
	initialized = 1;
	return 0;
}


int adc_open(adc_device *device, u_int32_t channels)
{
	int r;
	if(!initialized) {
		r = adc_init();
		if(r != 0)
			return r;
	}
	
	if( (channels & open_devices) != 0)
		return -EBUSY;
	
	open_devices |= channels;
	device->channels = channels;
	return 0;
}

int adc_convert(adc_device *device)
{
	if(!initialized)
		return -ENOTCONN;
	
	xSemaphoreTake(adc_busy_semaphore, portMAX_DELAY);
    AT91C_BASE_ADC->ADC_CHDR = ~(device->channels);
    AT91C_BASE_ADC->ADC_CHER = device->channels;
    
	AT91C_BASE_ADC->ADC_CR = AT91C_ADC_START;
	while( (AT91C_BASE_ADC->ADC_SR & device->channels) != device->channels );
	
	int i;
	for(i=0; i<ADC_MAX_CHANNELS; i++)  {
		if( device->channels & (1L<<i) )
			device->results[i] = *(&(AT91C_BASE_ADC->ADC_CDR0)+i);
	}

    xSemaphoreGive(adc_busy_semaphore);
    return 0;
}
