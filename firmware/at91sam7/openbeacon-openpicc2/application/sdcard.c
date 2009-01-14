#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <beacontypes.h>
#include <errno.h>
#include <string.h>
#include <task.h>
#include <stdio.h>

#include "sdcard.h"
#include "spi.h"

/* FIXME: What's the correct CPOL/NCPHA setting? */
#define SDCARD_SPI_CONFIG(scbr) (AT91C_SPI_BITS_8 | AT91C_SPI_CPOL | \
			(scbr << 8) | (1L << 16) | (0L << 24))
static const int SCBR = ((int) (MCK / 5e6) + 1) & 0xFF;
static const int SCBR_INIT = ((int) (MCK / 4e5) + 1) & 0xFF;
static spi_device sdcard_spi;

static int sdcard_is_open;

struct sdcard_command_frame {
	unsigned int startmagic:2;
	unsigned int command:6;
	u_int8_t argument[4];
	unsigned int crc:7;
	unsigned int stopmagic:1;
} __attribute__((packed));

int sdcard_open_card(void)
{
	if(sdcard_is_open) return -EALREADY;
	
	int r;
	while( (r=spi_start_bus_exclusive(&sdcard_spi)) == -EAGAIN) 
		vTaskDelay(1);
	if(r != 0)
		return r;
	
	/* Transmit at least 74 clock cycles (round up to 10 bytes * 8 bits) with no data */
	u_int8_t no_data[10];
	memset(no_data, 0xff, sizeof(no_data));
	spi_transceive(&sdcard_spi, no_data, sizeof(no_data));
	spi_wait_for_completion(&sdcard_spi);
	
	/* Deassert CS and send 16 more clock cycles to confirm to the initialisation sequence.*/
	AT91F_PIO_SetOutput(SDCARD_CS_PIO, SDCARD_CS_PIN);
	AT91F_PIO_CfgOutput(SDCARD_CS_PIO, SDCARD_CS_PIN);
	memset(no_data, 0xff, 2);
	spi_transceive(&sdcard_spi, no_data, 2);
	spi_wait_for_completion(&sdcard_spi);
	AT91F_PIO_CfgPeriph(SDCARD_CS_PIO, SDCARD_CS_PIN, 0);
	
	/* Put card into idle state by sending CMD0 */
	
	printf("Finished waiting\n");
	return 0;
}

/* Must fill in command and argument of cmd struct, crc and magic get set here */
int sdcard_send_command(struct sdcard_command_frame *cmd, u_int8_t *response, size_t response_length)
{
	if(cmd == NULL) return -EINVAL;
	cmd->startmagic = 1;
	cmd->stopmagic = 1;
	cmd->crc = 0x4A; /* Correct CRC for a CMD0 command, maybe add calculation later */
	/* FIXME implement (if at all possible */
	(void)response; (void)response_length;
	return -1;
}

int sdcard_init(void)
{
	AT91F_PIO_CfgPeriph(SDCARD_CS_PIO, SDCARD_CS_PIN, 0);
	
	spi_open(&sdcard_spi, SDCARD_CS, SDCARD_SPI_CONFIG(SCBR_INIT));
	sdcard_is_open = 0;
	
	return 0;
}
