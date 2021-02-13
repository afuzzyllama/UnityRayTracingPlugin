#pragma once

#include <map>
#include <memory>

#include "../../vulkan.h"
#include "../../Unity/IUnityGraphics.h"
#include "../../Unity/IUnityGraphicsVulkan.h"
#include "../RayTracerAPI.h"

#include "../ResourcePool.h"
#include "Buffer.h"
#include "Image.h"
#include "Shader.h"
#include "ShaderBindingTable.h"

#include "ShaderConstants.h"

namespace PixelsForGlory
{
    RayTracerAPI* CreateRayTracerAPI_Vulkan();
}

namespace PixelsForGlory::Vulkan
{
    /// <summary>
   /// Builds VkDeviceCreateInfo that supports ray tracing
   /// </summary>
   /// <param name="physicalDevice">Physical device used to query properties and queues</param>
   /// <param name="outDeviceCreateInfo"></param>
    VkResult CreateDevice_RayTracer(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* unityCreateInfo, const VkAllocationCallbacks* unityAllocator, VkDevice* device);

    /// <summary>
    /// Resolve properties and queues required for ray tracing
    /// </summary>
    /// <param name="physicalDevice"></param>
    void ResolvePropertiesAndQueues_RayTracer(VkPhysicalDevice physicalDevice);

    struct RayTracerDescriptorSetsData
    {
        RayTracerDescriptorSetsData()
            : acceleration_stucture_count(0)
            , uniform_buffer_count(0)
            , storage_image_count(0)
            , storage_buffer_count(0)
            , combined_image_sampler_count(0)
        {}

        std::vector<VkDescriptorSet> descriptorSets;

        uint32_t acceleration_stucture_count;
        uint32_t uniform_buffer_count;
        uint32_t storage_image_count;
        uint32_t storage_buffer_count;
        uint32_t combined_image_sampler_count;
    };


    struct RayTracerRenderTarget
    {
        RayTracerRenderTarget()
            : destination(nullptr)
            , format(VK_FORMAT_UNDEFINED)
            , extent(VkExtent3D())
            , cameraDataBufferInfo(VkDescriptorBufferInfo())
        {}

        void* destination;
        VkFormat format;
        VkExtent3D extent;
        Vulkan::Image stagingImage;
        
        // Buffer that represents ShaderCameraParam
        Vulkan::Buffer cameraData;
        VkDescriptorBufferInfo cameraDataBufferInfo;

        std::map<uint64_t, RayTracerDescriptorSetsData> descriptorSetsData;
    };

    struct RayTracerAccelerationStructure
    {
        RayTracerAccelerationStructure()
            : accelerationStructure(VK_NULL_HANDLE)
            , deviceAddress(0)
            , buffer(Vulkan::Buffer())
        {}

        VkAccelerationStructureKHR    accelerationStructure;
        uint64_t                      deviceAddress;
        Vulkan::Buffer                buffer;
    };

    struct RayTracerMeshSharedData
    {
        RayTracerMeshSharedData()
            : sharedMeshInstanceId(-1)
            , vertexCount(0)
            , indexCount(0)
            , blas(RayTracerAccelerationStructure())
        {}

        int sharedMeshInstanceId;

        int vertexCount;
        int indexCount;

        Vulkan::Buffer vertexBuffer;          // Stores: vertex : vec3 
        Vulkan::Buffer indexBuffer;           // Stores: index : int
        Vulkan::Buffer attributesBuffer;      // Stores: ShaderVertexAttribute Data
        Vulkan::Buffer facesBuffer;           // Stores: ShaderFacesData

        RayTracerAccelerationStructure blas;
    };
    
    struct RayTracerAccelerationStructureBuildInfo
    {
        VkAccelerationStructureBuildGeometryInfoKHR& accelerationBuildGeometryInfo;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*>& accelerationStructureBuildRangeInfos;
    };

    struct RayTracerMeshInstanceData
    {
        RayTracerMeshInstanceData()
            : gameObjectInstanceId(0)
            , sharedMeshInstanceId(0)
            , materialInstanceId(0)
            , localToWorld(mat4()) 
        {}

        int32_t  gameObjectInstanceId;
        int32_t  sharedMeshInstanceId;
        int32_t  materialInstanceId;
        mat4     localToWorld;
        mat4     worldToLocal;

        Vulkan::Buffer instanceData;
    };

