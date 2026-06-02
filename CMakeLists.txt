cmake_minimum_required(VERSION 3.25)

project(crude CXX)

set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY ON)
set(CMAKE_SKIP_PACKAGE_ALL_DEPENDENCY ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/NVIDIA_Nsight_Aftermath_SDK_2025.5.0.25317/")

message(STATUS "Using find_package to locate Nsight Aftermath SDK")
find_package(NsightAftermath)
get_filename_component(NsightAftermath_LIBRARY_DIR ${NsightAftermath_LIBRARY} DIRECTORY)

message(STATUS "Nsight Aftermath FOUND = ${NsightAftermath_FOUND}")
message(STATUS "Nsight Aftermath Lib Dir = ${NsightAftermath_LIBRARY_DIR}")
message(STATUS "Nsight Aftermath Lib = ${NsightAftermath_LIBRARY}")
message(STATUS "Nsight Aftermath Headers Include = ${NsightAftermath_INCLUDE_DIR}")

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

set(CMAKE_CONFIGURATION_TYPES "EditorDebug;EditorCompiledShaders;EditorRelease;GameDebug;GameDevelopment;GameProduction;" CACHE STRING "" FORCE)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "EditorDebug" CACHE STRING "Default build type" FORCE)
endif()

#if(CRUDE_SANITIZER_ANY_ENABLED)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:EditorDebug,EditorCompiledShaders,GameDebug>:EditAndContinue>")
#endif

# Debug Configuration
set(CMAKE_CXX_FLAGS_EDITORDEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_DEBUG")
add_compile_definitions("$<$<CONFIG:EditorDebug>:CRUDE_DEVELOP=1>")
add_compile_definitions("$<$<CONFIG:EditorDebug>:CRUDE_DEBUG=1>")
add_compile_definitions("$<$<CONFIG:EditorDebug>:CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED=1>")
add_compile_definitions("$<$<CONFIG:EditorDebug>:CRUDE_EDITOR=1>")
add_compile_definitions("$<$<CONFIG:EditorDebug>:CRUDE_COMPILE_SHADERS=1>")
add_compile_definitions("$<$<CONFIG:EditorDebug>:CRUDE_OPTIMIZE_SHADERS=0>")
set(CMAKE_EXE_LINKER_FLAGS_EDITORDEBUG "${CMAKE_EXE_LINKEEditorDebugR_FLAGS_EDITORDEBUG} /DEBUG" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_EDITORDEBUG "${CMAKE_SHARED_LINKER_FLAGS_EDITORDEBUG} /DEBUG" CACHE STRING "" FORCE)

# Develop Configuration
set(CMAKE_CXX_FLAGS_EDITORCOMPILEDSHADERS "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2")
add_compile_definitions("$<$<CONFIG:EditorCompiledShaders>:CRUDE_DEVELOP=1>")
add_compile_definitions("$<$<CONFIG:EditorCompiledShaders>:CRUDE_EDITOR=1>")
add_compile_definitions("$<$<CONFIG:EditorCompiledShaders>:CRUDE_COMPILE_SHADERS=1>")
add_compile_definitions("$<$<CONFIG:EditorCompiledShaders>:CRUDE_OPTIMIZE_SHADERS=1>")
add_compile_definitions("$<$<CONFIG:EditorCompiledShaders>:IMGUI_DISABLE_DEBUG_TOOLS=1>")
add_compile_definitions("$<$<CONFIG:EditorCompiledShaders>:IMGUI_DISABLE_DEMO_WINDOWS=1>")

# Develop Configuration
set(CMAKE_CXX_FLAGS_EDITORRELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2")
add_compile_definitions("$<$<CONFIG:EditorRelease>:CRUDE_DEVELOP=1>")
add_compile_definitions("$<$<CONFIG:EditorRelease>:CRUDE_EDITOR=1>")
add_compile_definitions("$<$<CONFIG:EditorRelease>:CRUDE_COMPILE_SHADERS=0>")
add_compile_definitions("$<$<CONFIG:EditorRelease>:CRUDE_OPTIMIZE_SHADERS=1>")
add_compile_definitions("$<$<CONFIG:EditorRelease>:IMGUI_DISABLE_DEBUG_TOOLS=1>")
add_compile_definitions("$<$<CONFIG:EditorRelease>:IMGUI_DISABLE_DEMO_WINDOWS=1>")

# Debug Configuration
set(CMAKE_CXX_FLAGS_GAMEDEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_DEBUG")
add_compile_definitions("$<$<CONFIG:GameDebug>:CRUDE_DEVELOP=1>")
add_compile_definitions("$<$<CONFIG:GameDebug>:CRUDE_DEBUG=1>")
add_compile_definitions("$<$<CONFIG:GameDebug>:CRUDE_GRAPHICS_VALIDATION_LAYERS_ENABLED=1>")
add_compile_definitions("$<$<CONFIG:GameDebug>:CRUDE_EDITOR=0>")
add_compile_definitions("$<$<CONFIG:GameDebug>:CRUDE_COMPILE_SHADERS=1>")
add_compile_definitions("$<$<CONFIG:GameDebug>:CRUDE_OPTIMIZE_SHADERS=0>")
set(CMAKE_EXE_LINKER_FLAGS_GAMEDEBUG "${CMAKE_EXE_LINKEEditorDebugR_FLAGS_GAMEDEBUG} /DEBUG" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_GAMEDEBUG "${CMAKE_SHARED_LINKER_FLAGS_GAMEDEBUG} /DEBUG" CACHE STRING "" FORCE)

# Release Configuration  
set(CMAKE_CXX_FLAGS_GAMEDEVELOPMENT "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2")
add_compile_definitions("$<$<CONFIG:GameDevelopment>:CRUDE_DEVELOP=1>")
add_compile_definitions("$<$<CONFIG:GameDevelopment>:CRUDE_PRODUCTION=1>")
add_compile_definitions("$<$<CONFIG:GameDevelopment>:CRUDE_EDITOR=0>")
add_compile_definitions("$<$<CONFIG:GameDevelopment>:CRUDE_COMPILE_SHADERS=0>")
add_compile_definitions("$<$<CONFIG:GameDevelopment>:CRUDE_OPTIMIZE_SHADERS=1>")
add_compile_definitions("$<$<CONFIG:GameDevelopment>:IMGUI_DISABLE_DEBUG_TOOLS=1>")
add_compile_definitions("$<$<CONFIG:GameDevelopment>:IMGUI_DISABLE_DEMO_WINDOWS=1>")

# Production Configuration  
set(CMAKE_CXX_FLAGS_GAMEPRODUCTION "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2")
add_compile_definitions("$<$<CONFIG:GameProduction>:CRUDE_DEVELOP=0>")
add_compile_definitions("$<$<CONFIG:GameProduction>:CRUDE_PRODUCTION=1>")
add_compile_definitions("$<$<CONFIG:GameProduction>:CRUDE_EDITOR=0>")
add_compile_definitions("$<$<CONFIG:GameProduction>:CRUDE_COMPILE_SHADERS=0>")
add_compile_definitions("$<$<CONFIG:GameProduction>:CRUDE_OPTIMIZE_SHADERS=1>")
add_compile_definitions("$<$<CONFIG:GameProduction>:IMGUI_DISABLE_DEBUG_TOOLS=1>")
add_compile_definitions("$<$<CONFIG:GameProduction>:IMGUI_DISABLE_DEMO_WINDOWS=1>")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:EditorDebug>:Debug>$<$<CONFIG:GameDebug>:Debug>DLL")

add_subdirectory(thirdparty)
add_subdirectory(engine)
add_subdirectory(game)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT crude_game)
