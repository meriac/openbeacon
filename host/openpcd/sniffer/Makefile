CXX     :=g++
TARGET  :=openpcd-sniffer
SOURCES :=src/filter.cpp

DUMP    :=iCLASS-002.img
MIRROR  :=http://mirror.openbeacon.net
LIBS    :=-lm

# determine program version
PROGRAM_VERSION:=$(shell git describe --tags --abbrev=4 --dirty 2>/dev/null | sed s/^v//)
ifeq ("$(PROGRAM_VERSION)","")
    PROGRAM_VERSION:='unknown'
endif

FILTERED:=$(DUMP:%.img=%-filtered.img)
LOGFILE :=$(DUMP:%.img=%-filtered.csv)
WAV     :=$(DUMP:%.img=%-filtered.wav)
WAV_ORIG:=$(DUMP:%.img=%.wav)

DEBUG   :=-O3
CXXOPT  :=$(DEBUG)
CXXFLAGS:=-Werror -Wall -D_REENTRANT -DPROGRAM_VERSION=\"$(PROGRAM_VERSION)\" -DPROGRAM_NAME=\"$(TARGET)\" $(ENCRYPTION_KEYS)
LDFLAGS :=$(LIBS)
OBJS    :=$(SOURCES:%.cpp=%.o)
PREFIX  :=/usr/local

all: $(TARGET)

.PHONY: all indent version install cleanall clean demo audacity

version:
	@echo "$(TARGET) version $(PROGRAM_VERSION)"

indent: $(SOURCES)
	find src -iname '*\.[cph]*' -exec indent -c81 -i4 -cli4 -bli0 -ts 4 \{\} \;
	rm -f src/*~

$(TARGET): .depend $(OBJS)
	$(CXX) $(LDFLAGS) $(LDOPT) $(OBJS) -o $@

.depend: $(SOURCES)
	$(CXX) $(CXXFLAGS) $(CXXOPT) -MM $^ > $@

.cpp.o:
	$(CXX) $(CXXFLAGS) $(CXXOPT) -c $< -o $@

$(WAV): $(TARGET) $(DUMP)
	./$(TARGET) $(DUMP) $(FILTERED) > $(LOGFILE)
	sox -2 -b 16 -s -c 2 -r 434027 -t raw $(FILTERED) $@

$(WAV_ORIG): $(DUMP)
	sox -2 -b 16 -s -c 1 -r 15625000 -t raw $^ $@

demo: $(WAV)

audacity: $(WAV) $(WAV_ORIG)
	audacity $(WAV)

audacity_orig: $(WAV_ORIG)
	audacity $(WAV_ORIG)

$(DUMP).bz2:
	curl -f -o $@ $(MIRROR)/$@

$(DUMP): $(DUMP).bz2
	bzip2 -cd $^ > $@

cleanall: clean
	rm -rf .depend $(WAV_ORIG) $(DUMP) $(DUMP).bz2

clean:
	rm -f $(TARGET) $(OBJS) $(FILTERED) $(LOGFILE) $(WAV) callgrind.out.* *~

include .depend
