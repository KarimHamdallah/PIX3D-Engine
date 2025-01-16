#include "VulkanBloomPass.h"
#include <Engine/Engine.hpp>

namespace PIX3D
{
    namespace VK
    {
        void VulkanBloomPass::Init(uint32_t width, uint32_t height)
        {
            m_Width = width;
            m_Height = height;

            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            // Create two textures for ping-pong blurring with mip levels
            m_ColorAttachments[HORIZONTAL_BLUR_BUFFER_INDEX] = new VulkanTexture();
            m_ColorAttachments[HORIZONTAL_BLUR_BUFFER_INDEX]->CreateColorAttachment(width, height, VK_FORMAT_R16G16B16A16_SFLOAT, BLUR_DOWN_SAMPLES);

            VulkanTextureHelper::TransitionImageLayout
            (
                m_ColorAttachments[HORIZONTAL_BLUR_BUFFER_INDEX]->GetImage(),
                m_ColorAttachments[HORIZONTAL_BLUR_BUFFER_INDEX]->GetFormat(),
                0, BLUR_DOWN_SAMPLES,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );

            m_ColorAttachments[VERTICAL_BLUR_BUFFER_INDEX] = new VulkanTexture();
            m_ColorAttachments[VERTICAL_BLUR_BUFFER_INDEX]->CreateColorAttachment(width, height, VK_FORMAT_R16G16B16A16_SFLOAT, BLUR_DOWN_SAMPLES);

            VulkanTextureHelper::TransitionImageLayout
            (
                m_ColorAttachments[VERTICAL_BLUR_BUFFER_INDEX]->GetImage(),
                m_ColorAttachments[VERTICAL_BLUR_BUFFER_INDEX]->GetFormat(),
                0, BLUR_DOWN_SAMPLES,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );

            // Load bloom shader
            m_BloomShader.LoadFromFile("../PIX3D/res/vk shaders/bloom.vert", "../PIX3D/res/vk shaders/bloom.frag");

            // Create uniform buffer for blur direction and mip level
            //m_UniformBuffer.Create(sizeof(BloomUBO));

            // Descriptor layout
            m_DescriptorSetLayout
                .AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)  // other buffer color attachment
                .Build();

            {
                m_DescriptorSets[HORIZONTAL_BLUR_BUFFER_INDEX]
                    .Init(m_DescriptorSetLayout)
                    .AddTexture(0, *m_ColorAttachments[VERTICAL_BLUR_BUFFER_INDEX])
                    .Build();
            }

            // buffer 1 reads color attachment of buffer 0 as input color attachment
            {
                m_DescriptorSets[VERTICAL_BLUR_BUFFER_INDEX]
                    .Init(m_DescriptorSetLayout)
                    .AddTexture(0, *m_ColorAttachments[HORIZONTAL_BLUR_BUFFER_INDEX])
                    .Build();
            }

            // Create renderpasses
            for (int i = 0; i < BLUR_BUFFERS_COUNT; i++)
            {
                m_Renderpasses[i]
                    .Init(Context->m_Device)
                    .AddColorAttachment(
                        m_ColorAttachments[i]->GetFormat(),
                        VK_SAMPLE_COUNT_1_BIT,
                        VK_ATTACHMENT_LOAD_OP_CLEAR,
                        VK_ATTACHMENT_STORE_OP_STORE,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                    .AddSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
                    // Initial dependency to wait for previous fragment shader reads
                    .AddDependency(
                        VK_SUBPASS_EXTERNAL,
                        0,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_ACCESS_SHADER_READ_BIT,
                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                        VK_DEPENDENCY_BY_REGION_BIT
                    )
                    // Final dependency for shader reads
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
            }

            // Create pipeline layouts
            for (int i = 0; i < BLUR_BUFFERS_COUNT; i++)
            {
                VkPushConstantRange pushConstant{};
                pushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                pushConstant.offset = 0;
                pushConstant.size = sizeof(BloomPushConstant);

                VkDescriptorSetLayout layouts[] = { m_DescriptorSetLayout.GetVkDescriptorSetLayout() };

                VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
                pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutInfo.setLayoutCount = 1;
                pipelineLayoutInfo.pSetLayouts = layouts;
                pipelineLayoutInfo.pushConstantRangeCount = 1;
                pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

                if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayouts[i]) != VK_SUCCESS)
                    PIX_ASSERT_MSG(false, "Failed to create pipeline layout!");
            }

            // Create graphics pipelines

            m_QuadMesh = VulkanStaticMeshGenerator::GenerateQuad();
            auto bindingDesc = m_QuadMesh.VertexLayout.GetBindingDescription();
            auto attributeDesc = m_QuadMesh.VertexLayout.GetAttributeDescriptions();

