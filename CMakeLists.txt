cmake_minimum_required(VERSION 3.16)
project(chameleon)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE  "Debug")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -g -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3")

include_directories(asio/include)
include_directories(json)
include_directories(util)
include_directories(net)
include_directories(src)


add_executable(chameleon
        main.cpp
        net/connection.cpp
        net/connection.h
        net/defines.h
        util/threadpool.cpp
        util/threadpool.h
        util/queue.h
        net/repeattimer.h
        net/server.h
        net/server.cpp
        net/client.h
        net/client.cpp
        net/iocpool.h
        net/iocpool.cpp
        net/msgbuffer.h
        net/msgbuffer.cpp
        net/objectpool.h
        src/protocol/proto.h
        src/protocol/proto.cpp
        util/nanolog.hpp
        net/instance.h
        src/application.h
        util/singleton.h
        src/application.cpp
        net/constant.h)


target_link_libraries(chameleon pthread)