    struct RayTracerGarbageBuffer
    {
        RayTracerGarbageBuffer()
            : frameCount(0) 
        {}

        std::unique_ptr<Vulkan::IResource> buffer;
        uint64_t frameCount;
    };

    class RayTracer : public RayTracerAPI
    {
    public:
        static bool CreateDeviceSuccess;

        static RayTracer& Instance()
        {
            static RayTracer instance;
            return instance;
        }

        VkDebugUtilsMessengerEXT debugMessenger_;

        // Friend so hook functions can access
        friend void ResolvePropertiesAndQueues_RayTracer(VkPhysicalDevice physicalDevice);
        friend VkResult CreateInstance_RayTracer(const VkInstanceCreateInfo* unityCreateInfo, const VkAllocationCallbacks* unityAllocator, VkInstance* instance);
        friend VkResult CreateDevice_RayTracer(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* unityCreateInfo, const VkAllocationCallbacks* unityAllocator, VkDevice* device);

        // Deleted functions should generally be public as it results in better error messages due to 
        // the compilers behavior to check accessibility before deleted status

        RayTracer(RayTracer const&) = delete;       // Deleted for singleton
        void operator=(RayTracer const&) = delete;  // Deleted for singleton

        /// <summary>
        /// Initalize instance with data from Unity
        /// </summary>
        /// <param name="unityVulkanInstance"></param>
        void InitializeFromUnityInstance(IUnityGraphicsVulkan* graphicsInterface);

        /// <summary>
        /// Shutdown the ray tracer and release all resources
        /// </summary>
        void Shutdown();

#pragma region RayTracerAPI
        virtual void SetShaderFolder(std::wstring shaderFolder);
        virtual int SetRenderTarget(int cameraInstanceId, int unityTextureFormat, int width, int height, void* textureHandle);
        virtual bool ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces);
        virtual AddResourceResult AddSharedMesh(int sharedMeshInstanceId, float* verticesArray, float* normalsArray, float* tangentsArray, float* uvsArray, int vertexCount, int* indicesArray, int indexCount);
        virtual AddResourceResult AddTlasInstance(int gameObjectInstanceId, int sharedMeshInstanceId, int materialInstanceId, float* l2wMatrix, float* w2lMatrix);
        virtual void UpdateTlasInstance(int gameObjectInstanceId, int materialInstanceId, float* l2wMatrix, float* w2lMatrix);
        virtual void RemoveTlasInstance(int gameObjectInstanceId);
        virtual void Prepare();
        virtual void ResetPipeline();
        virtual void UpdateCamera(int cameraInstanceId, float* camPos, float* camDir, float* camUp, float* camRight, float camNear, float camFar, float camFov);
        virtual void UpdateSceneData(float* color);
        virtual AddResourceResult AddLight(int lightInstanceId, float x, float y, float z, float r, float g, float b, float bounceIntensity, float intensity, float range, float spotAngle, int type, bool enabled);
        virtual void UpdateLight(int lightInstanceId, float x, float y, float z, float r, float g, float b, float bounceIntensity, float intensity, float range, float spotAngle, int type, bool enabled);
        virtual void RemoveLight(int lightInstanceId);
        virtual AddResourceResult AddTexture(int textureInstanceId, void* texture);
        virtual void RemoveTexture(int textureInstanceId);
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
                                              int ambientOcclusionTextureInstanceId);
              
        
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
                                    int ambientOcclusionTextureInstanceId);

        virtual void RemoveMaterial(int materialInstanceId);
        virtual void TraceRays(int cameraInstanceId);
        virtual RayTracerStatistics GetRayTracerStatistics();
#pragma endregion RayTracerAPI


    private:
        /// <summary>
        /// Dummy null device for device_ member
        /// </summary>
        static VkDevice NullDevice;

        /// <summary>
        /// Graphics interface from Unity3D.  Used to secure command buffers
        /// </summary>
        const IUnityGraphicsVulkan* graphicsInterface_;
        
        // Alias for graphicsInterface_->Instance().device
        VkDevice& device_;

        uint32_t graphicsQueueFamilyIndex_;
        uint32_t transferQueueFamilyIndex_;

        VkQueue graphicsQueue_;
        VkQueue transferQueue_;

        VkCommandPool graphicsCommandPool_;
        VkCommandPool transferCommandPool_;

        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties_;
        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties_;
        
        bool alreadyPrepared_;

        std::map<int, std::unique_ptr<RayTracerRenderTarget>> renderTargets_;

