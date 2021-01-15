#pragma once

// Standard base includes, defines that indicate our current platform, etc.
#include <stddef.h>

// Which platform we are on?
// UNITY_WIN - Windows (regular win32)
// UNITY_OSX - Mac OS X
// UNITY_LINUX - Linux
// UNITY_IOS - iOS
// UNITY_TVOS - tvOS
// UNITY_ANDROID - Android
// UNITY_METRO - WSA or UWP
// UNITY_WEBGL - WebGL
#if _MSC_VER
    #define UNITY_WIN 1
#else
    #error "Unknown platform!"
#endif

// Which graphics device APIs we possibly support?
#if UNITY_WIN
    //#define SUPPORT_D3D11 1 // comment this out if you don't have D3D11 header/library files
    //#define SUPPORT_D3D12 1 // comment this out if you don't have D3D12 header/library files
    //#define SUPPORT_OPENGL_UNIFIED 1
    //#define SUPPORT_OPENGL_CORE 1
    #define SUPPORT_VULKAN 1 // Requires Vulkan SDK to be installed
#endif
