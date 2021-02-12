#pragma once

#include "../PlatformBase.h"
#include "../Unity/IUnityGraphics.h"

#include <stddef.h>
#include <string>

struct IUnityInterfaces;

namespace PixelsForGlory
{
    class RayTracerAPI
    {
    public:

        enum class AddResourceResult
        {
            Error         = 0,
            Success       = 1,
            AlreadyExists = 2
        };

        struct RayTracerStatistics
        {
            RayTracerStatistics()
                : RegisteredLights(0)
                , RegisteredSharedMeshes(0)
                , RegisteredMeshInstances(0)
                , RegisteredMaterials(0)
                , RegisteredTextures(0)
                , RegisteredRenderTargets(0)
                , DescriptorSetCount(0)
                , AccelerationStuctureCount(0)
                , UniformBufferCount(0)
                , StorageImageCount(0)
                , StorageBufferCount(0)
                , CombinedImageSamplerCount(0)
            {}

            uint32_t RegisteredLights;
            uint32_t RegisteredSharedMeshes;
            uint32_t RegisteredMeshInstances;
            uint32_t RegisteredMaterials;
            uint32_t RegisteredTextures;
            uint32_t RegisteredRenderTargets;

            uint32_t DescriptorSetCount;
            uint32_t AccelerationStuctureCount;
            uint32_t UniformBufferCount;
            uint32_t StorageImageCount;
            uint32_t StorageBufferCount;
            uint32_t CombinedImageSamplerCount;
        };

        virtual ~RayTracerAPI() { }

        /// <summary>
        /// Set the shader folder to pull shaders from
        /// </summary>
        virtual void SetShaderFolder(std::wstring shaderFolder) = 0;

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
        virtual AddResourceResult AddSharedMesh(int sharedMeshInstanceId, float* verticesArray, float* normalsArray, float* tangentsArray, float* uvsArray, int vertexCount, int* indicesArray, int indexCount) = 0;

        /// <summary>
        /// Add transform for an instance to be build on the tlas
        /// </summary>
        /// <param name="gameObjectInstanceId"></param>
        /// <param name="meshInstanceId"></param>
        /// <param name="sharedMeshInstanceId"></param>
        /// <param name="l2wMatrix"></param>
        virtual AddResourceResult AddTlasInstance(int gameObjectInstanceId, int sharedMeshInstanceId, int materialInstanceId, float* l2wMatrix, float* w2lMatrix) = 0;

        virtual void UpdateTlasInstance(int gameObjectInstanceId, int materialInstanceId, float* l2wMatrix, float* w2lMatrix) = 0;
            
        /// <summary>
        /// Removes instance to be removed on next tlas build
        /// </summary>
        /// <param name="meshInstanceIndex"></param>
        virtual void RemoveTlasInstance(int gameObjectInstanceId) = 0;

        /// <summary>
        /// Prepare the ray tracer for rendering
        /// </summary>
        virtual void Prepare() = 0;

        /// <summary>
        /// Resets the pipeline to trigger reloading of shaders
        /// </summary>
        virtual void ResetPipeline() = 0;

        /// <summary>
        /// Update camera data
        /// </summary>
        /// <param name="camPos"></param>
        /// <param name="camDir"></param>
        /// <param name="camUp"></param>
        /// <param name="camSide"></param>
        /// <param name="camNearFarFov"></param>
        virtual void UpdateCamera(int cameraInstanceId, float* camPos, float* camDir, float* camUp, float* camRight, float camNear, float camFar, float camFov) = 0;

        /// <summary>
        /// Update scene data 
        /// </summary>
        /// <param name="color"></param>
        virtual void UpdateSceneData(float* color) = 0;

        virtual AddResourceResult AddLight(int lightInstanceId, float x, float y, float z, float r, float g, float b, float bounceIntensity, float intensity, float range, float spotAngle, int type, bool enabled) = 0;

        virtual void UpdateLight(int lightInstanceId, float x, float y, float z, float r, float g, float b, float bounceIntensity, float intensity, float range, float spotAngle, int type, bool enabled) = 0;

        virtual void RemoveLight(int lightInstanceId) = 0;

        virtual AddResourceResult AddTexture(int textureInstanceId, void* texture) = 0;
        virtual void RemoveTexture(int textureInstanceId) = 0;

        virtual AddResourceResult AddMaterial(int materialInstanceId,
                                              float albedo_r, float albedo_g, float albedo_b,
                                              float emission_r, float emission_g, float emission_b,
                                              float transmittance_r, float transmittance_g, float transmittance_b,
                                              float metallic,
                                              float roughness,
                                              float indexOfRefraction,
                                              bool albedoTextureSet,
                                              int albedoTextureInstanceId,
                                              bool emissionTextureSet,
                                              int emissionTextureInstanceId,
                                              bool normalTextureSet,
                                              int normalTextureInstanceId,
                                              bool metallicTextureSet,
                                              int metallicTextureInstanceId,
                                              bool roughnessTextureSet,
                                              int roughnessTextureInstanceId,
                                              bool ambientOcclusionTextureSet,
                                              int ambientOcclusionTextureInstanceId) = 0;

        virtual void UpdateMaterial(int materialInstanceId,
                                    float albedo_r, float albedo_g, float albedo_b,
                                    float emission_r, float emission_g, float emission_b,
                                    float transmittance_r, float transmittance_g, float transmittance_b,
                                    float metallic,
                                    float roughness,
                                    float indexOfRefraction,
                                    bool albedoTextureSet,
                                    int albedoTextureInstanceId,
                                    bool emissionTextureSet,
                                    int emissionTextureInstanceId,
                                    bool normalTextureSet,
                                    int normalTextureInstanceId,
                                    bool metallicTextureSet,
                                    int metallicTextureInstanceId,
                                    bool roughnessTextureSet,
                                    int roughnessTextureInstanceId,
                                    bool ambientOcclusionTextureSet,
                                    int ambientOcclusionTextureInstanceId) = 0;

        virtual void RemoveMaterial(int materialInstanceId) = 0;

        /// <summary>
        /// Ray those rays!
        /// </summary>
        virtual void TraceRays(int cameraInstanceId) = 0;

        virtual RayTracerStatistics GetRayTracerStatistics() = 0;
    };

    // Create a graphics API implementation instance for the given API type.
    RayTracerAPI* CreateRayTracerAPI(UnityGfxRenderer apiType);
}