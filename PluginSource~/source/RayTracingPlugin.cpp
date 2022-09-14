#include "PlatformBase.h"
#include "PixelsForGlory/RayTracerAPI.h"

#include <assert.h>
#include <math.h>
#include <map>
#include <vector>

#include "PixelsForGlory/Helpers.h"

#define PLUGIN_CHECK()  if (s_CurrentAPI == nullptr) { return; }
#define PLUGIN_CHECK_RETURN(returnValue)  if (s_CurrentAPI == nullptr) { return returnValue; }

// --------------------------------------------------------------------------
// UnitySetInterfaces
static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);

static IUnityInterfaces* s_UnityInterfaces = NULL;
static IUnityGraphics* s_Graphics = NULL;

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
    s_UnityInterfaces = unityInterfaces;
    s_Graphics = s_UnityInterfaces->Get<IUnityGraphics>();
    s_Graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);
    
#if SUPPORT_VULKAN
    if (s_Graphics->GetRenderer() == kUnityGfxRendererNull)
    {
        extern void RenderAPI_Vulkan_OnPluginLoad(IUnityInterfaces*);
        RenderAPI_Vulkan_OnPluginLoad(unityInterfaces);
    }
#endif // SUPPORT_VULKAN

    // Run OnGraphicsDeviceEvent(initialize) manually on plugin load
    OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
    s_Graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);
}

// --------------------------------------------------------------------------
// GraphicsDeviceEvent


static PixelsForGlory::RayTracerAPI* s_CurrentAPI = nullptr;
static UnityGfxRenderer s_DeviceType = kUnityGfxRendererNull;
static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
    // Create graphics API implementation upon initialization
    if (eventType == kUnityGfxDeviceEventInitialize)
    {
        assert(s_CurrentAPI == nullptr);
        s_DeviceType = s_Graphics->GetRenderer();
        s_CurrentAPI = PixelsForGlory::CreateRayTracerAPI(s_DeviceType);
    }

    // Let the implementation process the device related events
    bool success = false;
    if (s_CurrentAPI)
    {
        success = s_CurrentAPI->ProcessDeviceEvent(eventType, s_UnityInterfaces);
    }

    // If kUnityGfxDeviceEventInitialize was not successful, we don't want the plugin to run
    if (eventType == kUnityGfxDeviceEventInitialize && success == false)
    {
        s_CurrentAPI = nullptr;
        s_DeviceType = kUnityGfxRendererNull;
    }

    // Cleanup graphics API implementation upon shutdown
    if (eventType == kUnityGfxDeviceEventShutdown)
    {
        s_CurrentAPI = nullptr;
        s_DeviceType = kUnityGfxRendererNull;
    }
}

enum class Events
{
    None = 0,
    CmdTraceRaysKHR = 1,
    Blit = 2
};

static void UNITY_INTERFACE_API OnEvent(int eventId)
{
    // Unknown / unsupported graphics device type? Do nothing
    PLUGIN_CHECK();
}

static void UNITY_INTERFACE_API OnEventAndData(int eventId, void* data)
{
    // Unknown / unsupported graphics device type? Do nothing
    PLUGIN_CHECK();
    
    auto event = static_cast<Events>(eventId);

    switch (event)
    {
        case Events::CmdTraceRaysKHR:
            s_CurrentAPI->CmdTraceRaysKHR(data);
            break;
        case Events::Blit:
            s_CurrentAPI->Blit(data);
            break;
        default:
            PFG_EDITORLOGERROR("Unsupported event id: " + std::to_string(eventId));
    }
}

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetEventFunc()
{
    return OnEvent;
}

extern "C" UnityRenderingEventAndData UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetEventAndDataFunc()
{
    return OnEventAndData;
}

extern "C" InstancePointer UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetInstance()
{
    PLUGIN_CHECK_RETURN(nullptr);

    return s_CurrentAPI->GetInstance();
}

extern "C" DevicePointer UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetDevice()
{
    PLUGIN_CHECK_RETURN(nullptr);

    return s_CurrentAPI->GetDevice();
}

extern "C" PhysicalDevicePointer UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetPhysicalDevice()
{
    PLUGIN_CHECK_RETURN(nullptr);

    return s_CurrentAPI->GetPhysicalDevice();
}

extern "C" Image UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetImageFromTexture(void* nativeTexturePtr)
{
    PLUGIN_CHECK_RETURN(nullptr);

    return s_CurrentAPI->GetImageFromTexture(nativeTexturePtr);
}

extern "C" unsigned long long UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetSafeFrameNumber()
{
    PLUGIN_CHECK_RETURN(0);

    return s_CurrentAPI->GetSafeFrameNumber();
}

extern "C" uint32_t UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetQueueFamilyIndex()
{
    PLUGIN_CHECK_RETURN(0);

    return s_CurrentAPI->GetQueueFamilyIndex();
}

extern "C" uint32_t UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetQueueIndex()
{
    PLUGIN_CHECK_RETURN(0);

    return s_CurrentAPI->GetQueueIndex();
}
