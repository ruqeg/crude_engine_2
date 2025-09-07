include(FetchContent)

add_subdirectory(cgltf_cmake)
add_subdirectory(stb_cmake)
add_subdirectory(tlsf_cmake)
add_subdirectory(vma_cmake)
add_subdirectory(spirv_reflect_cmake)
add_subdirectory(cjson_cmake)
add_subdirectory(imgui_cmake)
add_subdirectory(renderdoc_api_cmake)

######################
#
# ENKITS
#
######################
FetchContent_Declare(
  enkiTS
	GIT_REPOSITORY https://github.com/ruqeg/enkiTS.git
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(enkiTS)

######################
#
# MINIAUDIO
#
######################
FetchContent_Declare(
  miniaudio
	GIT_REPOSITORY https://github.com/ruqeg/miniaudio.git
	GIT_TAG 0.11.22
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(miniaudio)

######################
#
# FLECS
#
######################
FetchContent_Declare(
  flecs
	GIT_REPOSITORY https://github.com/ruqeg/flecs.git
	GIT_TAG v4.1.0
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(flecs)

######################
#
# SDL
#
######################
FetchContent_Declare(
  SDL3
	GIT_REPOSITORY https://github.com/ruqeg/SDL.git
	GIT_TAG release-3.2.16
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(SDL3)

######################
#
# TRACY
#
######################
FetchContent_Declare(
	tracy
	GIT_REPOSITORY https://github.com/ruqeg/tracy.git
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

######################
#
# MESHOPTIMIZER
#
######################
FetchContent_Declare(
	meshoptimizer
	GIT_REPOSITORY https://github.com/ruqeg/meshoptimizer.git
	GIT_TAG v0.24
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(meshoptimizer)
