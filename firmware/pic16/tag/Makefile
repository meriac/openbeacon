COMPILER_PATH=/usr/hitech/picc/9.81/bin/
PK2=/usr/local/bin/pk2cmd
CC=$(COMPILER_PATH)picc

MODE=std
CHIP=16F684
PROGNAME=openbeacontag

SRCS=main.c timer.c nRF_CMD.c

PROG_EXTS=cof hxl map mum sym as sdb lst rlf obj
PROG_FILES=$(patsubst %, obj/$(PROGNAME).%, $(PROG_EXTS))

OBJ_EXTS=p1 rlf sdb lst pre as
OBJ_TMP=$(foreach T, $(OBJ_EXTS), $(patsubst %.c, obj/%.$(T), $(SRCS)))
OBJ_OUT=$(patsubst %.c, obj/%.p1, $(SRCS))

CLEANS=$(OBJ_OUT) $(OBJ_TMP) $(PROG_FILES)
CLEANS+=obj/startup.as obj/startup.lst obj/startup.rlf obj/$(PROGNAME).hex $(PROGNAME).hex obj/startup.obj funclist

COMMON_FLAGS=-q --outdir=obj --MODE=$(MODE) --chip=$(CHIP) --codeoffset=0 --warn=0 --errors=1- --char=signed --double=24
CFLAGS=$(COMMON_FLAGS) --pass1
LDFLAGS=$(COMMON_FLAGS) --summary=mem,file --asmlist --output=+intel --output=+mcof --runtime=+init --runtime=+osccal --runtime=+clear --runtime=+clib --runtime=+keep -m

all: $(PROGNAME).hex

obj/%.p1: src/%.c src/*.h
	$(CC) $(CFLAGS) -c $(filter-out %.h,$^)

$(PROGNAME).hex: $(OBJ_OUT)
	$(CC) $(LDFLAGS) -Oobj/$@ $^

clean:
	rm -f $(CLEANS)

flash: counted
	$(PK2) -A5 -PPIC$(CHIP) -F$(PROGNAME).hex -M -YP

publish: counted
	cp $(PROGNAME).hex /mnt/Projekte/OpenBeacon/

counted: $(PROGNAME).hex
	php ./create_counted_firmware.php
