COMPILER_PATH=/usr/hitech/picc/9.83/bin/
PK2=/usr/local/bin/pk2cmd
CC=$(COMPILER_PATH)picc

MODE=std
CHIP=16F684
PROGNAME=openbeacon-morse

MAIN=main.c
SRCS=$(MAIN) timer.c

PROG_EXTS=cof hxl map mum sym as sdb lst rlf obj
PROG_FILES=$(patsubst %, obj/$(PROGNAME).%, $(PROG_EXTS))
PROG_TARGET=$(PROGNAME)-PIC$(CHIP)-firmware.hex

OBJ_EXTS=p1 rlf sdb lst pre as
OBJ_TMP=$(foreach T, $(OBJ_EXTS), $(patsubst %.c, obj/%.$(T), $(SRCS)))
OBJ_OUT=$(patsubst %.c, obj/%.p1, $(SRCS))

CLEANS=$(OBJ_OUT) $(OBJ_TMP) $(PROG_FILES) $(PROG_TARGET)
CLEANS+=obj/startup.as obj/startup.lst obj/startup.rlf obj/$(PROGNAME).hex $(PROGNAME).hex obj/startup.obj funclist src/*.[ch]~

COMMON_FLAGS=-q --MODE=$(MODE) --outdir=obj --chip=$(CHIP) --codeoffset=0 --warn=0 --errors=1- --char=signed --double=24
CFLAGS=$(COMMON_FLAGS) --pass1
LDFLAGS=$(COMMON_FLAGS) --summary=mem,file --asmlist --output=+intel --output=+mcof --runtime=+init --runtime=+osccal --runtime=+clear --runtime=+clib --runtime=+keep -m

all: $(PROGNAME).hex

.PHONY: $(PROGNAME).hex clean flash publish $(PROGNAME).hex

obj/%.p1: src/%.c src/*.h
	$(CC) $(CFLAGS) -c $(filter-out %.h,$^)

obj/$(PROGNAME).hex: $(OBJ_OUT)
	$(CC) $(LDFLAGS) -O$@ $^

clean:
	rm -f $(CLEANS)

flash: $(PROGNAME).hex
	$(PK2) -B/usr/share/pk2/ -I -A5 -PPIC$(CHIP) -F$^ -M -YP

publish: $(PROGNAME).hex
	mv $^ ~/Share/OpenBeacon/$(PROG_TARGET)

vmware: $(PROGNAME).hex
	mv $^ ~/Share/OpenBeacon/$(PROGNAME).hex

$(PROGNAME).hex: obj/$(PROGNAME).hex
	cp $^ $@

indent:
	find src -iname '*.[ch]' -exec indent -c81 -i4 -cli4 -bli0 -ts 4 \{\} \;

