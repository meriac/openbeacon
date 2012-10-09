TARGET=openpcd2-libnfc
ARCH=LPC13
CPU=$(ARCH)42
DEBUG=-g
OPTIM=-O2 -mword-relocations

APP_CFLAGS=-Iinc -std=gnu99 -fgnu89-inline -D__USE_CMSIS
APP_LDFLAGS=-lm

APP_SRC= \
  src/main.c \
  src/usbserial.c

APP_SRC+=$(IMAGES_C)

all: $(TARGET).bin

indent:
	find src inc -iname '*.[ch]' -exec indent -c81 -i4 -cli4 -bli0 -ts 4 \{\} \;
	rm -f src/*.[ch]~ inc/*.[ch]~

app_clean:
	find src -name '*.o' -exec rm \{\} \;

include ../core/Makefile.rules
