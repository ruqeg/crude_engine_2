cmake_minimum_required(VERSION 3.25)

project(crude_engine C)

add_library(crude_engine)

target_link_libraries(crude_engine PUBLIC tlsf)
target_sources(crude_engine PRIVATE engine.c)
target_include_directories(crude_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
 target_compile_options(crude_engine PRIVATE "/MP")
endif()

add_subdirectory(core)

source_group(core REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/core/*")
source_group(engine FILES ${CRUDE_ENGINE_SOURCES})

install(TARGETS crude_engine
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
