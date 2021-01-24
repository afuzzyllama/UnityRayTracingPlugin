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

        PFG_EDITORLOG("Queues indices successfully reoslved")

        // Get the ray tracing pipeline properties, which we'll need later on in the sample
        PixelsForGlory::Vulkan::RayTracer::Instance().rayTracingProperties_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

        VkPhysicalDeviceProperties2 physicalDeviceProperties = { };
        physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        physicalDeviceProperties.pNext = &PixelsForGlory::Vulkan::RayTracer::Instance().rayTracingProperties_;

        PFG_EDITORLOG("Getting physical device properties")
        vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties);

        // Get memory properties
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &PixelsForGlory::Vulkan::RayTracer::Instance().physicalDeviceMemoryProperties_);
    }

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
        PFG_EDITORLOG("Getting device queues")
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
        , rebuildTlas_(true)
        , updateTlas_(false)
        , tlas_(RayTracerAccelerationStructure())
    {}

    void RayTracer::InitializeFromUnityInstance(IUnityGraphicsVulkan* graphicsInterface)
    {
        graphicsInterface_ = graphicsInterface;
        device_ = graphicsInterface_->Instance().device;

        // Setup one off command pools
        CreateCommandPool(graphicsQueueFamilyIndex_, graphicsCommandPool_);
        CreateCommandPool(transferQueueFamilyIndex_, transferCommandPool_);  
    }

    void RayTracer::Shutdown()
    {
        if (graphicsCommandPool_ != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device_, graphicsCommandPool_, nullptr);
            graphicsCommandPool_ = VK_NULL_HANDLE;
        }

        if (transferCommandPool_ != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(device_, transferCommandPool_, nullptr);
            transferCommandPool_ = VK_NULL_HANDLE;
        }

        for (auto const& itr : renderTargets_)
        {
            auto target = itr.second.get();
            target->image.Destroy();
        }

        for (auto itr = sharedMeshesPool_.pool_begin(); itr != sharedMeshesPool_.pool_end(); ++itr)
        {
            auto const& mesh = (*itr);

            mesh->vertexBuffer.Destroy();
            mesh->indexBuffer.Destroy();
            
            if (mesh->blas.accelerationStructure != VK_NULL_HANDLE)
            {
                vkDestroyAccelerationStructureKHR(device_, mesh->blas.accelerationStructure, nullptr);
                mesh->blas.accelerationStructure = VkAccelerationStructureKHR();
                mesh->blas.buffer.Destroy();
                mesh->blas.deviceAddress = 0;
            }
        }

        instancesAccelerationStructuresBuffer_.Destroy();

        if (tlas_.accelerationStructure != VK_NULL_HANDLE)
        {
            vkDestroyAccelerationStructureKHR(device_, tlas_.accelerationStructure, nullptr);
        }
        tlas_.buffer.Destroy();

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

    int RayTracer::SetRenderTarget(int cameraInstanceId, int unityTextureFormat, int width, int height, void* textureHandle)
    {
        VkFormat vkFormat;
        switch (unityTextureFormat)
        {
        // TextureFormat.RGBA32
        case 4:
            vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
            break;
        default:
            Debug::LogError("Attempted to set an unsupported Unity texture format" + std::to_string(unityTextureFormat));
            return 0;
        }

        // If the target already exists, make sure to destroy it first
        bool insert = false;
        if (renderTargets_.find(cameraInstanceId) != renderTargets_.end())
        {
            renderTargets_[cameraInstanceId]->image.Destroy();
        }
        else
        {
            renderTargets_.insert(std::make_pair(cameraInstanceId, std::make_unique<RayTracerRenderTarget>()));
            insert = true;
        }

        auto& target = renderTargets_[cameraInstanceId];
        target->format = vkFormat;
        target->extent.width = width;
        target->extent.height = height;
        target->extent.depth = 1;
        target->destination = textureHandle;

        target->image = Vulkan::Image(device_, physicalDeviceMemoryProperties_);
        
        if(target->image.Create(
            VK_IMAGE_TYPE_2D,
            target->format,
            target->extent,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != VK_SUCCESS) {

            Debug::LogError("Failed to create render image!");
            renderTargets_[cameraInstanceId]->image.Destroy();
            renderTargets_[cameraInstanceId] = nullptr;
            return 0;
        }

        VkImageSubresourceRange range = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        if (target->image.CreateImageView(VK_IMAGE_VIEW_TYPE_2D, target->format, range) != VK_SUCCESS) {
            Debug::LogError("Failed to create render image view!");
            renderTargets_[cameraInstanceId]->image.Destroy();
            renderTargets_[cameraInstanceId] = nullptr;
            return 0;
        }

        // Only display on creation
        if(insert)
        { 
            Debug::Log("Successfully set render target for camera: " + std::to_string(cameraInstanceId));
        }
        
        return 1;
    }
        
    int RayTracer::GetSharedMeshIndex(int sharedMeshInstanceId) 
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

    int RayTracer::AddSharedMesh(int instanceId, float* verticesArray, float* normalsArray, float* uvsArray, int vertexCount, int* indicesArray, int indexCount) 
    { 
        // Check that this shared mesh hasn't been added yet
        for (auto itr = sharedMeshesPool_.in_use_begin(); itr != sharedMeshesPool_.in_use_end(); ++itr)
        {
            auto i = (*itr);
            if (sharedMeshesPool_[i]->sharedMeshInstanceId == instanceId)
            {
                return i;
            }
        }
        
        // We can only add tris, make sure the index count reflects this
        assert(indexCount % 3 == 0);
    
        auto sentMesh = std::make_unique<RayTracerMeshSharedData>();
        
        // Setup where we are going to store the shared mesh data and all data needed for shaders
        sentMesh->sharedMeshInstanceId = instanceId;
        sentMesh->vertexCount = vertexCount;
        sentMesh->indexCount = indexCount;

        sentMesh->vertexAttributeIndex = sharedMeshAttributesPool_.get_next_index();
        sentMesh->faceDataIndex = sharedMeshFacesPool_.get_next_index();
    
        Vulkan::Buffer& sentMeshAttributes = sharedMeshAttributesPool_[sentMesh->vertexAttributeIndex];
        Vulkan::Buffer& sentMeshFaces = sharedMeshFacesPool_[sentMesh->faceDataIndex];
    
        // Setup buffers
        bool success = true;
        if (sentMesh->vertexBuffer.Create(
                device_,
                physicalDeviceMemoryProperties_,
                sizeof(vec3) * vertexCount,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                Vulkan::Buffer::kDefaultMemoryPropertyFlags) 
            != VK_SUCCESS)
        {
            Debug::LogError("Failed to create vertex buffer for shared mesh instance id " + std::to_string(instanceId));
            success = false;
        }
    
        if (sentMesh->indexBuffer.Create(
            device_,
            physicalDeviceMemoryProperties_,
            sizeof(int) * indexCount,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            Vulkan::Buffer::kDefaultMemoryPropertyFlags))
        {
            Debug::LogError("Failed to create index buffer for shared mesh instance id " + std::to_string(instanceId));
            success = false;
        }
    
        if (sentMeshAttributes.Create(
            device_,
                physicalDeviceMemoryProperties_,
                sizeof(ShaderVertexAttribute) * vertexCount,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                Vulkan::Buffer::kDefaultMemoryPropertyFlags)
            != VK_SUCCESS)
        {
            Debug::LogError("Failed to create vertex attribute buffer for shared mesh instance id " + std::to_string(instanceId));
            success = false;
        }
    
    
        if (sentMeshFaces.Create(
            device_,
            physicalDeviceMemoryProperties_,
            sizeof(ShaderFace) * vertexCount / 3,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            Vulkan::Buffer::kDefaultMemoryPropertyFlags)
            != VK_SUCCESS)
        {
            Debug::LogError("Failed to create face buffer for shared mesh instance id " + std::to_string(instanceId));
            success = false;
        }
    
        
        if (!success)
        {
            return -1;
        }
    
        // Creating buffers was successful.  Move onto getting the data in there
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
    
        // All done creating the data, get it added to the pool
        int sharedMeshIndex = sharedMeshesPool_.add(std::move(sentMesh));
    
        // Build blas here so we don't have to do it later
        BuildBlas(sharedMeshIndex);
    
        Debug::Log("Added mesh (sharedMeshInstanceId: " + std::to_string(instanceId) + ")");
    
        return sharedMeshIndex;
    
    }

    int RayTracer::AddTlasInstance(int sharedMeshIndex, float* l2wMatrix) 
    { 
        auto instance = std::make_unique<RayTracerMeshInstanceData>();

        instance->sharedMeshIndex = sharedMeshIndex;
        FloatArrayToMatrix(l2wMatrix, instance->localToWorld);

        int index = meshInstancePool_.add(std::move(instance));

        Debug::Log("Added mesh instance (sharedMeshIndex: " + std::to_string(sharedMeshIndex) + ")");

        // If we added an instance, we need to rebuild the tlas
        rebuildTlas_ = true;

        return index; 
    }

    void RayTracer::RemoveTlasInstance(int meshInstanceIndex) 
    {
        meshInstancePool_.remove(meshInstanceIndex);

        // If we added an instance, we need to rebuild the tlas
        rebuildTlas_ = true;
    }

    void RayTracer::BuildTlas() 
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

        
        if (!update)
        {
            // Build instance buffer from scratch
            std::vector<VkAccelerationStructureInstanceKHR> instanceAccelerationStructures;
            instanceAccelerationStructures.resize(meshInstancePool_.in_use_size(), VkAccelerationStructureInstanceKHR{});

            // Gather instances
            uint32_t instanceAccelerationStructuresIndex = 0;
            for (auto i = meshInstancePool_.in_use_begin(); i != meshInstancePool_.in_use_end(); ++i)
            {
                auto instanceIndex = (*i);

                const auto& t = meshInstancePool_[instanceIndex]->localToWorld;
                VkTransformMatrixKHR transform_matrix = {
                    t[0][0], t[0][1], t[0][2], t[0][3],
                    t[1][0], t[1][1], t[1][2], t[1][3],
                    t[2][0], t[2][1], t[2][2], t[2][3]
                };

                auto sharedMeshIndex = meshInstancePool_[instanceIndex]->sharedMeshIndex;

                VkAccelerationStructureInstanceKHR& accelerationStructureInstance = instanceAccelerationStructures[instanceAccelerationStructuresIndex];
                accelerationStructureInstance.transform = transform_matrix;
                accelerationStructureInstance.instanceCustomIndex = instanceIndex;
                accelerationStructureInstance.mask = 0xFF;
                accelerationStructureInstance.instanceShaderBindingTableRecordOffset = 0;
                accelerationStructureInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
                accelerationStructureInstance.accelerationStructureReference = sharedMeshesPool_[meshInstancePool_[instanceIndex]->sharedMeshIndex]->blas.deviceAddress;

                // Consumed current index, advance
                ++instanceAccelerationStructuresIndex;
            }

            // Destroy anytihng if created before
            instancesAccelerationStructuresBuffer_.Destroy();

            instancesAccelerationStructuresBuffer_.Create(
                device_,
                physicalDeviceMemoryProperties_,
                instanceAccelerationStructures.size() * sizeof(VkAccelerationStructureInstanceKHR),
                VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                Vulkan::Buffer::kDefaultMemoryPropertyFlags);
            instancesAccelerationStructuresBuffer_.UploadData(instanceAccelerationStructures.data(), instancesAccelerationStructuresBuffer_.GetSize());
        }
        else
        {
            // Update transforms in buffer, should be the exact same layout

            auto instances = reinterpret_cast<VkAccelerationStructureInstanceKHR*>(instancesAccelerationStructuresBuffer_.Map());

            // Gather instances
            uint32_t instanceAccelerationStructuresIndex = 0;
            for (auto i = meshInstancePool_.in_use_begin(); i != meshInstancePool_.in_use_end(); ++i)
            {
                auto instanceIndex = (*i);

                const auto& t = meshInstancePool_[instanceIndex]->localToWorld;
                VkTransformMatrixKHR transform_matrix = {
                    t[0][0], t[0][1], t[0][2], t[0][3],
                    t[1][0], t[1][1], t[1][2], t[1][3],
                    t[2][0], t[2][1], t[2][2], t[2][3]
                };

                instances[instanceAccelerationStructuresIndex].transform = transform_matrix;
                

                // Consumed current index, advance
                ++instanceAccelerationStructuresIndex;
            }
            instancesAccelerationStructuresBuffer_.Unmap();
        }

        // The top level acceleration structure contains (bottom level) instance as the input geometry
        VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {};
        accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
        accelerationStructureGeometry.geometry.instances.data = instancesAccelerationStructuresBuffer_.GetBufferDeviceAddressConst();

        // Get the size requirements for buffers involved in the acceleration structure build process
        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {};
        accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount = 1;
        accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

        // Number of acceleration structures
        const uint32_t primitiveCount = static_cast<uint32_t>(meshInstancePool_.in_use_size());

        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {};
        accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(
            device_,
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &accelerationStructureBuildGeometryInfo,
            &primitiveCount,
            &accelerationStructureBuildSizesInfo);

        // Create a buffer to hold the acceleration structure
        tlas_.buffer.Create(
            device_,
            physicalDeviceMemoryProperties_,
            accelerationStructureBuildSizesInfo.accelerationStructureSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
            Vulkan::Buffer::kDefaultMemoryPropertyFlags);

        
        if (!update)
        {
            // Create the acceleration structure
            VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {};
            accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
            accelerationStructureCreateInfo.buffer = tlas_.buffer.GetBuffer();
            accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
            accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
            VK_CHECK("vkCreateAccelerationStructureKHR", vkCreateAccelerationStructureKHR(device_, &accelerationStructureCreateInfo, nullptr, &tlas_.accelerationStructure))
        }

        // The actual build process starts here

        // Create a scratch buffer as a temporary storage for the acceleration structure build
        Vulkan::Buffer scratchBuffer;
        scratchBuffer.Create(
            device_,
            physicalDeviceMemoryProperties_,
            accelerationStructureBuildSizesInfo.buildScratchSize,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo = {};
        accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationBuildGeometryInfo.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        accelerationBuildGeometryInfo.srcAccelerationStructure = update ? tlas_.accelerationStructure : VK_NULL_HANDLE;
        accelerationBuildGeometryInfo.dstAccelerationStructure = tlas_.accelerationStructure;
        accelerationBuildGeometryInfo.geometryCount = 1;
        accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
        accelerationBuildGeometryInfo.scratchData = scratchBuffer.GetBufferDeviceAddress();

        VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo;
        accelerationStructureBuildRangeInfo.primitiveCount = 1;
        accelerationStructureBuildRangeInfo.primitiveOffset = 0;
        accelerationStructureBuildRangeInfo.firstVertex = 0;
        accelerationStructureBuildRangeInfo.transformOffset = 0;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationStructureBuildRangeInfos = { &accelerationStructureBuildRangeInfo };

        // Build the acceleration structure on the device via a one-time command buffer submission
        // Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
        VkCommandBuffer commandBuffer;
        CreateWorkerCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, graphicsCommandPool_, commandBuffer);
        vkCmdBuildAccelerationStructuresKHR(
            commandBuffer,
            1,
            &accelerationBuildGeometryInfo,
            accelerationStructureBuildRangeInfos.data());
        SubmitWorkerCommandBuffer(commandBuffer, graphicsQueue_);

        scratchBuffer.Destroy();

        // Get the top acceleration structure's handle, which will be used to setup it's descriptor
        VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo = {};
        accelerationStructureDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        accelerationStructureDeviceAddressInfo.accelerationStructure = tlas_.accelerationStructure;
        tlas_.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device_, &accelerationStructureDeviceAddressInfo);

        Debug::Log("Succesfully built tlas");
        
        // We did any pending work, reset flags
        rebuildTlas_= false;
        updateTlas_ = false;
    }

    void RayTracer::Prepare(int width, int height) {}

    void RayTracer::UpdateCamera(float* camPos, float* camDir, float* camUp, float* camSide, float* camNearFarFov) {}

    void RayTracer::UpdateSceneData(float* color) {}

    void RayTracer::TraceRays() {}

    void RayTracer::CopyImageToTexture(void* TextureHandle) {}

