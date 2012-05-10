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
#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <beacontypes.h>
#include <errno.h>

#include <task.h>
#include <stdio.h>
#include <queue.h>
#include <semphr.h>

#include "spi.h"

/* AT91SAM7X SPI declaration */
#define AT91C_BASE_SPI AT91C_BASE_SPI0
#define AT91C_BASE_PDC_SPI AT91C_BASE_PDC_SPI0
#define AT91C_ID_SPI AT91C_ID_SPI0
#define AT91F_SPI_CfgPMC AT91F_SPI0_CfgPMC

static int initialized = 0;		/* boolean */
static int open_devices = 0;	/* bitmask */
static int bus_exclusive = 0;	/* bitmask */
static int last_device_on_bus = -1;

struct spi_job
{
	spi_device *device;
	void *source_buf;
	unsigned int source_len;
	void *destination_buf;
	unsigned int destination_len;
	enum
	{
		SPI_JOB_FREE = 0,
		SPI_JOB_RESERVED,
		SPI_JOB_WAITING,
		SPI_JOB_EXECUTING,
	} state;
};

#define MAX_PENDING_JOBS 16
static struct spi_job _jobs[MAX_PENDING_JOBS];
static struct spi_job *current_job;
static xQueueHandle job_queue;

//#define DEBUG_IRQ_STORM 30
#ifdef DEBUG_IRQ_STORM
static int deadmansbrake = 0;
#endif

static void
_spi_irq (void)
{
	static portBASE_TYPE xTaskWoken;
	xTaskWoken = pdFALSE;
	u_int32_t sr = AT91C_BASE_SPI->SPI_SR;

#ifdef DEBUG_IRQ_STORM
	if (deadmansbrake++ > DEBUG_IRQ_STORM)
	{
		debug_printf ("IRQ abort\n");
		AT91C_BASE_SPI->SPI_IDR = 0xFFFF;
		return;
	}
#endif

	if (sr & AT91C_SPI_TXEMPTY)
	{
		/* The SPI is ready for a new job */

		/* First, check whether there was an executing job (that has finished) and handle that */
		if (current_job != NULL)
		{
			current_job->device->jobs_pending--;
			current_job->device->flags.now_job_scheduled = 0;
			if (current_job->device->jobs_pending == 0)
			{
				xSemaphoreGiveFromISR (current_job->device->
									   completion_semaphore, &xTaskWoken);
				if (current_job->device->callback != NULL)
				{
					current_job->device->callback (current_job->
												   destination_buf,
												   current_job->
												   destination_len,
												   &xTaskWoken);
				}
			}
			else
			{
				if (current_job->device->callback != NULL
					&& current_job->device->callback_on_all_jobs)
				{
					current_job->device->callback (current_job->
												   destination_buf,
												   current_job->
												   destination_len,
												   &xTaskWoken);
				}
			}

			if (current_job->device->flags.delayed_cs_inactive)
			{
				if (current_job->device->flags.now_job_scheduled)
				{
					/* Do nothing */
				}
				else
				{
					AT91F_SPI_CfgCs (AT91C_BASE_SPI, current_job->device->cs,
									 current_job->device->config);
					AT91C_BASE_SPI->SPI_CR = AT91C_SPI_LASTXFER;
				}
			}

			current_job->state = SPI_JOB_FREE;
			current_job = NULL;
		}

		/* See if there's a new job */
		if (xQueueReceiveFromISR (job_queue, &current_job, &xTaskWoken) ==
			pdTRUE)
		{
			current_job->state = SPI_JOB_EXECUTING;
			last_device_on_bus = current_job->device->cs;
			/* Determine how the CS should be set for the new job. Three cases:
			 *  + bus exclusively assigned to one device,
			 *    nothing needs to be done (there should be no jobs for other devices in the queue)
			 *  + bus not exclusive, but already set to the correct CS,
			 *    nothing needs to be done
			 *  + bus not exclusive, already set to a different CS,
			 *    need to set new CS
			 *    sub-case: if delayed_cs_inactive is set: configure CS for CSAAT
			 */
			if (bus_exclusive != 0)
			{
				/* Nothing */
			}
			else
			{
				if (current_job->device->flags.delayed_cs_inactive)
				{
					AT91F_SPI_CfgCs (AT91C_BASE_SPI, current_job->device->cs,
									 current_job->device->
									 config | AT91C_SPI_CSAAT);
				}
				if (((~(AT91C_BASE_SPI->SPI_MR >> 16)) & 0xF) ==
					(unsigned) 1 << (current_job->device->cs))
				{
					/* Nothing */
				}
				else
				{
					/* Set new CS */
					AT91C_BASE_SPI->SPI_MR =
						(AT91C_BASE_SPI->SPI_MR & ~(0x000F0000)) |
						((~(1L << (current_job->device->cs + 16))) &
						 0x000F0000);
				}
			}
			/* Set up and start transfer */
			AT91C_BASE_PDC_SPI->PDC_PTCR =
				AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS;
			AT91F_SPI_SendFrame (AT91C_BASE_SPI, current_job->source_buf,
								 current_job->source_len, NULL, 0);
			AT91F_SPI_ReceiveFrame (AT91C_BASE_SPI,
									current_job->destination_buf,
									current_job->destination_len, NULL, 0);
			AT91C_BASE_PDC_SPI->PDC_PTCR = AT91C_PDC_RXTEN | AT91C_PDC_TXTEN;
		}
		else
		{
			/* No new job, disable IRQ */
			AT91C_BASE_SPI->SPI_IDR = AT91C_SPI_TXEMPTY;
		}
	}

	AT91F_AIC_AcknowledgeIt ();
	if (xTaskWoken)
	{
		portYIELD_FROM_ISR ();
	}
}

