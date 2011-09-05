VERSION :=0.1
CXX     :=g++
TARGET  :=openbeacon-tracker
SOURCES :=src/bmMapHandleToItem.cpp src/main.cpp
FILTER  :=filter-singularsighting
LOGFILE :=logs/sputnik-24c3-log.bin

FILTERFILE_PREFIX:=logs/sightings.json

MIRROR  :=http://mirror.openbeacon.net
LIBS    :=-lm -lpthread -lpcap

CXXOPT  :=-O3 -ffunction-sections -fdata-sections
CXXFLAGS:=-Werror -Wall -std=gnu++0x -D_REENTRANT -DVERSION=\"$(VERSION)\"
LDOPT   :=-Wl,--gc-sections
LDFLAGS :=$(LIBS)
OBJS    :=$(SOURCES:%.cpp=%.o)
LOGJSON :=$(LOGFILE:%.bin=%.json.bz2)
PREFIX  :=/usr/local

all: $(TARGET) filters

run: $(TARGET)
	./$(TARGET)

install: $(TARGET) $(FILTER)
	install $(TARGET) $(FILTER) $(PREFIX)/bin/

demo-realtime: $(TARGET) $(LOGFILE) $(FILTER)
	rm -f $(FILTERFILE_PREFIX)*
	./$(TARGET) $(LOGFILE) 1 | ./$(FILTER) $(FILTERFILE_PREFIX) | bzip2 -9 >$(LOGJSON)

demo-logfile: $(TARGET) $(LOGFILE)
	rm -f $(FILTERFILE_PREFIX)*
	./$(TARGET) $(LOGFILE) 0 | bzip2 -9 >$(LOGJSON)

demo-fastest: $(TARGET) $(LOGFILE)
	rm -f $(FILTERFILE_PREFIX)*
	time ./$(TARGET) $(LOGFILE) 0 > /dev/null

logfile: $(LOGFILE)

$(LOGFILE): $(LOGFILE).bz2
	bzip2 -cd $^ > $@

$(LOGFILE).bz2:
	curl -C - -o $@ $(MIRROR)/$(shell basename $@)

filters: $(FILTER)

$(FILTER): src/$(FILTER).cpp
	$(CXX) $(CXXFLAGS) $(CXXOPT) $(LDOPT) -lz $^ -o $@

indent: $(SOURCES)
	indent $+
	rm -f *~

$(TARGET): .depend $(OBJS)
	$(CXX) $(LDFLAGS) $(LDOPT) $(OBJS) -o $@

debug: .depend $(SOURCES)
	$(CXX) $(CXXFLAGS) -DDEBUG -g $(SOURCES) $(LDFLAGS) -o $(TARGET)

.depend: $(SOURCES) $(FILTERS)
	$(CXX) $(CXXFLAGS) $(CXXOPT) -MM $^ > $@

.cpp.o:
	$(CXX) $(CXXFLAGS) $(CXXOPT) -c $< -o $@

cleanall: clean
	rm -rf .depend

clean:
	rm -f $(TARGET) $(OBJS) $(FILTER) $(LOGJSON) $(FILTERFILE_PREFIX)* callgrind.out.* *~

include .depend
