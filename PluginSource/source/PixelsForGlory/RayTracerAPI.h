#pragma once

#include "../PlatformBase.h"
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
        /// Creates or updates a camera render target
        /// </summary>
        /// <param name="cameraInstanceId"></param>
        /// <param name="unityTextureFormat"></param>
        /// <param name="width"></param>
        /// <param name="height"></param>
        /// <param name="textureHandle">Destination texture for render</param>
        /// <returns>1 if successful, 0 otherwise</returns>
        virtual int SetRenderTarget(int cameraInstanceId, int unityTextureFormat, int width, int height, void* textureHandle) = 0;

        /// <summary>
        /// Process general event like initialization, shutdown, device loss/reset etc.
        /// </summary>
        /// <param name="type"></param>
        /// <param name="interfaces"></param>
        /// <returns>true if processing was successful, false otherwise</returns>
        virtual bool ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces) = 0;

        /// <summary>
        /// Method to check if a shared mesh has already been added, saves gathering handles if exists
        /// </summary>
        /// <param name="sharedMeshInstanceId"></param>
        /// <returns></returns>
        virtual int GetSharedMeshIndex(int sharedMeshInstanceId) = 0;

        /// <summary>
        /// Add a shared mesh that can be ray traced
        /// </summary>
        /// <param name="instanceId"></param>
        /// <param name="vertices"></param>
        /// <param name="normals"></param>
        /// <param name="vertexCount"></param>
        /// <param name="uvs"></param>
        /// <param name="uvCount"></param>
        /// <param name="indices"></param>
        /// <param name="indexCount"></param>
        virtual int AddSharedMesh(int instanceId, float* verticesArray, float* normalsArray, float* uvsArray, int vertexCount, int* indicesArray, int indexCount) = 0;

        /// <summary>
        /// Add transform for an instance to be build on the tlas
        /// </summary>
        /// <param name="meshInstanceId"></param>
        /// <param name="sharedMeshInstanceId"></param>
        /// <param name="l2wMatrix"></param>
        virtual int AddTlasInstance(int sharedMeshIndex, float* l2wMatrix) = 0;
            
        /// <summary>
        /// Removes instance to be removed on next tlas build
        /// </summary>
        /// <param name="meshInstanceIndex"></param>
        virtual void RemoveTlasInstance(int meshInstanceIndex) = 0;

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