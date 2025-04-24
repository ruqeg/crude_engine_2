cmake_minimum_required(VERSION 3.25)

project(stb C)

set(CMAKE_C_STANDARD 11)

set(STB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/stb")
add_library(${PROJECT_NAME} SHARED ${STB_SOURCE_DIR}/stb_vorbis.c stb_implementation.c)
target_include_directories(${PROJECT_NAME} PUBLIC ${STB_SOURCE_DIR})

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE "/MP")
endif()
