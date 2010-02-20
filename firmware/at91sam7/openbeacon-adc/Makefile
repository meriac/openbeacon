TARGET=openbeacon-heartbeat
ARCH=AT91SAM7S
CPU=$(ARCH)32
DEBUG=-g
OPTIM=-O2

APP_CFLAGS=-Iapplication -std=gnu99 -fgnu89-inline -DDISABLE_WATCHDOG
APP_LDFLAGS=-lm

APP_SRC= \
  application/main.c \
  application/proto.c

APP_SRC+=$(IMAGES_C)

all: $(TARGET).bin

app_clean:
	find application -name '*.o' -exec rm \{\} \;

include ../core/Makefile.rules

LDSCRIPT=config/atmel-rom.ld
