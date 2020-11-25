cmake_minimum_required(VERSION 3.14)
project(Onitama)

set(CMAKE_CXX_STANDARD 17)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions("-march=native")
else ()
    add_definitions("-march=native -Ofast")
endif ()

add_executable(
        Onitama
        src/main.cpp
        src/types.h
        src/board.h
        src/move_tables.cpp
        src/move_tables.h
        src/move_gen.cpp
        src/move_gen.h
        src/util.cpp
        src/util.h
        src/make_move.cpp
        src/make_move.h
        src/board.cpp
        src/move_bits.h
        src/search.cpp
        src/search.h
        src/perft.cpp
        src/perft.h
        lib/easywsclient/easywsclient.cpp
        src/client.cpp
        src/client.h
        src/tb/tb_gen.cpp
        src/tb/tb_gen.h
        src/tb/tb_probe.cpp
        src/tb/tb_probe.h
)

set(EXTERNAL_LIB_HEADERS lib/easywsclient lib/rapidjson/include)

target_include_directories(
        Onitama PRIVATE
        ${EXTERNAL_LIB_HEADERS}
)
