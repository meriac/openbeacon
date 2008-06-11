TARGET=openbouncer
ARCH=AT91SAM7S128
DEBUG=-g
OPTIM=-O2

APP_CFLAGS=-Iapplication    

APP_SRC= \
  application/main.c \
  application/led.c \
  application/cmd_key.c \
  application/cmd_main.c \
  application/cmd_user.c \
  application/commandline.c \
  application/login.c \
  application/usbio.c \
  application/vsprintf.c \
  application/vt100.c  
  
all: $(TARGET).bin
  
app_clean:
	find application -name '*.o' -exec rm \{\} \;

include ../core/Makefile.rules
