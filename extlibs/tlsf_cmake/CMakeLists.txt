cmake_minimum_required(VERSION 3.25)

project(tlsf C)

set(CMAKE_C_STANDARD 11)

set(TLSF_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tlsf")
add_library(${PROJECT_NAME} ${TLSF_SOURCE_DIR}/tlsf.c)
target_include_directories(${PROJECT_NAME} PUBLIC ${TLSF_SOURCE_DIR})

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE "/MP")
endif()
