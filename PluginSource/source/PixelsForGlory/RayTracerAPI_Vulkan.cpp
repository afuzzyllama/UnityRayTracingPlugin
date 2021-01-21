#include "RayTracerAPI.h"
#include "../PlatformBase.h"

#if SUPPORT_VULKAN

#include <string.h>
#include <map>
#include <vector>
#include <math.h>
#include <memory>

// This plugin does not link to the Vulkan loader, easier to support multiple APIs and systems that don't have Vulkan support
#define VK_NO_PROTOTYPES
#include "../Unity/IUnityGraphicsVulkan.h"

#include "ResourcePool.h"

#include "Debug.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"
#include "VulkanShader.h"
#include "VulkanShaderBindingTable.h"
#include "ShaderConstants.h"

// include volk.c for implementation
#include "volk.c"

static void LoadVulkanAPI(PFN_vkGetInstanceProcAddr getInstanceProcAddr, VkInstance instance)
{
    if (!vkGetInstanceProcAddr && getInstanceProcAddr)
    {
        vkGetInstanceProcAddr = getInstanceProcAddr;
    }

    if (!vkCreateInstance && vkGetInstanceProcAddr)
    {
        vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
    }
        
}

static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
    PFG_EDITORLOG("Hook_vkCreateInstance")
    vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
    VkResult result = vkCreateInstance(pCreateInfo, pAllocator, pInstance);
    VK_CHECK(result)
    if (result == VK_SUCCESS)
    {
        LoadVulkanAPI(vkGetInstanceProcAddr, *pInstance);
        volkLoadInstance(*pInstance);
    }

    return result;
}

static uint32_t s_GraphicsQueueFamilyIndex;
static uint32_t s_ComputeQueueFamilyIndex;
static uint32_t s_TransferQueueFamilyIndex;
static VkPhysicalDeviceRayTracingPipelinePropertiesKHR s_RayTracingProperties;
static void ResolvePropertiesAndQueues(VkPhysicalDevice physicalDevice) {

    // find our queues
    const VkQueueFlagBits askingFlags[3] = { VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT, VK_QUEUE_TRANSFER_BIT };
    uint32_t queuesIndices[3] = { ~0u, ~0u, ~0u };

    // Call once to get the count
    uint32_t queueFamilyPropertyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

    // Call again to populate vector
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

    // Locate which queueFamilyProperties index 
    for (size_t i = 0; i < 3; ++i) {
        const VkQueueFlagBits flag = askingFlags[i];
        uint32_t& queueIdx = queuesIndices[i];

        // Find the queue just for compute
        if (flag == VK_QUEUE_COMPUTE_BIT) {
            for (uint32_t j = 0; j < queueFamilyPropertyCount; ++j) {
                if ((queueFamilyProperties[j].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
                    !(queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                    queueIdx = j;
                    break;
                }
            }
        }

        // Find the queue just for transfer
        else if (flag == VK_QUEUE_TRANSFER_BIT) {
            for (uint32_t j = 0; j < queueFamilyPropertyCount; ++j) {
                if ((queueFamilyProperties[j].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                    !(queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
                    !(queueFamilyProperties[j].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                    queueIdx = j;
                    break;
                }
            }
        }

        // If an index wasn't return by the above, just find any queue that supports the flag
        if (queueIdx == ~0u) {
            for (uint32_t j = 0; j < queueFamilyPropertyCount; ++j) {
                if (queueFamilyProperties[j].queueFlags & flag) {
                    queueIdx = j;
                    break;
                }
            }
        }
    }

    if (queuesIndices[0] == ~0u || queuesIndices[1] == ~0u || queuesIndices[2] == ~0u) {
        PFG_EDITORLOGERROR("Could not find queues to support all required flags");
        return;
    }

    s_GraphicsQueueFamilyIndex = queuesIndices[0];
    s_ComputeQueueFamilyIndex = queuesIndices[1];
    s_TransferQueueFamilyIndex = queuesIndices[2];

    PFG_EDITORLOG("Queues indices successfully reoslved")

    // Get the ray tracing pipeline properties, which we'll need later on in the sample
    s_RayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

    VkPhysicalDeviceProperties2 physicalDeviceProperties = { };
    physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    physicalDeviceProperties.pNext = &s_RayTracingProperties;

    PFG_EDITORLOG("Getting physical device properties")
    vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties);
}

static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
    PFG_EDITORLOG("Hook_vkCreateDevice")
    ResolvePropertiesAndQueues(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
    const float priority = 0.0f;

    //  Setup device queue
    VkDeviceQueueCreateInfo deviceQueueCreateInfo;
    deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    deviceQueueCreateInfo.pNext = nullptr;
    deviceQueueCreateInfo.flags = 0;
    deviceQueueCreateInfo.queueFamilyIndex = s_GraphicsQueueFamilyIndex;
    deviceQueueCreateInfo.queueCount = 1;
    deviceQueueCreateInfo.pQueuePriorities = &priority;
    deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);

    // Determine if we have individual queues that need to be added 
    if (s_ComputeQueueFamilyIndex != s_GraphicsQueueFamilyIndex) {
        deviceQueueCreateInfo.queueFamilyIndex = s_ComputeQueueFamilyIndex;
        deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
    }
    if (s_TransferQueueFamilyIndex != s_GraphicsQueueFamilyIndex && s_TransferQueueFamilyIndex != s_ComputeQueueFamilyIndex) {
        deviceQueueCreateInfo.queueFamilyIndex = s_TransferQueueFamilyIndex;
        deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
    }

    // Setup required features backwards for chaining

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR physicalDeviceRayTracingPipelineFeatures = { };
    physicalDeviceRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    physicalDeviceRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
    physicalDeviceRayTracingPipelineFeatures.pNext = nullptr;

    VkPhysicalDeviceBufferDeviceAddressFeatures physicalDeviceBufferDeviceAddressFeatures = { };
    physicalDeviceBufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
    physicalDeviceBufferDeviceAddressFeatures.pNext = &physicalDeviceRayTracingPipelineFeatures;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR physicalDeviceAccelerationStructureFeatures = { };
    physicalDeviceAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    physicalDeviceAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
    physicalDeviceAccelerationStructureFeatures.pNext = &physicalDeviceBufferDeviceAddressFeatures;

    VkPhysicalDeviceFeatures2 deviceFeatures = { };
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.pNext = &physicalDeviceAccelerationStructureFeatures;

    vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures); // enable all the features our GPU has

    // Setup extensions required for ray tracing.  Rebuild from Unity
    std::vector<const char*> requiredExtensions = {
        // Required by Unity3D
        VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
        VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
        VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
        VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
        VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
        VK_KHR_MAINTENANCE1_EXTENSION_NAME,
        VK_KHR_MAINTENANCE2_EXTENSION_NAME,
        VK_KHR_MULTIVIEW_EXTENSION_NAME,
        VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME,
        VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME,
        VK_EXT_HDR_METADATA_EXTENSION_NAME,
        VK_EXT_SHADER_VIEWPORT_INDEX_LAYER_EXTENSION_NAME,
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,

        // Required by Raytracing
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,

        // Required by acceleration structure
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
    };

    for (auto const& ext : requiredExtensions) {
        PFG_EDITORLOG("Enabling extension: " + std::string(ext))
    }

    VkDeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pNext = &deviceFeatures;
    deviceCreateInfo.flags = pCreateInfo->flags;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size());
    deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
    deviceCreateInfo.enabledLayerCount = pCreateInfo->enabledLayerCount;
    deviceCreateInfo.ppEnabledLayerNames = pCreateInfo->ppEnabledLayerNames;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();
    deviceCreateInfo.pEnabledFeatures = nullptr; // Must be null since pNext != nullptr

    VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, pAllocator, pDevice);
    VK_CHECK(result)

    if (result == VK_SUCCESS)
    {
        PFG_EDITORLOG("Hooked into vkCreateDevice successfully");
    }
    else
    {
        PFG_EDITORLOGERROR("Hooked into vkCreateDevice unsuccessfully\n");
    }

    return result;
}

