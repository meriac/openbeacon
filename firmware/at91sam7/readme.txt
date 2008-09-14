OpenBeacon core firmware source code tree
--------------------------------------------

Copyright 2007 Milosch Meriac <milosch@openbeacon.de>
based on http://www.freertos.org/ embedded operating system

Visit http://www.openbeacon.org/ and http://wiki.openbeacon.org/ for further
informations.

All supplied code is licensed under GNU General Public License Version 2,
June 1991. See "license.txt" file for license informations. Alternative
license types are available for the "/application"-tree on request. Please
contact milosch@openbeacon.de on license issues.

The contained firmware source code is verified to compile flawless by using
GCC version 4.2.3 configured as ARM cross compiler.

You can get a pre-built ARM EABI toolchain for free at:
http://www.codesourcery.com/gnu_toolchains/arm/download.html

The USB device creates a virtual serial COM port - you can access it by using
115200 baud, 8 data bit, no parity and one stop bit. Under Linux just plug it
into your computer - a /dev/ttyACM? device will appear automagically.

We personally prefer the "cu" command line tool from uucp-Package to access
the created serial port easily under Linux based operating systems.

For Microsoft Windows you have to use the supplied .inf-file from the
"win32driver" directory - you can verify the assigned COM port by using the
control panel.

For Linux use the contained "at91flash"-tool to udpate to contained firmware.
The supplied at91flash script depends on the sam7 tool:

http://oss.tekno.us/sam7utils/

Just issue a "make clean flash" to flash the current project specific firmware
image.

For Microsoft(tm) Windows(tm) use the AT91-ISP.exe package for flashing:
http://www.atmel.com/dyn/products/tools_card.asp?tool_id=3883 

Enjoy !