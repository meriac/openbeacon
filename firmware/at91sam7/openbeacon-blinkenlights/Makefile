TARGET=openbeacon-blinkenlights
ARCH=AT91SAM7S128
DEBUG=-g
OPTIM=-O2

APP_CFLAGS=-Iapplication    

APP_SRC= \
  application/main.c \
  application/led.c \
  application/proto.c \
  application/xxtea.c
  
all: $(TARGET).bin
  
app_clean:
	find application -name '*.o' -exec rm \{\} \;

include ../core/Makefile.rules