static void __attribute__ ((naked)) spi_irq (void)
{
	portSAVE_CONTEXT ();
	_spi_irq ();
	portRESTORE_CONTEXT ();
}


static struct spi_job *
get_free_job (void)
{
	int i;
	for (i = 0; i < MAX_PENDING_JOBS; i++)
	{
		if (_jobs[i].state == SPI_JOB_FREE)
		{
			_jobs[i].state = SPI_JOB_RESERVED;
			return &(_jobs[i]);
		}
	}
	return NULL;
}

static void
spi_transceive_prepare_internal (struct spi_job *job, spi_device * device,
								 void *buf, unsigned int len)
{
	job->destination_buf = buf;
	job->source_buf = buf;
	job->destination_len = len;
	job->source_len = len;
	job->device = device;
	job->state = SPI_JOB_WAITING;
}

int
spi_transceive (spi_device * device, void *buf, unsigned int len)
{
	if (!device->flags.open)
	{
		return -ENODEV;
	}
	taskENTER_CRITICAL ();
	if (bus_exclusive != 0 && !device->flags.bus_exclusive)
	{
		/* Another device holds the bus exclusively */
		taskEXIT_CRITICAL ();
		return -EAGAIN;
	}
	if (device->flags.blocking_job_active)
	{
		/* This device currently executes a blocking transceive */
		taskEXIT_CRITICAL ();
		return -EAGAIN;
	}
	struct spi_job *job = get_free_job ();
	taskEXIT_CRITICAL ();
	if (job == NULL)
		return -EAGAIN;
	spi_transceive_prepare_internal (job, device, buf, len);

	/* Put the job on the queue and enable the SSC TXEMPTY IRQ, the IRQ handler will do the rest */
	taskENTER_CRITICAL ();
	xSemaphoreTake (device->completion_semaphore, 0);
	device->jobs_pending++;
	if (xQueueSend (job_queue, &job, 0) != pdTRUE)
	{
		job->state = SPI_JOB_FREE;
		device->jobs_pending--;
		if (device->jobs_pending == 0)
			xSemaphoreGive (device->completion_semaphore);
		taskEXIT_CRITICAL ();
		return -EAGAIN;
	}
	else
		AT91C_BASE_SPI->SPI_IER = AT91C_SPI_TXEMPTY;
	taskEXIT_CRITICAL ();

	return 0;
}

