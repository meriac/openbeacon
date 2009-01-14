#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <beacontypes.h>

#include <semphr.h>
#include <task.h>

#include <stdio.h>

#include "pio_irq.h"
#include "eink/eink.h"
#include "eink/eink_lowlevel.h"

#define EINK_UPDATE_BUFFER_START 0x0
#define EINK_IMAGE_BUFFER_START (600*800*2)

#define EINK_READY() AT91F_PIO_IsInputSet(FPC_NHRDY_PIO, FPC_NHRDY_PIN)
/* Note that the address bits are not decoded */ 
volatile u_int16_t *eink_base = (volatile u_int16_t*)0x10000000; 

#define EINK_BEGIN_COMMAND() AT91F_PIO_ClearOutput(FPC_HDC_PIO, FPC_HDC_PIN);
#define EINK_END_COMMAND() AT91F_PIO_SetOutput(FPC_HDC_PIO, FPC_HDC_PIN);

static xSemaphoreHandle eink_irq_semaphore;
static portBASE_TYPE eink_irq(AT91PS_PIO pio, u_int32_t pin, portBASE_TYPE xTaskWoken)
{
	(void)pio; (void)pin;
	/* Wake up the main thread, then disable IRQ */
	xSemaphoreGiveFromISR(eink_irq_semaphore, &xTaskWoken);
	pio_irq_disable(FPC_HIRQ_PIO, FPC_HIRQ_PIN);
	return xTaskWoken;
}

static void __attribute__((unused)) eink_wait_for_irq(void)
{
	pio_irq_enable(FPC_HIRQ_PIO, FPC_HIRQ_PIN);
	xSemaphoreTake(eink_irq_semaphore, portMAX_DELAY);
}

/* in eink_mgmt.c, not public */
extern void eink_mgmt_flush_queue(void);
static void eink_main_task(void *parameter)
{
	(void)parameter;
	while(1) {
#if 0
		eink_wait_for_irq();
		printf("eInk IRQ\n");
#endif
		eink_mgmt_flush_queue();
		vTaskDelay(10);
	}
}

void eink_wait_for_completion(void) 
{
	while(!EINK_READY()) ;
}

void eink_wait_display_idle(void)
{
	eink_perform_command(EINK_CMD_WAIT_DSPE_TRG, 0, 0, 0, 0);
	eink_perform_command(EINK_CMD_WAIT_DSPE_FREND, 0, 0, 0, 0);
	eink_wait_for_completion();
}

/* Call eink_wait_display_idle() afterwards, if necessary */
void eink_display_update_full(u_int16_t mode)
{
	u_int16_t tmp[] = { mode };
	eink_perform_command(EINK_CMD_UPD_FULL, tmp, 1, 0, 0);
}

/* Call eink_wait_display_idle() afterwards, if necessary */
void eink_display_update_full_area(u_int16_t mode, u_int16_t x, u_int16_t y, u_int16_t width, u_int16_t height)
{
	u_int16_t tmp[] = { mode,  x, y, width, height};
	eink_perform_command(EINK_CMD_UPD_FULL_AREA, tmp, 5, 0, 0);
}

/* Call eink_wait_display_idle() afterwards, if necessary */
void eink_display_update_part(u_int16_t mode)
{
	u_int16_t tmp[] = { mode };
	eink_perform_command(EINK_CMD_UPD_PART, tmp, 1, 0, 0);
}

/* Call eink_wait_display_idle() afterwards, if necessary */
void eink_display_update_part_area(u_int16_t mode, u_int16_t x, u_int16_t y, u_int16_t width, u_int16_t height)
{
	u_int16_t tmp[] = { mode,  x, y, width, height};
	eink_perform_command(EINK_CMD_UPD_PART_AREA, tmp, 5, 0, 0);
}

static u_int16_t streamed_checksum;
static void eink_burst_write_begin(void)
{
	/* Zero the checksum register */
	eink_write_register(0x156, 0);
	streamed_checksum = 0;
	
	/* Burst Write, basically the same as writing the whole image to register 0x154 */
	EINK_BEGIN_COMMAND();
	eink_base[0] = EINK_CMD_WR_REG;
	EINK_END_COMMAND();
	eink_base[1] = 0x154;
	eink_wait_for_completion();
}

