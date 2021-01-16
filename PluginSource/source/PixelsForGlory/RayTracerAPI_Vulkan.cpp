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

#include "Debug.h"
#include "VulkanBuffer.h"


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

static VKAPI_ATTR void VKAPI_CALL Hook_vkCmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo* pRenderPassBegin, VkSubpassContents contents)
{
    // Change this to 'true' to override the clear color with green
    const bool allowOverrideClearColor = false;
    if (pRenderPassBegin->clearValueCount <= 16 && pRenderPassBegin->clearValueCount > 0 && allowOverrideClearColor)
    {
        VkClearValue clearValues[16] = {};
        memcpy(clearValues, pRenderPassBegin->pClearValues, pRenderPassBegin->clearValueCount * sizeof(VkClearValue));

        VkRenderPassBeginInfo patchedBeginInfo = *pRenderPassBegin;
        patchedBeginInfo.pClearValues = clearValues;
        for (unsigned int i = 0; i < pRenderPassBegin->clearValueCount - 1; ++i)
        {
            clearValues[i].color.float32[0] = 0.0;
            clearValues[i].color.float32[1] = 1.0;
            clearValues[i].color.float32[2] = 0.0;
            clearValues[i].color.float32[3] = 1.0;
        }
        vkCmdBeginRenderPass(commandBuffer, &patchedBeginInfo, contents);
    }
    else
    {
        vkCmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
    }
}



static int FindMemoryTypeIndex(VkPhysicalDeviceMemoryProperties const& physicalDeviceMemoryProperties, VkMemoryRequirements const& memoryRequirements, VkMemoryPropertyFlags memoryPropertyFlags)
{
    uint32_t memoryTypeBits = memoryRequirements.memoryTypeBits;

    // Search memtypes to find first index with those properties
    for (uint32_t memoryTypeIndex = 0; memoryTypeIndex < VK_MAX_MEMORY_TYPES; ++memoryTypeIndex)
    {
        if ((memoryTypeBits & 1) == 1)
        {
            // Type is available, does it match user properties?
            if ((physicalDeviceMemoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & memoryPropertyFlags) == memoryPropertyFlags)
                return memoryTypeIndex;
        }
        memoryTypeBits >>= 1;
    }

    return -1;
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
            
            vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);
        }

        VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level, bool begin)
        {
            VkCommandBufferAllocateInfo cmd_buf_allocate_info{};
            cmd_buf_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmd_buf_allocate_info.commandPool = commandPool;
            cmd_buf_allocate_info.level = level;
            cmd_buf_allocate_info.commandBufferCount = 1;

            VkCommandBuffer command_buffer;
            vkAllocateCommandBuffers(device, &cmd_buf_allocate_info, &command_buffer);

            // If requested, also start recording for the new command buffer
            if (begin)
            {
                VkCommandBufferBeginInfo commandBufferBeginInfo = { };
                commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                vkBeginCommandBuffer(command_buffer, &commandBufferBeginInfo);
            }

            return command_buffer;
        }

        void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue, bool free)
        {
            if (commandBuffer == VK_NULL_HANDLE)
            {
                return;
            }

            vkEndCommandBuffer(commandBuffer);

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
            vkCreateFence(device, &fenceCreateInfo, nullptr, &fence);

            // Submit to the queue
            VkResult result = vkQueueSubmit(queue, 1, &submitInfo, fence);
            // Wait for the fence to signal that command buffer has finished executing
            const uint64_t DEFAULT_FENCE_TIMEOUT = 100000000000;
            vkWaitForFences(device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);

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

    struct RayTracerMeshData
    {
        RayTracerMeshData()
            : sharedMeshInstanceId(0)
            , vertexCount(0)
            , indexCount(0)
            , vertices()
            , normals()
            , uvs()
            , indices()
            , blas()
        {}

        int sharedMeshInstanceId;
        int vertexCount;
        int indexCount;

        VulkanBuffer vertices;
        VulkanBuffer normals;
        VulkanBuffer uvs;
        VulkanBuffer indices;

        RayTracerAccelerationStructure blas;
    };

    struct RayTracerMeshInstanceData
    {
        int meshInstanceId;
        int sharedMeshInstanceId;
        mat4 worldTransform;
    };



    class RayTracerAPI_Vulkan : public RayTracerAPI
    {
    public:
        RayTracerAPI_Vulkan();
        virtual ~RayTracerAPI_Vulkan() { }

        virtual void ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces);
        
        virtual void AddMesh(int sharedMeshInstanceId, float* vertices, float* normals, float* uvs, int vertexCount, int* indices, int indexCount);

        virtual void AddTlasInstance(int meshInstanceId, int sharedMeshInstanceId, float* l2wMatrix);

        virtual void BuildTlas();

    //private:
        //typedef std::map<unsigned long long, VulkanBuffers> DeleteQueue;
       /* void SafeDestroy(unsigned long long frameNumber, const VulkanBuffer& buffer);
        void GarbageCollect(bool force = false);*/

    private:
        IUnityGraphicsVulkan* graphicsInterface_;
        RayTracerVulkanInstance rayTracerVulkanInstance_;
        
        // sharedMeshInstanceId => data
        std::map<int, std::shared_ptr<RayTracerMeshData>> meshes;

        // meshInstanceId => data
        std::map<int, RayTracerMeshInstanceData> meshInstances;

        RayTracerAccelerationStructure tlas;

        //std::map<unsigned long long, VulkanBuffers> m_DeleteQueue;

        void Destroy();

        void BuildBlas(const std::shared_ptr<RayTracerMeshData>& mesh);
        
        
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
        eventConfig.renderPassPrecondition = kUnityVulkanRenderPass_DontCare;
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
    // Destory all added mesh buffers
    for (auto const& item : meshes)
    {
        auto mesh = item.second;
        mesh->vertices.Destroy();
        mesh->normals.Destroy();
        mesh->uvs.Destroy();
        mesh->indices.Destroy();

        if (mesh->blas.accelerationStructure != VK_NULL_HANDLE)
        {
            mesh->blas.buffer.Destroy();
            vkDestroyAccelerationStructureKHR(rayTracerVulkanInstance_.device, mesh->blas.accelerationStructure, nullptr);
        }
    }

    if (tlas.accelerationStructure != VK_NULL_HANDLE)
    {
        tlas.buffer.Destroy();
        vkDestroyAccelerationStructureKHR(rayTracerVulkanInstance_.device, tlas.accelerationStructure, nullptr);
    }
}


