#pragma once
#include "VulkanVertexBuffer.h"

namespace PIX3D
{
    namespace VK
    {
        class VulkanUniformBuffer
        {
        public:
            VulkanUniformBuffer() = default;
            ~VulkanUniformBuffer() {}

            void Create();
            void FillData(const void* BufferData, size_t Size);
            void UpdateData(const void* BufferData, size_t Size);  // New method for dynamic updates
            void Destroy();

            VkBuffer GetBuffer() const { return m_UniformBuffer.m_Buffer; }
            size_t GetSize() const { return m_Size; }

        private:
            BufferAndMemory CreateBuffer(VkDeviceSize Size,
                VkBufferUsageFlags Usage,
                VkMemoryPropertyFlags Properties,
                VkPhysicalDevice PhysDevice);

            void CopyBuffer(VkCommandPool CmdPool,
                VkQueue Queue,
                VkBuffer DstBuffer,
                VkBuffer SrcBuffer,
                VkDeviceSize Size);

            uint32_t FindMemoryType(VkPhysicalDevice PhysDevice,
                uint32_t TypeFilter,
                VkMemoryPropertyFlags Properties);

        private:
            VkDevice m_Device = VK_NULL_HANDLE;
            VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
            BufferAndMemory m_UniformBuffer;
            void* m_MappedData = nullptr;  // For persistent mapping
            size_t m_Size = 0;
        };
    }
}
