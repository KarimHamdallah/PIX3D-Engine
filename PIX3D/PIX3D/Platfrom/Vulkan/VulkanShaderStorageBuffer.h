#pragma once
#include "VulkanVertexBuffer.h"


namespace PIX3D
{
    namespace VK
    {
        class VulkanShaderStorageBuffer
        {
        public:
            VulkanShaderStorageBuffer() = default;
            ~VulkanShaderStorageBuffer() {}

            void Create(size_t size);
            void UpdateData(void* BufferData, size_t Size);
            void Destroy();

            VkBuffer GetBuffer() const { return m_StorageBuffer.m_Buffer; }
            size_t GetSize() const { return m_Size; }

            void* GetMappedData() { return m_MappedData; }

        private:
            BufferAndMemory CreateBuffer(VkDeviceSize Size,
                VkBufferUsageFlags Usage,
                VkMemoryPropertyFlags Properties,
                VkPhysicalDevice PhysDevice);

            uint32_t FindMemoryType(VkPhysicalDevice PhysDevice,
                uint32_t TypeFilter,
                VkMemoryPropertyFlags Properties);

        private:
            VkDevice m_Device = VK_NULL_HANDLE;
            VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
            BufferAndMemory m_StorageBuffer;
            void* m_MappedData = nullptr;  // For persistent mapping
            size_t m_Size = 0;
        };
    }
}