void PixelsForGlory::RayTracerAPI_Vulkan::AddMesh(int shareMeshInstanceId, float* vertices, float* normals, float* uvs, int vertexCount, int* indices, int indexCount)
{
    if (meshes.find(shareMeshInstanceId) != meshes.end())
    {
        Debug::Log("Mesh already added (instanceId = " + std::to_string(shareMeshInstanceId) + ")");
        return;
    }
    
    auto sentMesh = std::make_shared<RayTracerMeshData>();
    sentMesh->sharedMeshInstanceId = shareMeshInstanceId;
    
    sentMesh->vertexCount = vertexCount;
    sentMesh->indexCount = indexCount;

    bool success = true;
    if (sentMesh->vertices.Create(
            rayTracerVulkanInstance_.device,
            rayTracerVulkanInstance_.physicalDeviceMemoryProperties,
            sizeof(float) * vertexCount * 3,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VulkanBuffer::kDefaultMemoryPropertyFlags) 
        != VK_SUCCESS)
    {
        Debug::LogError("Failed to create vertices buffer for mesh instance id " + std::to_string(shareMeshInstanceId));
        success = false;
    }

    if (sentMesh->normals.Create(
        rayTracerVulkanInstance_.device,
        rayTracerVulkanInstance_.physicalDeviceMemoryProperties,
        sizeof(float) * vertexCount * 3,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VulkanBuffer::kDefaultMemoryPropertyFlags)
        != VK_SUCCESS)
    {
        Debug::LogError("Failed to create normals buffer for mesh instance id " + std::to_string(shareMeshInstanceId));
        success = false;
    }

    if (sentMesh->uvs.Create(
        rayTracerVulkanInstance_.device,
        rayTracerVulkanInstance_.physicalDeviceMemoryProperties,
        sizeof(float) * vertexCount * 2,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VulkanBuffer::kDefaultMemoryPropertyFlags)
        != VK_SUCCESS)
    {
        Debug::LogError("Failed to create uvs buffer for mesh instance id " + std::to_string(shareMeshInstanceId));
        success = false;
    }
    
    if (sentMesh->indices.Create(
            rayTracerVulkanInstance_.device, 
            rayTracerVulkanInstance_.physicalDeviceMemoryProperties, 
            sizeof(int) * indexCount, 
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
            VulkanBuffer::kDefaultMemoryPropertyFlags))
    {
        Debug::LogError("Failed to create indices buffer for mesh instance id " + std::to_string(shareMeshInstanceId));
        success = false;
    }

    if (!success)
    {
        sentMesh->vertices.Destroy();
        sentMesh->normals.Destroy();
        sentMesh->uvs.Destroy();
        sentMesh->indices.Destroy();
        return;
    }

    auto verticesBuffer = reinterpret_cast<float*>(sentMesh->vertices.Map());
    auto normalsBuffer = reinterpret_cast<float*>(sentMesh->normals.Map());
    auto uvsBuffer = reinterpret_cast<float*>(sentMesh->uvs.Map());
    auto indicesBuffer = reinterpret_cast<int*>(sentMesh->indices.Map());

    for (int i = 0; i < vertexCount * 3; ++i)
    {
        verticesBuffer[i] = vertices[i];
        normalsBuffer[i] = normals[i];
    }

    for (int i = 0; i < vertexCount * 2; ++i)
    {
        uvsBuffer[i] = uvs[i];
    }

    for (int i = 0; i < indexCount; ++i)
    {
        indicesBuffer[i] = indices[i];
    }
    
    sentMesh->vertices.Unmap();
    sentMesh->normals.Unmap();
    sentMesh->uvs.Unmap();
    sentMesh->indices.Unmap();

    BuildBlas(sentMesh);

    meshes.insert(std::make_pair(sentMesh->sharedMeshInstanceId, std::move(sentMesh)));

    Debug::Log("Added mesh (sharedMeshInstanceId: " + std::to_string(shareMeshInstanceId) + ")");

}

