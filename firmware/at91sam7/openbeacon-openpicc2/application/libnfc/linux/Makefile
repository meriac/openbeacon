###############################################
# Linux settings
###############################################
LIBPCSC_HEADERS=`pkg-config --cflags libpcsclite`
LIBPCSC_LIB=`pkg-config --libs libpcsclite`
LIBUSB_HEADERS=`pkg-config --cflags libusb`
LIBUSB_LIB=`pkg-config --libs libusb`
LIBNFC_TYPE=so
LIBNFC_CFLAGS=-shared
LIBNFC_LINK=-Wl,-rpath,.

include ../Makefile.rules
