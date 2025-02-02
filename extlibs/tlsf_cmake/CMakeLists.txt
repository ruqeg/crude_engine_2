cmake_minimum_required(VERSION 3.25)

project(tlsf C)

set(CMAKE_C_STANDARD 17)

add_library(tlsf)

if(MSVC)
 target_compile_options(tlsf PRIVATE "/MP")
endif()

set(TLSF_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tlsf")

target_include_directories(tlsf PUBLIC ${TLSF_SOURCE_DIR})

target_sources(tlsf PRIVATE
  ${TLSF_SOURCE_DIR}/tlsf.c)

install(TARGETS tlsf
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
