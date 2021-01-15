#pragma once

#include "../Unity/IUnityGraphics.h"

#include <stddef.h>

struct IUnityInterfaces;

#define PFG_EDITORLOG(msg) fprintf(stdout, ("[Raytracing Plugin] " + std::string(msg) + "\n").c_str());
#define PFG_EDITORLOGERROR(msg) fprintf(stderr, ("[Raytracing Plugin] " + std::string(msg) + "\n").c_str());

namespace PixelsForGlory
{
    class RayTracerAPI
    {
    public:
        virtual ~RayTracerAPI() { }

        /// <summary>
        /// Process general event like initialization, shutdown, device loss/reset etc.
        /// </summary>
        /// <param name="type"></param>
        /// <param name="interfaces"></param>
        virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces) = 0;

        /// <summary>
        /// Add a mesh that can be ray traced
        /// </summary>
        /// <param name="instanceId"></param>
        /// <param name="vertices"></param>
        /// <param name="vertexCount"></param>
        /// <param name="indices"></param>
        /// <param name="indexCount"></param>
        virtual void AddMesh(int instanceId, float* vertices, float* normals, float* uvs, int vertexCount, int* indices, int indexCount) = 0;

        
    };

    // Create a graphics API implementation instance for the given API type.
    RayTracerAPI* CreateRayTracerAPI(UnityGfxRenderer apiType);
}