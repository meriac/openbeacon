#include <FreeRTOS.h>
#include <task.h>
#include <string.h>
#include <beacontypes.h>

#include "authentication.h"
#include "openbouncer.h"
#include "board.h"

u_int8_t test;

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

static void
authentication_task (void *parameter)
{
  (void) parameter;
  memset (&authentication_state, 0, sizeof (authentication_state));
  authentication_state.state = AUTHENTICATION_IDLE;

}

void
init_authentication (void)
{
  xTaskCreate (authentication_task, (signed portCHAR *) "AUTHENTICATION",
	       TASK_NRF_STACK, NULL, TASK_NRF_PRIORITY, NULL);
}
