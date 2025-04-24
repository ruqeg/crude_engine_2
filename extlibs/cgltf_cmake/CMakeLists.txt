cmake_minimum_required(VERSION 3.25)

project(cgltf C)

set(CMAKE_C_STANDARD 11)

set(CGLTF_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cgltf")
add_library(${PROJECT_NAME} SHARED ${CGLTF_SOURCE_DIR}/cgltf.h cgltf_implementation.c)
target_include_directories(${PROJECT_NAME} PUBLIC ${CGLTF_SOURCE_DIR})

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE "/MP")
endif()
