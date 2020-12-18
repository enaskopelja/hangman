cmake_minimum_required(VERSION 3.17)
project(hangman)

set(CMAKE_C_STANDARD 99)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
include_directories("./headers")

set(server-header header/protocol.h header/pqueue.h)
set(server-src src/server.c src/protocol.c src/pqueue.c)
add_executable(server ${server-header} ${server-src})
target_link_libraries(server PRIVATE Threads::Threads)

set(client-header header/protocol.h)
set(client-src src/client.c src/protocol.c)
add_executable(client ${client-header} ${client-src})
