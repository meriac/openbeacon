/*****************************************************************************
 *   gpio.h:  Header file for NXP LPC13xx Family Microprocessors
 *
 *   Copyright(C) 2008, NXP Semiconductor
 *   All rights reserved.
 *
 *   History
 *   2008.09.01  ver 1.00    Preliminary version, first Release
 *   2009.12.09  ver 1.05    Mod to use mask registers for GPIO writes + inlining (.h)
 *
******************************************************************************/
#ifndef __GPIO_H
#define __GPIO_H

#define PORT0		0
#define PORT1		1
#define PORT2		2
#define PORT3		3

void GPIO_IRQHandler(void);
void GPIOInit(void);
void GPIOSetInterrupt(uint32_t portNum, uint32_t bitPosi, uint32_t sense,
		      uint32_t single, uint32_t event);
void GPIOIntEnable(uint32_t portNum, uint32_t bitPosi);
void GPIOIntDisable(uint32_t portNum, uint32_t bitPosi);
uint32_t GPIOIntStatus(uint32_t portNum, uint32_t bitPosi);
void GPIOIntClear(uint32_t portNum, uint32_t bitPosi);

static LPC_GPIO_TypeDef(*const LPC_GPIO[4]) = {
LPC_GPIO0, LPC_GPIO1, LPC_GPIO2, LPC_GPIO3};

/*****************************************************************************
** Function name:		GPIOSetValue
**
** Descriptions:		Set/clear a bitvalue in a specific bit position
**						in GPIO portX(X is the port number.)
**
** parameters:			port num, bit position, bit value
** Returned value:		None
**
*****************************************************************************/
static __INLINE void GPIOSetValue(uint32_t portNum, uint32_t bitPosi,
				  uint32_t bitVal)
{
    LPC_GPIO[portNum]->MASKED_ACCESS[(1 << bitPosi)] = (bitVal << bitPosi);
}

/*****************************************************************************
** Function name:		GPIOSetDir
**
** Descriptions:		Set the direction in GPIO port
**
** parameters:			port num, bit position, direction (1 out, 0 input)
** Returned value:		None
**
*****************************************************************************/
static __INLINE void GPIOSetDir(uint32_t portNum, uint32_t bitPosi,
				uint32_t dir)
{
    if (dir)
	LPC_GPIO[portNum]->DIR |= 1 << bitPosi;
    else
	LPC_GPIO[portNum]->DIR &= ~(1 << bitPosi);
}

#endif				/* end __GPIO_H */
/*****************************************************************************
**                            End Of File
******************************************************************************/
