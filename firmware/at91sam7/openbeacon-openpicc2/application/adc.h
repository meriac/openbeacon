#ifndef ADC_H_
#define ADC_H_

#define ADC_MAX_CHANNELS 8

#include <beacontypes.h>

typedef struct {
	u_int32_t channels;
	u_int32_t results[ADC_MAX_CHANNELS];
} adc_device;

extern int adc_open(adc_device *device, u_int32_t channels);
extern int adc_close(adc_device *device);
extern int adc_convert(adc_device *device);

#endif /*ADC_H_*/
