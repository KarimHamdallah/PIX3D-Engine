#pragma once
#include "../VulkanHdrCubemap.h"

namespace PIX3D
{
    namespace VK
    {
        class VulkanPrefilteredCubemap
        {
        public:
            VulkanPrefilteredCubemap() = default;
            ~VulkanPrefilteredCubemap() = default;

            bool Generate(VulkanHdrCubemap* cubemap, uint32_t cubemapSize, uint32_t mips = 5);
            void Destroy();

        public:
            VkImage m_Image = VK_NULL_HANDLE;
            VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;
            VkImageView m_ImageView = VK_NULL_HANDLE;
            VkSampler m_Sampler = VK_NULL_HANDLE;

            VkFormat m_Format = VK_FORMAT_UNDEFINED;
            uint32_t m_BytePerPixel = 0;
            uint32_t m_Size = 0;
            uint32_t m_MipLevels = 1;
        };
    }
}
