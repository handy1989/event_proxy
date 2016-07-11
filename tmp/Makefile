ifeq (${DEBUG}, 1)
	CPPFLAGS = -DLOGGER_DEBUG_LEVEL
else
	CPPFLAGS = -DLOGGER_INFO_LEVEL
endif
INCLUDE = -I/opt/third_party/glog/include -I/home/zhangmenghan/libevent/include -I..
LIB = -L/opt/third_party/glog/lib \
	  -L/home/zhangmenghan/libevent/lib \
	  -lglog -levent -levent_extra -levent_core
CPPFLAGS += -g -ggdb -O0

all:test

%.o:%.cpp
	g++ -o $@ -c $< ${INCLUDE} ${CPPFLAGS}

SOURCE = ${wildcard *.cpp}
OBJECT = ${patsubst %.cpp, %.o, $(SOURCE)}

test: $(OBJECT)
	g++ -o $@ $^ ${INCLUDE} ${LIB} ${CPPFLAGS}

clean:
	rm -f *.o test
