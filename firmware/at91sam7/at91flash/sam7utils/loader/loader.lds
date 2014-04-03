/***********************************************************************/
/*                                                                     */
/*  ROM.ld:  Linker Script File                                        */
/*                                                                     */
/***********************************************************************/
ENTRY(_entry)

MEMORY
{
	LOADER(rwx) : ORIGIN = 0x00201600, LENGTH = 0xEA00
}

/* Section Definitions */
SECTIONS
{

  /* first section is .text which is used for code */
   .text :
  {
	_text = .;
	PROVIDE (text = _text);	
	*crt0.o (.text)            /* Startup code */
	*(.text .text.*)           /* remaining code */
	*(.gnu.linkonce.t.*)
	*(.glue_7)
	*(.glue_7t)
	*(.gcc_except_table)
	*(.rodata)                 /* read-only data (constants) */
	*(.rodata*)
	*(.gnu.linkonce.r.*)
  } > LOADER
  
  . = ALIGN(4);

  _etext = . ;
  PROVIDE (etext = .);

  /* .data section which is used for initialized data */
  .data : AT (_etext)
  {
    _data = .;
    *(.data)
	*(.data.*)
	*(.gnu.linkonce.d*)
	SORT(CONSTRUCTORS) /* mt 4/2005 */
  } > LOADER
 
  . = ALIGN(4);
  _edata = . ;
  PROVIDE (edata = .);

  /* .bss section which is used for uninitialized data */
  .bss (NOLOAD) :
  {
    __bss_start = . ;
    __bss_start__ = . ;
    *(.bss)
	*(.gnu.linkonce.b*)
    *(COMMON)
    . = ALIGN(4);
  } > LOADER

  . = ALIGN(4);
  __bss_end__ = . ;
  PROVIDE (__bss_end = .);



  _end = . ;
  PROVIDE (end = .);

  /* Stabs debugging sections.  */
  .stab          0 : { *(.stab) }
  .stabstr       0 : { *(.stabstr) }
  .stab.excl     0 : { *(.stab.excl) }
  .stab.exclstr  0 : { *(.stab.exclstr) }
  .stab.index    0 : { *(.stab.index) }
  .stab.indexstr 0 : { *(.stab.indexstr) }
  .comment       0 : { *(.comment) }
  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to the beginning
     of the section so we begin them at 0.  */
  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }
  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }
  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }
  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }
  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
}


PROVIDE(tc     = 0xFFFA0000);
PROVIDE(udp    = 0xFFFB0000);
PROVIDE(twi    = 0xFFFB8000);
PROVIDE(usart0 = 0xFFFC0000);
PROVIDE(usart1 = 0xFFFC4000);
PROVIDE(pwmc   = 0xFFFCC000);
PROVIDE(ssc    = 0xFFFD4000);
PROVIDE(adc    = 0xFFFD8000);
PROVIDE(spi    = 0xFFFE0000);
PROVIDE(aic    = 0xFFFFF000);
PROVIDE(dbgu   = 0xFFFFF200);
PROVIDE(pioa   = 0xFFFFF400);
PROVIDE(pmc    = 0xFFFFFC00);
PROVIDE(rstc   = 0xFFFFFD00);
PROVIDE(rtt    = 0xFFFFFD20);
PROVIDE(pit    = 0xFFFFFD30);
PROVIDE(wdt    = 0xFFFFFD40);
PROVIDE(vreg   = 0xFFFFFD60);
PROVIDE(mc     = 0xFFFFFF00);


