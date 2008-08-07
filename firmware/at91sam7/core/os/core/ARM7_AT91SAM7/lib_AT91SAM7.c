//* ----------------------------------------------------------------------------
//*         ATMEL Microcontroller Software Support  -  ROUSSET  -
//* ----------------------------------------------------------------------------
//* DISCLAIMER:  THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
//* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
//* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
//* DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
//* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
//* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
//* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//* ----------------------------------------------------------------------------
//* File Name           : lib_AT91SAM7S64.h
//* Object              : AT91SAM7S64 inlined functions
//* Generated           : AT91 SW Application Group  08/30/2005 (15:52:59)
//*

#include <sys/types.h>
#include <AT91SAM7.h>
#include <lib_AT91SAM7.h>

//*----------------------------------------------------------------------------
//* \fn    AT91F_AIC_ConfigureIt
//* \brief Interrupt Handler Initialization
//*----------------------------------------------------------------------------
unsigned int
AT91F_AIC_ConfigureIt (unsigned int irq_id,	// \arg interrupt number to initialize
		       unsigned int priority,	// \arg priority to give to the interrupt
		       unsigned int src_type,	// \arg activation and sense of activation
		       THandler handler)	// \arg address of the interrupt handler
{
  unsigned int oldHandler;
  unsigned int mask;

  oldHandler = AT91C_BASE_AIC->AIC_SVR[irq_id];

  mask = 0x1 << irq_id;
  //* Disable the interrupt on the interrupt controller
  AT91C_BASE_AIC->AIC_IDCR = mask;
  //* Save the interrupt handler routine pointer and the interrupt priority
  AT91C_BASE_AIC->AIC_SVR[irq_id] = (unsigned int) handler;
  //* Store the Source Mode Register
  AT91C_BASE_AIC->AIC_SMR[irq_id] = src_type | priority;
  //* Clear the interrupt on the interrupt controller
  AT91C_BASE_AIC->AIC_ICCR = mask;

  return (unsigned int) handler;
}

//*----------------------------------------------------------------------------
//* \fn    AT91F_AIC_SetExceptionVector
//* \brief Configure vector handler
//*----------------------------------------------------------------------------
unsigned int
AT91F_AIC_SetExceptionVector (unsigned int *pVector,	// \arg pointer to the AIC registers
			      THandler Handler)	// \arg Interrupt Handler
{
  unsigned int oldVector = *pVector;

  if ((unsigned int) Handler == (unsigned int) AT91C_AIC_BRANCH_OPCODE)
    *pVector = (unsigned int) AT91C_AIC_BRANCH_OPCODE;
  else
    *pVector =
      (((((unsigned int) Handler) - ((unsigned int) pVector) -
	 0x8) >> 2) & 0x00FFFFFF) | 0xEA000000;

  return oldVector;
}

//*----------------------------------------------------------------------------
//* \fn    AT91F_AIC_Open
//* \brief Set exception vectors and AIC registers to default values
//*----------------------------------------------------------------------------
void
AT91F_AIC_Open (THandler IrqHandler,	// \arg Default IRQ vector exception
		THandler FiqHandler,	// \arg Default FIQ vector exception
		THandler DefaultHandler,	// \arg Default Handler set in ISR
		THandler SpuriousHandler,	// \arg Default Spurious Handler
		unsigned int protectMode)	// \arg Debug Control Register
{
  int i;

  // Disable all interrupts and set IVR to the default handler
  for (i = 0; i < 32; ++i)
    {
      AT91F_AIC_DisableIt (i);
      AT91F_AIC_ConfigureIt (i, AT91C_AIC_PRIOR_LOWEST,
			     AT91C_AIC_SRCTYPE_HIGH_LEVEL, DefaultHandler);
    }

  // Set the IRQ exception vector
  AT91F_AIC_SetExceptionVector ((unsigned int *) 0x18, IrqHandler);
  // Set the Fast Interrupt exception vector
  AT91F_AIC_SetExceptionVector ((unsigned int *) 0x1C, FiqHandler);

  AT91C_BASE_AIC->AIC_SPU = (unsigned int) SpuriousHandler;
  AT91C_BASE_AIC->AIC_DCR = protectMode;
}