int
spi_transceive_from_irq (spi_device * device, void *buf, unsigned int len,
						 int now, portBASE_TYPE * xTaskWoken)
{
	if (!device->flags.open)
	{
		return -ENODEV;
	}
	if (now && device->cs != last_device_on_bus)
		return -EACCES;

	struct spi_job *job = get_free_job ();
	if (job == NULL)
		return -EAGAIN;
	spi_transceive_prepare_internal (job, device, buf, len);

	/* Put the job on the queue and enable the SSC TXEMPTY IRQ, the IRQ handler will do the rest */
	xQueueReceiveFromISR (device->completion_semaphore, NULL, xTaskWoken);		/* Like xSemaphoreTake, but ISR safe */
	device->jobs_pending++;
	portBASE_TYPE r;
	if (now)
	{
		r = xQueueSendToFrontFromISR (job_queue, &job, xTaskWoken);
		device->flags.now_job_scheduled = 1;
	}
	else
	{
		r = xQueueSendFromISR (job_queue, &job, xTaskWoken);
	}
	if (r != pdTRUE)
	{
		job->state = SPI_JOB_FREE;
		device->jobs_pending--;
		if (device->jobs_pending == 0)
			xSemaphoreGiveFromISR (device->completion_semaphore, xTaskWoken);
		return -EAGAIN;
	}
	else
		AT91C_BASE_SPI->SPI_IER = AT91C_SPI_TXEMPTY;

	return 0;
}

int
spi_transceive_automatic_retry (spi_device * device, void *buf,
								unsigned int len)
{
	int r;
	while ((r = spi_transceive (device, buf, len)) == -EAGAIN)
		vTaskDelay (1);
	return r;
}

int
spi_transceive_blocking (spi_device * device, void *buf, unsigned int len)
{
	if (!device->flags.open)
	{
		return -ENODEV;
	}

	taskENTER_CRITICAL ();
	if (!device->flags.bus_exclusive)
	{
		taskEXIT_CRITICAL ();
		return -EACCES;
	}
	if (current_job != NULL)
	{
		taskEXIT_CRITICAL ();
		return -EAGAIN;
	}
	if (device->flags.blocking_job_active)
	{
		taskEXIT_CRITICAL ();
		return -EAGAIN;
	}

	device->flags.blocking_job_active = 1;
	device->jobs_pending++;
	/* Interrupts can be re-enabled now for the benefit of other interrupt handlers. The SPI interrupt will
	 * stay disabled since a) the current device is bus_exclusive, so no other devices will be able to schedule
	 * jobs, and b) spi_transceive() will refuse to run, until the blocking_job_active flag is cleared.
	 */
	taskEXIT_CRITICAL ();

	/* Set up and start transfer */
	AT91C_BASE_PDC_SPI->PDC_PTCR = AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS;
	AT91F_SPI_SendFrame (AT91C_BASE_SPI, buf, len, NULL, 0);
	AT91F_SPI_ReceiveFrame (AT91C_BASE_SPI, buf, len, NULL, 0);
	AT91C_BASE_PDC_SPI->PDC_PTCR = AT91C_PDC_RXTEN | AT91C_PDC_TXTEN;

	/* Blocking wait for the end of transfer */
	while (!(AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TXEMPTY));

	taskENTER_CRITICAL ();

	device->flags.blocking_job_active = 0;
	device->jobs_pending--;
	if (device->jobs_pending == 0)
		xSemaphoreGive (device->completion_semaphore);

	taskEXIT_CRITICAL ();

	return 0;
}

/* Force the transmit pin to a certain value. Useful when you must transmit some idle
 * state, but don't want to have a memory buffer filled with that value for use in 
 * transceive operations. This decouples the output pin from the SPI peripheral
 * and sets a fixed value through the PIO controller.
 * You can only call this, when the device is bus exclusive.
 */
