ifeq (${DEBUG}, 1)
	CPPFLAGS = -DLOGGER_DEBUG_LEVEL
else
	CPPFLAGS = -DLOGGER_INFO_LEVEL
endif
INCLUDE = -I/opt/third_party/glog/include -I/opt/third_party/libevent-2.0/include
LIB = -L/opt/third_party/glog/lib \
	  -L/opt/third_party/libevent-2.0/lib \
	  -lglog -levent
CPPFLAGS += -g -ggdb -O0

all:test

%.o:%.cpp
	g++ -o $@ -c $< ${INCLUDE}  ${CPPFLAGS}

test:global.cpp http_msg.cpp http_header.cpp http_request.cpp main.cpp 
	g++ -o $@ $^ ${INCLUDE} ${LIB} ${CPPFLAGS}

clean:
	rm -f *.o test