//*----------------------------------------------------------------------------
//* \fn    AT91F_PDC_Open
//* \brief Open PDC: disable TX and RX reset transfer descriptors, re-enable RX and TX
//*----------------------------------------------------------------------------
void
AT91F_PDC_Open (AT91PS_PDC pPDC)	// \arg pointer to a PDC controller
{
  //* Disable the RX and TX PDC transfer requests
  AT91F_PDC_DisableRx (pPDC);
  AT91F_PDC_DisableTx (pPDC);

  //* Reset all Counter register Next buffer first
  AT91F_PDC_SetNextTx (pPDC, NULL, 0);
  AT91F_PDC_SetNextRx (pPDC, NULL, 0);
  AT91F_PDC_SetTx (pPDC, NULL, 0);
  AT91F_PDC_SetRx (pPDC, NULL, 0);

  //* Enable the RX and TX PDC transfer requests
  AT91F_PDC_EnableRx (pPDC);
  AT91F_PDC_EnableTx (pPDC);
}

//*----------------------------------------------------------------------------
//* \fn    AT91F_PDC_Close
//* \brief Close PDC: disable TX and RX reset transfer descriptors
//*----------------------------------------------------------------------------
void
AT91F_PDC_Close (AT91PS_PDC pPDC)	// \arg pointer to a PDC controller
{
  //* Disable the RX and TX PDC transfer requests
  AT91F_PDC_DisableRx (pPDC);
  AT91F_PDC_DisableTx (pPDC);

  //* Reset all Counter register Next buffer first
  AT91F_PDC_SetNextTx (pPDC, NULL, 0);
  AT91F_PDC_SetNextRx (pPDC, NULL, 0);
  AT91F_PDC_SetTx (pPDC, NULL, 0);
  AT91F_PDC_SetRx (pPDC, NULL, 0);

}

//*----------------------------------------------------------------------------
//* \fn    AT91F_PDC_SendFrame
//* \brief Close PDC: disable TX and RX reset transfer descriptors
//*----------------------------------------------------------------------------
unsigned int
AT91F_PDC_SendFrame (AT91PS_PDC pPDC,
		     const unsigned char *pBuffer,
		     unsigned int szBuffer,
		     const unsigned char *pNextBuffer,
		     unsigned int szNextBuffer)
{
  if (AT91F_PDC_IsTxEmpty (pPDC))
    {
      //* Buffer and next buffer can be initialized
      AT91F_PDC_SetTx (pPDC, pBuffer, szBuffer);
      AT91F_PDC_SetNextTx (pPDC, pNextBuffer, szNextBuffer);
      return 2;
    }
  else if (AT91F_PDC_IsNextTxEmpty (pPDC))
    {
      //* Only one buffer can be initialized
      AT91F_PDC_SetNextTx (pPDC, pBuffer, szBuffer);
      return 1;
    }
  else
    {
      //* All buffer are in use...
      return 0;
    }
}

//*----------------------------------------------------------------------------
//* \fn    AT91F_PDC_ReceiveFrame
//* \brief Close PDC: disable TX and RX reset transfer descriptors
//*----------------------------------------------------------------------------
unsigned int
AT91F_PDC_ReceiveFrame (AT91PS_PDC pPDC,
			unsigned char *pBuffer,
			unsigned int szBuffer,
			unsigned char *pNextBuffer, unsigned int szNextBuffer)
{
  if (AT91F_PDC_IsRxEmpty (pPDC))
    {
      //* Buffer and next buffer can be initialized
      AT91F_PDC_SetRx (pPDC, pBuffer, szBuffer);
      AT91F_PDC_SetNextRx (pPDC, pNextBuffer, szNextBuffer);
      return 2;
    }
  else if (AT91F_PDC_IsNextRxEmpty (pPDC))
    {
      //* Only one buffer can be initialized
      AT91F_PDC_SetNextRx (pPDC, pBuffer, szBuffer);
      return 1;
    }
  else
    {
      //* All buffer are in use...
      return 0;
    }
}

