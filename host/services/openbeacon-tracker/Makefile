VERSION =0.1
CXX     =g++
TARGET  =openbeacon-tracker
SOURCES =src/bmMapHandleToItem.cpp src/main.cpp
LOGFILE =sputnik-24c3-log.bin
MIRROR  =http://mirror.openbeacon.net
LIBS    =

CXXFLAGS=-O3 -Werror -Wall -std=gnu++0x -ffunction-sections -fdata-sections -D_REENTRANT -DVERSION=\"$(VERSION)\"
LDFLAGS =-lm -lpthread -lpcap -Wl,--gc-sections
OBJS    =$(SOURCES:%.cpp=%.o)

all: $(TARGET)

run: $(TARGET)
	./$(TARGET)

replay: $(TARGET) logs/$(LOGFILE)
	./$(TARGET) logs/$(LOGFILE) 1

logs/$(LOGFILE): logs/$(LOGFILE).bz2
	bzip2 -cd $^ > $@

logs/$(LOGFILE).bz2:
	curl -C - -o $@ $(MIRROR)/$(LOGFILE).bz2

indent: $(SOURCES)
	indent $+
	rm -f *~

$(TARGET): .depend $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $@

.depend: $(SOURCES)
	$(CXX) $(CXXFLAGS) -MM $^ > $@

cleanall: clean
	rm -rf .depend

clean:
	rm -f $(TARGET) $(OBJS) *~

include .depend
