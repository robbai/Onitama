cmake_minimum_required(VERSION 3.14)
project(Onitama)

set(CMAKE_CXX_STANDARD 17)

if (WIN32)
    link_libraries(ws2_32)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions("-march=native")
else ()
    add_definitions("-march=native -Ofast")
endif ()

# Produce version.cpp.
ADD_CUSTOM_COMMAND(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/_version.cpp
        COMMAND ${CMAKE_COMMAND} -P
        ${CMAKE_CURRENT_SOURCE_DIR}/version.cmake
)

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
        src/tb/tb_gen.cpp
        src/tb/tb_gen.h
        src/tb/tb_probe.cpp
        src/tb/tb_probe.h
        ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
        src/version.h
        src/tt/ttable.cpp
        src/tt/ttable.h
        src/tt/zobrist.cpp
        src/tt/zobrist.h
        src/evaluate.cpp
        src/evaluate.h
        src/nnue/layer.h
        src/nnue/nnue.cpp
        src/nnue/nnue.h
        src/nnue/network.cpp
        src/nnue/network.h
        src/tune.cpp
        src/tune.h
)

set(EXTERNAL_LIB_HEADERS lib/easywsclient lib/rapidjson/include)

target_include_directories(
        Onitama PRIVATE
        ${EXTERNAL_LIB_HEADERS}
)
