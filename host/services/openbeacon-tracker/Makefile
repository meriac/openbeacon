CXX     :=g++
TARGET  :=openbeacon-tracker
SOURCES :=src/bmMapHandleToItem.cpp src/main.cpp
FILTERSS:=filter-singularsighting
FILTERMG:=filter-mongodb
LOGFILE :=logs/brucon-2011-log.bin
MAKE    :=make

FILTERFILE_PREFIX:=logs/sightings.json

MIRROR  :=http://mirror.openbeacon.net
LIBS    :=-lm -pthread -lpcap
MONGOC  :=mongodb-mongo-c-driver

# determine program version
PROGRAM_VERSION:=$(shell git describe --tags --abbrev=4 --dirty 2>/dev/null | sed s/^v//)
ifeq ("$(PROGRAM_VERSION)","")
    PROGRAM_VERSION:='unknown'
endif

# determine if we have custom encryption keys
ifeq ("$(wildcard src/custom-encryption-keys.h)","")
    ENCRYPTION_KEYS:=
else
    ENCRYPTION_KEYS:=-DCUSTOM_ENCRYPTION_KEYS
endif

DEBUG   :=-O3
CXXOPT  :=$(DEBUG)
CXXFLAGS:=-Werror -Wall -D_REENTRANT -DPROGRAM_VERSION=\"$(PROGRAM_VERSION)\" -DPROGRAM_NAME=\"$(TARGET)\" $(ENCRYPTION_KEYS)
LDFLAGS :=$(LIBS)
OBJS    :=$(SOURCES:%.cpp=%.o)
LOGJSON :=$(LOGFILE:%.bin=%.json.bz2)
PREFIX  :=/usr/local

all: $(TARGET) $(FILTERSS)

.PHONY: all version run install demo-realtime demo-logfile demo-fastest logfile filters indent debug dependencies-fedora cleanall clean

version:
	@echo "$(TARGET) version $(PROGRAM_VERSION)"

run: $(TARGET)
	./$(TARGET)

install: $(TARGET) $(FILTER)
	install $(TARGET) $(FILTER) $(PREFIX)/bin/

demo-realtime: $(TARGET) $(LOGFILE) $(FILTERSS)
	rm -f $(FILTERFILE_PREFIX)*
	./$(TARGET) $(LOGFILE) 1 | ./$(FILTERSS) $(FILTERFILE_PREFIX) | bzip2 -9 >$(LOGJSON)

demo-logfile: $(TARGET) $(LOGFILE)
	rm -f $(FILTERFILE_PREFIX)*
	./$(TARGET) $(LOGFILE) 0 | bzip2 -9 >$(LOGJSON)

demo-fastest: $(TARGET) $(LOGFILE)
	rm -f $(FILTERFILE_PREFIX)*
	time ./$(TARGET) $(LOGFILE) 0 > /dev/null

demo-mongodb: $(TARGET) $(LOGFILE) $(FILTERMG)
	time ./$(TARGET) $(LOGFILE) 0 | ./$(FILTERMG) >/dev/null

logfile: $(LOGFILE)

$(LOGFILE): $(LOGFILE).bz2
	bzip2 -cd $^ > $@

$(LOGFILE).bz2:
	curl -C - -o $@ $(MIRROR)/$(shell basename $@)

$(FILTERSS): src/$(FILTERSS).cpp
	$(CXX) $(CXXFLAGS) $(CXXOPT) $(LDOPT) $^ -lz -o $@

$(MONGOC).tar.gz:
	curl -L -o $@ http://github.com/mongodb/mongo-c-driver/tarball/master

$(MONGOC): $(MONGOC).tar.gz
	rm -rf $(MONGOC)-*
	tar -xvzf $^
	mv $(MONGOC)-* $(MONGOC)
	$(MAKE) -C $(MONGOC)

$(FILTERMG): src/$(FILTERMG).cpp $(MONGOC)
	$(CXX) $(CXXFLAGS) $(CXXOPT) $(LDOPT) -DMONGO_HAVE_STDINT -I$(MONGOC)/src src/$(FILTERMG).cpp $(MONGOC)/libmongoc.a $(MONGOC)/libbson.a -ljson -o $@ 

indent: $(SOURCES)
	find src -iname '*\.[cph]*' -exec indent -c81 -i4 -cli4 -bli0 -ts 4 \{\} \;
	rm -f src/*~

$(TARGET): .depend $(OBJS)
	$(CXX) $(LDOPT) $(OBJS) $(LDFLAGS) -o $@

.depend: $(SOURCES) $(FILTERS)
	$(CXX) $(CXXFLAGS) $(CXXOPT) -MM $^ > $@

.cpp.o:
	$(CXX) $(CXXFLAGS) $(CXXOPT) -c $< -o $@

dependencies-fedora:
	sudo yum install libpcap libpcap-devel zlib zlib-devel gcc-c++ mongodb-devel mongodb boost-build boost-devel json-c json-c-devel

cleanall: clean
	rm -f .depend $(MONGOC).tar.gz
	rm -rf $(MONGOC)

clean:
	rm -f $(TARGET) $(OBJS) $(FILTERSS) $(FILTERMG) $(LOGJSON) $(FILTERFILE_PREFIX)* callgrind.out.* *~

include .depend
