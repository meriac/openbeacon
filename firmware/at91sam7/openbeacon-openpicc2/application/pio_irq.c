#include <FreeRTOS.h>
#include <AT91SAM7.h>
#include <board.h>
#include <task.h>
#include <string.h>
#include <beacontypes.h>
#include <errno.h>

#include "pio_irq.h"

static int initialized = 0;
static unsigned long count = 0;

struct pioirq_state {
	AT91PS_PIO pio;
	irq_handler_t *handlers[NR_PIN];
};
static struct pioirq_state state[NR_PIO];

void __ramfunc __pio_irq_demux(AT91PS_PIO pio, int index)
{
	int pin;
	count++;
	portBASE_TYPE xTaskWoken = pdFALSE;

	u_int32_t interrupted_pins = AT91F_PIO_GetInterruptStatus(pio);

	if(state[index].pio != pio) return;
	
	for (pin = 0; pin < NR_PIN; pin++) {
		if ( (interrupted_pins & (1 << pin)) && (state[index].handlers[pin] != NULL) )
			xTaskWoken = state[index].handlers[pin](pio, 1<<pin, xTaskWoken);
	}

	AT91F_AIC_AcknowledgeIt();
	//AT91F_AIC_ClearIt(AT91C_ID_PIOA);
	
	if(xTaskWoken) 
		portYIELD_FROM_ISR();
}


/* Interrupt handler wrapper template */
#define INTERRUPT_HANDLER_CODE(PIO, index) \
	portSAVE_CONTEXT(); \
	__pio_irq_demux(PIO, index); \
	portRESTORE_CONTEXT();
	
static void pio_irq_demux_a(void) __attribute__ ((naked));
static void pio_irq_demux_a(void) { INTERRUPT_HANDLER_CODE(AT91C_BASE_PIOA, 0) }

#if NR_PIO > 1
static void pio_irq_demux_b(void) __attribute__ ((naked));
static void pio_irq_demux_b(void) { INTERRUPT_HANDLER_CODE(AT91C_BASE_PIOB, 1) }
#endif

#if NR_PIO > 2
static void pio_irq_demux_c(void) __attribute__ ((naked));
static void pio_irq_demux_c(void) { INTERRUPT_HANDLER_CODE(AT91C_BASE_PIOC, 2) }
#endif

#if NR_PIO > 3
#error NR_PIO too big
#endif


void pio_irq_enable(AT91PS_PIO pio, u_int32_t pin)
{
	AT91F_PIO_InterruptEnable(pio, pin);
}

void pio_irq_disable(AT91PS_PIO pio, u_int32_t pin)
{
	AT91F_PIO_InterruptDisable(pio, pin);
}

/* Return the number of PIO IRQs received */ 
long pio_irq_get_count(void)
{
	return count;
}

int pio_irq_register(AT91PS_PIO pio, u_int32_t pin, irq_handler_t *handler)
{
	u_int8_t num = ffs(pin);

	if (num == 0)
		return -EINVAL;
	num--;
	
	int index;
	for(index = 0; index <= NR_PIO; index++) {
		if(index == NR_PIO)
			return -EINVAL;
		if(state[index].pio == pio)
			break;
	}

	if (state[index].handlers[num])
		return -EBUSY;

	pio_irq_disable(pio, pin);
	AT91F_PIO_CfgInput(pio, pin);
	state[index].handlers[num] = handler;
	//DEBUGPCRF("registering handler %p for PIO%c %u", handler, 'A'+index, num);

	return 0;
}

void pio_irq_unregister(AT91PS_PIO pio, u_int32_t pin)
{
	u_int8_t num = ffs(pin);

	if (num == 0)
		return;
	num--;

	int index;
	for(index = 0; index <= NR_PIO; index++) {
		if(index == NR_PIO)
			return;
		if(state[index].pio == pio)
			break;
	}

	pio_irq_disable(pio, pin);
	state[index].handlers[num] = NULL;
}

void pio_irq_init_once(void)
{
	if(!initialized)
		pio_irq_init();
}

void pio_irq_init(void)
{
	memset(&state, 0, sizeof(state));
	
	state[0].pio = AT91C_BASE_PIOA;
	AT91F_AIC_ConfigureIt(AT91C_ID_PIOA, OPENPICC_IRQ_PRIO_PIO, 
			AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, &pio_irq_demux_a);
	AT91F_AIC_EnableIt(AT91C_ID_PIOA);
#if NR_PIO > 1
	state[1].pio = AT91C_BASE_PIOB;
	AT91F_AIC_ConfigureIt(AT91C_ID_PIOB, OPENPICC_IRQ_PRIO_PIO, 
			AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, &pio_irq_demux_b);
	AT91F_AIC_EnableIt(AT91C_ID_PIOB);
#endif
#if NR_PIO > 2
	state[2].pio = AT91C_BASE_PIOC;
	AT91F_AIC_ConfigureIt(AT91C_ID_PIOC, OPENPICC_IRQ_PRIO_PIO, 
			AT91C_AIC_SRCTYPE_INT_HIGH_LEVEL, &pio_irq_demux_c);
	AT91F_AIC_EnableIt(AT91C_ID_PIOC);
#endif
	
	initialized = 1;
}
