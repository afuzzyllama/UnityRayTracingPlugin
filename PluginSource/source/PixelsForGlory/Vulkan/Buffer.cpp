#include "Buffer.h"

#include <memory>

#include "../Debug.h"

namespace PixelsForGlory::Vulkan
{

    Buffer::Buffer()
        : device_(VK_NULL_HANDLE)
        , buffer_(VK_NULL_HANDLE)
        , memory_(VK_NULL_HANDLE)
        , size_(0)
        , name_("[NOT CREATED]")
    {}

    Buffer::~Buffer() {
    }

    VkResult Buffer::Create(std::string name, VkDevice device, VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties) {
        name_ = name;
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
        VK_CHECK("vkCreateBuffer", result);
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
            VK_CHECK("vkAllocateMemory", result);
                if (VK_SUCCESS != result) {
                    vkDestroyBuffer(device_, buffer_, nullptr);
                    buffer_ = VK_NULL_HANDLE;
                    memory_ = VK_NULL_HANDLE;
                }
                else {
                    result = vkBindBufferMemory(device_, buffer_, memory_, 0);
                    VK_CHECK("vkBindBufferMemory", result);
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

    void Buffer::Destroy() {
        PFG_EDITORLOG("Destroying: " + name_);
        if (device_ == VK_NULL_HANDLE)
        {
            // No device set, nothing was created
            return;
        }

        if (buffer_ != VK_NULL_HANDLE) {
            vkDestroyBuffer(device_, buffer_, nullptr);
            buffer_ = VK_NULL_HANDLE;
        }
        if (memory_ != VK_NULL_HANDLE) {
            vkFreeMemory(device_, memory_, nullptr);
            memory_ = VK_NULL_HANDLE;
        }
    }

    void* Buffer::Map(VkDeviceSize size, VkDeviceSize offset) const {
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
    void Buffer::Unmap() const {
        vkUnmapMemory(device_, memory_);
    }

    bool Buffer::UploadData(const void* data, VkDeviceSize size, VkDeviceSize offset) const {
        bool result = false;

        void* mem = this->Map(size, offset);
        if (mem) {
            std::memcpy(mem, data, size);
            this->Unmap();
        }
        return true;
    }

    VkBuffer Buffer::GetBuffer() const {
        return buffer_;
    }

    VkDeviceSize Buffer::GetSize() const {
        return size_;
    }

    VkDeviceOrHostAddressKHR Buffer::GetBufferDeviceAddress() const {
        VkBufferDeviceAddressInfoKHR info = {
            VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            nullptr,
            buffer_
        };

        VkDeviceOrHostAddressKHR result;
        result.deviceAddress = vkGetBufferDeviceAddressKHR(device_, &info);

        return result;
    }

    VkDeviceOrHostAddressConstKHR Buffer::GetBufferDeviceAddressConst() const {
        VkDeviceOrHostAddressKHR address = GetBufferDeviceAddress();

        VkDeviceOrHostAddressConstKHR result;
        result.deviceAddress = address.deviceAddress;

        return result;
    }
}