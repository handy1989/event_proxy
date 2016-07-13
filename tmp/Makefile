ifeq (${DEBUG}, 1)
	CPPFLAGS = -DLOGGER_DEBUG_LEVEL
else
	CPPFLAGS = -DLOGGER_INFO_LEVEL
endif
INCLUDE = -I/opt/third_party/glog/include -I/opt/third_party/libevent-2.1.5/include -I..
LIB = -L/opt/third_party/glog/lib \
	  -L/opt/third_party/libevent-2.1.5/lib \
	  -lglog -levent -levent_extra -levent_core
CPPFLAGS += -g -ggdb -O0

all:test test2

%.o:%.cpp
	g++ -o $@ -c $< ${INCLUDE} ${CPPFLAGS}

SOURCE = ${wildcard *.cpp}
OBJECT = ${patsubst %.cpp, %.o, $(SOURCE)}

test: test.o
	g++ -o $@ $^ ${INCLUDE} ${LIB} ${CPPFLAGS}

test2: test2.o
	g++ -o $@ $^ ${INCLUDE} ${LIB} ${CPPFLAGS}

clean:
	rm -f *.o test test2