int
spi_force_transmit_pin (spi_device * device, int level)
{
	if (!device->flags.open)
	{
		return -ENODEV;
	}
	if (!device->flags.bus_exclusive)
	{
		return -EACCES;
	}

	if (level)
	{
		AT91F_PIO_SetOutput (SD_SPI_PIO, SD_SPI_MOSI);
	}
	else
	{
		AT91F_PIO_ClearOutput (SD_SPI_PIO, SD_SPI_MOSI);
	}
	AT91F_PIO_CfgOutput (SD_SPI_PIO, SD_SPI_MOSI);
	device->flags.force_output_pin = 1;
	return 0;
}

int
spi_release_transmit_pin (spi_device * device)
{
	if (!device->flags.open)
	{
		return -ENODEV;
	}
	if (device->flags.force_output_pin)
	{
		AT91F_PIO_CfgPeriph (SD_SPI_PIO, SD_SPI_MOSI, 0);
	}
	device->flags.force_output_pin = 0;
	return 0;
}


static void
spi_init (void)
{
	bus_exclusive = 0;
	job_queue = xQueueCreate (MAX_PENDING_JOBS, sizeof (&(_jobs[0])));

	AT91F_SPI_CfgPMC ();
	AT91F_PIO_CfgPeriph (SD_SPI_PIO, SD_SPI_MISO | SD_SPI_MOSI | SD_SPI_SCK,
						 0);

	AT91C_BASE_SPI->SPI_CR = AT91C_SPI_SWRST;
	AT91C_BASE_SPI->SPI_MR =
		AT91C_SPI_MSTR | AT91C_SPI_PS_FIXED | AT91C_SPI_MODFDIS;

	AT91C_BASE_SPI->SPI_IDR = 0xFFFF;
	AT91F_AIC_ConfigureIt (AT91C_ID_SPI, 4, AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL,
						   spi_irq);
	AT91F_SPI_Enable (AT91C_BASE_SPI);
	AT91F_AIC_EnableIt (AT91C_ID_SPI);

	initialized = 1;
}

static void
spi_deinit (void)
{
	initialized = 0;
	AT91F_AIC_DisableIt (AT91C_ID_SPI);
	AT91F_SPI_Disable (AT91C_BASE_SPI);
	vQueueDelete (job_queue);
	current_job = NULL;
}

int
spi_open (spi_device * device, int cs, u_int32_t config)
{
	taskENTER_CRITICAL ();
	if (!initialized)
		spi_init ();

	if (open_devices & (1L << cs))
	{
		taskEXIT_CRITICAL ();
		return -EADDRINUSE;
	}

	AT91F_SPI_CfgCs (AT91C_BASE_SPI, cs, config);
	device->cs = cs;
	device->config = config;
	device->flags.open = 1;
	device->flags.bus_exclusive = 0;
	device->flags.delayed_cs_inactive = 0;
	device->jobs_pending = 0;
	device->callback = NULL;
	device->callback_on_all_jobs = 0;
	vSemaphoreCreateBinary (device->completion_semaphore);

	open_devices |= 1L << cs;
	taskEXIT_CRITICAL ();
	return 0;
}

int
spi_change_config (spi_device * device, u_int32_t config)
{
	taskENTER_CRITICAL ();
	if (!device->flags.open)
	{
		taskEXIT_CRITICAL ();
		return -ENODEV;
	}
	if (device->flags.bus_exclusive)
	{
		AT91F_SPI_CfgCs (AT91C_BASE_SPI, device->cs,
						 config | AT91C_SPI_CSAAT);
	}
	else
	{
		AT91F_SPI_CfgCs (AT91C_BASE_SPI, device->cs, config);
	}
	device->config = config;
	taskEXIT_CRITICAL ();
	return 0;
}