void PixelsForGlory::RayTracerAPI_Vulkan::BuildBlas(const std::shared_ptr<RayTracerMeshData>& mesh)
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
    accelerationStructureGeometry.geometry.triangles.vertexData = GetBufferDeviceAddressConst(mesh->vertices);
    accelerationStructureGeometry.geometry.triangles.maxVertex = mesh->vertexCount / 3;
    accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(float) * 3;
    accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
    accelerationStructureGeometry.geometry.triangles.indexData = GetBufferDeviceAddressConst(mesh->indices);
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
    const uint32_t primitiveCount = mesh->indexCount / 3;

    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(
        rayTracerVulkanInstance_.device,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &accelerationStructureBuildGeometryInfo,
        &primitiveCount,
        &accelerationStructureBuildSizesInfo);

    // Create a buffer to hold the acceleration structure
    mesh->blas.buffer.Create(
        rayTracerVulkanInstance_.device,
        rayTracerVulkanInstance_.physicalDeviceMemoryProperties,
        accelerationStructureBuildSizesInfo.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
        VulkanBuffer::kDefaultMemoryPropertyFlags);
    
    // Create the acceleration structure
    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = mesh->blas.buffer.GetBuffer();
    accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    vkCreateAccelerationStructureKHR(rayTracerVulkanInstance_.device , &accelerationStructureCreateInfo, nullptr, &mesh->blas.accelerationStructure);

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
    accelerationBuildGeometryInfo.dstAccelerationStructure = mesh->blas.accelerationStructure;
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
    accelerationStructureDeviceAddressInfo.accelerationStructure = mesh->blas.accelerationStructure;
    mesh->blas.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(rayTracerVulkanInstance_.device, &accelerationStructureDeviceAddressInfo);

    Debug::Log("Built blas for mesh (sharedMeshInstanceId: " + std::to_string(mesh->sharedMeshInstanceId) + ")");

}


