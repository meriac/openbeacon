#
# Google Protocol Buffers declaration for OpenBeacon.org server/tag protocols
#

#
# for JAVA, Python and C++ support
#
# http://code.google.com/intl/de/apis/protocolbuffers/docs/overview.html

#
# for C support
#
# http://code.google.com/p/protobuf-c/

all: openbeacon.proto
	protoc $< --cpp_out=cpp --java_out=java --python_out=python
	protoc-c $< --c_out=c

clean:
	rm -f c/*.pb-c.c c/*.pb-c.h cpp/*.pb.cc cpp/*.pb.h python/*_pb2.py
	find java -name *.java -type f -exec rm -f \{\} \;