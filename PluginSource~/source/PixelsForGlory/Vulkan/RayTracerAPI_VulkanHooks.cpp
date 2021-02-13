#include "../../PlatformBase.h"

#if SUPPORT_VULKAN

#include "../../Unity/IUnityGraphics.h"
#include "../../Unity/IUnityGraphicsVulkan.h"

#include "../Debug.h"

namespace PixelsForGlory::Vulkan
{
    extern VkResult CreateInstance_RayTracer(const VkInstanceCreateInfo* unityCreateInfo, const VkAllocationCallbacks* unityAllocator, VkInstance* instance);
    extern VkResult CreateDevice_RayTracer(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* unityCreateInfo, const VkAllocationCallbacks* unityAllocator, VkDevice* device);
}

// include volk.c for implementation
#include "volk.c"

// Below are all the hooks required for setting up ray tracing in Vulkan.  These calls are injected into the Unity startup for Vulkan.

static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
    PFG_EDITORLOG("Hook_vkCreateInstance");

    VkResult result = PixelsForGlory::Vulkan::CreateInstance_RayTracer(pCreateInfo, pAllocator, pInstance);
    VK_CHECK("vkCreateInstance", result);

    PFG_EDITORLOG("Hooked into vkCreateInstance successfully");

    return result;
}

static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
    PFG_EDITORLOG("Hook_vkCreateDevice");

    VkResult result = PixelsForGlory::Vulkan::CreateDevice_RayTracer(physicalDevice, pCreateInfo, pAllocator, pDevice);
    VK_CHECK("vkCreateDevice", result);

    PFG_EDITORLOG("Hooked into vkCreateDevice successfully");

    return result;
}

static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL Hook_vkGetInstanceProcAddr(VkInstance device, const char* funcName)
{
    if (!funcName)
        return nullptr;

    // Only need to intercept vkCreateInstance and vkCreateDevice to support ray tracing
#define INTERCEPT(fn) if (strcmp(funcName, #fn) == 0) return (PFN_vkVoidFunction)&Hook_##fn
    INTERCEPT(vkCreateInstance);
    INTERCEPT(vkCreateDevice);
#undef INTERCEPT

    return nullptr;
}

static PFN_vkGetInstanceProcAddr UNITY_INTERFACE_API InterceptVulkanInitialization(PFN_vkGetInstanceProcAddr getInstanceProcAddr, void*)
{
    vkGetInstanceProcAddr = getInstanceProcAddr;
    return Hook_vkGetInstanceProcAddr;
}

extern "C" void RenderAPI_Vulkan_OnPluginLoad(IUnityInterfaces * interfaces)
{
    interfaces->Get<IUnityGraphicsVulkan>()->InterceptInitialization(InterceptVulkanInitialization, NULL);
}

#endif // #if SUPPORT_VULKAN

