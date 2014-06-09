TARGET=rgb-sensor
ARCH=LPC13
CPU=$(ARCH)43
DEBUG=-g
OPTIM=-O3 -mword-relocations

APP_CFLAGS=-Iinc -std=gnu99 -fgnu89-inline -D__USE_CMSIS
APP_LDFLAGS=-lm

APP_SRC= \
  src/main.c \
  src/usbserial.c

APP_SRC+=$(IMAGES_C)

all: $(TARGET).bin

app_clean:
	rm -f $(TARGET).zip $(TARGET)-firmware.bin
	find src -name '*.o' -exec rm \{\} \;

clean_all: clean
	rm -f README $(CUSTOM_KEY)

indent:
	find src inc -iname '*.[ch]' -exec indent -c81 -i4 -cli4 -bli0 -ts 4 \{\} \;
	rm -f src/*.[ch]~ inc/*.[ch]~

include ../core/Makefile.rules
