#pragma once

#include "../../vulkan.h"

#include "IResource.h"

namespace PixelsForGlory::Vulkan
{
    /// <summary>
    /// Represents a Vulkan image resource
    /// </summary>
    class Image : public IResource {
    public:
        static void UpdateImageBarrier(
            VkCommandBuffer commandBuffer,
            VkImage image,
            VkImageSubresourceRange& subresourceRange,
            VkAccessFlags srcAccessMask,
            VkAccessFlags dstAccessMask,
            VkImageLayout oldLayout,
            VkImageLayout newLayout)
        {
            VkImageMemoryBarrier imageMemoryBarrier = {};
            imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageMemoryBarrier.srcAccessMask = srcAccessMask;
            imageMemoryBarrier.dstAccessMask = dstAccessMask;
            imageMemoryBarrier.oldLayout = oldLayout;
            imageMemoryBarrier.newLayout = newLayout;
            imageMemoryBarrier.image = image;
            imageMemoryBarrier.subresourceRange = subresourceRange;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
        }


        Image();
        Image(VkDevice device, VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties);
        ~Image();

        /// <summary>
        /// Create image
        /// </summary>
        /// <param name="imageType"></param>
        /// <param name="format"></param>
        /// <param name="extent"></param>
        /// <param name="tiling"></param>
        /// <param name="usage"></param>
        /// <param name="memoryProperties"></param>
        /// <returns></returns>
        VkResult Create(std::string name,
            VkImageType imageType,
            VkFormat format,
            VkExtent3D extent,
            VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkMemoryPropertyFlags memoryProperties);

        /// <summary>
        /// Destroy image
        /// </summary>
        virtual void Destroy();

        /// <summary>
        /// Load image from file with passed information
        /// </summary>
        /// <param name="fileName"></param>
        /// <param name="format"></param>
        /// <param name="commandBuffer"></param>
        /// <param name="transferQueue"></param>
        /// <returns></returns>
        void LoadFromUnity(std::string name, VkImage image, VkFormat format);

        /// <summary>
        /// Create image view
        /// </summary>
        /// <param name="viewType"></param>
        /// <param name="format"></param>
        /// <param name="subresourceRange"></param>
        /// <returns></returns>
        VkResult CreateImageView(VkImageViewType viewType, VkFormat format, VkImageSubresourceRange subresourceRange);

        /// <summary>
        /// Create sampler
        /// </summary>
        /// <param name="magFilter"></param>
        /// <param name="minFilter"></param>
        /// <param name="mipmapMode"></param>
        /// <param name="addressMode"></param>
        /// <returns></returns>
        VkResult CreateSampler(VkFilter magFilter, VkFilter minFilter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode);

        // getters
        VkFormat    GetFormat() const;
        VkImage     GetImage() const;
        VkImageView GetImageView() const;
        VkSampler   GetSampler() const;

    private:
        VkDevice                            device_;
        VkPhysicalDeviceMemoryProperties    physicalDeviceMemoryProperties_;
        VkFormat                            format_;
        VkImage                             image_;
        VkDeviceMemory                      memory_;
        VkImageView                         imageView_;
        VkSampler                           sampler_;

        std::string name_;
        bool loadedFromUnity_;
    };
}