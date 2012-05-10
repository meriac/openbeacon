/***************************************************************
 *
 * OpenBeacon.org - SPI interface handling
 *
 * Copyright 2009 Henryk Ploetz <henryk@ploetzli.ch>
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
#ifndef SPI_H_
#define SPI_H_

#include <semphr.h>

typedef void (*spi_completion_callback_t) (void *buf, unsigned int len,
										   portBASE_TYPE * xTaskWoken);

typedef struct
{
	int cs;
	u_int32_t config;
	struct
	{
		unsigned int open:1;
		unsigned int bus_exclusive:1;
		unsigned int delayed_cs_inactive:1;	/* Keep CS line active until the completion callback returns, 
											   then decide whether to continue keeping it active (if the completion callback scheduled a
											   'now' job) or shortly deactivate it before executing the next job */
		unsigned int now_job_scheduled:1;
		unsigned int blocking_job_active:1;	/* There is an ongoing call to spi_transceive_blocking(), decline
											   all calls to spi_transceive() since they would re-enable the interrupt. */
		unsigned int force_output_pin:1;	/* The output pin has been forced to a value. */
	} flags;
	unsigned int jobs_pending;
	xSemaphoreHandle completion_semaphore;
	spi_completion_callback_t callback;
	int callback_on_all_jobs;
} spi_device;

enum spi_flag
{
	SPI_FLAG_NONE = 0,
	SPI_FLAG_OPEN,
	SPI_FLAG_BUS_EXCLUSIVE,
	SPI_FLAG_DELAYED_CS_INACTIVE,
	SPI_FLAG_BLOCKING_JOB_ACTIVE,
};

extern int spi_open (spi_device * device, int cs, u_int32_t config);
extern int spi_change_config (spi_device * device, u_int32_t config);
extern int spi_get_flag (spi_device * device, enum spi_flag flag);
extern void spi_set_flag (spi_device * device, enum spi_flag flag,
						  unsigned int value);
extern int spi_set_callback (spi_device * device,
							 spi_completion_callback_t cb, int all_jobs);
extern int spi_close (spi_device * device);

extern int spi_start_bus_exclusive (spi_device * device);
extern void spi_stop_bus_exclusive (spi_device * device);

extern int spi_transceive (spi_device * device, void *buf, unsigned int len);
extern int spi_transceive_automatic_retry (spi_device * device, void *buf,
										   unsigned int len);
extern int spi_transceive_from_irq (spi_device * device, void *buf,
									unsigned int len, int now,
									portBASE_TYPE * xTaskWoken);
extern int spi_transceive_blocking (spi_device * device, void *buf,
									unsigned int len);
extern void spi_wait_for_completion (spi_device * device);

extern int spi_force_transmit_pin (spi_device * device, int level);
extern int spi_release_transmit_pin (spi_device * device);

#endif /*SPI_H_ */
