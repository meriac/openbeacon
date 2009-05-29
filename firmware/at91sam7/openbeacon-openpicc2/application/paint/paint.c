#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <beacontypes.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>

#include <task.h>

#include "led.h"
#include "power.h"
#include "accelerometer.h"
#include "eink/eink.h"
#include "eink/eink_flash.h"
#include "lzo/minilzo.h"
#include "ebook/event.h"

#include "splash.h"

extern const struct splash_image bg_image;

extern unsigned char scratch_space[MAX_PART_SIZE] __attribute__((aligned (2)));

#define DISPLAY_SHORT (EINK_CURRENT_DISPLAY_CONFIGURATION->vsize)
#define DISPLAY_LONG (EINK_CURRENT_DISPLAY_CONFIGURATION->hsize)
#define ROUND_UP(i,j) ((j)*(((i)+(j-1))/(j)))

/* Warning: The actual display size must not exceed this definition. E.g.
 * assert( IMAGE_SIZE >= EINK_CURRENT_DISPLAY_CONFIGURATION->hsize * EINK_CURRENT_DISPLAY_CONFIGURATION->vsize );
 */
#define IMAGE_SIZE (1216*832)

struct image_buffer {
	unsigned int width;
	unsigned int height;
	u_int8_t data[IMAGE_SIZE];
};
static struct image_buffer __attribute__ ((section (".sdram"))) blank_data, black_data, bg_data;
static eink_image_buffer_t blank_buffer, black_buffer, bg_buffer;
static unsigned char eink_mgmt_data[10240] __attribute__ ((section (".sdram")));

static void unpack_image(struct image_buffer *target, const struct splash_image * const source)
{
	const struct splash_part * const * image_parts = source->splash_parts;
	void *current_target = &target->data;

	led_set_red(1);
	const struct splash_part * const * current_part = image_parts;
	while(*current_part != 0) {
		lzo_uint out_len = sizeof(scratch_space);
		if(lzo1x_decompress_safe((unsigned char*)((*current_part)->data), (*current_part)->compress_len, scratch_space, &out_len, NULL) == LZO_E_OK) {
			//printf(".");
		} else break;
		memcpy(current_target, scratch_space, out_len);
		current_target += out_len;
		current_part++;
	}
	led_set_red(0);
	target->width = source->width;
	target->height = source->height;
}



static void clear_screen(void)
{
	eink_job_t job;
	eink_job_begin(&job, 0);
	eink_job_add(job, blank_buffer, WAVEFORM_MODE_GC, UPDATE_MODE_FULL);
	eink_job_commit(job);
}

static void paint_die_error(enum eink_error error)
{
	switch(error) {
	case EINK_ERROR_NOT_DETECTED:
		printf("eInk controller not detected\n"); break;
	case EINK_ERROR_NOT_SUPPORTED:
		printf("eInk controller not supported (unknown revision)\n"); break;
	case EINK_ERROR_COMMUNICATIONS_FAILURE:
		printf("eInk controller communications failure, check FPC cable and connector\n"); break;
	case EINK_ERROR_CHECKSUM_ERROR:
		printf("eInk controller waveform flash checksum failure\n"); break;
	default:
		printf("eInk controller initialization: unknown error\n"); break;
	}
	led_halt_blinking(2);
}

