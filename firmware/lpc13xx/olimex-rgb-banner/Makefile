TARGET=olimex-rgb-banner
ARCH=LPC13
CPU=$(ARCH)43
DEBUG=-g
OPTIM=-Os -mword-relocations

IMAGE=src/image.h

APP_CFLAGS=-Iinc -std=gnu99 -fgnu89-inline -D__USE_CMSIS
APP_LDFLAGS=-lm

APP_SRC= \
  src/main.c

APP_SRC+=$(IMAGES_C)

all: $(IMAGE) $(TARGET).bin

$(IMAGE): image.png
	tools/convert.php > $@

indent:
	find src inc -iname '*.[ch]' -exec indent -c81 -i4 -cli4 -bli0 -ts 4 \{\} \;
	rm -f src/*.[ch]~ inc/*.[ch]~

app_clean:
	find src -name '*.o' -exec rm \{\} \;
	rm -f $(IMAGE)

include ../core/Makefile.rules
