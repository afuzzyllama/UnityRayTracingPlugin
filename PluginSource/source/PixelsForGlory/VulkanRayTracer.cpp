
//void PixelsForGlory::VulkanRayTracer::LoadScene() {
////    tinyobj::attrib_t attrib;
////    std::vector<tinyobj::shape_t> shapes;
////    std::vector<tinyobj::material_t> materials;
////    std::string warn, error;
////
////    std::string fileName = _scenesFolder + _sceneFile;
////    std::string baseDir = fileName;
////    const size_t slash = baseDir.find_last_of('/');
////    if (slash != std::string::npos) {
////        baseDir.erase(slash);
////    }
////
////    // Load scene obj
////    const bool result = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &error, fileName.c_str(), baseDir.c_str(), true);
////    if (result) {
////        // Successfully loaded object.  Process.
////        _scene.meshes.resize(shapes.size());
////        _scene.materials.resize(materials.size());
////
////        for (size_t meshIdx = 0; meshIdx < shapes.size(); ++meshIdx) {
////            RTMesh& mesh = _scene.meshes[meshIdx];
////            const tinyobj::shape_t& shape = shapes[meshIdx];
////
////            const size_t numFaces = shape.mesh.num_face_vertices.size();
////            const size_t numVertices = numFaces * 3;
////
////            mesh.numVertices = static_cast<uint32_t>(numVertices);
////            mesh.numFaces = static_cast<uint32_t>(numFaces);
////
////            const size_t positionsBufferSize = numVertices * sizeof(vec3);
////            const size_t indicesBufferSize = numFaces * 3 * sizeof(uint32_t);
////            const size_t facesBufferSize = numFaces * 4 * sizeof(uint32_t);
////            const size_t attribsBufferSize = numVertices * sizeof(VertexAttribute);
////            const size_t matIDsBufferSize = numFaces * sizeof(uint32_t);
////
////            // Create the buffers for Vulkan
////            VkResult error = mesh.positions.Create(positionsBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
////            CHECK_VK_ERROR(error, "mesh.positions.Create");
////
////            error = mesh.indices.Create(indicesBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
////            CHECK_VK_ERROR(error, "mesh.indices.Create");
////
////            error = mesh.faces.Create(facesBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
////            CHECK_VK_ERROR(error, "mesh.faces.Create");
////
////            error = mesh.attribs.Create(attribsBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
////            CHECK_VK_ERROR(error, "mesh.attribs.Create");
////
////            error = mesh.matIDs.Create(matIDsBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
////            CHECK_VK_ERROR(error, "mesh.matIDs.Create");
////
////            // Get application addresses for the Vulkan buffers
////            vec3* positions = reinterpret_cast<vec3*>(mesh.positions.Map());
////            VertexAttribute* attribs = reinterpret_cast<VertexAttribute*>(mesh.attribs.Map());
////            uint32_t* indices = reinterpret_cast<uint32_t*>(mesh.indices.Map());
////            uint32_t* faces = reinterpret_cast<uint32_t*>(mesh.faces.Map());
////            uint32_t* matIDs = reinterpret_cast<uint32_t*>(mesh.matIDs.Map());
////
////            // Build the mesh
////            size_t vIdx = 0;
////            for (size_t f = 0; f < numFaces; ++f) {
////                assert(shape.mesh.num_face_vertices[f] == 3);
////                for (size_t j = 0; j < shape.mesh.num_face_vertices[f]; ++j) {
////                    const tinyobj::index_t& i = shape.mesh.indices[vIdx];
////
////                    vec3& pos = positions[vIdx];
////                    vec4& normal = attribs[vIdx].normal;
////                    vec2& uv = attribs[vIdx].uv;
////
////                    pos.x = attrib.vertices[3 * i.vertex_index + 0];
////                    pos.y = attrib.vertices[3 * i.vertex_index + 1];
////                    pos.z = attrib.vertices[3 * i.vertex_index + 2];
////
////                    if (i.normal_index != -1)
////                    {
////                        normal.x = attrib.normals[3 * i.normal_index + 0];
////                        normal.y = attrib.normals[3 * i.normal_index + 1];
////                        normal.z = attrib.normals[3 * i.normal_index + 2];
////                    }
////
////                    if (i.texcoord_index != -1)
////                    {
////                        // I have no idea why uvs from Blender are importing this way, but this works?
////                        uv.x = attrib.texcoords[2 * i.texcoord_index + 0];
////                        uv.y = -attrib.texcoords[2 * i.texcoord_index + 1];
////                    }
////                    else
////                    {
////                        uv.x = 0.0f;
////                        uv.y = 0.0f;
////                    }
////                    ++vIdx;
////                }
////
////                const uint32_t a = static_cast<uint32_t>(3 * f + 0);
////                const uint32_t b = static_cast<uint32_t>(3 * f + 1);
////                const uint32_t c = static_cast<uint32_t>(3 * f + 2);
////                indices[a] = a;
////                indices[b] = b;
////                indices[c] = c;
////                faces[4 * f + 0] = a;
////                faces[4 * f + 1] = b;
////                faces[4 * f + 2] = c;
////
////                matIDs[f] = static_cast<uint32_t>(shape.mesh.material_ids[f]);
////            }
////
////#pragma warning( push )
////#pragma warning( disable : 4267 )
////            // Solve tangents for normal mapping
////            solveTangents(numVertices, positions, indices, attribs);
////#pragma warning( pop )
////
////            // We're done with the application addresses, unmap them
////            mesh.indices.Unmap();
////            mesh.faces.Unmap();
////            mesh.attribs.Unmap();
////            mesh.positions.Unmap();
////        }
////
////        // Start processing materials
////        assert(alignof(MaterialParam) == 16);
////        const size_t materialsAttribsBufferSize = materials.size() * sizeof(MaterialParam);
////        _scene.materialAttribs.Create(materialsAttribsBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
////        MaterialParam* mateiralAttribs = reinterpret_cast<MaterialParam*>(_scene.materialAttribs.Map());
////
////        std::map<std::string, uint32_t> textureMap;
////        for (size_t i = 0; i < materials.size(); ++i) {
////            const tinyobj::material_t& srcMat = materials[i];
////            MaterialParam& dstMat = mateiralAttribs[i];
////
////            // In the first pass we gather material information
////            dstMat.emission = vec4(srcMat.emission[0], srcMat.emission[1], srcMat.emission[2], 0.0f);
////            dstMat.albedo = vec4(srcMat.diffuse[0], srcMat.diffuse[1], srcMat.diffuse[2], 0.0f);
////            dstMat.transmittance = vec4(srcMat.transmittance[0], srcMat.transmittance[1], srcMat.transmittance[2], 0.0f);
////            dstMat.metallic = srcMat.metallic;
////            dstMat.roughness = srcMat.roughness;
////            dstMat.ior = srcMat.ior;
////            dstMat.albedoIndex = -1;
////            dstMat.normalIndex = -1;
////            dstMat.roughIndex = -1;
////            dstMat.reflIndex = -1;
////            dstMat.aoIndex = -1;
////
////            // In the first pass we do not load the textures.
////            // First we need to figure out what textures we need to load and map them back to the material later
////            // Store Vulkan format to load as the value of the map
////            if (!srcMat.diffuse_texname.empty())
////            {
////                auto it = textureMap.find(srcMat.diffuse_texname);
////                if (it == textureMap.end())
////                {
////                    textureMap[srcMat.diffuse_texname] = VK_FORMAT_R8G8B8A8_SRGB;
////                }
////            }
////
////            if (!srcMat.normal_texname.empty())
////            {
////                auto it = textureMap.find(srcMat.normal_texname);
////                if (it == textureMap.end())
////                {
////                    textureMap[srcMat.normal_texname] = VK_FORMAT_R8G8B8A8_UNORM;
////                }
////            }
////
////            if (!srcMat.roughness_texname.empty())
////            {
////                auto it = textureMap.find(srcMat.roughness_texname);
////                if (it == textureMap.end())
////                {
////                    textureMap[srcMat.roughness_texname] = VK_FORMAT_R8G8B8A8_SRGB;
////                }
////            }
////
////            if (!srcMat.ao_texname.empty())
////            {
////                auto it = textureMap.find(srcMat.ao_texname);
////                if (it == textureMap.end())
////                {
////                    textureMap[srcMat.ao_texname] = VK_FORMAT_R8G8B8A8_SRGB;
////                }
////            }
////
////            if (!srcMat.reflection_texname.empty())
////            {
////                auto it = textureMap.find(srcMat.reflection_texname);
////                if (it == textureMap.end())
////                {
////                    textureMap[srcMat.reflection_texname] = VK_FORMAT_R8G8B8A8_SRGB;
////                }
////            }
////        }
////
////        // Upload any materials if necessary 
////        VkImageSubresourceRange subresourceRange = {};
////        subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
////        subresourceRange.baseMipLevel = 0;
////        subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
////        subresourceRange.baseArrayLayer = 0;
////        subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
////
////        // At this point we know what textures we need to load, so load them here
////        _scene.textures.resize(textureMap.size());
////        int i = 0;
////        for (auto const& [key, val] : textureMap)
////        {
////            VulkanHelpers::Image& dstTex = _scene.textures[i];
////
////            std::string fullTexturePath = baseDir + "/" + key;
////
////            // Load texture based on val which is a Vulkan format
////            if (dstTex.Load(fullTexturePath.c_str(), static_cast<VkFormat>(val))) {
////                dstTex.CreateImageView(VK_IMAGE_VIEW_TYPE_2D, dstTex.GetFormat(), subresourceRange);
////                dstTex.CreateSampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
////
////                // Format is no longer needed, so replace it with the index that represents the texture in the buffer
////                textureMap[key] = i;
////                ++i;
////            }
////        }
////
////        for (size_t i = 0; i < materials.size(); ++i) {
////            const tinyobj::material_t& srcMat = materials[i];
////            MaterialParam& dstMat = mateiralAttribs[i];
////
////            // In this second pass, we know the indices of the loaded textures to map them back to their materials
////            if (!srcMat.diffuse_texname.empty())
////            {
////                auto it = textureMap.find(srcMat.diffuse_texname);
////                if (it != textureMap.end())
////                {
////                    dstMat.albedoIndex = it->second;
////                }
////            }
////            if (!srcMat.normal_texname.empty())
////            {
////                auto it = textureMap.find(srcMat.normal_texname);
////                if (it != textureMap.end())
////                {
////                    dstMat.normalIndex = it->second;
////                }
////            }
////
////            if (!srcMat.roughness_texname.empty())
////            {
////                auto it = textureMap.find(srcMat.roughness_texname);
////                if (it != textureMap.end())
////                {
////                    dstMat.roughIndex = it->second;
////                }
////            }
////
////            if (!srcMat.ao_texname.empty())
////            {
////                auto it = textureMap.find(srcMat.ao_texname);
////                if (it != textureMap.end())
////                {
////                    dstMat.aoIndex = it->second;
////                }
////            }
////
////            if (!srcMat.reflection_texname.empty())
////            {
////                auto it = textureMap.find(srcMat.reflection_texname);
////                if (it != textureMap.end())
////                {
////                    dstMat.reflIndex = it->second;
////                }
////            }
////        }
////
////        _scene.materialAttribs.Unmap();
////    }
////
////    const size_t numMeshes = _scene.meshes.size();
////    const size_t numMaterials = _scene.materials.size();
////    const size_t numTextures = _scene.textures.size();
////    assert(numMaterials < MAX_MATERIALS);
////
////    // Create lighting from config
////    auto lights = _config["lights"].get<std::vector<LightingParam>>(); // Get lighting from config
////    const size_t numLights = lights.size();
////    assert(numLights < MAX_LIGHTS);
////
////    assert(alignof(LightingParam) == 16);
////    const size_t lightingAttribsBufferSize = lights.size() * sizeof(LightingParam);
////    _scene.lightingAttribs.Create(lightingAttribsBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
////    LightingParam* lightingAttribs = reinterpret_cast<LightingParam*>(_scene.lightingAttribs.Map());
////
////    for (size_t i = 0; i < lights.size(); ++i) {
////        const LightingParam& light = lights[i];
////        _scene.lights.push_back(light);
////
////        lightingAttribs[i].type = light.type;
////        lightingAttribs[i].position = light.position;
////        lightingAttribs[i].color = light.color;
////
////        if (light.type == LIGHT_TYPE_POINT) {
////            lightingAttribs[i].intensity = light.intensity;
////            lightingAttribs[i].radius = light.radius;
////        }
////    }
////
////    _scene.lightingAttribs.Unmap();
////
////    // Create scene attributes
////    assert(alignof(SceneParam) == 16);
////    const size_t sceneAttribsBufferSize = sizeof(SceneParam);
////    _scene.sceneAttribs.Create(sceneAttribsBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
////    SceneParam* sceneAttribs = reinterpret_cast<SceneParam*>(_scene.sceneAttribs.Map());
////
////    auto sceneParams = _config["scene"].get<SceneParam>();
////
////    sceneAttribs->ambient = sceneParams.ambient;
////    sceneAttribs->depthThreshold = sceneParams.depthThreshold;
////#pragma warning( push )
////#pragma warning( disable : 4267 )
////    sceneAttribs->lightCount = lights.size();
////    sceneAttribs->materialCount = numMaterials;
////#pragma warning( pop )
////
////    _scene.sceneAttribs.Unmap();
////
////    // Prepare shader resources buffers
////    _scene.sceneDataBufferInfo.buffer = _scene.sceneAttribs.GetBuffer();
////    _scene.sceneDataBufferInfo.offset = 0;
////    _scene.sceneDataBufferInfo.range = _scene.sceneAttribs.GetSize();
////
////    _scene.lightingDataBufferInfos.resize(numLights);
////    for (size_t i = 0; i < numLights; ++i) {
////        VkDescriptorBufferInfo& lightDataInfo = _scene.lightingDataBufferInfos[i];
////
////        lightDataInfo.buffer = _scene.lightingAttribs.GetBuffer();
////        lightDataInfo.offset = 0;
////        lightDataInfo.range = _scene.lightingAttribs.GetSize();
////    }
////
////    _scene.matIDsBufferInfos.resize(numMeshes);
////    _scene.attribsBufferInfos.resize(numMeshes);
////    _scene.facesBufferInfos.resize(numMeshes);
////    for (size_t i = 0; i < numMeshes; ++i) {
////        const RTMesh& mesh = _scene.meshes[i];
////        VkDescriptorBufferInfo& matIDsInfo = _scene.matIDsBufferInfos[i];
////        VkDescriptorBufferInfo& attribsInfo = _scene.attribsBufferInfos[i];
////        VkDescriptorBufferInfo& facesInfo = _scene.facesBufferInfos[i];
////
////        matIDsInfo.buffer = mesh.matIDs.GetBuffer();
////        matIDsInfo.offset = 0;
////        matIDsInfo.range = mesh.matIDs.GetSize();
////
////        attribsInfo.buffer = mesh.attribs.GetBuffer();
////        attribsInfo.offset = 0;
////        attribsInfo.range = mesh.attribs.GetSize();
////
////        facesInfo.buffer = mesh.faces.GetBuffer();
////        facesInfo.offset = 0;
////        facesInfo.range = mesh.faces.GetSize();
////    }
////
////    _scene.materialDataBufferInfos.resize(numMaterials);
////    for (size_t i = 0; i < numMaterials; ++i) {
////        VkDescriptorBufferInfo& materialDataInfo = _scene.materialDataBufferInfos[i];
////
////        materialDataInfo.buffer = _scene.materialAttribs.GetBuffer();
////        materialDataInfo.offset = 0;
////        materialDataInfo.range = _scene.materialAttribs.GetSize();
////    }
////
////    _scene.texturesInfos.resize(numTextures);
////    for (size_t i = 0; i < numTextures; ++i) {
////        const VulkanHelpers::Image& tex = _scene.textures[i];
////        VkDescriptorImageInfo& textureInfo = _scene.texturesInfos[i];
////
////        textureInfo.sampler = tex.GetSampler();
////        textureInfo.imageView = tex.GetImageView();
////        textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
////    }
//}
//
//void PixelsForGlory::VulkanRayTracer::CreateScene()
//{
//    // Define base transform of acceleration structure as the identity matrix
//    const VkTransformMatrixKHR transform = {
//        1.0f, 0.0f, 0.0f, 0.0f,
//        0.0f, 1.0f, 0.0f, 0.0f,
//        0.0f, 0.0f, 1.0f, 0.0f,
//    };
//
//    const size_t numMeshes = 0;// _scene.meshes.size();
//
//    //std::vector<VkAccelerationStructureCreateGeometryTypeInfoKHR> geometryInfos(numMeshes, VkAccelerationStructureCreateGeometryTypeInfoKHR{});
//    //std::vector<VkAccelerationStructureGeometryKHR> geometries(numMeshes, VkAccelerationStructureGeometryKHR{});
//    //std::vector<VkAccelerationStructureInstanceKHR> instances(numMeshes, VkAccelerationStructureInstanceKHR{});
//
//    //// Create necessary data structures to handle each individual mesh for an acceleration structure
//    //for (size_t i = 0; i < numMeshes; ++i) {
//    //    RTMesh& mesh = _scene.meshes[i];
//
//    //    VkAccelerationStructureCreateGeometryTypeInfoKHR& geometryInfo = geometryInfos[i];
//    //    VkAccelerationStructureGeometryKHR& geometry = geometries[i];
//
//    //    geometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;
//    //    geometryInfo.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
//    //    geometryInfo.maxPrimitiveCount = mesh.numFaces;
//    //    geometryInfo.indexType = VK_INDEX_TYPE_UINT32;
//    //    geometryInfo.maxVertexCount = mesh.numVertices;
//    //    geometryInfo.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
//    //    geometryInfo.allowsTransforms = VK_FALSE;
//
//    //    geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
//    //    geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
//    //    geometry.geometryType = geometryInfo.geometryType;
//    //    geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
//    //    geometry.geometry.triangles.vertexData = VulkanHelpers::GetBufferDeviceAddressConst(mesh.positions);
//    //    geometry.geometry.triangles.vertexStride = sizeof(vec3);
//    //    geometry.geometry.triangles.vertexFormat = geometryInfo.vertexFormat;
//    //    geometry.geometry.triangles.indexData = VulkanHelpers::GetBufferDeviceAddressConst(mesh.indices);
//    //    geometry.geometry.triangles.indexType = geometryInfo.indexType;
//
//    //    // Here we create our bottom-level acceleration structure for our mesh
//    //    this->createAS(VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, 1, &geometryInfo, 0, mesh.blas);
//
//    //    VkAccelerationStructureInstanceKHR& instance = instances[i];
//    //    instance.transform = transform;
//    //    instance.instanceCustomIndex = static_cast<uint32_t>(i);
//    //    instance.mask = 0xff;
//    //    instance.instanceShaderBindingTableRecordOffset = 0;
//    //    instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
//    //    instance.accelerationStructureReference = mesh.blas.handle;
//    //}
//
//    //// Create instances for our meshes
//    //VulkanHelpers::Buffer instancesBuffer;
//    //VkResult error = instancesBuffer.Create(instances.size() * sizeof(VkAccelerationStructureInstanceKHR), VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
//    //CHECK_VK_ERROR(error, "instancesBuffer.Create");
//
//    //if (!instancesBuffer.UploadData(instances.data(), instancesBuffer.GetSize())) {
//    //    assert(false && "Failed to upload instances buffer");
//    //}
//
//    //// and here we create out top-level acceleration structure that'll represent our scene
//    //VkAccelerationStructureCreateGeometryTypeInfoKHR tlasGeoInfo = {};
//    //tlasGeoInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_GEOMETRY_TYPE_INFO_KHR;
//    //tlasGeoInfo.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
//    //tlasGeoInfo.maxPrimitiveCount = static_cast<uint32_t>(instances.size());
//    //tlasGeoInfo.allowsTransforms = VK_TRUE;
//
//    //this->createAS(VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, 1, &tlasGeoInfo, 1, _scene.topLevelAS);
//
//    //// now we have to build them
//    //VkAccelerationStructureMemoryRequirementsInfoKHR memoryRequirementsInfo = {};
//    //memoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_KHR;
//    //memoryRequirementsInfo.buildType = VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR;
//
//    //VkDeviceSize maximumBlasSize = 0;
//    //for (const RTMesh& mesh : _scene.meshes) {
//    //    memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR;
//    //    memoryRequirementsInfo.accelerationStructure = mesh.blas.accelerationStructure;
//
//    //    VkMemoryRequirements2 memReqBLAS = {};
//    //    memReqBLAS.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
//    //    vkGetAccelerationStructureMemoryRequirementsKHR(_device, &memoryRequirementsInfo, &memReqBLAS);
//
//    //    maximumBlasSize = Max(maximumBlasSize, memReqBLAS.memoryRequirements.size);
//    //}
//
//    //VkMemoryRequirements2 memReqTLAS = {};
//    //memReqTLAS.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
//    //memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_KHR;
//    //memoryRequirementsInfo.accelerationStructure = _scene.topLevelAS.accelerationStructure;
//    //vkGetAccelerationStructureMemoryRequirementsKHR(_device, &memoryRequirementsInfo, &memReqTLAS);
//
//    //const VkDeviceSize scratchBufferSize = Max(maximumBlasSize, memReqTLAS.memoryRequirements.size);
//
//    //VulkanHelpers::Buffer scratchBuffer;
//    //error = scratchBuffer.Create(scratchBufferSize, VK_BUFFER_USAGE_RAY_TRACING_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//    //CHECK_VK_ERROR(error, "scratchBuffer.Create");
//
//    //VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
//    //commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//    //commandBufferAllocateInfo.commandPool = _commandPool;
//    //commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//    //commandBufferAllocateInfo.commandBufferCount = 1;
//
//    //VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
//    //error = vkAllocateCommandBuffers(_device, &commandBufferAllocateInfo, &commandBuffer);
//    //CHECK_VK_ERROR(error, "vkAllocateCommandBuffers");
//
//    //VkCommandBufferBeginInfo beginInfo = {};
//    //beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//    //beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
//    //vkBeginCommandBuffer(commandBuffer, &beginInfo);
//
//    //VkMemoryBarrier memoryBarrier = {};
//    //memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
//    //memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
//    //memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
//
//    //// build bottom-level ASs
//    //for (size_t i = 0; i < numMeshes; ++i) {
//    //    VkAccelerationStructureGeometryKHR* geometryPtr = &geometries[i];
//
//    //    VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
//    //    buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
//    //    buildInfo.type = _scene.meshes[i].blas.accelerationStructureInfo.type;
//    //    buildInfo.flags = _scene.meshes[i].blas.accelerationStructureInfo.flags;
//    //    buildInfo.update = VK_FALSE;
//    //    buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
//    //    buildInfo.dstAccelerationStructure = _scene.meshes[i].blas.accelerationStructure;
//    //    buildInfo.geometryArrayOfPointers = VK_FALSE;
//    //    buildInfo.geometryCount = _scene.meshes[i].blas.accelerationStructureInfo.maxGeometryCount;
//    //    buildInfo.ppGeometries = &geometryPtr;
//    //    buildInfo.scratchData = VulkanHelpers::GetBufferDeviceAddress(scratchBuffer);
//
//    //    VkAccelerationStructureBuildOffsetInfoKHR offsetInfo;
//    //    offsetInfo.primitiveCount = geometryInfos[i].maxPrimitiveCount;
//    //    offsetInfo.primitiveOffset = 0;
//    //    offsetInfo.firstVertex = 0;
//    //    offsetInfo.transformOffset = 0;
//
//    //    VkAccelerationStructureBuildOffsetInfoKHR* offsets[1] = { &offsetInfo };
//
//    //    vkCmdBuildAccelerationStructureKHR(commandBuffer, 1, &buildInfo, offsets);
//
//    //    vkCmdPipelineBarrier(commandBuffer,
//    //        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
//    //        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
//    //        0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
//    //}
//
//    //// build top-level AS
//    //VkAccelerationStructureGeometryKHR topLevelGeometry = {};
//    //topLevelGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
//    //topLevelGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
//    //topLevelGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
//    //topLevelGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
//    //topLevelGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
//    //topLevelGeometry.geometry.instances.data.deviceAddress = VulkanHelpers::GetBufferDeviceAddress(instancesBuffer).deviceAddress;
//
//    //VkAccelerationStructureGeometryKHR* geometryPtr = &topLevelGeometry;
//
//    //VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
//    //buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
//    //buildInfo.type = _scene.topLevelAS.accelerationStructureInfo.type;
//    //buildInfo.flags = _scene.topLevelAS.accelerationStructureInfo.flags;
//    //buildInfo.update = VK_FALSE;
//    //buildInfo.srcAccelerationStructure = VK_NULL_HANDLE;
//    //buildInfo.dstAccelerationStructure = _scene.topLevelAS.accelerationStructure;
//    //buildInfo.geometryArrayOfPointers = VK_FALSE;
//    //buildInfo.geometryCount = 1;
//    //buildInfo.ppGeometries = &geometryPtr;
//    //buildInfo.scratchData = VulkanHelpers::GetBufferDeviceAddress(scratchBuffer);
//
//    //VkAccelerationStructureBuildOffsetInfoKHR offsetInfo;
//    //offsetInfo.primitiveCount = tlasGeoInfo.maxPrimitiveCount;
//    //offsetInfo.primitiveOffset = 0;
//    //offsetInfo.firstVertex = 0;
//    //offsetInfo.transformOffset = 0;
//
//    //VkAccelerationStructureBuildOffsetInfoKHR* offsets[1] = { &offsetInfo };
//
//    //vkCmdBuildAccelerationStructureKHR(commandBuffer, 1, &buildInfo, offsets);
//
//    //vkCmdPipelineBarrier(commandBuffer,
//    //    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
//    //    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
//    //    0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
//
//    //vkEndCommandBuffer(commandBuffer);
//
//    //VkSubmitInfo submitInfo = {};
//    //submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//    //submitInfo.commandBufferCount = 1;
//    //submitInfo.pCommandBuffers = &commandBuffer;
//
//    //vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
//    //error = vkQueueWaitIdle(_graphicsQueue);
//    //CHECK_VK_ERROR(error, "vkQueueWaitIdle");
//    //vkFreeCommandBuffers(_device, _commandPool, 1, &commandBuffer);
//}
//
//void PixelsForGlory::VulkanRayTracer::CreateDescriptorSetsLayouts()
//{
//    //// Create descriptor sets for the shader.  This setups up how data is bound to GPU memory and what shader stages will have access to what memory
//
//    //const uint32_t numMeshes = static_cast<uint32_t>(_scene.meshes.size());
//    //const uint32_t numMaterials = static_cast<uint32_t>(_scene.materials.size());
//    //const uint32_t numTextures = static_cast<uint32_t>(_scene.textures.size());
//    //const uint32_t numLights = static_cast<uint32_t>(_scene.lights.size());
//
//    //_rtDescriptorSetsLayouts.resize(SWS_NUM_SETS);
//
//    //// set 0:
//    ////  binding 0  ->  AS
//    ////  binding 1  ->  output image
//    ////  binding 2  ->  Scene data
//    ////  binding 3  ->  Camera data
//
//    //VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding;
//    //accelerationStructureLayoutBinding.binding = SWS_SCENE_AS_BINDING;
//    //accelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
//    //accelerationStructureLayoutBinding.descriptorCount = 1;
//    //accelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
//    //accelerationStructureLayoutBinding.pImmutableSamplers = nullptr;
//
//    //VkDescriptorSetLayoutBinding resultImageLayoutBinding;
//    //resultImageLayoutBinding.binding = SWS_RESULT_IMAGE_BINDING;
//    //resultImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
//    //resultImageLayoutBinding.descriptorCount = 1;
//    //resultImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
//    //resultImageLayoutBinding.pImmutableSamplers = nullptr;
//
//    //VkDescriptorSetLayoutBinding sceneBufferBinding;
//    //sceneBufferBinding.binding = SWS_SCENEDATA_BINDING;
//    //sceneBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//    //sceneBufferBinding.descriptorCount = 1;
//    //sceneBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
//    //sceneBufferBinding.pImmutableSamplers = nullptr;
//
//    //VkDescriptorSetLayoutBinding camdataBufferBinding;
//    //camdataBufferBinding.binding = SWS_CAMDATA_BINDING;
//    //camdataBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//    //camdataBufferBinding.descriptorCount = 1;
//    //camdataBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
//    //camdataBufferBinding.pImmutableSamplers = nullptr;
//
//    //std::vector<VkDescriptorSetLayoutBinding> bindings({
//    //    accelerationStructureLayoutBinding,
//    //    resultImageLayoutBinding,
//    //    sceneBufferBinding,
//    //    camdataBufferBinding
//    //    });
//
//    //VkDescriptorSetLayoutCreateInfo set0LayoutInfo;
//    //set0LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//    //set0LayoutInfo.pNext = nullptr;
//    //set0LayoutInfo.flags = 0;
//    //set0LayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
//    //set0LayoutInfo.pBindings = bindings.data();
//
//    //VkResult error = vkCreateDescriptorSetLayout(_device, &set0LayoutInfo, nullptr, &_rtDescriptorSetsLayouts[SWS_SCENE_AS_SET]);
//    //CHECK_VK_ERROR(error, "vkCreateDescriptorSetLayout");
//
//    //// set 1
//    ////  binding 0  ->  Material data
//    //VkDescriptorSetLayoutBinding materialDataBufferBinding;
//    //materialDataBufferBinding.binding = SWS_MATERIALDATA_BINDING;
//    //materialDataBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//    //materialDataBufferBinding.descriptorCount = numMaterials;
//    //materialDataBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
//    //materialDataBufferBinding.pImmutableSamplers = nullptr;
//
//    //VkDescriptorSetLayoutCreateInfo set1LayoutInfo;
//    //set1LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//    //set1LayoutInfo.pNext = nullptr;
//    //set1LayoutInfo.flags = 0;
//    //set1LayoutInfo.bindingCount = 1;
//    //set1LayoutInfo.pBindings = &materialDataBufferBinding;
//
//    //error = vkCreateDescriptorSetLayout(_device, &set1LayoutInfo, nullptr, &_rtDescriptorSetsLayouts[SWS_MATERIALDATA_SET]);
//    //CHECK_VK_ERROR(error, "vkCreateDescriptorSetLayout");
//
//    //// set 2
//    ////  binding 0  ->  Lighting data
//    //VkDescriptorSetLayoutBinding lightingDataBufferBinding;
//    //lightingDataBufferBinding.binding = SWS_LIGHTINGDATA_BINDING;
//    //lightingDataBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//    //lightingDataBufferBinding.descriptorCount = numLights;
//    //lightingDataBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
//    //lightingDataBufferBinding.pImmutableSamplers = nullptr;
//
//    //VkDescriptorSetLayoutCreateInfo set2LayoutInfo;
//    //set2LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//    //set2LayoutInfo.pNext = nullptr;
//    //set2LayoutInfo.flags = 0;
//    //set2LayoutInfo.bindingCount = 1;
//    //set2LayoutInfo.pBindings = &lightingDataBufferBinding;
//
//    //error = vkCreateDescriptorSetLayout(_device, &set2LayoutInfo, nullptr, &_rtDescriptorSetsLayouts[SWS_LIGHTINGDATA_SET]);
//    //CHECK_VK_ERROR(error, "vkCreateDescriptorSetLayout");
//
//
//    //// set 3
//    ////  binding 0 .. N  ->  per-face material IDs for our meshes  (N = num meshes)
//
//    //const VkDescriptorBindingFlags flag = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
//
//    //VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags;
//    //bindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
//    //bindingFlags.pNext = nullptr;
//    //bindingFlags.pBindingFlags = &flag;
//    //bindingFlags.bindingCount = 1;
//
//    //VkDescriptorSetLayoutBinding ssboBinding;
//    //ssboBinding.binding = 0;
//    //ssboBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
//    //ssboBinding.descriptorCount = numMeshes;
//    //ssboBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
//    //ssboBinding.pImmutableSamplers = nullptr;
//
//    //VkDescriptorSetLayoutCreateInfo set3456LayoutInfo;
//    //set3456LayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//    //set3456LayoutInfo.pNext = &bindingFlags;
//    //set3456LayoutInfo.flags = 0;
//    //set3456LayoutInfo.bindingCount = 1;
//    //set3456LayoutInfo.pBindings = &ssboBinding;
//
//    //error = vkCreateDescriptorSetLayout(_device, &set3456LayoutInfo, nullptr, &_rtDescriptorSetsLayouts[SWS_MATIDS_SET]);
//    //CHECK_VK_ERROR(error, L"vkCreateDescriptorSetLayout");
//
//    //// set 4
//    ////  binding 0 .. N  ->  vertex attributes for our meshes  (N = num meshes)
//    ////   (re-using second's set info)
//
//    //error = vkCreateDescriptorSetLayout(_device, &set3456LayoutInfo, nullptr, &_rtDescriptorSetsLayouts[SWS_ATTRIBS_SET]);
//    //CHECK_VK_ERROR(error, L"vkCreateDescriptorSetLayout");
//
//    //// set 5
//    ////  binding 0 .. N  ->  faces info (indices) for our meshes  (N = num meshes)
//    ////   (re-using second's set info)
//
//    //error = vkCreateDescriptorSetLayout(_device, &set3456LayoutInfo, nullptr, &_rtDescriptorSetsLayouts[SWS_FACES_SET]);
//    //CHECK_VK_ERROR(error, L"vkCreateDescriptorSetLayout");
//
//    //// set 6
//    ////  binding 0 .. N  ->  textures (N = num materials)
//
//    //VkDescriptorSetLayoutBinding textureBinding;
//    //textureBinding.binding = 0;
//    //textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//    //textureBinding.descriptorCount = numTextures;
//    //textureBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
//    //textureBinding.pImmutableSamplers = nullptr;
//
//    //set3456LayoutInfo.pBindings = &textureBinding;
//
//    //error = vkCreateDescriptorSetLayout(_device, &set3456LayoutInfo, nullptr, &_rtDescriptorSetsLayouts[SWS_TEXTURES_SET]);
//    //CHECK_VK_ERROR(error, L"vkCreateDescriptorSetLayout");
//}
//
//bool PixelsForGlory::VulkanRayTracer::CreatePipeline()
//{
//    //// Create pipeline layout for ray tracing 
//    //VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
//    //pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//    //pipelineLayoutCreateInfo.setLayoutCount = SWS_NUM_SETS;
//    //pipelineLayoutCreateInfo.pSetLayouts = _rtDescriptorSetsLayouts.data();
//
//    //VkResult error = vkCreatePipelineLayout(_device, &pipelineLayoutCreateInfo, nullptr, &_rtPipelineLayout);
//    //CHECK_VK_ERROR(error, "vkCreatePipelineLayout");
//
//    //// Create shader binding table for pipeline
//    //VulkanHelpers::Shader rayGenShader;
//    //VulkanHelpers::Shader rayChitShader;
//    //VulkanHelpers::Shader rayMissShader;
//    //VulkanHelpers::Shader shadowChit;
//    //VulkanHelpers::Shader shadowMiss;
//    //rayGenShader.LoadFromFile((_shadersFolder + "ray_gen.bin").c_str());
//    //rayChitShader.LoadFromFile((_shadersFolder + "ray_chit.bin").c_str());
//    //rayMissShader.LoadFromFile((_shadersFolder + "ray_miss.bin").c_str());
//    //shadowChit.LoadFromFile((_shadersFolder + "shadow_ray_chit.bin").c_str());
//    //shadowMiss.LoadFromFile((_shadersFolder + "shadow_ray_miss.bin").c_str());
//
//    //_sbt.Initialize(2, 2, _rtProps.shaderGroupHandleSize, _rtProps.shaderGroupBaseAlignment);
//
//    //// Define ray generationg stage
//    //_sbt.SetRaygenStage(rayGenShader.GetShaderStage(VK_SHADER_STAGE_RAYGEN_BIT_KHR));
//
//    //// Define hit stages for both primary and shadow hits
//    //_sbt.AddStageToHitGroup({ rayChitShader.GetShaderStage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) }, SWS_PRIMARY_HIT_SHADERS_IDX);
//    //_sbt.AddStageToHitGroup({ shadowChit.GetShaderStage(VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) }, SWS_SHADOW_HIT_SHADERS_IDX);
//
//    //// Define miss stages for both primary and shadow misses
//    //_sbt.AddStageToMissGroup(rayMissShader.GetShaderStage(VK_SHADER_STAGE_MISS_BIT_KHR), SWS_PRIMARY_MISS_SHADERS_IDX);
//    //_sbt.AddStageToMissGroup(shadowMiss.GetShaderStage(VK_SHADER_STAGE_MISS_BIT_KHR), SWS_SHADOW_MISS_SHADERS_IDX);
//
//    //// Create the pipeline for ray tracing based on shader binding table
//    //VkRayTracingPipelineCreateInfoKHR rayPipelineInfo = {};
//    //rayPipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
//    //rayPipelineInfo.stageCount = _sbt.GetNumStages();
//    //rayPipelineInfo.pStages = _sbt.GetStages();
//    //rayPipelineInfo.groupCount = _sbt.GetNumGroups();
//    //rayPipelineInfo.pGroups = _sbt.GetGroups();
//    //rayPipelineInfo.maxRecursionDepth = 1;
//    //rayPipelineInfo.layout = _rtPipelineLayout;
//    //rayPipelineInfo.libraries.sType = VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR;
//
//    //error = vkCreateRayTracingPipelinesKHR(_device, VK_NULL_HANDLE, 1, &rayPipelineInfo, VK_NULL_HANDLE, &_rtPipeline);
//    //CHECK_VK_ERROR(error, "vkCreateRayTracingPipelinesKHR");
//
//    //_sbt.CreateSBT(_device, _rtPipeline);
//    return true;
//}
//
//bool PixelsForGlory::VulkanRayTracer::FillCommandBuffer()
//{
//    //VkCommandBufferBeginInfo commandBufferBeginInfo;
//    //commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//    //commandBufferBeginInfo.pNext = nullptr;
//    //commandBufferBeginInfo.flags = 0;
//    //commandBufferBeginInfo.pInheritanceInfo = nullptr;
//
//    //VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
//
//    //for (size_t i = 0; i < _commandBuffers.size(); i++) {
//    //    const VkCommandBuffer commandBuffer = _commandBuffers[i];
//
//    //    if (vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)
//    //    {
//    //        std::cerr << "Failed to begin fill of command buffer!" << std::endl;
//    //        return false;
//    //    }
//
//    //    createPipelineCommand(
//    //        commandBuffer,
//    //        _offscreenImage->GetImage(),
//    //        subresourceRange,
//    //        0,
//    //        VK_ACCESS_SHADER_WRITE_BIT,
//    //        VK_IMAGE_LAYOUT_UNDEFINED,
//    //        VK_IMAGE_LAYOUT_GENERAL);
//
//    //    // Applcation specific draw code
//    //    // Add ray tracing command to command buffer
//    //    vkCmdBindPipeline(commandBuffer,
//    //        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
//    //        _rtPipeline);
//
//    //    vkCmdBindDescriptorSets(commandBuffer,
//    //        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
//    //        _rtPipelineLayout, 0,
//    //        static_cast<uint32_t>(_rtDescriptorSets.size()), _rtDescriptorSets.data(),
//    //        0, 0);
//
//    //    VkStridedBufferRegionKHR raygenSBT = {
//    //        _sbt.GetSBTBuffer(),
//    //        _sbt.GetRaygenOffset(),
//    //        _sbt.GetGroupsStride(),
//    //        _sbt.GetRaygenSize()
//    //    };
//
//    //    VkStridedBufferRegionKHR hitSBT = {
//    //        _sbt.GetSBTBuffer(),
//    //        _sbt.GetHitGroupsOffset(),
//    //        _sbt.GetGroupsStride(),
//    //        _sbt.GetHitGroupsSize()
//    //    };
//
//    //    VkStridedBufferRegionKHR missSBT = {
//    //        _sbt.GetSBTBuffer(),
//    //        _sbt.GetMissGroupsOffset(),
//    //        _sbt.GetGroupsStride(),
//    //        _sbt.GetMissGroupsSize()
//    //    };
//
//    //    VkStridedBufferRegionKHR callableSBT = {};
//
//    //    vkCmdTraceRaysKHR(commandBuffer, &raygenSBT, &missSBT, &hitSBT, &callableSBT, _width, _height, 1u);
//
//    //    createPipelineCommand(
//    //        commandBuffer,
//    //        _swapchainImages[i],
//    //        subresourceRange,
//    //        0,
//    //        VK_ACCESS_TRANSFER_WRITE_BIT,
//    //        VK_IMAGE_LAYOUT_UNDEFINED,
//    //        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
//
//    //    createPipelineCommand(
//    //        commandBuffer,
//    //        _offscreenImage->GetImage(),
//    //        subresourceRange,
//    //        VK_ACCESS_SHADER_WRITE_BIT,
//    //        VK_ACCESS_TRANSFER_READ_BIT,
//    //        VK_IMAGE_LAYOUT_GENERAL,
//    //        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
//
//    //    VkImageCopy copyRegion;
//    //    copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
//    //    copyRegion.srcOffset = { 0, 0, 0 };
//    //    copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
//    //    copyRegion.dstOffset = { 0, 0, 0 };
//    //    copyRegion.extent = { _width, _height, 1 };
//
//    //    vkCmdCopyImage(
//    //        commandBuffer,
//    //        _offscreenImage->GetImage(),
//    //        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
//    //        _swapchainImages[i],
//    //        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//    //        1,
//    //        &copyRegion);
//
//    //    createPipelineCommand(
//    //        commandBuffer,
//    //        _swapchainImages[i], subresourceRange,
//    //        VK_ACCESS_TRANSFER_WRITE_BIT,
//    //        0,
//    //        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//    //        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
//
//    //    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
//    //    {
//    //        std::cerr << "Failed to end fill of command buffer!" << std::endl;
//    //        return false;
//    //    }
//    //}
//
//    return true;
//}
//
//void PixelsForGlory::VulkanRayTracer::SafeDestroy(unsigned long long frameNumber, const VulkanBuffer& buffer)
//{
//    //m_DeleteQueue[frameNumber].push_back(buffer);
//}
//
//void PixelsForGlory::VulkanRayTracer::GarbageCollect(bool force /*= false*/)
//{
//    /*UnityVulkanRecordingState recordingState;
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
//    }*/
//}
//
//
//uint32_t PixelsForGlory::VulkanRayTracer::GetMemoryType(VkMemoryRequirements& memoryRequiriments, VkMemoryPropertyFlags memoryProperties) {
//    uint32_t result = 0;
//    for (uint32_t memoryTypeIndex = 0; memoryTypeIndex < VK_MAX_MEMORY_TYPES; ++memoryTypeIndex) {
//        if (memoryRequiriments.memoryTypeBits & (1 << memoryTypeIndex)) {
//            if ((physicalDeviceMemoryProperties_.memoryTypes[memoryTypeIndex].propertyFlags & memoryProperties) == memoryProperties) {
//                result = memoryTypeIndex;
//                break;
//            }
//        }
//    }
//    return result;
//}