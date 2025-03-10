cmake_minimum_required(VERSION 3.25)

project(crude_launcher CXX)
set(CMAKE_CXX_STANDARD 23)

add_executable(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PUBLIC crude_engine_simulation crude_editor crude_sandbox crude_paprika)

if(MSVC)
 target_compile_options(${PROJECT_NAME} PRIVATE "/MP")
endif()

source_group(launcher FILES launcher.cc)

target_sources(${PROJECT_NAME} PUBLIC launcher.cc)
