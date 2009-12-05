TARGET=openbeacon-fossin
ARCH=AT91SAM7S
CPU=$(ARCH)128
DEBUG=-g
OPTIM=-O2

APP_CFLAGS=-Iapplication

APP_SRC= \
  application/main.c \
  application/cmd.c

AUDIO_SRC=audio.raw

all: $(TARGET).bin

$(TARGET).bin: $(AUDIO_SRC)

%.raw: %.wav
	sox $< -c1 -1 -u -r 12000 $@

play: $(AUDIO_SRC)
	play -r 12000 -u -1 -c1 $<

app_clean:
	find application -name '*.o' -exec rm \{\} \;
	rm -rf $(AUDIO_SRC)

include ../core/Makefile.rules
