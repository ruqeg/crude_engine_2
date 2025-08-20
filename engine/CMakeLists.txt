cmake_minimum_required(VERSION 3.25)

project(crude_engine)

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SRC *.cc)
file(GLOB_RECURSE HEADERS *.h *.inl)

set(CRUDE_SOURCES ${SRC} ${HEADERS})

add_library(${PROJECT_NAME} SHARED ${CRUDE_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC stb flecs tlsf SDL3-shared ${Vulkan_LIBRARY} vma cgltf spirv_reflect miniaudio enkiTS TracyClient cjson_cmake imgui_cmake imguizmo_cmake meshoptimizer imgui_node_editor_cmake)

source_group(core REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/core/*")
source_group(platform REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/platform/*")
source_group(graphics REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/graphics/*")
source_group(graphics/passes REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/graphics/passes/*")
source_group(scene REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/scene/*")
source_group(physics REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/physics/*")
source_group(sound REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/sound/*")
source_group(gui REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/gui/*")
source_group(develop REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/develop/*")
source_group(engine FILES engine.cc engine.h)

add_definitions(
  -DCRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED
)

if(MSVC)
 target_compile_options(${PROJECT_NAME} PRIVATE "/MP")
endif()
