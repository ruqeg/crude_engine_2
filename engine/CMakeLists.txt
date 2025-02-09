cmake_minimum_required(VERSION 3.25)

project(crude_engine C)

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SRC *.c)
file(GLOB_RECURSE HEADERS *.h)

set(CRUDE_SOURCES ${SRC} ${HEADERS})

add_library(${PROJECT_NAME} SHARED ${CRUDE_SOURCES})

target_include_directories(crude_engine PUBLIC ${Vulkan_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(crude_engine PUBLIC stb flecs tlsf SDL3::SDL3-shared ${Vulkan_LIBRARY})

source_group(core REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/core/*")
source_group(platform REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/platform/*")
source_group(gui REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/gui/*")
source_group(graphics REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/graphics/*")
source_group(engine FILES engine.c engine.h)

if(MSVC)
 target_compile_options(crude_engine PRIVATE "/MP")
endif()
