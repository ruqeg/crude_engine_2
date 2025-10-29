cmake_minimum_required(VERSION 3.25)

project(imguizmo_cmake CXX)

set(CMAKE_CXX_STANDARD 20)

set(IMGUIZMO_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo")

add_library(${PROJECT_NAME} SHARED
  ${IMGUIZMO_SOURCE_DIR}/GraphEditor.cpp
  ${IMGUIZMO_SOURCE_DIR}/GraphEditor.h
  ${IMGUIZMO_SOURCE_DIR}/ImCurveEdit.cpp
  ${IMGUIZMO_SOURCE_DIR}/ImCurveEdit.h
  ${IMGUIZMO_SOURCE_DIR}/ImGradient.cpp
  ${IMGUIZMO_SOURCE_DIR}/ImGradient.h
  ${IMGUIZMO_SOURCE_DIR}/ImGuizmo.cpp
  ${IMGUIZMO_SOURCE_DIR}/ImGuizmo.h
  ${IMGUIZMO_SOURCE_DIR}/ImSequencer.cpp
  ${IMGUIZMO_SOURCE_DIR}/ImSequencer.h
  ${IMGUIZMO_SOURCE_DIR}/ImZoomSlider.h
)

target_link_libraries(${PROJECT_NAME} PUBLIC imgui_cmake)

target_include_directories(${PROJECT_NAME} PUBLIC ${IMGUIZMO_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE "/MP")
endif()
