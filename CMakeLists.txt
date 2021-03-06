cmake_minimum_required (VERSION 2.8)

project (proxy)

aux_source_directory(. SOURCE)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_BUILD_TYPE "Debug")
#set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb -DLOGGER_DEBUG_LEVEL")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall -DLOGGER_INFO_LEVEL")

INCLUDE_DIRECTORIES(/opt/third_party/glog/include
    /opt/local/libevent-2.1.5/include
    /opt/third_party/boost/include)

LINK_DIRECTORIES(/opt/third_party/glog/lib
    /opt/local/libevent-2.1.5/lib
    /opt/third_party/boost/lib)

LINK_LIBRARIES(glog event event_extra event_core event_pthreads
    boost_thread boost_system pthread)

add_executable(proxy ${SOURCE})
