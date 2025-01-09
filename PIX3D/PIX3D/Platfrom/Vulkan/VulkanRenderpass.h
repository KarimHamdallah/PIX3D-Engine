#pragma once
#include <vulkan/vulkan.h>
#include <vector>

namespace PIX3D
{
    namespace VK
    {
        class VulkanRenderPass
        {
        public:
            VulkanRenderPass& Init(VkDevice device);
            void Destoy();

            VulkanRenderPass& AddColorAttachment(VkFormat format, VkSampleCountFlagBits samples,
                VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
                VkImageLayout initialLayout, VkImageLayout finalLayout);

            VulkanRenderPass& AddDepthAttachment(VkFormat format, VkSampleCountFlagBits samples,
                VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
                VkImageLayout initialLayout, VkImageLayout finalLayout);

            VulkanRenderPass& AddSubpass(VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS);

            VulkanRenderPass& AddDependency(uint32_t srcSubpass, uint32_t dstSubpass, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkDependencyFlags dependencyFlags = 0);

            void Build();

            VkRenderPass GetVKRenderpass() { return m_Renderpass; }

        private:
            VkRenderPass m_Renderpass = VK_NULL_HANDLE;
            VkDevice m_device = VK_NULL_HANDLE;
            std::vector<VkAttachmentDescription> m_attachments;
            std::vector<VkAttachmentReference> m_colorReferences;
            VkAttachmentReference m_depthReference = {};
            std::vector<VkSubpassDescription> m_subpasses;
            std::vector<VkSubpassDependency> m_dependencies;
            bool m_hasDepthAttachment = false;
        };
    }
}
