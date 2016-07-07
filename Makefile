ifeq (${DEBUG}, 1)
	CPPFLAGS = -DLOGGER_DEBUG_LEVEL
else
	CPPFLAGS = -DLOGGER_INFO_LEVEL
endif
INCLUDE = -I/opt/third_party/glog/include -I/opt/third_party/libevent/include
LIB = -L/opt/third_party/glog/lib \
	  -L/opt/third_party/libevent/lib \
	  -lglog -levent
CPPFLAGS += -g -ggdb -O0

all:test

%.o:%.cpp
	g++ -o $@ -c $< ${INCLUDE}  ${CPPFLAGS}

SOURCE = ${wildcard *.cpp}

test: $(SOURCE)
	g++ -o $@ $^ ${INCLUDE} ${LIB} ${CPPFLAGS}

clean:
	rm -f *.o test
