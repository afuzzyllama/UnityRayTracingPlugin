#include "VulkanBuffer.h"

#include <memory>

PixelsForGlory::VulkanBuffer::VulkanBuffer()
    : device_(VK_NULL_HANDLE)
    , buffer_(VK_NULL_HANDLE)
    , memory_(VK_NULL_HANDLE)
    , size_(0)
{}

PixelsForGlory::VulkanBuffer::~VulkanBuffer() {
    this->Destroy();
}

VkResult PixelsForGlory::VulkanBuffer::Create(VkDevice device, VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties) {
    device_ = device;
    
    VkResult result = VK_SUCCESS;

    VkBufferCreateInfo bufferCreateInfo;
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.pNext = nullptr;
    bufferCreateInfo.flags = 0;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = usage;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.queueFamilyIndexCount = 0;
    bufferCreateInfo.pQueueFamilyIndices = nullptr;

    size_ = size;

    result = vkCreateBuffer(device_, &bufferCreateInfo, nullptr, &buffer_);
    if (VK_SUCCESS == result) {
        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(device_, buffer_, &memoryRequirements);

        VkMemoryAllocateInfo memoryAllocateInfo;
        memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocateInfo.pNext = nullptr;
        memoryAllocateInfo.allocationSize = memoryRequirements.size;
        memoryAllocateInfo.memoryTypeIndex = GetMemoryType(physicalDeviceMemoryProperties, memoryRequirements, memoryProperties);

        VkMemoryAllocateFlagsInfo allocationFlags = {};
        allocationFlags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
        allocationFlags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
        if ((usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) == VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
            memoryAllocateInfo.pNext = &allocationFlags;
        }

        result = vkAllocateMemory(device_, &memoryAllocateInfo, nullptr, &memory_);
        if (VK_SUCCESS != result) {
            vkDestroyBuffer(device_, buffer_, nullptr);
            buffer_ = VK_NULL_HANDLE;
            memory_ = VK_NULL_HANDLE;
        }
        else {
            result = vkBindBufferMemory(device_, buffer_, memory_, 0);
            if (VK_SUCCESS != result) {
                vkDestroyBuffer(device_, buffer_, nullptr);
                vkFreeMemory(device_, memory_, nullptr);
                buffer_ = VK_NULL_HANDLE;
                memory_ = VK_NULL_HANDLE;
            }
        }
    }

    return result;
}

void PixelsForGlory::VulkanBuffer::Destroy() {
    if (buffer_) {
        vkDestroyBuffer(device_, buffer_, nullptr);
        buffer_ = VK_NULL_HANDLE;
    }
    if (memory_) {
        vkFreeMemory(device_, memory_, nullptr);
        memory_ = VK_NULL_HANDLE;
    }
}

void* PixelsForGlory::VulkanBuffer::Map(VkDeviceSize size, VkDeviceSize offset) const {
    void* mem = nullptr;

    if (size > size_) {
        size = size_;
    }

    VkResult result = vkMapMemory(device_, memory_, offset, size, 0, &mem);
    if (VK_SUCCESS != result) {
        mem = nullptr;
    }

    return mem;
}
void PixelsForGlory::VulkanBuffer::Unmap() const {
    vkUnmapMemory(device_, memory_);
}

bool PixelsForGlory::VulkanBuffer::UploadData(const void* data, VkDeviceSize size, VkDeviceSize offset) const {
    bool result = false;

    void* mem = this->Map(size, offset);
    if (mem) {
        std::memcpy(mem, data, size);
        this->Unmap();
    }
    return true;
}

VkBuffer PixelsForGlory::VulkanBuffer::GetBuffer() const {
    return buffer_;
}

VkDeviceSize PixelsForGlory::VulkanBuffer::GetSize() const {
    return size_;
}