void PixelsForGlory::RayTracerAPI_Vulkan::AddTlasInstance(int meshInstanceId, int sharedMeshInstanceId, float* l2wMatrix)
{
    RayTracerMeshInstanceData instance;

    instance.meshInstanceId;
    instance.sharedMeshInstanceId = sharedMeshInstanceId;
    FloatArrayToMatrix(l2wMatrix, instance.worldTransform);

    meshInstances.insert(std::make_pair(meshInstanceId, instance));

    Debug::Log("Added mesh instance (meshInstanceId: " + std::to_string(meshInstanceId) + ")");
}

void PixelsForGlory::RayTracerAPI_Vulkan::BuildTlas()
{
    std::vector<VkAccelerationStructureInstanceKHR> instances(meshes.size(), VkAccelerationStructureInstanceKHR{});
    for (auto const& item : meshInstances)
    {
        auto instance = item.second;
        const auto& t = instance.worldTransform;
        VkTransformMatrixKHR transform_matrix = {
            t[0][0], t[0][1], t[0][2], t[0][3],
            t[1][0], t[1][1], t[1][2], t[1][3],
            t[2][0], t[2][1], t[2][2], t[2][3]
        };
            
        VkAccelerationStructureInstanceKHR accelerationStructureInstance = { };
        accelerationStructureInstance.transform = transform_matrix;
        accelerationStructureInstance.instanceCustomIndex = instance.meshInstanceId;
        accelerationStructureInstance.mask = 0xFF;
        accelerationStructureInstance.instanceShaderBindingTableRecordOffset = 0;
        accelerationStructureInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        accelerationStructureInstance.accelerationStructureReference = meshes[instance.sharedMeshInstanceId]->blas.deviceAddress;
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
    const uint32_t primitiveCount = static_cast<uint32_t>(meshes.size());

    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {};
    accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    vkGetAccelerationStructureBuildSizesKHR(
        rayTracerVulkanInstance_.device, 
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &accelerationStructureBuildGeometryInfo,
        &primitiveCount,
        &accelerationStructureBuildSizesInfo);

    // Create a buffer to hold the acceleration structure
    tlas.buffer.Create(
        rayTracerVulkanInstance_.device,
        rayTracerVulkanInstance_.physicalDeviceMemoryProperties,
        accelerationStructureBuildSizesInfo.accelerationStructureSize,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
        VulkanBuffer::kDefaultMemoryPropertyFlags);

    // Create the acceleration structure
    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = tlas.buffer.GetBuffer();
    accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    vkCreateAccelerationStructureKHR(rayTracerVulkanInstance_.device, &accelerationStructureCreateInfo, nullptr, &tlas.accelerationStructure);

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
    accelerationBuildGeometryInfo.dstAccelerationStructure = tlas.accelerationStructure;
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
    accelerationStructureDeviceAddressInfo.accelerationStructure = tlas.accelerationStructure;
    tlas.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(rayTracerVulkanInstance_.device, &accelerationStructureDeviceAddressInfo);

    Debug::Log("Succesfully built tlas");
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


//void RenderAPI_Vulkan::SafeDestroy(unsigned long long frameNumber, const VulkanBuffer& buffer)
//{
//    m_DeleteQueue[frameNumber].push_back(buffer);
//}
//
//void RenderAPI_Vulkan::GarbageCollect(bool force /*= false*/)
//{
//    UnityVulkanRecordingState recordingState;
//    if (force)
//        recordingState.safeFrameNumber = ~0ull;
//    else
//        if (!m_UnityVulkan->CommandRecordingState(&recordingState, kUnityVulkanGraphicsQueueAccess_DontCare))
//            return;
//
//    DeleteQueue::iterator it = m_DeleteQueue.begin();
//    while (it != m_DeleteQueue.end())
//    {
//        if (it->first <= recordingState.safeFrameNumber)
//        {
//            for (size_t i = 0; i < it->second.size(); ++i)
//                ImmediateDestroyVulkanBuffer(it->second[i]);
//            m_DeleteQueue.erase(it++);
//        }
//        else
//            ++it;
//    }
//}

// Enable "The enum type * is unscoped.  Prefere 'enum class' over 'enum'

#endif // #if SUPPORT_VULKAN
