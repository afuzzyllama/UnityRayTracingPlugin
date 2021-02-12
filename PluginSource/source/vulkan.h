#pragma once

// Don't load prototypes, volk will take care of this
#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif

#include <string>
#include "vulkan/vulkan.h"
#ifdef WIN32
#include <Windows.h>
#include "vulkan/vulkan_win32.h"
#endif
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
    case r: \
        return #r;
        STR(VK_SUCCESS)
        STR(VK_NOT_READY)
        STR(VK_TIMEOUT)
        STR(VK_EVENT_SET)
        STR(VK_EVENT_RESET)
        STR(VK_INCOMPLETE)
        STR(VK_ERROR_OUT_OF_HOST_MEMORY)
        STR(VK_ERROR_OUT_OF_DEVICE_MEMORY)
        STR(VK_ERROR_INITIALIZATION_FAILED)
        STR(VK_ERROR_DEVICE_LOST)
        STR(VK_ERROR_MEMORY_MAP_FAILED)
        STR(VK_ERROR_LAYER_NOT_PRESENT)
        STR(VK_ERROR_EXTENSION_NOT_PRESENT)
        STR(VK_ERROR_FEATURE_NOT_PRESENT)
        STR(VK_ERROR_INCOMPATIBLE_DRIVER)
        STR(VK_ERROR_TOO_MANY_OBJECTS)
        STR(VK_ERROR_FORMAT_NOT_SUPPORTED)
        STR(VK_ERROR_FRAGMENTED_POOL)
        STR(VK_ERROR_UNKNOWN)
        STR(VK_ERROR_OUT_OF_POOL_MEMORY)
        STR(VK_ERROR_INVALID_EXTERNAL_HANDLE)
        STR(VK_ERROR_FRAGMENTATION)
        STR(VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS)
        STR(VK_ERROR_SURFACE_LOST_KHR)
        STR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR)
        STR(VK_SUBOPTIMAL_KHR)
        STR(VK_ERROR_OUT_OF_DATE_KHR)
        STR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR)
        STR(VK_ERROR_VALIDATION_FAILED_EXT)
        STR(VK_ERROR_INVALID_SHADER_NV)
        STR(VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT)
        STR(VK_ERROR_NOT_PERMITTED_EXT)
        STR(VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT)
        STR(VK_THREAD_IDLE_KHR)
        STR(VK_THREAD_DONE_KHR)
        STR(VK_OPERATION_DEFERRED_KHR)
        STR(VK_OPERATION_NOT_DEFERRED_KHR)
        STR(VK_PIPELINE_COMPILE_REQUIRED_EXT)
        STR(VK_RESULT_MAX_ENUM)

#undef STR
    default:
        return "UNKNOWN_ERROR_CODE";
    }
}

#ifndef __FILENAME__ 
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#ifndef VK_CHECK
#define VK_CHECK(fn, code)                                                                                                                                            \
    do                                                                                                                                                                \
    {                                                                                                                                                                 \
        VkResult err = code;                                                                                                                                          \
        if (err != VK_SUCCESS)                                                                                                                                                      \
        {                                                                                                                                                             \
            PFG_EDITORLOGERROR(std::string(__FILENAME__) + " (" + std::to_string(__LINE__) + "): " + std::string(fn) + " returned error: " + vkresult_to_string(err)) \
            abort();                                                                                                                                                  \
        }                                                                                                                                                             \
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

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    std::string msg = std::string(pCallbackData->pMessage);

    // Only print validation errors
    if (msg.rfind("Validation Error:", 0) == 0)
    {
        PFG_EDITORLOG(msg);
    }

    return VK_FALSE;
}

static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

/// <summary>
/// Get the memory type index from the passed in requirements and properties
/// </summary>
/// <param name="physicalDeviceMemoryProperties"></param>
/// <param name="memoryRequiriments"></param>
/// <param name="memoryProperties"></param>
/// <returns></returns>
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

/// <summary>
/// Convert a float array representing a 4x4 matrix from Unity to a 4x4 matrix for Vulkan
/// </summary>
/// <param name="src"></param>
/// <param name="dst"></param>
void static FloatArrayToMatrix(const float* src, mat4& dst) {

    // Unity is column major, but Vulkan is row major.  Convert here
    dst = mat4(src[0], src[4], src[8],  src[12],
               src[1], src[5], src[9],  src[13],
               src[2], src[6], src[10], src[14],
               src[3], src[7], src[11], src[15]);
}
