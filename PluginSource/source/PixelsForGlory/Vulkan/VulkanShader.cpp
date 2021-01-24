#include "VulkanShader.h"

#include <fstream>
#include <vector>

PixelsForGlory::VulkanShader::VulkanShader(VkDevice device)
    : device_(device)
    , shaderModule_(VK_NULL_HANDLE)
    {}
 
PixelsForGlory::VulkanShader::~VulkanShader()
{
    Destroy();
}

bool PixelsForGlory::VulkanShader::LoadFromFile(const char* fileName)
{
    bool result = false;

    std::ifstream file(fileName, std::ios::in | std::ios::binary);
    if (file) {
        file.seekg(0, std::ios::end);
        const size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> bytecode(fileSize);
        bytecode.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        VkShaderModuleCreateInfo shaderModuleCreateInfo;
        shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shaderModuleCreateInfo.pNext = nullptr;
        shaderModuleCreateInfo.codeSize = bytecode.size();
        shaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(bytecode.data());
        shaderModuleCreateInfo.flags = 0;

        const VkResult error = vkCreateShaderModule(device_, &shaderModuleCreateInfo, nullptr, &shaderModule_);
        result = (VK_SUCCESS == error);

        if (!result)
        {
            Debug::LogError("Error loading shader: " + std::to_string((int)error));
        }
    }
    else
    {
        Debug::LogError("Cannot open shader file: " + std::string(fileName));
        return false;
    }

    return result;
}

void PixelsForGlory::VulkanShader::Destroy()
{
    if (shaderModule_) {
        vkDestroyShaderModule(device_, shaderModule_, nullptr);
        shaderModule_ = VK_NULL_HANDLE;
    }
}

VkPipelineShaderStageCreateInfo PixelsForGlory::VulkanShader::GetShaderStage(VkShaderStageFlagBits stage) {
    return VkPipelineShaderStageCreateInfo{
        /*sType*/ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        /*pNext*/ nullptr,
        /*flags*/ 0,
        /*stage*/ stage,
        /*module*/ shaderModule_,
        /*pName*/ "main",
        /*pSpecializationInfo*/ nullptr
    };
}
