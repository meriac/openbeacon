TARGET=openbeacon-wlan
ARCH=AT91SAM7SE
CPU=$(ARCH)512
DEBUG=-g
OPTIM=-O2

APP_CFLAGS=-Iapplication -std=gnu99 -fgnu89-inline
APP_LDFLAGS=-lm

APP_SRC= \
  $(CORE)openbeacon/crc32.c \
  application/main.c \
  application/proto.c \
  application/xxtea.c \
  application/sdram.c \
  application/spi.c

APP_SRC+=$(IMAGES_C)

all_foo:
	make OPTIM=-O0 application/spi.o
	make all

all: $(TARGET).bin 

app_clean:
	find application -name '*.o' -exec rm \{\} \;

include ../core/Makefile.rules

LDSCRIPT=config/atmel-rom.ld
