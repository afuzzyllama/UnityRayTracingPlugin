#include "RayTracer.h"

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

            if (flag == VK_QUEUE_TRANSFER_BIT) {
                for (uint32_t j = 0; j < queueFamilyPropertyCount; ++j) {
                    if ((queueFamilyProperties[j].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
                        !(queueFamilyProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
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

        // Get the ray tracing pipeline properties, which we'll need later on in the sample
        PixelsForGlory::Vulkan::RayTracer::Instance().rayTracingProperties_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

        VkPhysicalDeviceProperties2 physicalDeviceProperties = { };
        physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        physicalDeviceProperties.pNext = &PixelsForGlory::Vulkan::RayTracer::Instance().rayTracingProperties_;

        PFG_EDITORLOG("Getting physical device properties");
        vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties);

        // Get memory properties
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &PixelsForGlory::Vulkan::RayTracer::Instance().physicalDeviceMemoryProperties_);
    }

    /// <summary>
    /// Injects what is required for ray tracing into vkCreateInstance
    /// </summary>
    /// <param name="unityCreateInfo"></param>
    /// <param name="unityAllocator"></param>
    /// <param name="instance"></param>
    /// <returns></returns>
    VkResult CreateInstance_RayTracer(const VkInstanceCreateInfo* unityCreateInfo, const VkAllocationCallbacks* unityAllocator, VkInstance* instance)
    {
        // Copy ApplicationInfo from Unity and force it up to Vulkan 1.2
        VkApplicationInfo applicationInfo;
        applicationInfo.sType = unityCreateInfo->pApplicationInfo->sType;
        applicationInfo.apiVersion = VK_API_VERSION_1_2;
        applicationInfo.applicationVersion = unityCreateInfo->pApplicationInfo->applicationVersion;
        applicationInfo.engineVersion = unityCreateInfo->pApplicationInfo->engineVersion;
        applicationInfo.pApplicationName = unityCreateInfo->pApplicationInfo->pApplicationName;
        applicationInfo.pEngineName = unityCreateInfo->pApplicationInfo->pEngineName;
        applicationInfo.pNext = unityCreateInfo->pApplicationInfo->pNext;

        // Define Vulkan create information for instance
        VkInstanceCreateInfo createInfo;
        createInfo.pNext = nullptr;
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.flags = 0;
        createInfo.pApplicationInfo = &applicationInfo;

        // At time of writing this was 0, make sure Unity didn't change anything
        assert(unityCreateInfo->enabledLayerCount == 0);

        // Depending if we have validations layers defined, populate debug messenger create info
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
        const char* layerName = "VK_LAYER_KHRONOS_validation";

        // TODO: debug flag
        if (/*_debug*/ false) {
            createInfo.enabledLayerCount = 1;
            createInfo.ppEnabledLayerNames = &layerName;

            debugCreateInfo = {};
            debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugCreateInfo.pfnUserCallback = debugCallback;

            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        // Gather extensions required for instaqnce
        std::vector<const char*> requiredExtensions = {
            VK_KHR_DISPLAY_EXTENSION_NAME,
            VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
            VK_KHR_SURFACE_EXTENSION_NAME,
    #ifdef WIN32
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    #endif
            VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
            VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME,

            /*VK_EXT_DEBUG_UTILS_EXTENSION_NAME*/  // Enable when debugging of Vulkan is needed
        };

        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();

        vkCreateInstance = (PFN_vkCreateInstance)vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance");
        VkResult result = vkCreateInstance(&createInfo, unityAllocator, instance);
        VK_CHECK("vkCreateInstance", result);

        if (result == VK_SUCCESS)
        {
            volkLoadInstance(*instance);
        }

        // Unity automatically picks up on VkDebugUtilsMessengerCreateInfoEXT
        // If for some reason it doesn't, we'd hook up a debug callback here
        //if (/*_debug*/ true) {
        //    if (CreateDebugUtilsMessengerEXT(*instance, &debugCreateInfo, nullptr, &PixelsForGlory::Vulkan::RayTracer::Instance().debugMessenger_) != VK_SUCCESS) 
        //    {
        //        PFG_EDITORLOGERROR("failed to set up debug messenger!");
        //    }
        //}

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
        ResolvePropertiesAndQueues_RayTracer(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
        const float priority = 0.0f;

        //  Setup device queue
        VkDeviceQueueCreateInfo deviceQueueCreateInfo;
        deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueCreateInfo.pNext = nullptr;
        deviceQueueCreateInfo.flags = 0;
        deviceQueueCreateInfo.queueFamilyIndex = PixelsForGlory::Vulkan::RayTracer::Instance().graphicsQueueFamilyIndex_;
        deviceQueueCreateInfo.queueCount = 1;
        deviceQueueCreateInfo.pQueuePriorities = &priority;
        deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);

        // Determine if we have individual queues that need to be added 
        if (PixelsForGlory::Vulkan::RayTracer::Instance().transferQueueFamilyIndex_ != PixelsForGlory::Vulkan::RayTracer::Instance().graphicsQueueFamilyIndex_) {
            deviceQueueCreateInfo.queueFamilyIndex = PixelsForGlory::Vulkan::RayTracer::Instance().transferQueueFamilyIndex_;
            deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
        }

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
            VK_KHR_SPIRV_1_4_EXTENSION_NAME,

            // Required by SPRIV 1.4
            VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,

            // Required by acceleration structure
            VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
            VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
        };

        for (auto const& ext : requiredExtensions) {
            PFG_EDITORLOG("Enabling extension: " + std::string(ext));
        }

        VkDeviceCreateInfo deviceCreateInfo;
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pNext = &deviceFeatures;
        deviceCreateInfo.flags = VK_NO_FLAGS;
        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
        deviceCreateInfo.enabledLayerCount = 0;
        deviceCreateInfo.ppEnabledLayerNames = nullptr;
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = requiredExtensions.data();
        deviceCreateInfo.pEnabledFeatures = nullptr; // Must be null since pNext != nullptr

        // Inject parts of Unity's VkDeviceCreateInfo just in case
        deviceCreateInfo.flags = unityCreateInfo->flags;
        deviceCreateInfo.enabledLayerCount = unityCreateInfo->enabledLayerCount;
        deviceCreateInfo.ppEnabledLayerNames = unityCreateInfo->ppEnabledLayerNames;

        VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, unityAllocator, device);


        // get our queues handles with our new logical device!
        PFG_EDITORLOG("Getting device queues");
        vkGetDeviceQueue(*device, PixelsForGlory::Vulkan::RayTracer::Instance().graphicsQueueFamilyIndex_, 0, &PixelsForGlory::Vulkan::RayTracer::Instance().graphicsQueue_);
        vkGetDeviceQueue(*device, PixelsForGlory::Vulkan::RayTracer::Instance().transferQueueFamilyIndex_, 0, &PixelsForGlory::Vulkan::RayTracer::Instance().transferQueue_);

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
        , graphicsQueueFamilyIndex_(0)
        , transferQueueFamilyIndex_(0)
        , graphicsQueue_(VK_NULL_HANDLE)
        , transferQueue_(VK_NULL_HANDLE)
        , graphicsCommandPool_(VK_NULL_HANDLE)
        , transferCommandPool_(VK_NULL_HANDLE)
        , physicalDeviceMemoryProperties_(VkPhysicalDeviceMemoryProperties())
        , rayTracingProperties_(VkPhysicalDeviceRayTracingPipelinePropertiesKHR())
        , device_(NullDevice)
        , alreadyPrepared_(false)
        , rebuildTlas_(true)
        , updateTlas_(false)
        , tlas_(RayTracerAccelerationStructure())
        , descriptorPool_(VK_NULL_HANDLE)
        , sceneBufferInfo_(VkDescriptorBufferInfo())
        , pipelineLayout_(VK_NULL_HANDLE)
        , pipeline_(VK_NULL_HANDLE)
        , debugMessenger_(VK_NULL_HANDLE)
        , noLight_(Vulkan::Buffer())
        , noLightBufferInfo_(VkDescriptorBufferInfo())
    {}

    void RayTracer::InitializeFromUnityInstance(IUnityGraphicsVulkan* graphicsInterface)
    {
        graphicsInterface_ = graphicsInterface;
        device_ = graphicsInterface_->Instance().device;
    }

    void RayTracer::Shutdown()
    {
        if (debugMessenger_ != VK_NULL_HANDLE)
        {
            vkDestroyDebugUtilsMessengerEXT(graphicsInterface_->Instance().instance, debugMessenger_, nullptr);
            debugMessenger_ = VK_NULL_HANDLE;
        }

        for (auto itr = renderTargets_.begin(); itr != renderTargets_.end(); ++itr)
        {
            auto& renderTarget = (*itr).second;

            renderTarget->stagingImage.Destroy();
            renderTarget->cameraData.Destroy();

            for(auto itr = renderTarget->descriptorSetsData.begin(); itr != renderTarget->descriptorSetsData.end(); ++itr)
            {
                auto& descriptorSetsData = (*itr).second;
                vkFreeDescriptorSets(device_, descriptorPool_, DESCRIPTOR_SET_SIZE, descriptorSetsData.descriptorSets.data());
            }
            renderTarget->descriptorSetsData.clear();

            renderTarget.release();
        }
        renderTargets_.clear();
        

        for (auto meshItr = meshInstancePool_.pool_begin(); meshItr != meshInstancePool_.pool_end(); ++meshItr)
        {
            auto& instance = (*meshItr);
            instance.instanceData.Destroy();
        }

        for (auto itr = sharedMeshesPool_.pool_begin(); itr != sharedMeshesPool_.pool_end(); ++itr)
        {
            auto& mesh = (*itr);

            mesh->vertexBuffer.Destroy();
            mesh->indexBuffer.Destroy();
            mesh->attributesBuffer.Destroy();
            mesh->facesBuffer.Destroy();
            
            if (mesh->blas.accelerationStructure != VK_NULL_HANDLE)
            {
                vkDestroyAccelerationStructureKHR(device_, mesh->blas.accelerationStructure, nullptr);
                mesh->blas.accelerationStructure = VkAccelerationStructureKHR();
                mesh->blas.buffer.Destroy();
                mesh->blas.deviceAddress = 0;
            }
        }

        sceneData_.Destroy();
        noLight_.Destroy();

        for (auto itr = lightsPool_.pool_begin(); itr != lightsPool_.pool_end(); ++itr)
        {
            auto& item = (*itr);
            item->Destroy();
            item.release();
        }

        blankTexture_.Destroy();

        for (auto itr = texturePool_.pool_begin(); itr != texturePool_.pool_end(); ++itr)
        {
            auto& item = (*itr);
            item->Destroy();
            item.release();
        }

        defaultMaterial_.Destroy();
        

        for (auto itr = materialPool_.pool_begin(); itr != materialPool_.pool_end(); ++itr)
        {
            auto& item = (*itr);
            item->Destroy();
            item.release();
        }

        for (auto itr = garbageBuffers_.begin(); itr != garbageBuffers_.end(); ++itr)
        {
            auto& item = (*itr);
            item.buffer->Destroy();
            item.buffer.release();
        }

        instancesAccelerationStructuresBuffer_.Destroy();

        if (tlas_.accelerationStructure != VK_NULL_HANDLE)
        {
            vkDestroyAccelerationStructureKHR(device_, tlas_.accelerationStructure, nullptr);
        }
        tlas_.buffer.Destroy();

        if (descriptorPool_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
            descriptorPool_ = VK_NULL_HANDLE;
        }

        shaderBindingTable_.Destroy();

        if (pipeline_ != VK_NULL_HANDLE) 
        {
            vkDestroyPipeline(device_, pipeline_, nullptr);
            pipeline_ = VK_NULL_HANDLE;
        }

        if (pipelineLayout_ != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
            pipelineLayout_ = VK_NULL_HANDLE;
        }

        for (auto descriptorSetLayout : descriptorSetLayouts_)
        {
            vkDestroyDescriptorSetLayout(device_, descriptorSetLayout, nullptr);
        }
        descriptorSetLayouts_.clear();


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

                // alternative way to intercept API
                //graphicsInterface_->InterceptVulkanAPI("vkCmdBeginRenderPass", (PFN_vkVoidFunction)Hook_vkCmdBeginRenderPass);
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

    void RayTracer::SetShaderFolder(std::wstring shaderFolder)
    {
        shaderFolder_ = shaderFolder;
        

        if (shaderFolder_.back() != '/' || shaderFolder_.back() != '\\')
        {
            shaderFolder_ = shaderFolder_ + L"/";
        }

        PFG_EDITORLOGW(L"Shader folder set to: " + shaderFolder_);
    }

    int RayTracer::SetRenderTarget(int cameraInstanceId, int unityTextureFormat, int width, int height, void* textureHandle)
    {
        VkFormat vkFormat;
        switch (unityTextureFormat)
        {
        
            //// RenderTextureFormat.ARGB32
        //case 0:
        //    vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
        //    break;
        
        // TextureFormat.RGBA32
        case 4:
            vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
            break;
        
        default:
            PFG_EDITORLOGERROR("Attempted to set an unsupported Unity texture format" + std::to_string(unityTextureFormat));
            return 0;
        }

        if (renderTargets_.find(cameraInstanceId) != renderTargets_.end())
        {
            renderTargets_[cameraInstanceId]->cameraData.Destroy();
            renderTargets_[cameraInstanceId]->stagingImage.Destroy();
        }
        else
        {
            renderTargets_.insert(std::make_pair(cameraInstanceId, std::make_unique<RayTracerRenderTarget>()));
        }

        auto& renderTarget = renderTargets_[cameraInstanceId];

        renderTarget->format = vkFormat;
        renderTarget->extent.width = width;
        renderTarget->extent.height = height;
        renderTarget->extent.depth = 1;
        renderTarget->destination = textureHandle;
        
        renderTarget->stagingImage = Vulkan::Image(device_, physicalDeviceMemoryProperties_);

        if(renderTarget->cameraData.Create(
            "cameraData",
            device_,
            physicalDeviceMemoryProperties_,
            sizeof(ShaderCameraData),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            Vulkan::Buffer::kDefaultMemoryPropertyFlags) != VK_SUCCESS)
        {

            PFG_EDITORLOGERROR("Failed to create camera data!");
            renderTarget->stagingImage.Destroy();
            return 0;
        }

        if (renderTarget->stagingImage.Create(
            "renderTarget",
            VK_IMAGE_TYPE_2D,
            renderTarget->format,
            renderTarget->extent,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != VK_SUCCESS) 
        {

            PFG_EDITORLOGERROR("Failed to create render image!");
            renderTarget->stagingImage.Destroy();
            return 0;
        }

        VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        if(renderTarget->stagingImage.CreateImageView(VK_IMAGE_VIEW_TYPE_2D, renderTarget->format, range) != VK_SUCCESS) {
            PFG_EDITORLOGERROR("Failed to create render image view!");
            renderTarget->stagingImage.Destroy();
            return 0;
        }

        return 1;
    }
        
    RayTracerAPI::AddResourceResult RayTracer::AddSharedMesh(int sharedMeshInstanceId, float* verticesArray, float* normalsArray, float* tangentsArray, float* uvsArray, int vertexCount, int* indicesArray, int indexCount)
    { 
        // Check that this shared mesh hasn't been added yet
        if (sharedMeshesPool_.find(sharedMeshInstanceId) != sharedMeshesPool_.in_use_end())
        {
            return RayTracerAPI::AddResourceResult::AlreadyExists;
        }
      
        // We can only add tris, make sure the index count reflects this
        assert(indexCount % 3 == 0);
    
        auto sentMesh = std::make_unique<RayTracerMeshSharedData>();
        
        // Setup where we are going to store the shared mesh data and all data needed for shaders
        sentMesh->sharedMeshInstanceId = sharedMeshInstanceId;
        sentMesh->vertexCount = vertexCount;
        sentMesh->indexCount = indexCount;
    
        // Setup buffers
        bool success = true;
        if (sentMesh->vertexBuffer.Create(
                "vertexBuffer",
                device_,
                physicalDeviceMemoryProperties_,
                sizeof(vec3) * sentMesh->vertexCount,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                Vulkan::Buffer::kDefaultMemoryPropertyFlags) 
            != VK_SUCCESS)
        {
            PFG_EDITORLOGERROR("Failed to create vertex buffer for shared mesh instance id " + std::to_string(sharedMeshInstanceId));
            success = false;
        }
    
        if (sentMesh->indexBuffer.Create(
                "indexBuffer",
                device_,
                physicalDeviceMemoryProperties_,
                sizeof(uint32_t) * sentMesh->indexCount,
                VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                Vulkan::Buffer::kDefaultMemoryPropertyFlags)
            != VK_SUCCESS)
        {
            PFG_EDITORLOGERROR("Failed to create index buffer for shared mesh instance id " + std::to_string(sharedMeshInstanceId));
            success = false;
        }
    
        if (sentMesh->attributesBuffer.Create(
                "attributesBuffer",
                device_,
                physicalDeviceMemoryProperties_,
                sizeof(ShaderVertexAttributeData) * sentMesh->vertexCount,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                Vulkan::Buffer::kDefaultMemoryPropertyFlags)
            != VK_SUCCESS)
        {
            PFG_EDITORLOGERROR("Failed to create vertex attribute buffer for shared mesh instance id " + std::to_string(sharedMeshInstanceId));
            success = false;
        }
    
    
        if (sentMesh->facesBuffer.Create(
                "facesBuffer",
                device_,
                physicalDeviceMemoryProperties_,
                sizeof(ShaderFaceData) * sentMesh->indexCount / 3,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                Vulkan::Buffer::kDefaultMemoryPropertyFlags)
            != VK_SUCCESS)
        {
            PFG_EDITORLOGERROR("Failed to create face buffer for shared mesh instance id " + std::to_string(sharedMeshInstanceId));
            success = false;
        }
    
        
        if (!success)
        {
            return RayTracerAPI::AddResourceResult::Error;
        }
    
        // Creating buffers was successful.  Move onto getting the data in there
        auto vertices = reinterpret_cast<vec3*>(sentMesh->vertexBuffer.Map());
        auto indices = reinterpret_cast<uint32_t*>(sentMesh->indexBuffer.Map());
        auto vertexAttributes = reinterpret_cast<ShaderVertexAttributeData*>(sentMesh->attributesBuffer.Map());
        auto faces = reinterpret_cast<ShaderFaceData*>(sentMesh->facesBuffer.Map());
        
        // verticesArray and normalsArray are size vertexCount * 3 since they actually represent an array of vec3
        // uvsArray is size vertexCount * 2 since it actually represents an array of vec2
        for (int i = 0; i < vertexCount; ++i)
        {
            // Build for acceleration structure
            vertices[i].x = verticesArray[3 * i + 0];
            vertices[i].y = verticesArray[3 * i + 1];
            vertices[i].z = verticesArray[3 * i + 2];
            
            //PFG_EDITORLOG("vertex #" + std::to_string(i) + ": " + std::to_string(vertices[i].x) + ", " + std::to_string(vertices[i].y) + ", " + std::to_string(vertices[i].z));

            // Build for shader
            vertexAttributes[i].normal.x = normalsArray[3 * i + 0];
            vertexAttributes[i].normal.y = normalsArray[3 * i + 1];
            vertexAttributes[i].normal.z = normalsArray[3 * i + 2];
    
            vertexAttributes[i].tangent.x = tangentsArray[4 * i + 0];
            vertexAttributes[i].tangent.y = tangentsArray[4 * i + 1];
            vertexAttributes[i].tangent.z = tangentsArray[4 * i + 2];
            vertexAttributes[i].tangent.w = tangentsArray[4 * i + 3];

            vertexAttributes[i].uv.x = uvsArray[2 * i + 0];
            vertexAttributes[i].uv.y = uvsArray[2 * i + 1];
        }
        
        
        for (int i = 0; i < indexCount / 3; ++i)
        {
            // Build for acceleration structure
            indices[3 * i + 0] = static_cast<uint32_t>(indicesArray[3 * i + 0]);
            indices[3 * i + 1] = static_cast<uint32_t>(indicesArray[3 * i + 1]);
            indices[3 * i + 2] = static_cast<uint32_t>(indicesArray[3 * i + 2]);

            //PFG_EDITORLOG("face #" + std::to_string(i) + ": " + std::to_string(indices[3 * i + 0]) + ", " + std::to_string(indices[3 * i + 1]) + ", " + std::to_string(indices[3 * i + 2]));

            // Build for shader
            faces[i].index0 = static_cast<uint32_t>(indicesArray[3 * i + 0]);
            faces[i].index1 = static_cast<uint32_t>(indicesArray[3 * i + 1]);
            faces[i].index2 = static_cast<uint32_t>(indicesArray[3 * i + 2]);
        }
        
        sentMesh->vertexBuffer.Unmap();
        sentMesh->indexBuffer.Unmap();
        sentMesh->attributesBuffer.Unmap();
        sentMesh->facesBuffer.Unmap();

        // All done creating the data, get it added to the pool
        sharedMeshesPool_.add(sharedMeshInstanceId, std::move(sentMesh));
    
        // Build blas here so we don't have to do it later
        BuildBlas(sharedMeshInstanceId);
    
        PFG_EDITORLOG("Added mesh (sharedMeshInstanceId: " + std::to_string(sharedMeshInstanceId) + ")");
    
        return RayTracerAPI::AddResourceResult::Success;
    
    }

    RayTracerAPI::AddResourceResult RayTracer::AddTlasInstance(int gameObjectInstanceId, int sharedMeshInstanceId, int materialInstanceId, float* l2wMatrix, float* w2lMatrix)
    { 
        if (meshInstancePool_.find(gameObjectInstanceId) != meshInstancePool_.in_use_end())
        {
            return RayTracerAPI::AddResourceResult::AlreadyExists;
        }

        meshInstancePool_.add(gameObjectInstanceId, RayTracerMeshInstanceData());
        auto& instance = meshInstancePool_[gameObjectInstanceId];

        instance.gameObjectInstanceId = gameObjectInstanceId;
        instance.sharedMeshInstanceId = sharedMeshInstanceId;
        instance.materialInstanceId   = materialInstanceId;
        FloatArrayToMatrix(l2wMatrix, instance.localToWorld);
        FloatArrayToMatrix(w2lMatrix, instance.worldToLocal);


        instance.instanceData.Create(
            "instanceData",
            device_,
            physicalDeviceMemoryProperties_,
            sizeof(ShaderInstanceData),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            Vulkan::Buffer::kDefaultMemoryPropertyFlags);
                
        PFG_EDITORLOG("Added mesh instance (gameObjectInstanceId: " + std::to_string(gameObjectInstanceId) + ", sharedMeshInstanceId: " + std::to_string(sharedMeshInstanceId) + ")");

        // If we added an instance, we need to rebuild the tlas
        rebuildTlas_ = true;

        return RayTracerAPI::AddResourceResult::Success;
    }

    void RayTracer::UpdateTlasInstance(int gameObjectInstanceId, int materialInstanceId, float* l2wMatrix, float* w2lMatrix)
    {
        FloatArrayToMatrix(l2wMatrix, meshInstancePool_[gameObjectInstanceId].localToWorld);
        FloatArrayToMatrix(w2lMatrix, meshInstancePool_[gameObjectInstanceId].worldToLocal);
        meshInstancePool_[gameObjectInstanceId].materialInstanceId = materialInstanceId;
        updateTlas_ = true;
    }

    void RayTracer::RemoveTlasInstance(int gameObjectInstanceId)
    {
        meshInstancePool_.remove(gameObjectInstanceId);

        // If we added an instance, we need to rebuild the tlas
        rebuildTlas_ = true;
    }

    void RayTracer::Prepare() 
    {
        if (alreadyPrepared_)
        {
            return;
        }

        CreateDescriptorSetsLayouts();
        CreateDescriptorPool();

        // Create "light" for when there are no lights
        noLight_.Create(
            "noLight",
            device_,
            physicalDeviceMemoryProperties_,
            sizeof(ShaderLightingData),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            Vulkan::Buffer::kDefaultMemoryPropertyFlags);
        
        auto light = reinterpret_cast<ShaderLightingData*>(noLight_.Map());

        light->color = vec3(0.0f, 0.0f, 0.0f);
        light->bounceIntensity = 0.0f;
        light->intensity = 0.0f;
        light->range = 0.0f;
        light->spotAngle = 0.0f;
        light->type = SHADER_LIGHTTYPE_NONE;
        light->enabled = false;

        noLight_.Unmap();

        noLightBufferInfo_.buffer = noLight_.GetBuffer();
        noLightBufferInfo_.offset = 0;
        noLightBufferInfo_.range = noLight_.GetSize();

        // Create default material
        defaultMaterial_.Create(
            "defaultMaterial",
            device_,
            physicalDeviceMemoryProperties_,
            sizeof(ShaderMaterialData),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            Vulkan::Buffer::kDefaultMemoryPropertyFlags);

        auto material = reinterpret_cast<ShaderMaterialData*>(defaultMaterial_.Map());

        material->albedo = vec4(1.0f, 0.0f, 1.0f, 0.0);
        material->emission = vec4(0.0f);
        material->metallic = 0.0f;
        material->roughness = 0.0f;
        material->indexOfRefraction = 1.0f;
        material->albedoTextureSet = false;
        material->albedoTextureInstanceId = -1;
        material->albedoTextureIndex = 0;
        material->emissionTextureSet = false;
        material->emissionTextureInstanceId = -1;
        material->emissionTextureIndex = 0;
        material->normalTextureSet = false;
        material->normalTextureInstanceId = -1;
        material->normalTextureIndex = 0;
        material->metallicTextureSet = false;
        material->metallicTextureInstanceId = -1;
        material->metallicTextureIndex = 0;
        material->roughnessTextureSet = false;
        material->roughnessTextureInstanceId = -1;
        material->roughnessTextureIndex = 0;
        material->ambientOcclusionTextureSet = false;
        material->ambientOcclusionTextureInstanceId = -1;
        material->ambientOcclusionTextureIndex = 0;

        defaultMaterial_.Unmap();

        graphicsInterface_->EnsureOutsideRenderPass();

        
        
        // Create blank texture
        blankTexture_ = Vulkan::Image(device_, physicalDeviceMemoryProperties_);

        blankTexture_.Create(
            "blankTexture",
            VK_IMAGE_TYPE_2D,
            VK_FORMAT_R32G32B32A32_SFLOAT,
            { 8, 8, 1 },
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        VK_CHECK("vkCreateImageView", blankTexture_.CreateImageView(VK_IMAGE_VIEW_TYPE_2D, VK_FORMAT_R32G32B32A32_SFLOAT, subresourceRange));
        VK_CHECK("vkCreateSampler", blankTexture_.CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT));

        assert(blankTexture_.GetImageView() != VK_NULL_HANDLE);
        assert(blankTexture_.GetSampler() != VK_NULL_HANDLE);

        UnityVulkanRecordingState recordingState;
        if (graphicsInterface_->CommandRecordingState(&recordingState, kUnityVulkanGraphicsQueueAccess_DontCare))
        {
            Vulkan::Image::UpdateImageBarrier(
                recordingState.commandBuffer,
                blankTexture_.GetImage(),
                subresourceRange,
                VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        else
        {
            PFG_EDITORLOGERROR("Cannot get command buffer to update blank texture image barrier");
        }

        meshInstancesAttributesBufferInfos_.resize(0);
        meshInstancesFacesBufferInfos_.resize(0);
        textureImageInfos_.resize(0);
        materialBufferInfos_.resize(0);

        alreadyPrepared_ = true;
    }

    void RayTracer::ResetPipeline()
    {
        if (pipeline_ != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(device_, pipeline_, nullptr);
            pipeline_ = VK_NULL_HANDLE;
        }
    }

    void RayTracer::UpdateCamera(int cameraInstanceId, float* camPos, float* camDir, float* camUp, float* camRight, float camNear, float camFar, float camFov)
    {
        if (renderTargets_.find(cameraInstanceId) == renderTargets_.end())
        {
            // The camera isn't in the system yet, don't attempt to update
            return;
        }

        auto& renderTarget = renderTargets_[cameraInstanceId];

        auto camera = reinterpret_cast<ShaderCameraData*>(renderTarget->cameraData.Map());
        camera->position.x = camPos[0];
        camera->position.y = camPos[1];
        camera->position.z = camPos[2];

        camera->direction.x = camDir[0];
        camera->direction.y = camDir[1];
        camera->direction.z = camDir[2];

        camera->up.x = camUp[0];
        camera->up.y = camUp[1];
        camera->up.z = camUp[2];

        camera->right.x = camRight[0];
        camera->right.y = camRight[1];
        camera->right.z = camRight[2];

        camera->nearClipPlane = camNear;
        camera->farClipPlane = camFar;
        camera->fieldOfView = camFov;

        renderTarget->cameraData.Unmap();
        
        //PFG_EDITORLOG("Updated camera " + std::to_string(cameraInstanceId));
    }

    void RayTracer::UpdateSceneData(float* color) 
    {
        if (sceneData_.GetBuffer() == VK_NULL_HANDLE)
        {
            sceneData_.Create(
                "sceneData",
                device_,
                physicalDeviceMemoryProperties_,
                sizeof(ShaderSceneData),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                Vulkan::Buffer::kDefaultMemoryPropertyFlags);
        }
    
        auto scene = reinterpret_cast<ShaderSceneData*>(sceneData_.Map());
        scene->ambient.r = color[0];
        scene->ambient.g = color[1];
        scene->ambient.b = color[2];
        scene->ambient.a = color[3];
        
        // While we're here, update this for good measure :D
        scene->lightCount = static_cast<uint32_t>(lightsPool_.in_use_size());
        
        sceneData_.Unmap();
    }

    RayTracerAPI::AddResourceResult RayTracer::AddLight(int lightInstanceId, float x, float y, float z, float r, float g, float b, float bounceIntensity, float intensity, float range, float spotAngle, int type, bool enabled)
    {
        if (lightsPool_.find(lightInstanceId) != lightsPool_.in_use_end())
        {
            return RayTracerAPI::AddResourceResult::AlreadyExists;
        }

        auto buffer = std::make_unique<Vulkan::Buffer>();

        lightsPool_.add(lightInstanceId, std::move(buffer));

        auto& light = lightsPool_[lightInstanceId];

        light->Create(
            "light",
            device_,
            physicalDeviceMemoryProperties_,
            sizeof(ShaderLightingData),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            Vulkan::Buffer::kDefaultMemoryPropertyFlags);
        
        UpdateLight(lightInstanceId, x, y, z, r, g, b, bounceIntensity, intensity, range, spotAngle, type, enabled);
        PFG_EDITORLOG("Added light (lightInstanceId: " + std::to_string(lightInstanceId) + ")");

        return RayTracerAPI::AddResourceResult::Success;
    }

    void RayTracer::UpdateLight(int lightInstanceId, float x, float y, float z, float r, float g, float b, float bounceIntensity, float intensity, float range, float spotAngle, int type, bool enabled)
    {
        auto light = reinterpret_cast<ShaderLightingData*>(lightsPool_[lightInstanceId]->Map());

        light->position = vec3(x, y, z);
        light->color = vec3(r, g, b);
        light->bounceIntensity = bounceIntensity;
        light->intensity = intensity;
        light->range = range;
        light->spotAngle = spotAngle;

        switch (type)
        {
        case SHADER_LIGHTTYPE_AREA:
            light->type = SHADER_LIGHTTYPE_AREA;
            break;

        case SHADER_LIGHTTYPE_DIRECTIONAL:
            light->type = SHADER_LIGHTTYPE_DIRECTIONAL;
            break;

        case SHADER_LIGHTTYPE_POINT:
            light->type = SHADER_LIGHTTYPE_POINT;
            break;

        case SHADER_LIGHTTYPE_SPOT:
            light->type = SHADER_LIGHTTYPE_SPOT;
            break;

        default:
            PFG_EDITORLOGERROR("Unrecognized light type: " + std::to_string(type) + ".");
            light->type = SHADER_LIGHTTYPE_NONE;
            break;
        }

        light->enabled = enabled;

        lightsPool_[lightInstanceId]->Unmap();
    }

    void RayTracer::RemoveLight(int lightInstanceId)
    {    
        UnityVulkanRecordingState recordingState;
        if (!graphicsInterface_->CommandRecordingState(&recordingState, kUnityVulkanGraphicsQueueAccess_DontCare))
        {
            PFG_EDITORLOGERROR("Cannot get recording state to remove light");
            return;
        }

        auto index = garbageBuffers_.size();
        garbageBuffers_.push_back(RayTracerGarbageBuffer());

        garbageBuffers_[index].frameCount = recordingState.currentFrameNumber;
        garbageBuffers_[index].buffer = std::move(lightsPool_[lightInstanceId]);

        lightsPool_.remove(lightInstanceId);

        PFG_EDITORLOG("Removed light (lightInstanceId: " + std::to_string(lightInstanceId) + ")");
    }

    RayTracerAPI::AddResourceResult RayTracer::AddTexture(int textureInstanceId, void* texture)
    {
        graphicsInterface_->EnsureOutsideRenderPass();

        if (texturePool_.find(textureInstanceId) != texturePool_.in_use_end())
        {
            return RayTracerAPI::AddResourceResult::AlreadyExists;
        }

        UnityVulkanRecordingState recordingState;
        if (!graphicsInterface_->CommandRecordingState(&recordingState, kUnityVulkanGraphicsQueueAccess_DontCare))
        {
            PFG_EDITORLOGERROR("Cannot get recording state to add texture");
            return RayTracerAPI::AddResourceResult::Error;
        }

        UnityVulkanImage image;
        if (!graphicsInterface_->AccessTexture(texture,
                                               UnityVulkanWholeImage,
                                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                               VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
                                               VK_ACCESS_SHADER_READ_BIT,
                                               kUnityVulkanResourceAccess_PipelineBarrier,
                                               &image))
        {
            PFG_EDITORLOGERROR("Cannot get accesss texture to add");
            return RayTracerAPI::AddResourceResult::Error;
        }

        texturePool_.add(textureInstanceId, std::make_unique<Vulkan::Image>(device_, physicalDeviceMemoryProperties_));

        VkImageSubresourceRange subresourceRange = {};
        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresourceRange.baseMipLevel = 0;
        subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        subresourceRange.baseArrayLayer = 0;
        subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

        texturePool_[textureInstanceId]->LoadFromUnity("texture", image.image, image.format);

        VK_CHECK("vkCreateImageView", texturePool_[textureInstanceId]->CreateImageView(VK_IMAGE_VIEW_TYPE_2D, image.format, subresourceRange));
        VK_CHECK("vkCreateSampler", texturePool_[textureInstanceId]->CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT));

        UpdateMaterialsTextureIndices();

        PFG_EDITORLOG("Added texture (textureInstanceId: " + std::to_string(textureInstanceId) + ")")

        return RayTracerAPI::AddResourceResult::Success;
    }

    void RayTracer::RemoveTexture(int textureInstanceId)
    {
        UnityVulkanRecordingState recordingState;
        if (!graphicsInterface_->CommandRecordingState(&recordingState, kUnityVulkanGraphicsQueueAccess_DontCare))
        {
            PFG_EDITORLOGERROR("Cannot get recording state to remove texture");
            return;
        }

        auto index = garbageBuffers_.size();
        garbageBuffers_.push_back(RayTracerGarbageBuffer());

        garbageBuffers_[index].frameCount = recordingState.currentFrameNumber;
        garbageBuffers_[index].buffer = std::move(texturePool_[textureInstanceId]);

        texturePool_.remove(textureInstanceId);

        UpdateMaterialsTextureIndices();

        PFG_EDITORLOG("Removed texture (textureInstanceId: " + std::to_string(textureInstanceId) + ")");
    }

    RayTracerAPI::AddResourceResult RayTracer::AddMaterial(int materialInstanceId,
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
                                                           int ambientOcclusionTextureInstanceId)
    {
        if (materialPool_.find(materialInstanceId) != materialPool_.in_use_end())
        {
            return RayTracerAPI::AddResourceResult::AlreadyExists;
        }

        auto buffer = std::make_unique<Vulkan::Buffer>();

        materialPool_.add(materialInstanceId, std::move(buffer));

        auto& material = materialPool_[materialInstanceId];

        material->Create(
            "material",
            device_,
            physicalDeviceMemoryProperties_,
            sizeof(ShaderMaterialData),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            Vulkan::Buffer::kDefaultMemoryPropertyFlags);

        UpdateMaterial(materialInstanceId, 
                       albedo_r, albedo_g, albedo_b, 
                       emission_r, emission_g, emission_b, 
                       transmittance_r, transmittance_g, transmittance_b,
                       metallic, 
                       roughness, 
                       indexOfRefraction,
                       albedoTextureSet,
                       albedoTextureInstanceId,
                       emissionTextureSet,
                       emissionTextureInstanceId,
                       normalTextureSet,
                       normalTextureInstanceId,
                       metallicTextureSet,
                       metallicTextureInstanceId,
                       roughnessTextureSet,
                       roughnessTextureInstanceId,
                       ambientOcclusionTextureSet,
                       ambientOcclusionTextureInstanceId);

        PFG_EDITORLOG("Added material (materialInstanceId: " + std::to_string(materialInstanceId) + ")");

        return RayTracerAPI::AddResourceResult::Success;
    }

    void RayTracer::UpdateMaterial(int materialInstanceId,
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
                                   int ambientOcclusionTextureInstanceId)
    {
        auto material = reinterpret_cast<ShaderMaterialData*>(materialPool_[materialInstanceId]->Map());

        material->albedo = vec4(albedo_r, albedo_g, albedo_b, 1.0f);
        material->emission = vec4(emission_r, emission_g, emission_b, 1.0f);
        material->transmittance = vec4(transmittance_r, transmittance_g, transmittance_b, 1.0f);
        material->metallic = metallic;
        material->roughness = roughness;
        material->indexOfRefraction = indexOfRefraction;
        material->albedoTextureSet = albedoTextureSet;
        material->albedoTextureInstanceId = albedoTextureInstanceId;
        material->albedoTextureIndex = 0;
        material->emissionTextureSet = emissionTextureSet;
        material->emissionTextureInstanceId = emissionTextureInstanceId;
        material->emissionTextureIndex = 0;
        material->normalTextureSet = normalTextureSet;
        material->normalTextureInstanceId = normalTextureInstanceId;
        material->normalTextureIndex = 0;
        material->metallicTextureSet = metallicTextureSet;
        material->metallicTextureInstanceId = metallicTextureInstanceId;
        material->metallicTextureIndex = 0;
        material->roughnessTextureSet = roughnessTextureSet;
        material->roughnessTextureInstanceId = roughnessTextureInstanceId;
        material->roughnessTextureIndex = 0;
        material->ambientOcclusionTextureSet = ambientOcclusionTextureSet;
        material->ambientOcclusionTextureInstanceId = ambientOcclusionTextureInstanceId;
        material->ambientOcclusionTextureIndex = 0;

        // Update texture indices 
        UpdateMaterialTextureIndices(material);

        materialPool_[materialInstanceId]->Unmap();
    }

    void RayTracer::RemoveMaterial(int materialInstanceId)
    {
        UnityVulkanRecordingState recordingState;
        if (!graphicsInterface_->CommandRecordingState(&recordingState, kUnityVulkanGraphicsQueueAccess_DontCare))
        {
            PFG_EDITORLOGERROR("Cannot get recording state to remove material");
            return;
        }

        auto index = garbageBuffers_.size();
        garbageBuffers_.push_back(RayTracerGarbageBuffer());

        garbageBuffers_[index].frameCount = recordingState.currentFrameNumber;
        garbageBuffers_[index].buffer = std::move(materialPool_[materialInstanceId]);

        materialPool_.remove(materialInstanceId);

        PFG_EDITORLOG("Removed material (materialInstanceId: " + std::to_string(materialInstanceId) + ")");
    }

    void RayTracer::TraceRays(int cameraInstanceId)
    {
        // cannot manage resources inside renderpass
        graphicsInterface_->EnsureOutsideRenderPass();

        UnityVulkanRecordingState recordingState;
        if (!graphicsInterface_->CommandRecordingState(&recordingState, kUnityVulkanGraphicsQueueAccess_DontCare))
        {
            return;
        }

        if (renderTargets_.find(cameraInstanceId) == renderTargets_.end())
        {
            GarbageCollect(recordingState.safeFrameNumber);
            // The camera isn't in the system yet, don't attempt to trace
            return;
        }

        if (renderTargets_[cameraInstanceId]->cameraData.GetBuffer() == VK_NULL_HANDLE)
        {
            GarbageCollect(recordingState.safeFrameNumber);
            // This camera hasn't been updated get for render
            return;
        }

        BuildTlas(recordingState.commandBuffer, recordingState.currentFrameNumber);

        if (tlas_.accelerationStructure == VK_NULL_HANDLE)
        {
            PFG_EDITORLOG("We don't have a tlas, so we cannot trace rays!");
            GarbageCollect(recordingState.safeFrameNumber);
            return;
        }

        if (pipelineLayout_ == VK_NULL_HANDLE)
        {
            CreatePipelineLayout();
            if (pipelineLayout_ == VK_NULL_HANDLE)
            {
                PFG_EDITORLOG("Something went wrong with creating the pipeline layout");
                // Something went wrong, don't continue
                GarbageCollect(recordingState.safeFrameNumber);
                return;
            }
        }

        if (pipeline_ == VK_NULL_HANDLE)
        {
            CreatePipeline();
            if (pipeline_ == VK_NULL_HANDLE)
            {
                PFG_EDITORLOG("Something went wrong with creating the pipeline");
                GarbageCollect(recordingState.safeFrameNumber);
                return;
            }
        }
        
        if (pipeline_ != VK_NULL_HANDLE && pipelineLayout_!= VK_NULL_HANDLE)
        {
            BuildDescriptorBufferInfos(cameraInstanceId, recordingState.currentFrameNumber);
            UpdateDescriptorSets(cameraInstanceId, recordingState.currentFrameNumber);

            BuildAndSubmitRayTracingCommandBuffer(cameraInstanceId, recordingState.commandBuffer, recordingState.currentFrameNumber);
            CopyRenderToRenderTarget(cameraInstanceId, recordingState.commandBuffer);
        }
        
        GarbageCollect(recordingState.safeFrameNumber);
    }

    RayTracerAPI::RayTracerStatistics RayTracer::GetRayTracerStatistics()
    {
        RayTracerAPI::RayTracerStatistics currentStats;

        currentStats.RegisteredLights = static_cast<uint32_t>(lightsPool_.in_use_size());
        currentStats.RegisteredSharedMeshes = static_cast<uint32_t>(sharedMeshesPool_.in_use_size());
        currentStats.RegisteredMeshInstances = static_cast<uint32_t>(meshInstancePool_.in_use_size());
        currentStats.RegisteredMaterials = static_cast<uint32_t>(materialPool_.in_use_size());
        currentStats.RegisteredTextures = static_cast<uint32_t>(texturePool_.in_use_size());
        currentStats.RegisteredRenderTargets = static_cast<uint32_t>(renderTargets_.size());
        
        for (auto renderTargetItr = renderTargets_.begin(); renderTargetItr != renderTargets_.end(); ++renderTargetItr)
        {
            auto& renderTarget = (*renderTargetItr).second;
            for (auto itr = renderTarget->descriptorSetsData.begin(); itr != renderTarget->descriptorSetsData.end(); ++itr)
            {
                auto& descriptorSetsData = (*itr).second;
                
                currentStats.DescriptorSetCount += 1;
                currentStats.AccelerationStuctureCount += descriptorSetsData.acceleration_stucture_count;
                currentStats.UniformBufferCount += descriptorSetsData.uniform_buffer_count;
                currentStats.StorageImageCount += descriptorSetsData.storage_image_count;
                currentStats.StorageBufferCount += descriptorSetsData.storage_buffer_count;
                currentStats.CombinedImageSamplerCount += descriptorSetsData.combined_image_sampler_count;
            }
        }     

        return currentStats;
    }

#pragma endregion RayTracerAPI

    void RayTracer::BuildBlas(int sharedMeshInstanceId)
    {
        // Create buffers for the bottom level geometry
    
        // Setup a single transformation matrix that can be used to transform the whole geometry for a single bottom level acceleration structure
        VkTransformMatrixKHR transformMatrix = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f };
        
        auto transformBuffer = std::make_unique<Vulkan::Buffer>();
        transformBuffer->Create(
            "transformBuffer",
            device_, 
            physicalDeviceMemoryProperties_, 
            sizeof(VkTransformMatrixKHR),
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            Vulkan::Buffer::kDefaultMemoryPropertyFlags);
        transformBuffer->UploadData(&transformMatrix, sizeof(VkTransformMatrixKHR));

        //auto identity = reinterpret_cast<VkTransformMatrixKHR*>(transformBuffer.Map());
        //PFG_EDITORLOG("Identity matrix: ");
        //PFG_EDITORLOG(std::to_string(identity->matrix[0][0]) + ", " + std::to_string(identity->matrix[0][1]) + ", " + std::to_string(identity->matrix[0][2]) + ", " + std::to_string(identity->matrix[0][3]));
        //PFG_EDITORLOG(std::to_string(identity->matrix[1][0]) + ", " + std::to_string(identity->matrix[1][1]) + ", " + std::to_string(identity->matrix[1][2]) + ", " + std::to_string(identity->matrix[1][3]));
        //PFG_EDITORLOG(std::to_string(identity->matrix[2][0]) + ", " + std::to_string(identity->matrix[2][1]) + ", " + std::to_string(identity->matrix[2][2]) + ", " + std::to_string(identity->matrix[2][3]));
        //transformBuffer.Unmap();

        // The bottom level acceleration structure contains one set of triangles as the input geometry
        VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {};
        accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;

        accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        accelerationStructureGeometry.geometry.triangles.pNext = nullptr;
        accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        accelerationStructureGeometry.geometry.triangles.vertexData = sharedMeshesPool_[sharedMeshInstanceId]->vertexBuffer.GetBufferDeviceAddressConst();
        accelerationStructureGeometry.geometry.triangles.maxVertex = sharedMeshesPool_[sharedMeshInstanceId]->vertexCount;
        accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(vec3);
        accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        accelerationStructureGeometry.geometry.triangles.indexData = sharedMeshesPool_[sharedMeshInstanceId]->indexBuffer.GetBufferDeviceAddressConst();
        accelerationStructureGeometry.geometry.triangles.transformData = transformBuffer->GetBufferDeviceAddressConst();
        
        // Get the size requirements for buffers involved in the acceleration structure build process
        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {};
        accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount = 1;
        accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

        // Number of triangles 
        const uint32_t primitiveCount = sharedMeshesPool_[sharedMeshInstanceId]->indexCount / 3;
        /*PFG_EDITORLOG("BuildBlas() vertexCount: " + std::to_string(sharedMeshesPool_[sharedMeshPoolIndex]->vertexCount));
        PFG_EDITORLOG("BuildBlas() indexCount: " + std::to_string(sharedMeshesPool_[sharedMeshPoolIndex]->indexCount));*/

        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {};
        accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(
            device_,
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &accelerationStructureBuildGeometryInfo,
            &primitiveCount,
            &accelerationStructureBuildSizesInfo);

        // Create a buffer to hold the acceleration structure
        sharedMeshesPool_[sharedMeshInstanceId]->blas.buffer.Create(
            "blas",
            device_,
            physicalDeviceMemoryProperties_,
            accelerationStructureBuildSizesInfo.accelerationStructureSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
            Vulkan::Buffer::kDefaultMemoryPropertyFlags);
    
        // Create the acceleration structure
        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {};
        accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStructureCreateInfo.buffer = sharedMeshesPool_[sharedMeshInstanceId]->blas.buffer.GetBuffer();
        accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
        accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        VK_CHECK("vkCreateAccelerationStructureKHR", vkCreateAccelerationStructureKHR(device_, &accelerationStructureCreateInfo, nullptr, &sharedMeshesPool_[sharedMeshInstanceId]->blas.accelerationStructure));

        // The actual build process starts here
        // Create a scratch buffer as a temporary storage for the acceleration structure build
        auto scratchBuffer = std::make_unique<Vulkan::Buffer>();
        scratchBuffer->Create(
            "scratch",
            device_, 
            physicalDeviceMemoryProperties_, 
            accelerationStructureBuildSizesInfo.buildScratchSize, 
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo = { };
        accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        accelerationBuildGeometryInfo.dstAccelerationStructure = sharedMeshesPool_[sharedMeshInstanceId]->blas.accelerationStructure;
        accelerationBuildGeometryInfo.geometryCount = 1;
        accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
        accelerationBuildGeometryInfo.scratchData = scratchBuffer->GetBufferDeviceAddress();

        VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo = { };
        accelerationStructureBuildRangeInfo.primitiveCount = primitiveCount;
        accelerationStructureBuildRangeInfo.primitiveOffset = 0;
        accelerationStructureBuildRangeInfo.firstVertex = 0;
        accelerationStructureBuildRangeInfo.transformOffset = 0;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationStructureBuildRangeInfos = { &accelerationStructureBuildRangeInfo };

        // Get the bottom acceleration structure's handle, which will be used during the top level acceleration build
        VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo{};
        accelerationStructureDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        accelerationStructureDeviceAddressInfo.accelerationStructure = sharedMeshesPool_[sharedMeshInstanceId]->blas.accelerationStructure;
        sharedMeshesPool_[sharedMeshInstanceId]->blas.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device_, &accelerationStructureDeviceAddressInfo);

        // Build the acceleration structure on the device
        PFG_EDITORLOG("Building blas for mesh (sharedMeshInstanceId: " + std::to_string(sharedMeshInstanceId) + ")");
        //RayTracerAccelerationStructureBuildInfo buildInfo =
        //{
        //    accelerationBuildGeometryInfo,
        //    accelerationStructureBuildRangeInfos
        //};
        //graphicsInterface_->AccessQueue(BuildAccelerationStructureQueueCallback, 0, &buildInfo, true);

        graphicsInterface_->EnsureOutsideRenderPass();

        UnityVulkanRecordingState recordingState;
        if (!graphicsInterface_->CommandRecordingState(&recordingState, kUnityVulkanGraphicsQueueAccess_DontCare))
        {
            PFG_EDITORLOGERROR("Cannot obtain recording state to build tlas");
            return;
        }

        vkCmdBuildAccelerationStructuresKHR(
            recordingState.commandBuffer,
            1,
            &accelerationBuildGeometryInfo,
            accelerationStructureBuildRangeInfos.data());

        auto index = garbageBuffers_.size();
        garbageBuffers_.push_back(RayTracerGarbageBuffer());
        garbageBuffers_[index].frameCount = recordingState.currentFrameNumber;
        garbageBuffers_[index].buffer = std::move(scratchBuffer);

        index = garbageBuffers_.size();
        garbageBuffers_.push_back(RayTracerGarbageBuffer());
        garbageBuffers_[index].frameCount = recordingState.currentFrameNumber;
        garbageBuffers_[index].buffer = std::move(transformBuffer);

        PFG_EDITORLOG("Built blas for mesh (sharedMeshInstanceId: " + std::to_string(sharedMeshInstanceId) + ")");
    }

    void RayTracer::BuildTlas(VkCommandBuffer commandBuffer, uint64_t currentFrameNumber)
    {
        // If there is nothing to do, skip building the tlas
        if (rebuildTlas_ == false && updateTlas_ == false)
        {
            return;
        }

        bool update = updateTlas_;
        if (rebuildTlas_)
        {
            update = false;
        }

        if (meshInstancePool_.in_use_size() == 0)
        {
            // We have no instances, so there is nothing to build 
            return;
        }

        // TODO: this is copied from BuildDescriptorBufferInfos.... kinda not great way to do this?
        uint32_t i = 1;
        std::map<int, uint32_t> materialInstanceIdToShaderIndex;
        for (auto itr = materialPool_.in_use_begin(); itr != materialPool_.in_use_end(); ++itr)
        {
            auto materialInstanceId = (*itr).first;
            materialInstanceIdToShaderIndex.insert(std::make_pair(materialInstanceId, i));
            ++i;
        }

        if (!update)
        {
            // Build instance buffer from scratch
            std::vector<VkAccelerationStructureInstanceKHR> instanceAccelerationStructures;
            instanceAccelerationStructures.resize(meshInstancePool_.in_use_size(), VkAccelerationStructureInstanceKHR{});

            // Gather instances
            uint32_t instanceAccelerationStructuresIndex = 0;
            for (auto i = meshInstancePool_.in_use_begin(); i != meshInstancePool_.in_use_end(); ++i)
            {
                auto gameObjectInstanceId = (*i).first;

                auto& instance = meshInstancePool_[gameObjectInstanceId];

                const auto& t = instance.localToWorld;
                VkTransformMatrixKHR transformMatrix = {
                    t[0][0], t[0][1], t[0][2], t[0][3],
                    t[1][0], t[1][1], t[1][2], t[1][3],
                    t[2][0], t[2][1], t[2][2], t[2][3]
                };

                /*PFG_EDITORLOG("Instance transform matrix loop: ");
                PFG_EDITORLOG(std::to_string(transformMatrix.matrix[0][0]) + ", " + std::to_string(transformMatrix.matrix[0][1]) + ", " + std::to_string(transformMatrix.matrix[0][2]) + ", " + std::to_string(transformMatrix.matrix[0][3]));
                PFG_EDITORLOG(std::to_string(transformMatrix.matrix[1][0]) + ", " + std::to_string(transformMatrix.matrix[1][1]) + ", " + std::to_string(transformMatrix.matrix[1][2]) + ", " + std::to_string(transformMatrix.matrix[1][3]));
                PFG_EDITORLOG(std::to_string(transformMatrix.matrix[2][0]) + ", " + std::to_string(transformMatrix.matrix[2][1]) + ", " + std::to_string(transformMatrix.matrix[2][2]) + ", " + std::to_string(transformMatrix.matrix[2][3]));*/

                VkAccelerationStructureInstanceKHR& accelerationStructureInstance = instanceAccelerationStructures[instanceAccelerationStructuresIndex];
                accelerationStructureInstance.transform = transformMatrix;
                accelerationStructureInstance.instanceCustomIndex = instanceAccelerationStructuresIndex;
                accelerationStructureInstance.mask = 0xFF;
                accelerationStructureInstance.instanceShaderBindingTableRecordOffset = 0;
                accelerationStructureInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
                accelerationStructureInstance.accelerationStructureReference = sharedMeshesPool_[instance.sharedMeshInstanceId]->blas.deviceAddress;

                /*PFG_EDITORLOG("VkAccelerationStructureInstanceKHR.transform: ");
                PFG_EDITORLOG(std::to_string(accelerationStructureInstance.transform.matrix[0][0]) + ", " + std::to_string(accelerationStructureInstance.transform.matrix[0][1]) + ", " + std::to_string(accelerationStructureInstance.transform.matrix[0][2]) + ", " + std::to_string(accelerationStructureInstance.transform.matrix[0][3]));
                PFG_EDITORLOG(std::to_string(accelerationStructureInstance.transform.matrix[1][0]) + ", " + std::to_string(accelerationStructureInstance.transform.matrix[1][1]) + ", " + std::to_string(accelerationStructureInstance.transform.matrix[1][2]) + ", " + std::to_string(accelerationStructureInstance.transform.matrix[1][3]));
                PFG_EDITORLOG(std::to_string(accelerationStructureInstance.transform.matrix[2][0]) + ", " + std::to_string(accelerationStructureInstance.transform.matrix[2][1]) + ", " + std::to_string(accelerationStructureInstance.transform.matrix[2][2]) + ", " + std::to_string(accelerationStructureInstance.transform.matrix[2][3]));*/

                // Map materials here, but seems like not the greatest place to do so?
                auto instanceData = reinterpret_cast<ShaderInstanceData*>(instance.instanceData.Map());
                if (instance.materialInstanceId == -1)
                {
                    instanceData->materialIndex = 0;
                }
                else
                {
                    instanceData->materialIndex = materialInstanceIdToShaderIndex[instance.materialInstanceId];
                }

                instanceData->localToWorld = instance.localToWorld;
                instanceData->worldToLocal = instance.worldToLocal;

                instance.instanceData.Unmap();

                // Consumed current index, advance
                ++instanceAccelerationStructuresIndex;
            }

            // Destroy anytihng if created before
            instancesAccelerationStructuresBuffer_.Destroy();

            instancesAccelerationStructuresBuffer_.Create(
                "instanceBuffer",
                device_,
                physicalDeviceMemoryProperties_,
                instanceAccelerationStructures.size() * sizeof(VkAccelerationStructureInstanceKHR),
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                Vulkan::Buffer::kDefaultMemoryPropertyFlags);
            instancesAccelerationStructuresBuffer_.UploadData(instanceAccelerationStructures.data(), instancesAccelerationStructuresBuffer_.GetSize());

            /*auto instances = reinterpret_cast<VkAccelerationStructureInstanceKHR*>(instancesAccelerationStructuresBuffer_.Map());
            for (int i = 0; i < meshInstancePool_.in_use_size(); ++i)
            {

                PFG_EDITORLOG("Instance transform matrix after upload: ");
                PFG_EDITORLOG(std::to_string(instances[i].transform.matrix[0][0]) + ", " + std::to_string(instances[i].transform.matrix[0][1]) + ", " + std::to_string(instances[i].transform.matrix[0][2]) + ", " + std::to_string(instances[i].transform.matrix[0][3]));
                PFG_EDITORLOG(std::to_string(instances[i].transform.matrix[1][0]) + ", " + std::to_string(instances[i].transform.matrix[1][1]) + ", " + std::to_string(instances[i].transform.matrix[1][2]) + ", " + std::to_string(instances[i].transform.matrix[1][3]));
                PFG_EDITORLOG(std::to_string(instances[i].transform.matrix[2][0]) + ", " + std::to_string(instances[i].transform.matrix[2][1]) + ", " + std::to_string(instances[i].transform.matrix[2][2]) + ", " + std::to_string(instances[i].transform.matrix[2][3]));

            }
            instancesAccelerationStructuresBuffer_.Unmap();*/
        }
        else
        {
            // Update transforms in buffer, should be the exact same layout

            auto instances = reinterpret_cast<VkAccelerationStructureInstanceKHR*>(instancesAccelerationStructuresBuffer_.Map());

            // Gather instances
            uint32_t instanceAccelerationStructuresIndex = 0;
            for (auto i = meshInstancePool_.in_use_begin(); i != meshInstancePool_.in_use_end(); ++i)
            {
                auto gameObjectInstanceId = (*i).first;

                auto& instance = meshInstancePool_[gameObjectInstanceId];

                const auto& t = instance.localToWorld;
                VkTransformMatrixKHR transformMatrix = {
                    t[0][0], t[0][1], t[0][2], t[0][3],
                    t[1][0], t[1][1], t[1][2], t[1][3],
                    t[2][0], t[2][1], t[2][2], t[2][3]
                };

                //PFG_EDITORLOG("Instance transform matrix: ");
                //PFG_EDITORLOG(std::to_string(t[0][0]) + ", " + std::to_string(t[0][1]) + ", " + std::to_string(t[0][2]) + ", " + std::to_string(t[0][3]));
                //PFG_EDITORLOG(std::to_string(t[1][0]) + ", " + std::to_string(t[1][1]) + ", " + std::to_string(t[1][2]) + ", " + std::to_string(t[1][3]));
                //PFG_EDITORLOG(std::to_string(t[2][0]) + ", " + std::to_string(t[2][1]) + ", " + std::to_string(t[2][2]) + ", " + std::to_string(t[2][3]));

                instances[instanceAccelerationStructuresIndex].transform = transformMatrix;

                // Map materials here, but seems like not the greatest place to do so?
                auto instanceData = reinterpret_cast<ShaderInstanceData*>(instance.instanceData.Map());
                if (instance.materialInstanceId == -1)
                {
                    instanceData->materialIndex = 0;
                }
                else
                {
                    instanceData->materialIndex = materialInstanceIdToShaderIndex[instance.materialInstanceId];
                }

                instanceData->localToWorld = instance.localToWorld;
                instanceData->worldToLocal = instance.worldToLocal;

                instance.instanceData.Unmap();


                // Consumed current index, advance
                ++instanceAccelerationStructuresIndex;
            }
            instancesAccelerationStructuresBuffer_.Unmap();
        }

        //auto instances = reinterpret_cast<VkAccelerationStructureInstanceKHR*>(instancesAccelerationStructuresBuffer_.Map());
        //for (int i = 0; i < meshInstancePool_.in_use_size(); ++i)
        //{
        //    
        //    PFG_EDITORLOG("Instance transform matrix: ");
        //        PFG_EDITORLOG(std::to_string(instances[i].transform.matrix[0][0]) + ", " + std::to_string(instances[i].transform.matrix[0][1]) + ", " + std::to_string(instances[i].transform.matrix[0][2]) + ", " + std::to_string(instances[i].transform.matrix[0][3]));
        //        PFG_EDITORLOG(std::to_string(instances[i].transform.matrix[1][0]) + ", " + std::to_string(instances[i].transform.matrix[1][1]) + ", " + std::to_string(instances[i].transform.matrix[1][2]) + ", " + std::to_string(instances[i].transform.matrix[1][3]));
        //        PFG_EDITORLOG(std::to_string(instances[i].transform.matrix[2][0]) + ", " + std::to_string(instances[i].transform.matrix[2][1]) + ", " + std::to_string(instances[i].transform.matrix[2][2]) + ", " + std::to_string(instances[i].transform.matrix[2][3]));
        //        
        //}
        //instancesAccelerationStructuresBuffer_.Unmap();

        // The top level acceleration structure contains (bottom level) instance as the input geometry
        VkAccelerationStructureGeometryInstancesDataKHR accelerationStructureGeometryInstancesData = {};
        accelerationStructureGeometryInstancesData.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        accelerationStructureGeometryInstancesData.arrayOfPointers = VK_FALSE;
        accelerationStructureGeometryInstancesData.data.deviceAddress = instancesAccelerationStructuresBuffer_.GetBufferDeviceAddressConst().deviceAddress;

        VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {};
        accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        accelerationStructureGeometry.geometry.instances = accelerationStructureGeometryInstancesData;

        // Get the size requirements for buffers involved in the acceleration structure build process
        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {};
        accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount = 1;
        accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

        // Number of instances
        uint32_t instancesCount = static_cast<uint32_t>(meshInstancePool_.in_use_size());

        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {};
        accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(
            device_,
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &accelerationStructureBuildGeometryInfo,
            &instancesCount,
            &accelerationStructureBuildSizesInfo);

        if (!update)
        {
            if (tlas_.accelerationStructure != VK_NULL_HANDLE)
            {
                vkDestroyAccelerationStructureKHR(device_, tlas_.accelerationStructure, nullptr);
            }
            tlas_.buffer.Destroy();

            // Create a buffer to hold the acceleration structure
            tlas_.buffer.Create(
                "tlas",
                device_,
                physicalDeviceMemoryProperties_,
                accelerationStructureBuildSizesInfo.accelerationStructureSize,
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
                Vulkan::Buffer::kDefaultMemoryPropertyFlags);

            // Create the acceleration structure
            VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {};
            accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
            accelerationStructureCreateInfo.buffer = tlas_.buffer.GetBuffer();
            accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
            accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
            VK_CHECK("vkCreateAccelerationStructureKHR", vkCreateAccelerationStructureKHR(device_, &accelerationStructureCreateInfo, nullptr, &tlas_.accelerationStructure));

            assert(tlas_.accelerationStructure != VK_NULL_HANDLE);
        }

        // The actual build process starts here

        // Create a scratch buffer as a temporary storage for the acceleration structure build
        auto scratchBuffer = std::make_unique<Vulkan::Buffer>();
        scratchBuffer->Create(
            "scratch",
            device_,
            physicalDeviceMemoryProperties_,
            accelerationStructureBuildSizesInfo.buildScratchSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo = {};
        accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR;
        accelerationBuildGeometryInfo.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        accelerationBuildGeometryInfo.srcAccelerationStructure = update ? tlas_.accelerationStructure : VK_NULL_HANDLE;
        accelerationBuildGeometryInfo.dstAccelerationStructure = tlas_.accelerationStructure;
        accelerationBuildGeometryInfo.geometryCount = 1;
        accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
        accelerationBuildGeometryInfo.scratchData = scratchBuffer->GetBufferDeviceAddress();


        VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo;
        accelerationStructureBuildRangeInfo.primitiveCount = static_cast<uint32_t>(meshInstancePool_.in_use_size());
        accelerationStructureBuildRangeInfo.primitiveOffset = 0;
        accelerationStructureBuildRangeInfo.firstVertex = 0;
        accelerationStructureBuildRangeInfo.transformOffset = 0;

        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationStructureBuildRangeInfos = { &accelerationStructureBuildRangeInfo };

        // Get the top acceleration structure's handle, which will be used to setup it's descriptor
        VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo = {};
        accelerationStructureDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        accelerationStructureDeviceAddressInfo.accelerationStructure = tlas_.accelerationStructure;
        tlas_.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device_, &accelerationStructureDeviceAddressInfo);

        vkCmdBuildAccelerationStructuresKHR(
            commandBuffer,
            1,
            &accelerationBuildGeometryInfo,
            accelerationStructureBuildRangeInfos.data());

        auto index = garbageBuffers_.size();
        garbageBuffers_.push_back(RayTracerGarbageBuffer());
        garbageBuffers_[index].frameCount = currentFrameNumber;
        garbageBuffers_[index].buffer = std::move(scratchBuffer);

        //// Build the acceleration structure on the device
        //RayTracerAccelerationStructureBuildInfo buildInfo =
        //{
        //    accelerationBuildGeometryInfo,
        //    accelerationStructureBuildRangeInfos
        //};
        //graphicsInterface_->AccessQueue(BuildAccelerationStructureQueueCallback, 0, &buildInfo, true);

        //scratchBuffer.Destroy();

        if (!update)
        {
            PFG_EDITORLOG("Succesfully built tlas");
        }
        //else
        //{
        //    PFG_EDITORLOG("Succesfully updated tlas");
        //}


        // We did any pending work, reset flags
        rebuildTlas_ = false;
        updateTlas_ = false;
    }

    void UNITY_INTERFACE_API RayTracer::BuildAccelerationStructureQueueCallback(int eventId, void* data)
    {
        PixelsForGlory::Vulkan::RayTracer::Instance().BuildAccelerationStructureInQueue(data);

    }

    void RayTracer::BuildAccelerationStructureInQueue(void* data)
    {
        // Build the acceleration structure on the device 
        RayTracerAccelerationStructureBuildInfo* buildInfo = reinterpret_cast<RayTracerAccelerationStructureBuildInfo*>(data);

        graphicsInterface_->EnsureOutsideRenderPass();

        UnityVulkanRecordingState recordingState;
        if (!graphicsInterface_->CommandRecordingState(&recordingState, kUnityVulkanGraphicsQueueAccess_DontCare))
        {
            PFG_EDITORLOGERROR("Cannot obtain recording state to build blas");
            return;
        }

        PFG_EDITORLOG("vkCmdBuildAccelerationStructuresKHR");

        vkCmdBuildAccelerationStructuresKHR(
            recordingState.commandBuffer,
            1,
            &buildInfo->accelerationBuildGeometryInfo,
            buildInfo->accelerationStructureBuildRangeInfos.data());
    }

    void RayTracer::CreateDescriptorSetsLayouts()
    {
        // Create descriptor sets for the shader.  This setups up how data is bound to GPU memory and what shader stages will have access to what memory
    
        descriptorSetLayouts_.resize(DESCRIPTOR_SET_SIZE);

        {
            //  binding 0  ->  Acceleration structure
            //  binding 1  ->  Scene data
            //  binding 2  ->  Camera data
            VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding;
            accelerationStructureLayoutBinding.binding = DESCRIPTOR_BINDING_ACCELERATION_STRUCTURE;
            accelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            accelerationStructureLayoutBinding.descriptorCount = 1;
            accelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

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
                    sceneDataLayoutBinding,
                    cameraDataLayoutBinding
                });

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
            descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            descriptorSetLayoutCreateInfo.pBindings = bindings.data();

            VK_CHECK("vkCreateDescriptorSetLayout", 
                vkCreateDescriptorSetLayout(device_, 
                                            &descriptorSetLayoutCreateInfo, 
                                            nullptr, 
                                            // Overkill, but represents the sets its creating                        
                                            &descriptorSetLayouts_[DESCRIPTOR_SET_ACCELERATION_STRUCTURE & DESCRIPTOR_SET_SCENE_DATA & DESCRIPTOR_SET_CAMERA_DATA]));
        }

        {
            // binding 0 -> render target 
            VkDescriptorSetLayoutBinding imageLayoutBinding;
            imageLayoutBinding.binding = DESCRIPTOR_BINDING_RENDER_TARGET;
            imageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            imageLayoutBinding.descriptorCount = 1;
            imageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
            descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo.bindingCount = 1;
            descriptorSetLayoutCreateInfo.pBindings = &imageLayoutBinding;
            
            VK_CHECK("vkCreateDescriptorSetLayout", vkCreateDescriptorSetLayout(device_, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayouts_[DESCRIPTOR_SET_RENDER_TARGET]));
        }

        {
            // binding 0 -> faces
            const VkDescriptorBindingFlags setFlag = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

            VkDescriptorSetLayoutBindingFlagsCreateInfo setBindingFlags;
            setBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            setBindingFlags.pNext = nullptr;
            setBindingFlags.pBindingFlags = &setFlag;
            setBindingFlags.bindingCount = 1;

            VkDescriptorSetLayoutBinding facesLayoutBinding;
            facesLayoutBinding.binding = DESCRIPTOR_BINDING_FACE_DATA;
            facesLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            facesLayoutBinding.descriptorCount = 200000; // Upper bound on the size of the binding
            facesLayoutBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
            descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo.bindingCount = 1;
            descriptorSetLayoutCreateInfo.pBindings = &facesLayoutBinding;
            descriptorSetLayoutCreateInfo.pNext = &setBindingFlags;

            VK_CHECK("vkCreateDescriptorSetLayout", vkCreateDescriptorSetLayout(device_, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayouts_[DESCRIPTOR_SET_FACE_DATA]));
        }

        {
            // binding 0 -> attributes
            const VkDescriptorBindingFlags setFlag = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

            VkDescriptorSetLayoutBindingFlagsCreateInfo setBindingFlags;
            setBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            setBindingFlags.pNext = nullptr;
            setBindingFlags.pBindingFlags = &setFlag;
            setBindingFlags.bindingCount = 1;

            VkDescriptorSetLayoutBinding attributesLayoutBinding;
            attributesLayoutBinding.binding = DESCRIPTOR_BINDING_VERTEX_ATTRIBUTES;
            attributesLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            attributesLayoutBinding.descriptorCount = 200000;   // Upper bound on the size of the binding
            attributesLayoutBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
            descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo.bindingCount = 1;
            descriptorSetLayoutCreateInfo.pBindings = &attributesLayoutBinding;
            descriptorSetLayoutCreateInfo.pNext = &setBindingFlags;

            VK_CHECK("vkCreateDescriptorSetLayout", vkCreateDescriptorSetLayout(device_, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayouts_[DESCRIPTOR_SET_VERTEX_ATTRIBUTES]));
        }

        {
            // binding 0 -> lights
            const VkDescriptorBindingFlags setFlag = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

            VkDescriptorSetLayoutBindingFlagsCreateInfo setBindingFlags;
            setBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            setBindingFlags.pNext = nullptr;
            setBindingFlags.pBindingFlags = &setFlag;
            setBindingFlags.bindingCount = 1;

            VkDescriptorSetLayoutBinding lightsLayoutBinding;
            lightsLayoutBinding.binding = DESCRIPTOR_BINDING_LIGHTS_DATA;
            lightsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            lightsLayoutBinding.descriptorCount = 50;   // Upper bound on the size of the binding
            lightsLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
            descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo.bindingCount = 1;
            descriptorSetLayoutCreateInfo.pBindings = &lightsLayoutBinding;
            descriptorSetLayoutCreateInfo.pNext = &setBindingFlags;

            VK_CHECK("vkCreateDescriptorSetLayout", vkCreateDescriptorSetLayout(device_, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayouts_[DESCRIPTOR_SET_LIGHTS_DATA]));
        }

        {
            // binding 0 -> materials
            const VkDescriptorBindingFlags setFlag = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

            VkDescriptorSetLayoutBindingFlagsCreateInfo setBindingFlags;
            setBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            setBindingFlags.pNext = nullptr;
            setBindingFlags.pBindingFlags = &setFlag;
            setBindingFlags.bindingCount = 1;

            VkDescriptorSetLayoutBinding materialsLayoutBinding;
            materialsLayoutBinding.binding = DESCRIPTOR_BINDING_MATERIALS_DATA;
            materialsLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            materialsLayoutBinding.descriptorCount = 100000;   // Upper bound on the size of the binding
            materialsLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
            descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo.bindingCount = 1;
            descriptorSetLayoutCreateInfo.pBindings = &materialsLayoutBinding;
            descriptorSetLayoutCreateInfo.pNext = &setBindingFlags;

            VK_CHECK("vkCreateDescriptorSetLayout", vkCreateDescriptorSetLayout(device_, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayouts_[DESCRIPTOR_SET_MATERIALS_DATA]));
        }

        {
            // binding 0 -> textures
            const VkDescriptorBindingFlags setFlag = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

            VkDescriptorSetLayoutBindingFlagsCreateInfo setBindingFlags;
            setBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            setBindingFlags.pNext = nullptr;
            setBindingFlags.pBindingFlags = &setFlag;
            setBindingFlags.bindingCount = 1;

            VkDescriptorSetLayoutBinding texturesLayoutBinding;
            texturesLayoutBinding.binding = DESCRIPTOR_BINDING_TEXTURES_DATA;
            texturesLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            texturesLayoutBinding.descriptorCount = 100;   // Upper bound on the size of the binding
            texturesLayoutBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
            texturesLayoutBinding.pImmutableSamplers = nullptr; // If descriptorCount != 0 and pImmutableSamplers is not NULL, pImmutableSamplers must be a valid pointer.  We don't want this so null

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
            descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo.bindingCount = 1;
            descriptorSetLayoutCreateInfo.pBindings = &texturesLayoutBinding;
            descriptorSetLayoutCreateInfo.pNext = &setBindingFlags;

            VK_CHECK("vkCreateDescriptorSetLayout", vkCreateDescriptorSetLayout(device_, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayouts_[DESCRIPTOR_SET_TEXTURES_DATA]));
        }

        {
            // binding 0 -> instance data
            const VkDescriptorBindingFlags setFlag = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

            VkDescriptorSetLayoutBindingFlagsCreateInfo setBindingFlags;
            setBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            setBindingFlags.pNext = nullptr;
            setBindingFlags.pBindingFlags = &setFlag;
            setBindingFlags.bindingCount = 1;

            VkDescriptorSetLayoutBinding instanceDataLayoutBinding;
            instanceDataLayoutBinding.binding = DESCRIPTOR_BINDING_INSTANCE_DATA;
            instanceDataLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            instanceDataLayoutBinding.descriptorCount = 100000;   // Upper bound on the size of the binding
            instanceDataLayoutBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

            VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
            descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptorSetLayoutCreateInfo.bindingCount = 1;
            descriptorSetLayoutCreateInfo.pBindings = &instanceDataLayoutBinding;
            descriptorSetLayoutCreateInfo.pNext = &setBindingFlags;

            VK_CHECK("vkCreateDescriptorSetLayout", vkCreateDescriptorSetLayout(device_, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayouts_[DESCRIPTOR_SET_INSTANCE_DATA]));
        }


    }

    void RayTracer::CreatePipelineLayout()
    {
        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.setLayoutCount = DESCRIPTOR_SET_SIZE;
        pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts_.data();

        VK_CHECK("vkCreatePipelineLayout", vkCreatePipelineLayout(device_, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout_));
    }

    void RayTracer::CreatePipeline()
    {
        Vulkan::Shader rayGenShader(device_);
        Vulkan::Shader rayChitShader(device_);
        Vulkan::Shader rayMissShader(device_);
        Vulkan::Shader shadowChit(device_);
        Vulkan::Shader shadowMiss(device_);

        rayGenShader.LoadFromFile((shaderFolder_ + L"ray_gen.bin").c_str());
        rayChitShader.LoadFromFile((shaderFolder_ + L"ray_chit.bin").c_str());
        rayMissShader.LoadFromFile((shaderFolder_ + L"ray_miss.bin").c_str());
        shadowChit.LoadFromFile((shaderFolder_ + L"shadow_ray_chit.bin").c_str());
        shadowMiss.LoadFromFile((shaderFolder_ + L"shadow_ray_miss.bin").c_str());

        // Destroy any existing shader table before creating a new one
        shaderBindingTable_.Destroy();

        shaderBindingTable_.Initialize(2, 2, rayTracingProperties_.shaderGroupHandleSize, rayTracingProperties_.shaderGroupBaseAlignment);

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

        VK_CHECK("vkCreateRayTracingPipelinesKHR", vkCreateRayTracingPipelinesKHR(device_, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayPipelineInfo, nullptr, &pipeline_));

        shaderBindingTable_.CreateSBT(device_, physicalDeviceMemoryProperties_, pipeline_);
    }

    void RayTracer::BuildAndSubmitRayTracingCommandBuffer(int cameraInstanceId, VkCommandBuffer commandBuffer, uint64_t currentFrameNumber)
    {
        // NOTE: assumes that renderTargets_ has already been checked

        auto& renderTarget = renderTargets_[cameraInstanceId];
        auto& descriptorSetsData = renderTargets_[cameraInstanceId]->descriptorSetsData[currentFrameNumber];

        vkCmdBindPipeline(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
            pipeline_);

        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
            pipelineLayout_, 0,
            static_cast<uint32_t>(descriptorSetsData.descriptorSets.size()), descriptorSetsData.descriptorSets.data(),
            0, 0);

        VkStridedDeviceAddressRegionKHR raygenShaderEntry = {};
        raygenShaderEntry.deviceAddress = shaderBindingTable_.GetBuffer().GetBufferDeviceAddress().deviceAddress + shaderBindingTable_.GetRaygenOffset();
        raygenShaderEntry.stride = shaderBindingTable_.GetGroupsStride();
        raygenShaderEntry.size = shaderBindingTable_.GetRaygenSize();

        VkStridedDeviceAddressRegionKHR missShaderEntry{};
        missShaderEntry.deviceAddress = shaderBindingTable_.GetBuffer().GetBufferDeviceAddress().deviceAddress + shaderBindingTable_.GetMissGroupsOffset();
        missShaderEntry.stride = shaderBindingTable_.GetGroupsStride();
        missShaderEntry.size = shaderBindingTable_.GetMissGroupsSize();

        VkStridedDeviceAddressRegionKHR hitShaderEntry{};
        hitShaderEntry.deviceAddress = shaderBindingTable_.GetBuffer().GetBufferDeviceAddress().deviceAddress + shaderBindingTable_.GetHitGroupsOffset();
        hitShaderEntry.stride = shaderBindingTable_.GetGroupsStride();
        hitShaderEntry.size = shaderBindingTable_.GetHitGroupsSize();

        VkStridedDeviceAddressRegionKHR callableShaderEntry{};

        // Dispatch the ray tracing commands
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline_);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout_, 0, static_cast<uint32_t>(descriptorSetsData.descriptorSets.size()), descriptorSetsData.descriptorSets.data(), 0, 0);

        // Make into a storage image
        VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        Vulkan::Image::UpdateImageBarrier(
            commandBuffer,
            renderTargets_[cameraInstanceId]->stagingImage.GetImage(),
            range,
            0, VK_ACCESS_SHADER_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        //PFG_EDITORLOG("Tracing for " + std::to_string(cameraInstanceId));

        vkCmdTraceRaysKHR(
            commandBuffer,
            &raygenShaderEntry,
            &missShaderEntry,
            &hitShaderEntry,
            &callableShaderEntry,
            renderTargets_[cameraInstanceId]->extent.width,
            renderTargets_[cameraInstanceId]->extent.height,
            renderTargets_[cameraInstanceId]->extent.depth);

        //SubmitWorkerCommandBuffer(commandBuffer, graphicsCommandPool_, graphicsQueue_);
    }

    void RayTracer::CopyRenderToRenderTarget(int cameraInstanceId, VkCommandBuffer commandBuffer)
    {
        // NOTE: assumes that renderTargets_ has already been checked

        UnityVulkanImage image;
        if (!graphicsInterface_->AccessTexture(renderTargets_[cameraInstanceId]->destination,
            UnityVulkanWholeImage,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            kUnityVulkanResourceAccess_PipelineBarrier,
            &image))
        {
            return;
        }

        // PFG_EDITORLOG("Copying render for " + std::to_string(cameraInstanceId));

        VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        VkImageCopy region;
        region.extent.width = renderTargets_[cameraInstanceId]->extent.width;
        region.extent.height = renderTargets_[cameraInstanceId]->extent.height;
        region.extent.depth = 1;
        region.srcOffset.x = 0;
        region.srcOffset.y = 0;
        region.srcOffset.z = 0;
        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.baseArrayLayer = 0;
        region.srcSubresource.layerCount = 1;
        region.srcSubresource.mipLevel = 0;
        region.dstOffset.x = 0;
        region.dstOffset.y = 0;
        region.dstOffset.z = 0;
        region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.baseArrayLayer = 0;
        region.dstSubresource.layerCount = 1;
        region.dstSubresource.mipLevel = 0;

        // Assign target image to be transfer optimal
        Vulkan::Image::UpdateImageBarrier(
            commandBuffer,
            renderTargets_[cameraInstanceId]->stagingImage.GetImage(),
            range,
            VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        // BUG? Unity destination is not set to the correct layout, do it here
        Vulkan::Image::UpdateImageBarrier(
            commandBuffer,
            image.image,
            range,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        vkCmdCopyImage(commandBuffer, renderTargets_[cameraInstanceId]->stagingImage.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        // Revert target image 
        Vulkan::Image::UpdateImageBarrier(
            commandBuffer,
            renderTargets_[cameraInstanceId]->stagingImage.GetImage(),
            range,
            VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);

        // BUG? Unity destination is not set to the correct layout, revert
        Vulkan::Image::UpdateImageBarrier(
            commandBuffer,
            image.image,
            range,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }

    void RayTracer::BuildDescriptorBufferInfos(int cameraInstanceId, uint64_t currentFrameNumber)
    {
        // NOTE: assumes that renderTargets_ has already been checked 
        auto& renderTarget = renderTargets_[cameraInstanceId];
        {
            const Vulkan::Buffer& buffer = renderTarget->cameraData;
            renderTarget->cameraDataBufferInfo.buffer = buffer.GetBuffer();
            renderTarget->cameraDataBufferInfo.offset = 0;
            renderTarget->cameraDataBufferInfo.range = buffer.GetSize();
        }

        //PFG_EDITORLOG("Updated BuildDescriptorBufferInfos for " + std::to_string(cameraInstanceId));

        // TODO: move all below here because its unnecessary to do this each build?
        {
            const Vulkan::Buffer& buffer = sceneData_;
            sceneBufferInfo_.buffer = buffer.GetBuffer();
            sceneBufferInfo_.offset = 0;
            sceneBufferInfo_.range = buffer.GetSize();
        }

        // NOTE: Instance index is the custom index used by the hit shader.
        //       Make sure all infos can map back to an instance with the same index as used in BuildTlas!
        uint32_t i;


        textureImageInfos_.clear();
        textureImageInfos_.resize(texturePool_.in_use_size() + 1);
        {
            const Vulkan::Image& image = blankTexture_;
            textureImageInfos_[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            textureImageInfos_[0].imageView = blankTexture_.GetImageView();
            textureImageInfos_[0].sampler = blankTexture_.GetSampler();
        }

        i = 1;
        for (auto itr = texturePool_.in_use_begin(); itr != texturePool_.in_use_end(); ++itr)
        {
            auto textureInstanceId = (*itr).first;
            {
                const std::unique_ptr<Vulkan::Image>& image = texturePool_[textureInstanceId];
                textureImageInfos_[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                textureImageInfos_[i].imageView = image->GetImageView();
                textureImageInfos_[i].sampler = image->GetSampler();
            }
            ++i;
        }

        materialBufferInfos_.clear();
        materialBufferInfos_.resize(materialPool_.in_use_size() + 1);
        {
            const Vulkan::Buffer& buffer = defaultMaterial_;
            materialBufferInfos_[0].buffer = buffer.GetBuffer();
            materialBufferInfos_[0].offset = 0;
            materialBufferInfos_[0].range = buffer.GetSize();

        }

        i = 1;
        for (auto itr = materialPool_.in_use_begin(); itr != materialPool_.in_use_end(); ++itr)
        {
            auto materialInstanceId = (*itr).first;
            {
                const std::unique_ptr<Vulkan::Buffer>& buffer = materialPool_[materialInstanceId];
                materialBufferInfos_[i].buffer = buffer->GetBuffer();
                materialBufferInfos_[i].offset = 0;
                materialBufferInfos_[i].range = buffer->GetSize();
            }
            ++i;
        }

        meshInstancesAttributesBufferInfos_.resize(meshInstancePool_.in_use_size());
        meshInstancesFacesBufferInfos_.resize(meshInstancePool_.in_use_size());
        meshInstancesDataBufferInfos_.resize(meshInstancePool_.in_use_size());
        i = 0;
        for (auto itr = meshInstancePool_.in_use_begin(); itr != meshInstancePool_.in_use_end(); ++itr)
        {
            auto gameObjectInstanceId = (*itr).first;
            auto sharedMeshInstanceId = meshInstancePool_[gameObjectInstanceId].sharedMeshInstanceId;

            // Setup attributes
            {
                const Vulkan::Buffer& buffer = sharedMeshesPool_[sharedMeshInstanceId]->attributesBuffer;
                meshInstancesAttributesBufferInfos_[i].buffer = buffer.GetBuffer();
                meshInstancesAttributesBufferInfos_[i].offset = 0;
                meshInstancesAttributesBufferInfos_[i].range = buffer.GetSize();
            }

            // Setup faces
            {
                const Vulkan::Buffer& buffer = sharedMeshesPool_[sharedMeshInstanceId]->facesBuffer;
                meshInstancesFacesBufferInfos_[i].buffer = buffer.GetBuffer();
                meshInstancesFacesBufferInfos_[i].offset = 0;
                meshInstancesFacesBufferInfos_[i].range = buffer.GetSize();
            }

            // Setup instance data
            {
                const Vulkan::Buffer& buffer = meshInstancePool_[gameObjectInstanceId].instanceData;
                meshInstancesDataBufferInfos_[i].buffer = buffer.GetBuffer();
                meshInstancesDataBufferInfos_[i].offset = 0;
                meshInstancesDataBufferInfos_[i].range = buffer.GetSize();
            }
            ++i;
        }

        lightsBufferInfos_.clear();
        lightsBufferInfos_.resize(lightsPool_.in_use_size());
        i = 0;
        for (auto itr = lightsPool_.in_use_begin(); itr != lightsPool_.in_use_end(); ++itr)
        {
            auto lightInstanceId = (*itr).first;

            {
                const std::unique_ptr<Vulkan::Buffer>& buffer = lightsPool_[lightInstanceId];
                lightsBufferInfos_[i].buffer = buffer->GetBuffer();
                lightsBufferInfos_[i].offset = 0;
                lightsBufferInfos_[i].range = buffer->GetSize();
            }

            ++i;
        }
    }
    
    void RayTracer::CreateDescriptorPool() 
    {   
        // TODO: ADD SOME VALUES TO MONITOR POOL SIZES!

        // Descriptors are not generated directly, but from a pool.  Create that pool here
        std::vector<VkDescriptorPoolSize> poolSizes({
            { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 5       },     // Top level acceleration structure
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              10       },    // Camera render targets
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             3000    },     // Scene data + Camera data 
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             3000    },     // Lights data + vertex attribs + materials
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,     3000    }      // Textures
            });
    
        VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
        descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolCreateInfo.pNext = nullptr;
        descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // This allows vkFreeDescriptorSets to be called
        descriptorPoolCreateInfo.maxSets = DESCRIPTOR_SET_SIZE * 100; 
        descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
    
        VK_CHECK("vkCreateDescriptorPool", vkCreateDescriptorPool(device_, &descriptorPoolCreateInfo, nullptr, &descriptorPool_));

        PFG_EDITORLOG("Successfully created descriptor pool");
    }
    
    void RayTracer::UpdateDescriptorSets(int cameraInstanceId, uint64_t currentFrameNumber)
    {
        // NOTE: assumes that renderTargets_ has already been checked 
        auto& renderTarget = renderTargets_[cameraInstanceId];

        renderTarget->descriptorSetsData.insert(std::make_pair(currentFrameNumber, RayTracerDescriptorSetsData()));
        auto& descriptorSetsData = renderTarget->descriptorSetsData[currentFrameNumber];
        
        // Update the descriptor sets with the actual data to store in memory.
    
        // Now use the pool to upload data for each descriptor
        descriptorSetsData.descriptorSets.resize(DESCRIPTOR_SET_SIZE);
        
        std::vector<uint32_t> variableDescriptorCounts;
        variableDescriptorCounts.resize(DESCRIPTOR_SET_SIZE);

        variableDescriptorCounts[DESCRIPTOR_SET_ACCELERATION_STRUCTURE & DESCRIPTOR_SET_SCENE_DATA & DESCRIPTOR_SET_CAMERA_DATA] = 1;
        variableDescriptorCounts[DESCRIPTOR_SET_RENDER_TARGET]                                                                   = 1;
        variableDescriptorCounts[DESCRIPTOR_SET_FACE_DATA]                                                                       = static_cast<uint32_t>(meshInstancesFacesBufferInfos_.size());
        variableDescriptorCounts[DESCRIPTOR_SET_VERTEX_ATTRIBUTES]                                                               = static_cast<uint32_t>(meshInstancesAttributesBufferInfos_.size());
        variableDescriptorCounts[DESCRIPTOR_SET_LIGHTS_DATA]                                                                     = static_cast<uint32_t>(lightsBufferInfos_.size() == 0 ? 1 : static_cast<uint32_t>(lightsBufferInfos_.size()));
        variableDescriptorCounts[DESCRIPTOR_SET_MATERIALS_DATA]                                                                  = static_cast<uint32_t>(materialBufferInfos_.size() == 0 ? 1 : static_cast<uint32_t>(materialBufferInfos_.size()));
        variableDescriptorCounts[DESCRIPTOR_SET_TEXTURES_DATA]                                                                   = static_cast<uint32_t>(textureImageInfos_.size() == 0 ? 1 : static_cast<uint32_t>(textureImageInfos_.size()));
        variableDescriptorCounts[DESCRIPTOR_SET_INSTANCE_DATA]                                                                   = static_cast<uint32_t>(meshInstancesDataBufferInfos_.size());

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
    
        VK_CHECK("vkAllocateDescriptorSets", vkAllocateDescriptorSets(device_, &descriptorSetAllocateInfo, descriptorSetsData.descriptorSets.data()));
    
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        {
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
                accelerationStructureWrite.dstSet = descriptorSetsData.descriptorSets[DESCRIPTOR_SET_ACCELERATION_STRUCTURE];
                accelerationStructureWrite.dstBinding = DESCRIPTOR_BINDING_ACCELERATION_STRUCTURE;
                accelerationStructureWrite.dstArrayElement = 0;
                accelerationStructureWrite.descriptorCount = 1;
                accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
                accelerationStructureWrite.pImageInfo = nullptr;
                accelerationStructureWrite.pBufferInfo = nullptr;
                accelerationStructureWrite.pTexelBufferView = nullptr;

                descriptorSetsData.acceleration_stucture_count += accelerationStructureWrite.descriptorCount;

                descriptorWrites.push_back(accelerationStructureWrite);
            }

            // Scene data
            {
                VkWriteDescriptorSet sceneBufferWrite;
                sceneBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                sceneBufferWrite.pNext = nullptr;
                sceneBufferWrite.dstSet = descriptorSetsData.descriptorSets[DESCRIPTOR_SET_SCENE_DATA];
                sceneBufferWrite.dstBinding = DESCRIPTOR_BINDING_SCENE_DATA;
                sceneBufferWrite.dstArrayElement = 0;
                sceneBufferWrite.descriptorCount = 1;
                sceneBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                sceneBufferWrite.pImageInfo = nullptr;
                sceneBufferWrite.pBufferInfo = &sceneBufferInfo_;
                sceneBufferWrite.pTexelBufferView = nullptr;

                descriptorSetsData.uniform_buffer_count += sceneBufferWrite.descriptorCount;

                descriptorWrites.push_back(sceneBufferWrite);
            }

            // Camera data
            {
                VkWriteDescriptorSet camdataBufferWrite;
                camdataBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                camdataBufferWrite.pNext = nullptr;
                camdataBufferWrite.dstSet = descriptorSetsData.descriptorSets[DESCRIPTOR_SET_CAMERA_DATA];
                camdataBufferWrite.dstBinding = DESCRIPTOR_BINDING_CAMERA_DATA;
                camdataBufferWrite.dstArrayElement = 0;
                camdataBufferWrite.descriptorCount = 1;
                camdataBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                camdataBufferWrite.pImageInfo = nullptr;
                camdataBufferWrite.pBufferInfo = &renderTargets_[cameraInstanceId]->cameraDataBufferInfo;
                camdataBufferWrite.pTexelBufferView = nullptr;

                descriptorSetsData.uniform_buffer_count += camdataBufferWrite.descriptorCount;

                descriptorWrites.push_back(camdataBufferWrite);
            }
        }

        {
            // Render target
            {
                // From renderTargets_ map
                VkDescriptorImageInfo descriptorRenderTargetGameImageInfo;
                descriptorRenderTargetGameImageInfo.sampler = VK_NULL_HANDLE;
                descriptorRenderTargetGameImageInfo.imageView = renderTargets_[cameraInstanceId]->stagingImage.GetImageView();
                descriptorRenderTargetGameImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

                VkWriteDescriptorSet renderTargetGameImageWrite;
                renderTargetGameImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                renderTargetGameImageWrite.pNext = nullptr;
                renderTargetGameImageWrite.dstSet = descriptorSetsData.descriptorSets[DESCRIPTOR_SET_RENDER_TARGET];
                renderTargetGameImageWrite.dstBinding = DESCRIPTOR_BINDING_RENDER_TARGET;
                renderTargetGameImageWrite.dstArrayElement = 0;
                renderTargetGameImageWrite.descriptorCount = 1;
                renderTargetGameImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                renderTargetGameImageWrite.pImageInfo = &descriptorRenderTargetGameImageInfo;
                renderTargetGameImageWrite.pBufferInfo = nullptr;
                renderTargetGameImageWrite.pTexelBufferView = nullptr;

                descriptorSetsData.storage_image_count += renderTargetGameImageWrite.descriptorCount;

                descriptorWrites.push_back(renderTargetGameImageWrite);
            }
        }

        {
            // Faces
            {
                VkWriteDescriptorSet facesBufferWrite;
                facesBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                facesBufferWrite.pNext = nullptr;
                facesBufferWrite.dstSet = descriptorSetsData.descriptorSets[DESCRIPTOR_SET_FACE_DATA];
                facesBufferWrite.dstBinding = DESCRIPTOR_BINDING_FACE_DATA;
                facesBufferWrite.dstArrayElement = 0;
                facesBufferWrite.descriptorCount = static_cast<uint32_t>(meshInstancesFacesBufferInfos_.size());
                facesBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                facesBufferWrite.pImageInfo = nullptr;
                facesBufferWrite.pBufferInfo = meshInstancesFacesBufferInfos_.data();
                facesBufferWrite.pTexelBufferView = nullptr;

                descriptorSetsData.storage_buffer_count += facesBufferWrite.descriptorCount;

                descriptorWrites.push_back(facesBufferWrite);
            }
        }

        {
            // Vertex attributes
            {
                VkWriteDescriptorSet attribsBufferWrite;
                attribsBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                attribsBufferWrite.pNext = nullptr;
                attribsBufferWrite.dstSet = descriptorSetsData.descriptorSets[DESCRIPTOR_SET_VERTEX_ATTRIBUTES];
                attribsBufferWrite.dstBinding = DESCRIPTOR_BINDING_VERTEX_ATTRIBUTES;
                attribsBufferWrite.dstArrayElement = 0;
                attribsBufferWrite.descriptorCount = static_cast<uint32_t>(meshInstancesAttributesBufferInfos_.size());
                attribsBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                attribsBufferWrite.pImageInfo = nullptr;
                attribsBufferWrite.pBufferInfo = meshInstancesAttributesBufferInfos_.data();
                attribsBufferWrite.pTexelBufferView = nullptr;

                descriptorSetsData.storage_buffer_count += attribsBufferWrite.descriptorCount;

                descriptorWrites.push_back(attribsBufferWrite);
            }
        }

        {
            // Lights
            {
                VkWriteDescriptorSet lightsBufferWrite;
                lightsBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                lightsBufferWrite.pNext = nullptr;
                lightsBufferWrite.dstSet = descriptorSetsData.descriptorSets[DESCRIPTOR_SET_LIGHTS_DATA];
                lightsBufferWrite.dstBinding = DESCRIPTOR_BINDING_LIGHTS_DATA;
                lightsBufferWrite.dstArrayElement = 0;
                lightsBufferWrite.descriptorCount = lightsBufferInfos_.size() == 0 ? 1 : static_cast<uint32_t>(lightsBufferInfos_.size());
                lightsBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                lightsBufferWrite.pImageInfo = nullptr;
                lightsBufferWrite.pBufferInfo = lightsBufferInfos_.size() == 0 ? &noLightBufferInfo_ : lightsBufferInfos_.data();
                lightsBufferWrite.pTexelBufferView = nullptr;

                descriptorSetsData.storage_buffer_count += lightsBufferWrite.descriptorCount;

                descriptorWrites.push_back(lightsBufferWrite);
            }
        }

        {
            // Materials
            {
                VkWriteDescriptorSet materialsBufferWrite;
                materialsBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                materialsBufferWrite.pNext = nullptr;
                materialsBufferWrite.dstSet = descriptorSetsData.descriptorSets[DESCRIPTOR_SET_MATERIALS_DATA];
                materialsBufferWrite.dstBinding = DESCRIPTOR_BINDING_MATERIALS_DATA;
                materialsBufferWrite.dstArrayElement = 0;
                materialsBufferWrite.descriptorCount = static_cast<uint32_t>(materialBufferInfos_.size());
                materialsBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                materialsBufferWrite.pImageInfo = nullptr;
                materialsBufferWrite.pBufferInfo = materialBufferInfos_.data();
                materialsBufferWrite.pTexelBufferView = nullptr;

                descriptorSetsData.storage_buffer_count += materialsBufferWrite.descriptorCount;

                descriptorWrites.push_back(materialsBufferWrite);
            }
        }

        {
            // Textures
            {
                VkWriteDescriptorSet texturesBufferWrite;
                texturesBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                texturesBufferWrite.pNext = nullptr;
                texturesBufferWrite.dstSet = descriptorSetsData.descriptorSets[DESCRIPTOR_SET_TEXTURES_DATA];
                texturesBufferWrite.dstBinding = DESCRIPTOR_BINDING_TEXTURES_DATA;
                texturesBufferWrite.dstArrayElement = 0;
                texturesBufferWrite.descriptorCount = static_cast<uint32_t>(textureImageInfos_.size());
                texturesBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                texturesBufferWrite.pImageInfo = textureImageInfos_.data();
                texturesBufferWrite.pBufferInfo = nullptr;
                texturesBufferWrite.pTexelBufferView = nullptr;

                descriptorSetsData.combined_image_sampler_count += texturesBufferWrite.descriptorCount;

                descriptorWrites.push_back(texturesBufferWrite);
            }
        }

        {
            // Instance Data
            {
                VkWriteDescriptorSet instanceDataBufferWrite;
                instanceDataBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                instanceDataBufferWrite.pNext = nullptr;
                instanceDataBufferWrite.dstSet = descriptorSetsData.descriptorSets[DESCRIPTOR_SET_INSTANCE_DATA];
                instanceDataBufferWrite.dstBinding = DESCRIPTOR_BINDING_INSTANCE_DATA;
                instanceDataBufferWrite.dstArrayElement = 0;
                instanceDataBufferWrite.descriptorCount = static_cast<uint32_t>(meshInstancesDataBufferInfos_.size());
                instanceDataBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                instanceDataBufferWrite.pImageInfo = nullptr;
                instanceDataBufferWrite.pBufferInfo = meshInstancesDataBufferInfos_.data();
                instanceDataBufferWrite.pTexelBufferView = nullptr;

                descriptorSetsData.storage_buffer_count += instanceDataBufferWrite.descriptorCount;

                descriptorWrites.push_back(instanceDataBufferWrite);
            }
        }

        vkUpdateDescriptorSets(device_, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, VK_NULL_HANDLE);
    }

    void RayTracer::UpdateMaterialsTextureIndices()
    {
        for (auto itr = materialPool_.in_use_begin(); itr != materialPool_.in_use_end(); ++itr)
        {
            auto materialInstanceId = (*itr).first;
            auto material = reinterpret_cast<ShaderMaterialData*>(materialPool_[materialInstanceId]->Map());
            UpdateMaterialTextureIndices(material);
            materialPool_[materialInstanceId]->Unmap();
        }
    }

    void RayTracer::UpdateMaterialTextureIndices(ShaderMaterialData * const material)
    {
        // Remember, i == 0 is the "default texture"
        int i = 1;
        for (auto itr = texturePool_.in_use_begin(); itr != texturePool_.in_use_end(); ++itr)
        {
            auto textureInstanceId = (*itr).first;
            auto texture = (*itr).second;

            if (material->albedoTextureInstanceId == textureInstanceId)
            {
                material->albedoTextureIndex = i;
            }

            if (material->ambientOcclusionTextureInstanceId == textureInstanceId)
            {
                material->ambientOcclusionTextureIndex = i;
            }

            if (material->emissionTextureInstanceId == textureInstanceId)
            {
                material->emissionTextureIndex = i;
            }
            
            if (material->metallicTextureInstanceId == textureInstanceId)
            {
                material->metallicTextureIndex = i;
            }

            if (material->normalTextureInstanceId == textureInstanceId)
            {
                material->normalTextureIndex = i;
            }

            if (material->roughnessTextureInstanceId == textureInstanceId)
            {
                material->roughnessTextureIndex = i;
            }
            ++i;
        }
    }

    void RayTracer::GarbageCollect(uint64_t frameCount)
    {

        // Clean up buffers
        std::vector<uint32_t> removeIndices;
        for (int32_t i = 0; i < static_cast<int32_t>(garbageBuffers_.size()); ++i)
        {
            if (garbageBuffers_[i].frameCount < frameCount)
            {
                garbageBuffers_[i].buffer->Destroy();
                garbageBuffers_[i].buffer.release();
                removeIndices.push_back(i);
            }
        }

        for (int32_t i = static_cast<int32_t>(removeIndices.size()) - 1; i >= 0; --i)
        {
            garbageBuffers_.erase(garbageBuffers_.begin() + removeIndices[i]);
        }

        // Clean up descriptor sets
        std::vector<uint64_t> removeFrames;
        for (auto renderItr = renderTargets_.begin(); renderItr != renderTargets_.end(); ++renderItr)
        {
            auto& renderTarget = (*renderItr).second;

            removeFrames.clear();
            for (auto itr = renderTarget->descriptorSetsData.begin(); itr != renderTarget->descriptorSetsData.end(); ++itr)
            {
                auto frameNumber = (*itr).first;
                auto& descriptorSetsData = (*itr).second;

                if (frameNumber < frameCount)
                {
                    vkFreeDescriptorSets(device_, descriptorPool_, DESCRIPTOR_SET_SIZE, descriptorSetsData.descriptorSets.data());
                    removeFrames.push_back(frameNumber);
                }
            }

            for (auto removeFrameNumber : removeFrames)
            {
                renderTarget->descriptorSetsData.erase(removeFrameNumber);
            }
        }
    }
}
