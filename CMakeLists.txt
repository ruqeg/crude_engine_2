cmake_minimum_required(VERSION 3.25)

project(crude CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(CRUDE_FORCE_STATIC "Force static linking for all libraries" OFF)

set(CMAKE_CONFIGURATION_TYPES "Debug;Develop;Release;Production;" CACHE STRING "" FORCE)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Develop" CACHE STRING "Default build type" FORCE)
endif()

# Debug Configuration
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_DEBUG")
add_compile_definitions("$<$<CONFIG:Debug>:CRUDE_DEVELOP=1>")
add_compile_definitions("$<$<CONFIG:Debug>:CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED=1>")

# Develop Configuration
set(CMAKE_CXX_FLAGS_DEVELOP "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2")
add_compile_definitions("$<$<CONFIG:Develop>:CRUDE_DEVELOP=1>")
add_compile_definitions("$<$<CONFIG:Develop>:CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED=1>")
add_compile_definitions("$<$<CONFIG:Develop>:CRUDE_GRAPHICS_OPTIMAIZE_SHADERS=1>")
add_compile_definitions("$<$<CONFIG:Develop>:IMGUI_DISABLE_DEBUG_TOOLS=1>")
add_compile_definitions("$<$<CONFIG:Develop>:IMGUI_DISABLE_DEMO_WINDOWS=1>")

# Release Configuration  
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2")
add_compile_definitions("$<$<CONFIG:Release>:CRUDE_DEVELOP=1>")
add_compile_definitions("$<$<CONFIG:Release>:CRUDE_PRODUCTION=1>")
add_compile_definitions("$<$<CONFIG:Release>:CRUDE_GRAPHICS_OPTIMAIZE_SHADERS=1>")
add_compile_definitions("$<$<CONFIG:Release>:IMGUI_DISABLE_DEBUG_TOOLS=1>")
add_compile_definitions("$<$<CONFIG:Release>:IMGUI_DISABLE_DEMO_WINDOWS=1>")

# Production Configuration  
set(CMAKE_CXX_FLAGS_PRODUCTION "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2")
add_compile_definitions("$<$<CONFIG:Production>:CRUDE_DEVELOP=0>")
add_compile_definitions("$<$<CONFIG:Production>:CRUDE_PRODUCTION=1>")
add_compile_definitions("$<$<CONFIG:Production>:CRUDE_GRAPHICS_OPTIMAIZE_SHADERS=1>")
add_compile_definitions("$<$<CONFIG:Production>:IMGUI_DISABLE_DEBUG_TOOLS=1>")
add_compile_definitions("$<$<CONFIG:Production>:IMGUI_DISABLE_DEMO_WINDOWS=1>")

# Common Configuration
add_compile_definitions(
  CRUDE_GRAPHICS_MESH_SHADER_DISBLED=0
  CRUDE_GRAPHICS_FRAGMENT_SHADING_RATE_DISBLED=0
  CRUDE_GRAPHICS_DEFERRED_HOST_OPERATIONS_DISBLED=0
  CRUDE_GRAPHICS_SHADER_RELAXED_EXTENDED_INSTRUCTION_DISBLED=0
  CRUDE_GRAPHICS_SHADOWS_ENABLED=0
  CRUDE_GRAPHICS_RAY_TRACING_ENABLED=0
  CRUDE_GPU_PROFILER=CRUDE_DEVELOP
  )

set(CRUDE_GAME_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/game)

add_subdirectory(extlibs)
add_subdirectory(engine)
add_subdirectory(game)
add_subdirectory(editor)

#set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT crude_host)
