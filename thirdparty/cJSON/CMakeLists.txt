# cjson 1.7.19

cmake_minimum_required(VERSION 3.25)

project(cjson)

file(GLOB_RECURSE SRC *.c *.cpp *.cc)
file(GLOB_RECURSE HEADERS *.h *.inl)

set(SOURCES ${SRC} ${HEADERS})

add_library(${PROJECT_NAME} STATIC ${SOURCES})
