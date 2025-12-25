cmake_minimum_required(VERSION 3.25)

project(crude_editor)

file(GLOB_RECURSE SRC *.cc)
file(GLOB_RECURSE HEADERS *.h)
file(GLOB_RECURSE HEADERS *.crude_environment)

set(CRUDE_SOURCES ${SRC} ${HEADERS})

add_executable(${PROJECT_NAME} ${CRUDE_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}\\..\\)

target_link_libraries(${PROJECT_NAME} PUBLIC crude_engine)

source_group(${PROJECT_NAME} FILES ${CRUDE_SOURCES})

target_compile_definitions(${PROJECT_NAME} PUBLIC CRUDE_EDITOR)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE "/MP" "/ZI")
endif()
