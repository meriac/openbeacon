MEMORY 
{
	flash	: ORIGIN = 0x00100000, LENGTH = 64K - 1K /* save 1K for environment on top of flash */
	ram		: ORIGIN = 0x00200000, LENGTH = 16K
}

__stack_end__ = 0x00200000 + 16K - 4;

ENTRY(start)

SECTIONS 
{
	. = 0;
	startup : {
	    *(.bootloader)
	    *(.startup)
	} >flash

	prog : 
	{
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata*)
		*(.glue_7)
		*(.glue_7t)
	} >flash

	__end_of_text__ = .;

	.data : 
	{
		__data_beg__ = .;
		__data_beg_src__ = __end_of_text__;
		*(.data)
		*(.data.*)
		*(.ramfunc)
		__data_end__ = .;
	} >ram AT>flash

	.bss : 
	{
		__bss_beg__ = .;
		*(.bss)
		*(.bss.*)
	} >ram

	/* Align here to ensure that the .bss section occupies space up to
	_end.  Align after .bss to ensure correct alignment even if the
	.bss section disappears because there are no input sections.  */
	. = ALIGN(32 / 8);
}
	. = ALIGN(32 / 8);
	_end = .;
	_bss_end__ = . ; __bss_end__ = . ; __end__ = . ;
	PROVIDE (end = .);


