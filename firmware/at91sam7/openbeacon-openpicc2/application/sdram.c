#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <beacontypes.h>

#include <stdint.h>

#include "sdram.h"

static void sdram_setup_pio(void)
{
	switch(BOARD->identifier) {
	case BOARD_V0_1:
		AT91F_PIO_CfgPeriph(AT91C_BASE_PIOA, 0,
				(1L<<2)|(1L<<5)|(1L<<8)|(1L<<9)|(1L<<10)|(0x1ffL<<23) );
		AT91F_PIO_CfgPeriph(AT91C_BASE_PIOB, 0,
				(1L<<3)|(1L<<4)|(1L<<6)|(1L<<7)|(1L<<8)|(1L<<11)|(1L<<13)|(0xffffL<<16));
		AT91F_PIO_CfgPeriph(AT91C_BASE_PIOC,
				0xFFFFL, 0);
		break;
	case BOARD_V0_2:
		AT91F_PIO_CfgPeriph(AT91C_BASE_PIOA, 0,
				(7L)|(1L<<5)|(1L<<9)|(1L<<10)|(1L<<18)|(0x1ffL<<23) );
		AT91F_PIO_CfgPeriph(AT91C_BASE_PIOB, 0,
				(1L<<3)|(1L<<4)|(1L<<6)|(1L<<7)|(1L<<8)|(1L<<11)|(1L<<13)|(0xffffL<<16));
		AT91F_PIO_CfgPeriph(AT91C_BASE_PIOC,
				0xFFFFL, 0);
		break;
	}
}

//*----------------------------------------------------------------------------
//* \fn AT91F_InitSdram
//* \brief Init EBI and SDRAM controller for MT48LC16M16A2
//*----------------------------------------------------------------------------
void sdram_init(void)
{
	volatile unsigned int i;
	volatile unsigned int *pSdram = (unsigned int *) SDRAM_BASE;
	AT91PS_SDRC psdrc = AT91C_BASE_SDRC;
	// Init the EBI for SDRAM
	AT91C_BASE_EBI->EBI_CSA = AT91C_EBI_CS1A_SDRAMC;	// Chip Select is assigned to SDRAM
	// controller
	//Configure PIO for EBI CS1
	sdram_setup_pio();
	//*** Step 1 ***
	// Set Configuration Register
	psdrc->SDRC_CR = AT91C_SDRC_NC_8 |	// 8 bits Column Addressing: 256 (A0-A7)
		// AT91C_SDRC_NC_9
		AT91C_SDRC_NR_12 |		// 12 bits Row Addressing: 4Ki (A0-A11)
		// AT91C_SDRC_NR_13
		AT91C_SDRC_CAS_2 |		// SDRAMC only supports CAS 2
		AT91C_SDRC_NB_4_BANKS |	// 4 banks
		/* TWR */ (2L<<7)  | // Minimum value is 2 clocks
		/* TRC */ (4L<<11) |
		/* TRP */ (2L<<15) | // Minimum value is 2 clocks
		/* TRCD */(2L<<19) |
		/* TRAS */(3L<<23) |
		/* TXSR */(4L<<27);
	//*** Step 2 ***
	// Wait 200us
	for(i=0;i<9600; i++);
	//*** Step 3 ***
	// NOP Command
	psdrc->SDRC_MR = AT91C_SDRC_DBW_32_BITS | AT91C_SDRC_MODE_NOP_CMD;	// Set NOP
	pSdram[0] = 0x00000000;	// Perform NOP

	//*** Step 4 *** (micron step 6)
	//All Banks Precharge Command
	psdrc->SDRC_MR = AT91C_SDRC_DBW_32_BITS | 0x00000002;	// Set PRCHG AL
	pSdram[0] = 0x00000000;	// Perform PRCHG

	// micron step 7
	for(i=0;i<1;i++) ;

	//*** Step 5 *** (micron step 8)
	//8 Refresh Command
	psdrc->SDRC_MR = AT91C_SDRC_DBW_32_BITS | AT91C_SDRC_MODE_RFSH_CMD;	// Set 1st CBR
	pSdram[1] = 0x00000001;	// Perform CBR
	for(i=0;i<2;i++) ;
	psdrc->SDRC_MR = AT91C_SDRC_DBW_32_BITS | AT91C_SDRC_MODE_RFSH_CMD;	// Set 2nd CBR
	pSdram[2] = 0x00000002;	// Perform CBR
	for(i=0;i<2;i++) ;
	psdrc->SDRC_MR = AT91C_SDRC_DBW_32_BITS | AT91C_SDRC_MODE_RFSH_CMD;	// Set 3rd CBR
	pSdram[3] = 0x00000003;	// Perform CBR
	for(i=0;i<2;i++) ;
	psdrc->SDRC_MR = AT91C_SDRC_DBW_32_BITS | AT91C_SDRC_MODE_RFSH_CMD;	// Set 4th CBR
	pSdram[4] = 0x00000004;	// Perform CBR
	for(i=0;i<2;i++) ;
	psdrc->SDRC_MR = AT91C_SDRC_DBW_32_BITS | AT91C_SDRC_MODE_RFSH_CMD;	// Set 5th CBR
	pSdram[5] = 0x00000005;	// Perform CBR
	for(i=0;i<2;i++) ;
	psdrc->SDRC_MR = AT91C_SDRC_DBW_32_BITS | AT91C_SDRC_MODE_RFSH_CMD;	// Set 6th CBR
	pSdram[6] = 0x00000006;	// Perform CBR
	for(i=0;i<2;i++) ;
	psdrc->SDRC_MR = AT91C_SDRC_DBW_32_BITS | AT91C_SDRC_MODE_RFSH_CMD;	// Set 7th CBR
	pSdram[7] = 0x00000007;	// Perform CBR
	for(i=0;i<2;i++) ;
	psdrc->SDRC_MR = AT91C_SDRC_DBW_32_BITS | AT91C_SDRC_MODE_RFSH_CMD;	// Set 8th CBR
	pSdram[8] = 0x00000008;	// Perform CBR
	for(i=0;i<2;i++) ;

	//*** Step 6 *** (micron step 12)
	//Mode Register Command
	psdrc->SDRC_MR = AT91C_SDRC_DBW_32_BITS | AT91C_SDRC_MODE_LMR_CMD;	// Set LMR operation
	((uint32_t*)(SDRAM_BASE))[SDRAM_MODE] = 0x0;	// Perform LMR burst=1, lat=2
	for(i=0;i<2;i++) ;

	//*** Step 7 ***
	//Normal Mode Command
	psdrc->SDRC_MR = AT91C_SDRC_DBW_32_BITS | AT91C_SDRC_MODE_NORMAL_CMD;	// Set Normal mode
	// 32 bits
	pSdram[0] = 0x00000000;	// Perform Normal mode
	//*** Step 8 ***
	// Set Refresh Timer
	psdrc->SDRC_TR = SDRAM_TR_TIME;
}
