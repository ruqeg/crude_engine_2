set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF)
add_subdirectory(Build)
target_compile_options(Jolt PRIVATE /wd5045)
