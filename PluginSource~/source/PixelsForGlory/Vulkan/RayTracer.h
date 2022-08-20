#pragma once

#include "vulkan/vulkan.h"
#include "../../Unity/IUnityGraphics.h"
#include "../../Unity/IUnityGraphicsVulkan.h"
#include "../RayTracerAPI.h"

namespace PixelsForGlory
{
    RayTracerAPI* CreateRayTracerAPI_Vulkan();
}

namespace PixelsForGlory::Vulkan
{
    /// <summary>
   /// Builds VkDeviceCreateInfo that supports ray tracing
   /// </summary>
   /// <param name="physicalDevice">Physical device used to query properties and queues</param>
   /// <param name="outDeviceCreateInfo"></param>
    VkResult CreateDevice_RayTracer(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* unityCreateInfo, const VkAllocationCallbacks* unityAllocator, VkDevice* device);

    /// <summary>
    /// Resolve properties and queues required for ray tracing
    /// </summary>
    /// <param name="physicalDevice"></param>
    void ResolvePropertiesAndQueues_RayTracer(VkPhysicalDevice physicalDevice);

    class RayTracer : public RayTracerAPI
    {
    public:
        static bool CreateDeviceSuccess;

        static RayTracer& Instance()
        {
            static RayTracer instance;
            return instance;
        }

        VkDebugUtilsMessengerEXT debugMessenger_;

        // Friend so hook functions can access
        friend void ResolvePropertiesAndQueues_RayTracer(VkPhysicalDevice physicalDevice);
        friend VkResult CreateInstance_RayTracer(const VkInstanceCreateInfo* unityCreateInfo, const VkAllocationCallbacks* unityAllocator, VkInstance* instance);
        friend VkResult CreateDevice_RayTracer(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* unityCreateInfo, const VkAllocationCallbacks* unityAllocator, VkDevice* device);

        // Deleted functions should generally be public as it results in better error messages due to 
        // the compilers behavior to check accessibility before deleted status

        RayTracer(RayTracer const&) = delete;       // Deleted for singleton
        void operator=(RayTracer const&) = delete;  // Deleted for singleton

        /// <summary>
        /// Initalize instance with data from Unity
        /// </summary>
        /// <param name="unityVulkanInstance"></param>
        void InitializeFromUnityInstance(IUnityGraphicsVulkan* graphicsInterface);

        /// <summary>
        /// Shutdown the ray tracer and release all resources
        /// </summary>
        void Shutdown();

#pragma region RayTracerAPI

        virtual bool ProcessDeviceEvent(UnityGfxDeviceEventType type, IUnityInterfaces* interfaces);
        
        virtual void* GetInstance();
        virtual void* GetDevice();
        virtual void* GetPhysicalDevice();

        virtual void TraceRays(void* data);

#pragma endregion RayTracerAPI


    private:
        /// <summary>
        /// Dummy null device for device_ member
        /// </summary>
        static VkDevice NullDevice;

        /// <summary>
        /// Graphics interface from Unity3D.  Used to secure command buffers
        /// </summary>
        const IUnityGraphicsVulkan* graphicsInterface_;

        uint32_t graphicsQueueFamilyIndex_;
        uint32_t transferQueueFamilyIndex_;

        RayTracer();    // Private for singleton

    };
}

