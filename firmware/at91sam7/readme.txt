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
GCC version 4.0.2 configured as ARM cross compiler.

The USB device creates a virtual serial COM port - you can access it by using
115200 baud, 8 data bit, no parity and one stop bit. Under Linux just plug it
into your computer - a /dev/ttyACM? device will appear automagically.

We personally prefer the "cu" command line tool from uucp-Package to access
the created serial port easily under Linux based operating systems.

For Microsoft Windows you have to use the supplied .inf-file from the
"win32driver" directory - you can verify the assigned COM port by using the
control panel.

Use the contained "at91flash"-tool to udpate to contained firmware. The
supplied script depends on our modified sam7 tool:
http://www.openpcd.org/dl/sam7utils-0.1.0-bm.tar.bz2

Subdirectory list:

[openbeacon-usb]

This direcectory contains the Standard Tag Distance Estimator version. This
firmware makes a list of tags within its range and sorts them by distance and
by up-to-dateness. Nearer tags expire earlier because 

Please make sure to change the contained key in xxtea.c to a random key that matches
your tag!

[openbouncer]

...




Enjoy !