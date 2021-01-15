#pragma once

#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif

#ifndef VK_ENABLE_BETA_EXTENSIONS
#define VK_ENABLE_BETA_EXTENSIONS
#endif 

#include "vulkan/vulkan.h"
#include "volk.h"

uint32_t static GetMemoryType(VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties, VkMemoryRequirements& memoryRequiriments, VkMemoryPropertyFlags memoryProperties) {
    uint32_t result = 0;
    for (uint32_t memoryTypeIndex = 0; memoryTypeIndex < VK_MAX_MEMORY_TYPES; ++memoryTypeIndex) {
        if (memoryRequiriments.memoryTypeBits & (1 << memoryTypeIndex)) {
            if ((physicalDeviceMemoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & memoryProperties) == memoryProperties) {
                result = memoryTypeIndex;
                break;
            }
        }
    }
    return result;
}