static int event_loop_running = 0;
static void paint_task(void *params)
{
	(void)params;
	int error, idle_time=0;
	vTaskDelay(100/portTICK_RATE_MS);

	eink_mgmt_init(eink_mgmt_data, sizeof(eink_mgmt_data));

#if 0 /* FIXME Seems to be broken, sometimes (hangs at boot) */
	error = eink_flash_conditional_reflash();
	if(error > 0) { /* Not an error, but the controller was reinitialized and we must wait */
		vTaskDelay(5/portTICK_RATE_MS);
	} else if(error < 0) {
		paint_die_error(-error);
	}
#endif

	if (lzo_init() != LZO_E_OK) {
		printf("internal error - lzo_init() failed !!!\n");
		printf("(this usually indicates a compiler bug - try recompiling\nwithout optimizations, and enable `-DLZO_DEBUG' for diagnostics)\n");
	} else {
		printf("LZO real-time data compression library (v%s, %s).\n",
				lzo_version_string(), lzo_version_date());
	}

	eink_controller_reset();

	printf("Initializing eInk controller ...\n");
	error=eink_controller_init();
	if(error < 0) paint_die_error(-error);
	printf("OK\n");
	
	
	if( (error=eink_image_buffer_acquire(&blank_buffer)) < 0) {
		printf("Reason: %i\nCould not acquire blank image buffer\n", error);
		led_halt_blinking(3);
	}
	if( (error=eink_image_buffer_acquire(&black_buffer)) < 0) {
		printf("Reason: %i\nCould not acquire black image buffer\n", error);
		led_halt_blinking(3);
	}
	if( (error=eink_image_buffer_acquire(&bg_buffer)) < 0) {
		printf("Reason: %i\nCould not acquire bg image buffer\n", error);
		led_halt_blinking(3);
	}
	
	/*
	 * bg_image is in the flash
	 * black_data, blank_data, bg_data is in the microcontroller SDRAM
	 * black_buffer, blank_buffer, bg_buffer are in the display controller SDRAM
	 */
	
	memset(blank_data.data, 0xff, sizeof(blank_data.data));
	memset(black_data.data, 0x00, sizeof(black_data.data));
	
	unpack_image(&bg_data, &bg_image);
	
	portTickType start = xTaskGetTickCount(), stop;
	
	error = 0;
	error |= eink_image_buffer_load(blank_buffer, PACK_MODE_2BIT, ROTATION_MODE_90,
			blank_data.data, ROUND_UP(DISPLAY_LONG,8)*ROUND_UP(DISPLAY_SHORT,8)) /8;
	
	error |= eink_image_buffer_load(black_buffer, PACK_MODE_2BIT, ROTATION_MODE_90,
			black_data.data, ROUND_UP(DISPLAY_LONG,8)*ROUND_UP(DISPLAY_SHORT,8)) /8;
	
	error |= eink_image_buffer_load(bg_buffer, PACK_MODE_2BIT, ROTATION_MODE_90,
			blank_data.data, ROUND_UP(DISPLAY_LONG,8)*ROUND_UP(DISPLAY_SHORT,8)) /8;
	error |= eink_image_buffer_load_area(bg_buffer, PACK_MODE_1BYTE, ROTATION_MODE_90,
			(DISPLAY_SHORT-bg_data.width)/2, (DISPLAY_LONG-bg_data.height)/2,
			bg_data.width, bg_data.height,
			bg_data.data, bg_data.height*bg_data.width);
	
	stop = xTaskGetTickCount();
	printf("Images loaded in %li ticks\n", (long)(stop-start));

	if(error >= 0) {
		printf("All images loaded ok\n");
		eink_job_t job;
		eink_job_begin(&job, 0);
		eink_job_add(job, blank_buffer, WAVEFORM_MODE_INIT, UPDATE_MODE_FULL);
		eink_job_commit(job);
	} else {
		printf("Image load error %i: %s\n", error, strerror(-error));
	}

	int i=0;
	eink_job_t job;
	eink_job_begin(&job, 0);
	eink_job_add(job, bg_buffer, WAVEFORM_MODE_GC, UPDATE_MODE_FULL);
	eink_job_commit(job);

	event_t received_event;
	event_loop_running = 1;
	int battery_update_counter = 5;
	while(1) {
		received_event.class = EVENT_NONE;
		event_receive(&received_event, 1000/portTICK_RATE_MS);
		i++;

		switch(received_event.class) {
		case EVENT_POWEROFF:
			clear_screen();
			while(eink_job_count_pending() > 0) vTaskDelay(10/portTICK_RATE_MS);
			power_off();
		default:
			break;
		}

		if(received_event.class == EVENT_NONE && !power_is_usb_connected()) {
			idle_time++;
		} else {
			idle_time=0;
		}
#if 0
		if(idle_time > 600) {
			clear_screen();
			while(eink_job_count_pending() > 0) vTaskDelay(10/portTICK_RATE_MS);
			power_off();
		}
#endif
		
		if(battery_update_counter++ >= 2) {
			const int MIN_V = 600, MAX_V = 900;
			int voltage = power_get_battery_voltage();
			battery_update_counter = 0;
			int barlen = (DISPLAY_LONG * (voltage-MIN_V)) / (MAX_V-MIN_V);
			if(barlen >= DISPLAY_LONG) barlen = DISPLAY_LONG-1;
			if(barlen < 0) barlen = 0;
			
			eink_job_t job;
			eink_job_begin(&job, 0);
			if(barlen > 0) {
				eink_job_add_area(job, blank_buffer, WAVEFORM_MODE_GU, UPDATE_MODE_FULL, 
						DISPLAY_SHORT-1, 0, 
						1, barlen);
			}
			if(barlen < DISPLAY_LONG-1) {
				eink_job_add_area(job, black_buffer, WAVEFORM_MODE_GU, UPDATE_MODE_FULL, 
						DISPLAY_SHORT-1, DISPLAY_LONG-1 - barlen, /* x, y */ 
						1, DISPLAY_LONG-1); /* width, height */
			}
			eink_job_commit(job);
		}
	}
}

static void power_pressed_cb(void)
{
	if(event_loop_running)
		event_send(EVENT_POWEROFF, 0);
	else
		power_off();
}


int paint_init(void)
{
	int r;
	if( (r=event_init()) < 0)
		return r;

	if( (r=power_set_pressed_callback(power_pressed_cb)) < 0)
		return r;

	xTaskCreate(paint_task, (signed portCHAR *) "PAINT_ TASK", TASK_EBOOK_STACK,
			NULL, TASK_EBOOK_PRIORITY, NULL);

	return 0;
}
