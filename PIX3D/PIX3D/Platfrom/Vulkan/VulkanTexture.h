#pragma once
#include <vulkan/vulkan.h>
#include <filesystem>
#include <vector>
#include <Asset/Asset.h>

namespace PIX3D
{
    namespace VK
    {

        struct VulkanImageAndMemory
        {
            VkImage Image = VK_NULL_HANDLE;
            VkDeviceMemory Memory = VK_NULL_HANDLE;
        };

        struct VulkanBufferAndMemory
        {
            VkBuffer Buffer = VK_NULL_HANDLE;
            VkDeviceMemory Memory = VK_NULL_HANDLE;
            VkDeviceSize AllocationSize = 0;

            void Destroy(VkDevice device)
            {
                if (Buffer != VK_NULL_HANDLE)
                    vkDestroyBuffer(device, Buffer, nullptr);
                if (Memory != VK_NULL_HANDLE)
                    vkFreeMemory(device, Memory, nullptr);
            }
        };

        class VulkanTextureHelper
        {
        public:

            static void TransitionImageLayout(VkImage Image, VkFormat Format, uint32_t base_mip, uint32_t mip_count, VkImageLayout OldLayout, VkImageLayout NewLayout, VkCommandBuffer ExistingCommandBuffer = VK_NULL_HANDLE);

            static void CopyTextureWithAllMips(VkImage src_image, VkFormat src_format, VkImage dst_image, VkFormat dst_format, uint32_t width, uint32_t height, uint32_t mip_count, VkCommandBuffer existing_commandbuffer);

            static VulkanBufferAndMemory CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);

            static VulkanImageAndMemory CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, uint32_t mip_count = 1);

            static void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

            static VkImageView CreateImageView(VkImage image, VkFormat format, uint32_t base_mip = 0, uint32_t mip_count = 1);
            
            static VkSampler CreateSampler(float mip_count = 1, bool clamp_to_edge = false);

            static uint32_t FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties);

            static uint32_t CalculateMipLevels(uint32_t width, uint32_t height);
        };

        class VulkanTexture : public PIX3D::Asset
        {
        public:
            VulkanTexture() = default;
            ~VulkanTexture() = default;

            bool LoadFromFile(const std::filesystem::path& FilePath, bool gen_mips = true, bool IsSRGB = false, bool clamp_to_edge = false);

            bool LoadFromHDRFile(const std::filesystem::path& FilePath, VkFormat  format = VK_FORMAT_R32G32B32A32_SFLOAT, uint32_t miplevels = 1);
            
            bool LoadFromData(void* Data, uint32_t Width, uint32_t Height, VkFormat Format);
            
            bool CreateColorAttachment(uint32_t Width, uint32_t Height, VkFormat Format, uint32_t MipLevels = 1, bool ClampToEdge = true);
            
            void GenerateMipmaps(VkCommandBuffer commandBuffer);
            
            void Destroy();


            std::filesystem::path GetPath() { return m_Path; }

            VkImageView GetImageView() const { return m_ImageView; }
            VkSampler GetSampler() const { return m_Sampler; }
            uint32_t GetWidth() const { return m_Width; }
            uint32_t GetHeight() const { return m_Height; }
            VkFormat GetFormat() const { return m_Format; }
            VkImage GetImage() const { return m_Image; }
            uint32_t GetMipLevels() { return m_MipLevels; }
            VkDescriptorSet GetImGuiDescriptorSet();

        private:
            VkImage m_Image = VK_NULL_HANDLE;
            VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;
            VkImageView m_ImageView = VK_NULL_HANDLE;
            VkSampler m_Sampler = VK_NULL_HANDLE;

            uint32_t m_Width = 0;
            uint32_t m_Height = 0;
            VkFormat m_Format = VK_FORMAT_R8G8B8A8_UNORM;
            uint32_t m_MipLevels = 1;
            bool m_SamplerClampToEdge = false;
            bool m_IsSRGB = false;
        public:
            VkDescriptorSet m_ImGuiDescriptorset = nullptr;
        };
    }
}
