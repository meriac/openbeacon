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

###############################################
# General settings
###############################################
LIBNFC_LDFLAGS=$(LIBPCSC_LIB) $(LIBUSB_LIB)
LIBNFC_PATH=..
VPATH=$(LIBNFC_PATH)
DRIVERS=pn531 acr122

###############################################
# Compiler settings
###############################################
CC = gcc
LD = gcc
CFLAGS = -fPIC -Wall -std=c99 -O4 $(LIBPCSC_HEADERS) $(LIBUSB_HEADERS)
LDFLAGS = -fPIC -Wall -std=c99 -O4

DRIVER_BASES=$(addprefix dev_,$(DRIVERS))
DRIVER_OBJS=$(addsuffix .o,$(DRIVER_BASES))
DRIVER_HEADERS=$(addsuffix .h,$(addprefix $(VPATH)/,$(DRIVER_BASES)))
COMMA=,
DRIVER_INFO_NAMES=$(addsuffix _DRIVER_INFO,$(shell echo $(DRIVERS) | tr a-z A-Z)) 
ENABLED_DRIVERS=$(addsuffix $(COMMA),$(addprefix &, $(strip $(DRIVER_INFO_NAMES))))
OBJS = bitutils.o libnfc.o $(DRIVER_OBJS)
HEADERS = bitutils.h defines.h types.h libnfc.h
LIBNFC = libnfc.$(LIBNFC_TYPE)
EXES = anticol list simulate relay mftool

all: $(LIBNFC) $(EXES)

libnfc.$(LIBNFC_TYPE): $(OBJS)
	$(LD) $(LDFLAGS) -o $(LIBNFC) $(LIBNFC_CFLAGS) $(LIBNFC_LDFLAGS) $(OBJS)

libnfc.a: $(OBJS)
	$(AR) rs libnfc.a $(OBJS)

$(DRIVER_OBJS): %.o : %.c %.h $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $< -DLIBNFC_INTERNALS 

libnfc.o: %.o : %.c %.h $(HEADERS) Makefile
	$(CC) $(CFLAGS) -c -o $@ $< $(patsubst %,-include %,$(DRIVER_HEADERS)) -DLIBNFC_INTERNALS -DENABLED_DRIVERS="$(ENABLED_DRIVERS)" 

% : %.c $(LIBNFC)
	$(LD) $(LDFLAGS) -o $@ $< -L. -lnfc $(LIBNFC_LINK)

clean: 
	rm -f $(OBJS) $(LIBNFC) $(EXES)
