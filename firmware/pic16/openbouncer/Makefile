COMPILER_PATH=/usr/hitech/picc/pro/9.70
PK2=/usr/local/bin/pk2
CC=$(COMPILER_PATH)/bin/picc

CONFIG=0x3d4
CHIP=16F684
PROGNAME=openbouncer

SRCS=main.c timer.c nRF_CMD.c protocol.c

PROG_EXTS=cof hxl map mum sym as sdb
PROG_FILES=$(patsubst %, obj/$(PROGNAME).%, $(PROG_EXTS))

OBJ_EXTS=p1 rlf sdb lst pre as
OBJ_TMP=$(foreach T, $(OBJ_EXTS), $(patsubst %.c, obj/%.$(T), $(SRCS)))
OBJ_OUT=$(patsubst %.c, obj/%.p1, $(SRCS))

CLEANS=$(OBJ_OUT) $(OBJ_TMP) $(PROG_FILES)
CLEANS+=obj/startup.as obj/startup.lst obj/startup.rlf obj/$(PROGNAME).hex $(PROGNAME).hex 

COMMON_FLAGS=-q --MODE=pro --outdir=obj --chip=$(CHIP) --codeoffset=0 --warn=0 --errors=1- --char=signed --double=24
CFLAGS=$(COMMON_FLAGS) --pass1
LDFLAGS=$(COMMON_FLAGS) --summary=mem,file --asmlist --output=+intel --output=+mcof --runtime=+init --runtime=+osccal --runtime=+clear --runtime=+clib --runtime=+keep -m

all: $(PROGNAME).hex

obj/%.p1: src/%.c src/*.h
	$(CC) $(CFLAGS) -c $(filter-out %.h,$^)

$(PROGNAME).hex: $(OBJ_OUT)
	$(CC) $(LDFLAGS) -Oobj/$@ $^

clean:
	rm -f $(CLEANS)

install: $(PROGNAME).hex
	$(PK2) --device=$(CHIP) --write obj/$(PROGNAME).hex

publish: $(PROGNAME).hex
	scp -P30303 obj/$(PROGNAME).hex bitmanufaktur.de:/home/wwwrun/wildcard.bitmanufaktur/www/thelasthope/$(PROGNAME).raw
	scp -P30303 obj/$(PROGNAME).sym bitmanufaktur.de:/home/wwwrun/wildcard.bitmanufaktur/www/thelasthope/

verify: $(PROGNAME).hex
	sudo $(PK2) --verify obj/$(PROGNAME).hex

counted: $(PROGNAME).hex
	./create_counted_firmware.php

on:
	sudo $(PK2) --on

off:
	sudo $(PK2) --off
