# Unity Ray Tracing Plugin
Proof of concept native plugin for Unity3d that replaces the renderer with a hardware accelerated ray tracer.

Heavily influenced by:
- [rtxOn](https://github.com/iOrange/rtxON)
- [NVIDIA Vulkan Ray Tracing Tutorials](https://github.com/nvpro-samples/vk_raytracing_tutorial_KHR)
- [Khronos Vulkan Samples](https://github.com/KhronosGroup/Vulkan-Samples)

Includes zips with versions used to build for:
- [glm](https://github.com/g-truc/glm) - OpenGL Mathematics (GLM) library for graphics software based on the OpenGL Shading Language (GLSL) specifications.
- [glslang](https://github.com/KhronosGroup/glslang) - An OpenGL GLSL and OpenGL|ES GLSL (ESSL) front-end for reference validation and translation of GLSL/ESSL into an internal abstract syntax tree (AST).
- [stb](https://github.com/nothings/stb) - Image loader 
- [volk](https://github.com/zeux/volk) - Meta-loader for Vulkan

Tested on RTX 2080 with [NVIDIA Vulkan Beta drivers](https://developer.nvidia.com/vulkan-driver) installed.
Builds with [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) version 1.2.162.1
