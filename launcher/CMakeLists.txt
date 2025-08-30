cmake_minimum_required(VERSION 3.25)

project(launcher CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC game crude_engine_simulation)

if(MSVC)
 target_compile_options(${PROJECT_NAME} PRIVATE "/MP")
endif()

source_group(launcher FILES main.cc)

target_sources(${PROJECT_NAME} PUBLIC main.cc)