static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL Hook_vkGetInstanceProcAddr(VkInstance device, const char* funcName)
{
    if (!funcName)
        return nullptr;

#define INTERCEPT(fn) if (strcmp(funcName, #fn) == 0) return (PFN_vkVoidFunction)&Hook_##fn
    INTERCEPT(vkCreateInstance);
    INTERCEPT(vkCreateDevice);
#undef INTERCEPT

    return nullptr;
}

static PFN_vkGetInstanceProcAddr UNITY_INTERFACE_API InterceptVulkanInitialization(PFN_vkGetInstanceProcAddr getInstanceProcAddr, void*)
{
    vkGetInstanceProcAddr = getInstanceProcAddr;
    return Hook_vkGetInstanceProcAddr;
}

extern "C" void RenderAPI_Vulkan_OnPluginLoad(IUnityInterfaces * interfaces)
{
    interfaces->Get<IUnityGraphicsVulkan>()->InterceptInitialization(InterceptVulkanInitialization, NULL);
}

namespace PixelsForGlory
{
    struct RayTracerVulkanInstance
    {
        RayTracerVulkanInstance()
            : instance(VK_NULL_HANDLE)
            , physicalDevice(VK_NULL_HANDLE)
            , physicalDeviceMemoryProperties(VkPhysicalDeviceMemoryProperties())
            , device(VK_NULL_HANDLE)
            , graphicsQueueFamilyIndex(0)
            , computeQueueFamilyIndex(0)
            , transferQueueFamilyIndex(0)
            , graphicsQueue(VK_NULL_HANDLE)
            , computeQueue(VK_NULL_HANDLE)
            , transferQueue(VK_NULL_HANDLE)
            , rayTracingProperties(VkPhysicalDeviceRayTracingPipelinePropertiesKHR())
            , commandPool(VK_NULL_HANDLE)
        {}

        ~RayTracerVulkanInstance()
        {
            if (commandPool != VK_NULL_HANDLE)
            {
                vkDestroyCommandPool(device, commandPool, nullptr);
                commandPool = VK_NULL_HANDLE;
            }
        }
        
        void GetQueuesAndProperties()
        {
            // Assign indices resolved during device creation
            computeQueueFamilyIndex = s_ComputeQueueFamilyIndex;
            graphicsQueueFamilyIndex = s_GraphicsQueueFamilyIndex;
            transferQueueFamilyIndex = s_TransferQueueFamilyIndex;
            rayTracingProperties = s_RayTracingProperties;

            // Get memory properties
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);

            // get our queues handles with our new logical device!
            PFG_EDITORLOG("Getting device queues")
            vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
            vkGetDeviceQueue(device, computeQueueFamilyIndex, 0, &computeQueue);
            vkGetDeviceQueue(device, transferQueueFamilyIndex, 0, &transferQueue);
        }

        void CreateCommandPool(uint32_t queueFamilyIndex)
        {
            VkCommandPoolCreateInfo commandPoolCreateInfo = {};
            commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            
            VK_CHECK(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool))
        }

        VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level, bool begin)
        {
            VkCommandBufferAllocateInfo cmd_buf_allocate_info{};
            cmd_buf_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmd_buf_allocate_info.commandPool = commandPool;
            cmd_buf_allocate_info.level = level;
            cmd_buf_allocate_info.commandBufferCount = 1;

            VkCommandBuffer command_buffer;
            VK_CHECK(vkAllocateCommandBuffers(device, &cmd_buf_allocate_info, &command_buffer))

            // If requested, also start recording for the new command buffer
            if (begin)
            {
                VkCommandBufferBeginInfo commandBufferBeginInfo = { };
                commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                VK_CHECK(vkBeginCommandBuffer(command_buffer, &commandBufferBeginInfo))
            }

            return command_buffer;
        }

        void EndCommandBuffer(VkCommandBuffer commandBuffer)
        {
            VK_CHECK(vkEndCommandBuffer(commandBuffer))
        }

        void SubmitCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue)
        {
            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            /*if (signalSemaphore)
            {
                submit_info.pSignalSemaphores = &signalSemaphore;
                submit_info.signalSemaphoreCount = 1;
            }*/

            // Create fence to ensure that the command buffer has finished executing
            VkFenceCreateInfo fenceCreateInfo = { };
            fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceCreateInfo.flags = 0;

            VkFence fence;
            VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence))

                // Submit to the queue
            VkResult result = vkQueueSubmit(queue, 1, &submitInfo, fence);
            VK_CHECK(result)

                // Wait for the fence to signal that command buffer has finished executing
            const uint64_t DEFAULT_FENCE_TIMEOUT = 100000000000;
            VK_CHECK(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT))

            vkDestroyFence(device, fence, nullptr);
        }

        void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
        {
            if (commandBuffer == VK_NULL_HANDLE)
            {
                return;
            }

            VK_CHECK(vkEndCommandBuffer(commandBuffer))

            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;
            
            /*if (signalSemaphore)
            {
                submit_info.pSignalSemaphores = &signalSemaphore;
                submit_info.signalSemaphoreCount = 1;
            }*/

            // Create fence to ensure that the command buffer has finished executing
            VkFenceCreateInfo fenceCreateInfo = { };
            fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceCreateInfo.flags = 0;

            VkFence fence;
            VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &fence))

            // Submit to the queue
            VkResult result = vkQueueSubmit(queue, 1, &submitInfo, fence);
            VK_CHECK(result)

            // Wait for the fence to signal that command buffer has finished executing
            const uint64_t DEFAULT_FENCE_TIMEOUT = 100000000000;
            VK_CHECK(vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT))

            vkDestroyFence(device, fence, nullptr);

            if (commandPool != VK_NULL_HANDLE && free)
            {
                vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
            }
        }

        VkInstance instance;
        VkPhysicalDevice physicalDevice;
        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
        VkDevice device;

        uint32_t graphicsQueueFamilyIndex;
        uint32_t computeQueueFamilyIndex;
        uint32_t transferQueueFamilyIndex;

        VkQueue graphicsQueue;
        VkQueue computeQueue;
        VkQueue transferQueue;

        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties;

        VkCommandPool commandPool;

    };

    struct RayTracerAccelerationStructure
    {
        RayTracerAccelerationStructure()
            : accelerationStructure(VK_NULL_HANDLE)
            , deviceAddress(0)
            , buffer(VulkanBuffer())
        {}

        VkAccelerationStructureKHR    accelerationStructure;
        uint64_t                      deviceAddress;
        VulkanBuffer                  buffer;
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

        VulkanBuffer vertexBuffer;          // Stores: vertex : vec3 
        VulkanBuffer indexBuffer;           // Stores: index : int

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

    class RayTracerAPI_Vulkan : public RayTracerAPI
    {
    public:
        RayTracerAPI_Vulkan();
        virtual ~RayTracerAPI_Vulkan() { }

        virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces);
        virtual int GetSharedMeshIndex(int sharedMeshInstanceId);
        virtual int AddMesh(int sharedMeshInstanceId, float* vertices, float* normals, float* uvs, int vertexCount, int* indices, int indexCount);
        virtual int AddTlasInstance(int sharedMeshIndex, float* l2wMatrix);
        virtual void BuildTlas();
        virtual void Prepare(int width, int height);
        virtual void TraceRays();

        virtual void UpdateCamera(float* camPos, float* camDir, float* camUp, float* camSide, float* camNearFarFov);
        virtual void UpdateSceneData(float* color);

       

    private:
        IUnityGraphicsVulkan* graphicsInterface_;
        RayTracerVulkanInstance rayTracerVulkanInstance_;
       
        std::unique_ptr<VulkanImage> offscreenImage_;
        VkFormat offscreenSurfaceFormat_;

        VulkanBuffer sceneData_;
        VkDescriptorBufferInfo sceneBufferInfo_;

        VulkanBuffer cameraData_;
        VkDescriptorBufferInfo cameraBufferInfo_;

        PixelsForGlory::resourcePool<std::unique_ptr<RayTracerMeshSharedData>> sharedMeshesPool_;
        PixelsForGlory::resourcePool<std::unique_ptr<RayTracerMeshInstanceData>> meshInstancePool_;

        PixelsForGlory::resourcePool<VulkanBuffer> sharedMeshAttributesPool_;
        PixelsForGlory::resourcePool<VulkanBuffer> sharedMeshFacesPool_;

        std::vector<VkDescriptorBufferInfo> sharedMeshAttributesBufferInfos_;
        std::vector<VkDescriptorBufferInfo> sharedMeshFacesBufferInfos_;
                
        RayTracerAccelerationStructure tlas_;

        std::vector<VkDescriptorSetLayout> descriptorSetLayouts_;
        VkDescriptorPool                   descriptorPool_;
        std::vector<VkDescriptorSet>       descriptorSets_;

        VkPipelineLayout pipelineLayout_;
        VkPipeline pipeline_;

        VkCommandBuffer commandBuffer_;

        std::string shadersFolder_;
        VulkanShaderBindingTable shaderBindingTable_;

        uint32_t width_;
        uint32_t height_;

        //std::map<unsigned long long, VulkanBuffers> m_DeleteQueue;

        void Destroy();

        bool CreateOffscreenImage();
        void BuildBlas(uint32_t sharedMeshIndex);
        void CreateDescriptorSetsLayouts();
        void CreatePipeline();
        void CreateDescriptorPool();
        void CreateCommandBuffer();
        void BuildDescriptorBufferInfos();
        void UpdateDescriptorSets();
        void BuildCommandBuffer();

        
        VkDeviceOrHostAddressKHR GetBufferDeviceAddress(const VulkanBuffer& buffer);
        VkDeviceOrHostAddressConstKHR GetBufferDeviceAddressConst(const VulkanBuffer& buffer);
    };

    RayTracerAPI* CreateRayTracerAPI_Vulkan();
}

