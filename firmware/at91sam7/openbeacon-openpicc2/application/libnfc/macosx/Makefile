###############################################
# Mac OS X settings
###############################################
LIBPCSC_HEADERS=-I/System/Library/Frameworks/PCSC.framework/Headers
LIBPCSC_LIB=-framework PCSC
LIBUSB_HEADERS=`libusb-config --cflags`
LIBUSB_LIB=`libusb-config --libs`
LIBNFC_TYPE=dylib
LIBNFC_CFLAGS=-dynamiclib
LIBNFC_LINK=

include ../Makefile.rules
