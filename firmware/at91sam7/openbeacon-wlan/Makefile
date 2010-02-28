TARGET=openbeacon-wlan
ARCH=AT91SAM7SE
CPU=$(ARCH)512
DEBUG=-g
OPTIM=-O2

APP_CFLAGS=-Iapplication -std=gnu99 -fgnu89-inline
APP_LDFLAGS=-lm

APP_SRC= \
  $(CORE)/openbeacon/rnd.c \
  application/main.c \
  application/sdram.c \
  application/wifi.c \
  application/xxtea.c

APP_SRC+=$(IMAGES_C)

all: $(TARGET).bin 

app_clean:
	find application -name '*.o' -exec rm \{\} \;

include ../core/Makefile.rules

LDSCRIPT=config/atmel-rom.ld
