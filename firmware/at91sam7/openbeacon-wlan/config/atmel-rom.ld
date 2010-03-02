MEMORY 
{
	/* FIXME: LENGTH should be 512k - 1k, but this needs an update of env.c */
	flash	: ORIGIN = 0x00100000, LENGTH = 256K - 1K /* save 1K for environment on top of flash */
	ram	: ORIGIN = 0x00200000, LENGTH = 32K
	sdram	: ORIGIN = 0x20000000, LENGTH = 16M
}

__stack_end__ = 0x00200000 + 32K - 4;

ENTRY(start)

SECTIONS 
{
	. = 0;
	startup : { *(.startup)} >flash

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
		*(.fastrun)
		*(.ramfunc)
		__data_end__ = .;
	} >sdram AT>flash
  /* .ARM.exidx is sorted, so has to go in its own output section.  */
  __exidx_start = .;
  .ARM.exidx :
  {
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
  } >sdram AT>flash
  __exidx_end = .;
  
	.bss : 
	{
		__bss_beg__ = .;
		*(.bss)
		*(.bss.*)
	} >sdram

	/* Align here to ensure that the .bss section occupies space up to
	_end.  Align after .bss to ensure correct alignment even if the
	.bss section disappears because there are no input sections.  */
	. = ALIGN(32 / 8);
	_end = .;
	_bss_end__ = . ; __bss_end__ = . ; __end__ = . ;
	PROVIDE (end = .);

	/* SDRAM segment */
	
	. = ALIGN(32 / 8);
	.sdram (NOLOAD) : 
	{
		*(.sdram)
		*(.sdram.*)
		PROVIDE (sdram_end = .);
	} >sdram
	. = ALIGN(32 / 8);
}
	. = ALIGN(32 / 8);


