cmake_minimum_required(VERSION 3.25)

project(crude_engine)

find_package(Vulkan REQUIRED)

file(GLOB_RECURSE SRC *.cc *.crude_shader *.crude_techniques)
file(GLOB_RECURSE HEADERS *.h *.inl)

set(CRUDE_SOURCES ${SRC} ${HEADERS})

add_library(${PROJECT_NAME} STATIC ${CRUDE_SOURCES})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${NsightAftermath_LIBRARY}"
  $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying Nsight Aftermath library to output directory")
install(FILES "${NsightAftermath_LIBRARY}" DESTINATION ${CMAKE_INSTALL_BINDIR})
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${NsightAftermath_DLL}"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying Nsight Aftermath DLL to output directory")
install(FILES "${NsightAftermath_DLL}" DESTINATION ${CMAKE_INSTALL_BINDIR})

target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}\\..\\)
target_link_libraries(${PROJECT_NAME} PUBLIC ${Vulkan_LIBRARY} imgui imguizmo flecs SDL3-static stb tlsf VulkanMemoryAllocator cgltf spirv-reflect-static miniaudio enkiTS TracyClient cjson meshoptimizer nfd DirectXMath imgui-node-editor NsightAftermath::NsightAftermath Jolt)

# Also install the library to the installation directory
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${CRUDE_SOURCES})
