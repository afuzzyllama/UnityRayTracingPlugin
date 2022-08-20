#include "RayTracer.h"

#include <assert.h>
#include <vector>
#include "../Helpers.h"

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
    void ResolvePropertiesAndQueues_RayTracer(VkPhysicalDevice physicalDevice) {

        // find our queues
        const VkQueueFlagBits askingFlags[3] = { VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_TRANSFER_BIT };
        uint32_t queuesIndices[2] = { ~0u, ~0u };

        // Call once to get the count
        uint32_t queueFamilyPropertyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

        // Call again to populate vector
        std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

        // Locate which queueFamilyProperties index 
        for (size_t i = 0; i < 2; ++i) {
            const VkQueueFlagBits flag = askingFlags[i];
            uint32_t& queueIdx = queuesIndices[i];

            if ((flag & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT) {
                for (uint32_t j = 0; j < queueFamilyPropertyCount; ++j) {
                    if (((queueFamilyProperties[j].queueFlags & VK_QUEUE_TRANSFER_BIT) == VK_QUEUE_TRANSFER_BIT) &&
                        !((queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) == VK_QUEUE_GRAPHICS_BIT)) {
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

        if (queuesIndices[0] == ~0u || queuesIndices[1] == ~0u) {
            PFG_EDITORLOGERROR("Could not find queues to support all required flags");
            return;
        }

        PixelsForGlory::Vulkan::RayTracer::Instance().graphicsQueueFamilyIndex_ = queuesIndices[0];
        PixelsForGlory::Vulkan::RayTracer::Instance().transferQueueFamilyIndex_ = queuesIndices[1];

        PFG_EDITORLOG("Queues indices successfully reoslved");
    };
    


    /// <summary>
    /// Injects what is required for ray tracing into vkCreateInstance
    /// </summary>
    /// <param name="unityCreateInfo"></param>
    /// <param name="unityAllocator"></param>
    /// <param name="instance"></param>
    /// <returns></returns>
    VkResult CreateInstance_RayTracer(const VkInstanceCreateInfo* unityCreateInfo, const VkAllocationCallbacks* unityAllocator, VkInstance* instance)
    {
        VkResult result = vkCreateInstance(unityCreateInfo, unityAllocator, instance);
        VK_CHECK("vkCreateInstance", result);

        return result;
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

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO; 
        deviceCreateInfo.pNext = &deviceFeatures;
        deviceCreateInfo.flags = 0;
        deviceCreateInfo.queueCreateInfoCount = unityCreateInfo->queueCreateInfoCount;
        deviceCreateInfo.pQueueCreateInfos = unityCreateInfo->pQueueCreateInfos;
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(enableExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = enableExtensions.data();
        deviceCreateInfo.pEnabledFeatures = unityCreateInfo->pEnabledFeatures;

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

            UnityVulkanPluginEventConfig eventConfig;
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
