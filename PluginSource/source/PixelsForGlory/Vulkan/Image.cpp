#include "Image.h"

#include "Buffer.h"

#define STB_IMAGE_IMPLEMENTATION
// excluding old and unuseful formats
#define STBI_NO_PSD
#define STBI_NO_GIF
#define STBI_NO_PIC
#define STBI_NO_PNM

#include "stb_image.h"

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
    {}

    Image::Image(VkDevice device, VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties)
        : device_(device)
        , physicalDeviceMemoryProperties_(physicalDeviceMemoryProperties)
        , format_(VK_FORMAT_B8G8R8A8_UNORM)
        , image_(VK_NULL_HANDLE)
        , memory_(VK_NULL_HANDLE)
        , imageView_(VK_NULL_HANDLE)
        , sampler_(VK_NULL_HANDLE)
    {}

    Image::~Image()
    {
        this->Destroy();
    }

    VkResult Image::Create(VkImageType imageType,
        VkFormat format,
        VkExtent3D extent,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags memoryProperties) {
        VkResult result = VK_SUCCESS;

        assert(image_ == VK_NULL_HANDLE);

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
        if (sampler_) {
            vkDestroySampler(device_, sampler_, nullptr);
            sampler_ = VK_NULL_HANDLE;
        }
        if (imageView_) {
            vkDestroyImageView(device_, imageView_, nullptr);
            imageView_ = VK_NULL_HANDLE;
        }
        if (memory_) {
            vkFreeMemory(device_, memory_, nullptr);
            memory_ = VK_NULL_HANDLE;
        }
        if (image_) {
            vkDestroyImage(device_, image_, nullptr);
            image_ = VK_NULL_HANDLE;
        }
    }

    bool Image::Load(const char* fileName, VkFormat format, VkCommandBuffer commandBuffer, VkQueue transferQueue) {
        int width, height, channels;
        bool textureHDR = false;
        stbi_uc* imageData = nullptr;

        std::string fileNameString(fileName);
        const std::string extension = fileNameString.substr(fileNameString.length() - 3);

        if (extension == "hdr") {
            textureHDR = true;
            imageData = reinterpret_cast<stbi_uc*>(stbi_loadf(fileName, &width, &height, &channels, STBI_rgb_alpha));
        }
        else {
            imageData = stbi_load(fileName, &width, &height, &channels, STBI_rgb_alpha);
        }

        if (imageData) {
            const int bpp = textureHDR ? sizeof(float[4]) : sizeof(uint8_t[4]);
            VkDeviceSize imageSize = static_cast<VkDeviceSize>(static_cast<uint64_t>(width) * static_cast<uint64_t>(height) * static_cast<uint64_t>(bpp));

            Vulkan::Buffer stagingBuffer;
            VkResult error = stagingBuffer.Create(device_, physicalDeviceMemoryProperties_, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, Vulkan::Buffer::kDefaultMemoryPropertyFlags);
            if (VK_SUCCESS == error && stagingBuffer.UploadData(imageData, imageSize)) {
                stbi_image_free(imageData);

                VkExtent3D imageExtent{
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height),
                    1
                };

                const VkFormat fmt = textureHDR ? VK_FORMAT_R32G32B32A32_SFLOAT : format;

                error = this->Create(VK_IMAGE_TYPE_2D, fmt, imageExtent, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
                if (VK_SUCCESS != error) {
                    return false;
                }

                VkCommandBufferBeginInfo beginInfo = {};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
                beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

                error = vkBeginCommandBuffer(commandBuffer, &beginInfo);

                VkImageMemoryBarrier barrier = {};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = image_;
                barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

                vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

                VkBufferImageCopy region = {};
                region.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
                region.imageExtent = imageExtent;

                vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.GetBuffer(), image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

                vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

                error = vkEndCommandBuffer(commandBuffer);
                if (VK_SUCCESS != error) {
                    return false;
                }

                VkSubmitInfo submitInfo = {};
                submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers = &commandBuffer;

                error = vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
                if (VK_SUCCESS != error) {
                    return false;
                }

                error = vkQueueWaitIdle(transferQueue);
                if (VK_SUCCESS != error) {
                    return false;
                }

                // Loaded an image
                return true;
            }
            else {
                stbi_image_free(imageData);

                // Didn't load an image
                return false;
            }
        }

        // Didn't load an image
        return false;
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