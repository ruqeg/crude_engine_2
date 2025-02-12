cmake_minimum_required(VERSION 3.25)

project(vma C)

add_library(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/vma_impl.cc)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIR})

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE "/MP")
endif()
