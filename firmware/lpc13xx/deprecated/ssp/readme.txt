The SSP project is a simple program to test both SSP master
and slave mode on the NXP LPC13xx microcontroller. 

There are three modes in SSP: loopback, master or slave.

Here are the combination of all the tests. You need to modify
the setting in ssp.h first in order to configure the proper mode
for testing:

(1) LOOPBACK test:	LOOPBACK_MODE=1, TX_RX_ONLY=0, USE_CS=1;
(2) Serial EEPROM test:	LOOPBACK_MODE=0, TX_RX_ONLY=0, USE_CS=0; (default)
(3) TX(Master) Only:	LOOPBACK_MODE=0, SSP_SLAVE=0, TX_RX_ONLY=1, USE_CS=1
(4) RX(Slave) Only:	LOOPBACK_MODE=0, SSP_SLAVE=1, TX_RX_ONLY=0, USE_CS=1

#define LOOPBACK_MODE	0	/* 1 is loopback, 0 is normal operation. */
#define SSP_SLAVE	0	/* 1 is SLAVE mode, 0 is master mode */
#define TX_RX_ONLY	0	/* 1 is TX or RX only depending on SSP_SLAVE
				flag, 0 is either loopback mode or communicate
				with a serial EEPROM. */

If USE_CS is zero, set SSEL as GPIO that the master has total control of the 
sequence. Depending on the timing sequence of the slave device, manually control 
the SSEL may be required on the master side.

When test serial SEEPROM(LOOPBACK_MODE=0, TX_RX_ONLY=0), set USE_CS to 0.
When LOOPBACK_MODE=1 or TX_RX_ONLY=1, set USE_CS to 1.

#define USE_CS			0

The result of the RX may also be checked in the dest_addr[].


