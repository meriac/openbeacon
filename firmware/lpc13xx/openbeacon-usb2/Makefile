TARGET=openbeacon-usb2
ARCH=LPC13
CPU=$(ARCH)43
DEBUG=-g
OPTIM=-Os -mword-relocations

APP_CFLAGS=-Iinc -std=gnu99 -fgnu89-inline -D__USE_CMSIS
APP_LDFLAGS=-lm

APP_SRC= \
  src/main.c \
  src/nRF_CMD.c \
  src/nRF_API.c \
  src/bluetooth.c \
  src/hid.c \
  src/pin.c \
  src/3d_acceleration.c \
  src/storage.c \
  src/spi.c

APP_SRC+=$(IMAGES_C)

all: $(TARGET).bin

app_clean:
	find src -name '*.o' -exec rm \{\} \;

include ../core/Makefile.rules
