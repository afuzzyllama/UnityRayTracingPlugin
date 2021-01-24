#pragma once

// Don't load prototypes, volk will take care of this
#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif

#include <string>
#include "vulkan/vulkan.h"
#include "volk.h"

#ifndef VK_NO_FLAGS
#define VK_NO_FLAGS 0
#endif

#include "PixelsForGlory/Debug.h"

static const std::string vkresult_to_string(VkResult result)
{
    switch (result)
    {
#define STR(r)   \
    case VK_##r: \
        return #r
        STR(NOT_READY);
        STR(TIMEOUT);
        STR(EVENT_SET);
        STR(EVENT_RESET);
        STR(INCOMPLETE);
        STR(ERROR_OUT_OF_HOST_MEMORY);
        STR(ERROR_OUT_OF_DEVICE_MEMORY);
        STR(ERROR_INITIALIZATION_FAILED);
        STR(ERROR_DEVICE_LOST);
        STR(ERROR_MEMORY_MAP_FAILED);
        STR(ERROR_LAYER_NOT_PRESENT);
        STR(ERROR_EXTENSION_NOT_PRESENT);
        STR(ERROR_FEATURE_NOT_PRESENT);
        STR(ERROR_INCOMPATIBLE_DRIVER);
        STR(ERROR_TOO_MANY_OBJECTS);
        STR(ERROR_FORMAT_NOT_SUPPORTED);
        STR(ERROR_SURFACE_LOST_KHR);
        STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
        STR(SUBOPTIMAL_KHR);
        STR(ERROR_OUT_OF_DATE_KHR);
        STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
        STR(ERROR_VALIDATION_FAILED_EXT);
        STR(ERROR_INVALID_SHADER_NV);
#undef STR
    default:
        return "UNKNOWN_ERROR";
    }
}

#ifndef __FILENAME__ 
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#ifndef VK_CHECK
#define VK_CHECK(fn, code)                                                                                                                                        \
    do                                                                                                                                                            \
    {                                                                                                                                                             \
        VkResult err = code;                                                                                                                                      \
        if (err)                                                                                                                                                  \
        {                                                                                                                                                         \
            PFG_EDITORLOGERROR(std::string(__FILENAME__) + " (" + std::to_string(__LINE__) + "): " + std::string(fn) + " returned error: " + vkresult_to_string(err)) \
            abort();                                                                                                                                              \
        }                                                                                                                                                         \
    } while (0);
#endif

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

using vec2 = glm::highp_vec2;
using vec3 = glm::highp_vec3;
using vec4 = glm::highp_vec4;
using mat4 = glm::highp_mat4;
using quat = glm::highp_quat;

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

void static FloatArrayToMatrix(const float* src, mat4& dst) {

    dst = mat4(src[0],  src[1],  src[2],  src[3],
               src[4],  src[5],  src[6],  src[7],
               src[8],  src[9],  src[10], src[10],
               src[12], src[13], src[14], src[15]);
}
