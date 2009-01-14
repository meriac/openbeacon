#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <beacontypes.h>

#include <errno.h>

#include <task.h>
#include <queue.h>

#include "event.h"

static xQueueHandle event_queue;
static int dropped_events = 0;

int event_receive(event_t *next_event, portBASE_TYPE wait_time)
{
	if( xQueueReceive(event_queue, next_event, wait_time) == pdTRUE ) return 0;
	else return -ETIMEDOUT;
}

int event_send(enum event_class event_class, int param)
{
	event_t scratch;
	taskENTER_CRITICAL();
	/* Handle the event queue as a FIFO: If an event is to be posted but there is no more room,
	 * drop the oldest event in the queue */
	if(uxQueueMessagesWaiting(event_queue) >= EVENT_MAX_WAITING) {
		event_receive(&scratch, 0);
		dropped_events++;
	}
	
	scratch.class = event_class;
	scratch.param = param;
	
	xQueueSend(event_queue, &scratch, 0);
	
	taskEXIT_CRITICAL();
	return 0;
}


int event_init(void)
{
	event_queue = xQueueCreate(sizeof(event_t), EVENT_MAX_WAITING);
	if(event_queue == 0)
		return -ENOMEM;
	return 0;
}
