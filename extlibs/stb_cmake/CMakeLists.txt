cmake_minimum_required(VERSION 3.25)

project(stb C)

set(CMAKE_C_STANDARD 17)

add_library(stb)

if(MSVC)
 target_compile_options(stb PRIVATE "/MP")
endif()

set(STB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/stb")

target_include_directories(stb PUBLIC ${STB_SOURCE_DIR})

target_sources(stb PRIVATE
  ${STB_SOURCE_DIR}/stb_vorbis.c)

install(TARGETS stb
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
