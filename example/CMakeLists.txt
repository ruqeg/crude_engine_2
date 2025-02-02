cmake_minimum_required(VERSION 3.25)

project(crude_example C)

add_executable(crude_example WIN32)

target_link_libraries(crude_example PUBLIC crude_engine)

if(MSVC)
 target_compile_options(crude_example PRIVATE "/MP")
endif()

target_sources(crude_example
  PUBLIC
  main.c)

source_group(crude_example FILES main.c)
