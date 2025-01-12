#include "VulkanBrdfLutTexture.h"
#include <Engine/Engine.hpp>
#include "../VulkanHelper.h"
#include "../VulkanStaticMeshFactory.h"

namespace PIX3D
{
    namespace VK
    {
        bool VulkanBrdfLutTexture::Generate(uint32_t size)
        {
            m_Size = size;

            auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

            // Create 2D Texture
            {
                auto usage =
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_SAMPLED_BIT;

                VkImageCreateInfo imageInfo{};
                imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
                imageInfo.imageType = VK_IMAGE_TYPE_2D;
                imageInfo.extent.width = m_Size;
                imageInfo.extent.height = m_Size;
                imageInfo.extent.depth = 1;
                imageInfo.mipLevels = 1;
                imageInfo.arrayLayers = 1;
                imageInfo.format = m_Format;
                imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
                imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                imageInfo.usage = usage;
                imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
                imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                if (vkCreateImage(Context->m_Device, &imageInfo, nullptr, &m_Image) != VK_SUCCESS)
                    return false;

                VkMemoryRequirements memRequirements;
                vkGetImageMemoryRequirements(Context->m_Device, m_Image, &memRequirements);

                VkMemoryAllocateInfo allocInfo{};
                allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                allocInfo.allocationSize = memRequirements.size;
                allocInfo.memoryTypeIndex = FindMemoryType(
                    Context->m_PhysDevice.GetSelected().m_physDevice,
                    memRequirements.memoryTypeBits,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                );

                if (vkAllocateMemory(Context->m_Device, &allocInfo, nullptr, &m_ImageMemory) != VK_SUCCESS)
                    return false;

                vkBindImageMemory(Context->m_Device, m_Image, m_ImageMemory, 0);

                // Create image view
                VkImageViewCreateInfo viewInfo{};
                viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                viewInfo.image = m_Image;
                viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                viewInfo.format = m_Format;
                viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                viewInfo.subresourceRange.baseMipLevel = 0;
                viewInfo.subresourceRange.levelCount = 1;
                viewInfo.subresourceRange.baseArrayLayer = 0;
                viewInfo.subresourceRange.layerCount = 1;

                if (vkCreateImageView(Context->m_Device, &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS)
                    return false;

                // Create sampler
                VkSamplerCreateInfo samplerInfo{};
                samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                samplerInfo.magFilter = VK_FILTER_LINEAR;
                samplerInfo.minFilter = VK_FILTER_LINEAR;
                samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
                samplerInfo.anisotropyEnable = VK_FALSE;
                samplerInfo.maxAnisotropy = 1.0f;
                samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
                samplerInfo.unnormalizedCoordinates = VK_FALSE;
                samplerInfo.compareEnable = VK_FALSE;
                samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
                samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

                if (vkCreateSampler(Context->m_Device, &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS)
                    return false;
            }

            // Render BRDF LUT
            {
                VulkanShader brdfShader;
                brdfShader.LoadFromFile("../PIX3D/res/vk shaders/brdf_lut.vert", "../PIX3D/res/vk shaders/brdf_lut.frag");

                // Create renderpass
                VulkanRenderPass renderpass;
                renderpass
                    .Init(Context->m_Device)
                    .AddColorAttachment(
                        m_Format,
                        VK_SAMPLE_COUNT_1_BIT,
                        VK_ATTACHMENT_LOAD_OP_CLEAR,
                        VK_ATTACHMENT_STORE_OP_STORE,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                    .AddSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
                    .AddDependency(
                        VK_SUBPASS_EXTERNAL,
                        0,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_ACCESS_SHADER_READ_BIT,
                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                        VK_DEPENDENCY_BY_REGION_BIT
                    )
                    .AddDependency(
                        0,
                        VK_SUBPASS_EXTERNAL,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                        VK_ACCESS_SHADER_READ_BIT,
                        VK_DEPENDENCY_BY_REGION_BIT
                    )
                    .Build();

                // Create framebuffer
                VulkanFramebuffer framebuffer;
                framebuffer
                    .Init(Context->m_Device, renderpass.GetVKRenderpass(), m_Size, m_Size)
                    .AddAttachment(m_Image, m_Format, 0)
                    .Build();

                // Create pipeline layout
                VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
                {
                    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
                    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

                    if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
                        return false;
                }

                VulkanStaticMeshData quadMesh = VulkanStaticMeshGenerator::GenerateQuad();

                auto bindingDesc = quadMesh.VertexLayout.GetBindingDescription();
                auto attributeDesc = quadMesh.VertexLayout.GetAttributeDescriptions();

                // Create graphics pipeline
                VulkanGraphicsPipeline graphicsPipeline;
                graphicsPipeline.Init(Context->m_Device, renderpass.GetVKRenderpass())
                    .AddShaderStages(brdfShader.GetVertexShader(), brdfShader.GetFragmentShader())
                    .AddVertexInputState(&bindingDesc, attributeDesc.data(), 1, attributeDesc.size())
                    .AddViewportState(m_Size, m_Size)
                    .AddInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
                    .AddRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                    .AddMultisampleState(VK_SAMPLE_COUNT_1_BIT)
                    .AddDepthStencilState(false, false)
                    .AddColorBlendState(false, 1)
                    .SetPipelineLayout(pipelineLayout)
                    .Build();

                // Record and submit command buffer
                VkCommandBuffer commandBuffer = VulkanHelper::BeginSingleTimeCommands(
                    Context->m_Device,
                    Context->m_CommandPool
                );

                VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

                VkRenderPassBeginInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassInfo.renderPass = renderpass.GetVKRenderpass();
                renderPassInfo.framebuffer = framebuffer.GetVKFramebuffer();
                renderPassInfo.renderArea = { {0, 0}, {m_Size, m_Size} };
                renderPassInfo.clearValueCount = 1;
                renderPassInfo.pClearValues = &clearValue;

                vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.GetVkPipeline());

                VkViewport viewport{};
                viewport.x = 0.0f;
                viewport.y = (float)m_Size;
                viewport.width = (float)m_Size;
                viewport.height = -(float)m_Size;
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;

                VkRect2D scissor{};
                scissor.offset = { 0, 0 };
                scissor.extent = { m_Size, m_Size };

                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

                VkBuffer vertexBuffers[] = { quadMesh.VertexBuffer.GetBuffer() };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(commandBuffer, quadMesh.IndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(commandBuffer, quadMesh.IndicesCount, 1, 0, 0, 0);

                vkCmdEndRenderPass(commandBuffer);

                VulkanHelper::EndSingleTimeCommands(
                    Context->m_Device,
                    Context->m_Queue.m_Queue,
                    Context->m_CommandPool,
                    commandBuffer
                );

                // Cleanup
                
            }

            return true;
        }

        void VulkanBrdfLutTexture::Destroy()
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            if (Context->m_Device != VK_NULL_HANDLE)
            {
                if (m_Sampler != VK_NULL_HANDLE)
                {
                    vkDestroySampler(Context->m_Device, m_Sampler, nullptr);
                    m_Sampler = VK_NULL_HANDLE;
                }
                if (m_ImageView != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(Context->m_Device, m_ImageView, nullptr);
                    m_ImageView = VK_NULL_HANDLE;
                }
                if (m_Image != VK_NULL_HANDLE)
                {
                    vkDestroyImage(Context->m_Device, m_Image, nullptr);
                    m_Image = VK_NULL_HANDLE;
                }
                if (m_ImageMemory != VK_NULL_HANDLE)
                {
                    vkFreeMemory(Context->m_Device, m_ImageMemory, nullptr);
                    m_ImageMemory = VK_NULL_HANDLE;
                }
            }
        }


        uint32_t VulkanBrdfLutTexture::FindMemoryType(VkPhysicalDevice PhysDevice,
            uint32_t TypeFilter,
            VkMemoryPropertyFlags Properties)
        {
            VkPhysicalDeviceMemoryProperties MemProperties;
            vkGetPhysicalDeviceMemoryProperties(PhysDevice, &MemProperties);

            for (uint32_t i = 0; i < MemProperties.memoryTypeCount; i++)
            {
                if ((TypeFilter & (1 << i)) &&
                    (MemProperties.memoryTypes[i].propertyFlags & Properties) == Properties) {
                    return i;
                }
            }

            PIX_ASSERT_MSG(false, "Failed to find suitable memory type!");
            return 0;
        }
    }
}
