cmake_minimum_required(VERSION 3.14)
project(Onitama)

set(CMAKE_CXX_STANDARD 17)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions("-march=native")
else()
    add_definitions("-march=native -Ofast")
endif()

add_executable(Onitama src/main.cpp src/types.h src/board.h src/move_tables.cpp src/move_tables.h src/move_gen.cpp src/move_gen.h src/util.cpp src/util.h src/make_move.cpp src/make_move.h src/board.cpp src/search.cpp src/search.h src/perft.cpp)
