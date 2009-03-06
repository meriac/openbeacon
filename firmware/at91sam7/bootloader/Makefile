TARGET=bootloader
ARCH=AT91SAM7S
CPU=$(ARCH)128
DEBUG=
OPTIM=-O2
TARGETS=$(TARGET).bin $(TARGET).hex

APP_CFLAGS=-Iapplication

APP_SRC= \
  application/main.c \
  application/update.c \
  application/console.c
 
BOOTLOADER=application/update.s

all: $(TARGETS)

app_clean:
	find application -name '*.o' -exec rm \{\} \;
	rm -rf $(TARGETS)

include ../core/Makefile.rules
