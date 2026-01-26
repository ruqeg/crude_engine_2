# v0.9.3 - bugfix release 

cmake_minimum_required(VERSION 3.25)

project(imgui-node-editor)

file(GLOB_RECURSE SRC *.c *.cpp *.cc)
file(GLOB_RECURSE HEADERS *.h *.inl)

set(SOURCES ${SRC} ${HEADERS})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE imgui)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/external/gl3w/Include ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../imgui/)
