#ifndef PIO_IRQ_H_
#define PIO_IRQ_H_

#include "FreeRTOS.h"

#define NR_PIO 3
#define NR_PIN 32
typedef portBASE_TYPE irq_handler_t(AT91PS_PIO pio, u_int32_t pin, portBASE_TYPE xTaskWoken);

extern void pio_irq_enable(AT91PS_PIO pio, u_int32_t pin);
extern void pio_irq_disable(AT91PS_PIO pio, u_int32_t pin);
extern long pio_irq_get_count(void);
extern int pio_irq_register(AT91PS_PIO pio, u_int32_t pin, irq_handler_t *func);
extern void pio_irq_unregister(AT91PS_PIO pio, u_int32_t pin);

extern void pio_irq_init_once(void);
extern void pio_irq_init(void);

#endif /*PIO_IRQ_H_*/
