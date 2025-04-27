include(FetchContent)

add_subdirectory(SDL)
add_subdirectory(cgltf_cmake)
add_subdirectory(cimgui)
add_subdirectory(cr_cmake)
add_subdirectory(flecs)
add_subdirectory(stb_cmake)
add_subdirectory(tlsf_cmake)
add_subdirectory(vma_cmake)
add_subdirectory(spirv_reflect_cmake)
add_subdirectory(miniaudio)
add_subdirectory(enkiTS)

FetchContent_Declare(
	tracy
	GIT_REPOSITORY https://github.com/wolfpld/tracy.git
	GIT_TAG v0.11.1
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(tracy)

target_compile_definitions(TracyClient PRIVATE
  TRACY_ENABLE
  TRACY_ON_DEMAND
  TRACY_NO_SYSTEM_TRACING
)
