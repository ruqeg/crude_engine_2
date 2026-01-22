cmake_minimum_required(VERSION 3.25)

project(crude_engine)

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SRC *.cc *.crude_shader *.crude_techniques)
file(GLOB_RECURSE HEADERS *.h *.inl)

set(CRUDE_SOURCES ${SRC} ${HEADERS})

add_library(${PROJECT_NAME} SHARED ${CRUDE_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}\\..\\)
target_link_libraries(${PROJECT_NAME} PUBLIC imguizmo flecs SDL3-static stb tlsf ${Vulkan_LIBRARY} vma cgltf spirv_reflect miniaudio enkiTS TracyClient cjson meshoptimizer nfd)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${CRUDE_SOURCES})
