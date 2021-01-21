#include "VulkanShaderBindingTable.h"

#include "Debug.h"

PixelsForGlory::VulkanShaderBindingTable::VulkanShaderBindingTable()
    : shaderHandleSize_(0u)
    , shaderGroupAlignment_(0u)
    , numHitGroups_(0u)
    , numMissGroups_(0u) {
}

void PixelsForGlory::VulkanShaderBindingTable::Initialize(const uint32_t numHitGroups, const uint32_t numMissGroups, const uint32_t shaderHandleSize, const uint32_t shaderGroupAlignment) {
    shaderHandleSize_ = shaderHandleSize;
    shaderGroupAlignment_ = shaderGroupAlignment;
    numHitGroups_ = numHitGroups;
    numMissGroups_ = numMissGroups;

    numHitShaders_.resize(numHitGroups, 0u);
    numMissShaders_.resize(numMissGroups, 0u);

    stages_.clear();
    groups_.clear();
}

void PixelsForGlory::VulkanShaderBindingTable::Destroy() {
    numHitShaders_.clear();
    numMissShaders_.clear();
    stages_.clear();
    groups_.clear();

    sbtBuffer_.Destroy();
}

void PixelsForGlory::VulkanShaderBindingTable::SetRaygenStage(const VkPipelineShaderStageCreateInfo& stage) {
    // this shader stage should go first!
    assert(stages_.empty());
    stages_.push_back(stage);

    VkRayTracingShaderGroupCreateInfoKHR groupInfo = {};
    groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    groupInfo.generalShader = 0;
    groupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
    groupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
    groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;
    groups_.push_back(groupInfo); // group 0 is always for raygen
}

void PixelsForGlory::VulkanShaderBindingTable::AddStageToHitGroup(const std::vector<VkPipelineShaderStageCreateInfo>& stages, const uint32_t groupIndex) {
    // raygen stage should go first!
    assert(!stages_.empty());

    assert(groupIndex < numHitShaders_.size());
    assert(!stages.empty() && stages.size() <= 3);// only 3 hit shaders per group (intersection, any-hit and closest-hit)
    assert(numHitShaders_[groupIndex] == 0);

    uint32_t offset = 1; // there's always raygen shader

    for (uint32_t i = 0; i <= groupIndex; ++i) {
        offset += numHitShaders_[i];
    }

    auto itStage = stages_.begin() + offset;
    stages_.insert(itStage, stages.begin(), stages.end());

    VkRayTracingShaderGroupCreateInfoKHR groupInfo = {};
    groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    groupInfo.generalShader = VK_SHADER_UNUSED_KHR;
    groupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
    groupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
    groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

    for (size_t i = 0; i < stages.size(); i++) {
        const VkPipelineShaderStageCreateInfo& stageInfo = stages[i];
        const uint32_t shaderIdx = static_cast<uint32_t>(offset + i);

        if (stageInfo.stage == VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) {
            groupInfo.closestHitShader = shaderIdx;
        }
        else if (stageInfo.stage == VK_SHADER_STAGE_ANY_HIT_BIT_KHR) {
            groupInfo.anyHitShader = shaderIdx;
        }
    };

    groups_.insert((groups_.begin() + 1 + groupIndex), groupInfo);

    numHitShaders_[groupIndex] += static_cast<uint32_t>(stages.size());
}

void PixelsForGlory::VulkanShaderBindingTable::AddStageToMissGroup(const VkPipelineShaderStageCreateInfo& stage, const uint32_t groupIndex) {
    // raygen stage should go first!
    assert(!stages_.empty());

    assert(groupIndex < numMissShaders_.size());
    assert(numMissShaders_[groupIndex] == 0); // only 1 miss shader per group

    uint32_t offset = 1; // there's always raygen shader

    // now skip all hit shaders
    for (const uint32_t numHitShader : numHitShaders_) {
        offset += numHitShader;
    }

    for (uint32_t i = 0; i <= groupIndex; ++i) {
        offset += numMissShaders_[i];
    }

    stages_.insert(stages_.begin() + offset, stage);

    // group create info 
    VkRayTracingShaderGroupCreateInfoKHR groupInfo = {};
    groupInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    groupInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    groupInfo.generalShader = offset;
    groupInfo.closestHitShader = VK_SHADER_UNUSED_KHR;
    groupInfo.anyHitShader = VK_SHADER_UNUSED_KHR;
    groupInfo.intersectionShader = VK_SHADER_UNUSED_KHR;

    // group 0 is always for raygen, then go hit shaders
    groups_.insert((groups_.begin() + (groupIndex + 1 + numHitGroups_)), groupInfo);

    numMissShaders_[groupIndex]++;
}

