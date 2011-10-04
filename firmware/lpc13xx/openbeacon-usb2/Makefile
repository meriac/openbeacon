TARGET=openbeacon-usb2
ARCH=LPC13
CPU=$(ARCH)43
DEBUG=-g
OPTIM=-Os -mword-relocations

# determine if we have custom encryption keys
ifeq ("$(wildcard inc/custom-encryption-key.h)","")
	ENCRYPTION_KEY:=
else
	ENCRYPTION_KEY:=-DCUSTOM_ENCRYPTION_KEY
endif

APP_CFLAGS=-Iinc -std=gnu99 -fgnu89-inline -D__USE_CMSIS $(ENCRYPTION_KEY)
APP_LDFLAGS=-lm

APP_SRC= \
  src/main.c \
  src/pmu.c \
  src/nRF_CMD.c \
  src/nRF_API.c \
  src/storage.c \
  src/bluetooth.c \
  src/3d_acceleration.c

APP_SRC+=$(IMAGES_C)

all: README $(TARGET).bin

publish: clean $(TARGET).bin
	rm -f $(TARGET).zip
	../lpc-flash/src/lpc-flash $(TARGET).bin $(TARGET)-firmware.bin
	zip $(TARGET).zip $(TARGET)-firmware.bin
	scp $(TARGET).zip $(TARGET)-firmware.bin meri@bitmanufaktur.net:/home/wwwrun/open.bitmanufaktur.com/web/www/people/milosch/usb2tag/

README: .readme
	groff -Tascii $< >$@

app_clean:
	rm -f $(TARGET).zip $(TARGET)-firmware.bin
	find src -name '*.o' -exec rm \{\} \;

clean_all: clean
	rm -f README

include ../core/Makefile.rules