//*------------------------------------------------------------------------------
//* \fn    AT91F_PMC_GetMasterClock
//* \brief Return master clock in Hz which correponds to processor clock for ARM7
//*------------------------------------------------------------------------------
unsigned int
AT91F_PMC_GetMasterClock (AT91PS_PMC pPMC,	// \arg pointer to PMC controller
			  AT91PS_CKGR pCKGR,	// \arg pointer to CKGR controller
			  unsigned int slowClock)	// \arg slowClock in Hz
{
  unsigned int reg = pPMC->PMC_MCKR;
  unsigned int prescaler = (1 << ((reg & AT91C_PMC_PRES) >> 2));
  unsigned int pllDivider, pllMultiplier;

  switch (reg & AT91C_PMC_CSS)
    {
    case AT91C_PMC_CSS_SLOW_CLK:	// Slow clock selected
      return slowClock / prescaler;
    case AT91C_PMC_CSS_MAIN_CLK:	// Main clock is selected
      return AT91F_CKGR_GetMainClock (pCKGR, slowClock) / prescaler;
    case AT91C_PMC_CSS_PLL_CLK:	// PLLB clock is selected
      reg = pCKGR->CKGR_PLLR;
      pllDivider = (reg & AT91C_CKGR_DIV);
      pllMultiplier = ((reg & AT91C_CKGR_MUL) >> 16) + 1;
      return AT91F_CKGR_GetMainClock (pCKGR,
				      slowClock) / pllDivider *
	pllMultiplier / prescaler;
    }
  return 0;
}

//*--------------------------------------------------------------------------------------
//* \fn     AT91F_RTT_ReadValue()
//* \brief  Read the RTT value
//*--------------------------------------------------------------------------------------
unsigned int
AT91F_RTTReadValue (AT91PS_RTTC pRTTC)
{
  register volatile unsigned int val1, val2;
  do
    {
      val1 = pRTTC->RTTC_RTVR;
      val2 = pRTTC->RTTC_RTVR;
    }
  while (val1 != val2);
  return (val1);
}

//*----------------------------------------------------------------------------
//* \fn    AT91F_SPI_Close
//* \brief Close SPI: disable IT disable transfert, close PDC
//*----------------------------------------------------------------------------
void
AT91F_SPI_Close (AT91PS_SPI pSPI)	// \arg pointer to a SPI controller
{
  //* Reset all the Chip Select register
  pSPI->SPI_CSR[0] = 0;
  pSPI->SPI_CSR[1] = 0;
  pSPI->SPI_CSR[2] = 0;
  pSPI->SPI_CSR[3] = 0;

  //* Reset the SPI mode
  pSPI->SPI_MR = 0;

  //* Disable all interrupts
  pSPI->SPI_IDR = 0xFFFFFFFF;

  //* Abort the Peripheral Data Transfers
  AT91F_PDC_Close ((AT91PS_PDC) & (pSPI->SPI_RPR));

  //* Disable receiver and transmitter and stop any activity immediately
  pSPI->SPI_CR = AT91C_SPI_SPIDIS;
}

//*----------------------------------------------------------------------------
//* \fn    AT91F_ADC_CfgTimings
//* \brief Configure the different necessary timings of the ADC controller
//*----------------------------------------------------------------------------
void
AT91F_ADC_CfgTimings (AT91PS_ADC pADC,	// pointer to a ADC controller
		      unsigned int mck_clock,	// in MHz 
		      unsigned int adc_clock,	// in MHz 
		      unsigned int startup_time,	// in us 
		      unsigned int sample_and_hold_time)	// in ns  
{
  unsigned int prescal, startup, shtim;

  prescal = mck_clock / (2 * adc_clock) - 1;
  startup = adc_clock * startup_time / 8 - 1;
  shtim = adc_clock * sample_and_hold_time / 1000 - 1;

  //* Write to the MR register
  pADC->ADC_MR =
    ((prescal << 8) & AT91C_ADC_PRESCAL) | ((startup << 16) &
					    AT91C_ADC_STARTUP) | ((shtim <<
								   24) &
								  AT91C_ADC_SHTIM);
}

//*----------------------------------------------------------------------------
//* \fn    AT91F_SSC_SetBaudrate
//* \brief Set the baudrate according to the CPU clock
//*----------------------------------------------------------------------------
void
AT91F_SSC_SetBaudrate (AT91PS_SSC pSSC,	// \arg pointer to a SSC controller
		       unsigned int mainClock,	// \arg peripheral clock
		       unsigned int speed)	// \arg SSC baudrate
{
  unsigned int baud_value;
  //* Define the baud rate divisor register
  if (speed == 0)
    baud_value = 0;
  else
    {
      baud_value = (unsigned int) (mainClock * 10) / (2 * speed);
      if ((baud_value % 10) >= 5)
	baud_value = (baud_value / 10) + 1;
      else
	baud_value /= 10;
    }

  pSSC->SSC_CMR = baud_value;
}