#pragma region SharedMeshMembers

        resourcePool<int, std::unique_ptr<RayTracerMeshSharedData>> sharedMeshesPool_;

#pragma endregion SharedMeshMembers

#pragma region MeshInstanceMembers

       // meshInstanceID -> Buffer that represents ShaderMeshInstanceData
       resourcePool<int, RayTracerMeshInstanceData> meshInstancePool_;

       std::vector<VkDescriptorBufferInfo> meshInstancesAttributesBufferInfos_;
       std::vector<VkDescriptorBufferInfo> meshInstancesFacesBufferInfos_;
       std::vector<VkDescriptorBufferInfo> meshInstancesDataBufferInfos_;
       
       // Buffer that represents VkAccelerationStructureInstanceKHR
       Vulkan::Buffer instancesAccelerationStructuresBuffer_;

       RayTracerAccelerationStructure tlas_;
       bool rebuildTlas_;
       bool updateTlas_;

#pragma endregion MeshInstanceMembers

#pragma region ShaderResources

       std::wstring shaderFolder_;
       
       Vulkan::ShaderBindingTable shaderBindingTable_;

       // Buffer that represents ShaderSceneParam
       Vulkan::Buffer sceneData_;
       VkDescriptorBufferInfo sceneBufferInfo_;

       Vulkan::Buffer noLight_;
       VkDescriptorBufferInfo noLightBufferInfo_;

       resourcePool<int, std::unique_ptr<Vulkan::Buffer>> lightsPool_;
       std::vector<VkDescriptorBufferInfo> lightsBufferInfos_;

       Vulkan::Image blankTexture_;
       
       resourcePool<int, std::unique_ptr<Vulkan::Image>> texturePool_;
       std::vector<VkDescriptorImageInfo> textureImageInfos_;

       Vulkan::Buffer defaultMaterial_;

       resourcePool<int, std::unique_ptr<Vulkan::Buffer>> materialPool_;
       std::vector<VkDescriptorBufferInfo> materialBufferInfos_;

       std::vector<VkDescriptorSetLayout> descriptorSetLayouts_;
       VkDescriptorPool                   descriptorPool_;

       std::vector<RayTracerGarbageBuffer> garbageBuffers_;
       
#pragma endregion ShaderResources

#pragma region PipelineResources

       VkPipelineLayout pipelineLayout_;
       VkPipeline pipeline_;

#pragma endregion PipelineResources

        RayTracer();    // Private for singleton

        /// <summary>
        /// Build a bottom level acceleration structure for an added shared mesh
        /// </summary>
        /// <param name="sharedMeshPoolIndex"></param>
        void BuildBlas(int sharedMeshInstanceId);

        void BuildTlas(VkCommandBuffer commandBuffer, uint64_t currentFrameNumber);

        static void UNITY_INTERFACE_API BuildAccelerationStructureQueueCallback(int eventId, void* data);
        void BuildAccelerationStructureInQueue(void* data);

        /// <summary>
        /// Create descriptor set layouts for shaders
        /// </summary>
        void CreateDescriptorSetsLayouts();

        /// <summary>
        /// Creates ray tracing pipeline layout
        /// </summary>
        void CreatePipelineLayout();

        /// <summary>
        /// Creates ray tracing pipeline
        /// </summary>
        void CreatePipeline();

        /// <summary>
        /// Builds and submits ray tracing commands
        /// </summary>
        void BuildAndSubmitRayTracingCommandBuffer(int cameraInstanceId, VkCommandBuffer commandBuffer, uint64_t currentFrameNumber);

        void CopyRenderToRenderTarget(int cameraInstanceId, VkCommandBuffer commandBuffer);

        /// <summary>
        /// Builds descriptor buffer infos for descriptor sets
        /// </summary>
        void BuildDescriptorBufferInfos(int cameraInstanceId, uint64_t currentFrameNumber);

        /// <summary>
        /// Create the descriptor pool for generating descriptor sets
        /// </summary>
        void CreateDescriptorPool();

        /// <summary>
        /// Update the descriptor sets for the shader
        /// </summary>
        void UpdateDescriptorSets(int cameraInstanceId, uint64_t currentFrameNumber);

        void UpdateMaterialsTextureIndices();
        void UpdateMaterialTextureIndices(ShaderMaterialData * const material);

        void GarbageCollect(uint64_t frameCount);
    };
}

