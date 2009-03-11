/***************************************************************
 *
 * OpenBeacon.org - flash routines for persistent environment
 *
 * Copyright 2007 Milosch Meriac <meriac@openbeacon.de>
 *
 ***************************************************************

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/

#include <FreeRTOS.h>
#include <string.h>
#include <board.h>
#include <task.h>
#include <beacontypes.h>
#include "env.h"

#define PAGES_PER_LOCKREGION	(AT91C_IFLASH_LOCK_REGION_SIZE>>AT91C_IFLASH_PAGE_SHIFT)
#define IS_FIRST_PAGE_OF_LOCKREGION(x)	((x % PAGES_PER_LOCKREGION) == 0)
#define LOCKREGION_FROM_PAGE(x)	(x / PAGES_PER_LOCKREGION)
#define ENV_FLASH ((unsigned int *)AT91C_IFLASH + AT91C_IFLASH_SIZE - ENVIRONMENT_SIZE)

TEnvironmentBlock env;

static inline unsigned short RAMFUNC
page_from_ramaddr (const void *addr)
{
  unsigned long ramaddr = (unsigned long) addr;
  ramaddr -= (unsigned long) AT91C_IFLASH;
  return ((ramaddr >> AT91C_IFLASH_PAGE_SHIFT));
}

static inline int RAMFUNC
is_page_locked (unsigned short page)
{
  unsigned short lockregion = LOCKREGION_FROM_PAGE (page);

  return (AT91C_BASE_MC->MC_FSR & (lockregion << 16));
}

static inline void RAMFUNC
flash_cmd_wait (void)
{
  while ((*AT91C_MC_FSR & AT91C_MC_FRDY) != AT91C_MC_FRDY);
}

static inline void RAMFUNC
unlock_page (unsigned short page)
{
  page &= 0x3ff;
  AT91F_MC_EFC_PerformCmd (AT91C_BASE_MC, AT91C_MC_FCMD_UNLOCK |
			   AT91C_MC_CORRECT_KEY | (page << 8));
  flash_cmd_wait ();
}

void RAMFUNC
env_flash_to (const void *addr)
{
  unsigned int i, *src, *dst;

  unsigned short page;

  src = env.data;
  dst = (unsigned int *) addr;
  for (i = 0; i < (sizeof (env.data) / sizeof (env.data[0])); i++)
    *dst++ = *src++;

  page = page_from_ramaddr (addr) & 0x3ff;

  if (is_page_locked (page))
    unlock_page (page);

  AT91F_MC_EFC_PerformCmd (AT91C_BASE_MC, AT91C_MC_FCMD_START_PROG |
			   AT91C_MC_CORRECT_KEY | (page << 8));

  flash_cmd_wait ();
}

unsigned short
env_crc16 (const unsigned char *buffer, int size)
{
  unsigned short crc = 0xFFFF;

  if (buffer && size)
    while (size--)
      {
	crc = (crc >> 8) | (crc << 8);
	crc ^= *buffer++;
	crc ^= ((unsigned char) crc) >> 4;
	crc ^= crc << 12;
	crc ^= (crc & 0xFF) << 5;
      }

  return crc;
}

#if defined(__AT91SAM7X__) || defined(__AT91SAM7SE__)
void RAMFUNC
env_reboot_to_update (void)
{
  /* During flashing only RAMFUNC code may be executed. 
   * For now, this means that no other code whatsoever may
   * be run until this function returns. */
  vTaskSuspendAll ();
  portENTER_CRITICAL ();

  flash_cmd_wait ();

  AT91F_MC_EFC_PerformCmd (AT91C_BASE_MC, AT91C_MC_CORRECT_KEY |
			   AT91C_MC_FCMD_CLR_GP_NVM |
			   (AT91C_MC_PAGEN & (2 << 8)));

  // endless loop to trigger watchdog reset
  while(1){};
}
#endif/*__AT91SAM7X__ || __AT91SAM7SE__*/

void
env_store (void)
{
  /* During flashing only RAMFUNC code may be executed. 
   * For now, this means that no other code whatsoever may
   * be run until this function returns. */
  vTaskSuspendAll ();
  portENTER_CRITICAL ();

  env.e.magic = TENVIRONMENT_MAGIC;
  env.e.crc16 = 0;
  env.e.size = sizeof (env);
  env.e.crc16 = env_crc16 ((unsigned char *) &env, sizeof (env));

  env_flash_to (ENV_FLASH);

  portEXIT_CRITICAL ();
  xTaskResumeAll ();
}

int
env_load (void)
{
  unsigned int crc;

  memcpy (&env, ENV_FLASH, sizeof (env));

  if (env.e.magic != TENVIRONMENT_MAGIC || env.e.size != sizeof (env))
    return 0;

  crc = env.e.crc16;
  env.e.crc16 = 0;

  return env_crc16 ((unsigned char *) &env, sizeof (env)) == crc;
}

void
env_init (void)
{
  unsigned int fmcn = AT91F_MC_EFC_ComputeFMCN (MCK);

  AT91F_MC_EFC_CfgModeReg (AT91C_BASE_MC, fmcn << 16 | AT91C_MC_FWS_3FWS);
}
