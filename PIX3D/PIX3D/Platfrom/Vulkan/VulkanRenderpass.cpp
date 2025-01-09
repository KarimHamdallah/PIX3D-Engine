#include "VulkanRenderPass.h"
#include <Core/Core.h>

namespace PIX3D
{
    namespace VK
    {
        VulkanRenderPass& VulkanRenderPass::Init(VkDevice device)
        {
            m_device = device;
            return *this;
        }

        void VulkanRenderPass::Destoy()
        {
            if (m_Renderpass != VK_NULL_HANDLE)
            {
                vkDestroyRenderPass(m_device, m_Renderpass, nullptr);
                m_Renderpass = VK_NULL_HANDLE;
            }
        }

        VulkanRenderPass& VulkanRenderPass::AddColorAttachment(VkFormat format, VkSampleCountFlagBits samples,
            VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
            VkImageLayout initialLayout, VkImageLayout finalLayout)
        {
            VkAttachmentDescription attachment = {};
            attachment.flags = 0;
            attachment.format = format;
            attachment.samples = samples;
            attachment.loadOp = loadOp;
            attachment.storeOp = storeOp;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = initialLayout;
            attachment.finalLayout = finalLayout;
            m_attachments.push_back(attachment);

            VkAttachmentReference reference = {};
            reference.attachment = static_cast<uint32_t>(m_attachments.size() - 1);
            reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            m_colorReferences.push_back(reference);

            return *this;
        }

        VulkanRenderPass& VulkanRenderPass::AddDepthAttachment(VkFormat format, VkSampleCountFlagBits samples,
            VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp,
            VkImageLayout initialLayout, VkImageLayout finalLayout)
        {
            VkAttachmentDescription attachment = {};
            attachment.flags = 0;
            attachment.format = format;
            attachment.samples = samples;
            attachment.loadOp = loadOp;
            attachment.storeOp = storeOp;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = initialLayout;
            attachment.finalLayout = finalLayout;
            m_attachments.push_back(attachment);

            m_depthReference.attachment = static_cast<uint32_t>(m_attachments.size() - 1);
            m_depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            m_hasDepthAttachment = true;
            return *this;
        }

        VulkanRenderPass& VulkanRenderPass::AddSubpass(VkPipelineBindPoint bindPoint)
        {
            VkSubpassDescription subpass = {};
            subpass.pipelineBindPoint = bindPoint;
            subpass.colorAttachmentCount = static_cast<uint32_t>(m_colorReferences.size());
            subpass.pColorAttachments = m_colorReferences.data();

            if (m_hasDepthAttachment)
            {
                subpass.pDepthStencilAttachment = &m_depthReference;
            }

            m_subpasses.push_back(subpass);
            return *this;
        }

        VulkanRenderPass& VulkanRenderPass::AddDependency(uint32_t srcSubpass, uint32_t dstSubpass, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
            VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkDependencyFlags dependencyFlags)
        {
            VkSubpassDependency dependency = {};
            dependency.srcSubpass = srcSubpass;
            dependency.dstSubpass = dstSubpass;
            dependency.srcStageMask = srcStageMask;
            dependency.dstStageMask = dstStageMask;
            dependency.srcAccessMask = srcAccessMask;
            dependency.dstAccessMask = dstAccessMask;
            dependency.dependencyFlags = dependencyFlags;
            m_dependencies.push_back(dependency);
            return *this;
        }

        void VulkanRenderPass::Build()
        {
            VkRenderPassCreateInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = static_cast<uint32_t>(m_attachments.size());
            renderPassInfo.pAttachments = m_attachments.data();
            renderPassInfo.subpassCount = static_cast<uint32_t>(m_subpasses.size());
            renderPassInfo.pSubpasses = m_subpasses.data();
            renderPassInfo.dependencyCount = static_cast<uint32_t>(m_dependencies.size());
            renderPassInfo.pDependencies = m_dependencies.data();

            VkResult result = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_Renderpass);
            PIX_ASSERT_MSG(result == VK_SUCCESS, "Failed to create render pass!");
        }
    }
}
