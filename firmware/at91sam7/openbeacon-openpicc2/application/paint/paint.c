#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <beacontypes.h>

#include <USB-CDC.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <stdint.h>

#include <task.h>

#include "utils.h"
#include "led.h"
#include "power.h"
#include "accelerometer.h"
#include "eink/eink.h"
#include "eink/eink_flash.h"
#include "ebook/event.h"
#include "dosfs/fat_helper.h"

#include "image/splash.h"

extern const struct splash_image bg_splash_image;

#define DISPLAY_SHORT (EINK_CURRENT_DISPLAY_CONFIGURATION->vsize)
#define DISPLAY_LONG (EINK_CURRENT_DISPLAY_CONFIGURATION->hsize)

/* Warning: The actual display size must not exceed this definition. E.g.
 * assert( IMAGE_SIZE >= EINK_CURRENT_DISPLAY_CONFIGURATION->hsize * EINK_CURRENT_DISPLAY_CONFIGURATION->vsize );
 */
#define IMAGE_SIZE (1216*832)


static uint8_t eink_mgmt_data[10240] __attribute__ ((section (".sdram")));
static eink_image_buffer_t blank_buffer, bg_buffer, bg_fast_buffer;
static uint8_t __attribute__((aligned(4), section (".sdram")))
	_blank_data[IMAGE_SIZE], _bg_data[IMAGE_SIZE];
static struct image blank_image = {
		.data = _blank_data,
		.size = sizeof(_blank_data),
}, bg_image = {
		.data = _bg_data,
		.size = sizeof(_bg_data),
};


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

enum {
	MODE_BLANK,            /* Direct update on white */
	MODE_BACKGROUND,       /* Gray clear on background */
	MODE_BACKGROUND_FAST,  /* Direct update on background, with a trick */
} current_mode = MODE_BACKGROUND;
static void reset_background(void)
{
	switch(current_mode) {
	case MODE_BLANK:
	case MODE_BACKGROUND_FAST:
		memset(bg_image.data, 0xff, bg_image.size);
		break;
	case MODE_BACKGROUND: 
		{
			int error = image_unpack_splash(&bg_image, &bg_splash_image);
			if(error < 0) {
				printf("Error during splash unpack: %i (%s)\n", error, strerror(-error));
			}
		}
		break;
	}
	eink_image_buffer_load_area(bg_buffer, image_get_bpp_as_pack_mode(&bg_image), ROTATION_MODE_90,
			(DISPLAY_SHORT-bg_image.width)/2, (DISPLAY_LONG-bg_image.height)/2,
			bg_image.width, bg_image.height,
			bg_image.data, bg_image.size);
}

static void reset_screen(void)
{
	eink_job_t job;
	if(current_mode == MODE_BACKGROUND_FAST) {
		eink_job_begin(&job, 0);
		eink_job_add(job, bg_fast_buffer, WAVEFORM_MODE_GC, UPDATE_MODE_FULL);
		eink_job_commit(job);
		while(eink_job_count_pending() > 0) vTaskDelay(10/portTICK_RATE_MS);
		eink_job_begin(&job, 0);
		eink_job_add(job, bg_buffer, WAVEFORM_MODE_GC, UPDATE_MODE_INIT);
		eink_job_commit(job);
		while(eink_job_count_pending() > 0) vTaskDelay(10/portTICK_RATE_MS);
	} else {
		eink_job_begin(&job, 0);
		eink_job_add(job, bg_buffer, WAVEFORM_MODE_GC, UPDATE_MODE_FULL);
		eink_job_commit(job);
	}
}

static int pause_refreshing = 0;
static int dirty = 1;
void stop_refreshing(void) {
	pause_refreshing = 1;
	while(eink_job_count_pending() > 0) vTaskDelay(10/portTICK_RATE_MS);
}
void start_refreshing(void) {
	while(eink_job_count_pending() > 0) vTaskDelay(10/portTICK_RATE_MS);
	dirty = 1;
	pause_refreshing = 0;
}

void process_line(char *line, size_t length)
{
	(void)length;
	if(strncmp(line, "clear", 5) == 0) {
		stop_refreshing();
		reset_background();
		reset_screen();
		start_refreshing();
	} else if(strncmp(line, "init", 4) == 0) {
		stop_refreshing();
		eink_job_t job;
		eink_job_begin(&job, 0);
		eink_job_add(job, blank_buffer, WAVEFORM_MODE_INIT, UPDATE_MODE_FULL);
		eink_job_commit(job);
		if(current_mode == MODE_BACKGROUND_FAST) {
			reset_background();
			reset_screen();
		}
		start_refreshing();
	} else if(strncmp(line, "mode_blank", 10) == 0) {
		stop_refreshing();
		current_mode = MODE_BLANK;
		reset_background();
		reset_screen();  
		start_refreshing();
	} else if(strncmp(line, "mode_background_fast", 20) == 0) {
		stop_refreshing();
		current_mode = MODE_BACKGROUND_FAST;
		reset_background();
		reset_screen();
		start_refreshing();
	} else if(strncmp(line, "mode_background", 15) == 0) {
		stop_refreshing();
		current_mode = MODE_BACKGROUND;
		reset_background();
		reset_screen();
		start_refreshing();
	} else if(strncmp(line, "volt", 4) == 0) {
		power_get_battery_voltage();
		vTaskDelay(10);
		printf("%i\n", power_get_battery_voltage());
	} else {
		int x, y, r = 10, v = 0;
		if(sscanf(line, " %i , %i , %i , %i ", &x, &y, &r, &v) >= 2) {
			image_draw_circle(&bg_image, x, y, r, v, 1);
		}
	}
	dirty = 1;
}


