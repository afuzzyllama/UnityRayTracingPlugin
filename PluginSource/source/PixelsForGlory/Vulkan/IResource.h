#pragma once
namespace PixelsForGlory::Vulkan
{
    class IResource
    {
    public:
        virtual ~IResource() { }
        virtual void Destroy() = 0;
    };
}