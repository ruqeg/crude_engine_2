cmake_minimum_required(VERSION 3.25)

project(crude_editor C)

file(GLOB_RECURSE SRC *.c)
file(GLOB_RECURSE HEADERS *.h)

set(CRUDE_SOURCES ${SRC} ${HEADERS})
add_library(${PROJECT_NAME} SHARED ${CRUDE_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(crude_editor PUBLIC crude_engine cr)

source_group(editor FILES ${CRUDE_SOURCES})

if(MSVC)
 target_compile_options(crude_editor PRIVATE "/MP")
endif()