            for (int i = 0; i < BLUR_BUFFERS_COUNT; i++)
            {
                m_Pipelines[i]
                    .Init(Context->m_Device, m_Renderpasses[i].GetVKRenderpass())
                    .AddShaderStages(m_BloomShader.GetVertexShader(), m_BloomShader.GetFragmentShader())
                    .AddVertexInputState(&bindingDesc, attributeDesc.data(), 1, attributeDesc.size())
                    .AddViewportState(width, height)
                    .AddInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
                    .AddRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
                    .AddMultisampleState(VK_SAMPLE_COUNT_1_BIT)
                    .AddColorBlendState(true, 1)
                    .SetPipelineLayout(m_PipelineLayouts[i])
                    .Build();
            }

            // Setup initial framebuffers
            /* 
            buffer HORIZONTAL BLUR has 6 frame buffers
            each one hook into (DOWN SAMPLES (MIP_LEVEL)) of it's color attachment to renderer to it's mip
            */
            /*
            buffer VERTICAL BLUR has 6 frame buffers
            each one hook into (DOWN SAMPLES (MIP_LEVEL)) of it's color attachment to renderer to it's mip
            */
            m_Framebuffers[HORIZONTAL_BLUR_BUFFER_INDEX].resize(BLUR_DOWN_SAMPLES);
            m_Framebuffers[VERTICAL_BLUR_BUFFER_INDEX].resize(BLUR_DOWN_SAMPLES);
            
