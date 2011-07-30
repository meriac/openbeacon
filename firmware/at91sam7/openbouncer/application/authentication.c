#include <FreeRTOS.h>
#include <task.h>
#include <string.h>
#include <beacontypes.h>

#include "authentication.h"
#include "openbouncer.h"
#include "board.h"
#include "led.h"
#include "nRF24L01/nRF_HW.h"
#include "nRF24L01/nRF_CMD.h"
#include "nRF24L01/nRF_API.h"

static const unsigned char broadcast_mac[NRF_MAX_MAC_SIZE] = { 1, 2, 3, 2, 1 };

struct
{
  enum
  { AUTHENTICATION_UNITIALIZED,
    AUTHENTICATION_IDLE,
    AUTHENTICATION_HELLO_RECEIVED,
    AUTHENTICATION_SETUP_SENT,
    AUTHENTICATION_CHALLENGE_SENT,
    AUTHENTICATION_RESPONSE_RECEIVED,
    AUTHENTICATION_ACCEPTED,
    AUTHENTICATION_FAILED,
  } state;
  u_int8_t hello_salt[4];
  u_int8_t tag_retries;
  u_int8_t setup[8];
  u_int8_t challenge[8];
  unsigned int authentication_time;
} authentication_state;

struct tag_key
{
  u_int8_t KEY[16];
  unsigned int refcount;
};

#define MAX_KEYS 200
struct tag_key keys[MAX_KEYS];

static void process_message(TBouncerEnvelope *message) {
	/* Tag -> Door: HELLO
	 * Door -> Tag: CHALLENGE_SETUP
	 * Door -> Tag: CHALLENGE
	 * Tag -> Door: Response
	 */
	switch(message->hdr.command) {
	case BOUNCERPKT_CMD_HELLO: break;
	case BOUNCERPKT_CMD_RESPONSE: break;
	default: break;
	}
}

static void
authentication_task (void *parameter)
{
	TBouncerEnvelope Bouncer;
	(void) parameter;
	memset (&authentication_state, 0, sizeof (authentication_state));
	authentication_state.state = AUTHENTICATION_IDLE;
	
	
	while(1) {
		if (nRFCMD_WaitRx (10)) {
			
			do {
				vLedSetRed (1);
				// read packet from nRF chip
				nRFCMD_RegReadBuf (DEFAULT_DEV, RD_RX_PLOAD, (unsigned char *) &Bouncer, sizeof (Bouncer));
				
				process_message(&Bouncer);
				
				vLedSetRed (0);
			} while ((nRFAPI_GetFifoStatus (DEFAULT_DEV) & FIFO_RX_EMPTY) == 0);
			
			nRFAPI_GetFifoStatus(DEFAULT_DEV);
		}
		nRFAPI_ClearIRQ (DEFAULT_DEV, MASK_IRQ_FLAGS);
	}
}

void
init_authentication (void)
{
	if (!nRFAPI_Init(DEFAULT_DEV, DEFAULT_CHANNEL, broadcast_mac, sizeof (broadcast_mac), ENABLED_NRF_FEATURES))
		return;

	nRFAPI_SetPipeSizeRX (DEFAULT_DEV, 0, 16);
	nRFAPI_SetTxPower (DEFAULT_DEV, 3);
	nRFAPI_SetRxMode (DEFAULT_DEV, 1);
	nRFCMD_CE (DEFAULT_DEV, 1);

	xTaskCreate (authentication_task, (signed portCHAR *) "AUTHENTICATION",
	       TASK_NRF_STACK, NULL, TASK_NRF_PRIORITY, NULL);
}
