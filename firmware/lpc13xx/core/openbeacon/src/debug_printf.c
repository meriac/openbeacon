#include <openbeacon.h>
#if !defined UART_DISABLE || defined ENABLE_USB_FULLFEATURED
#include <ctype.h>
#include <uart.h>
#include <printf.h>
#include <stdarg.h>
#include <string.h>

static void
putc_debug (void *p, char c)
{
	(void) p;

	if (c == '\n')
		default_putchar ('\r');

	default_putchar (c);
}

void
debug_printf (const char *fmt, ...)
{
	va_list va;

	va_start (va, fmt);
	tfp_format (NULL, putc_debug, fmt, va);
	va_end (va);
}

char
hex_char (unsigned char hex)
{
	hex &= 0xF;
	return (hex < 0xA) ? (hex + '0') : ((hex - 0xA) + 'A');
}

void
hex_dump (const unsigned char *buf, unsigned int addr, unsigned int len)
{
	unsigned int start, i, j;
	char c;

	start = addr & ~0xf;

	for (j = 0; j < len; j += 16)
	{
		debug_printf ("%08x:", start + j);

		for (i = 0; i < 16; i++)
		{
			if (start + i + j >= addr && start + i + j < addr + len)
				debug_printf (" %02x", buf[start + i + j]);
			else
				debug_printf ("   ");
		}
		debug_printf ("  |");
		for (i = 0; i < 16; i++)
		{
			if (start + i + j >= addr && start + i + j < addr + len)
			{
				c = buf[start + i + j];
				if (c >= ' ' && c < 127)
					debug_printf ("%c", c);
				else
					debug_printf (".");
			}
			else
				debug_printf (" ");
		}
		debug_printf ("|\n\r");
	}
}
#endif /* UART_DISABLE */
