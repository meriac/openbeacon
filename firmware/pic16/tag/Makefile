COMPILER_PATH=/usr/hitech/picc/9.80/bin/
PK2=/usr/local/bin/pk2cmd
CC=$(COMPILER_PATH)picc

CONFIG=0x3d4
CHIP=16F684
PROGNAME=openbeacontag

SRCS=main.c timer.c nRF_CMD.c xxtea.c

PROG_EXTS=cof hxl map mum sym as sdb
PROG_FILES=$(patsubst %, obj/$(PROGNAME).%, $(PROG_EXTS))

OBJ_EXTS=p1 rlf sdb lst pre as
OBJ_TMP=$(foreach T, $(OBJ_EXTS), $(patsubst %.c, obj/%.$(T), $(SRCS)))
OBJ_OUT=$(patsubst %.c, obj/%.p1, $(SRCS))

CLEANS=$(OBJ_OUT) $(OBJ_TMP) $(PROG_FILES)
CLEANS+=obj/startup.as obj/startup.lst obj/startup.rlf obj/$(PROGNAME).hex $(PROGNAME).hex obj/$(PROGNAME).obj obj/$(PROGNAME).lst obj/$(PROGNAME).rlf obj/startup.obj funclist

COMMON_FLAGS=-q --MODE=std --outdir=obj --chip=$(CHIP) --codeoffset=0 --warn=0 --errors=1- --char=signed --double=24
CFLAGS=$(COMMON_FLAGS) --pass1
LDFLAGS=$(COMMON_FLAGS) --summary=mem,file --asmlist --output=+intel --output=+mcof --runtime=+init --runtime=+osccal --runtime=+clear --runtime=+clib --runtime=+keep -m

all: $(PROGNAME).hex

obj/%.p1: src/%.c src/*.h
	$(CC) $(CFLAGS) -c $(filter-out %.h,$^)

$(PROGNAME).hex: $(OBJ_OUT)
	$(CC) $(LDFLAGS) -Oobj/$@ $^

clean:
	rm -f $(CLEANS)

install: counted
	$(PK2) -A4 -PPIC$(CHIP) -E -Fobj/$(PROGNAME).hex -M -YP

publish: counted
	scp -P30303 $(PROGNAME).hex singularity.bitmanufaktur.net:/home/wwwrun/open.bitmanufaktur.com/web/www/people/milosch/pictag/

counted: $(PROGNAME).hex
	php ./create_counted_firmware.php
