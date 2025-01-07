#include "VulkanShaderStorageBuffer.h"
#include <Engine/Engine.hpp>
#include "VulkanHelper.h"

namespace PIX3D
{
    namespace VK
    {
        void VulkanShaderStorageBuffer::Create()
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            m_Device = Context->m_Device;
            m_PhysicalDevice = Context->m_PhysDevice.GetSelected().m_physDevice;
        }

        void VulkanShaderStorageBuffer::FillData(const void* BufferData, size_t Size)
        {
            m_Size = Size;

            // Create staging buffer
            BufferAndMemory StagingBuffer = CreateBuffer(
                Size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_PhysicalDevice
            );

            // Map and copy data to staging buffer
            void* Data = nullptr;
            VkResult res = vkMapMemory(m_Device, StagingBuffer.m_Memory, 0, Size, 0, &Data);
            VK_CHECK_RESULT(res, "Failed to map staging buffer memory");

            memcpy(Data, BufferData, Size);
            vkUnmapMemory(m_Device, StagingBuffer.m_Memory);

            // Create device local buffer
            m_StorageBuffer = CreateBuffer(
                Size,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                m_PhysicalDevice
            );

            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            // Copy from staging buffer to device local buffer
            CopyBuffer(Context->m_CommandPool, Context->m_Queue.GetVkQueue(), m_StorageBuffer.m_Buffer, StagingBuffer.m_Buffer, Size);

            // Cleanup staging buffer
            StagingBuffer.Destroy(m_Device);
        }

        void VulkanShaderStorageBuffer::Destroy()
        {
            if (m_Device != VK_NULL_HANDLE) {
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

        void VulkanShaderStorageBuffer::CopyBuffer(VkCommandPool CmdPool,
            VkQueue Queue,
            VkBuffer DstBuffer,
            VkBuffer SrcBuffer,
            VkDeviceSize Size)
        {
            VkCommandBuffer CommandBuffer = VulkanHelper::BeginSingleTimeCommands(m_Device, CmdPool);

            VkBufferCopy CopyRegion{};
            CopyRegion.size = Size;
            vkCmdCopyBuffer(CommandBuffer, SrcBuffer, DstBuffer, 1, &CopyRegion);

            VulkanHelper::EndSingleTimeCommands(m_Device, Queue, CmdPool, CommandBuffer);
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
