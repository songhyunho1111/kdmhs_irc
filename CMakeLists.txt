cmake_minimum_required(VERSION 4.1)
project(kdmhs_irc C)

set(CMAKE_C_STANDARD 17)

add_executable(kdmhs_irc main.c
        irc_header.h
)
target_link_libraries(kdmhs_irc ws2_32)