PixelsForGlory::RayTracerAPI* PixelsForGlory::CreateRayTracerAPI_Vulkan()
{
    return new PixelsForGlory::RayTracerAPI_Vulkan();
}

PixelsForGlory::RayTracerAPI_Vulkan::RayTracerAPI_Vulkan()
    : graphicsInterface_(nullptr)
    , rayTracerVulkanInstance_()
    , width_(800)
    , height_(600)
    , offscreenSurfaceFormat_(VK_FORMAT_B8G8R8A8_UNORM)
    , sceneBufferInfo_(VkDescriptorBufferInfo())
    , cameraBufferInfo_(VkDescriptorBufferInfo())
    , commandBuffer_(VK_NULL_HANDLE)
    , descriptorPool_(VK_NULL_HANDLE)
    , pipelineLayout_(VK_NULL_HANDLE)
    , pipeline_(VK_NULL_HANDLE)
    , shadersFolder_("C:\\Users\\afuzzyllama\\Development\\Unity3d\\RayTracing\\UnityProject\\Assets\\Plugins\\x86_64\\")
{}

void PixelsForGlory::RayTracerAPI_Vulkan::ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces)
{
    switch (type)
    {
    case kUnityGfxDeviceEventInitialize:
        PFG_EDITORLOG("Processing kUnityGfxDeviceEventInitialize...");

        graphicsInterface_ = interfaces->Get<IUnityGraphicsVulkan>();
        
        // Copy values from Unity's Vulkan instance to our own instance
        auto unityVulkanInstance = graphicsInterface_->Instance();
        rayTracerVulkanInstance_.instance = unityVulkanInstance.instance;
        rayTracerVulkanInstance_.physicalDevice = unityVulkanInstance.physicalDevice;
        rayTracerVulkanInstance_.device = unityVulkanInstance.device;
        rayTracerVulkanInstance_.GetQueuesAndProperties();
        rayTracerVulkanInstance_.CreateCommandPool(rayTracerVulkanInstance_.graphicsQueueFamilyIndex);
                
        UnityVulkanPluginEventConfig eventConfig;
        eventConfig.graphicsQueueAccess = kUnityVulkanGraphicsQueueAccess_Allow;
        eventConfig.renderPassPrecondition = kUnityVulkanRenderPass_EnsureOutside;
        eventConfig.flags = kUnityVulkanEventConfigFlag_EnsurePreviousFrameSubmission | kUnityVulkanEventConfigFlag_ModifiesCommandBuffersState;
        graphicsInterface_->ConfigureEvent(1, &eventConfig);

        // alternative way to intercept API
        //graphicsInterface_->InterceptVulkanAPI("vkCmdBeginRenderPass", (PFN_vkVoidFunction)Hook_vkCmdBeginRenderPass);
        break;
    case kUnityGfxDeviceEventShutdown:
        PFG_EDITORLOG("Processing kUnityGfxDeviceEventShutdown...");
        Destroy();

        graphicsInterface_ = nullptr;
        rayTracerVulkanInstance_ = RayTracerVulkanInstance();

        break;
    }
}

