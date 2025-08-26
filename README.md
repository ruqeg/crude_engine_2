
# Crude Engine 2

![Hey sweetie have you heard about crude engine](https://github.com/ruqeg/crude_engine_2/blob/main/docs/intro.png?raw=true)

## Overview
CE2 is a 3D game engine created to have fun at free time, written in C++ due to some dependencies, but I use C99-style to reduce headache with extra abstraction. The renderer is based on Vulkan with no current plans to support other graphics APIs. While the engine was initially designed to support mesh/classic pipeline, recent changes have shifted the focus to a task/mesh shader. Currently CE2 only supports Windows, but Linux support is planned at the future. The project is still under active development, but I think that readme won't hurt.

## Features
- Render Graph
- Clustered Shading, Occlusion Culling, Tetrahedron Shadow Mapping
- Asynchronous Resources Loading, Multi-Threading Commands Recording
- Json Scene, GLTF Models
- GPU Profiler
- Shader Hot Reloading, Task/Mesh Shader, ECS, etc

## Setup
Download and install Vulkan SDK from [LunarG website](https://vulkan.lunarg.com/) for your system. Then clone this repository
```
git clone https://github.com/ruqeg/crude_engine_2
cd crude_engine_2
git submodule update --init --recursive
```
build VS2022 project with cmake
```
mkdir build
cd build
cmake ../ -G "Visual Studio 17 2022"
```
open generated `.sln` file in Visual Studio 2022 and build solution. 

To edit shaders in VSCode, download the [GLSL Lints](https://marketplace.visualstudio.com/items?itemName=dtoplak.vscode-glsllint) extension and configure custom glslangValidatorArgs
```
"glsllint.glslangValidatorArgs": [
	"--target-env",
	"vulkan1.2",
	"--glsl-version",
	"460",
	"-DCRUDE_VALIDATOR_LINTING"
],
```

## Dependencies
- [cgltf](https://github.com/jkuhlmann/cgltf)
- [cJSON](https://github.com/DaveGamble/cJSON)
- [cr](https://github.com/fungos/cr)
- [imgui](https://github.com/ocornut/imgui)
- [SPIRV-Reflect](https://github.com/KhronosGroup/SPIRV-Reflect)
- [stb](https://github.com/nothings/stb)
- [tlsf](https://github.com/mattconte/tlsf)
- [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- [enkiTS](https://github.com/dougbinks/enkiTS)
- [miniaudio](https://github.com/mackron/miniaudio)
- [flecs](https://github.com/ruqeg/flecs)
- [SDL3](https://github.com/ruqeg/SDL)
- [tracy](https://github.com/ruqeg/tracy)
- [meshoptimizer](https://github.com/ruqeg/meshoptimizer)
- [flecs](https://github.com/SanderMertens/flecs)

