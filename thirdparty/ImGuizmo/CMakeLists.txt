# commit a15acd87a3f3241a29ea1363ceafc680dca3a96b

cmake_minimum_required(VERSION 3.25)

project(imguizmo)

file(GLOB_RECURSE SRC *.c *.cpp *.cc)
file(GLOB_RECURSE HEADERS *.h *.inl)

set(SOURCES ${SRC} ${HEADERS})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
add_library(${PROJECT_NAME} STATIC ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3-static ${Vulkan_LIBRARY})
target_include_directories(${PROJECT_NAME} PRIVATE imgui ${Vulkan_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../imgui/)
