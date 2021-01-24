//void PixelsForGlory::RayTracerAPI_Vulkan::Destroy()
//{
//    if (offscreenImage_ )
//    {
//        offscreenImage_->Destroy();
//        offscreenImage_.release();
//        offscreenImage_ = nullptr;
//    }
//
//    // Destory all added mesh buffers
//    for (auto i = sharedMeshesPool_.pool_begin(); i != sharedMeshesPool_.pool_end(); ++i)
//    {
//        auto mesh = (*i).get();
//
//        mesh->vertexBuffer.Destroy();
//        mesh->indexBuffer.Destroy();
//
//        if (mesh->blas.accelerationStructure != VK_NULL_HANDLE)
//        {
//            mesh->blas.buffer.Destroy();
//            vkDestroyAccelerationStructureKHR(rayTracerVulkanInstance_.device, mesh->blas.accelerationStructure, nullptr);
//        }
//    }
//
//    for (auto i = sharedMeshAttributesPool_.pool_begin(); i != sharedMeshAttributesPool_.pool_end(); ++i)
//    {
//        (*i).Destroy();
//    }
//
//    for (auto i = sharedMeshFacesPool_.pool_begin(); i != sharedMeshFacesPool_.pool_end(); ++i)
//    {
//        (*i).Destroy();
//    }
//
//    if (tlas_.accelerationStructure != VK_NULL_HANDLE)
//    {
//        tlas_.buffer.Destroy();
//        vkDestroyAccelerationStructureKHR(rayTracerVulkanInstance_.device, tlas_.accelerationStructure, nullptr);
//    }
//
//    if (descriptorPool_ != VK_NULL_HANDLE) {
//        vkDestroyDescriptorPool(rayTracerVulkanInstance_.device, descriptorPool_, nullptr);
//        descriptorPool_ = VK_NULL_HANDLE;
//    }
//
//    shaderBindingTable_.Destroy();
//
//    if (pipeline_ != VK_NULL_HANDLE) {
//        vkDestroyPipeline(rayTracerVulkanInstance_.device, pipeline_, nullptr);
//        pipeline_ = VK_NULL_HANDLE;
//    }
//
//    if (pipelineLayout_ != VK_NULL_HANDLE) {
//        vkDestroyPipelineLayout(rayTracerVulkanInstance_.device, pipelineLayout_, nullptr);
//        pipelineLayout_ = VK_NULL_HANDLE;
//    }
//
//    for (auto descriptorSetLayout : descriptorSetLayouts_)
//    {
//        vkDestroyDescriptorSetLayout(rayTracerVulkanInstance_.device, descriptorSetLayout, nullptr);
//    }
//    descriptorSetLayouts_.clear();
//}

