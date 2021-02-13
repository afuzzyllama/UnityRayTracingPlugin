#include "RayTracerAPI.h"

namespace PixelsForGlory
{
    RayTracerAPI* CreateRayTracerAPI(UnityGfxRenderer apiType)
    {
#if SUPPORT_VULKAN
        if (apiType == kUnityGfxRendererVulkan)
        {
            extern RayTracerAPI* CreateRayTracerAPI_Vulkan();
            return CreateRayTracerAPI_Vulkan();
        }
#endif // if SUPPORT_VULKAN

        // Unknown or unsupported graphics API
        return nullptr;
    }
}
