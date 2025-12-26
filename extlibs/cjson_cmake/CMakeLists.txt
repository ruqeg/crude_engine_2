cmake_minimum_required(VERSION 3.25)

project(cjson_cmake C)

set(CMAKE_C_STANDARD 11)

set(CJSON_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cJSON)
add_library(${PROJECT_NAME} STATIC 
  ${CJSON_SOURCE_DIR}/cJSON.h ${CJSON_SOURCE_DIR}/cJSON.c
  ${CJSON_SOURCE_DIR}/cJSON_Utils.h ${CJSON_SOURCE_DIR}/cJSON_Utils.c
)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/cJSON)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE "/MP")
endif()
