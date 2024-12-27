cmake_minimum_required(VERSION 3.6)
project(GateServer)


set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++17 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

set(SRCS
    src/gate_server.cc
    src/http_connection.cc
    src/logic_system.cc
    src/main.cc
    )

include_directories(. src)

set(LIBS 
    pthread
    jsoncpp    
)

add_executable(GateServer ${SRCS})
target_link_libraries(GateServer ${LIBS})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
