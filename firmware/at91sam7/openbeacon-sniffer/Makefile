TARGET=openbeacon-sniffer
ARCH=AT91SAM7S
CPU=$(ARCH)128
DEBUG=-g
OPTIM=-O2

APP_CFLAGS=-Iapplication

APP_SRC= \
  application/main.c \
  application/proto.c \
  application/xxtea.c

all: $(TARGET).bin

app_clean:
	find application -name '*.o' -exec rm \{\} \;

indent:
	for FILE in application/*.[ch] config/*.[ch]; do indent -c81 -i4 -cli4 -bli0 -ts 4 $$FILE; done
	rm -f application/*.[ch]~ config/*.[ch]~

include ../core/Makefile.rules
