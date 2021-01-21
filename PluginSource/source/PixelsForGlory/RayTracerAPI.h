#pragma once

#include "../Unity/IUnityGraphics.h"

#include <stddef.h>

struct IUnityInterfaces;

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

        virtual int GetSharedMeshIndex(int sharedMeshInstanceId) = 0;

        /// <summary>
        /// Add a mesh that can be ray traced
        /// </summary>
        /// <param name="instanceId"></param>
        /// <param name="vertices"></param>
        /// <param name="normals"></param>
        /// <param name="vertexCount"></param>
        /// <param name="uvs"></param>
        /// <param name="uvCount"></param>
        /// <param name="indices"></param>
        /// <param name="indexCount"></param>
        virtual int AddMesh(int instanceId, float* vertices, float* normals, float* uvs, int vertexCount, int* indices, int indexCount) = 0;

        /// <summary>
        /// Add transform for an instance to be build on the tlas
        /// </summary>
        /// <param name="meshInstanceId"></param>
        /// <param name="sharedMeshInstanceId"></param>
        /// <param name="l2wMatrix"></param>
        virtual int AddTlasInstance(int sharedMeshIndex, float* l2wMatrix) = 0;
            
        /// <summary>
        /// Build top level acceleration structure
        /// </summary>
        virtual void BuildTlas() = 0;

        /// <summary>
        /// Prepare ray tracer to render
        /// </summary>
        virtual void Prepare(int width, int height) = 0;

        virtual void UpdateCamera(float* camPos, float* camDir, float* camUp, float* camSide, float* camNearFarFov) = 0;
        virtual void UpdateSceneData(float* color) = 0;

        virtual void TraceRays() = 0;
        virtual void CopyImageToTexture(void* TextureHandle) = 0;
    };

    // Create a graphics API implementation instance for the given API type.
    RayTracerAPI* CreateRayTracerAPI(UnityGfxRenderer apiType);
}