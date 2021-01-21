// Example low level rendering Unity plugin
#include "PlatformBase.h"
#include "PixelsForGlory/RayTracerAPI.h"

#include <assert.h>
#include <math.h>
#include <map>
#include <vector>

#include "PixelsForGlory/Debug.h"

// --------------------------------------------------------------------------
// SetTimeFromUnity, an example function we export which is called by one of the scripts.

static float g_Time;
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API SetTimeFromUnity (float t) { g_Time = t; }

// --------------------------------------------------------------------------
// UnitySetInterfaces

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);

static IUnityInterfaces* s_UnityInterfaces = NULL;
static IUnityGraphics* s_Graphics = NULL;

extern "C" void    UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginLoad(IUnityInterfaces* unityInterfaces)
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


static PixelsForGlory::RayTracerAPI* s_CurrentAPI = NULL;
static UnityGfxRenderer s_DeviceType = kUnityGfxRendererNull;
static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
    // Create graphics API implementation upon initialization
    if (eventType == kUnityGfxDeviceEventInitialize)
    {
        assert(s_CurrentAPI == NULL);
        s_DeviceType = s_Graphics->GetRenderer();
        s_CurrentAPI = PixelsForGlory::CreateRayTracerAPI(s_DeviceType);
    }

    // Let the implementation process the device related events
    if (s_CurrentAPI)
    {
        s_CurrentAPI->ProcessDeviceEvent(eventType, s_UnityInterfaces);
    }

    // Cleanup graphics API implementation upon shutdown
    if (eventType == kUnityGfxDeviceEventShutdown)
    {
        delete s_CurrentAPI;
        s_CurrentAPI = NULL;
        s_DeviceType = kUnityGfxRendererNull;
    }
}

static void UNITY_INTERFACE_API OnRenderEvent(int eventID)
{
    // Unknown / unsupported graphics device type? Do nothing
    if (s_CurrentAPI == nullptr)
    {
        return;
    }

    s_CurrentAPI->TraceRays();
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetSharedMeshIndex(int sharedMeshInstanceId)
{
    if (s_CurrentAPI == nullptr)
    {
        return -1;
    }

    return s_CurrentAPI->GetSharedMeshIndex(sharedMeshInstanceId);
}


extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API AddMesh(int instanceId, float* vertices, float* normals, float* uvs, int vertexCount, int* indices, int indexCount)
{
    if (s_CurrentAPI == nullptr)
    {
        return -1;
    }

    return s_CurrentAPI->AddMesh(instanceId, vertices, normals, uvs, vertexCount, indices, indexCount);
}

extern "C" int UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API AddTlasInstance(int sharedMeshIndex, float* l2wMatrix)
{
    if (s_CurrentAPI == nullptr)
    {
        return -1;
    }

    return s_CurrentAPI->AddTlasInstance(sharedMeshIndex, l2wMatrix);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API BuildTlas()
{
    if (s_CurrentAPI == nullptr)
    {
        return;
    }

    s_CurrentAPI->BuildTlas();
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API Prepare(int width, int height)
{
    if (s_CurrentAPI == nullptr)
    {
        return;
    }

    s_CurrentAPI->Prepare(width, height);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UpdateCamera(float* camPos, float* camDir, float* camUp, float* camSide, float* camNearFarFov)
{
    if (s_CurrentAPI == nullptr)
    {
        return;
    }

    s_CurrentAPI->UpdateCamera(camPos, camDir, camUp, camSide, camNearFarFov);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UpdateSceneData(float* color)
{
    if (s_CurrentAPI == nullptr)
    {
        return;
    }

    s_CurrentAPI->UpdateSceneData(color);
}

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API GetRenderEventFunc()
{
    return OnRenderEvent;
}