void PixelsForGlory::RayTracerAPI_Vulkan::Destroy()
{
    if (offscreenImage_ )
    {
        offscreenImage_->Destroy();
        offscreenImage_.release();
        offscreenImage_ = nullptr;
    }

    // Destory all added mesh buffers
    for (auto i = sharedMeshesPool_.pool_begin(); i != sharedMeshesPool_.pool_end(); ++i)
    {
        auto mesh = (*i).get();

        mesh->vertexBuffer.Destroy();
        mesh->indexBuffer.Destroy();

        if (mesh->blas.accelerationStructure != VK_NULL_HANDLE)
        {
            mesh->blas.buffer.Destroy();
            vkDestroyAccelerationStructureKHR(rayTracerVulkanInstance_.device, mesh->blas.accelerationStructure, nullptr);
        }
    }

    for (auto i = sharedMeshAttributesPool_.pool_begin(); i != sharedMeshAttributesPool_.pool_end(); ++i)
    {
        (*i).Destroy();
    }

    for (auto i = sharedMeshFacesPool_.pool_begin(); i != sharedMeshFacesPool_.pool_end(); ++i)
    {
        (*i).Destroy();
    }

    if (tlas_.accelerationStructure != VK_NULL_HANDLE)
    {
        tlas_.buffer.Destroy();
        vkDestroyAccelerationStructureKHR(rayTracerVulkanInstance_.device, tlas_.accelerationStructure, nullptr);
    }

    if (descriptorPool_ != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(rayTracerVulkanInstance_.device, descriptorPool_, nullptr);
        descriptorPool_ = VK_NULL_HANDLE;
    }

    shaderBindingTable_.Destroy();

    if (pipeline_ != VK_NULL_HANDLE) {
        vkDestroyPipeline(rayTracerVulkanInstance_.device, pipeline_, nullptr);
        pipeline_ = VK_NULL_HANDLE;
    }

    if (pipelineLayout_ != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(rayTracerVulkanInstance_.device, pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
    }

    for (auto descriptorSetLayout : descriptorSetLayouts_)
    {
        vkDestroyDescriptorSetLayout(rayTracerVulkanInstance_.device, descriptorSetLayout, nullptr);
    }
    descriptorSetLayouts_.clear();
}

int PixelsForGlory::RayTracerAPI_Vulkan::GetSharedMeshIndex(int sharedMeshInstanceId)
{
    for (auto itr = sharedMeshesPool_.in_use_begin(); itr != sharedMeshesPool_.in_use_end(); ++itr)
    {
        auto i = (*itr);
        if (sharedMeshesPool_[i]->sharedMeshInstanceId == sharedMeshInstanceId)
        {
            return i;
        }
    }
    
    return -1;
}

int PixelsForGlory::RayTracerAPI_Vulkan::AddMesh(int sharedMeshInstanceId, float* verticesArray, float* normalsArray, float* uvsArray, int vertexCount, int* indicesArray, int indexCount)
{
    for (auto itr = sharedMeshesPool_.in_use_begin(); itr != sharedMeshesPool_.in_use_end(); ++itr)
    {
        auto i = (*itr);
        if (sharedMeshesPool_[i]->sharedMeshInstanceId == sharedMeshInstanceId)
        {
            return i;
        }
    }
    
    assert(indexCount % 3 == 0);

    auto sentMesh = std::make_unique<RayTracerMeshSharedData>();
    
    
    // RayTracerMeshSharedData
    sentMesh->sharedMeshInstanceId = sharedMeshInstanceId;
    sentMesh->vertexCount = vertexCount;
    sentMesh->indexCount = indexCount;

    sentMesh->vertexAttributeIndex = sharedMeshAttributesPool_.get_next_index();
    sentMesh->faceDataIndex = sharedMeshFacesPool_.get_next_index();

    VulkanBuffer& sentMeshAttributes = sharedMeshAttributesPool_[sentMesh->vertexAttributeIndex];
    VulkanBuffer& sentMeshFaces = sharedMeshFacesPool_[sentMesh->faceDataIndex];

    bool success = true;
    if (sentMesh->vertexBuffer.Create(
            rayTracerVulkanInstance_.device,
            rayTracerVulkanInstance_.physicalDeviceMemoryProperties,
            sizeof(vec3) * vertexCount,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VulkanBuffer::kDefaultMemoryPropertyFlags) 
        != VK_SUCCESS)
    {
        Debug::LogError("Failed to create vertex buffer for shared mesh instance id " + std::to_string(sharedMeshInstanceId));
        success = false;
    }

    if (sentMesh->indexBuffer.Create(
        rayTracerVulkanInstance_.device,
        rayTracerVulkanInstance_.physicalDeviceMemoryProperties,
        sizeof(int) * indexCount,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VulkanBuffer::kDefaultMemoryPropertyFlags))
    {
        Debug::LogError("Failed to create index buffer for shared mesh instance id " + std::to_string(sharedMeshInstanceId));
        success = false;
    }

    if (sentMeshAttributes.Create(
            rayTracerVulkanInstance_.device,
            rayTracerVulkanInstance_.physicalDeviceMemoryProperties,
            sizeof(ShaderVertexAttribute) * vertexCount,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VulkanBuffer::kDefaultMemoryPropertyFlags)
        != VK_SUCCESS)
    {
        Debug::LogError("Failed to create vertex attribute buffer for shared mesh instance id " + std::to_string(sharedMeshInstanceId));
        success = false;
    }


    if (sentMeshFaces.Create(
        rayTracerVulkanInstance_.device,
        rayTracerVulkanInstance_.physicalDeviceMemoryProperties,
        sizeof(ShaderFace) * vertexCount / 3,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VulkanBuffer::kDefaultMemoryPropertyFlags)
        != VK_SUCCESS)
    {
        Debug::LogError("Failed to create face buffer for shared mesh instance id " + std::to_string(sharedMeshInstanceId));
        success = false;
    }

    
    if (!success)
    {
        return -1;
    }

    auto vertices = reinterpret_cast<vec3*>(sentMesh->vertexBuffer.Map());
    auto indices = reinterpret_cast<int*>(sentMesh->indexBuffer.Map());

    auto vertexAttributes = reinterpret_cast<ShaderVertexAttribute*>(sentMeshAttributes.Map());

    auto faces = reinterpret_cast<ShaderFace*>(sentMeshFaces.Map());
    
    // verticesArray and normalsArray are size vertexCount * 3 since they actually represent an array of vec3
    // uvsArray is size vertexCount * 2 since it actually represents an array of vec2
    for (int i = 0; i < vertexCount; ++i)
    {
        // Build for acceleration structure
        vertices[i].x = verticesArray[3 * i + 0];
        vertices[i].y = verticesArray[3 * i + 1];
        vertices[i].z = verticesArray[3 * i + 2];

        // Build for shader
        vertexAttributes[i].normal.x = normalsArray[3 * i + 0];
        vertexAttributes[i].normal.y = normalsArray[3 * i + 1];
        vertexAttributes[i].normal.z = normalsArray[3 * i + 2];

        vertexAttributes[i].uv.x = uvsArray[2 * i + 0];
        vertexAttributes[i].uv.y = uvsArray[2 * i + 1];
    }
    
    
    for (int i = 0; i < indexCount / 3; ++i)
    {
        // Build for acceleration structure
        indices[i + 0] = indicesArray[i + 0];
        indices[i + 1] = indicesArray[i + 1];
        indices[i + 2] = indicesArray[i + 2];

        // Build for shader
        faces[i].index0 = indicesArray[i + 0];
        faces[i].index1 = indicesArray[i + 1];
        faces[i].index2 = indicesArray[i + 2];
    }
    
    sentMesh->vertexBuffer.Unmap();
    sentMesh->indexBuffer.Unmap();
    
    sentMeshAttributes.Unmap();

    sentMeshFaces.Unmap();

    int sharedMeshIndex = sharedMeshesPool_.add(std::move(sentMesh));

    BuildBlas(sharedMeshIndex);

    Debug::Log("Added mesh (sharedMeshInstanceId: " + std::to_string(sharedMeshInstanceId) + ")");

    return sharedMeshIndex;

}

bool PixelsForGlory::RayTracerAPI_Vulkan::CreateOffscreenImage()
{
    const VkExtent3D extent = { width_, height_, 1 };

    offscreenImage_ = std::make_unique<VulkanImage>(rayTracerVulkanInstance_.device, 
                                                    rayTracerVulkanInstance_.physicalDeviceMemoryProperties, 
                                                    rayTracerVulkanInstance_.commandPool, 
                                                    rayTracerVulkanInstance_.transferQueue);

    if (offscreenImage_->Create(
        VK_IMAGE_TYPE_2D,
        offscreenSurfaceFormat_,
        extent,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != VK_SUCCESS) {

        Debug::LogError("Failed to create offscreen image!");
        return false;
    }

    VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    if (offscreenImage_->CreateImageView(VK_IMAGE_VIEW_TYPE_2D, offscreenSurfaceFormat_, range) != VK_SUCCESS) {
        Debug::LogError("Failed to create offscreen image view!");
        return false;
    }

    return true;
}

void PixelsForGlory::RayTracerAPI_Vulkan::BuildBlas(uint32_t sharedMeshIndex)
{
    // Create buffers for the bottom level geometry
    
    // Setup a single transformation matrix that can be used to transform the whole geometry for a single bottom level acceleration structure
    VkTransformMatrixKHR transformMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f };
        
    VulkanBuffer transformBuffer;
    transformBuffer.Create(
        rayTracerVulkanInstance_.device, 
        rayTracerVulkanInstance_.physicalDeviceMemoryProperties, 
        sizeof(transformMatrix), 
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VulkanBuffer::kDefaultMemoryPropertyFlags);

    auto transform = reinterpret_cast<VkTransformMatrixKHR*>(transformBuffer.Map());
    *transform = transformMatrix;
    transformBuffer.Unmap();

    // The bottom level acceleration structure contains one set of triangles as the input geometry
    VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    
    accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
    accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    accelerationStructureGeometry.geometry.triangles.vertexData = GetBufferDeviceAddressConst(sharedMeshesPool_[sharedMeshIndex]->vertexBuffer);
    accelerationStructureGeometry.geometry.triangles.maxVertex = sharedMeshesPool_[sharedMeshIndex]->vertexCount;
    accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(vec3);
    accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    accelerationStructureGeometry.geometry.triangles.indexData = GetBufferDeviceAddressConst(sharedMeshesPool_[sharedMeshIndex]->indexBuffer);
    accelerationStructureGeometry.geometry.triangles.transformData = GetBufferDeviceAddressConst(transformBuffer);
    accelerationStructureGeometry.geometry.triangles.pNext = nullptr;
    
    // Get the size requirements for buffers involved in the acceleration structure build process
    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {};
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationStructureBuildGeometryInfo.geometryCount = 1;
    accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

    // Number of triangles 
    const uint32_t primitiveCount = sharedMeshesPool_[sharedMeshIndex]->indexCount / 3;

    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(
        rayTracerVulkanInstance_.device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &accelerationStructureBuildGeometryInfo,
        &primitiveCount,
        &accelerationStructureBuildSizesInfo);

    // Create a buffer to hold the acceleration structure
    sharedMeshesPool_[sharedMeshIndex]->blas.buffer.Create(
        rayTracerVulkanInstance_.device,
        rayTracerVulkanInstance_.physicalDeviceMemoryProperties,
        accelerationStructureBuildSizesInfo.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
        VulkanBuffer::kDefaultMemoryPropertyFlags);
    
    // Create the acceleration structure
    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = sharedMeshesPool_[sharedMeshIndex]->blas.buffer.GetBuffer();
    accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    VK_CHECK(vkCreateAccelerationStructureKHR(rayTracerVulkanInstance_.device , &accelerationStructureCreateInfo, nullptr, &sharedMeshesPool_[sharedMeshIndex]->blas.accelerationStructure))

    // The actual build process starts here
    // Create a scratch buffer as a temporary storage for the acceleration structure build
    VulkanBuffer scratchBuffer;
    scratchBuffer.Create(
        rayTracerVulkanInstance_.device, 
        rayTracerVulkanInstance_.physicalDeviceMemoryProperties, 
        accelerationStructureBuildSizesInfo.buildScratchSize, 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo = { };
    accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationBuildGeometryInfo.dstAccelerationStructure = sharedMeshesPool_[sharedMeshIndex]->blas.accelerationStructure;
    accelerationBuildGeometryInfo.geometryCount = 1;
    accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
    accelerationBuildGeometryInfo.scratchData = GetBufferDeviceAddress(scratchBuffer);

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo = { };
    accelerationStructureBuildRangeInfo.primitiveCount = 1;
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationStructureBuildRangeInfos = { &accelerationStructureBuildRangeInfo };

    // Build the acceleration structure on the device via a one-time command buffer submission
    // Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
    VkCommandBuffer commandBuffer = rayTracerVulkanInstance_.CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    vkCmdBuildAccelerationStructuresKHR(
        commandBuffer,
        1,
        &accelerationBuildGeometryInfo,
        accelerationStructureBuildRangeInfos.data());
    rayTracerVulkanInstance_.FlushCommandBuffer(commandBuffer, rayTracerVulkanInstance_.graphicsQueue, true);

    scratchBuffer.Destroy();

    // Get the bottom acceleration structure's handle, which will be used during the top level acceleration build
    VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo{};
    accelerationStructureDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationStructureDeviceAddressInfo.accelerationStructure = sharedMeshesPool_[sharedMeshIndex]->blas.accelerationStructure;
    sharedMeshesPool_[sharedMeshIndex]->blas.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(rayTracerVulkanInstance_.device, &accelerationStructureDeviceAddressInfo);

    Debug::Log("Built blas for mesh (sharedMeshInstanceId: " + std::to_string(sharedMeshesPool_[sharedMeshIndex]->sharedMeshInstanceId) + ")");

}

int PixelsForGlory::RayTracerAPI_Vulkan::AddTlasInstance(int sharedMeshIndex, float* l2wMatrix)
{
    auto instance = std::make_unique<RayTracerMeshInstanceData>();

    instance->sharedMeshIndex = sharedMeshIndex;
    FloatArrayToMatrix(l2wMatrix, instance->localToWorld);

    int index = meshInstancePool_.add(std::move(instance));

    Debug::Log("Added mesh instance (sharedMeshIndex: " + std::to_string(sharedMeshIndex) + ")");

    return index;
}

void PixelsForGlory::RayTracerAPI_Vulkan::BuildTlas()
{
    std::vector<VkAccelerationStructureInstanceKHR> instances(meshInstancePool_.in_use_size(), VkAccelerationStructureInstanceKHR{});
    std::vector<uint32_t> uniqueSharedMeshIndices;

    for(auto i = meshInstancePool_.in_use_begin(); i != meshInstancePool_.in_use_end(); ++i)
    {
        auto instanceIndex = (*i);

        const auto& t = meshInstancePool_[instanceIndex]->localToWorld;
        VkTransformMatrixKHR transform_matrix = {
            t[0][0], t[0][1], t[0][2], t[0][3],
            t[1][0], t[1][1], t[1][2], t[1][3],
            t[2][0], t[2][1], t[2][2], t[2][3]
        };

        auto sharedMeshIndex = meshInstancePool_[instanceIndex]->sharedMeshIndex;

        VkAccelerationStructureInstanceKHR accelerationStructureInstance = { };
        accelerationStructureInstance.transform = transform_matrix;
        accelerationStructureInstance.instanceCustomIndex = instanceIndex;
        accelerationStructureInstance.mask = 0xFF;
        accelerationStructureInstance.instanceShaderBindingTableRecordOffset = 0;
        accelerationStructureInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        accelerationStructureInstance.accelerationStructureReference = sharedMeshesPool_[meshInstancePool_[instanceIndex]->sharedMeshIndex]->blas.deviceAddress;

        if (std::find(uniqueSharedMeshIndices.begin(), uniqueSharedMeshIndices.end(), sharedMeshIndex) == uniqueSharedMeshIndices.end())
        {
            uniqueSharedMeshIndices.push_back(sharedMeshIndex);
        }
        
    }

    VulkanBuffer instancesBuffer;
    instancesBuffer.Create(
        rayTracerVulkanInstance_.device,
        rayTracerVulkanInstance_.physicalDeviceMemoryProperties,
        instances.size() * sizeof(VkAccelerationStructureInstanceKHR),
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VulkanBuffer::kDefaultMemoryPropertyFlags);
    instancesBuffer.UploadData(instances.data(), instancesBuffer.GetSize());

    // The top level acceleration structure contains (bottom level) instance as the input geometry
    VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    accelerationStructureGeometry.geometry.instances.data = GetBufferDeviceAddressConst(instancesBuffer);

    // Get the size requirements for buffers involved in the acceleration structure build process
    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {};
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationStructureBuildGeometryInfo.geometryCount = 1;
    accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

    // Number of acceleration structures
    const uint32_t primitiveCount = static_cast<uint32_t>(uniqueSharedMeshIndices.size());

    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(
        rayTracerVulkanInstance_.device, 
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &accelerationStructureBuildGeometryInfo,
        &primitiveCount,
        &accelerationStructureBuildSizesInfo);

    // Create a buffer to hold the acceleration structure
    tlas_.buffer.Create(
        rayTracerVulkanInstance_.device,
        rayTracerVulkanInstance_.physicalDeviceMemoryProperties,
        accelerationStructureBuildSizesInfo.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
        VulkanBuffer::kDefaultMemoryPropertyFlags);

    // Create the acceleration structure
    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = tlas_.buffer.GetBuffer();
    accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    VK_CHECK(vkCreateAccelerationStructureKHR(rayTracerVulkanInstance_.device, &accelerationStructureCreateInfo, nullptr, &tlas_.accelerationStructure))

    // The actual build process starts here

    // Create a scratch buffer as a temporary storage for the acceleration structure build
    VulkanBuffer scratchBuffer;
    scratchBuffer.Create(
        rayTracerVulkanInstance_.device,
        rayTracerVulkanInstance_.physicalDeviceMemoryProperties,
        accelerationStructureBuildSizesInfo.buildScratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo = {};
    accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationBuildGeometryInfo.dstAccelerationStructure = tlas_.accelerationStructure;
    accelerationBuildGeometryInfo.geometryCount = 1;
    accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
    accelerationBuildGeometryInfo.scratchData = GetBufferDeviceAddress(scratchBuffer);

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo;
    accelerationStructureBuildRangeInfo.primitiveCount = 1;
    accelerationStructureBuildRangeInfo.primitiveOffset = 0;
    accelerationStructureBuildRangeInfo.firstVertex = 0;
    accelerationStructureBuildRangeInfo.transformOffset = 0;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationStructureBuildRangeInfos = { &accelerationStructureBuildRangeInfo };

    // Build the acceleration structure on the device via a one-time command buffer submission
    // Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
    VkCommandBuffer commandBuffer = rayTracerVulkanInstance_.CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
    vkCmdBuildAccelerationStructuresKHR(
        commandBuffer,
        1,
        &accelerationBuildGeometryInfo,
        accelerationStructureBuildRangeInfos.data());
    rayTracerVulkanInstance_.FlushCommandBuffer(commandBuffer, rayTracerVulkanInstance_.graphicsQueue, true);

    scratchBuffer.Destroy();

    // Get the top acceleration structure's handle, which will be used to setup it's descriptor
    VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo = {};
    accelerationStructureDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
    accelerationStructureDeviceAddressInfo.accelerationStructure = tlas_.accelerationStructure;
    tlas_.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(rayTracerVulkanInstance_.device, &accelerationStructureDeviceAddressInfo);

    Debug::Log("Succesfully built tlas");
}

void PixelsForGlory::RayTracerAPI_Vulkan::Prepare(int width, int height)
{
    width_ = width;
    height_ = height;
    CreateOffscreenImage();
    CreateDescriptorSetsLayouts();
    CreatePipeline();
    BuildDescriptorBufferInfos();
    CreateDescriptorPool();
    UpdateDescriptorSets();
    BuildCommandBuffer();
}

void PixelsForGlory::RayTracerAPI_Vulkan::TraceRays()
{
    rayTracerVulkanInstance_.SubmitCommandBuffer(commandBuffer_, rayTracerVulkanInstance_.graphicsQueue);
}

void PixelsForGlory::RayTracerAPI_Vulkan::UpdateCamera(float* camPos, float* camDir, float* camUp, float* camSide, float* camNearFarFov)
{
    if (cameraData_.GetBuffer() == VK_NULL_HANDLE)
    {
        cameraData_.Create(
            rayTracerVulkanInstance_.device,
            rayTracerVulkanInstance_.physicalDeviceMemoryProperties,
            sizeof(ShaderCameraParam),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VulkanBuffer::kDefaultMemoryPropertyFlags);
    }

    auto camera = reinterpret_cast<ShaderCameraParam*>(cameraData_.Map());
    camera->camPos.x = camPos[0];
    camera->camPos.y = camPos[1];
    camera->camPos.z = camPos[2];

    camera->camDir.x = camDir[0];
    camera->camDir.y = camDir[1];
    camera->camDir.z = camDir[2];

    camera->camUp.x = camUp[0];
    camera->camUp.y = camUp[1];
    camera->camUp.z = camUp[2];

    camera->camSide.x = camSide[0];
    camera->camSide.y = camSide[1];
    camera->camSide.z = camSide[2];

    camera->camNearFarFov.x = camNearFarFov[0];
    camera->camNearFarFov.y = camNearFarFov[1];
    camera->camNearFarFov.z = camNearFarFov[2];

    cameraData_.Unmap();
}

void PixelsForGlory::RayTracerAPI_Vulkan::UpdateSceneData(float* color)
{
    if (sceneData_.GetBuffer() == VK_NULL_HANDLE)
    {
        sceneData_.Create(
            rayTracerVulkanInstance_.device,
            rayTracerVulkanInstance_.physicalDeviceMemoryProperties,
            sizeof(ShaderSceneParam),
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VulkanBuffer::kDefaultMemoryPropertyFlags);
    }

    auto scene = reinterpret_cast<ShaderSceneParam*>(sceneData_.Map());
    scene->ambient.r = color[0];
    scene->ambient.g = color[1];
    scene->ambient.b = color[2];
    scene->ambient.a = color[3];
}

void PixelsForGlory::RayTracerAPI_Vulkan::CreateDescriptorSetsLayouts() {

    // Create descriptor sets for the shader.  This setups up how data is bound to GPU memory and what shader stages will have access to what memory
    //const uint32_t numMeshes = static_cast<uint32_t>(sharedMeshesPool_.pool_size());
    //const uint32_t numMaterials = static_cast<uint32_t>(_scene.materials.size());
    //const uint32_t numTextures = static_cast<uint32_t>(_scene.textures.size());
    //const uint32_t numLights = static_cast<uint32_t>(_scene.lights.size());

    descriptorSetLayouts_.resize(DESCRIPTOR_SET_SIZE);

    // set 0:
    //  binding 0  ->  Acceleration structure
    //  binding 1  ->  Output image
    //  binding 2  ->  Scene data
    //  binding 3  ->  Camera data
    {
        VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding;
        accelerationStructureLayoutBinding.binding = DESCRIPTOR_BINDING_ACCELERATION_STRUCTURE;
        accelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        accelerationStructureLayoutBinding.descriptorCount = 1;
        accelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        VkDescriptorSetLayoutBinding outputImageLayoutBinding;
        outputImageLayoutBinding.binding = DESCRIPTOR_BINDING_OUTPUT_IMAGE;
        outputImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        outputImageLayoutBinding.descriptorCount = 1;
        outputImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        VkDescriptorSetLayoutBinding sceneDataLayoutBinding;
        sceneDataLayoutBinding.binding = DESCRIPTOR_BINDING_SCENE_DATA;
        sceneDataLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        sceneDataLayoutBinding.descriptorCount = 1;
        sceneDataLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        VkDescriptorSetLayoutBinding cameraDataLayoutBinding;
        cameraDataLayoutBinding.binding = DESCRIPTOR_BINDING_CAMERA_DATA;
        cameraDataLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        cameraDataLayoutBinding.descriptorCount = 1;
        cameraDataLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

        std::vector<VkDescriptorSetLayoutBinding> bindings({
                accelerationStructureLayoutBinding,
                outputImageLayoutBinding,
                sceneDataLayoutBinding,
                cameraDataLayoutBinding
            });

        VkDescriptorSetLayoutCreateInfo descriptorSet0LayoutCreateInfo = {};
        descriptorSet0LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSet0LayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        descriptorSet0LayoutCreateInfo.pBindings = bindings.data();

        VK_CHECK(vkCreateDescriptorSetLayout(rayTracerVulkanInstance_.device, &descriptorSet0LayoutCreateInfo, nullptr, &descriptorSetLayouts_[0]))
    }

    // set 1
    // binding 0 -> attributes
    {
        const VkDescriptorBindingFlags set1Flag = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

        VkDescriptorSetLayoutBindingFlagsCreateInfo set1BindingFlags;
        set1BindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        set1BindingFlags.pNext = nullptr;
        set1BindingFlags.pBindingFlags = &set1Flag;
        set1BindingFlags.bindingCount = 1;

        VkDescriptorSetLayoutBinding verticesLayoutBinding;
        verticesLayoutBinding.binding = DESCRIPTOR_SET_VERTEX_ATTRIBUTES;
        verticesLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        verticesLayoutBinding.descriptorCount = static_cast<uint32_t>(sharedMeshAttributesPool_.pool_size());
        verticesLayoutBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        VkDescriptorSetLayoutCreateInfo descriptorSet1LayoutCreateInfo = {};
        descriptorSet1LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSet1LayoutCreateInfo.bindingCount = 1;
        descriptorSet1LayoutCreateInfo.pBindings = &verticesLayoutBinding;
        descriptorSet1LayoutCreateInfo.pNext = &set1BindingFlags;

        VK_CHECK(vkCreateDescriptorSetLayout(rayTracerVulkanInstance_.device, &descriptorSet1LayoutCreateInfo, nullptr, &descriptorSetLayouts_[1]))
    }

    // set 2
    // binding 0 -> faces
    {
        const VkDescriptorBindingFlags set2Flag = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

        VkDescriptorSetLayoutBindingFlagsCreateInfo set2BindingFlags;
        set2BindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
        set2BindingFlags.pNext = nullptr;
        set2BindingFlags.pBindingFlags = &set2Flag;
        set2BindingFlags.bindingCount = 1;

        VkDescriptorSetLayoutBinding indicesLayoutBinding;
        indicesLayoutBinding.binding = DESCRIPTOR_SET_FACE_DATA;
        indicesLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        indicesLayoutBinding.descriptorCount = static_cast<uint32_t>(sharedMeshFacesPool_.pool_size());;
        indicesLayoutBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

        VkDescriptorSetLayoutCreateInfo descriptorSet2LayoutCreateInfo = {};
        descriptorSet2LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSet2LayoutCreateInfo.bindingCount = 1;
        descriptorSet2LayoutCreateInfo.pBindings = &indicesLayoutBinding;
        descriptorSet2LayoutCreateInfo.pNext = &set2BindingFlags;

        VK_CHECK(vkCreateDescriptorSetLayout(rayTracerVulkanInstance_.device, &descriptorSet2LayoutCreateInfo, nullptr, &descriptorSetLayouts_[2]))
    }
}

void PixelsForGlory::RayTracerAPI_Vulkan::CreatePipeline()
{
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = DESCRIPTOR_SET_SIZE;
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts_.data();

    VK_CHECK(vkCreatePipelineLayout(rayTracerVulkanInstance_.device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout_));

    VulkanShader rayGenShader(rayTracerVulkanInstance_.device);
    VulkanShader rayChitShader(rayTracerVulkanInstance_.device);
    VulkanShader rayMissShader(rayTracerVulkanInstance_.device);
    VulkanShader shadowChit(rayTracerVulkanInstance_.device);
    VulkanShader shadowMiss(rayTracerVulkanInstance_.device);

    rayGenShader.LoadFromFile((shadersFolder_ + "ray_gen.bin").c_str());
    rayChitShader.LoadFromFile((shadersFolder_ + "ray_chit.bin").c_str());
    rayMissShader.LoadFromFile((shadersFolder_ + "ray_miss.bin").c_str());
    shadowChit.LoadFromFile((shadersFolder_ + "shadow_ray_chit.bin").c_str());
    shadowMiss.LoadFromFile((shadersFolder_ + "shadow_ray_miss.bin").c_str());

    shaderBindingTable_.Initialize(2, 2, rayTracerVulkanInstance_.rayTracingProperties.shaderGroupHandleSize, rayTracerVulkanInstance_.rayTracingProperties.shaderGroupBaseAlignment);

    // Ray generation stage
    shaderBindingTable_.SetRaygenStage(rayGenShader.GetShaderStage(VK_SHADER_STAGE_RAYGEN_BIT_KHR));

    // Hit stages
    shaderBindingTable_.AddStageToHitGroup({ rayChitShader.GetShaderStage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) }, PRIMARY_HIT_SHADERS_INDEX);
    shaderBindingTable_.AddStageToHitGroup({ shadowChit.GetShaderStage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) }, SHADOW_HIT_SHADERS_INDEX);

    // Define miss stages for both primary and shadow misses
    shaderBindingTable_.AddStageToMissGroup(rayMissShader.GetShaderStage(VK_SHADER_STAGE_MISS_BIT_KHR), PRIMARY_MISS_SHADERS_INDEX);
    shaderBindingTable_.AddStageToMissGroup(shadowMiss.GetShaderStage(VK_SHADER_STAGE_MISS_BIT_KHR), SHADOW_MISS_SHADERS_INDEX);

    // Create the pipeline for ray tracing based on shader binding table
    VkRayTracingPipelineCreateInfoKHR rayPipelineInfo = {};
    rayPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    rayPipelineInfo.stageCount = shaderBindingTable_.GetNumStages();
    rayPipelineInfo.pStages = shaderBindingTable_.GetStages();
    rayPipelineInfo.groupCount = shaderBindingTable_.GetNumGroups();
    rayPipelineInfo.pGroups = shaderBindingTable_.GetGroups();
    rayPipelineInfo.maxPipelineRayRecursionDepth = 1;
    rayPipelineInfo.layout = pipelineLayout_;

    VK_CHECK(vkCreateRayTracingPipelinesKHR(rayTracerVulkanInstance_.device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayPipelineInfo, nullptr, &pipeline_))

    shaderBindingTable_.CreateSBT(rayTracerVulkanInstance_.device, rayTracerVulkanInstance_.physicalDeviceMemoryProperties, pipeline_);

    Debug::Log("Successfully created pipeline");
}

void PixelsForGlory::RayTracerAPI_Vulkan::CreateDescriptorPool()
{
    auto meshCount = static_cast<uint32_t>(sharedMeshesPool_.pool_size());

    // Descriptors are not generate directly, but from a pool.  Create that pool here
    std::vector<VkDescriptorPoolSize> poolSizes({
        { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },       // Top level acceleration structure
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },                    // Output image
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},                    // Scene data
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },                   // Camera data
        //{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, numMaterials },      // Material data
        //{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, numLights },         // Lighting data
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, meshCount * 2 }        // vertex attribs for each mesh + faces buffer for each mesh
        //{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, numMaterials } // textures for each material
        });

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolCreateInfo.pNext = nullptr;
    descriptorPoolCreateInfo.flags = 0;
    descriptorPoolCreateInfo.maxSets = DESCRIPTOR_SET_SIZE;
    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();

    VK_CHECK(vkCreateDescriptorPool(rayTracerVulkanInstance_.device, &descriptorPoolCreateInfo, nullptr, &descriptorPool_))

    Debug::Log("Successfully created descriptor pool");
}

