#include <AT91SAM7.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <USB-CDC.h>
#include <board.h>
#include <beacontypes.h>
#include <dbgu.h>
#include <env.h>

#include "cmd.h"
#include "nRF24L01/nRF_API.h"
#include "nRF24L01/nRF_HW.h"
#include "led.h"
#include "nRF24L01/nRF_CMD.h"

extern unsigned char audio_start,audio_end;

static const int notes[]={440,494,523,587,659,698,783,880,988};
#define NOTE_COUNT (sizeof(notes)/sizeof(notes[0]))

/**********************************************************************/
#define PWM_CHANNEL0 (1L<<0)
#define PWM_CHANNEL1 (1L<<1)
#define PWM_CHANNEL2 (1L<<2)
#define PWM_CHANNEL3 (1L<<3)
/**********************************************************************/
#define PWMC_DEFAULTCMR (3|(1<<10))
/**********************************************************************/

xTaskHandle xCmdRecvUsbTask;
/**********************************************************************/

void
DumpUIntToUSB (unsigned int data)
{
  int i = 0;
  unsigned char buffer[10], *p = &buffer[sizeof (buffer)];

  do
    {
      *--p = '0' + (unsigned char) (data % 10);
      data /= 10;
      i++;
    }
  while (data);

  while (i--)
    vUSBSendByte (*p++);
}

/**********************************************************************/

void
DumpStringToUSB (char *text)
{
  unsigned char data;

  if (text)
    while ((data = *text++) != 0)
      vUSBSendByte (data);
}

/**********************************************************************/

static inline unsigned char
HexChar (unsigned char nibble)
{
  return nibble + ((nibble < 0x0A) ? '0' : ('A' - 0xA));
}

/**********************************************************************/

void
DumpBufferToUSB (char *buffer, int len)
{
  int i;

  for (i = 0; i < len; i++)
    {
      vUSBSendByte (HexChar (*buffer >> 4));
      vUSBSendByte (HexChar (*buffer++ & 0xf));
    }
}

/**********************************************************************/

int
atoiEx (const char *nptr)
{
  int sign = 1, i = 0, curval = 0;

  while (nptr[i] == ' ' && nptr[i] != 0)
    i++;

  if (nptr[i] != 0)
    {
      if (nptr[i] == '-')
	{
	  sign *= -1;
	  i++;
	}
      else if (nptr[i] == '+')
	i++;

      while (nptr[i] != 0 && nptr[i] >= '0' && nptr[i] <= '9')
	curval = curval * 10 + (nptr[i++] - '0');
    }

  return sign * curval;
}

static void
vBeep (int frequency)
{
	int duration;
	
	if(frequency>=100)
	{
		duration = (MCK / 8) / frequency;

		AT91C_BASE_PWMC_CH0->PWMC_CDTYR = duration / 2;
		AT91C_BASE_PWMC_CH0->PWMC_CPRDR = duration;
	}
	else
		AT91C_BASE_PWMC_CH0->PWMC_CDTYR = AT91C_BASE_PWMC_CH0->PWMC_CPRDR = 0;
}

/**********************************************************************/

static void
vSweep(int maxfrequency)
{
  int i;

  for (i=100;i<maxfrequency;i++)
    {
		vBeep(i);
		vTaskDelay(2 / portTICK_RATE_MS);
    }
}

/**********************************************************************/

static void
vPlaySample(unsigned char* data, u_int32_t size, u_int32_t frequency)
{
  int i;
  (void) data;
  (void) size;
  (void) frequency;

  AT91C_BASE_PWMC_CH0->PWMC_CMR = 1;
  AT91C_BASE_PWMC_CH0->PWMC_CPRDR = 0x100;

  for(i=0;i<255;i++)
  {
    vTaskDelay(10 * portTICK_RATE_MS);
    AT91C_BASE_PWMC_CH0->PWMC_CDTYR=i;
  }

  for(i=255;i>=0;i--)
  {
    vTaskDelay(10 * portTICK_RATE_MS);
    AT91C_BASE_PWMC_CH0->PWMC_CDTYR=i;
  }

/*  DumpStringToUSB("Playing Samples=");
  DumpUIntToUSB(size);
  DumpStringToUSB(" Frequency=");
  DumpUIntToUSB(frequency);
  DumpStringToUSB("\n\r");*/

  AT91C_BASE_PWMC_CH0->PWMC_CMR = PWMC_DEFAULTCMR;
}

/**********************************************************************/
// A task to read commands from USB
static void
vCmdRecvUsbCode (void *pvParameters)
{
  int freq;
  char data;
  (void) pvParameters;

  for (;;)
    {
		if (vUSBRecvByte (&data, 1, 100))
		{
			if((data >= '1' && data <= '9'))			
			  {
				freq = notes[data-'1'];

				DumpUIntToUSB(freq);
				DumpStringToUSB("\n\r");
		
				vBeep(freq);
			  }
			 else
			 	switch(data)
			 	{
					case '0':
						vBeep(0);
						break;
			 		case 's':
			 			vSweep(20000);
			 			break;
			 		case 'p':
			 			vPlaySample(&audio_start,(int)(&audio_end-&audio_start),AUDIO_FREQUENCY);
			 			break;
			 	}
		}		  
	}
}

portBASE_TYPE
vCmdInit (void)
{
  AT91F_PIO_CfgPeriph (PWM_PIO, 0,PWM_AUDIO);
  AT91C_BASE_PWMC_CH0->PWMC_CMR = PWMC_DEFAULTCMR;
  AT91C_BASE_PWMC->PWMC_MR  = 1;
  AT91C_BASE_PWMC->PWMC_ENA = PWM_CHANNEL0;

  if (xTaskCreate
      (vCmdRecvUsbCode, (signed portCHAR *) "CMDUSB", TASK_CMD_STACK, NULL,
       TASK_CMD_PRIORITY, &xCmdRecvUsbTask) != pdPASS)
    {
      return 0;
    }

  return 1;
}
