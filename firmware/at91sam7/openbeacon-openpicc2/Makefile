TARGET=openbeacon-openpicc2
ARCH=AT91SAM7SE
CPU=$(ARCH)512
DEBUG=-g
OPTIM=-O2

APP_CFLAGS=-Iapplication -DLZO_CFG_NO_UNALIGNED -DLZO_ALIGNED_OK_4 -DHAVE_FLASH_CONTENT
APP_LDFLAGS=-lm

IMAGES=splash directory arrows

APP_SRC= \
  application/main.c \
  application/proto.c \
  application/xxtea.c \
  application/sdram.c \
  application/power.c \
  application/pio_irq.c \
  application/spi.c \
  application/sdcard.c \
  application/adc.c \
  application/accelerometer.c \
  application/eink/eink.c \
  application/eink/eink_flash.c \
  application/eink/eink_mgmt.c \
  application/lzo/minilzo.c \
  application/touch/ad7147.c \
  application/touch/ad7147_registers.c \
  application/touch/slider.c \
  application/ui/ui.c \
  application/nfc/pn532.c \
  application/nfc/pn532_demo.c \
  application/nfc/picc_emu.c \
  application/ebook/ebook.c \
  application/ebook/event.c

IMAGES_C=$(foreach i,$(IMAGES),application/$(i)_pgm.c)
APP_SRC+=$(IMAGES_C)

all_foo:
	make OPTIM=-O0 application/spi.o
	make all

all: $(TARGET).bin

app_clean:
	find application -name '*.o' -exec rm \{\} \;
	rm -f generate_splash $(IMAGES_C)

include ../core/Makefile.rules

generate_splash: generate_splash.c application/lzo/minilzo.c
	gcc generate_splash.c application/lzo/minilzo.c -o generate_splash -Iapplication -lnetpbm -ggdb -lm

$(IMAGES_C):application/%_pgm.c:%.pgm generate_splash
	./generate_splash $< `basename $< .pgm` > $@

application/eink/eink_flash.c: application/eink/eink_flash_content.h
application/eink/eink_flash_content.h:
	echo 'const unsigned char eink_flash_content[] = {};' > application/eink/eink_flash_content.h
