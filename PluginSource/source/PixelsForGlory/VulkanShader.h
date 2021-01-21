#pragma once
#include "../vulkan.h"

namespace PixelsForGlory
{
    class VulkanShader {
    public:
        VulkanShader(VkDevice device);
        ~VulkanShader();

        /// <summary>
        /// Load shader from file
        /// </summary>
        /// <param name="fileName"></param>
        /// <returns></returns>
        bool LoadFromFile(const char* fileName);

        /// <summary>
        /// Destory shader
        /// </summary>
        void Destroy();

        /// <summary>
        /// Get shader stage information
        /// </summary>
        /// <param name="stage"></param>
        /// <returns></returns>
        VkPipelineShaderStageCreateInfo GetShaderStage(VkShaderStageFlagBits stage);

    private:
        VkDevice        device_;
        VkShaderModule  shaderModule_;
    };
}


