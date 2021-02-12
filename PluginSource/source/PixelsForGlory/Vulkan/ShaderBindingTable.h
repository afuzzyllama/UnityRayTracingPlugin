#pragma once
#include "../../vulkan.h"
#include "Buffer.h"
#include <vector>


namespace PixelsForGlory::Vulkan
{
    /// <summary>
    /// Represents a shader binding table for ray tracing pipeline
    /// </summary>
    class ShaderBindingTable
    {
    public:
        ShaderBindingTable();
        ~ShaderBindingTable() = default;

        /// <summary>
        /// Initialize the table
        /// </summary>
        /// <param name="numHitGroups"></param>
        /// <param name="numMissGroups"></param>
        /// <param name="shaderHandleSize"></param>
        /// <param name="shaderGroupAlignment"></param>
        void Initialize(const uint32_t numHitGroups, const uint32_t numMissGroups, const uint32_t shaderHandleSize, const uint32_t shaderGroupAlignment);

        /// <summary>
        /// Destory the table
        /// </summary>
        void Destroy();

        /// <summary>
        /// Add ray gen state to pipeline shader
        /// </summary>
        /// <param name="stage"></param>
        void SetRaygenStage(const VkPipelineShaderStageCreateInfo& stage);

        /// <summary>
        /// Add shader to hit group
        /// </summary>
        /// <param name="stages"></param>
        /// <param name="groupIndex"></param>
        void AddStageToHitGroup(const std::vector<VkPipelineShaderStageCreateInfo>& stages, const uint32_t groupIndex);

        /// <summary>
        /// Add shader to miss group
        /// </summary>
        /// <param name="stage"></param>
        /// <param name="groupIndex"></param>
        void AddStageToMissGroup(const VkPipelineShaderStageCreateInfo& stage, const uint32_t groupIndex);

        // Getters for structure related information
        uint32_t GetGroupsStride() const;
        uint32_t GetNumGroups() const;
        uint32_t GetRaygenOffset() const;
        uint32_t GetRaygenSize() const;
        uint32_t GetHitGroupsOffset() const;
        uint32_t GetHitGroupsSize() const;
        uint32_t GetMissGroupsOffset() const;
        uint32_t GetMissGroupsSize() const;

        uint32_t GetNumStages() const;
        const VkPipelineShaderStageCreateInfo* GetStages() const;
        const VkRayTracingShaderGroupCreateInfoKHR* GetGroups() const;

        uint32_t    GetSBTSize() const;

        /// <summary>
        /// Create shader binding table on device
        /// </summary>
        /// <param name="device"></param>
        /// <param name="rtPipeline"></param>
        /// <returns></returns>
        bool        CreateSBT(VkDevice device, VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties, VkPipeline rtPipeline);

        Vulkan::Buffer    GetBuffer() const;

    private:
        uint32_t                                            shaderHandleSize_;
        uint32_t                                            shaderGroupAlignment_;
        uint32_t                                            numHitGroups_;
        uint32_t                                            numMissGroups_;
        std::vector<uint32_t>                               numHitShaders_;
        std::vector<uint32_t>                               numMissShaders_;
        std::vector<VkPipelineShaderStageCreateInfo>        stages_;
        std::vector<VkRayTracingShaderGroupCreateInfoKHR>   groups_;
        Vulkan::Buffer                                        sbtBuffer_;
    };
}

