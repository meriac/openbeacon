#ifndef SDRAM_H_
#define SDRAM_H_

#define SDRAM_BASE ((volatile unsigned char *) 0x20000000)
#define SDRAM_SIZE 0x1000000L
#define SDRAM_NO_BANKS 4

#define SDRAM_TR_TIME 0x250
#define SDRAM_MODE (0x0 | (2L<<4))

extern void sdram_init (void);

#endif /*SDRAM_H_ */
