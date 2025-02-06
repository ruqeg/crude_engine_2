cmake_minimum_required(VERSION 3.25)

project(crude_launcher CXX)
set(CMAKE_CXX_STANDARD 23)

add_executable(crude_launcher WIN32)

target_link_libraries(crude_launcher PUBLIC crude_engine_simulation crude_editor)

if(MSVC)
 target_compile_options(crude_launcher PRIVATE "/MP")
endif()

target_sources(crude_launcher
  PUBLIC
  launcher.cc
)

source_group(crude_launcher FILES main.c)
