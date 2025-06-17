cmake_minimum_required(VERSION 3.25)

project(crude_shaders)

add_library(${PROJECT_NAME} INTERFACE)

find_package(Vulkan REQUIRED)

set(CRUDE_SHADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB_RECURSE CRUDE_SHADER_SOURCES *.frag *.vert *.mesh *.task)

include(${CMAKE_CURRENT_SOURCE_DIR}/compile_spirv_shader.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/compile_shader.cmake)

foreach(SHADER_SOURCE ${CRUDE_SHADER_SOURCES})
  COMPILE_SHADER(${SHADER_SOURCE})
endforeach()

target_include_directories(${PROJECT_NAME} INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

source_group(shaders FILES ${CRUDE_SHADER_SOURCES})
