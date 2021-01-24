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

    struct RayTracerRenderTarget
    {
        RayTracerRenderTarget()
            : image(Vulkan::Image())
            , format(VK_FORMAT_UNDEFINED)
            , extent(VkExtent3D())
            , destination(nullptr)
        {}

        Vulkan::Image image;
        VkFormat format;
        VkExtent3D extent;
        void* destination;
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
            , faceDataIndex(-1)
            , vertexAttributeIndex(-1)
            , vertexCount(0)
            , indexCount(0)
        {}

        int sharedMeshInstanceId;

        int faceDataIndex;
        int vertexAttributeIndex;

        int vertexCount;
        int indexCount;

        Vulkan::Buffer vertexBuffer;          // Stores: vertex : vec3 
        Vulkan::Buffer indexBuffer;           // Stores: index : int

        RayTracerAccelerationStructure blas;
    };
   
    struct RayTracerMeshInstanceData
    {
        RayTracerMeshInstanceData()
            : sharedMeshIndex(-1)
            , localToWorld(mat4())
        {}

        int sharedMeshIndex;
        mat4 localToWorld;
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

        // Friend so hook functions can access
        friend void ResolvePropertiesAndQueues_RayTracer(VkPhysicalDevice physicalDevice);
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
        virtual int SetRenderTarget(int cameraInstanceId, int unityTextureFormat, int width, int height, void* textureHandle);
        virtual bool ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces);
        virtual int GetSharedMeshIndex(int sharedMeshInstanceId);
        virtual int AddSharedMesh(int instanceId, float* verticesArray, float* normalsArray, float* uvsArray, int vertexCount, int* indicesArray, int indexCount);
        virtual int AddTlasInstance(int sharedMeshIndex, float* l2wMatrix);
        virtual void RemoveTlasInstance(int meshInstanceIndex);
        virtual void BuildTlas();
        virtual void Prepare(int width, int height);
        virtual void UpdateCamera(float* camPos, float* camDir, float* camUp, float* camSide, float* camNearFarFov);
        virtual void UpdateSceneData(float* color);
        virtual void TraceRays();
        virtual void CopyImageToTexture(void* TextureHandle);
#pragma endregion RayTracerAPI


    private:
        /// <summary>
        /// Dummy null device for device_ member
        /// </summary>
        static VkDevice NullDevice;

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
        
        // camera.InstanceId => target
        std::map<int, std::unique_ptr<RayTracerRenderTarget>> renderTargets_;

#pragma region SharedMeshMembers
       resourcePool<std::unique_ptr<RayTracerMeshSharedData>> sharedMeshesPool_;

       // ShaderConstants -> ShaderVertexAttribute buffer
       resourcePool<Vulkan::Buffer> sharedMeshAttributesPool_;

       // SharedConstants -> ShaderFace
       resourcePool<Vulkan::Buffer> sharedMeshFacesPool_;
#pragma endregion SharedMeshMembers

#pragma region MeshInstanceMembers
       resourcePool<std::unique_ptr<RayTracerMeshInstanceData>> meshInstancePool_;
       Vulkan::Buffer instancesAccelerationStructuresBuffer_;

       RayTracerAccelerationStructure tlas_;
       bool rebuildTlas_;
       bool updateTlas_;
#pragma endregion MeshInstanceMembers



        RayTracer();    // Private for singleton

        /// <summary>
        /// Creates command pool for one off commands
        /// </summary>
        /// <param name="queueFamilyIndex"></param>
        void CreateCommandPool(uint32_t queueFamilyIndex, VkCommandPool& outCommandPool);

        /// <summary>
        /// Creates a command buffer that is intended for quick use and will be destroyed when flushed
        /// </summary>
        void CreateWorkerCommandBuffer(VkCommandBufferLevel level, const VkCommandPool& commandPool, VkCommandBuffer& outCommandBuffer);

        /// <summary>
        /// Submits a command buffer created by CreateWorkerCommandBuffer
        /// </summary>
        /// <param name="commandBuffer"></param>
        /// <param name="queue"></param>
        void SubmitWorkerCommandBuffer(VkCommandBuffer commandBuffer, const VkQueue& queue);

        /// <summary>
        /// Build a bottom level acceleration structure for an added shared mesh
        /// </summary>
        /// <param name="sharedMeshPoolIndex"></param>
        void BuildBlas(int sharedMeshPoolIndex);

    };
}
