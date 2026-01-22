# flecs-4.1.4
cmake_minimum_required(VERSION 3.25)

project(flecs)

file(GLOB_RECURSE SRC *.c *.cpp *.cc)
file(GLOB_RECURSE HEADERS *.h *.inl)

set(SOURCES ${SRC} ${HEADERS})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
add_library(${PROJECT_NAME} INTERFACE ${SOURCES})
