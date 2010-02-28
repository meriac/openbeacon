#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <beacontypes.h>

#include <stdint.h>

#include "sdram.h"

static inline void
sdram_setup_pio (void)
{
  AT91F_PIO_CfgPeriph (AT91C_BASE_PIOA, 0,
		       (1L << 0) | (1L << 1) | (1L << 3) | (1L << 9) | (1L <<
									18) |
		       (0x1ffL << 23));
  AT91F_PIO_CfgPeriph (AT91C_BASE_PIOB, 0,
		       (1L << 2) | (1L << 4) | (1L << 5) | (1L << 6) | (1L <<
									7) |
		       (1L << 8) | (1L << 10) | (1L << 11) | (1L << 13) |
		       (0xffffL << 16));
  AT91F_PIO_CfgPeriph (AT91C_BASE_PIOC, 0xFFFFL, 0);
}

//*----------------------------------------------------------------------------
//* \fn AT91F_InitSdram
//* \brief Init EBI and SDRAM controller for MT48LC16M16A2
//*----------------------------------------------------------------------------
void
sdram_init (void)
{
  volatile unsigned int i;
  volatile unsigned int *pSdram = (unsigned int *) SDRAM_BASE;
  AT91PS_SDRC psdrc = AT91C_BASE_SDRC;
  // Init the EBI for SDRAM
  AT91C_BASE_EBI->EBI_CSA = AT91C_EBI_CS1A_SDRAMC;	// Chip Select is assigned to SDRAM
  // controller
  //Configure PIO for EBI CS1
  sdram_setup_pio ();
  //*** Step 1 ***
  // Set Configuration Register
  psdrc->SDRC_CR = AT91C_SDRC_NC_8 |	// 8 bits Column Addressing: 256 (A0-A7)
    // AT91C_SDRC_NC_9
    AT91C_SDRC_NR_12 |		// 12 bits Row Addressing: 4Ki (A0-A11)
    // AT91C_SDRC_NR_13
    AT91C_SDRC_CAS_2 |		// SDRAMC only supports CAS 2
    AT91C_SDRC_NB_4_BANKS |	// 4 banks
				/* TWR */ (2L << 7) |
				// Minimum value is 2 clocks
    /* TRC */ (4L << 11) |
				/* TRP */ (2L << 15) |
				// Minimum value is 2 clocks
    /* TRCD */ (2L << 19) |
    /* TRAS */ (3L << 23) |
    /* TXSR */ (4L << 27);
  //*** Step 2 ***
  // Wait 200us
  for (i = 0; i < 9600; i++);
  //*** Step 3 ***
  // NOP Command
  psdrc->SDRC_MR = AT91C_SDRC_DBW_32_BITS | AT91C_SDRC_MODE_NOP_CMD;	// Set NOP
  pSdram[0] = 0x00000000;	// Perform NOP

  //*** Step 4 *** (micron step 6)
  //All Banks Precharge Command
  psdrc->SDRC_MR = AT91C_SDRC_DBW_32_BITS | 0x00000002;	// Set PRCHG AL
  pSdram[0] = 0x00000000;	// Perform PRCHG

  // micron step 7
  for (i = 0; i < 1000; i++);

  //*** Step 5 *** (micron step 8)
  //8 Refresh Command
  volatile int j = 0;
  for (j = 0; j < 8; j++)
    {
      psdrc->SDRC_MR = AT91C_SDRC_DBW_32_BITS | AT91C_SDRC_MODE_RFSH_CMD;	// Set j-st CBR
      pSdram[0] = 0;		// Perform CBR
      for (i = 0; i < 2000; i++);
    }

  //*** Step 6 *** (micron step 12)
  //Mode Register Command
  psdrc->SDRC_MR = AT91C_SDRC_DBW_32_BITS | AT91C_SDRC_MODE_LMR_CMD;	// Set LMR operation
  /* A note on the Mode Register programming:
   * The mode register is loaded with the data on the address lines of the SDRAM chip, e.g. A0-A11.
   * However, A0 on the SDRAM is connected to A2 on the AT91SAM7, so the mode register programming
   * access must really go to MODE_REGISTER_VALUE<<2. This is neatly accomplished by having 
   * pSdram of type int* and then accessing pSdram[MODE_REGISTER_VALUE], which will of course access
   * pSdram + MODE_REGISTER_VALUE * sizeof(*pSdram).
   * 
   * As far as I can tell, the example in the Atmel documentation (doc 6287) is wrong: 
   *      *AT91C_SDRAM_BASE = 0x00000000;
   * would program the mode register with 0, leading to undefined operation.
   * Also, there are some examples on the internet that do
   *  *(pSdram + MODE_REGISTER_VALUE<<2) = 0;
   * (where MODE_REGISTER_VALUE<<2 is obfuscated by using the literal value 0x80) which is equivalent
   * to pSdram[MODE_REGISTER_VALUE<<2] and therefore an access to pSdram + (MODE_REGISTER_VALUE<<2) * sizeof(*pSdram)
   * which would program the mode register with a wrong value, again leading to undefined operation.
   * 
   * -- Henryk, 2009-06-10
   */
  pSdram[SDRAM_MODE] = 0;	// Perform LMR burst=1, lat=2
  for (i = 0; i < 2000; i++);

  //*** Step 7 ***
  //Normal Mode Command
  psdrc->SDRC_MR = AT91C_SDRC_DBW_32_BITS | AT91C_SDRC_MODE_NORMAL_CMD;	// Set Normal mode
  // 32 bits
  pSdram[0] = 0x00000000;	// Perform Normal mode
  //*** Step 8 ***
  // Set Refresh Timer
  psdrc->SDRC_TR = SDRAM_TR_TIME;
}
