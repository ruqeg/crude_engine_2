cmake_minimum_required(VERSION 3.25)

project(crude_game)

file(GLOB_RECURSE SRC *.cc)
file(GLOB_RECURSE HEADERS *.h)
file(GLOB_RECURSE CRUDE_ENVIROMENT *.crude_environment)
file(COPY ${ENVIROMENT} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(CRUDE_SOURCES ${SRC} ${HEADERS} ${CRUDE_ENVIROMENT})

add_executable(${PROJECT_NAME} ${CRUDE_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}\\..\\)

target_link_libraries(${PROJECT_NAME} PUBLIC crude_engine)


add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo "Copy ${CRUDE_ENVIROMENT} to ${CMAKE_CURRENT_BINARY_DIR}!"
  COMMAND ${CMAKE_COMMAND} -E copy ${CRUDE_ENVIROMENT} ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Running post-build steps..."
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${CRUDE_SOURCES})
