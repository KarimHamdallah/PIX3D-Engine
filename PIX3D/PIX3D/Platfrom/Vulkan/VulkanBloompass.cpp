#include "VulkanBloomPass.h"
#include <Engine/Engine.hpp>

namespace PIX3D
{
    namespace VK
    {
        void VulkanBloomPass::Init(uint32_t width, uint32_t height, VulkanTexture* bloom_brightness_texture)
        {
            m_Width = width;
            m_Height = height;
            m_InputBrightnessTexture = bloom_brightness_texture;

            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            // Create two textures for ping-pong blurring with mip levels
            m_ColorAttachments[HORIZONTAL_BLUR_BUFFER_INDEX] = new VulkanTexture();
            m_ColorAttachments[HORIZONTAL_BLUR_BUFFER_INDEX]->Create();
            m_ColorAttachments[HORIZONTAL_BLUR_BUFFER_INDEX]->CreateColorAttachment(width, height, TextureFormat::RGBA16F, BLUR_DOWN_SAMPLES);

            m_ColorAttachments[HORIZONTAL_BLUR_BUFFER_INDEX]->TransitionImageLayout(
                m_ColorAttachments[HORIZONTAL_BLUR_BUFFER_INDEX]->GetVKormat(),
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );

            m_ColorAttachments[VERTICAL_BLUR_BUFFER_INDEX] = new VulkanTexture();
            m_ColorAttachments[VERTICAL_BLUR_BUFFER_INDEX]->Create();
            m_ColorAttachments[VERTICAL_BLUR_BUFFER_INDEX]->CreateColorAttachment(width, height, TextureFormat::RGBA16F, BLUR_DOWN_SAMPLES);

            m_ColorAttachments[VERTICAL_BLUR_BUFFER_INDEX]->TransitionImageLayout(
                m_ColorAttachments[VERTICAL_BLUR_BUFFER_INDEX]->GetVKormat(),
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );

            // Load bloom shader
            m_BloomShader.LoadFromFile("../PIX3D/res/vk shaders/bloom.vert", "../PIX3D/res/vk shaders/bloom.frag");

            // Create uniform buffer for blur direction and mip level
            //m_UniformBuffer.Create(sizeof(BloomUBO));

            // Descriptor layout
            m_DescriptorSetLayout
                .AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)  // Input Brightness texture
                .AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)  // other buffer color attachment
                .Build();

            {
                m_DescriptorSets[HORIZONTAL_BLUR_BUFFER_INDEX]
                    .Init(m_DescriptorSetLayout)
                    .AddTexture(0, *m_InputBrightnessTexture)
                    .AddTexture(1, *m_ColorAttachments[VERTICAL_BLUR_BUFFER_INDEX])
                    .Build();
            }

            // buffer 1 reads color attachment of buffer 0 as input color attachment
            {
                m_DescriptorSets[VERTICAL_BLUR_BUFFER_INDEX]
                    .Init(m_DescriptorSetLayout)
                    .AddTexture(0, *m_InputBrightnessTexture)
                    .AddTexture(1, *m_ColorAttachments[HORIZONTAL_BLUR_BUFFER_INDEX])
                    .Build();
            }

