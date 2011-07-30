TARGET=openbeacon-fossin
ARCH=AT91SAM7S
CPU=$(ARCH)128
DEBUG=-g
OPTIM=-O2

AUDIO_FREQUENCY=8000

APP_CFLAGS=-Iapplication -DAUDIO_FREQUENCY=$(AUDIO_FREQUENCY)
APP_SRC= \
  application/main.c \
  application/cmd.c

BOOTLOADER=application/audio.s

AUDIO_SRC=audio.raw

all: $(AUDIO_SRC) $(TARGET).bin

%.raw: %.wav
	sox $< -c1 -1 -u -r $(AUDIO_FREQUENCY) $@

play: $(AUDIO_SRC)
	play -r $(AUDIO_FREQUENCY) -u -1 -c1 $<

app_clean:
	find application -name '*.o' -exec rm \{\} \;
	rm -f $(AUDIO_SRC)

include ../core/Makefile.rules
