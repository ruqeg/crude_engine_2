cmake_minimum_required(VERSION 3.25)

project(imgui_cmake CXX)

set(CMAKE_CXX_STANDARD 20)

set(IMGUI_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
set(IMGUI_BACKENDS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends")

add_library(${PROJECT_NAME}
  ${IMGUI_SOURCE_DIR}/imgui.cpp
  ${IMGUI_SOURCE_DIR}/imgui_tables.cpp
  ${IMGUI_SOURCE_DIR}/imgui_widgets.cpp
  ${IMGUI_SOURCE_DIR}/imgui_draw.cpp
  ${IMGUI_SOURCE_DIR}/imgui_demo.cpp
  ${IMGUI_SOURCE_DIR}/imconfig.h 
  ${IMGUI_SOURCE_DIR}/imgui.h
  ${IMGUI_SOURCE_DIR}/imgui_internal.h 
  ${IMGUI_SOURCE_DIR}/imstb_rectpack.h 
  ${IMGUI_SOURCE_DIR}/imstb_textedit.h 
  ${IMGUI_SOURCE_DIR}/imstb_truetype.h 
  ${IMGUI_BACKENDS_SOURCE_DIR}/imgui_impl_sdlrenderer3.cpp
  ${IMGUI_BACKENDS_SOURCE_DIR}/imgui_impl_sdlrenderer3.h
  ${IMGUI_BACKENDS_SOURCE_DIR}/imgui_impl_sdl3.cpp
  ${IMGUI_BACKENDS_SOURCE_DIR}/imgui_impl_sdl3.h
  ${IMGUI_BACKENDS_SOURCE_DIR}/imgui_impl_vulkan.cpp
  ${IMGUI_BACKENDS_SOURCE_DIR}/imgui_impl_vulkan.h)

find_package(Vulkan REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE ${Vulkan_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3 ${Vulkan_LIBRARY})

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE "/MP")
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${IMGUI_SOURCE_DIR})
