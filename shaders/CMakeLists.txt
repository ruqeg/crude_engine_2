cmake_minimum_required(VERSION 3.25)

project(crude_shaders)

file(GLOB_RECURSE SRC *.frag *.vert)

set(CRUDE_SOURCES ${SRC} ${HEADERS})

add_library(${PROJECT_NAME} INTERFACE ${CRUDE_SOURCES})

target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

source_group(shaders FILES ${CRUDE_SOURCES})
