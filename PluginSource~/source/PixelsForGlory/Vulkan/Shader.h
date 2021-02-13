#pragma once
#include "../../vulkan.h"

namespace PixelsForGlory::Vulkan
{
    /// <summary>
    /// Represents a shader for a shader binding table
    /// </summary>
    class Shader {
    public:
        Shader(VkDevice device);
        ~Shader();

        /// <summary>
        /// Load shader from file
        /// </summary>
        /// <param name="fileName"></param>
        /// <returns></returns>
        bool LoadFromFile(const wchar_t* fileName);

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


