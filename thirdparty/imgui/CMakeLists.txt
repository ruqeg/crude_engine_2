# imgui v1.92.5

cmake_minimum_required(VERSION 3.25)

project(imgui)

file(GLOB_RECURSE SRC *.c *.cpp *.cc)
file(GLOB_RECURSE HEADERS *.h *.inl)

set(SOURCES ${SRC} ${HEADERS})

add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIR} ${Vulkan_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3-static ${Vulkan_LIBRARY})