            // Create renderpasses
            for (int i = 0; i < BLUR_BUFFERS_COUNT; i++)
            {
                m_Renderpasses[i]
                    .Init(Context->m_Device)
                    .AddColorAttachment(
                        m_ColorAttachments[i]->GetVKormat(),
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
            
            SetupFramebuffers(0);
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

        void VulkanBloomPass::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t numIterations)
        {
            // First generate mipmaps for input brightness texture (down sampling)            
            {
                m_InputBrightnessTexture->TransitionImageLayout(
                    m_InputBrightnessTexture->GetVKormat(),
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    commandBuffer
                );

                m_InputBrightnessTexture->GenerateMipmaps(commandBuffer);
            }

            for (int mipLevel = 0; mipLevel < BLUR_DOWN_SAMPLES; mipLevel++)
            {
                uint32_t mipWidth = m_Width / std::pow(2, mipLevel);
                uint32_t mipHeight = m_Height / std::pow(2, mipLevel);

                SetupFramebuffers(mipLevel);

                // Initial Horizontal Blur Of Input Brightness Texture
                {
                    VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

                    VkRenderPassBeginInfo renderPassInfo = {};
                    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    renderPassInfo.renderPass = m_Renderpasses[HORIZONTAL_BLUR_BUFFER_INDEX].GetVKRenderpass();
                    renderPassInfo.framebuffer = m_Framebuffers[HORIZONTAL_BLUR_BUFFER_INDEX][mipLevel].GetVKFramebuffer();
                    renderPassInfo.renderArea = { {0, 0}, {mipWidth, mipHeight} };
                    renderPassInfo.clearValueCount = 1;
                    renderPassInfo.pClearValues = &clearValue;

                    BloomPushConstant pushData = { glm::vec2(1.0f, 0.0f), 0, 1 };
                    vkCmdPushConstants(commandBuffer, m_PipelineLayouts[0],
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

                    auto descriptorSet = m_DescriptorSets[0].GetVkDescriptorSet();
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        m_PipelineLayouts[HORIZONTAL_BLUR_BUFFER_INDEX], 0, 1, &descriptorSet, 0, nullptr);

                    vkCmdDrawIndexed(commandBuffer, m_QuadMesh.IndicesCount, 1, 0, 0, 0);
                    vkCmdEndRenderPass(commandBuffer);
                }

                // Ping-pong between buffers for remaining iterations
                int currentBuffer = 0;
                for (uint32_t i = 1; i < numIterations; i++)
                {
                    int targetBuffer = 1 - currentBuffer;

                    uint32_t w = m_Width / std::pow(2, mipLevel);
                    uint32_t h = m_Height / std::pow(2, mipLevel);

                    VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

                    VkRenderPassBeginInfo renderPassInfo = {};
                    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    renderPassInfo.renderPass = m_Renderpasses[targetBuffer].GetVKRenderpass();
                    renderPassInfo.framebuffer = m_Framebuffers[targetBuffer][mipLevel].GetVKFramebuffer();
                    renderPassInfo.renderArea = { {0, 0}, {mipWidth, mipHeight} };
                    renderPassInfo.clearValueCount = 1;
                    renderPassInfo.pClearValues = &clearValue;

                    BloomPushConstant pushData = 
                    {
                        (targetBuffer == 1) ? glm::vec2(0.0f, 1.0f) : glm::vec2(1.0f, 0.0f),
                        mipLevel, 0
                    };
                    vkCmdPushConstants(commandBuffer, m_PipelineLayouts[0],
                        VK_SHADER_STAGE_FRAGMENT_BIT,
                        0, sizeof(BloomPushConstant), &pushData);


                    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipelines[targetBuffer].GetVkPipeline());

                    VkViewport viewport{};
                    viewport.x = 0.0f;
                    viewport.y = (float)h;
                    viewport.width = (float)w;
                    viewport.height = -(float)h;
                    viewport.minDepth = 0.0f;
                    viewport.maxDepth = 1.0f;

                    VkRect2D scissor{};
                    scissor.offset = { 0, 0 };
                    scissor.extent = { w, h };

                    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
                    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);


                    VkBuffer vertexBuffers[] = { m_QuadMesh.VertexBuffer.GetBuffer() };
                    VkDeviceSize offsets[] = { 0 };
                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                    vkCmdBindIndexBuffer(commandBuffer, m_QuadMesh.IndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

                    auto descriptorSet = m_DescriptorSets[targetBuffer].GetVkDescriptorSet();
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        m_PipelineLayouts[targetBuffer], 0, 1, &descriptorSet, 0, nullptr);

                    vkCmdDrawIndexed(commandBuffer, m_QuadMesh.IndicesCount, 1, 0, 0, 0);
                    vkCmdEndRenderPass(commandBuffer);

                    currentBuffer = targetBuffer;
                    m_FinalResultBufferIndex = currentBuffer;
                }
            }
        }

        void VulkanBloomPass::Resize(uint32_t width, uint32_t height)
        {
            /*
            m_Width = width;
            m_Height = height;

            // Recreate textures with new size
            for (int i = 0; i < 2; i++)
            {
                m_BloomTextures[i]->Resize(width, height);
            }

            // Recreate framebuffers for all mip levels
            for (int mipLevel = 0; mipLevel < MAX_MIP_LEVELS; mipLevel++)
            {
                SetupFramebuffers(mipLevel);
            }
            */
        }

        void VulkanBloomPass::Destroy()
        {
            // TODO:: Destroy Vulkan Objects
        }
    }
}