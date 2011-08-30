VERSION :=0.1
CXX     :=g++
TARGET  :=openbeacon-tracker
SOURCES :=src/bmMapHandleToItem.cpp src/main.cpp
FILTER  :=filter-singularsighting
LOGFILE :=logs/sputnik-24c3-log.bin

FILTERFILE_PREFIX:=logs/sightings.json

MIRROR  :=http://mirror.openbeacon.net
LIBS    :=-lm -lpthread -lpcap

CXXFLAGS:=-O3 -Werror -Wall -std=gnu++0x -ffunction-sections -fdata-sections -D_REENTRANT -DVERSION=\"$(VERSION)\"
LDFLAGS :=$(LIBS) -Wl,--gc-sections
OBJS    :=$(SOURCES:%.cpp=%.o)
LOGJSON :=$(LOGFILE:%.bin=%.json.bz2)
PREFIX  :=/usr/local

all: $(TARGET) filters

run: $(TARGET)
	./$(TARGET)

install: $(TARGET) $(FILTER)
	install $(TARGET) $(FILTER) $(PREFIX)/bin/

demo_realtime: $(TARGET) $(LOGFILE) $(FILTER)
	./$(TARGET) $(LOGFILE) 1 | ./$(FILTER) $(FILTERFILE_PREFIX) | bzip2 -9 >$(LOGJSON)

demo_fastest: $(TARGET) $(LOGFILE)
	rm -f $(FILTERFILE_PREFIX)*
	./$(TARGET) $(LOGFILE) 0 | bzip2 -9 >$(LOGJSON)

logfile: $(LOGFILE)

$(LOGFILE): $(LOGFILE).bz2
	bzip2 -cd $^ > $@

$(LOGFILE).bz2:
	curl -C - -o $@ $(MIRROR)/$(shell basename $@)

filters: $(FILTER)

$(FILTER): src/$(FILTER).cpp
	$(CXX) -lz $^ -o $@

indent: $(SOURCES)
	indent $+
	rm -f *~

$(TARGET): .depend $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $@

.depend: $(SOURCES) $(FILTERS)
	$(CXX) $(CXXFLAGS) -MM $^ > $@

cleanall: clean
	rm -rf .depend

clean:
	rm -f $(TARGET) $(OBJS) $(FILTER) $(LOGJSON) $(FILTERFILE_PREFIX)* *~

include .depend