static void eink_burst_write_with_checksum(const unsigned char *data, unsigned int length)
{
	const u_int16_t *sendbuf = (const u_int16_t*)data;
	unsigned int i;
	for(i=0; i<length/2; i++) {
		streamed_checksum += *sendbuf;
		eink_base[0] = *(sendbuf++);
	}
}

static int eink_burst_write_check_checksum(void)
{
	return streamed_checksum == eink_read_register(0x156);
}

int eink_display_raw_image(const unsigned char *data, unsigned int length)
{
	
	eink_perform_command(EINK_CMD_BST_END_SDR, 0, 0, 0, 0);
	u_int16_t params[] = {EINK_IMAGE_BUFFER_START & 0xFFFF, EINK_IMAGE_BUFFER_START >> 16, 
			(length/2) & 0xFFFF, (length/2) >> 16};
	eink_perform_command(EINK_CMD_BST_WR_SDR, params, 4, 0, 0);
	eink_wait_for_completion();
	
	eink_burst_write_begin();
	
	eink_burst_write_with_checksum(data, length);
	
	eink_perform_command(EINK_CMD_BST_END_SDR, 0, 0, 0, 0);
	eink_wait_for_completion();
	
	return eink_burst_write_check_checksum();
}

void eink_init_rotmode(u_int16_t rotation_mode)
{
	const u_int16_t rotmode[] = { rotation_mode };
	eink_perform_command(EINK_CMD_INIT_ROTMODE, rotmode, 1, 0, 0);
}

void eink_set_load_address(u_int32_t load_address)
{
	u_int16_t buf[] = { load_address & 0xFFFF, (load_address >> 16) & 0xFFFF};
	eink_perform_command(EINK_CMD_LD_IMG_SETADR, buf, 2, 0, 0);
	eink_wait_for_completion();
}

void eink_set_display_address(u_int32_t display_address)
{
	u_int16_t buf[] = { display_address & 0xFFFF, (display_address >> 16) & 0xFFFF};
	eink_perform_command(EINK_CMD_UPD_SET_IMGADR, buf, 2, 0, 0);
	eink_wait_for_completion();
}

int eink_display_packed_image(u_int16_t packmode, const unsigned char *data, unsigned int length)
{
	eink_display_streamed_image_begin(packmode);
	eink_display_streamed_image_update(data, length);
	return eink_display_streamed_image_end();
}

void eink_display_streamed_area_begin(u_int16_t packmode, u_int16_t xstart, u_int16_t ystart, 
		u_int16_t width, u_int16_t height)
{
	u_int16_t buf[] = {packmode, xstart, ystart, width, height};
	eink_perform_command(EINK_CMD_LD_IMG_AREA, buf, 5, 0, 0);
	eink_wait_for_completion();
	
	eink_burst_write_begin();
}

void eink_display_streamed_image_begin(u_int16_t packmode)
{
	eink_perform_command(EINK_CMD_LD_IMG, &packmode, 1, 0, 0);
	eink_wait_for_completion();
	
	eink_burst_write_begin();
}

void eink_display_streamed_image_update(const unsigned char *data, unsigned int length)
{
	eink_burst_write_with_checksum(data, length);
}

int eink_display_streamed_image_end(void)
{
	eink_perform_command(EINK_CMD_LD_IMG_END, 0, 0, 0, 0);
	eink_wait_for_completion();
	
	return eink_burst_write_check_checksum();
}

