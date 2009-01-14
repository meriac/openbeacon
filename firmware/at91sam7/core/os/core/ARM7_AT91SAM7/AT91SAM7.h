#ifndef __AT91SAM7_H__
#define __AT91SAM7_H__

// *****************************************************************************
//               MEMORY MAPPING DEFINITIONS FOR AT91SAM7
// *****************************************************************************
// ISRAM
#define AT91C_ISRAM      ((char *)      0x00200000)     // Internal SRAM base address
// IFLASH
#define AT91C_IFLASH     ((char *)      0x00100000)     // Internal FLASH base address

#if defined(__AT91SAM7S32__)
#include <AT91SAM7S.h>
#define AT91C_ISRAM_SIZE         ((unsigned int) 0x00002000)    // Internal SRAM size in byte (8 Kbytes)
#define AT91C_IFLASH_SIZE        ((unsigned int) 0x00008000)    // Internal FLASH size in byte (32 Kbytes)
#define AT91C_IFLASH_PAGE_SIZE   ((unsigned int) 128)   // Internal FLASH Page Size: 128 bytes
#define AT91C_IFLASH_PAGE_SHIFT          7
#define AT91C_IFLASH_NB_OF_LOCK_BITS     ((unsigned int) 8)     // Internal FLASH Number of Lock Bits: 8

#define USB_PULLUP_EXTERNAL

#elif defined(__AT91SAM7S64__)
#include <AT91SAM7S.h>
#define AT91C_ISRAM_SIZE         ((unsigned int) 0x00004000)    // Internal SRAM size in byte (16 Kbytes)
#define AT91C_IFLASH_SIZE        ((unsigned int) 0x00010000)    // Internal FLASH size in byte (64 Kbytes)
#define AT91C_IFLASH_PAGE_SIZE   ((unsigned int) 128)   // Internal FLASH Page Size: 128 bytes
#define AT91C_IFLASH_PAGE_SHIFT          7
#define AT91C_IFLASH_NB_OF_LOCK_BITS     ((unsigned int) 16)    // Internal FLASH Number of Lock Bits: 16

#define USB_PULLUP_EXTERNAL

#elif defined(__AT91SAM7S128__)
#include <AT91SAM7S.h>
#define AT91C_ISRAM_SIZE         ((unsigned int) 0x00008000)    // Internal SRAM size in byte (32 Kbytes)
#define AT91C_IFLASH_SIZE        ((unsigned int) 0x00020000)    // Internal FLASH size in byte (128 Kbytes)
#define AT91C_IFLASH_PAGE_SIZE   ((unsigned int) 256)   // Internal FLASH Page Size: 256 bytes
#define AT91C_IFLASH_PAGE_SHIFT          8
#define AT91C_IFLASH_NB_OF_LOCK_BITS     ((unsigned int) 8)     // Internal FLASH Number of Lock Bits: 8

#define USB_PULLUP_EXTERNAL

#elif defined(__AT91SAM7S256__) || defined(__AT91SAM7X256__)
#define AT91C_ISRAM_SIZE         ((unsigned int) 0x00010000)    // Internal SRAM size in byte (64 Kbytes)
#define AT91C_IFLASH_SIZE        ((unsigned int) 0x00040000)    // Internal FLASH size in byte (256 Kbytes)
#define AT91C_IFLASH_PAGE_SIZE   ((unsigned int) 256)   // Internal FLASH Page Size: 256 bytes
#define AT91C_IFLASH_PAGE_SHIFT          8
#define AT91C_IFLASH_NB_OF_LOCK_BITS     ((unsigned int) 16)    // Internal FLASH Number of Lock Bits: 16

#define USB_PULLUP_EXTERNAL

#elif defined(__AT91SAM7SE512__)
/* FIXME: Maybe generalize for other AT91SAM7SE types? */
#include <AT91SAM7SE512.h>
#define AT91C_IFLASH_PAGE_SHIFT          8

/* FIXME: For great Flash! */
#define MC_FSR MC0_FSR
#define MC_FCR MC0_FCR
#define MC_FMR MC0_FMR
#define AT91C_MC_FSR (&(AT91C_BASE_MC->MC0_FSR))

#define USB_PULLUP_INTERNAL

#else
#error Have to define which __AT91SAM7xxx__ type
#endif

#if   defined(__AT91SAM7X__)
#include <AT91SAM7X.h>

#elif defined(__AT91SAM7S__)
#include <AT91SAM7S.h>

#elif !defined(__AT91SAM7SE512__)
/* FIXME: Maybe generalize for other AT91SAM7SE types? */
#error Have to define __AT91SAM7?__ architecture type
#endif


#define AT91C_IFLASH_NB_OF_PAGES         (AT91C_IFLASH_SIZE>>AT91C_IFLASH_PAGE_SHIFT)
#define AT91C_IFLASH_LOCK_REGION_SIZE    (AT91C_IFLASH_SIZE/AT91C_IFLASH_NB_OF_LOCK_BITS)

#endif