//*----------------------------------------------------------------------------
//* \fn    AT91F_SSC_Configure
//* \brief Configure SSC
//*----------------------------------------------------------------------------
void
AT91F_SSC_Configure (AT91PS_SSC pSSC,	// \arg pointer to a SSC controller
		     unsigned int syst_clock,	// \arg System Clock Frequency
		     unsigned int baud_rate,	// \arg Expected Baud Rate Frequency
		     unsigned int clock_rx,	// \arg Receiver Clock Parameters
		     unsigned int mode_rx,	// \arg mode Register to be programmed
		     unsigned int clock_tx,	// \arg Transmitter Clock Parameters
		     unsigned int mode_tx)	// \arg mode Register to be programmed
{
  //* Disable interrupts
  pSSC->SSC_IDR = (unsigned int) -1;

  //* Reset receiver and transmitter
  pSSC->SSC_CR = AT91C_SSC_SWRST | AT91C_SSC_RXDIS | AT91C_SSC_TXDIS;

  //* Define the Clock Mode Register
  AT91F_SSC_SetBaudrate (pSSC, syst_clock, baud_rate);

  //* Write the Receive Clock Mode Register
  pSSC->SSC_RCMR = clock_rx;

  //* Write the Transmit Clock Mode Register
  pSSC->SSC_TCMR = clock_tx;

  //* Write the Receive Frame Mode Register
  pSSC->SSC_RFMR = mode_rx;

  //* Write the Transmit Frame Mode Register
  pSSC->SSC_TFMR = mode_tx;

  //* Clear Transmit and Receive Counters
  AT91F_PDC_Open ((AT91PS_PDC) & (pSSC->SSC_RPR));


}

//*----------------------------------------------------------------------------
//* \fn    AT91F_US_Configure
//* \brief Configure USART
//*----------------------------------------------------------------------------
void
AT91F_US_Configure (AT91PS_USART pUSART,	// \arg pointer to a USART controller
		    unsigned int mainClock,	// \arg peripheral clock
		    unsigned int mode,	// \arg mode Register to be programmed
		    unsigned int baudRate,	// \arg baudrate to be programmed
		    unsigned int timeguard)	// \arg timeguard to be programmed
{
  //* Disable interrupts
  pUSART->US_IDR = (unsigned int) -1;

  //* Reset receiver and transmitter
  pUSART->US_CR =
    AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RXDIS | AT91C_US_TXDIS;

  //* Define the baud rate divisor register
  AT91F_US_SetBaudrate (pUSART, mainClock, baudRate);

  //* Write the Timeguard Register
  AT91F_US_SetTimeguard (pUSART, timeguard);

  //* Clear Transmit and Receive Counters
  AT91F_PDC_Open ((AT91PS_PDC) & (pUSART->US_RPR));

  //* Define the USART mode
  pUSART->US_MR = mode;

}

//*----------------------------------------------------------------------------
//* \fn    AT91F_US_Close
//* \brief Close USART: disable IT disable receiver and transmitter, close PDC
//*----------------------------------------------------------------------------
void
AT91F_US_Close (AT91PS_USART pUSART)	// \arg pointer to a USART controller
{
  //* Reset the baud rate divisor register
  pUSART->US_BRGR = 0;

  //* Reset the USART mode
  pUSART->US_MR = 0;

  //* Reset the Timeguard Register
  pUSART->US_TTGR = 0;

  //* Disable all interrupts
  pUSART->US_IDR = 0xFFFFFFFF;

  //* Abort the Peripheral Data Transfers
  AT91F_PDC_Close ((AT91PS_PDC) & (pUSART->US_RPR));

  //* Disable receiver and transmitter and stop any activity immediately
  pUSART->US_CR =
    AT91C_US_TXDIS | AT91C_US_RXDIS | AT91C_US_RSTTX | AT91C_US_RSTRX;
}