#if 0
void eink_dump_raw_image(void)
{
	unsigned int length = 100;
	u_int16_t tmp[length/2];
	eink_perform_command(EINK_CMD_BST_END_SDR, 0, 0, 0, 0);
	u_int16_t params[] = {EINK_IMAGE_BUFFER_START & 0xFFFF, EINK_IMAGE_BUFFER_START >> 16, 
			(length/2) & 0xFFFF, (length/2) >> 16};
	eink_perform_command(EINK_CMD_BST_RD_SDR, params, 4, 0, 0);
	eink_wait_for_completion();
	
	/* Burst Write, basically the same as reading the whole image from register 0x154 */
	EINK_BEGIN_COMMAND();
	eink_base[0] = EINK_CMD_RD_REG;
	EINK_END_COMMAND();
	eink_base[1] = 0x154;
	eink_wait_for_completion();
	
	unsigned int i;
	for(i=0; i<length/2; i++) {
		tmp[i] = eink_base[0];
	}
	
	eink_perform_command(EINK_CMD_BST_END_SDR, 0, 0, 0, 0);
	eink_wait_for_completion();
	
	for(i=0; i<length/2; i++) {
		printf("%04X ", tmp[i]);
	}
	printf("\n");
	
	eink_write_register(0x140, 1L<<15); /* Reset host memory interface (recommended in spec after read) */
	eink_wait_for_completion();
}
#endif

u_int16_t eink_read_register(const u_int16_t reg)
{
	EINK_BEGIN_COMMAND();
	eink_base[0] = EINK_CMD_RD_REG;
	EINK_END_COMMAND();
	eink_base[1] = reg;
	return eink_base[2];
}

void eink_write_register(const u_int16_t reg, const u_int16_t value)
{
	EINK_BEGIN_COMMAND();
	eink_base[0] = EINK_CMD_WR_REG;
	EINK_END_COMMAND();
	eink_base[1] = reg;
	eink_base[2] = value;
}

int eink_perform_command(const u_int16_t command, 
		const u_int16_t * const params, const size_t params_len,
		u_int16_t * const response, const size_t response_len)
{
	unsigned int i;
	eink_wait_for_completion();
	EINK_BEGIN_COMMAND();
	eink_base[0] = command;
	EINK_END_COMMAND();
	for(i = 0; i<params_len; i++) eink_base[i] = params[i];
	for(i = 0; i<response_len; i++) response[i] = eink_base[i];
	return 0;
}

void eink_controller_reset(void)
{
	volatile int i;
	AT91F_PIO_ClearOutput(FPC_NRESET_PIO, FPC_NRESET_PIN);
	for(i=0; i<192000; i++) ;
	AT91F_PIO_SetOutput(FPC_NRESET_PIO, FPC_NRESET_PIN);
}

int eink_comm_test(u_int16_t reg1, u_int16_t reg2)
{
	/* Check the data bus: Write something to two registers, 
	 * read it back and verify that it's correct. (Also: make a backup of the old value
	 * and restore it afterwards.) */
	u_int16_t backup1 = eink_read_register(reg1),
		backup2 = eink_read_register(reg2);
	const u_int16_t test1 = 0x0123;
	const u_int16_t test2 = 0x4567;
	
	eink_write_register(reg1, test1);
	eink_write_register(reg2, test2);
	eink_read_register(reg1); /* Dummy read, fixme */
	if(eink_read_register(reg1) != test1) 
		return 0;
	if(eink_read_register(reg2) != test2) 
		return 0;
	
	eink_write_register(reg1, ~test2);
	eink_write_register(reg2, ~test1);
	if(eink_read_register(reg1) != (u_int16_t)~test2) 
		return 0;
	if(eink_read_register(reg2) != (u_int16_t)~test1) 
		return 0;
	
	eink_write_register(reg1, backup1);
	eink_write_register(reg2, backup2);
	return 1;
}

