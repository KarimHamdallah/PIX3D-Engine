#include "VulkanFramebuffer.h"
#include <Core/Core.h>

#include <Engine/Engine.hpp>

namespace PIX3D
{
    namespace VK
    {
        void VulkanFramebuffer::Destroy()
        {
            for (size_t i = 0; i < m_attachments.size(); i++)
            {
                if (m_attachments[i] != VK_NULL_HANDLE)
                    vkDestroyImageView(m_device, m_attachments[i], nullptr);
                m_attachments[i] = VK_NULL_HANDLE;
            }

            if (m_Framebuffer != VK_NULL_HANDLE)
            {
                vkDestroyFramebuffer(m_device, m_Framebuffer, nullptr);
                m_Framebuffer = VK_NULL_HANDLE;
            }
        }

        VulkanFramebuffer& VulkanFramebuffer::Init(VkDevice device, VkRenderPass renderPass, uint32_t width, uint32_t height)
        {
            if (m_Framebuffer != VK_NULL_HANDLE)
            {
                Destroy();
            }

            m_attachments.clear();

            m_device = device;
            m_renderPass = renderPass;
            m_width = width;
            m_height = height;
            return *this;
        }

        VulkanFramebuffer& VulkanFramebuffer::AddAttachment(VulkanTexture* texture, uint32_t mipLevel)
        {
            auto CreateImageView = [](VkDevice device, VulkanTexture* texture, uint32_t mipLevel, VkImageView* imageview) -> void
            {
                VkImageViewCreateInfo viewInfo{};
                viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewInfo.image = texture->GetVKImage();
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewInfo.format = texture->GetVKormat();
                viewInfo.subresourceRange.aspectMask = IsDepthFormat(texture->GetVKormat()) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
                viewInfo.subresourceRange.baseMipLevel = mipLevel;
                viewInfo.subresourceRange.levelCount = 1;
                viewInfo.subresourceRange.baseArrayLayer = 0;
                viewInfo.subresourceRange.layerCount = 1;

                if (vkCreateImageView(device, &viewInfo, nullptr, imageview) != VK_SUCCESS)
                    PIX_ASSERT_MSG(false, "Failed to create texture image view!");
            };

            VkImageView view;
            CreateImageView(m_device, texture, mipLevel, &view);

            m_attachments.push_back(view);
            return *this;
        }


        VulkanFramebuffer& VulkanFramebuffer::AddSwapChainAttachment(uint32_t index)
        {

            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            auto CreateImageView = [](VkDevice device, VkImage image, VkFormat format, VkImageView* imageview) -> void
            {
                VkImageViewCreateInfo viewInfo{};
                viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewInfo.image = image;
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewInfo.format = format;
                viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                viewInfo.subresourceRange.baseMipLevel = 0;
                viewInfo.subresourceRange.levelCount = 1;
                viewInfo.subresourceRange.baseArrayLayer = 0;
                viewInfo.subresourceRange.layerCount = 1;

                if (vkCreateImageView(device, &viewInfo, nullptr, imageview) != VK_SUCCESS)
                    PIX_ASSERT_MSG(false, "Failed to create texture image view!");
            };

            VkImageView view;
            CreateImageView(m_device, Context->m_SwapChainImages[index], Context->m_SwapChainSurfaceFormat.format, &view);

            m_attachments.push_back(view);
            return *this;
        }

        VulkanFramebuffer& VulkanFramebuffer::SetLayers(uint32_t layers)
        {
            m_layers = layers;
            return *this;
        }

        void VulkanFramebuffer::Build()
        {
            VkFramebufferCreateInfo frameBufferInfo = {};
            frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            frameBufferInfo.renderPass = m_renderPass;
            frameBufferInfo.attachmentCount = static_cast<uint32_t>(m_attachments.size());
            frameBufferInfo.pAttachments = m_attachments.data();
            frameBufferInfo.width = m_width;
            frameBufferInfo.height = m_height;
            frameBufferInfo.layers = m_layers;

            VkResult result = vkCreateFramebuffer(m_device, &frameBufferInfo, nullptr, &m_Framebuffer);
            PIX_ASSERT_MSG(result == VK_SUCCESS, "Failed to create framebuffer!");
        }
    }
}