int
spi_get_flag (spi_device * device, enum spi_flag flag)
{
	switch (flag)
	{
		case SPI_FLAG_NONE:
			return 0;
		case SPI_FLAG_OPEN:
			return device->flags.open;
		case SPI_FLAG_BUS_EXCLUSIVE:
			return device->flags.bus_exclusive;
		case SPI_FLAG_DELAYED_CS_INACTIVE:
			return device->flags.delayed_cs_inactive;
		case SPI_FLAG_BLOCKING_JOB_ACTIVE:
			return device->flags.blocking_job_active;
	}
	return 0;
}

void
spi_set_flag (spi_device * device, enum spi_flag flag, unsigned int value)
{
	switch (flag)
	{
		case SPI_FLAG_DELAYED_CS_INACTIVE:
			device->flags.delayed_cs_inactive = value;
		default:
			return;
	}
}

int
spi_set_callback (spi_device * device, spi_completion_callback_t cb,
				  int all_jobs)
{
	if (device == NULL)
		return -EINVAL;
	if (!device->flags.open)
		return -EINVAL;
	taskENTER_CRITICAL ();
	device->callback = cb;
	device->callback_on_all_jobs = all_jobs;
	taskEXIT_CRITICAL ();
	return 0;
}

int
spi_close (spi_device * device)
{
	taskENTER_CRITICAL ();
	if (device->jobs_pending > 0)
	{
		taskEXIT_CRITICAL ();
		return -EBUSY;
	}
	if (device->flags.bus_exclusive)
	{
		spi_stop_bus_exclusive (device);
	}
	device->flags.open = 0;
	open_devices &= ~(1L << device->cs);
	vQueueDelete (device->completion_semaphore);

	if (open_devices == 0)
	{
		spi_deinit ();
	}
	taskEXIT_CRITICAL ();
	return 0;
}


int
spi_start_bus_exclusive (spi_device * device)
{
	taskENTER_CRITICAL ();
	if (bus_exclusive != 0)
	{
		taskEXIT_CRITICAL ();
		return -EBUSY;
	}

	/* Make sure that no jobs are currently executing or pending */
	if (current_job != NULL || uxQueueMessagesWaiting (job_queue) > 0)
	{
		taskEXIT_CRITICAL ();
		return -EBUSY;
	}

	AT91F_SPI_CfgCs (AT91C_BASE_SPI, device->cs,
					 device->config | AT91C_SPI_CSAAT);
	AT91C_BASE_SPI->SPI_MR =
		(AT91C_BASE_SPI->SPI_MR & ~(0x000F0000)) |
		((~(1L << (device->cs + 16))) & 0x000F0000);

	bus_exclusive |= 1L << device->cs;
	device->flags.bus_exclusive = 1;
	taskEXIT_CRITICAL ();
	return 0;
}

void
spi_stop_bus_exclusive (spi_device * device)
{
	taskENTER_CRITICAL ();
	if (bus_exclusive & (1L << device->cs))
	{
		if (device->flags.force_output_pin)
		{
			spi_release_transmit_pin (device);
		}

		AT91F_SPI_CfgCs (AT91C_BASE_SPI, device->cs, device->config);

		device->flags.bus_exclusive = 0;
		bus_exclusive &= ~(1L << device->cs);
	}
	taskEXIT_CRITICAL ();
}

void
spi_wait_for_completion (spi_device * device)
{
	while (1)
	{
		taskENTER_CRITICAL ();
		if (!device->flags.open || device->jobs_pending == 0)
		{
			taskEXIT_CRITICAL ();
			return;
		}
		taskEXIT_CRITICAL ();
		if (xSemaphoreTake
			(device->completion_semaphore, 5000 / portTICK_RATE_MS) == pdTRUE)
		{
			taskENTER_CRITICAL ();
			if (device->jobs_pending == 0)
			{
				xSemaphoreGive (device->completion_semaphore);
			}
			taskEXIT_CRITICAL ();
			return;
		}
	}
}
