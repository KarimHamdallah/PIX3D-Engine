#pragma once
#include <vulkan/vulkan.h>
#include "VulkanTexture.h"
#include <vector>

namespace PIX3D
{
    namespace VK
    {
        struct VulkanFramebufferAttachment
        {
            VkImage image;
            VkImageView view;
            VkFormat format;
            bool ownsView;  // Indicates if this class should cleanup the view
        };

        class VulkanFramebuffer
        {
        public:
            VulkanFramebuffer() = default;
            ~VulkanFramebuffer() {}

            void Destroy();

            VulkanFramebuffer& Init(VkDevice device, VkRenderPass renderPass, uint32_t width, uint32_t height);
            VulkanFramebuffer& AddAttachment(VkImage image, VkFormat format, uint32_t layer, uint32_t mipLevel = 0);
            VulkanFramebuffer& AddAttachment(VulkanTexture* texture, uint32_t mipLevel = 0);
            VulkanFramebuffer& AddSwapChainAttachment(uint32_t index);
            VulkanFramebuffer& SetLayers(uint32_t layers);
            void Build();

            const VkFramebuffer& GetVKFramebuffer() const { return m_Framebuffer; }

            uint32_t GetWidth() { return m_width; }
            uint32_t GetHeight() { return m_height; }

        private:
            VkDevice m_device = VK_NULL_HANDLE;
            VkRenderPass m_renderPass = VK_NULL_HANDLE;
            VkFramebuffer m_Framebuffer = VK_NULL_HANDLE;

            uint32_t m_width = 0;
            uint32_t m_height = 0;
            uint32_t m_layers = 1;

            std::vector<VkImageView> m_attachments;
        };
    }
}