void PixelsForGlory::RayTracerAPI_Vulkan::BuildDescriptorBufferInfos()
{
    sceneBufferInfo_.buffer = sceneData_.GetBuffer();
    sceneBufferInfo_.offset = 0;
    sceneBufferInfo_.range = sceneData_.GetSize();

    cameraBufferInfo_.buffer = cameraData_.GetBuffer();
    cameraBufferInfo_.offset = 0;
    cameraBufferInfo_.range = cameraData_.GetSize();

    sharedMeshAttributesBufferInfos_.clear();
    sharedMeshAttributesBufferInfos_.resize(sharedMeshAttributesPool_.pool_size());
    for (int i = 0; i < sharedMeshAttributesPool_.pool_size(); ++i)
    {
        VkDescriptorBufferInfo& bufferInfo = sharedMeshAttributesBufferInfos_[i];
        const VulkanBuffer& buffer = sharedMeshAttributesPool_[i];

        bufferInfo.buffer = buffer.GetBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = buffer.GetSize();

        sharedMeshAttributesBufferInfos_.push_back(bufferInfo);
    }

    sharedMeshFacesBufferInfos_.clear();
    sharedMeshFacesBufferInfos_.resize(sharedMeshFacesPool_.pool_size());
    for (int i = 0; i < sharedMeshFacesPool_.pool_size(); ++i)
    {
        VkDescriptorBufferInfo& bufferInfo = sharedMeshFacesBufferInfos_[i];
        const VulkanBuffer& buffer = sharedMeshFacesPool_[i];

        bufferInfo.buffer = buffer.GetBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = buffer.GetSize();

        sharedMeshFacesBufferInfos_.push_back(bufferInfo);
    }
}

