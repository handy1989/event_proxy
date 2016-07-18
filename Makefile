ifeq (${DEBUG}, 1)
	CPPFLAGS = -DLOGGER_DEBUG_LEVEL
else
	CPPFLAGS = -DLOGGER_INFO_LEVEL
endif
INCLUDE = -I/opt/third_party/glog/include \
	-I/opt/local/libevent-2.1.5/include \
	-I/opt/third_party/boost/include

LIB = -L/opt/third_party/glog/lib \
	  -L/opt/local/libevent-2.1.5/lib \
	  -L/opt/third_party/boost/lib \
	  -lglog -levent -levent_extra -levent_core -levent_pthreads -lboost_thread -lboost_system
CPPFLAGS += -g -ggdb -O0

all:proxy

%.o:%.cpp
	g++ -o $@ -c $< ${INCLUDE}  ${CPPFLAGS}

SOURCE = ${wildcard *.cpp}
OBJECT = ${patsubst %.cpp, %.o, $(SOURCE)}

proxy: $(OBJECT)
	g++ -o $@ $^ ${INCLUDE} ${LIB} ${CPPFLAGS}

clean:
	rm -f *.o proxy
