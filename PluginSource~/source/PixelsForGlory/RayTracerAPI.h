#pragma once

#include "../PlatformBase.h"
#include "../Unity/IUnityGraphics.h"

#include <stddef.h>
#include <string>

struct IUnityInterfaces;

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

        virtual void TraceRays(void* data) = 0;
    };

    // Create a graphics API implementation instance for the given API type.
    RayTracerAPI* CreateRayTracerAPI(UnityGfxRenderer apiType);
}