void PixelsForGlory::RayTracerAPI_Vulkan::UpdateDescriptorSets()
{
    // Update the descriptor sets with the actual data to store in memory.

    // Now use the pool to upload data for each descriptor
    descriptorSets_.resize(DESCRIPTOR_SET_SIZE);
    std::vector<uint32_t> variableDescriptorCounts({
        1,                                                              // Set 0
        static_cast<uint32_t>(sharedMeshAttributesPool_.pool_size()),   // Set 1
        static_cast<uint32_t>(sharedMeshFacesPool_.pool_size())         // Set 2
        });

    VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountInfo;
    variableDescriptorCountInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    variableDescriptorCountInfo.pNext = nullptr;
    variableDescriptorCountInfo.descriptorSetCount = DESCRIPTOR_SET_SIZE;
    variableDescriptorCountInfo.pDescriptorCounts = variableDescriptorCounts.data(); // actual number of descriptors

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocateInfo.pNext = &variableDescriptorCountInfo;
    descriptorSetAllocateInfo.descriptorPool = descriptorPool_;
    descriptorSetAllocateInfo.descriptorSetCount = DESCRIPTOR_SET_SIZE;
    descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts_.data();

    VK_CHECK(vkAllocateDescriptorSets(rayTracerVulkanInstance_.device, &descriptorSetAllocateInfo, descriptorSets_.data()))

    std::vector<VkWriteDescriptorSet> descriptorWrites;
    // Acceleration Structure
    {
        VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo;
        descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        descriptorAccelerationStructureInfo.pNext = nullptr;
        descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
        descriptorAccelerationStructureInfo.pAccelerationStructures = &tlas_.accelerationStructure;

        VkWriteDescriptorSet accelerationStructureWrite;
        accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo; // Notice that pNext is assigned here!
        accelerationStructureWrite.dstSet = descriptorSets_[DESCRIPTOR_SET_ACCELERATION_STRUCTURE];
        accelerationStructureWrite.dstBinding = DESCRIPTOR_BINDING_ACCELERATION_STRUCTURE;
        accelerationStructureWrite.dstArrayElement = 0;
        accelerationStructureWrite.descriptorCount = 1;
        accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        accelerationStructureWrite.pImageInfo = nullptr;
        accelerationStructureWrite.pBufferInfo = nullptr;
        accelerationStructureWrite.pTexelBufferView = nullptr;

        descriptorWrites.push_back(accelerationStructureWrite);
    }

    // Output image
    {
        VkDescriptorImageInfo descriptorOutputImageInfo;
        descriptorOutputImageInfo.sampler = VK_NULL_HANDLE;
        descriptorOutputImageInfo.imageView = offscreenImage_->GetImageView();
        descriptorOutputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        VkWriteDescriptorSet resultImageWrite;
        resultImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        resultImageWrite.pNext = nullptr;
        resultImageWrite.dstSet = descriptorSets_[DESCRIPTOR_SET_OUTPUT_IMAGE];
        resultImageWrite.dstBinding = DESCRIPTOR_BINDING_OUTPUT_IMAGE;
        resultImageWrite.dstArrayElement = 0;
        resultImageWrite.descriptorCount = 1;
        resultImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        resultImageWrite.pImageInfo = &descriptorOutputImageInfo;
        resultImageWrite.pBufferInfo = nullptr;
        resultImageWrite.pTexelBufferView = nullptr;

        descriptorWrites.push_back(resultImageWrite);
    }

    // Scene data
    {
        VkWriteDescriptorSet sceneBufferWrite;
        sceneBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        sceneBufferWrite.pNext = nullptr;
        sceneBufferWrite.dstSet = descriptorSets_[DESCRIPTOR_SET_SCENE_DATA];
        sceneBufferWrite.dstBinding = DESCRIPTOR_BINDING_SCENE_DATA;
        sceneBufferWrite.dstArrayElement = 0;
        sceneBufferWrite.descriptorCount = 1;
        sceneBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        sceneBufferWrite.pImageInfo = nullptr;
        sceneBufferWrite.pBufferInfo = &sceneBufferInfo_;
        sceneBufferWrite.pTexelBufferView = nullptr;

        descriptorWrites.push_back(sceneBufferWrite);
    }

    // Camera data
    {
        VkWriteDescriptorSet camdataBufferWrite;
        camdataBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        camdataBufferWrite.pNext = nullptr;
        camdataBufferWrite.dstSet = descriptorSets_[DESCRIPTOR_SET_CAMERA_DATA];
        camdataBufferWrite.dstBinding = DESCRIPTOR_BINDING_CAMERA_DATA;
        camdataBufferWrite.dstArrayElement = 0;
        camdataBufferWrite.descriptorCount = 1;
        camdataBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        camdataBufferWrite.pImageInfo = nullptr;
        camdataBufferWrite.pBufferInfo = &cameraBufferInfo_;
        camdataBufferWrite.pTexelBufferView = nullptr;

        descriptorWrites.push_back(camdataBufferWrite);
    }
   
    // Vertex attributes
    {
        VkWriteDescriptorSet attribsBufferWrite;
        attribsBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        attribsBufferWrite.pNext = nullptr;
        attribsBufferWrite.dstSet = descriptorSets_[DESCRIPTOR_SET_VERTEX_ATTRIBUTES];
        attribsBufferWrite.dstBinding = DESCRIPTOR_BINDING_VERTEX_ATTRIBUTES;
        attribsBufferWrite.dstArrayElement = 0;
        attribsBufferWrite.descriptorCount = static_cast<uint32_t>(sharedMeshAttributesPool_.pool_size());
        attribsBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        attribsBufferWrite.pImageInfo = nullptr;
        attribsBufferWrite.pBufferInfo = sharedMeshAttributesBufferInfos_.data();
        attribsBufferWrite.pTexelBufferView = nullptr;

        descriptorWrites.push_back(attribsBufferWrite);
    }
        
    // Faces
    {
        VkWriteDescriptorSet facesBufferWrite;
        facesBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        facesBufferWrite.pNext = nullptr;
        facesBufferWrite.dstSet = descriptorSets_[DESCRIPTOR_SET_FACE_DATA];
        facesBufferWrite.dstBinding = DESCRIPTOR_BINDING_FACE_DATA;
        facesBufferWrite.dstArrayElement = 0;
        facesBufferWrite.descriptorCount = static_cast<uint32_t>(sharedMeshFacesPool_.pool_size());;
        facesBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        facesBufferWrite.pImageInfo = nullptr;
        facesBufferWrite.pBufferInfo = sharedMeshFacesBufferInfos_.data();
        facesBufferWrite.pTexelBufferView = nullptr;

        descriptorWrites.push_back(facesBufferWrite);
    }

    vkUpdateDescriptorSets(rayTracerVulkanInstance_.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, VK_NULL_HANDLE);

    Debug::Log("Successfully updated descriptor sets");
}

