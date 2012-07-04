COMPILER_PATH=/usr/hitech/picc/9.83/bin/
PK2=/usr/local/bin/pk2cmd
CC=$(COMPILER_PATH)picc

MODE=std
CHIP=16F688
PROGNAME=openbeacon-proximity

MAIN=main.c
SRCS=$(MAIN) timer.c nRF_CMD.c

# determine program version
PROGRAM_VERSION:=$(shell git describe --tags --abbrev=4 --dirty 2>/dev/null | sed s/^v//)
ifeq ("$(PROGRAM_VERSION)","")
    PROGRAM_VERSION:='unknown'
endif

PROG_EXTS=cof hxl map mum sym as sdb lst rlf obj
PROG_FILES=$(patsubst %, obj/$(PROGNAME).%, $(PROG_EXTS))
PROG_TARGET=$(PROGNAME)-PIC$(CHIP)-v$(PROGRAM_VERSION)

OBJ_EXTS=p1 rlf sdb lst pre as
OBJ_TMP=$(foreach T, $(OBJ_EXTS), $(patsubst %.c, obj/%.$(T), $(SRCS)))
OBJ_OUT=$(patsubst %.c, obj/%.p1, $(SRCS))

CLEANS=$(OBJ_OUT) $(OBJ_TMP) $(PROG_FILES) $(PROG_TARGET)
CLEANS+=obj/startup.as obj/startup.lst obj/startup.rlf obj/$(PROGNAME).hex $(PROGNAME).hex obj/startup.obj funclist $(PROGNAME)-*.zip src/*.[ch]~

COMMON_FLAGS=-q --MODE=$(MODE) --outdir=obj --chip=$(CHIP) --codeoffset=0 --warn=0 --errors=1- --char=signed --double=24
CFLAGS=$(COMMON_FLAGS) --pass1
LDFLAGS=$(COMMON_FLAGS) --summary=mem,file --asmlist --output=+intel --output=+mcof --runtime=+init --runtime=+osccal --runtime=+clear --runtime=+clib --runtime=+keep -m

all: $(PROGNAME).hex

.PHONY: $(PROGNAME).hex clean flash vmware publish counted production

obj/%.p1: src/%.c src/*.h
	$(CC) $(CFLAGS) -c $(filter-out %.h,$^)

obj/$(PROGNAME).hex: $(OBJ_OUT)
	$(CC) $(LDFLAGS) -O$@ $^

clean:
	rm -f $(CLEANS)

README: .readme
	groff -Tascii $< >$@

clean_all: clean
	rm -f README

flash: $(PROGNAME).hex
	$(PK2) -B/usr/share/pk2/ -I -A5 -PPIC$(CHIP) -F$^ -M -YP

vmware: $(PROGNAME).hex
	mv $^ ~/Share/OpenBeacon/$(PROGNAME).hex

counted: $(PROGNAME).hex

$(PROGNAME).hex: obj/$(PROGNAME).hex
	php ./create_counted_firmware.php

production: clean $(PROG_TARGET).zip

$(PROG_TARGET).zip: obj/$(PROGNAME).hex
	zip $@ $^ obj/$(PROGNAME).sym create_counted_firmware.php set-id

publish: production
	scp $(PROG_TARGET).zip meri@bitmanufaktur.net:/home/wwwrun/open.bitmanufaktur.com/web/www/people/milosch/firmware/pic16/

indent:
	find src -iname '*.[ch]' -exec indent -c81 -i4 -cli4 -bli0 -ts 4 \{\} \;