#pragma endregion RayTracerAPI

    void RayTracer::CreateCommandPool(uint32_t queueFamilyIndex, VkCommandPool& outCommandPool)
    {
        VkCommandPoolCreateInfo commandPoolCreateInfo = {};
        commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
        commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VK_CHECK("vkCreateCommandPool", vkCreateCommandPool(device_, &commandPoolCreateInfo, nullptr, &outCommandPool))
    }

    void RayTracer::CreateWorkerCommandBuffer(VkCommandBufferLevel level, const VkCommandPool& commandPool, VkCommandBuffer& outCommandBuffer)
    {
        VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
        commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferAllocateInfo.commandPool = commandPool;
        commandBufferAllocateInfo.level = level;
        commandBufferAllocateInfo.commandBufferCount = 1;

        VK_CHECK("vkAllocateCommandBuffers", vkAllocateCommandBuffers(device_, &commandBufferAllocateInfo, &outCommandBuffer))
    
        // Start recording for the new command buffer
        VkCommandBufferBeginInfo commandBufferBeginInfo = { };
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_CHECK("vkBeginCommandBuffer", vkBeginCommandBuffer(outCommandBuffer, &commandBufferBeginInfo))
    }

    void RayTracer::SubmitWorkerCommandBuffer(VkCommandBuffer commandBuffer, const VkQueue& queue)
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
        VK_CHECK("vkCreateFence", vkCreateFence(device_, &fenceCreateInfo, nullptr, &fence))
    
        // Submit to the queue
        VkResult result = vkQueueSubmit(queue, 1, &submitInfo, fence);
        VK_CHECK("vkQueueSubmit", result)
    
        // Wait for the fence to signal that command buffer has finished executing
        const uint64_t DEFAULT_FENCE_TIMEOUT = 100000000000;
        VK_CHECK("vkWaitForFences", vkWaitForFences(device_, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT))
    
        vkDestroyFence(device_, fence, nullptr);
    }

    void RayTracer::BuildBlas(int sharedMeshPoolIndex)
    {
        // Create buffers for the bottom level geometry
    
        // Setup a single transformation matrix that can be used to transform the whole geometry for a single bottom level acceleration structure
        VkTransformMatrixKHR transformMatrix = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f };
        
        Vulkan::Buffer transformBuffer;
        transformBuffer.Create(
            device_, 
            physicalDeviceMemoryProperties_, 
            sizeof(transformMatrix), 
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            Vulkan::Buffer::kDefaultMemoryPropertyFlags);

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
        accelerationStructureGeometry.geometry.triangles.vertexData = sharedMeshesPool_[sharedMeshPoolIndex]->vertexBuffer.GetBufferDeviceAddressConst();
        accelerationStructureGeometry.geometry.triangles.maxVertex = sharedMeshesPool_[sharedMeshPoolIndex]->vertexCount;
        accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(vec3);
        accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        accelerationStructureGeometry.geometry.triangles.indexData = sharedMeshesPool_[sharedMeshPoolIndex]->indexBuffer.GetBufferDeviceAddressConst();
        accelerationStructureGeometry.geometry.triangles.transformData = transformBuffer.GetBufferDeviceAddressConst();
        accelerationStructureGeometry.geometry.triangles.pNext = nullptr;
    
        // Get the size requirements for buffers involved in the acceleration structure build process
        VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {};
        accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationStructureBuildGeometryInfo.geometryCount = 1;
        accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

        // Number of triangles 
        const uint32_t primitiveCount = sharedMeshesPool_[sharedMeshPoolIndex]->indexCount / 3;

        VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = {};
        accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        vkGetAccelerationStructureBuildSizesKHR(
            device_,
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &accelerationStructureBuildGeometryInfo,
            &primitiveCount,
            &accelerationStructureBuildSizesInfo);

        // Create a buffer to hold the acceleration structure
        sharedMeshesPool_[sharedMeshPoolIndex]->blas.buffer.Create(
            device_,
            physicalDeviceMemoryProperties_,
            accelerationStructureBuildSizesInfo.accelerationStructureSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
            Vulkan::Buffer::kDefaultMemoryPropertyFlags);
    
        // Create the acceleration structure
        VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {};
        accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationStructureCreateInfo.buffer = sharedMeshesPool_[sharedMeshPoolIndex]->blas.buffer.GetBuffer();
        accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
        accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        VK_CHECK("vkCreateAccelerationStructureKHR", vkCreateAccelerationStructureKHR(device_ , &accelerationStructureCreateInfo, nullptr, &sharedMeshesPool_[sharedMeshPoolIndex]->blas.accelerationStructure))

        // The actual build process starts here
        // Create a scratch buffer as a temporary storage for the acceleration structure build
        Vulkan::Buffer scratchBuffer;
        scratchBuffer.Create(
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
        accelerationBuildGeometryInfo.dstAccelerationStructure = sharedMeshesPool_[sharedMeshPoolIndex]->blas.accelerationStructure;
        accelerationBuildGeometryInfo.geometryCount = 1;
        accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
        accelerationBuildGeometryInfo.scratchData = scratchBuffer.GetBufferDeviceAddress();

        VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo = { };
        accelerationStructureBuildRangeInfo.primitiveCount = 1;
        accelerationStructureBuildRangeInfo.primitiveOffset = 0;
        accelerationStructureBuildRangeInfo.firstVertex = 0;
        accelerationStructureBuildRangeInfo.transformOffset = 0;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationStructureBuildRangeInfos = { &accelerationStructureBuildRangeInfo };

        // Build the acceleration structure on the device via a one-time command buffer submission.  We will NOT use the Unity command buffer in this case
        // Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds


        
        VkCommandBuffer buildCommandBuffer;
        CreateWorkerCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, graphicsCommandPool_, buildCommandBuffer);
        vkCmdBuildAccelerationStructuresKHR(
            buildCommandBuffer,
            1,
            &accelerationBuildGeometryInfo,
            accelerationStructureBuildRangeInfos.data());
        SubmitWorkerCommandBuffer(buildCommandBuffer, graphicsQueue_);

        scratchBuffer.Destroy();

        // Get the bottom acceleration structure's handle, which will be used during the top level acceleration build
        VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo{};
        accelerationStructureDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
        accelerationStructureDeviceAddressInfo.accelerationStructure = sharedMeshesPool_[sharedMeshPoolIndex]->blas.accelerationStructure;
        sharedMeshesPool_[sharedMeshPoolIndex]->blas.deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device_, &accelerationStructureDeviceAddressInfo);

        Debug::Log("Built blas for mesh (sharedMeshInstanceId: " + std::to_string(sharedMeshesPool_[sharedMeshPoolIndex]->sharedMeshInstanceId) + ")");
    }
}