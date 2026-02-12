cmake_minimum_required(VERSION 3.25)

project(crude CXX)

set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY ON)
set(CMAKE_SKIP_PACKAGE_ALL_DEPENDENCY ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CRUDE_SANITIZER_ANY_ENABLED OFF)
if(CRUDE_THREAD_SANITIZER)
add_compile_options(-fsanitize=thread)
add_link_options(-fsanitize=thread)
set(CRUDE_SANITIZER_ANY_ENABLED ON)
endif()
if(CRUDE_LEAK_SANITIZER)
add_compile_options(-fsanitize=leak)
add_link_options(-fsanitize=leak)
set(CRUDE_SANITIZER_ANY_ENABLED ON)
endif()
if(CRUDE_MEMORY_SANITIZER)
add_compile_options(-fsanitize=memory)
add_link_options(-fsanitize=memory)
set(CRUDE_SANITIZER_ANY_ENABLED ON)
endif()
if(CRUDE_UNDEFINED_SANITIZER)
add_compile_options(-fsanitize=undefined)
add_link_options(-fsanitize=undefined)
set(CRUDE_SANITIZER_ANY_ENABLED ON)
endif()
if(CRUDE_HDADDRESS_SANITIZER)
add_compile_options(-fsanitize=hwaddress)
add_link_options(-fsanitize=hwaddress)
set(CRUDE_SANITIZER_ANY_ENABLED ON)
endif()
if(CRUDE_ADDRESS_SANITIZER)
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)
set(CRUDE_SANITIZER_ANY_ENABLED ON)
endif()
  
add_compile_options("/MP")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_CONFIGURATION_TYPES "Debug;Develop;Release;Production;" CACHE STRING "" FORCE)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Develop" CACHE STRING "Default build type" FORCE)
endif()

#if(CRUDE_SANITIZER_ANY_ENABLED)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,Develop>:EditAndContinue>")
#endif

# Debug Configuration
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_DEBUG")
add_compile_definitions("$<$<CONFIG:Debug>:CRUDE_DEVELOP=1>")
add_compile_definitions("$<$<CONFIG:Debug>:CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED=1>")

# Develop Configuration
set(CMAKE_CXX_FLAGS_DEVELOP "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2")
add_compile_definitions("$<$<CONFIG:Develop>:CRUDE_DEVELOP=1>")
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

add_subdirectory(thirdparty)
add_subdirectory(engine)
add_subdirectory(game)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT crude_game)