void PixelsForGlory::RayTracerAPI_Vulkan::BuildCommandBuffer()
{
    //if (width != storage_image.width || height != storage_image.height)
    //{
    //    // If the view port size has changed, we need to recreate the storage image
    //    vkDestroyImageView(get_device().get_handle(), storage_image.view, nullptr);
    //    vkDestroyImage(get_device().get_handle(), storage_image.image, nullptr);
    //    vkFreeMemory(get_device().get_handle(), storage_image.memory, nullptr);
    //    create_storage_image();

    //    // The descriptor also needs to be updated to reference the new image
    //    VkDescriptorImageInfo image_descriptor{};
    //    image_descriptor.imageView = storage_image.view;
    //    image_descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    //    VkWriteDescriptorSet result_image_write = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &image_descriptor);
    //    vkUpdateDescriptorSets(get_device().get_handle(), 1, &result_image_write, 0, VK_NULL_HANDLE);
    //    build_command_buffers();
    //}

    commandBuffer_ = rayTracerVulkanInstance_.CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

    vkCmdBindPipeline(
        commandBuffer_,
        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        pipeline_);

    vkCmdBindDescriptorSets(
        commandBuffer_,
        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        pipelineLayout_, 0,
        static_cast<uint32_t>(descriptorSets_.size()), descriptorSets_.data(),
        0, 0);
        
    VkStridedDeviceAddressRegionKHR raygenShaderEntry = {};
    raygenShaderEntry.deviceAddress = GetBufferDeviceAddress(shaderBindingTable_.GetBuffer()).deviceAddress + shaderBindingTable_.GetRaygenOffset();
    raygenShaderEntry.stride = shaderBindingTable_.GetGroupsStride();
    raygenShaderEntry.size = shaderBindingTable_.GetRaygenSize();

    VkStridedDeviceAddressRegionKHR missShaderEntry{};
    missShaderEntry.deviceAddress = GetBufferDeviceAddress(shaderBindingTable_.GetBuffer()).deviceAddress + shaderBindingTable_.GetMissGroupsOffset();
    missShaderEntry.stride = shaderBindingTable_.GetGroupsStride();
    missShaderEntry.size = shaderBindingTable_.GetMissGroupsSize();

    VkStridedDeviceAddressRegionKHR hitShaderEntry{};
    hitShaderEntry.deviceAddress = GetBufferDeviceAddress(shaderBindingTable_.GetBuffer()).deviceAddress + shaderBindingTable_.GetHitGroupsOffset();
    hitShaderEntry.stride = shaderBindingTable_.GetGroupsStride();
    hitShaderEntry.size = shaderBindingTable_.GetHitGroupsSize();

    VkStridedDeviceAddressRegionKHR callableShaderEntry{};
 
    // Dispatch the ray tracing commands
    vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline_);
    vkCmdBindDescriptorSets(commandBuffer_, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout_, 0, static_cast<uint32_t>(descriptorSets_.size()), descriptorSets_.data(), 0, 0);

    vkCmdTraceRaysKHR(
        commandBuffer_,
        &raygenShaderEntry,
        &missShaderEntry,
        &hitShaderEntry,
        &callableShaderEntry,
        width_,
        height_,
        1);
   
    rayTracerVulkanInstance_.EndCommandBuffer(commandBuffer_);
    //}
}

VkDeviceOrHostAddressKHR PixelsForGlory::RayTracerAPI_Vulkan::GetBufferDeviceAddress(const VulkanBuffer& buffer) {
    VkBufferDeviceAddressInfoKHR info = {
        VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        nullptr,
        buffer.GetBuffer()
    };

    VkDeviceOrHostAddressKHR result;
    result.deviceAddress = vkGetBufferDeviceAddressKHR(rayTracerVulkanInstance_.device, &info);

    return result;
}

VkDeviceOrHostAddressConstKHR PixelsForGlory::RayTracerAPI_Vulkan::GetBufferDeviceAddressConst(const VulkanBuffer& buffer) {
    VkDeviceOrHostAddressKHR address = GetBufferDeviceAddress(buffer);

    VkDeviceOrHostAddressConstKHR result;
    result.deviceAddress = address.deviceAddress;

    return result;
}

#endif // #if SUPPORT_VULKAN

