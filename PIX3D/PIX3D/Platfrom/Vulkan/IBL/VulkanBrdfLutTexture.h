#pragma once
#include <vulkan/vulkan.h>

namespace PIX3D
{
    namespace VK
    {
        class VulkanBrdfLutTexture
        {
        public:
            bool Generate(uint32_t size);
            void Destroy();

            VkImage GetImage() const { return m_Image; }
            VkImageView GetImageView() const { return m_ImageView; }
            VkSampler GetSampler() const { return m_Sampler; }
            uint32_t GetSize() const { return m_Size; }

        private:
            uint32_t FindMemoryType(VkPhysicalDevice PhysDevice,
                    uint32_t TypeFilter,
                    VkMemoryPropertyFlags Properties);
        private:
            VkImage m_Image = VK_NULL_HANDLE;
            VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;
            VkImageView m_ImageView = VK_NULL_HANDLE;
            VkSampler m_Sampler = VK_NULL_HANDLE;

            uint32_t m_Size = 0;
            VkFormat m_Format = VK_FORMAT_R16G16_SFLOAT;
            uint32_t m_BytePerPixel = 4;
        };
    }
}
