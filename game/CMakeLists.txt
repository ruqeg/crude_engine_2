cmake_minimum_required(VERSION 3.25)

project(crude_game)

file(GLOB_RECURSE SRC *.cc)
file(GLOB_RECURSE HEADERS *.h)
file(GLOB_RECURSE CRUDE_ENVIROMENT *.crude_environment)
file(COPY ${ENVIROMENT} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(CRUDE_SOURCES ${SRC} ${HEADERS})

add_executable(${PROJECT_NAME} ${CRUDE_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}\\..\\)

target_link_libraries(${PROJECT_NAME} PUBLIC crude_engine)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${CRUDE_SOURCES})
