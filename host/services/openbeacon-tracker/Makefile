VERSION =0.1
CXX     =g++
TARGET  =openbeacon-tracker
SOURCES =src/bmMapHandleToItem.cpp src/main.cpp
LIBS    =

CXXFLAGS=-O3 -Werror -Wall -std=gnu++0x -ffunction-sections -fdata-sections -D_REENTRANT -DVERSION=\"$(VERSION)\"
LDFLAGS =-lm -lpthread -lpcap -Wl,--gc-sections
OBJS    =$(SOURCES:%.cpp=%.o)

all: $(TARGET)

run: $(TARGET)
	./$(TARGET)

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
