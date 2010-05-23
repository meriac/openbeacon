MEMORY.
{
	flash	: ORIGIN = 0x00000000, LENGTH = 32K-1K /* save 1K for environment on top of flash */
	ram	: ORIGIN = 0x10000180, LENGTH = 8K - 0x180
}

__stack_end__ = 0x10000000 + 8K - 4;

INCLUDE "config/LPC13xx.ld"