cmake_minimum_required(VERSION 3.25)

project(renderdoc_api_cmake CXX)

add_library(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/renderdoc_app.h)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
