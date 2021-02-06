#include "Image.h"

#include "Buffer.h"

namespace PixelsForGlory::Vulkan
{
    Image::Image()
        : device_(VK_NULL_HANDLE)
        , physicalDeviceMemoryProperties_(VkPhysicalDeviceMemoryProperties())
        , format_(VK_FORMAT_B8G8R8A8_UNORM)
        , image_(VK_NULL_HANDLE)
        , memory_(VK_NULL_HANDLE)
        , imageView_(VK_NULL_HANDLE)
        , sampler_(VK_NULL_HANDLE)
        , loadedFromUnity_(false)
        , name_("[NOT CREATED]")
    {}

    Image::Image(VkDevice device, VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties)
        : device_(device)
        , physicalDeviceMemoryProperties_(physicalDeviceMemoryProperties)
        , format_(VK_FORMAT_B8G8R8A8_UNORM)
        , image_(VK_NULL_HANDLE)
        , memory_(VK_NULL_HANDLE)
        , imageView_(VK_NULL_HANDLE)
        , sampler_(VK_NULL_HANDLE)
        , loadedFromUnity_(false)
        , name_("[NOT CREATED]")
    {}

    Image::~Image()
    {
        this->Destroy();
    }

    VkResult Image::Create(
        std::string name,
        VkImageType imageType,
        VkFormat format,
        VkExtent3D extent,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags memoryProperties) {
        VkResult result = VK_SUCCESS;


        assert(image_ == VK_NULL_HANDLE);

        name_ = name;
        format_ = format;

        VkImageCreateInfo imageCreateInfo = {};
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.imageType = imageType;
        imageCreateInfo.format = format;
        imageCreateInfo.extent = extent;
        imageCreateInfo.mipLevels = 1;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = tiling;
        imageCreateInfo.usage = usage;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        result = vkCreateImage(device_, &imageCreateInfo, nullptr, &image_);

        if (VK_SUCCESS == result) {
            VkMemoryRequirements memoryRequirements = {};
            vkGetImageMemoryRequirements(device_, image_, &memoryRequirements);

            VkMemoryAllocateInfo memoryAllocateInfo = {};
            memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            memoryAllocateInfo.allocationSize = memoryRequirements.size;
            memoryAllocateInfo.memoryTypeIndex = GetMemoryType(physicalDeviceMemoryProperties_, memoryRequirements, memoryProperties);

            result = vkAllocateMemory(device_, &memoryAllocateInfo, nullptr, &memory_);
            if (VK_SUCCESS != result) {
                vkDestroyImage(device_, image_, nullptr);
                image_ = VK_NULL_HANDLE;
                memory_ = VK_NULL_HANDLE;
            }
            else {
                result = vkBindImageMemory(device_, image_, memory_, 0);
                if (VK_SUCCESS != result) {
                    vkDestroyImage(device_, image_, nullptr);
                    vkFreeMemory(device_, memory_, nullptr);
                    image_ = VK_NULL_HANDLE;
                    memory_ = VK_NULL_HANDLE;
                }
            }
        }

        return result;
    }

    void Image::Destroy() {
        if (sampler_ != VK_NULL_HANDLE) {
            vkDestroySampler(device_, sampler_, nullptr);
            sampler_ = VK_NULL_HANDLE;
        }
        if (imageView_ != VK_NULL_HANDLE) {
            vkDestroyImageView(device_, imageView_, nullptr);
            imageView_ = VK_NULL_HANDLE;
        }
        if (loadedFromUnity_ == false && memory_ != VK_NULL_HANDLE) {
            vkFreeMemory(device_, memory_, nullptr);
            memory_ = VK_NULL_HANDLE;
        }
        if (loadedFromUnity_ == false && image_ != VK_NULL_HANDLE) {
            vkDestroyImage(device_, image_, nullptr);
            image_ = VK_NULL_HANDLE;
        }
    }

    void  Image::LoadFromUnity(std::string name, VkImage image, VkFormat format)
    {
        name_ = name;
        image_ = image;
        format_ = format;
        
        // Hanlded exteranlly by Unity3D
        memory_ = VK_NULL_HANDLE;

        loadedFromUnity_ = true;
    }

    VkResult Image::CreateImageView(VkImageViewType viewType, VkFormat format, VkImageSubresourceRange subresourceRange) {
        VkImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewCreateInfo.pNext = nullptr;
        imageViewCreateInfo.viewType = viewType;
        imageViewCreateInfo.format = format;
        imageViewCreateInfo.subresourceRange = subresourceRange;
        imageViewCreateInfo.image = image_;
        imageViewCreateInfo.flags = 0;
        imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };

        return vkCreateImageView(device_, &imageViewCreateInfo, nullptr, &imageView_);
    }

    VkResult Image::CreateSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode) {
        VkSamplerCreateInfo samplerCreateInfo;
        samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCreateInfo.pNext = nullptr;
        samplerCreateInfo.flags = 0;
        samplerCreateInfo.magFilter = magFilter;
        samplerCreateInfo.minFilter = minFilter;
        samplerCreateInfo.mipmapMode = mipmapMode;
        samplerCreateInfo.addressModeU = addressMode;
        samplerCreateInfo.addressModeV = addressMode;
        samplerCreateInfo.addressModeW = addressMode;
        samplerCreateInfo.mipLodBias = 0;
        samplerCreateInfo.anisotropyEnable = VK_FALSE;
        samplerCreateInfo.maxAnisotropy = 1;
        samplerCreateInfo.compareEnable = VK_FALSE;
        samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerCreateInfo.minLod = 0;
        samplerCreateInfo.maxLod = 0;
        samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

        return vkCreateSampler(device_, &samplerCreateInfo, nullptr, &sampler_);
    }

    // getters
    VkFormat Image::GetFormat() const {
        return format_;
    }

    VkImage Image::GetImage() const {
        return image_;
    }

    VkImageView Image::GetImageView() const {
        return imageView_;
    }

    VkSampler Image::GetSampler() const {
        return sampler_;
    }
}