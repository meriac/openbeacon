/*****************************************************************************/
/* startup_LPC13xx.s: Startup file for LPC13xx device series                 */
/*****************************************************************************/
/* Version: CodeSourcery Sourcery G++ Lite (with CS3)                        */
/*****************************************************************************/


/* 
//*** <<< Use Configuration Wizard in Context Menu >>> *** 
*/


/*
// <h> Stack Configuration
//   <o> Stack Size (in Bytes) <0x0-0xFFFFFFFF:8>
// </h>
*/

    .equ    Stack_Size, 0x00000100
    .section ".stack", "w"
    .align  3
    .globl  __cs3_stack_mem
    .globl  __cs3_stack_size
__cs3_stack_mem:
    .if     Stack_Size
    .space  Stack_Size
    .endif
    .size   __cs3_stack_mem,  . - __cs3_stack_mem
    .set    __cs3_stack_size, . - __cs3_stack_mem


/*
// <h> Heap Configuration
//   <o>  Heap Size (in Bytes) <0x0-0xFFFFFFFF:8>
// </h>
*/

    .equ    Heap_Size,  0x00001000
    
    .section ".heap", "w"
    .align  3
    .globl  __cs3_heap_start
    .globl  __cs3_heap_end
__cs3_heap_start:
    .if     Heap_Size
    .space  Heap_Size
    .endif
__cs3_heap_end:


/* Vector Table */

    .section ".cs3.interrupt_vector"
    .globl  __cs3_interrupt_vector_cortex_m
    .type   __cs3_interrupt_vector_cortex_m, %object

__cs3_interrupt_vector_cortex_m:
    .long   __cs3_stack                 /* Top of Stack                  */
    .long   __cs3_reset                 /* Reset Handler                 */
    .long   NMI_Handler                 /* NMI Handler                   */
    .long   HardFault_Handler           /* Hard Fault Handler            */
    .long   MemManage_Handler           /* MPU Fault Handler             */
    .long   BusFault_Handler            /* Bus Fault Handler             */
    .long   UsageFault_Handler          /* Usage Fault Handler           */
    .long   0                           /* Reserved                      */
    .long   0                           /* Reserved                      */
    .long   0                           /* Reserved                      */
    .long   0                           /* Reserved                      */
    .long   SVC_Handler                 /* SVCall Handler                */
    .long   DebugMon_Handler            /* Debug Monitor Handler         */
    .long   0                           /* Reserved                      */
    .long   PendSV_Handler              /* PendSV Handler                */
    .long   SysTick_Handler             /* SysTick Handler               */

    /* External Interrupts */
    .long   WAKEUP_IRQHandler           /* 16+ 0: Wakeup PIO0.0          */
    .long   WAKEUP_IRQHandler           /* 16+ 1: Wakeup PIO0.1          */
    .long   WAKEUP_IRQHandler           /* 16+ 2: Wakeup PIO0.2          */
    .long   WAKEUP_IRQHandler           /* 16+ 3: Wakeup PIO0.3          */
    .long   WAKEUP_IRQHandler           /* 16+ 4: Wakeup PIO0.4          */
    .long   WAKEUP_IRQHandler           /* 16+ 5: Wakeup PIO0.5          */
    .long   WAKEUP_IRQHandler           /* 16+ 6: Wakeup PIO0.6          */
    .long   WAKEUP_IRQHandler           /* 16+ 7: Wakeup PIO0.7          */
    .long   WAKEUP_IRQHandler           /* 16+ 8: Wakeup PIO0.8          */
    .long   WAKEUP_IRQHandler           /* 16+ 9: Wakeup PIO0.9          */
    .long   WAKEUP_IRQHandler           /* 16+10: Wakeup PIO0.10         */
    .long   WAKEUP_IRQHandler           /* 16+11: Wakeup PIO0.11         */
    .long   WAKEUP_IRQHandler           /* 16+12: Wakeup PIO1.0          */
    .long   WAKEUP_IRQHandler           /* 16+13: Wakeup PIO1.1          */
    .long   WAKEUP_IRQHandler           /* 16+14: Wakeup PIO1.2          */
    .long   WAKEUP_IRQHandler           /* 16+15: Wakeup PIO1.3          */
    .long   WAKEUP_IRQHandler           /* 16+16: Wakeup PIO1.4          */
    .long   WAKEUP_IRQHandler           /* 16+17: Wakeup PIO1.5          */
    .long   WAKEUP_IRQHandler           /* 16+18: Wakeup PIO1.6          */
    .long   WAKEUP_IRQHandler           /* 16+19: Wakeup PIO1.7          */
    .long   WAKEUP_IRQHandler           /* 16+20: Wakeup PIO1.8          */
    .long   WAKEUP_IRQHandler           /* 16+21: Wakeup PIO1.9          */
    .long   WAKEUP_IRQHandler           /* 16+22: Wakeup PIO1.10         */
    .long   WAKEUP_IRQHandler           /* 16+23: Wakeup PIO1.11         */
    .long   WAKEUP_IRQHandler           /* 16+24: Wakeup PIO2.0          */
    .long   WAKEUP_IRQHandler           /* 16+25: Wakeup PIO2.1          */
    .long   WAKEUP_IRQHandler           /* 16+26: Wakeup PIO2.2          */
    .long   WAKEUP_IRQHandler           /* 16+27: Wakeup PIO2.3          */
    .long   WAKEUP_IRQHandler           /* 16+28: Wakeup PIO2.4          */
    .long   WAKEUP_IRQHandler           /* 16+29: Wakeup PIO2.5          */
    .long   WAKEUP_IRQHandler           /* 16+30: Wakeup PIO2.6          */
    .long   WAKEUP_IRQHandler           /* 16+31: Wakeup PIO2.7          */
    .long   WAKEUP_IRQHandler           /* 16+32: Wakeup PIO2.8          */
    .long   WAKEUP_IRQHandler           /* 16+33: Wakeup PIO2.9          */
    .long   WAKEUP_IRQHandler           /* 16+34: Wakeup PIO2.10         */
    .long   WAKEUP_IRQHandler           /* 16+35: Wakeup PIO2.11         */
    .long   WAKEUP_IRQHandler           /* 16+36: Wakeup PIO3.0          */
    .long   WAKEUP_IRQHandler           /* 16+37: Wakeup PIO3.1          */
    .long   WAKEUP_IRQHandler           /* 16+38: Wakeup PIO3.2          */
    .long   WAKEUP_IRQHandler           /* 16+39: Wakeup PIO3.3          */
    .long   I2C_IRQHandler              /* 16+40: I2C                    */
    .long   TIMER16_0_IRQHandler        /* 16+41: 16-bit Counter-Timer 0 */
    .long   TIMER16_1_IRQHandler        /* 16+42: 16-bit Counter-Timer 1 */
    .long   TIMER32_0_IRQHandler        /* 16+43: 32-bit Counter-Timer 0 */
    .long   TIMER32_1_IRQHandler        /* 16+44: 32-bit Counter-Timer 1 */
    .long   SSP_IRQHandler              /* 16+45: SSP                    */
    .long   UART_IRQHandler             /* 16+46: UART                   */
    .long   USB_IRQHandler              /* 16+47: USB IRQ                */
    .long   USB_FIQHandler              /* 16+48: USB FIQ                */
    .long   ADC_IRQHandler              /* 16+49: A/D Converter          */
    .long   WDT_IRQHandler              /* 16+50: Watchdog Timer         */
    .long   BOD_IRQHandler              /* 16+51: Brown Out Detect       */
    .long   FMC_IRQHandler              /* 16+52: IP2111 Flash Memory    */
    .long   PIOINT3_IRQHandler          /* 16+53: PIO INT3               */
    .long   PIOINT2_IRQHandler          /* 16+54: PIO INT2               */
    .long   PIOINT1_IRQHandler          /* 16+55: PIO INT1               */
    .long   PIOINT0_IRQHandler          /* 16+56: PIO INT0               */

    .size   __cs3_interrupt_vector_cortex_m, . - __cs3_interrupt_vector_cortex_m


    .thumb


