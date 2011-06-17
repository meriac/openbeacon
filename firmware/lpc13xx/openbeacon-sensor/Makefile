TARGET=openbeacon-sensor
ARCH=LPC13
CPU=$(ARCH)42
DEBUG=-g
OPTIM=-Os -mword-relocations

APP_CFLAGS=-Iinc -std=gnu99 -fgnu89-inline -D__USE_CMSIS
APP_LDFLAGS=-lm

APP_SRC= \
  src/main.c \
  src/pmu.c \
  src/sound.c \
  src/nRF_CMD.c \
  src/nRF_API.c \
  src/3d_acceleration.c

APP_SRC+=$(IMAGES_C)

all: $(TARGET).bin

publish: clean $(TARGET).bin
	rm -f $(TARGET).zip
	../lpc-flash/src/lpc-flash $(TARGET).bin $(TARGET)-tag.bin
	zip $(TARGET)-tag.zip $(TARGET)-tag.bin
	scp $(TARGET)-tag.zip $(TARGET)-tag.bin meri@bitmanufaktur.net:/home/wwwrun/open.bitmanufaktur.com/web/www/people/milosch/artcom/

app_clean:
	rm -f $(TARGET).zip $(TARGET)-firmware.bin
	find src -name '*.o' -exec rm \{\} \;

include ../core/Makefile.rules
