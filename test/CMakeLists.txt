cmake_minimum_required (VERSION 2.8)

project (proxy_test)

aux_source_directory(. SOURCE)

set(CMAKE_BUILD_TYPE "Debug")
#set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb3 -DLOGGER_DEBUG_LEVEL")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall -DLOGGER_INFO_LEVEL")

INCLUDE_DIRECTORIES(${PROJECT_SOURCE_DIR}/../)

LINK_DIRECTORIES(${PROJECT_SOURCE_DIR}/../lib)

LINK_LIBRARIES(hash)

add_executable(proxy_test ${SOURCE} ${PROJECT_SOURCE_DIR}/../hash_table.cpp)
