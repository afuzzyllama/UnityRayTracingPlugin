#pragma once
#include "../PlatformBase.h"
#include "../Unity/IUnityGraphics.h"

#include <stddef.h>
#include <string>

struct IUnityInterfaces;

typedef void* InstancePointer;
typedef void* DevicePointer;
typedef void* PhysicalDevicePointer;
typedef void* Image;

namespace PixelsForGlory
{
    class RayTracerAPI
    {
    public:
        virtual ~RayTracerAPI() { }

        /// <summary>
        /// Process general event like initialization, shutdown, device loss/reset etc.
        /// </summary>
        /// <param name="type"></param>
        /// <param name="interfaces"></param>
        /// <returns>true if processing was successful, false otherwise</returns>
        virtual bool ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces) = 0;

        virtual void* GetInstance() = 0;
        virtual void* GetDevice() = 0;
        virtual void* GetPhysicalDevice() = 0;
        virtual void* GetImageFromTexture(void* nativeTexturePtr) = 0;
        virtual unsigned long long GetSafeFrameNumber() = 0;
        virtual uint32_t GetQueueFamilyIndex() = 0;
        virtual uint32_t GetQueueIndex() = 0;
        virtual void CmdTraceRaysKHR(void* data) = 0;
        virtual void Blit(void* data) = 0;

        virtual void TraceRays(void* data) = 0;
    };

    // Create a graphics API implementation instance for the given API type.
    RayTracerAPI* CreateRayTracerAPI(UnityGfxRenderer apiType);
}