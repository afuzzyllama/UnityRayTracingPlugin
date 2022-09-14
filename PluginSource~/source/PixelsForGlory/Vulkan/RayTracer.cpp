#include "RayTracer.h"

#include <assert.h>
#include <vector>
#include "../Helpers.h"

#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

namespace PixelsForGlory
{
    RayTracerAPI* CreateRayTracerAPI_Vulkan()
    {
        return &Vulkan::RayTracer::Instance();
    }
}

namespace PixelsForGlory::Vulkan
{

#pragma region VulkanHooks
    /// <summary>
    /// Resolve properties and queues required for ray tracing
    /// </summary>
    /// <param name="physicalDevice"></param>
    bool ResolveQueueFamily_RayTracer(VkPhysicalDevice physicalDevice, uint32_t &index) {

        // Call once to get the count
        uint32_t queueFamilyPropertyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

        // Call again to populate vector
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

        // Locate which queueFamilyProperties index 
        for (uint32_t i = 0; i < queueFamilyPropertyCount; ++i) {
            if(
                    (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 &&
                    (queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0
            ){
                PFG_EDITORLOG("Queue family found");
                index = i;
                return true;
            }
        }

        return false;
    }

    /// <summary>
    /// Injects what is required for ray tracing into vkCreateInstance
    /// </summary>
    /// <param name="unityCreateInfo"></param>
    /// <param name="unityAllocator"></param>
    /// <param name="instance"></param>
    /// <returns></returns>
    VkResult CreateInstance_RayTracer(const VkInstanceCreateInfo* unityCreateInfo, const VkAllocationCallbacks* unityAllocator, VkInstance* pInstance)
    {
        PFG_EDITORLOG("Creating instance");

        std::vector<std::string> unityLayers;

        if (unityCreateInfo->enabledLayerCount > 0)
        {
            unityLayers = std::vector<std::string>(
                unityCreateInfo->ppEnabledLayerNames, 
                unityCreateInfo->ppEnabledLayerNames + unityCreateInfo->enabledLayerCount);
        }
        
        std::vector<const char*> enableLayers;
#if _DEBUG
        // If debug build, add debug support.
        auto VK_LAYER_KHRONOS_validation_enabled = false;
#endif

        for (auto const& layer : unityLayers) {
            PFG_EDITORLOG("Unity enabled layer: " + std::string(layer));

#if _DEBUG
            if (strcmp(layer.c_str(), "VK_LAYER_KHRONOS_validation") == 0)
            {
                VK_LAYER_KHRONOS_validation_enabled = true;
            }
#endif

            enableLayers.emplace_back(layer.c_str());
        }

#if _DEBUG
        if (VK_LAYER_KHRONOS_validation_enabled == false)
        {
            PFG_EDITORLOG("Adding layer: VK_LAYER_KHRONOS_validation");
            enableLayers.emplace_back("VK_LAYER_KHRONOS_validation");
        }
#endif


        
        std::vector<std::string> unityExtensions(
            unityCreateInfo->ppEnabledExtensionNames,
            unityCreateInfo->ppEnabledExtensionNames + unityCreateInfo->enabledExtensionCount);

        std::vector<const char*> enableExtensions;

#if _DEBUG
        // If debug build, add debug support.
        auto VK_EXT_DEBUG_REPORT_EXTENSION_NAME_enabled = false;
        auto VK_EXT_DEBUG_UTILS_EXTENSION_NAME_enabled = false;
#endif
      
        for (auto const& ext : unityExtensions) {
            PFG_EDITORLOG("Unity enabled extension: " + std::string(ext));

#if _DEBUG
            if (strcmp(ext.c_str(), VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0)
            {
                VK_EXT_DEBUG_REPORT_EXTENSION_NAME_enabled = true;
            }
            
            if (strcmp(ext.c_str(), VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
            {
                VK_EXT_DEBUG_UTILS_EXTENSION_NAME_enabled = true;
            }
#endif

            enableExtensions.emplace_back(ext.c_str());
        }

#if _DEBUG
        if (VK_EXT_DEBUG_REPORT_EXTENSION_NAME_enabled == false)
        {
            PFG_EDITORLOG("Adding support for extension: " + std::string(VK_EXT_DEBUG_REPORT_EXTENSION_NAME));
            enableExtensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        }

        if (VK_EXT_DEBUG_UTILS_EXTENSION_NAME_enabled == false)
        {
            PFG_EDITORLOG("Adding support for extension: " + std::string(VK_EXT_DEBUG_UTILS_EXTENSION_NAME));
            enableExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
#endif
        VkApplicationInfo applicationInfo = {};
        applicationInfo.sType = unityCreateInfo->pApplicationInfo->sType;
        applicationInfo.pNext = unityCreateInfo->pApplicationInfo->pNext;
        applicationInfo.pApplicationName = unityCreateInfo->pApplicationInfo->pApplicationName;
        applicationInfo.applicationVersion = unityCreateInfo->pApplicationInfo->applicationVersion;
        applicationInfo.pEngineName = unityCreateInfo->pApplicationInfo->pEngineName;
        applicationInfo.engineVersion = unityCreateInfo->pApplicationInfo->engineVersion;
        applicationInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pNext = unityCreateInfo->pNext;
        createInfo.flags = unityCreateInfo->flags;
        createInfo.pApplicationInfo = &applicationInfo;
        createInfo.enabledLayerCount = static_cast<uint32_t>(enableLayers.size());
        createInfo.ppEnabledLayerNames = enableLayers.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(enableExtensions.size());
        createInfo.ppEnabledExtensionNames = enableExtensions.data();

        return vkCreateInstance(&createInfo, unityAllocator, pInstance);
    }

    /// <summary>
    /// Injects what is required for ray tracing into vkCreateDevice
    /// </summary>
    /// <param name="physicalDevice"></param>
    /// <param name="unityCreateInfo"></param>
    /// <param name="unityAllocator"></param>
    /// <param name="device"></param>
    /// <returns></returns>
    VkResult CreateDevice_RayTracer(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* unityCreateInfo, const VkAllocationCallbacks* unityAllocator, VkDevice* device)
    {
        //std::this_thread::sleep_for(std::chrono::seconds(10));

        // Setup required features backwards for chaining
        VkPhysicalDeviceDescriptorIndexingFeatures physicalDeviceDescriptorIndexingFeatures = { };
        physicalDeviceDescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
        physicalDeviceDescriptorIndexingFeatures.pNext = nullptr;

        VkPhysicalDeviceRayTracingPipelineFeaturesKHR physicalDeviceRayTracingPipelineFeatures = { };
        physicalDeviceRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        physicalDeviceRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
        physicalDeviceRayTracingPipelineFeatures.pNext = &physicalDeviceDescriptorIndexingFeatures;

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
        //deviceFeatures.features = *unityCreateInfo->pEnabledFeatures;
        deviceFeatures.pNext = &physicalDeviceAccelerationStructureFeatures;

        vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures); // enable all the features our GPU has

        std::vector<std::string> unityExtensions(
            unityCreateInfo->ppEnabledExtensionNames, 
            unityCreateInfo->ppEnabledExtensionNames + unityCreateInfo->enabledExtensionCount);

        std::vector<const char*> enableExtensions;

        // Required by Raytracing
        // VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME
        // VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME
        // VK_KHR_SPIRV_1_4_EXTENSION_NAME
        auto VK_KHR_ACCELERATION_STRUCTURE_enabled = false;
        auto VK_KHR_RAY_TRACING_PIPELINE_enabled = false;
        auto VK_KHR_SPIRV_1_4_enabled = false;

        // Required by SPRIV 1.4
        // VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME
        auto VK_KHR_SHADER_FLOAT_CONTROLS_enabled = false;

        // Required by acceleration structure
        // VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
        // VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
        // VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
        auto VK_KHR_BUFFER_DEVICE_ADDRESS_enabled = false;
        auto VK_KHR_DEFERRED_HOST_OPERATIONS_enabled = false;
        auto VK_EXT_DESCRIPTOR_INDEXING_enabled = false;

        for (auto const& ext : unityExtensions) {
            PFG_EDITORLOG("Unity enabled extension: " + std::string(ext));

            if (strcmp(ext.c_str(), VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) == 0)
            {
                VK_KHR_ACCELERATION_STRUCTURE_enabled = true;
            }

            if (strcmp(ext.c_str(), VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) == 0)
            {
                VK_KHR_RAY_TRACING_PIPELINE_enabled = true;
            }
            if (strcmp(ext.c_str(), VK_KHR_SPIRV_1_4_EXTENSION_NAME) == 0)
            {
                VK_KHR_SPIRV_1_4_enabled = true;
            }
            if (strcmp(ext.c_str(), VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME) == 0)
            {
                VK_KHR_SHADER_FLOAT_CONTROLS_enabled = true;
            }
            if (strcmp(ext.c_str(), VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) == 0)
            {
                VK_KHR_BUFFER_DEVICE_ADDRESS_enabled = true;
            }
            if (strcmp(ext.c_str(), VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME) == 0)
            {
                VK_KHR_DEFERRED_HOST_OPERATIONS_enabled = true;
            }
            if (strcmp(ext.c_str(), VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME) == 0)
            {
                VK_EXT_DESCRIPTOR_INDEXING_enabled = true;
            }

            enableExtensions.emplace_back(ext.c_str());
        }
        
        if (VK_KHR_ACCELERATION_STRUCTURE_enabled == false)
        {
            PFG_EDITORLOG("Adding support for extension: " + std::string(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME));
            enableExtensions.emplace_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);

        }

        if (VK_KHR_RAY_TRACING_PIPELINE_enabled == false)
        {
            PFG_EDITORLOG("Adding support for extension: " + std::string(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME));
            enableExtensions.emplace_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

        }

        if (VK_KHR_SPIRV_1_4_enabled == false)
        {
            PFG_EDITORLOG("Adding support for extension: " + std::string(VK_KHR_SPIRV_1_4_EXTENSION_NAME));
            enableExtensions.emplace_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);

        }

        if (VK_KHR_SHADER_FLOAT_CONTROLS_enabled == false)
        {
            PFG_EDITORLOG("Adding support for extension: " + std::string(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME));
            enableExtensions.emplace_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);

        }

        if (VK_KHR_BUFFER_DEVICE_ADDRESS_enabled == false)
        {
            PFG_EDITORLOG("Adding support for extension: " + std::string(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME));
            enableExtensions.emplace_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);

        }

        if (VK_KHR_DEFERRED_HOST_OPERATIONS_enabled == false)
        {
            PFG_EDITORLOG("Adding support for extension: " + std::string(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME));
            enableExtensions.emplace_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);

        }

        if (VK_EXT_DESCRIPTOR_INDEXING_enabled == false)
        {
            PFG_EDITORLOG("Adding support for extension: " + std::string(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME));
            enableExtensions.emplace_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

        }

        uint32_t requiredQueueFamilyIndex = 0;
        std::vector<VkDeviceQueueCreateInfo> pQueueCreateInfos(
            unityCreateInfo->pQueueCreateInfos,
            unityCreateInfo->pQueueCreateInfos + unityCreateInfo->queueCreateInfoCount);
        if (ResolveQueueFamily_RayTracer(physicalDevice, requiredQueueFamilyIndex))
        {
            // If we found a queue family, make our own queue
            // This was the case when I started development
            assert(pQueueCreateInfos.size() == 1);

            if (pQueueCreateInfos[0].queueFamilyIndex == requiredQueueFamilyIndex) {
                PFG_EDITORLOG("Created extra queue on existing family " + std::to_string(requiredQueueFamilyIndex));
                pQueueCreateInfos[0].queueCount = pQueueCreateInfos[0].queueCount + 1;

                Vulkan::RayTracer::Instance().queueFamilyIndex_ = requiredQueueFamilyIndex;
                Vulkan::RayTracer::Instance().queueIndex_ = pQueueCreateInfos[0].queueCount - 1;
            }
            else
            {
                VkDeviceQueueCreateInfo deviceQueueCreateInfo = {};
                deviceQueueCreateInfo.queueFamilyIndex = requiredQueueFamilyIndex;
                deviceQueueCreateInfo.queueCount = 1;

                PFG_EDITORLOG("Created new queue on family " + std::to_string(requiredQueueFamilyIndex));
                pQueueCreateInfos.emplace_back(deviceQueueCreateInfo);

                Vulkan::RayTracer::Instance().queueFamilyIndex_ = requiredQueueFamilyIndex;
                Vulkan::RayTracer::Instance().queueIndex_ = 0;
            }
        }
        
        // Otherwise, just create Unity's queue
        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext = &deviceFeatures;
        deviceCreateInfo.flags = 0;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(pQueueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = pQueueCreateInfos.data();
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enableExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = enableExtensions.data();
        deviceCreateInfo.pEnabledFeatures = nullptr; // unityCreateInfo->pEnabledFeatures - not using because using VkPhysicalDeviceFeatures2
        
        VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, unityAllocator, device);

        if (result == VK_SUCCESS)
        {
            Vulkan::RayTracer::CreateDeviceSuccess = true;
        }

        return result;
    }