static int event_loop_running = 0;
static void paint_draw_task(void *params)
{
	(void)params;
	char line[1024];
	unsigned int pos = 0, received;
	
	while(!event_loop_running) vTaskDelay(100/portTICK_RATE_MS);
	
	while(1) {
		received = vUSBRecvByte(line+pos, 1, portMAX_DELAY);
		if(received == 0) continue;
		if(line[pos] != '\n') printf("%c", line[pos]);
		if(line[pos] == '\n' || line[pos] == '\r') {
			printf("\n");
			line[pos+1] = 0;
			process_line(line, pos+1);
			pos = 0;
		} else if(pos+2 >= sizeof(line)) {
			printf("\n");
			line[pos+1] = 0;
			process_line(line, pos+1);
			pos = 0;
		} else pos++;
	}
	
}

static void paint_task(void *params)
{
	(void)params;
	int error, idle_time=0;
	vTaskDelay(100/portTICK_RATE_MS);
	
	/*
	 * bg_splash_image is in the flash
	 * blank_data, bg_data is in the microcontroller SDRAM
	 * blank_buffer, bg_buffer, bg_fast_buffer are in the display controller SDRAM
	 */
	
	int bg_image_loaded = 0;
	if(fat_init() == 0) {
		image_t test = &bg_image;
		portTickType start = xTaskGetTickCount();
		int res = image_load_pgm(&test, "test.pgm");
		portTickType stop = xTaskGetTickCount();
		printf("From card in %li ticks\n", stop-start);
		if(res != 0) {
			printf("Result: %i (%s)\n", -res, strerror(-res));
		} else {
			bg_image_loaded = 1;
		}
	}
	
	if(!bg_image_loaded) {
		error = image_unpack_splash(&bg_image, &bg_splash_image);
		if(error < 0) {
			printf("Error during splash unpack: %i (%s)\n", error, strerror(-error));
			led_halt_blinking(3);
		}
	}
	
	/* The white image needs to have 4 bpp since the controller will unpack 2bpp to
	 * 8bpp by appending 6 zero-bits. So 11b in 2 bpp gets 11000000b in 8 bpp, which
	 * is a slight grey (even with the P4N LUT mode).
	 */
	blank_image.bits_per_pixel = IMAGE_BPP_4;
	blank_image.rowstride = ROUND_UP(DISPLAY_SHORT, 4) / 2;
	
	error = image_create_solid(&blank_image, 0xff, DISPLAY_SHORT, DISPLAY_LONG);
	if(error < 0) {
		printf("Error during blank create: %i (%s)\n", error, strerror(-error));
		led_halt_blinking(3);
	}
	
	eink_mgmt_init(eink_mgmt_data, sizeof(eink_mgmt_data));

#if 0 /* Disabled since the flash image plus background image will overflow the AT91SAM7 flash */
	error = eink_flash_conditional_reflash();
	if(error > 0) { /* Not an error, but the controller was reinitialized and we must wait */
		vTaskDelay(5/portTICK_RATE_MS);
	} else if(error < 0) {
		paint_die_error(-error);
	}
#endif

	{
		int reset = (AT91C_BASE_RSTC->RSTC_RSR >> 8) & 0x7;
		const char * const reasons[] = {
				[0] = "PWR",
				[2] = "WDT",
				[3] = "Software reset",
				[4] = "USR",
				[5] = "Brownout reset",
		};
		if(reasons[reset] != NULL) printf("%s\n", reasons[reset]);
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
	if( (error=eink_image_buffer_acquire(&bg_buffer)) < 0) {
		printf("Reason: %i\nCould not acquire bg image buffer\n", error);
		led_halt_blinking(3);
	}
	if( (error=eink_image_buffer_acquire(&bg_fast_buffer)) < 0) {
		printf("Reason: %i\nCould not acquire bg fast image buffer\n", error);
		led_halt_blinking(3);
	}
	
	error = 0;
	
	portTickType start = xTaskGetTickCount(), stop, cumulative=0;
	error |= eink_image_buffer_load(blank_buffer, image_get_bpp_as_pack_mode(&blank_image), ROTATION_MODE_90,
			blank_image.data, blank_image.rowstride*blank_image.height) ;
	stop = xTaskGetTickCount();
	printf("Blank image: %li\n", (long)(stop-start));
	cumulative += stop-start;
	
	start = xTaskGetTickCount();
	error |= eink_image_buffer_load(bg_buffer, image_get_bpp_as_pack_mode(&blank_image), ROTATION_MODE_90,
			blank_image.data, blank_image.rowstride*blank_image.height);
	stop = xTaskGetTickCount();
	printf("Blank image: %li\n", (long)(stop-start));
	cumulative += stop-start;

	start = xTaskGetTickCount();
	error |= eink_image_buffer_load(bg_fast_buffer, image_get_bpp_as_pack_mode(&blank_image), ROTATION_MODE_90,
			blank_image.data, blank_image.rowstride*blank_image.height);
	stop = xTaskGetTickCount();
	printf("Blank image: %li\n", (long)(stop-start));
	cumulative += stop-start;

	start = xTaskGetTickCount();
	error |= eink_image_buffer_load_area(bg_fast_buffer, image_get_bpp_as_pack_mode(&bg_image), ROTATION_MODE_90,
			(DISPLAY_SHORT-bg_image.width)/2, (DISPLAY_LONG-bg_image.height)/2,
			bg_image.width, bg_image.height,
			bg_image.data, bg_image.size);
	stop = xTaskGetTickCount();
	printf("Background image: %li\n", (long)(stop-start));
	cumulative += stop-start;

	/* We unpack the splash image even when we're going to overwrite it with white, in order
	 * to set up the width, height, rowstride and size.
	 */
	if(current_mode == MODE_BLANK || current_mode == MODE_BACKGROUND_FAST)
		reset_background();

	start = xTaskGetTickCount();
	error |= eink_image_buffer_load_area(bg_buffer, image_get_bpp_as_pack_mode(&bg_image), ROTATION_MODE_90,
			(DISPLAY_SHORT-bg_image.width)/2, (DISPLAY_LONG-bg_image.height)/2,
			bg_image.width, bg_image.height,
			bg_image.data, bg_image.size);
	stop = xTaskGetTickCount();
	printf("Background image: %li\n", (long)(stop-start));
	cumulative += stop-start;

	printf("All images loaded in %li ticks\n", (long)(cumulative));

	if(error >= 0) {
		printf("All images loaded ok\n");
		eink_job_t job;
		eink_job_begin(&job, 0);
		eink_job_add(job, blank_buffer, WAVEFORM_MODE_INIT, UPDATE_MODE_INIT);
		eink_job_commit(job);
		while(eink_job_count_pending() > 0) vTaskDelay(10/portTICK_RATE_MS);
	} else {
		printf("Image load error %i: %s\n", error, strerror(-error));
	}
	int i=0;
	
	reset_screen();
	
	event_t received_event;
	event_loop_running = 1;
	
	portTickType last_draw = 0, now;
#define WAIT_TIME 50
#define MAX_IDLE ((10 * 60 * 1000) / WAIT_TIME)
	while(1) {
		received_event.class = EVENT_NONE;
		event_receive(&received_event, WAIT_TIME/portTICK_RATE_MS);
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
		if(idle_time > MAX_IDLE) {
			event_send(EVENT_POWEROFF, 0);
		}
		
		if(!pause_refreshing) {
			now = xTaskGetTickCount();
			if(last_draw - now > 100 && dirty) {
				dirty = 0;
				/* Download the full bg_image every 100ms and send a partial update for all changed pixels */
				error = eink_image_buffer_load_area(bg_buffer, image_get_bpp_as_pack_mode(&bg_image), ROTATION_MODE_90,
						(DISPLAY_SHORT-bg_image.width)/2, (DISPLAY_LONG-bg_image.height)/2,
						bg_image.width, bg_image.height,
						bg_image.data, bg_image.size);
				if(!pause_refreshing) { /* Might have been set in the mean time */ 
					if(error == 0) {
						eink_job_t job;
						eink_job_begin(&job, 2);
						switch(current_mode) {
						case MODE_BLANK:
						case MODE_BACKGROUND_FAST:
							eink_job_add_area(job, bg_buffer, WAVEFORM_MODE_DU, UPDATE_MODE_PART_SPECIAL,
									(DISPLAY_SHORT-bg_image.width)/2, (DISPLAY_LONG-bg_image.height)/2,
									bg_image.width, bg_image.height);
							break;
						case MODE_BACKGROUND:
							eink_job_add_area(job, bg_buffer, WAVEFORM_MODE_GC, UPDATE_MODE_PART_SPECIAL,
									(DISPLAY_SHORT-bg_image.width)/2, (DISPLAY_LONG-bg_image.height)/2,
									bg_image.width, bg_image.height);
							break;
						}
						eink_job_commit(job);
						last_draw = now;
					} else printf("E\n");
				}
			}
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

	xTaskCreate(paint_task, (signed portCHAR *) "PAINT TASK", TASK_PAINT_STACK,
			NULL, TASK_PAINT_PRIORITY, NULL);
	xTaskCreate(paint_draw_task, (signed portCHAR *) "PAINT DRAW TASK", TASK_PAINT_STACK,
			NULL, TASK_PAINT_PRIORITY+1, NULL);

	return 0;
}
