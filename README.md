
# Crude Engine 2
![Hey sweetie, have you heard about crude engine](https://github.com/ruqeg/crude_engine_2/blob/main/docs/images/intro.png?raw=true)

## Overview
CE2 is a 3D game engine with gpu driven rendering created to have fun at free time, written in C++ due to some dependencies, but I use C99-style to reduce headache with extra abstraction. The renderer is based on Vulkan with no current plans to support other graphics APIs. Currently CE2 only supports Windows, but Linux support is planned at the future. The project is still under active development, but I think that readme won't hurt.

## Features
- Render Graph, Clustered Shading, Occlusion Culling, Tetrahedron Shadow Mapping, Automatic Exposure
- Asynchronous Resources Loading, Multi-Threading Commands Recording
- Json Scene, GLTF Models
- GPU Profiler, Shader Hot Reloading, Task/Mesh Shader, ECS, etc

## Setup
Download and install Vulkan SDK from [LunarG website](https://vulkan.lunarg.com/) for your system. Then clone this repository
```
git clone https://github.com/ruqeg/crude_engine_2
cd crude_engine_2
```
build VS2022 project with cmake
```
mkdir build
cd build
cmake ../ -G "Visual Studio 17 2022"
```
open generated `.sln` file in Visual Studio 2022 and build solution. 

To edit shaders in VS download the [GLSL language integration (for VS2022)](https://marketplace.visualstudio.com/items?itemName=DanielScherzer.GLSL2022) extension and configure:
```
"Audo detect shader type file extension":
.crude_shader

"Arguments for the external compiler executable":
--target-env vulkan1.2 --glsl-version 460 -DCRUDE_VALIDATOR_LINTING -e GL_GOOGLE_include_directive

"External compiler executable file path (without quotes)":
glslangValidator.exe
```

To edit shaders in VSCode, download the [GLSL Lints](https://marketplace.visualstudio.com/items?itemName=dtoplak.vscode-glsllint) extension and configure custom glslangValidatorArgs
```
"glsllint.glslangValidatorArgs": [
	"--target-env",
	"vulkan1.2",
	"--glsl-version",
	"460",
	"-DCRUDE_VALIDATOR_LINTING"
    "-e"
    "GL_GOOGLE_include_directive"
],
```
