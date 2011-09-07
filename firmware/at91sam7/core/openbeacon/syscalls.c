/* (Source: nfirmware/src/os/syscalls.c)                               */
/***********************************************************************/
/*                                                                     */
/*  SYSCALLS.C:  System Calls                                          */
/*  most of this is from newlib-lpc and a Keil-demo                    */
/*                                                                     */
/*  These are "reentrant functions" as needed by                       */
/*  the WinARM-newlib-config, see newlib-manual.                       */
/*  Collected and modified by Martin Thomas                            */
/*                                                                     */
/***********************************************************************/


#include <stdlib.h>
#include <reent.h>
#include <sys/stat.h>

#include <board.h>
#include <FreeRTOS.h>
#include <USB-CDC.h>

static void my_putc(char c) 
{
#ifdef  DISABLE_USB
	(void)c;
#else
	vUSBSendByte(c);
#endif/*DISABLE_USB*/
}

static char my_getc( void )
{
#ifdef  DISABLE_USB
	return 0;
#else
	char c;
	return vUSBRecvByte(&c, sizeof(c), portMAX_DELAY);
#endif/*DISABLE_USB*/
}

_ssize_t _read_r(
    struct _reent *r, 
    int file, 
    void *ptr, 
    size_t len)
{
	char c;
	unsigned int  i;
	unsigned char *p;
	(void)r; (void)file;
	
	p = (unsigned char*)ptr;
	
	for (i = 0; i < len; i++) {
		// c = uart0Getch();
		// c = uart0GetchW();
		c = (char) my_getc();
		if (c == 0x0D) {
			*p='\0';
			break;
		}
		*p++ = c;
		////// uart0_putc(c);
	}
	return len - i;
}


_ssize_t _write_r (
    struct _reent *r, 
    int file, 
    const void *ptr, 
    size_t len)
{
	unsigned int i;
	const unsigned char *p;
	(void)r; (void)file;
	
	p = (const unsigned char*) ptr;
	
	for (i = 0; i < len; i++) {
		if (*p == '\n' ) my_putc('\r');
		my_putc(*p++);
	}
	
	return len;
}


int _close_r(
    struct _reent *r, 
    int file)
{
	(void)r; (void)file;
	return 0;
}


_off_t _lseek_r(
    struct _reent *r, 
    int file, 
    _off_t ptr, 
    int dir)
{
	(void)r; (void)file; (void)ptr; (void)dir;
	return (_off_t)0;	/*  Always indicate we are at file beginning.  */
}


int _fstat_r(
    struct _reent *r, 
    int file, 
    struct stat *st)
{
	(void)r; (void)file;
	/*  Always set as character device.				*/
	st->st_mode = S_IFCHR;
	/* assigned to strong type with implicit 	*/
	/* signed/unsigned conversion.  Required by 	*/
	/* newlib.					*/

	return 0;
}


int _isatty_r(struct _reent *r, int file)
{
	(void)r; (void)file;
	return 1;
}


#if 0
static void _exit (int n) {
label:  goto label; /* endless loop */
}
#endif 
