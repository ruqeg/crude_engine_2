# f1c79c02822848a9bed4315b12c8c8f3761e1296

cmake_minimum_required(VERSION 3.25)

project(tlsf)

file(GLOB_RECURSE SRC *.c *.cpp *.cc)
file(GLOB_RECURSE HEADERS *.h *.inl)

set(SOURCES ${SRC} ${HEADERS})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
add_library(${PROJECT_NAME} STATIC ${SOURCES})
