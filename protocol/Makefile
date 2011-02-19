#
# Google Protocol Buffers declaration for OpenBeacon.org server/tag protocols
#

#
# for JAVA, Python and C++ support
#
# http://code.google.com/intl/de/apis/protocolbuffers/docs/overview.html

all: openbeacon.proto
	protoc $< --cpp_out=cpp --java_out=java --python_out=python

clean:
	rm -f cpp/*.pb.cc cpp/*.pb.h python/*_pb2.py
	find java -name *.java -type f -exec rm -f \{\} \;