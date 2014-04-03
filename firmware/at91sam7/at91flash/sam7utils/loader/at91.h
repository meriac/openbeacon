/*
 * loader/at91.h
 *
 * Copyright (C) 2006 Erik Gilling, all rights reserved
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 */

#define AT91_FLASH_ADDR ((uint32_t *) 0x100000)
#define AT91_CHIP_ID    ((uint32_t *) 0xfffff240)
#define AT91_CHIPID_NVPSIZ( chipid ) (((chipid)>>8)&0xf)

typedef unsigned int    uint32_t;
typedef signed int      int32_t;

typedef unsigned short  uint16_t;
typedef signed short    int16_t;

typedef unsigned char   uint8_t;
typedef signed char     int8_t;

typedef volatile uint32_t arm_reg_t;

typedef struct {
  arm_reg_t  fmr;      /* X0 - Flash Mode Register */
  arm_reg_t  fcr;      /* X4 - Flash Command Register */
  arm_reg_t  fsr;      /* X8 - Flash Status Register */
  arm_reg_t  res;      /* XC - reserved */
} at91_mc_efc_t;

typedef struct {
  arm_reg_t  rcr;      /* 00 - Remap Control Register */
  arm_reg_t  asr;      /* 04 - Abort Status Rebister */
  arm_reg_t  aasr;     /* 08 - Abort Address Status Register */
  arm_reg_t  res[21];  /* 0C..5C - Reserved */
  at91_mc_efc_t efc[2];/* 60-7F - Embedded Flash Controllers */
} at91_mc_t;

extern volatile at91_mc_t mc;

#define AT91_MC_FMR_FRDY            (1<<0)
#define AT91_MC_FMR_LOCKE           (1<<2)
#define AT91_MC_FMR_PROGE           (1<<3)
#define AT91_MC_FMR_NEBP            (1<<7)
#define AT91_MC_FMR_WAIT_STATE_1_2  (0<<8)
#define AT91_MC_FMR_WAIT_STATE_2_3  (1<<8)
#define AT91_MC_FMR_WAIT_STATE_3_4  (2<<8)
#define AT91_MC_FMR_WAIT_STATE_4_4  (3<<8)
#define AT91_MC_FMR_FMCN( x )       (((x)&0xff) << 16)


#define AT91_MC_FCR_FCMD_NOP        0x0
#define AT91_MC_FCR_FCMD_WP         0x1
#define AT91_MC_FCR_FCMD_SLB        0x2
#define AT91_MC_FCR_FCMD_WPL        0x3
#define AT91_MC_FCR_FCMD_CLB        0x4
#define AT91_MC_FCR_FCMD_EA         0x8
#define AT91_MC_FCR_FCMD_SGPB       0xb
#define AT91_MC_FCR_FCMD_CGPB       0xd
#define AT91_MC_FCR_FCMD_SSB        0xf

#define AT91_MC_FCR_PAGEN( x )      (((x)&0x3ff)<<8)

#define AT91_MC_FCR_KEY             (0x5A << 24)