int eink_controller_init(void)
{
	u_int16_t product = eink_read_register(2);
	if(product != 0x0047) return -EINK_ERROR_NOT_DETECTED;
	
	u_int16_t revision = eink_read_register(0);
	if(revision != 0x0100) return -EINK_ERROR_NOT_SUPPORTED;
	
	
	eink_perform_command(EINK_CMD_INIT_SYS_RUN, 0, 0, 0, 0);
	eink_wait_for_completion();
	
	/* Perform comm test, use host memory count and checksum registers as scratch space */
	if(!eink_comm_test(0x148, 0x156))
		return -EINK_ERROR_COMMUNICATIONS_FAILURE;
	/* Now reset host memory interface */
	eink_write_register(0x140, 1L<<15);

	/* the following three commands are from the example code */ 
	eink_write_register(0x106, 0x203);
	
	const u_int16_t dspe_cfg[] = { BS60_INIT_HSIZE, BS60_INIT_VSIZE,
			BS60_INIT_SDRV_CFG, BS60_INIT_GDRV_CFG, BS60_INIT_LUTIDXFMT
	};
	eink_perform_command(EINK_CMD_INIT_DSPE_CFG, dspe_cfg, 5, 0, 0);

	const u_int16_t dspe_tmg[] = { BS60_INIT_FSLEN, ( BS60_INIT_FELEN << 8 ) | BS60_INIT_FBLEN,
			BS60_INIT_LSLEN, ( BS60_INIT_LELEN << 8 ) | BS60_INIT_LBLEN,
			BS60_INIT_PIXCLKDIV
	};
	eink_perform_command(EINK_CMD_INIT_DSPE_TMG, dspe_tmg, 5, 0, 0);
	
	const u_int16_t waveform_address[2] = {0x886, 0};
	eink_perform_command(EINK_CMD_RD_WFM_INFO, waveform_address, 2, 0, 0);
	
	eink_perform_command(EINK_CMD_UPD_GDRV_CLR, 0, 0, 0, 0);
	eink_perform_command(EINK_CMD_WAIT_DSPE_TRG, 0, 0, 0, 0);
	
	eink_wait_for_completion();
	
	u_int16_t status = eink_read_register(0x338);
	if(status & 0x08F00) return -EINK_ERROR_CHECKSUM_ERROR;
	
	eink_init_rotmode(EINK_ROTATION_MODE_90);
	
	/* from example code */
	eink_write_register(0x01A, 4);
	eink_write_register(0x320, 0);
	
	return 0;
}

/* Note: wait at least 4ms between eink_interface_init() and eink_controller_reset() */
void eink_interface_init(void)
{
	/* The eInk controller is connected to the External Bus Interface on NCS0 (PC23) */
	
	/* Reset for the eInk controller (PIO output) */
	AT91F_PIO_ClearOutput(FPC_NRESET_PIO, FPC_NRESET_PIN);
	AT91F_PIO_CfgOutput(FPC_NRESET_PIO, FPC_NRESET_PIN);
	
	/* According to the spec this pin must be set high */
	AT91F_PIO_SetOutput(FPC_NRMODE_PIO, FPC_NRMODE_PIN);
	AT91F_PIO_CfgOutput(FPC_NRMODE_PIO, FPC_NRMODE_PIN);
	
	/* High when the eInk controller is ready for a command (PIO input) */
	AT91F_PIO_CfgInput(FPC_NHRDY_PIO, FPC_NHRDY_PIN);
	
	/* HD/C selects between command and data (PIO output) */
	AT91F_PIO_SetOutput(FPC_HDC_PIO, FPC_HDC_PIN);
	AT91F_PIO_CfgOutput(FPC_HDC_PIO, FPC_HDC_PIN);
	
	/* D0-15, Write Enable, Read Enable and Chip Select go to the Static Memory Controller peripheral */
	AT91F_PIO_CfgPeriph(FPC_SMC_PIO, 0xFFFFL, FPC_SMC_PINS);
	
	/* 16-bit device on 16-bit interface, no extra float time, wait state enabled, no extra wait states,
	 * no extra setup or hold time. */
	AT91C_BASE_SMC->SMC2_CSR[0] = AT91C_SMC2_BAT | AT91C_SMC2_DBW_16 | (0L<<8) |
		AT91C_SMC2_WSEN | 5 | (0L<<24) | (0L<<28);
	
	pio_irq_init_once();
	vSemaphoreCreateBinary(eink_irq_semaphore);
	AT91F_PIO_CfgInput(FPC_HIRQ_PIO, FPC_HIRQ_PIN);
	pio_irq_register(FPC_HIRQ_PIO, FPC_HIRQ_PIN, eink_irq);
	xTaskCreate (eink_main_task, (signed portCHAR *) "EINK MAIN TASK", TASK_EINK_STACK, NULL, TASK_EINK_PRIORITY, NULL);
}