            // Recreate framebuffers for all mip levels
            for (int mipLevel = 0; mipLevel < BLUR_DOWN_SAMPLES; mipLevel++)
                SetupFramebuffers(mipLevel);
        }

        void VulkanBloomPass::SetupFramebuffers(int mipLevel)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            uint32_t mipWidth = m_Width >> mipLevel;
            uint32_t mipHeight = m_Height >> mipLevel;

            m_Framebuffers[HORIZONTAL_BLUR_BUFFER_INDEX][mipLevel]
                .Init(Context->m_Device, m_Renderpasses[HORIZONTAL_BLUR_BUFFER_INDEX].GetVKRenderpass(), mipWidth, mipHeight)
                .AddAttachment(m_ColorAttachments[HORIZONTAL_BLUR_BUFFER_INDEX], mipLevel)
                .Build();


            m_Framebuffers[VERTICAL_BLUR_BUFFER_INDEX][mipLevel]
                .Init(Context->m_Device, m_Renderpasses[VERTICAL_BLUR_BUFFER_INDEX].GetVKRenderpass(), mipWidth, mipHeight)
                .AddAttachment(m_ColorAttachments[VERTICAL_BLUR_BUFFER_INDEX], mipLevel)
                .Build();
        }

        void VulkanBloomPass::RecordCommandBuffer(VulkanTexture* bloom_brightness_texture, VkCommandBuffer commandBuffer, uint32_t numIterations)
        {
            m_FinalResultBufferIndex = (numIterations % 2 == 0) ? 1 : 0;

            // First generate mipmaps for input brightness texture (down sampling)
            {
                VulkanTextureHelper::TransitionImageLayout
                (
                    bloom_brightness_texture->GetImage(),
                    bloom_brightness_texture->GetFormat(),
                    0, bloom_brightness_texture->GetMipLevels(),
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    commandBuffer
                );

                bloom_brightness_texture->GenerateMipmaps(commandBuffer);
            }

            // Copy Data To Vertical Pass Color Attachment For Ping - Pong Blur

            VulkanTextureHelper::CopyTextureWithAllMips
            (
                bloom_brightness_texture->GetImage(),
                bloom_brightness_texture->GetFormat(),
                m_ColorAttachments[VERTICAL_BLUR_BUFFER_INDEX]->GetImage(),
                m_ColorAttachments[VERTICAL_BLUR_BUFFER_INDEX]->GetFormat(),
                bloom_brightness_texture->GetWidth(),
                bloom_brightness_texture->GetHeight(),
                bloom_brightness_texture->GetMipLevels(),
                commandBuffer
            );

            for (int mipLevel = 0; mipLevel < BLUR_DOWN_SAMPLES; mipLevel++)
            {
                uint32_t mipWidth = m_Width / std::pow(2, mipLevel);
                uint32_t mipHeight = m_Height / std::pow(2, mipLevel);

                // SetupFramebuffers(mipLevel); // draw to specific mip map

                for (size_t i = 0; i < numIterations; i++)
                {
                    // Horizontal Blur Pass
                    {
                        VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

                        VkRenderPassBeginInfo renderPassInfo = {};
                        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                        renderPassInfo.renderPass = m_Renderpasses[HORIZONTAL_BLUR_BUFFER_INDEX].GetVKRenderpass();
                        renderPassInfo.framebuffer = m_Framebuffers[HORIZONTAL_BLUR_BUFFER_INDEX][mipLevel].GetVKFramebuffer();
                        renderPassInfo.renderArea = { {0, 0}, {mipWidth, mipHeight} };
                        renderPassInfo.clearValueCount = 1;
                        renderPassInfo.pClearValues = &clearValue;

                        BloomPushConstant pushData = { glm::vec2(1.0f, 0.0f), mipLevel, 1 };
                        vkCmdPushConstants(commandBuffer, m_PipelineLayouts[HORIZONTAL_BLUR_BUFFER_INDEX],
                            VK_SHADER_STAGE_FRAGMENT_BIT,
                            0, sizeof(BloomPushConstant), &pushData);

                        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipelines[HORIZONTAL_BLUR_BUFFER_INDEX].GetVkPipeline());

                        VkViewport viewport{};
                        viewport.x = 0.0f;
                        viewport.y = (float)mipHeight;
                        viewport.width = (float)mipWidth;
                        viewport.height = -(float)mipHeight;
                        viewport.minDepth = 0.0f;
                        viewport.maxDepth = 1.0f;

                        VkRect2D scissor{};
                        scissor.offset = { 0, 0 };
                        scissor.extent = { mipWidth, mipHeight };

                        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
                        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

                        VkBuffer vertexBuffers[] = { m_QuadMesh.VertexBuffer.GetBuffer() };
                        VkDeviceSize offsets[] = { 0 };
                        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                        vkCmdBindIndexBuffer(commandBuffer, m_QuadMesh.IndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

                        auto descriptorSet = m_DescriptorSets[HORIZONTAL_BLUR_BUFFER_INDEX].GetVkDescriptorSet();
                        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_PipelineLayouts[HORIZONTAL_BLUR_BUFFER_INDEX], 0, 1, &descriptorSet, 0, nullptr);

                        vkCmdDrawIndexed(commandBuffer, m_QuadMesh.IndicesCount, 1, 0, 0, 0);
                        vkCmdEndRenderPass(commandBuffer);
                    }

                    // Vertical Blur Pass
                    {
                        VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

                        VkRenderPassBeginInfo renderPassInfo = {};
                        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                        renderPassInfo.renderPass = m_Renderpasses[VERTICAL_BLUR_BUFFER_INDEX].GetVKRenderpass();
                        renderPassInfo.framebuffer = m_Framebuffers[VERTICAL_BLUR_BUFFER_INDEX][mipLevel].GetVKFramebuffer();
                        renderPassInfo.renderArea = { {0, 0}, {mipWidth, mipHeight} };
                        renderPassInfo.clearValueCount = 1;
                        renderPassInfo.pClearValues = &clearValue;

                        BloomPushConstant pushData = { glm::vec2(0.0f, 1.0f), mipLevel, 1 };
                        vkCmdPushConstants(commandBuffer, m_PipelineLayouts[VERTICAL_BLUR_BUFFER_INDEX],
                            VK_SHADER_STAGE_FRAGMENT_BIT,
                            0, sizeof(BloomPushConstant), &pushData);

                        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipelines[VERTICAL_BLUR_BUFFER_INDEX].GetVkPipeline());

                        VkViewport viewport{};
                        viewport.x = 0.0f;
                        viewport.y = (float)mipHeight;
                        viewport.width = (float)mipWidth;
                        viewport.height = -(float)mipHeight;
                        viewport.minDepth = 0.0f;
                        viewport.maxDepth = 1.0f;

                        VkRect2D scissor{};
                        scissor.offset = { 0, 0 };
                        scissor.extent = { mipWidth, mipHeight };

                        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
                        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

                        VkBuffer vertexBuffers[] = { m_QuadMesh.VertexBuffer.GetBuffer() };
                        VkDeviceSize offsets[] = { 0 };
                        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                        vkCmdBindIndexBuffer(commandBuffer, m_QuadMesh.IndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

                        auto descriptorSet = m_DescriptorSets[VERTICAL_BLUR_BUFFER_INDEX].GetVkDescriptorSet();
                        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_PipelineLayouts[VERTICAL_BLUR_BUFFER_INDEX], 0, 1, &descriptorSet, 0, nullptr);

                        vkCmdDrawIndexed(commandBuffer, m_QuadMesh.IndicesCount, 1, 0, 0, 0);
                        vkCmdEndRenderPass(commandBuffer);
                    }
                }
            }
        }

        void VulkanBloomPass::OnResize(uint32_t width, uint32_t height)
        {
            m_Width = width;
            m_Height = height;

            // Destroy existing color attachments
            for (int i = 0; i < BLUR_BUFFERS_COUNT; i++)
            {
                if (m_ColorAttachments[i])
                {
                    m_ColorAttachments[i]->Destroy();
                    delete m_ColorAttachments[i];
                    m_ColorAttachments[i] = nullptr;
                }
            }

            // Destroy existing framebuffers
            for (int i = 0; i < BLUR_BUFFERS_COUNT; i++)
            {
                for (auto& framebuffer : m_Framebuffers[i])
                {
                    framebuffer.Destroy();
                }
                m_Framebuffers[i].clear();
            }

            // Recreate color attachments with new size
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            // Create two textures for ping-pong blurring with mip levels
            m_ColorAttachments[HORIZONTAL_BLUR_BUFFER_INDEX] = new VulkanTexture();
            m_ColorAttachments[HORIZONTAL_BLUR_BUFFER_INDEX]->CreateColorAttachment(width, height, VK_FORMAT_R16G16B16A16_SFLOAT, BLUR_DOWN_SAMPLES);

            VulkanTextureHelper::TransitionImageLayout
            (
                m_ColorAttachments[HORIZONTAL_BLUR_BUFFER_INDEX]->GetImage(),
                m_ColorAttachments[HORIZONTAL_BLUR_BUFFER_INDEX]->GetFormat(),
                0, BLUR_DOWN_SAMPLES,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );

            m_ColorAttachments[VERTICAL_BLUR_BUFFER_INDEX] = new VulkanTexture();
            m_ColorAttachments[VERTICAL_BLUR_BUFFER_INDEX]->CreateColorAttachment(width, height, VK_FORMAT_R16G16B16A16_SFLOAT, BLUR_DOWN_SAMPLES);

            VulkanTextureHelper::TransitionImageLayout
            (
                m_ColorAttachments[VERTICAL_BLUR_BUFFER_INDEX]->GetImage(),
                m_ColorAttachments[VERTICAL_BLUR_BUFFER_INDEX]->GetFormat(),
                0, BLUR_DOWN_SAMPLES,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );

            // Resize framebuffer arrays
            m_Framebuffers[HORIZONTAL_BLUR_BUFFER_INDEX].resize(BLUR_DOWN_SAMPLES);
            m_Framebuffers[VERTICAL_BLUR_BUFFER_INDEX].resize(BLUR_DOWN_SAMPLES);

            // Update descriptor sets with new textures
            m_DescriptorSets[HORIZONTAL_BLUR_BUFFER_INDEX]
                .Init(m_DescriptorSetLayout)
                .AddTexture(0, *m_ColorAttachments[VERTICAL_BLUR_BUFFER_INDEX])
                .Build();

            m_DescriptorSets[VERTICAL_BLUR_BUFFER_INDEX]
                .Init(m_DescriptorSetLayout)
                .AddTexture(0, *m_ColorAttachments[HORIZONTAL_BLUR_BUFFER_INDEX])
                .Build();

            // Recreate framebuffers for all mip levels
            for (int mipLevel = 0; mipLevel < BLUR_DOWN_SAMPLES; mipLevel++)
            {
                SetupFramebuffers(mipLevel);
            }
        }

        void VulkanBloomPass::Destroy()
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            // Destroy color attachments
            for (int i = 0; i < BLUR_BUFFERS_COUNT; i++)
            {
                if (m_ColorAttachments[i])
                {
                    m_ColorAttachments[i]->Destroy();
                    delete m_ColorAttachments[i];
                    m_ColorAttachments[i] = nullptr;
                }
            }

            // Destroy descriptor sets and layout
            for (int i = 0; i < BLUR_BUFFERS_COUNT; i++)
            {
                m_DescriptorSets[i].Destroy();
            }
            m_DescriptorSetLayout.Destroy();

            // Destroy pipeline layouts
            for (int i = 0; i < BLUR_BUFFERS_COUNT; i++)
            {
                if (m_PipelineLayouts[i] != VK_NULL_HANDLE)
                {
                    vkDestroyPipelineLayout(Context->m_Device, m_PipelineLayouts[i], nullptr);
                    m_PipelineLayouts[i] = VK_NULL_HANDLE;
                }
            }

            // Destroy graphics pipelines
            for (int i = 0; i < BLUR_BUFFERS_COUNT; i++)
            {
                m_Pipelines[i].Destroy();
            }

            // Destroy render passes
            for (int i = 0; i < BLUR_BUFFERS_COUNT; i++)
            {
                m_Renderpasses[i].Destroy();
            }

            // Destroy framebuffers
            for (int i = 0; i < BLUR_BUFFERS_COUNT; i++)
            {
                for (auto& framebuffer : m_Framebuffers[i])
                {
                    framebuffer.Destroy();
                }
                m_Framebuffers[i].clear();
            }

            // Destroy shader
            m_BloomShader.Destroy();

            // Reset member pointers
            m_Width = 0;
            m_Height = 0;
            m_FinalResultBufferIndex = 0;
        }
    }
}
