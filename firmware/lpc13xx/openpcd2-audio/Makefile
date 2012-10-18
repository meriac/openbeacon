TARGET=openpcd2-audio
ARCH=LPC13
CPU=$(ARCH)43
DEBUG=-g
OPTIM=-O1 -mword-relocations

APP_CFLAGS=-Iinc -Ifont -std=gnu99 -fgnu89-inline -D__USE_CMSIS
APP_LDFLAGS=-lm

APP_SRC= \
  src/main.c \
  src/audio.c \
  src/lcd.c \
  src/storage.c

APP_SRC+=$(IMAGES_C)

all: $(TARGET).bin

indent:
	find src inc -iname '*.[ch]' -exec indent -c81 -i4 -cli4 -bli0 -ts 4 \{\} \;
	rm -f src/*.[ch]~ inc/*.[ch]~

app_clean:
	find src font -name '*.o' -exec rm \{\} \;

include ../core/Makefile.rules
