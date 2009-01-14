#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <beacontypes.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>

#include <task.h>

#include "accelerometer.h"
#include "adc.h"
#include "led.h"

static adc_device accelerometer_adc;

static int axis_to_channel[] = {
		[AXIS_X] = ACCELEROMETER_X_CHANNEL,
		[AXIS_Y] = ACCELEROMETER_Y_CHANNEL,
		[AXIS_Z] = ACCELEROMETER_Z_CHANNEL,
};

/* Derived from free-fall measurements (probably only accurate to two decimal digits) */
static float offsets[] = {
		[AXIS_X] = -0.045429,
		[AXIS_Y] = -0.102000,
		[AXIS_Z] = -0.157214,
};

//#define ACCELEROMETER_PRINT_MEASUREMENTS

static orientation_changed_callback_t *orientation_changed_callback = NULL;

int accelerometer_register_orientation_changed_callback(orientation_changed_callback_t *callback)
{
	if(orientation_changed_callback == NULL) orientation_changed_callback = callback;
	else return -EBUSY;
	return 0;
}

#define ORIENTATION_FILTER 3
static enum ORIENTATION orientation_filter[ORIENTATION_FILTER] = {ORIENTATION_X_UP};
static int orientation_filter_index = 0;
static enum ORIENTATION orientation = ORIENTATION_X_UP;
enum ORIENTATION accelerometer_get_orientation(void)
{
	return orientation;
}

int accelerometer_get_raw(enum AXIS axis)
{
	return accelerometer_adc.results[ axis_to_channel[axis] ];
}

float accelerometer_get_g(enum AXIS axis)
{
	int raw = accelerometer_get_raw(axis);
	float voltage = (((float)raw) / (float)0x400) * 3.3;
	return (voltage - 1.65)/0.8 + offsets[axis];
}


static float angle(float a, float b) { return ((a*a+b*b) > 0.4) ? atan2(a, b) : 0; }

static void accelerometer_set_sleep(int sleep)
{
	if(sleep)
		AT91F_PIO_ClearOutput(ACCELEROMETER_SLEEP_PIO, ACCELEROMETER_SLEEP_PIN);
	else
		AT91F_PIO_SetOutput(ACCELEROMETER_SLEEP_PIO, ACCELEROMETER_SLEEP_PIN);
	
}

#define ACCELEROMETER_FLIP_THRESHOLD 0.33
static void accelerometer_task(void *parameter)
{
	(void)parameter;
	int i;
	while(1) {
		accelerometer_set_sleep(0);
		vTaskDelay(4/portTICK_RATE_MS);
		adc_convert(&accelerometer_adc);
		accelerometer_set_sleep(1);
		
		float x = accelerometer_get_g(AXIS_X);
		float y = accelerometer_get_g(AXIS_Y);
		float angle1 = angle(x, y);
#ifdef ACCELEROMETER_PRINT_MEASUREMENTS
		float z = accelerometer_get_g(AXIS_Z);
		float magnitude = sqrt(x*x+y*y+z*z);
		float angle2 = angle(y, z);
		float angle3 = angle(z, x);
		printf("%# 1.3f %# 1.3f %# 1.3f  %# 1.3f  %# 1.3f %# 1.3f %# 1.3f\n", x, y, z, 
				magnitude, 
				angle1, angle2, angle3);
#endif
		
		enum ORIENTATION new_orient = orientation;
		
		if( x*x + y*y > 0.66*0.66 ) {
			if(fabs(angle1 - 0.5*M_PI) < ACCELEROMETER_FLIP_THRESHOLD) {
				new_orient = ORIENTATION_X_UP;
			} else if(fabs(angle1 + 0.5*M_PI) < ACCELEROMETER_FLIP_THRESHOLD) {
				new_orient = ORIENTATION_X_DOWN;
			} else if(fabs(angle1 - 0) < ACCELEROMETER_FLIP_THRESHOLD) {
				new_orient = ORIENTATION_Y_UP;
			} else if(fabs(fabs(angle1) - M_PI) < ACCELEROMETER_FLIP_THRESHOLD) {
				new_orient = ORIENTATION_Y_DOWN;
			}
		}
		
		/* Keep history of ORIENTATION_FILTER values */
		orientation_filter[orientation_filter_index] = new_orient;
		orientation_filter_index = (orientation_filter_index+1)%ORIENTATION_FILTER;
		/* Only issue a switch when all history values are the same and equal to the new value */
		for(i=0; i<ORIENTATION_FILTER; i++) 
			if(orientation_filter[i] !=new_orient) {
				new_orient = orientation;
				break;
			}
		
		if(new_orient != orientation) {
			printf("Orientation changed: %i\n", new_orient);
			orientation = new_orient;
			if(orientation_changed_callback != NULL)
				orientation_changed_callback(new_orient);
		}
		vTaskDelay(16/portTICK_RATE_MS);
	}
}

void accelerometer_init(void)
{
    accelerometer_set_sleep(1);
    AT91F_PIO_CfgOutput(ACCELEROMETER_SLEEP_PIO, ACCELEROMETER_SLEEP_PIN);

	adc_open(&accelerometer_adc,
			(1L<<ACCELEROMETER_X_CHANNEL) |
			(1L<<ACCELEROMETER_Y_CHANNEL) |
			(1L<<ACCELEROMETER_Z_CHANNEL) );
	xTaskCreate (accelerometer_task, (signed portCHAR *) "ACCELEROMETER", TASK_ACCELEROMETER_STACK,
			NULL, TASK_ACCELEROMETER_PRIORITY, NULL);
}