uint32_t PixelsForGlory::VulkanShaderBindingTable::GetGroupsStride() const {
    return shaderGroupAlignment_;
}

uint32_t PixelsForGlory::VulkanShaderBindingTable::GetNumGroups() const {
    return 1 + numHitGroups_ + numMissGroups_;
}

uint32_t PixelsForGlory::VulkanShaderBindingTable::GetRaygenOffset() const {
    return 0;
}

uint32_t PixelsForGlory::VulkanShaderBindingTable::GetRaygenSize() const {
    return shaderGroupAlignment_;
}

uint32_t PixelsForGlory::VulkanShaderBindingTable::GetHitGroupsOffset() const {
    return GetRaygenOffset() + GetRaygenSize();
}

uint32_t PixelsForGlory::VulkanShaderBindingTable::GetHitGroupsSize() const {
    return numHitGroups_ * shaderGroupAlignment_;
}

uint32_t PixelsForGlory::VulkanShaderBindingTable::GetMissGroupsOffset() const {
    return GetHitGroupsOffset() + GetHitGroupsSize();
}

uint32_t PixelsForGlory::VulkanShaderBindingTable::GetMissGroupsSize() const {
    return numMissGroups_ * shaderGroupAlignment_;
}

uint32_t PixelsForGlory::VulkanShaderBindingTable::GetNumStages() const {
    return static_cast<uint32_t>(stages_.size());
}

const VkPipelineShaderStageCreateInfo* PixelsForGlory::VulkanShaderBindingTable::GetStages() const {
    return stages_.data();
}

const VkRayTracingShaderGroupCreateInfoKHR* PixelsForGlory::VulkanShaderBindingTable::GetGroups() const {
    return groups_.data();
}

uint32_t PixelsForGlory::VulkanShaderBindingTable::GetSBTSize() const {
    return GetNumGroups() * shaderGroupAlignment_;
}

bool PixelsForGlory::VulkanShaderBindingTable::CreateSBT(VkDevice device, VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties,  VkPipeline pipeline) {
    const size_t sbtSize = GetSBTSize();

    VkResult error = 
        sbtBuffer_.Create(
            device, 
            physicalDeviceMemoryProperties, 
            sbtSize, 
            VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 
            VulkanBuffer::kDefaultMemoryPropertyFlags);

    if (VK_SUCCESS != error) {
        return false;
    }

    // get shader group handles
    std::vector<uint8_t> groupHandles(this->GetNumGroups() * shaderHandleSize_);
    error = vkGetRayTracingShaderGroupHandlesKHR(device, pipeline, 0, this->GetNumGroups(), groupHandles.size(), groupHandles.data());
    VK_CHECK("vkGetRayTracingShaderGroupHandlesKHR", error);

    // now we fill our SBT
    uint8_t* mem = static_cast<uint8_t*>(sbtBuffer_.Map());
    for (size_t i = 0; i < this->GetNumGroups(); ++i) {
        memcpy(mem, groupHandles.data() + i * shaderHandleSize_, shaderHandleSize_);
        mem += shaderGroupAlignment_;
    }
    sbtBuffer_.Unmap();

    return (VK_SUCCESS == error);
}

PixelsForGlory::VulkanBuffer PixelsForGlory::VulkanShaderBindingTable::GetBuffer() const {
    return sbtBuffer_;
}
