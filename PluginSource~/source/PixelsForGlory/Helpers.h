#pragma once

#include <stdio.h>
#include <string>

#include "../Unity/IUnityGraphicsVulkan.h"

#ifndef PFG_EDITORLOG
#define PFG_EDITORLOG(msg) {\
    auto log_msg = "[Raytracing Plugin] " + std::string(msg) + "\n"; \
    fprintf(stdout, log_msg.c_str()); \
}
#endif

#ifndef PFG_EDITORLOGW
#define PFG_EDITORLOGW(msg) {\
    auto log_msg = L"[Raytracing Plugin] " + std::wstring(msg) + L"\n"; \
    fwprintf(stdout, log_msg.c_str()); \
}
#endif

#ifndef PFG_EDITORLOGERROR
#define PFG_EDITORLOGERROR(msg) {\
    auto log_msg = "[Raytracing Plugin] " + std::string(msg) + "\n"; \
    fprintf(stderr, log_msg.c_str()); \
}
#endif

#ifndef PFG_EDITORLOGERRORW
#define PFG_EDITORLOGERRORW(msg) {\
    auto log_msg = L"[Raytracing Plugin] " + std::wstring(msg) + L"\n"; \
    fwprintf(stderr, log_msg.c_str()); \
}
#endif

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

