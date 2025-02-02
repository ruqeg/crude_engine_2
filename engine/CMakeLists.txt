cmake_minimum_required(VERSION 3.25)

project(crude_engine C)

add_library(crude_engine)

target_sources(crude_engine PRIVATE engine.c)

if(MSVC)
 target_compile_options(crude_engine PRIVATE "/MP")
endif()

source_group(crude_engine FILES engine.c)

install(TARGETS crude_engine
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
