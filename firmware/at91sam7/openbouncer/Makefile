TARGET=openbouncer
ARCH=AT91SAM7S
CPU=$(ARCH)128
DEBUG=-g
OPTIM=-O2

APP_CFLAGS=-Iapplication    

APP_SRC= \
  application/main.c \
  application/xxtea.c \
  application/authentication.c \
  application/cmd/cmd.c \
  application/cmd/cmd_key.c \
  application/cmd/cmd_main.c \
  application/cmd/cmd_user.c \
  application/cmd/commandline.c \
  application/cmd/login.c \
  application/cmd/usbio.c \
  application/cmd/vsprintf.c \
  application/cmd/vt100.c \
  application/BurgWaechter_TSE_3000/lock.c
  
all: $(TARGET).bin
  
app_clean:
	find application -name '*.o' -exec rm \{\} \;

include ../core/Makefile.rules
