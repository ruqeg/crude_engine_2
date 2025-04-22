cmake_minimum_required(VERSION 3.25)

project(vma)

find_package(Vulkan REQUIRED)

set(VMA_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/VulkanMemoryAllocator")
add_library(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/vma_usage.h ${CMAKE_CURRENT_SOURCE_DIR}/vma_usage.cc ${VMA_SOURCE_DIR}/include/vk_mem_alloc.h)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${Vulkan_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC ${Vulkan_LIBRARY})

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE "/MP")
endif()
