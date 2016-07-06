INCLUDE = -I/opt/third_party/glog/include -I/opt/third_party/libevent-2.0/include
LIB = -L/opt/third_party/glog/lib \
	  -L/opt/third_party/libevent-2.0/lib \
	  -lglog -levent
CPPFLAGS = -g -ggdb -O0

all:test

main.o:main.cpp
	g++ -o $@ -c $< ${INCLUDE} ${LIB} ${CPPFLAGS}

test:main.o
	g++ -o $@ $< ${INCLUDE} ${LIB} ${CPPFLAGS}