/* Reset Handler */

    .section .cs3.reset,"x",%progbits
    .thumb_func
    .globl  __cs3_reset_cortex_m
    .type   __cs3_reset_cortex_m, %function
__cs3_reset_cortex_m:
    .fnstart
    LDR     R0, =SystemInit
    BLX     R0
    LDR     R0,=_start
    BX      R0
    .pool
    .cantunwind
    .fnend
    .size   __cs3_reset_cortex_m,.-__cs3_reset_cortex_m

    .section ".text"

/* Exception Handlers */

    .weak   NMI_Handler
    .type   NMI_Handler, %function
NMI_Handler:
    B       .
    .size   NMI_Handler, . - NMI_Handler

    .weak   HardFault_Handler
    .type   HardFault_Handler, %function
HardFault_Handler:
    B       .
    .size   HardFault_Handler, . - HardFault_Handler

    .weak   MemManage_Handler
    .type   MemManage_Handler, %function
MemManage_Handler:
    B       .
    .size   MemManage_Handler, . - MemManage_Handler

    .weak   BusFault_Handler
    .type   BusFault_Handler, %function
BusFault_Handler:
    B       .
    .size   BusFault_Handler, . - BusFault_Handler

    .weak   UsageFault_Handler
    .type   UsageFault_Handler, %function
UsageFault_Handler:
    B       .
    .size   UsageFault_Handler, . - UsageFault_Handler

    .weak   SVC_Handler
    .type   SVC_Handler, %function
SVC_Handler:
    B       .
    .size   SVC_Handler, . - SVC_Handler

    .weak   DebugMon_Handler
    .type   DebugMon_Handler, %function
DebugMon_Handler:
    B       .
    .size   DebugMon_Handler, . - DebugMon_Handler

    .weak   PendSV_Handler
    .type   PendSV_Handler, %function
PendSV_Handler:
    B       .
    .size   PendSV_Handler, . - PendSV_Handler

    .weak   SysTick_Handler
    .type   SysTick_Handler, %function
SysTick_Handler:
    B       .
    .size   SysTick_Handler, . - SysTick_Handler


/* IRQ Handlers */

    .globl  Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    B       .
    .size   Default_Handler, . - Default_Handler

    .macro  IRQ handler
    .weak   \handler
    .set    \handler, Default_Handler
    .endm

    IRQ     WAKEUP_IRQHandler
    IRQ     I2C_IRQHandler
    IRQ     TIMER16_0_IRQHandler
    IRQ     TIMER16_1_IRQHandler
    IRQ     TIMER32_0_IRQHandler
    IRQ     TIMER32_1_IRQHandler
    IRQ     SSP_IRQHandler
    IRQ     UART_IRQHandler
    IRQ     USB_IRQHandler
    IRQ     USB_FIQHandler
    IRQ     ADC_IRQHandler
    IRQ     WDT_IRQHandler
    IRQ     BOD_IRQHandler
    IRQ     FMC_IRQHandler
    IRQ     PIOINT3_IRQHandler
    IRQ     PIOINT2_IRQHandler
    IRQ     PIOINT1_IRQHandler
    IRQ     PIOINT0_IRQHandler

    .end
