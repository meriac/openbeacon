TARGET=usbhid_rom_tiny
ARCH=LPC13
CPU=$(ARCH)42
DEBUG=-g
OPTIM=-Os -mword-relocations

APP_CFLAGS=-Iinc -std=gnu99 -fgnu89-inline -D__USE_CMSIS
APP_LDFLAGS=-lm

APP_SRC= \
  src/usbhidrom_tiny_main.c \
  src/cr_startup_lpc13_tiny.c

APP_SRC+=$(IMAGES_C)

all: $(TARGET).bin

app_clean:
	find src -name '*.o' -exec rm \{\} \;

include ../core/Makefile.rules

