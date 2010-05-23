TARGET=usbcdc
ARCH=LPC13
CPU=$(ARCH)42
DEBUG=-g
OPTIM=-O2

APP_CFLAGS=-Iinc -std=gnu99 -fgnu89-inline -D__USE_CMSIS
APP_LDFLAGS=-lm

APP_SRC= \
  src/cdcuser.c \
  src/cr_startup_lpc13.c \
  src/serial.c \
  src/usbcore.c \
  src/usbdesc.c \
  src/usbhw.c \
  src/usbuser.c \
  src/vcomdemo.c

APP_SRC+=$(IMAGES_C)

all: $(TARGET).bin

app_clean:
	find src -name '*.o' -exec rm \{\} \;

include ../core/Makefile.rules

