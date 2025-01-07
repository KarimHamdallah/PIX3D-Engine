#pragma once
#include <vulkan/vulkan.h>

namespace PIX3D
{
    namespace VK
    {
        struct BufferAndMemory
        {
            VkBuffer m_Buffer = VK_NULL_HANDLE;
            VkDeviceMemory m_Memory = VK_NULL_HANDLE;
            VkDeviceSize m_AllocationSize = 0;

            void Destroy(VkDevice Device);
        };

        class VulkanVertexBuffer
        {
        public:
            VulkanVertexBuffer() = default;
            ~VulkanVertexBuffer() {}

            void Create();
            void FillData(const void* VertexData, size_t Size);
            void Destroy();

            VkBuffer GetBuffer() const { return m_VertexBuffer.m_Buffer; }
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
            BufferAndMemory m_VertexBuffer;
            size_t m_Size = 0;
        };
    }
}