//void PixelsForGlory::RayTracerAPI_Vulkan::Prepare(int width, int height)
//{
//    width_ = width;
//    height_ = height;
//    CreateDescriptorSetsLayouts();
//    CreatePipeline();
//    BuildDescriptorBufferInfos();
//    CreateDescriptorPool();
//    UpdateDescriptorSets();
//    BuildCommandBuffer();
//    ready_ = true;
//}
//
//void PixelsForGlory::RayTracerAPI_Vulkan::TraceRays()
//{
//    if (!ready_)
//    {
//        return;
//    }
//
//    rayTracerVulkanInstance_.SubmitCommandBuffer(commandBuffer_, rayTracerVulkanInstance_.graphicsQueue);
//}
//
//void PixelsForGlory::RayTracerAPI_Vulkan::CopyImageToTexture(void* textureHandle)
//{
//    if (!ready_)
//    {
//        return;
//    }
//
//    // cannot do resource uploads inside renderpass
//    graphicsInterface_->EnsureOutsideRenderPass();
//
//    UnityVulkanImage image;
//    if (!graphicsInterface_->AccessTexture(textureHandle,
//                                           UnityVulkanWholeImage,
//                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//                                           VK_PIPELINE_STAGE_TRANSFER_BIT,
//                                           VK_ACCESS_TRANSFER_WRITE_BIT,
//                                           kUnityVulkanResourceAccess_PipelineBarrier,
//                                           &image))
//    {
//        return;
//    }
//
//    UnityVulkanRecordingState recordingState;
//    if (!graphicsInterface_->CommandRecordingState(&recordingState, kUnityVulkanGraphicsQueueAccess_DontCare))
//    {
//        return;
//    }
//
//    VkImageCopy region;
//    region.extent.width = width_;
//    region.extent.height = height_;
//    region.extent.depth = 1;
//    region.srcOffset.x = 0;
//    region.srcOffset.y = 0;
//    region.srcOffset.z = 0;
//    region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//    region.srcSubresource.baseArrayLayer = 0;
//    region.srcSubresource.layerCount = 1;
//    region.srcSubresource.mipLevel = 0;
//    region.dstOffset.x = 0;
//    region.dstOffset.y = 0;
//    region.dstOffset.z = 0;
//    region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//    region.dstSubresource.baseArrayLayer = 0;
//    region.dstSubresource.layerCount = 1;
//    region.dstSubresource.mipLevel = 0;
//
//    vkCmdCopyImage(recordingState.commandBuffer, offscreenImage_->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
//}
//
//void PixelsForGlory::RayTracerAPI_Vulkan::UpdateCamera(float* camPos, float* camDir, float* camUp, float* camSide, float* camNearFarFov)
//{
//    if (cameraData_.GetBuffer() == VK_NULL_HANDLE)
//    {
//        cameraData_.Create(
//            rayTracerVulkanInstance_.device,
//            rayTracerVulkanInstance_.physicalDeviceMemoryProperties,
//            sizeof(ShaderCameraParam),
//            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
//            VulkanBuffer::kDefaultMemoryPropertyFlags);
//    }
//
//    auto camera = reinterpret_cast<ShaderCameraParam*>(cameraData_.Map());
//    camera->camPos.x = camPos[0];
//    camera->camPos.y = camPos[1];
//    camera->camPos.z = camPos[2];
//
//    camera->camDir.x = camDir[0];
//    camera->camDir.y = camDir[1];
//    camera->camDir.z = camDir[2];
//
//    camera->camUp.x = camUp[0];
//    camera->camUp.y = camUp[1];
//    camera->camUp.z = camUp[2];
//
//    camera->camSide.x = camSide[0];
//    camera->camSide.y = camSide[1];
//    camera->camSide.z = camSide[2];
//
//    camera->camNearFarFov.x = camNearFarFov[0];
//    camera->camNearFarFov.y = camNearFarFov[1];
//    camera->camNearFarFov.z = camNearFarFov[2];
//
//    cameraData_.Unmap();
//}
//
//void PixelsForGlory::RayTracerAPI_Vulkan::UpdateSceneData(float* color)
//{
//    if (sceneData_.GetBuffer() == VK_NULL_HANDLE)
//    {
//        sceneData_.Create(
//            rayTracerVulkanInstance_.device,
//            rayTracerVulkanInstance_.physicalDeviceMemoryProperties,
//            sizeof(ShaderSceneParam),
//            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
//            VulkanBuffer::kDefaultMemoryPropertyFlags);
//    }
//
//    auto scene = reinterpret_cast<ShaderSceneParam*>(sceneData_.Map());
//    scene->ambient.r = color[0];
//    scene->ambient.g = color[1];
//    scene->ambient.b = color[2];
//    scene->ambient.a = color[3];
//}
//
//void PixelsForGlory::RayTracerAPI_Vulkan::CreateDescriptorSetsLayouts() {
//
//    // Create descriptor sets for the shader.  This setups up how data is bound to GPU memory and what shader stages will have access to what memory
//    //const uint32_t numMeshes = static_cast<uint32_t>(sharedMeshesPool_.pool_size());
//    //const uint32_t numMaterials = static_cast<uint32_t>(_scene.materials.size());
//    //const uint32_t numTextures = static_cast<uint32_t>(_scene.textures.size());
//    //const uint32_t numLights = static_cast<uint32_t>(_scene.lights.size());
//
//    descriptorSetLayouts_.resize(DESCRIPTOR_SET_SIZE);
//
//    // set 0:
//    //  binding 0  ->  Acceleration structure
//    //  binding 1  ->  Output image
//    //  binding 2  ->  Scene data
//    //  binding 3  ->  Camera data
//    {
//        VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding;
//        accelerationStructureLayoutBinding.binding = DESCRIPTOR_BINDING_ACCELERATION_STRUCTURE;
//        accelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
//        accelerationStructureLayoutBinding.descriptorCount = 1;
//        accelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
//
//        VkDescriptorSetLayoutBinding outputImageLayoutBinding;
//        outputImageLayoutBinding.binding = DESCRIPTOR_BINDING_OUTPUT_IMAGE;
//        outputImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
//        outputImageLayoutBinding.descriptorCount = 1;
//        outputImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
//
//        VkDescriptorSetLayoutBinding sceneDataLayoutBinding;
//        sceneDataLayoutBinding.binding = DESCRIPTOR_BINDING_SCENE_DATA;
//        sceneDataLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//        sceneDataLayoutBinding.descriptorCount = 1;
//        sceneDataLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
//
//        VkDescriptorSetLayoutBinding cameraDataLayoutBinding;
//        cameraDataLayoutBinding.binding = DESCRIPTOR_BINDING_CAMERA_DATA;
//        cameraDataLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//        cameraDataLayoutBinding.descriptorCount = 1;
//        cameraDataLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
//
//        std::vector<VkDescriptorSetLayoutBinding> bindings({
//                accelerationStructureLayoutBinding,
//                outputImageLayoutBinding,
//                sceneDataLayoutBinding,
//                cameraDataLayoutBinding
//            });
//
//        VkDescriptorSetLayoutCreateInfo descriptorSet0LayoutCreateInfo = {};
//        descriptorSet0LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//        descriptorSet0LayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
//        descriptorSet0LayoutCreateInfo.pBindings = bindings.data();
//
//        VK_CHECK("vkCreateDescriptorSetLayout", vkCreateDescriptorSetLayout(rayTracerVulkanInstance_.device, &descriptorSet0LayoutCreateInfo, nullptr, &descriptorSetLayouts_[0]))
//    }
//
//    // set 1
//    // binding 0 -> attributes
//    {
//        const VkDescriptorBindingFlags set1Flag = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
//
//        VkDescriptorSetLayoutBindingFlagsCreateInfo set1BindingFlags;
//        set1BindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
//        set1BindingFlags.pNext = nullptr;
//        set1BindingFlags.pBindingFlags = &set1Flag;
//        set1BindingFlags.bindingCount = 1;
//
//        VkDescriptorSetLayoutBinding verticesLayoutBinding;
//        verticesLayoutBinding.binding = DESCRIPTOR_SET_VERTEX_ATTRIBUTES;
//        verticesLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
//        verticesLayoutBinding.descriptorCount = static_cast<uint32_t>(sharedMeshAttributesPool_.pool_size());
//        verticesLayoutBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
//
//        VkDescriptorSetLayoutCreateInfo descriptorSet1LayoutCreateInfo = {};
//        descriptorSet1LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//        descriptorSet1LayoutCreateInfo.bindingCount = 1;
//        descriptorSet1LayoutCreateInfo.pBindings = &verticesLayoutBinding;
//        descriptorSet1LayoutCreateInfo.pNext = &set1BindingFlags;
//
//        VK_CHECK("vkCreateDescriptorSetLayout", vkCreateDescriptorSetLayout(rayTracerVulkanInstance_.device, &descriptorSet1LayoutCreateInfo, nullptr, &descriptorSetLayouts_[1]))
//    }
//
//    // set 2
//    // binding 0 -> faces
//    {
//        const VkDescriptorBindingFlags set2Flag = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
//
//        VkDescriptorSetLayoutBindingFlagsCreateInfo set2BindingFlags;
//        set2BindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
//        set2BindingFlags.pNext = nullptr;
//        set2BindingFlags.pBindingFlags = &set2Flag;
//        set2BindingFlags.bindingCount = 1;
//
//        VkDescriptorSetLayoutBinding indicesLayoutBinding;
//        indicesLayoutBinding.binding = DESCRIPTOR_SET_FACE_DATA;
//        indicesLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
//        indicesLayoutBinding.descriptorCount = static_cast<uint32_t>(sharedMeshFacesPool_.pool_size());;
//        indicesLayoutBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
//
//        VkDescriptorSetLayoutCreateInfo descriptorSet2LayoutCreateInfo = {};
//        descriptorSet2LayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//        descriptorSet2LayoutCreateInfo.bindingCount = 1;
//        descriptorSet2LayoutCreateInfo.pBindings = &indicesLayoutBinding;
//        descriptorSet2LayoutCreateInfo.pNext = &set2BindingFlags;
//
//        VK_CHECK("vkCreateDescriptorSetLayout", vkCreateDescriptorSetLayout(rayTracerVulkanInstance_.device, &descriptorSet2LayoutCreateInfo, nullptr, &descriptorSetLayouts_[2]))
//    }
//}
//
//void PixelsForGlory::RayTracerAPI_Vulkan::CreatePipeline()
//{
//    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { };
//    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//    pipelineLayoutCreateInfo.setLayoutCount = DESCRIPTOR_SET_SIZE;
//    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts_.data();
//
//    VK_CHECK("vkCreatePipelineLayout", vkCreatePipelineLayout(rayTracerVulkanInstance_.device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout_));
//
//    VulkanShader rayGenShader(rayTracerVulkanInstance_.device);
//    VulkanShader rayChitShader(rayTracerVulkanInstance_.device);
//    VulkanShader rayMissShader(rayTracerVulkanInstance_.device);
//    VulkanShader shadowChit(rayTracerVulkanInstance_.device);
//    VulkanShader shadowMiss(rayTracerVulkanInstance_.device);
//
//    rayGenShader.LoadFromFile((shadersFolder_ + "ray_gen.bin").c_str());
//    rayChitShader.LoadFromFile((shadersFolder_ + "ray_chit.bin").c_str());
//    rayMissShader.LoadFromFile((shadersFolder_ + "ray_miss.bin").c_str());
//    shadowChit.LoadFromFile((shadersFolder_ + "shadow_ray_chit.bin").c_str());
//    shadowMiss.LoadFromFile((shadersFolder_ + "shadow_ray_miss.bin").c_str());
//
//    shaderBindingTable_.Initialize(2, 2, rayTracerVulkanInstance_.rayTracingProperties.shaderGroupHandleSize, rayTracerVulkanInstance_.rayTracingProperties.shaderGroupBaseAlignment);
//
//    // Ray generation stage
//    shaderBindingTable_.SetRaygenStage(rayGenShader.GetShaderStage(VK_SHADER_STAGE_RAYGEN_BIT_KHR));
//
//    // Hit stages
//    shaderBindingTable_.AddStageToHitGroup({ rayChitShader.GetShaderStage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) }, PRIMARY_HIT_SHADERS_INDEX);
//    shaderBindingTable_.AddStageToHitGroup({ shadowChit.GetShaderStage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) }, SHADOW_HIT_SHADERS_INDEX);
//
//    // Define miss stages for both primary and shadow misses
//    shaderBindingTable_.AddStageToMissGroup(rayMissShader.GetShaderStage(VK_SHADER_STAGE_MISS_BIT_KHR), PRIMARY_MISS_SHADERS_INDEX);
//    shaderBindingTable_.AddStageToMissGroup(shadowMiss.GetShaderStage(VK_SHADER_STAGE_MISS_BIT_KHR), SHADOW_MISS_SHADERS_INDEX);
//
//    // Create the pipeline for ray tracing based on shader binding table
//    VkRayTracingPipelineCreateInfoKHR rayPipelineInfo = {};
//    rayPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
//    rayPipelineInfo.stageCount = shaderBindingTable_.GetNumStages();
//    rayPipelineInfo.pStages = shaderBindingTable_.GetStages();
//    rayPipelineInfo.groupCount = shaderBindingTable_.GetNumGroups();
//    rayPipelineInfo.pGroups = shaderBindingTable_.GetGroups();
//    rayPipelineInfo.maxPipelineRayRecursionDepth = 1;
//    rayPipelineInfo.layout = pipelineLayout_;
//
//    VK_CHECK("vkCreateRayTracingPipelinesKHR", vkCreateRayTracingPipelinesKHR(rayTracerVulkanInstance_.device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayPipelineInfo, nullptr, &pipeline_))
//
//    shaderBindingTable_.CreateSBT(rayTracerVulkanInstance_.device, rayTracerVulkanInstance_.physicalDeviceMemoryProperties, pipeline_);
//
//    Debug::Log("Successfully created pipeline");
//}
//
//void PixelsForGlory::RayTracerAPI_Vulkan::CreateDescriptorPool()
//{
//    auto meshCount = static_cast<uint32_t>(sharedMeshesPool_.pool_size());
//
//    // Descriptors are not generate directly, but from a pool.  Create that pool here
//    std::vector<VkDescriptorPoolSize> poolSizes({
//        { VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },       // Top level acceleration structure
//        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },                    // Output image
//        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},                    // Scene data
//        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },                   // Camera data
//        //{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, numMaterials },      // Material data
//        //{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, numLights },         // Lighting data
//        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, meshCount * 2 }        // vertex attribs for each mesh + faces buffer for each mesh
//        //{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, numMaterials } // textures for each material
//        });
//
//    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
//    descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
//    descriptorPoolCreateInfo.pNext = nullptr;
//    descriptorPoolCreateInfo.flags = 0;
//    descriptorPoolCreateInfo.maxSets = DESCRIPTOR_SET_SIZE;
//    descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
//    descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
//
//    VK_CHECK("vkCreateDescriptorPool", vkCreateDescriptorPool(rayTracerVulkanInstance_.device, &descriptorPoolCreateInfo, nullptr, &descriptorPool_))
//
//    Debug::Log("Successfully created descriptor pool");
//}
//
//void PixelsForGlory::RayTracerAPI_Vulkan::BuildDescriptorBufferInfos()
//{
//    sceneBufferInfo_.buffer = sceneData_.GetBuffer();
//    sceneBufferInfo_.offset = 0;
//    sceneBufferInfo_.range = sceneData_.GetSize();
//
//    cameraBufferInfo_.buffer = cameraData_.GetBuffer();
//    cameraBufferInfo_.offset = 0;
//    cameraBufferInfo_.range = cameraData_.GetSize();
//
//    sharedMeshAttributesBufferInfos_.clear();
//    sharedMeshAttributesBufferInfos_.resize(sharedMeshAttributesPool_.pool_size());
//    for (int i = 0; i < sharedMeshAttributesPool_.pool_size(); ++i)
//    {
//        VkDescriptorBufferInfo& bufferInfo = sharedMeshAttributesBufferInfos_[i];
//        const VulkanBuffer& buffer = sharedMeshAttributesPool_[i];
//
//        bufferInfo.buffer = buffer.GetBuffer();
//        bufferInfo.offset = 0;
//        bufferInfo.range = buffer.GetSize();
//
//        sharedMeshAttributesBufferInfos_.push_back(bufferInfo);
//    }
//
//    sharedMeshFacesBufferInfos_.clear();
//    sharedMeshFacesBufferInfos_.resize(sharedMeshFacesPool_.pool_size());
//    for (int i = 0; i < sharedMeshFacesPool_.pool_size(); ++i)
//    {
//        VkDescriptorBufferInfo& bufferInfo = sharedMeshFacesBufferInfos_[i];
//        const VulkanBuffer& buffer = sharedMeshFacesPool_[i];
//
//        bufferInfo.buffer = buffer.GetBuffer();
//        bufferInfo.offset = 0;
//        bufferInfo.range = buffer.GetSize();
//
//        sharedMeshFacesBufferInfos_.push_back(bufferInfo);
//    }
//}
//
//void PixelsForGlory::RayTracerAPI_Vulkan::UpdateDescriptorSets()
//{
//    // Update the descriptor sets with the actual data to store in memory.
//
//    // Now use the pool to upload data for each descriptor
//    descriptorSets_.resize(DESCRIPTOR_SET_SIZE);
//    std::vector<uint32_t> variableDescriptorCounts({
//        1,                                                              // Set 0
//        static_cast<uint32_t>(sharedMeshAttributesPool_.pool_size()),   // Set 1
//        static_cast<uint32_t>(sharedMeshFacesPool_.pool_size())         // Set 2
//        });
//
//    VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountInfo;
//    variableDescriptorCountInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
//    variableDescriptorCountInfo.pNext = nullptr;
//    variableDescriptorCountInfo.descriptorSetCount = DESCRIPTOR_SET_SIZE;
//    variableDescriptorCountInfo.pDescriptorCounts = variableDescriptorCounts.data(); // actual number of descriptors
//
//    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
//    descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//    descriptorSetAllocateInfo.pNext = &variableDescriptorCountInfo;
//    descriptorSetAllocateInfo.descriptorPool = descriptorPool_;
//    descriptorSetAllocateInfo.descriptorSetCount = DESCRIPTOR_SET_SIZE;
//    descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts_.data();
//
//    VK_CHECK("vkAllocateDescriptorSets", vkAllocateDescriptorSets(rayTracerVulkanInstance_.device, &descriptorSetAllocateInfo, descriptorSets_.data()))
//
//    std::vector<VkWriteDescriptorSet> descriptorWrites;
//    // Acceleration Structure
//    {
//        VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo;
//        descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
//        descriptorAccelerationStructureInfo.pNext = nullptr;
//        descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
//        descriptorAccelerationStructureInfo.pAccelerationStructures = &tlas_.accelerationStructure;
//
//        VkWriteDescriptorSet accelerationStructureWrite;
//        accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo; // Notice that pNext is assigned here!
//        accelerationStructureWrite.dstSet = descriptorSets_[DESCRIPTOR_SET_ACCELERATION_STRUCTURE];
//        accelerationStructureWrite.dstBinding = DESCRIPTOR_BINDING_ACCELERATION_STRUCTURE;
//        accelerationStructureWrite.dstArrayElement = 0;
//        accelerationStructureWrite.descriptorCount = 1;
//        accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
//        accelerationStructureWrite.pImageInfo = nullptr;
//        accelerationStructureWrite.pBufferInfo = nullptr;
//        accelerationStructureWrite.pTexelBufferView = nullptr;
//
//        descriptorWrites.push_back(accelerationStructureWrite);
//    }
//
//    // Output image
//    {
//        VkDescriptorImageInfo descriptorOutputImageInfo;
//        descriptorOutputImageInfo.sampler = VK_NULL_HANDLE;
//        descriptorOutputImageInfo.imageView = offscreenImage_->GetImageView();
//        descriptorOutputImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
//
//        VkWriteDescriptorSet resultImageWrite;
//        resultImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        resultImageWrite.pNext = nullptr;
//        resultImageWrite.dstSet = descriptorSets_[DESCRIPTOR_SET_OUTPUT_IMAGE];
//        resultImageWrite.dstBinding = DESCRIPTOR_BINDING_OUTPUT_IMAGE;
//        resultImageWrite.dstArrayElement = 0;
//        resultImageWrite.descriptorCount = 1;
//        resultImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
//        resultImageWrite.pImageInfo = &descriptorOutputImageInfo;
//        resultImageWrite.pBufferInfo = nullptr;
//        resultImageWrite.pTexelBufferView = nullptr;
//
//        descriptorWrites.push_back(resultImageWrite);
//    }
//
//    // Scene data
//    {
//        VkWriteDescriptorSet sceneBufferWrite;
//        sceneBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        sceneBufferWrite.pNext = nullptr;
//        sceneBufferWrite.dstSet = descriptorSets_[DESCRIPTOR_SET_SCENE_DATA];
//        sceneBufferWrite.dstBinding = DESCRIPTOR_BINDING_SCENE_DATA;
//        sceneBufferWrite.dstArrayElement = 0;
//        sceneBufferWrite.descriptorCount = 1;
//        sceneBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//        sceneBufferWrite.pImageInfo = nullptr;
//        sceneBufferWrite.pBufferInfo = &sceneBufferInfo_;
//        sceneBufferWrite.pTexelBufferView = nullptr;
//
//        descriptorWrites.push_back(sceneBufferWrite);
//    }
//
//    // Camera data
//    {
//        VkWriteDescriptorSet camdataBufferWrite;
//        camdataBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        camdataBufferWrite.pNext = nullptr;
//        camdataBufferWrite.dstSet = descriptorSets_[DESCRIPTOR_SET_CAMERA_DATA];
//        camdataBufferWrite.dstBinding = DESCRIPTOR_BINDING_CAMERA_DATA;
//        camdataBufferWrite.dstArrayElement = 0;
//        camdataBufferWrite.descriptorCount = 1;
//        camdataBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//        camdataBufferWrite.pImageInfo = nullptr;
//        camdataBufferWrite.pBufferInfo = &cameraBufferInfo_;
//        camdataBufferWrite.pTexelBufferView = nullptr;
//
//        descriptorWrites.push_back(camdataBufferWrite);
//    }
//   
//    // Vertex attributes
//    {
//        VkWriteDescriptorSet attribsBufferWrite;
//        attribsBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        attribsBufferWrite.pNext = nullptr;
//        attribsBufferWrite.dstSet = descriptorSets_[DESCRIPTOR_SET_VERTEX_ATTRIBUTES];
//        attribsBufferWrite.dstBinding = DESCRIPTOR_BINDING_VERTEX_ATTRIBUTES;
//        attribsBufferWrite.dstArrayElement = 0;
//        attribsBufferWrite.descriptorCount = static_cast<uint32_t>(sharedMeshAttributesPool_.pool_size());
//        attribsBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
//        attribsBufferWrite.pImageInfo = nullptr;
//        attribsBufferWrite.pBufferInfo = sharedMeshAttributesBufferInfos_.data();
//        attribsBufferWrite.pTexelBufferView = nullptr;
//
//        descriptorWrites.push_back(attribsBufferWrite);
//    }
//        
//    // Faces
//    {
//        VkWriteDescriptorSet facesBufferWrite;
//        facesBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        facesBufferWrite.pNext = nullptr;
//        facesBufferWrite.dstSet = descriptorSets_[DESCRIPTOR_SET_FACE_DATA];
//        facesBufferWrite.dstBinding = DESCRIPTOR_BINDING_FACE_DATA;
//        facesBufferWrite.dstArrayElement = 0;
//        facesBufferWrite.descriptorCount = static_cast<uint32_t>(sharedMeshFacesPool_.pool_size());;
//        facesBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
//        facesBufferWrite.pImageInfo = nullptr;
//        facesBufferWrite.pBufferInfo = sharedMeshFacesBufferInfos_.data();
//        facesBufferWrite.pTexelBufferView = nullptr;
//
//        descriptorWrites.push_back(facesBufferWrite);
//    }
//
//    vkUpdateDescriptorSets(rayTracerVulkanInstance_.device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, VK_NULL_HANDLE);
//
//    Debug::Log("Successfully updated descriptor sets");
//}
//
//void PixelsForGlory::RayTracerAPI_Vulkan::BuildCommandBuffer()
//{
//    //if (width != storage_image.width || height != storage_image.height)
//    //{
//    //    // If the view port size has changed, we need to recreate the storage image
//    //    vkDestroyImageView(get_device().get_handle(), storage_image.view, nullptr);
//    //    vkDestroyImage(get_device().get_handle(), storage_image.image, nullptr);
//    //    vkFreeMemory(get_device().get_handle(), storage_image.memory, nullptr);
//    //    create_storage_image();
//
//    //    // The descriptor also needs to be updated to reference the new image
//    //    VkDescriptorImageInfo image_descriptor{};
//    //    image_descriptor.imageView = storage_image.view;
//    //    image_descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
//    //    VkWriteDescriptorSet result_image_write = vkb::initializers::write_descriptor_set(descriptor_set, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, &image_descriptor);
//    //    vkUpdateDescriptorSets(get_device().get_handle(), 1, &result_image_write, 0, VK_NULL_HANDLE);
//    //    build_command_buffers();
//    //}
//
//    commandBuffer_ = rayTracerVulkanInstance_.CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
//
//    vkCmdBindPipeline(
//        commandBuffer_,
//        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
//        pipeline_);
//
//    vkCmdBindDescriptorSets(
//        commandBuffer_,
//        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
//        pipelineLayout_, 0,
//        static_cast<uint32_t>(descriptorSets_.size()), descriptorSets_.data(),
//        0, 0);
//        
//    VkStridedDeviceAddressRegionKHR raygenShaderEntry = {};
//    raygenShaderEntry.deviceAddress = GetBufferDeviceAddress(shaderBindingTable_.GetBuffer()).deviceAddress + shaderBindingTable_.GetRaygenOffset();
//    raygenShaderEntry.stride = shaderBindingTable_.GetGroupsStride();
//    raygenShaderEntry.size = shaderBindingTable_.GetRaygenSize();
//
//    VkStridedDeviceAddressRegionKHR missShaderEntry{};
//    missShaderEntry.deviceAddress = GetBufferDeviceAddress(shaderBindingTable_.GetBuffer()).deviceAddress + shaderBindingTable_.GetMissGroupsOffset();
//    missShaderEntry.stride = shaderBindingTable_.GetGroupsStride();
//    missShaderEntry.size = shaderBindingTable_.GetMissGroupsSize();
//
//    VkStridedDeviceAddressRegionKHR hitShaderEntry{};
//    hitShaderEntry.deviceAddress = GetBufferDeviceAddress(shaderBindingTable_.GetBuffer()).deviceAddress + shaderBindingTable_.GetHitGroupsOffset();
//    hitShaderEntry.stride = shaderBindingTable_.GetGroupsStride();
//    hitShaderEntry.size = shaderBindingTable_.GetHitGroupsSize();
//
//    VkStridedDeviceAddressRegionKHR callableShaderEntry{};
// 
//    // Dispatch the ray tracing commands
//    vkCmdBindPipeline(commandBuffer_, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline_);
//    vkCmdBindDescriptorSets(commandBuffer_, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineLayout_, 0, static_cast<uint32_t>(descriptorSets_.size()), descriptorSets_.data(), 0, 0);
//
//    vkCmdTraceRaysKHR(
//        commandBuffer_,
//        &raygenShaderEntry,
//        &missShaderEntry,
//        &hitShaderEntry,
//        &callableShaderEntry,
//        width_,
//        height_,
//        1);
//   
//    rayTracerVulkanInstance_.EndCommandBuffer(commandBuffer_);
//    //}
//}
//

//
//#endif // #if SUPPORT_VULKAN
//
