cmake_minimum_required(VERSION 3.25)

project(imgui_node_editor_cmake CXX)

set(CMAKE_CXX_STANDARD 20)

set(IMGUI_NODE_EDITOR_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui-node-editor")

add_library(${PROJECT_NAME}
  ${IMGUI_NODE_EDITOR_SOURCE_DIR}/crude_json.cpp
  ${IMGUI_NODE_EDITOR_SOURCE_DIR}/crude_json.h
  ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_bezier_math.h
  ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_bezier_math.inl
  ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_canvas.cpp
  ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_canvas.cpp
  ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_canvas.h
  ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_canvas.h
  ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_extra_math.h
  ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_extra_math.inl
  ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_node_editor_api.cpp
  ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_node_editor_internal.h
  ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_node_editor_internal.inl
  ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_node_editor.cpp
  ${IMGUI_NODE_EDITOR_SOURCE_DIR}/imgui_node_editor.h
  ${IMGUI_NODE_EDITOR_SOURCE_DIR}/misc/imgui_node_editor.natvis
)

target_include_directories(${PROJECT_NAME} PUBLIC ${IMGUI_NODE_EDITOR_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE imgui_cmake)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE "/MP")
endif()
