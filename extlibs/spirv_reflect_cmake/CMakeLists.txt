cmake_minimum_required(VERSION 3.25)

project(spirv_reflect C)

set(CMAKE_C_STANDARD 99)

set(SPIRV_REFLECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SPIRV-Reflect")
add_library(${PROJECT_NAME} SHARED ${SPIRV_REFLECT_SOURCE_DIR}/spirv_reflect.c)
target_include_directories(${PROJECT_NAME} PUBLIC ${SPIRV_REFLECT_SOURCE_DIR})

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE "/MP")
endif()
