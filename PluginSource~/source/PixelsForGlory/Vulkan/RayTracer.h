#pragma once

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
    bool ResolveQueueFamily_RayTracer(VkPhysicalDevice physicalDevice, uint32_t& index);

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
        friend bool ResolveQueueFamily_RayTracer(VkPhysicalDevice physicalDevice, uint32_t& index);
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
        virtual void* GetImageFromTexture(void* nativeTexturePtr);
        virtual unsigned long long GetSafeFrameNumber();
        virtual uint32_t GetQueueFamilyIndex();
        virtual uint32_t GetQueueIndex();
        virtual void Blit(void* data);

        virtual void CmdTraceRaysKHR(void* data);

        virtual void TraceRays(void* data);

#pragma endregion RayTracerAPI


    private:
        struct vkCmdTraceRaysKHRData
        {
        public:
            VkPipeline pipeline;
            VkPipelineLayout pipelineLayout;
            uint32_t descriptorCount;
            VkDescriptorSet* pDescriptorSets;
            VkImage offscreenImage;
            void* dstTexture;
            VkExtent3D extent;
            VkStridedDeviceAddressRegionKHR raygenShaderEntry;
            VkStridedDeviceAddressRegionKHR missShaderEntry;
            VkStridedDeviceAddressRegionKHR hitShaderEntry;
            VkStridedDeviceAddressRegionKHR callableShaderEntry;
        };

        struct BlitData {
            VkImage srcImage;
            void* dstHandle;
            VkExtent3D extent;
        };

        /// <summary>
        /// Dummy null device for device_ member
        /// </summary>
        static VkDevice NullDevice;

        /// <summary>
        /// Graphics interface from Unity3D.  Used to secure command buffers
        /// </summary>
        const IUnityGraphicsVulkan* graphicsInterface_;

        uint32_t queueFamilyIndex_;
        uint32_t queueIndex_;
        
        RayTracer();    // Private for singleton

    };
}

