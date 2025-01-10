#pragma once
#include "VulkanVertexBuffer.h"
#include <filesystem>

namespace PIX3D
{
    namespace VK
    {
        enum class TextureFormat
        {
            R8,
            RG8,
            RGB8,
            RGBA8,
            RGBA8_SRGB,
            RGB16F,
            RGBA16F,
            RGBA32F,

            DEPTH16,
            DEPTH24,
            DEPTH32,
            DEPTH24_STENCIL8,
            DEPTH32_STENCIL8
        };

        class VulkanTexture
        {
        public:
            VulkanTexture() = default;
            ~VulkanTexture() { Destroy(); }

            void Create(uint32_t miplevels = 1);
            bool LoadFromFile(const std::filesystem::path& FilePath, bool IsSRGB = false);
            bool LoadFromData(void* Data, uint32_t Width, uint32_t Height, TextureFormat Format);
            bool CreateColorAttachment(uint32_t Width, uint32_t Height, TextureFormat Format, uint32_t MipLevels = 1);
            void GenerateMipmaps(VkCommandBuffer commandBuffer);
            void Destroy();

            VkImageView GetImageView() const { return m_ImageView; }
            VkSampler GetSampler() const { return m_Sampler; }
            uint32_t GetWidth() const { return m_Width; }
            uint32_t GetHeight() const { return m_Height; }
            VkFormat GetVKormat() const { return GetVulkanFormat(m_Format); }
            VkImage GetVKImage() const { return m_Image; }

            void TransitionImageLayout(VkFormat Format,
                VkImageLayout OldLayout, VkImageLayout NewLayout, VkCommandBuffer ExistingCommandBuffer = VK_NULL_HANDLE);

        private:
            void CreateImage(uint32_t Width, uint32_t Height, VkFormat Format, VkImageTiling Tiling,
                VkImageUsageFlags Usage, VkMemoryPropertyFlags Properties);
            
            void CreateImageView(VkFormat Format);
            void CreateSampler(float MaxLod = 0.0f);

            void CopyBufferToImage(VkBuffer Buffer, uint32_t Width, uint32_t Height);
            VkFormat GetVulkanFormat(TextureFormat Format) const;
            uint32_t GetBytesPerPixel(TextureFormat Format);

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
            VkQueue m_Queue = VK_NULL_HANDLE;
            VkCommandPool m_CmdPool = VK_NULL_HANDLE;

            VkImage m_Image = VK_NULL_HANDLE;
            VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;
            VkImageView m_ImageView = VK_NULL_HANDLE;
            VkSampler m_Sampler = VK_NULL_HANDLE;

            uint32_t m_Width = 0;
            uint32_t m_Height = 0;
            TextureFormat m_Format = TextureFormat::RGBA8;
            uint32_t m_MipLevels = 1;
        };
    }
}
