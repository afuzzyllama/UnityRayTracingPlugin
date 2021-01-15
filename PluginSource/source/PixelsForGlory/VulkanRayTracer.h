//#pragma once
//
//#include <vector>
//#include <string>
//#include "../vulkan.h"
//
//
//class VulkanBuffer
//{
//    bool CreateVulkanBuffer(size_t sizeInBytes, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties)
//    {
//        if (sizeInBytes == 0)
//        {
//            Debug::LogError("Cannot create Vulkan buffer of 0 size");
//            return false;
//        }
//
//        VkBufferCreateInfo bufferCreateInfo = { };
//        bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//        bufferCreateInfo.pNext = nullptr;
//        bufferCreateInfo.flags = 0;
//        bufferCreateInfo.size = sizeInBytes;
//        bufferCreateInfo.usage = usage;
//        bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//        bufferCreateInfo.pQueueFamilyIndices = nullptr;
//
//        sizeInBytes_ = sizeInBytes;
//        if (vkCreateBuffer(device_, &bufferCreateInfo, NULL, &buffer_) != VK_SUCCESS)
//        {
//            Debug::LogError("Cannot create Vulkan buffer");
//            return false;
//        }
//
//        VkMemoryRequirements memoryRequirements;
//        vkGetBufferMemoryRequirements(device_, buffer_, &memoryRequirements);
//
//        const int memoryTypeIndex = GetMemoryType(memoryRequirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
//        if (memoryTypeIndex < 0)
//        {
//            Debug::LogError("Cannot find memory type index for buffer");
//            ImmediateDestroyVulkanBuffer(*buffer);
//            return false;
//        }
//
//        VkMemoryAllocateInfo memoryAllocateInfo = { };
//        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//        memoryAllocateInfo.pNext = nullptr;
//        memoryAllocateInfo.allocationSize = memoryRequirements.size;
//        memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;
//
//        VkMemoryAllocateFlagsInfo allocationFlags = {};
//        allocationFlags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
//        allocationFlags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
//        if ((usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) == VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
//            memoryAllocateInfo.pNext = &allocationFlags;
//        }
//
//        if (vkAllocateMemory(device_, &memoryAllocateInfo, nullptr, &buffer->deviceMemory) != VK_SUCCESS)
//        {
//            Debug::LogError("Cannot allocate memory for buffer");
//            ImmediateDestroyVulkanBuffer(*buffer);
//            return false;
//        }
//
//        if (vkBindBufferMemory(device_, buffer->buffer, buffer->deviceMemory, 0) != VK_SUCCESS)
//        {
//            Debug::LogError("Cannot bind memory for buffer");
//            ImmediateDestroyVulkanBuffer(*buffer);
//            return false;
//        }
//
//        return true;
//    }
//
//    void PixelsForGlory::VulkanRayTracer::ImmediateDestroyVulkanBuffer(const VulkanBuffer& buffer)
//    {
//        if (buffer.buffer != VK_NULL_HANDLE)
//            vkDestroyBuffer(device_, buffer.buffer, NULL);
//
//        if (buffer.mapped && buffer.deviceMemory != VK_NULL_HANDLE)
//            vkUnmapMemory(device_, buffer.deviceMemory);
//
//        if (buffer.deviceMemory != VK_NULL_HANDLE)
//            vkFreeMemory(device_, buffer.deviceMemory, NULL);
//    }
//
//private:
//    VkBuffer buffer_;
//    VkDeviceMemory deviceMemory_;
//    VkDeviceSize sizeInBytes_;
//};
//
//namespace PixelsForGlory
//{
//    class VulkanRayTracer
//    {
//    public:
//        static VulkanRayTracer& Instance()
//        {
//            static VulkanRayTracer instance;
//            return instance;
//        }
//
//        /// <summary>
//        /// Set the Vulkan instance from Unity3D
//        /// </summary>
//        /// <param name="instance_"></param>
//        void SetInstance(VkInstance* instance);
//        
//        /// <summary>
//        /// Set the Vulkan physical device and query memory properties
//        /// </summary>
//        /// <param name="physicalDevice"></param>
//        void SetAndQueryPhysicalDevice(VkPhysicalDevice physicalDevice);
//
//        /// <summary>
//        /// Resolve render queues required for ray tracing
//        /// </summary>
//        /// <returns>true if queues are resolved, false otherwise</returns>
//        bool ResolveQueues();
//
//        /// <summary>
//        /// Creates a device that can support ray tracing
//        /// </summary>
//        VkResult CreateDevice(const VkDeviceCreateInfo* unityCreateInfo, const VkAllocationCallbacks* unityAllocator, VkDevice* unityDevice);
//
//        void AddMesh(int instanceId, float* vertices, int vertexCount, int* indices, int indexCount);
//
//        void LoadScene();
//        void CreateScene();
//        void CreateDescriptorSetsLayouts();
//        bool CreatePipeline();
//        bool FillCommandBuffer();
//
//
//        VulkanRayTracer(VulkanRayTracer const&) = delete;   // not implemented for singleton
//        void operator= (VulkanRayTracer const&) = delete;   // not implemented for singleton
//
//    private:
//        VkInstance*                      instance_;
//        VkPhysicalDevice                 physicalDevice_;
//        VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties_;   
//        VkDevice                         device_;
//        
//        VkQueue  graphicsQueue_;
//        VkQueue  computeQueue_;
//        VkQueue  transferQueue_;
//        uint32_t graphicsQueueFamilyIndex_;
//        uint32_t computeQueueFamilyIndex_;
//        uint32_t transferQueueFamilyIndex_;
//        
//        VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties_;
//
//        VkSurfaceKHR*      surface_;
//        VkSurfaceFormatKHR surfaceFormat_;
//
//        VkSwapchainKHR           swapchain_;
//        std::vector<VkImage>     swapchainImages_;
//        std::vector<VkImageView> swapchainImageViews_;
//
//        std::vector<VkFence> frameFences_;
//
//        VkCommandPool commandPool_;
//
//        //VulkanHelpers::Image* offscreenImage_;
//
//        std::vector<VkCommandBuffer> commandBuffers_;
//
//        struct MeshData {
//            int instanceId;
//
//            int vertexCount;
//            int indexCount;
//
//            VulkanBuffer vertices;
//            VulkanBuffer indices;
//            // VulkanBuffer attribs;
//            // VulkanHelpers::Buffer       matIDs;
//
//            //RTAccelerationStructure     blas;
//        };
//
//        struct AccelerationStructureResource {
//            VkDeviceMemory                          memory;
//            VkAccelerationStructureCreateInfoKHR    accelerationStructureCreateInfo;
//            VkAccelerationStructureKHR              accelerationStructure;
//            VkDeviceAddress                         handle;
//        };
//
//
//        VkSemaphore semaphoreImageAcquired_;
//        VkSemaphore semaphoreRenderFinished_;
//
//        bool CreateVulkanBuffer(size_t bytes, VulkanBuffer* buffer, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties);
//        void ImmediateDestroyVulkanBuffer(const VulkanBuffer& buffer);
//        void SafeDestroy(unsigned long long frameNumber, const VulkanBuffer& buffer);
//        void GarbageCollect(bool force = false);
//
//        bool CreateAccelerationStructure(
//            const VkAccelerationStructureTypeKHR type,
//            const uint32_t geometryCount,
//            const VkAccelerationStructureGeometryKHR* geometries,
//            const uint32_t instanceCount,
//            AccelerationStructureResource& accelerationStructureResource);
//
//
//        uint32_t GetMemoryType(VkMemoryRequirements& memoryRequiriments, VkMemoryPropertyFlags memoryProperties);
//
//
//        VulkanRayTracer() :
//            instance_(VK_NULL_HANDLE),
//            physicalDevice_(VK_NULL_HANDLE),
//            physicalDeviceMemoryProperties_(VkPhysicalDeviceMemoryProperties()),
//            device_(VK_NULL_HANDLE),
//            graphicsQueue_(VK_NULL_HANDLE),
//            computeQueue_(VK_NULL_HANDLE),
//            transferQueue_(VK_NULL_HANDLE)
//            {}    // private so cannot be instantiated publically 
//
//    };
//}