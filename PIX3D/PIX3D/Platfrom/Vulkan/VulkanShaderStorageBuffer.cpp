#include "VulkanShaderStorageBuffer.h"
#include <Engine/Engine.hpp>
#include "VulkanHelper.h"
#include <Core/Core.h>

namespace PIX3D
{
    namespace VK
    {
        void VulkanShaderStorageBuffer::Create(size_t size)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            m_Device = Context->m_Device;
            m_PhysicalDevice = Context->m_PhysDevice.GetSelected().m_physDevice;

            m_Size = size;

            // Create storage buffer - directly host visible for frequent updates
            m_StorageBuffer = CreateBuffer(
                size,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_PhysicalDevice
            );

            // Map memory persistently
            VkResult res = vkMapMemory(m_Device, m_StorageBuffer.m_Memory, 0, size, 0, &m_MappedData);
            VK_CHECK_RESULT(res, "Failed to map storage buffer memory");
        }

        void VulkanShaderStorageBuffer::UpdateData(void* BufferData, size_t Size)
        {
            PIX_ASSERT(Size <= m_Size);
            PIX_ASSERT(m_MappedData != nullptr);

            memcpy(m_MappedData, BufferData, Size);
        }

        void VulkanShaderStorageBuffer::Destroy()
        {
            if (m_Device != VK_NULL_HANDLE)
            {
                if (m_MappedData)
                {
                    vkUnmapMemory(m_Device, m_StorageBuffer.m_Memory);
                    m_MappedData = nullptr;
                }

                m_StorageBuffer.Destroy(m_Device);
                m_Device = VK_NULL_HANDLE;
            }
        }

        BufferAndMemory VulkanShaderStorageBuffer::CreateBuffer(VkDeviceSize Size,
            VkBufferUsageFlags Usage,
            VkMemoryPropertyFlags Properties,
            VkPhysicalDevice PhysDevice)
        {
            BufferAndMemory Result;

            VkBufferCreateInfo BufferInfo{};
            BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            BufferInfo.size = Size;
            BufferInfo.usage = Usage;
            BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VkResult res = vkCreateBuffer(m_Device, &BufferInfo, nullptr, &Result.m_Buffer);
            VK_CHECK_RESULT(res, "Failed to create buffer");

            VkMemoryRequirements MemRequirements;
            vkGetBufferMemoryRequirements(m_Device, Result.m_Buffer, &MemRequirements);

            VkMemoryAllocateInfo AllocInfo{};
            AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            AllocInfo.allocationSize = MemRequirements.size;
            AllocInfo.memoryTypeIndex = FindMemoryType(PhysDevice,
                MemRequirements.memoryTypeBits,
                Properties);

            res = vkAllocateMemory(m_Device, &AllocInfo, nullptr, &Result.m_Memory);
            VK_CHECK_RESULT(res, "Failed to allocate buffer memory");

            Result.m_AllocationSize = AllocInfo.allocationSize;

            vkBindBufferMemory(m_Device, Result.m_Buffer, Result.m_Memory, 0);

            return Result;
        }

        uint32_t VulkanShaderStorageBuffer::FindMemoryType(VkPhysicalDevice PhysDevice,
            uint32_t TypeFilter,
            VkMemoryPropertyFlags Properties)
        {
            VkPhysicalDeviceMemoryProperties MemProperties;
            vkGetPhysicalDeviceMemoryProperties(PhysDevice, &MemProperties);

            for (uint32_t i = 0; i < MemProperties.memoryTypeCount; i++)
            {
                if ((TypeFilter & (1 << i)) &&
                    (MemProperties.memoryTypes[i].propertyFlags & Properties) == Properties) {
                    return i;
                }
            }

            PIX_ASSERT_MSG(false, "Failed to find suitable memory type!");
            return 0;
        }
    }
}
