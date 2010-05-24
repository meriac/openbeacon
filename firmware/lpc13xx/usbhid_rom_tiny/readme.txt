usbhid_rom_tiny
=====================
This project contains an HID USB example for the LPCXpresso board
mounted with an LPC1343 Cortex-M3 part. This example uses the USB
driver in on-chip ROM, simplifying the code and saving flash space.
It has been heavily optimized for size and it consumes a total of
404 bytes of flash memory.

Here is a summary of the changes that have been made:
Optimization
	Size optimization has been enabled with the -Os option.
CMSIS
	Only the headers are included in the project. The library
	containing the comprehensive initialization code (for example,
	pll setup and calculation of SystemFrequency) is not used.
C libraries
	Are not linked into the project. Setting is in:
	Project Properties
	C/C++ Build   Settings   MCU Linker   Target 
	The setting can't be changed right now because of the custom
	linker script.
Startup code
	No chip initialization code is done. Out of reset the MCU runs
	from the internal oscillator at 12 MHz. The USB init function in
	ROM sets up the PLL for 48 MHz.
Vector table/exception handlers
	The vector table has been split into two sections. All 40 of
	the WAKEUP vectors have been removed (160 bytes). A third
	section has been inserted at the location of the WAKEUP vectors
	and some of the executable code has been moved into this section.
	These changes are in cr_startup_lpc13_tiny.c which is based on
	the original cr_startup_lpc13.c from Code Red.
	Also, the default exception handlers perform a while(1) and are
	only useful when viewed in the debugger. They have all been removed.
	The default interrupt handler has been retained- the same single
	empty handler for all unused interrupts. Review the .map file in the
	Tiny directory to see these changes. Another useful tool to see the
	memory organization is objdump located at:
	c:\nxp\lpcxpresso_3.2\Tools\arm-none-eabi\bin\objdump.exe
	The -d option will disassemble code and show its location.
Misc code changes
	The SetOutReport and GetInReport functions were simplified to
	control just one input line and just one output line. These
	functions were moved to the .vectorcode section so they are
	placed in unused vector table space. Initialization code for
	the HidDevInfo structure was removed and replaced by a "const"
	initialized global declaration which will is accessed in-place
	from flash.
Removal of string descriptors
	The USB string descriptors are optional and are not included in this
	example.

To run this example, you must attach a USB cable to the board. See
the "Getting Started Guide" appendix for details.

When downloaded to the board and executed, the PC will recognize
a USB HID device. LPC1343 HID Demonstration.exe (in the project
directory) can be used to turn the LED on and off.

One thing we have seen that can cause trouble with the USB examples
is the Windows driver install. Since the example projects all use
the same USB Vendor ID and Product ID, if you try out the HID
example and then try out the Mass Storage example, Windows may try
to use the HID driver it had already installed with the Mass
Storage example code. To fix this, go to the Windows Device Manager,
find the broken "HID" device and select "Uninstall." Then unplug the
device and plug it back in. Windows should correctly identify the
device as being a Mass Storage Class device and install the correct
driver.

One thing that makes this project different from many of the other
ones is that 0x180 bytes at the beginning of RAM have been left
free for the ROM USB driver to use. It is important to use the
modified linker script usb_buffer_mem.ld and not to let LPCXpresso
re-generate the linker scripts automatically.

The project makes use of code from the following library projects:
- CMSISv1p30_LPC13xx : for CMSIS 1.30 files relevant to LPC13xx
- LPC13xx_Lib            : for LPC13xx peripheral driver files

These two library projects must exist in the same workspace in order
for the project to successfully build.

For more details, read the comments in config.h