    VkDevice RayTracer::NullDevice = VK_NULL_HANDLE;
    bool RayTracer::CreateDeviceSuccess = false;
#pragma endregion VulkanHooks


    RayTracer::RayTracer()
        : graphicsInterface_(nullptr)
        , queueFamilyIndex_(0)
        , queueIndex_(0)
    {}

    void RayTracer::InitializeFromUnityInstance(IUnityGraphicsVulkan* graphicsInterface)
    {
        graphicsInterface_ = graphicsInterface;
    }

    void RayTracer::Shutdown()
    {
        
    }


#pragma region RayTracerAPI

    bool RayTracer::ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces)
    {
        switch (type)
        {
        case kUnityGfxDeviceEventInitialize:
        {
            PFG_EDITORLOG("Processing kUnityGfxDeviceEventInitialize...");

            auto graphicsInterface = interfaces->Get<IUnityGraphicsVulkan>();

            auto eventConfig = UnityVulkanPluginEventConfig();
            eventConfig.graphicsQueueAccess = kUnityVulkanGraphicsQueueAccess_Allow;
            eventConfig.renderPassPrecondition = kUnityVulkanRenderPass_EnsureOutside;
            eventConfig.flags = kUnityVulkanEventConfigFlag_EnsurePreviousFrameSubmission | kUnityVulkanEventConfigFlag_ModifiesCommandBuffersState;
            graphicsInterface->ConfigureEvent(1, &eventConfig);

            if (CreateDeviceSuccess == false)
            {
                PFG_EDITORLOG("Ray Tracing Plugin initialization failed.  Check that plugin is loading at startup");
                return false;
            }

            // Finish setting up RayTracer instance
            InitializeFromUnityInstance(graphicsInterface);

        }
        break;

        case kUnityGfxDeviceEventBeforeReset:
        {
            PFG_EDITORLOG("Processing kUnityGfxDeviceEventBeforeReset...");
        }
        break;

        case kUnityGfxDeviceEventAfterReset:
        {
            PFG_EDITORLOG("Processing kUnityGfxDeviceEventAfterReset...");
        }
        break;

        case kUnityGfxDeviceEventShutdown:
        {
            PFG_EDITORLOG("Processing kUnityGfxDeviceEventShutdown...");

            if (CreateDeviceSuccess == false)
            {
                // Nothing to do
                return false;
            }

            Shutdown();
        }
        break;
        }

        return true;
    }

    void* RayTracer::GetInstance()
    {
        return (void*)graphicsInterface_->Instance().instance;
    }

    void* RayTracer::GetDevice() 
    {
        return (void*)graphicsInterface_->Instance().device;
    }

    void* RayTracer::GetPhysicalDevice()
    {
        return (void*)graphicsInterface_->Instance().physicalDevice;
    }

    void* RayTracer::GetImageFromTexture(void* nativeTexturePtr)
    {
        UnityVulkanImage image;
        if (!graphicsInterface_->AccessTexture(
                nativeTexturePtr,
                UnityVulkanWholeImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                VK_ACCESS_SHADER_READ_BIT,
                kUnityVulkanResourceAccess_PipelineBarrier,
                &image))
        {
            PFG_EDITORLOGERROR("Cannot get accesss texture to add");
            return nullptr;
        }

        return image.image;
    }

    unsigned long long RayTracer::GetSafeFrameNumber()
    {
        UnityVulkanRecordingState unityRecordingState;
        if (!graphicsInterface_->CommandRecordingState(&unityRecordingState, kUnityVulkanGraphicsQueueAccess_DontCare))
        {
            return ULLONG_MAX;
        }

        return unityRecordingState.safeFrameNumber;
    }

    uint32_t RayTracer::GetQueueFamilyIndex()
    {
        return Vulkan::RayTracer::Instance().queueFamilyIndex_;
    }

    uint32_t RayTracer::GetQueueIndex()
    {
        return Vulkan::RayTracer::Instance().queueIndex_;
    }

    void RayTracer::CmdTraceRaysKHR(void* data)
    {
        graphicsInterface_->EnsureInsideRenderPass();

        PFG_EDITORLOG("CmdTraceRaysKHR was called")

        UnityVulkanRecordingState unityRecordingState;
        if (!graphicsInterface_->CommandRecordingState(&unityRecordingState, kUnityVulkanGraphicsQueueAccess_DontCare))
        {
            return;
        }

        auto commandData = (vkCmdTraceRaysKHRData*)data;

        UnityVulkanImage dstImage;
        if (!graphicsInterface_->AccessTexture(
                commandData->dstTexture,
                UnityVulkanWholeImage,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                kUnityVulkanResourceAccess_PipelineBarrier,
                &dstImage
            ))
        {
            return;
        }


        // Dispatch the ray tracing commands
        vkCmdBindPipeline(
            unityRecordingState.commandBuffer, 
            VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, 
            commandData->pipeline);

        vkCmdBindDescriptorSets(
            unityRecordingState.commandBuffer,
            VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
            commandData->pipelineLayout,
            0,
            commandData->descriptorCount,
            commandData->pDescriptorSets,
            0,
            0);

        // Update image barrier Make into a storage image
        VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        
        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_NONE;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageMemoryBarrier.image = commandData->offscreenImage;
        imageMemoryBarrier.subresourceRange = range;

        vkCmdPipelineBarrier(
            unityRecordingState.commandBuffer, 
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 
            0, 
            0, 
            nullptr, 
            0, 
            nullptr, 
            1, 
            &imageMemoryBarrier);

        vkCmdTraceRaysKHR(
            unityRecordingState.commandBuffer,
            &commandData->raygenShaderEntry,
            &commandData->missShaderEntry,
            &commandData->hitShaderEntry,
            &commandData->callableShaderEntry,
            commandData->extent.width,
            commandData->extent.height,
            commandData->extent.depth);


        // Copy result
          // src
        imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imageMemoryBarrier.image = commandData->offscreenImage;
        imageMemoryBarrier.subresourceRange = range;

        vkCmdPipelineBarrier(
            unityRecordingState.commandBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &imageMemoryBarrier);

        // dst 
        imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_NONE;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.image = dstImage.image;
        imageMemoryBarrier.subresourceRange = range;

        vkCmdPipelineBarrier(
            unityRecordingState.commandBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &imageMemoryBarrier);

        VkImageCopy imageCopy = {};
        imageCopy.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        imageCopy.srcOffset = { 0, 0, 0 };
        imageCopy.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        imageCopy.dstOffset = { 0, 0, 0 };
        imageCopy.extent = commandData->extent;

        vkCmdCopyImage(
            unityRecordingState.commandBuffer,
            commandData->offscreenImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dstImage.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &imageCopy);

    }

    void RayTracer::Blit(void* data) {

        // cannot do resource uploads inside renderpass
        graphicsInterface_->EnsureOutsideRenderPass();

        auto blitData = (BlitData*)data;

        UnityVulkanRecordingState recordingState;
        if (!graphicsInterface_->CommandRecordingState(&recordingState, kUnityVulkanGraphicsQueueAccess_DontCare))
        {
            PFG_EDITORLOGERROR("Cannot get recording state for blit");
            return;
        }

        UnityVulkanImage dstImage;
        if (!graphicsInterface_->AccessTexture(
            blitData->dstHandle,
            UnityVulkanWholeImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            kUnityVulkanResourceAccess_PipelineBarrier,
            &dstImage))
        {
            PFG_EDITORLOGERROR("Cannot access destination for blit");
            return;
        }

        // Setup source image barrier
        VkImageMemoryBarrier srcImageMemoryBarrier;
        srcImageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        srcImageMemoryBarrier.pNext = nullptr;
        srcImageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        srcImageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        srcImageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        srcImageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        srcImageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        srcImageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        srcImageMemoryBarrier.image = blitData->srcImage;
        srcImageMemoryBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };

        vkCmdPipelineBarrier(
            recordingState.commandBuffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            0, 0, nullptr, 0, nullptr, 1,
            &srcImageMemoryBarrier);

        VkImageCopy imageCopy;
        imageCopy.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        imageCopy.srcOffset = { 0, 0, 0 };
        imageCopy.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
        imageCopy.dstOffset = { 0, 0, 0 };
        imageCopy.extent = blitData->extent;

        vkCmdCopyImage(
            recordingState.commandBuffer,
            blitData->srcImage,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            dstImage.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &imageCopy);
    }

    struct TraceRaysData {
        VkPipelineBindPoint pipelineBindPoint;
        VkPipeline pipeline;
    };

    void RayTracer::TraceRays(void* data) {
        graphicsInterface_->EnsureInsideRenderPass();

        auto traceRaysData = reinterpret_cast<TraceRaysData*>(data);

        UnityVulkanRecordingState recordingState;
        if (!graphicsInterface_->CommandRecordingState(&recordingState, kUnityVulkanGraphicsQueueAccess_DontCare))
        {
            return;
        }

        vkCmdBindPipeline(
            recordingState.commandBuffer,
            traceRaysData->pipelineBindPoint,
            traceRaysData->pipeline);

        //vkCmdBindDescriptorSets(
        //  commandBuffer,
        //  VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        //  pipelineLayout_, 0,
        //  static_cast<uint32_t>(descriptorSetsData.descriptorSets.size()), descriptorSetsData.descriptorSets.data(),
        //  0, 0);

        //VkStridedDeviceAddressRegionKHR raygenShaderEntry = {};
        //raygenShaderEntry.deviceAddress = shaderBindingTable_.GetBuffer().GetBufferDeviceAddress().deviceAddress + shaderBindingTable_.GetRaygenOffset();
        //raygenShaderEntry.stride = shaderBindingTable_.GetGroupsStride();
        //raygenShaderEntry.size = shaderBindingTable_.GetRaygenSize();

        //VkStridedDeviceAddressRegionKHR missShaderEntry{};
        //missShaderEntry.deviceAddress = shaderBindingTable_.GetBuffer().GetBufferDeviceAddress().deviceAddress + shaderBindingTable_.GetMissGroupsOffset();
        //missShaderEntry.stride = shaderBindingTable_.GetGroupsStride();
        //missShaderEntry.size = shaderBindingTable_.GetMissGroupsSize();

        //VkStridedDeviceAddressRegionKHR hitShaderEntry{};
        //hitShaderEntry.deviceAddress = shaderBindingTable_.GetBuffer().GetBufferDeviceAddress().deviceAddress + shaderBindingTable_.GetHitGroupsOffset();
        //hitShaderEntry.stride = shaderBindingTable_.GetGroupsStride();
        //hitShaderEntry.size = shaderBindingTable_.GetHitGroupsSize();

        //VkStridedDeviceAddressRegionKHR callableShaderEntry{};

        //// Dispatch the ray tracing commands
        //vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline_);
        //vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout_, 0, static_cast<uint32_t>(descriptorSetsData.descriptorSets.size()), descriptorSetsData.descriptorSets.data(), 0, 0);

        //// Make into a storage image
        //VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        //Vulkan::Image::UpdateImageBarrier(
        //    commandBuffer,
        //    renderTargets_[cameraInstanceId]->stagingImage.GetImage(),
        //    range,
        //    0, VK_ACCESS_SHADER_WRITE_BIT,
        //    VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        ////PFG_EDITORLOG("Tracing for " + std::to_string(cameraInstanceId));

        //vkCmdTraceRaysKHR(
        //    commandBuffer,
        //    &raygenShaderEntry,
        //    &missShaderEntry,
        //    &hitShaderEntry,
        //    &callableShaderEntry,
        //    renderTargets_[cameraInstanceId]->extent.width,
        //    renderTargets_[cameraInstanceId]->extent.height,
        //    renderTargets_[cameraInstanceId]->extent.depth);
    }

#pragma endregion RayTracerAPI

}
