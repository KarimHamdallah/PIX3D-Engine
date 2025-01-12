#pragma once
#include "VulkanTexture.h"

namespace PIX3D
{
    namespace VK
    {

        class VulkanCubemapHelper
        {
        public:
            static void CreateImage
            (
                uint32_t width, uint32_t height,
                VkFormat format,
                uint32_t miplevels,
                VkImageTiling tiling,
                VkImageUsageFlags usage,
                VkMemoryPropertyFlags properties,
                VkImage& Image,
                VkDeviceMemory& ImageMemory
            );

            static void CreateImageView
            (
                VkImage Image,
                VkFormat format,
                VkImageView& ImageView
            );

            static void CreateSampler
            (
                float maxLod,
                VkSampler& Sampler
            );

            static uint32_t FindMemoryType
            (
                VkPhysicalDevice PhysDevice,
                uint32_t TypeFilter,
                VkMemoryPropertyFlags Properties
            );

            static void TransitionImageLayout
            (
                VkImage Image,
                uint32_t MipLevels,
                VkImageLayout OldLayout,
                VkImageLayout NewLayout,
                VkCommandBuffer ExistingCommandBuffer
            );
        };








        class VulkanHdrCubemap
        {
        public:
            VulkanHdrCubemap() = default;
            ~VulkanHdrCubemap() = default;

            bool LoadHdrToCubemapGPU(const std::filesystem::path& hdrPath, uint32_t cubemapSize, uint32_t mips = 1);
            void Destroy();

        public:
            VulkanTexture* m_EquirectangularMap = nullptr;

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
