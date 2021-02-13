![Current State](Screenshot.png?raw=true "Current State")

# Unity Ray Tracing Plugin
Proof of concept native plugin for Unity3d that replaces the renderer with a hardware accelerated ray tracer implemented with Vulkan.

Tested on RTX 2080 with [NVIDIA Vulkan Beta drivers](https://developer.nvidia.com/vulkan-driver) installed (457.67).

Builds with [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) version 1.2.162.1

## Usage
- Point to this repository to [install as a package in a Unity project](https://docs.unity3d.com/Manual/upm-git.html)
- In Unity
  - Set the Graphics API to Vulkan under `Project Settings => Player` 
  - Create a `Ray Tracing Render Pipeline` under `Pixels for Glory => Ray Tracing`
  - Set created asset to `Scriptable Render Pipeline Settings` under `Project Settings => Graphics` 
  - Add `Ray Traceable Light` to any light in the scene to have the plugin register it for lighting
  - Add `Ray Traceable Object` to any object in the scene to have the plugin register it for rendering.  Must have at least a valid Mesh assigned in the `MeshFilter`
  - Use `Ray Tracer Material` which uses a PBR model to shade objects
  
  ![Ray Tracer Material](RayTracerMaterial.png?raw=true "Ray Tracer Material")
  

## Credits
Heavily influenced by:
- [Unity Native Rendering Plugin](https://github.com/Unity-Technologies/NativeRenderingPlugin)
- [rtxOn](https://github.com/iOrange/rtxON)
- [NVIDIA Vulkan Ray Tracing Tutorials](https://github.com/nvpro-samples/vk_raytracing_tutorial_KHR)
- [Khronos Vulkan Samples](https://github.com/KhronosGroup/Vulkan-Samples)

Shaders built with theories referenced in:
- [coding labs](http://www.codinglabs.net/Authors.aspx)
- [Graphics Programming Compendium](https://graphicscompendium.com/)
- [Learn OpenGL](https://learnopengl.com/)
- [Scratchpixel](https://www.scratchapixel.com/)

Includes zips with versions used to build for:
- [glm](https://github.com/g-truc/glm) - OpenGL Mathematics (GLM) library for graphics software based on the OpenGL Shading Language (GLSL) specifications.
- [glslang](https://github.com/KhronosGroup/glslang) - An OpenGL GLSL and OpenGL|ES GLSL (ESSL) front-end for reference validation and translation of GLSL/ESSL into an internal abstract syntax tree (AST).
- [volk](https://github.com/zeux/volk) - Meta-loader for Vulkan
