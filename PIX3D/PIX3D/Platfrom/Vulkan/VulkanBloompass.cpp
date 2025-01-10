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
            m_InputTexture = bloom_brightness_texture;

            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            // Create two textures for ping-pong blurring with mip levels
            m_BloomTextures[0] = new VulkanTexture();
            m_BloomTextures[0]->Create();
            m_BloomTextures[0]->CreateColorAttachment(width, height, TextureFormat::RGBA16F, MAX_MIP_LEVELS);
            m_BloomTextures[0]->TransitionImageLayout(
                m_BloomTextures[0]->GetVKormat(),
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            );

            m_BloomTextures[1] = new VulkanTexture();
            m_BloomTextures[1]->Create();
            m_BloomTextures[1]->CreateColorAttachment(width, height, TextureFormat::RGBA16F, MAX_MIP_LEVELS);
            m_BloomTextures[1]->TransitionImageLayout(
                m_BloomTextures[1]->GetVKormat(),
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            );

            // Get quad mesh
            m_QuadMesh = VulkanStaticMeshGenerator::GenerateQuad();

            // Load bloom shader
            m_BloomShader.LoadFromFile("../PIX3D/res/vk shaders/bloom.vert", "../PIX3D/res/vk shaders/bloom.frag");

            // Create uniform buffer for blur direction and mip level
            //m_UniformBuffer.Create(sizeof(BloomUBO));

            // Descriptor layout
            m_DescriptorSetLayout
                .AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)  // Brightness texture
                .AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)  // other buffer color attachment
                .Build();

            // buffer 0 reads color attachment of buffer 1 as input color attachment
            {
                m_DescriptorSets[0]
                    .Init(m_DescriptorSetLayout)
                    .AddTexture(0, *m_InputTexture)
                    .AddTexture(1, *m_BloomTextures[1])
                    .Build();
            }

            // buffer 1 reads color attachment of buffer 0 as input color attachment
            {
                m_DescriptorSets[1]
                    .Init(m_DescriptorSetLayout)
                    .AddTexture(0, *m_InputTexture)
                    .AddTexture(1, *m_BloomTextures[0])
                    .Build();
            }

            // Create renderpasses
            for (int i = 0; i < 2; i++)
            {
                m_Renderpasses[i]
                    .Init(Context->m_Device)
                    .AddColorAttachment(
                        m_BloomTextures[i]->GetVKormat(),
                        VK_SAMPLE_COUNT_1_BIT,
                        VK_ATTACHMENT_LOAD_OP_CLEAR,
                        VK_ATTACHMENT_STORE_OP_STORE,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                    .AddSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
                    .Build();
            }

            // Create pipeline layouts
            for (int i = 0; i < 2; i++)
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
            auto bindingDesc = m_QuadMesh.VertexLayout.GetBindingDescription();
            auto attributeDesc = m_QuadMesh.VertexLayout.GetAttributeDescriptions();

            for (int i = 0; i < 2; i++)
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
            buffer 0 has 6 (MAX_MIP_LEVELS) frame buffers
            each one hook into mip level of it's color attachment to renderer to it's mip
            */
            /* 
            buffer 1 has 6 (MAX_MIP_LEVELS) frame buffers
            each one hook into mip level of it's color attachment to renderer to it's mip
            */
            m_Framebuffers[0].resize(MAX_MIP_LEVELS);
            m_Framebuffers[1].resize(MAX_MIP_LEVELS);
            SetupFramebuffers(0);
        }

        /*
           Recreate Frame Buffers For Buffer 0 and Buffer 1 And Hook Them To Specific Mip Level To Render To Them
        */
        void VulkanBloomPass::SetupFramebuffers(int mipLevel)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();
            
            uint32_t mipWidth = m_Width >> mipLevel;
            uint32_t mipHeight = m_Height >> mipLevel;

            {
                m_Framebuffers[0][mipLevel]
                    .Init(Context->m_Device, m_Renderpasses[0].GetVKRenderpass(), mipWidth, mipHeight)
                    .AddAttachment(m_BloomTextures[0], mipLevel)
                    .Build();
            }

            {
                m_Framebuffers[1][mipLevel]
                    .Init(Context->m_Device, m_Renderpasses[1].GetVKRenderpass(), mipWidth, mipHeight)
                    .AddAttachment(m_BloomTextures[1], mipLevel)
                    .Build();
            }
        }

        void VulkanBloomPass::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t numIterations)
        {

            // First generate mipmaps for input texture (down sampling)
            
            {
                /////////////// Change Layout To Transfer Destination To Generate Mipmaps  ////////////////////
                m_InputTexture->TransitionImageLayout(
                    m_InputTexture->GetVKormat(),
                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    commandBuffer
                );

                m_InputTexture->GenerateMipmaps(commandBuffer);
                /////////////////////////// End As Shader Read ////////////////////////////////////////////////
            }

            uint32_t mipWidth = m_Width >> 0;
            uint32_t mipHeight = m_Height >> 0;


            // Redirect Framebuffers To Render To Specific Mip Map Level Of Color Attachment Of Ping - Pong buffers
            SetupFramebuffers(0);

            // Initial horizontal blur from input to first ping-pong buffer
            {
                VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

                VkRenderPassBeginInfo renderPassInfo = {};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassInfo.renderPass = m_Renderpasses[0].GetVKRenderpass();
                renderPassInfo.framebuffer = m_Framebuffers[0][0].GetVKFramebuffer();
                renderPassInfo.renderArea = { {0, 0}, {mipWidth, mipHeight} };
                renderPassInfo.clearValueCount = 1;
                renderPassInfo.pClearValues = &clearValue;

                BloomPushConstant pushData = { glm::vec2(1.0f, 0.0f), 0, 1 };
                vkCmdPushConstants(commandBuffer, m_PipelineLayouts[0],
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                    0, sizeof(BloomPushConstant), &pushData);

                vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipelines[0].GetVkPipeline());

                VkBuffer vertexBuffers[] = { m_QuadMesh.VertexBuffer.GetBuffer() };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(commandBuffer, m_QuadMesh.IndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

                auto descriptorSet = m_DescriptorSets[0].GetVkDescriptorSet();
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    m_PipelineLayouts[0], 0, 1, &descriptorSet, 0, nullptr);

                vkCmdDrawIndexed(commandBuffer, m_QuadMesh.IndicesCount, 1, 0, 0, 0);
                vkCmdEndRenderPass(commandBuffer);
            }


            /*
            // For each mip level
            for (int mipLevel = 0; mipLevel < MAX_MIP_LEVELS; mipLevel++)
            {


                uint32_t mipWidth = m_Width >> mipLevel;
                uint32_t mipHeight = m_Height >> mipLevel;


                // Redirect Framebuffers To Render To Specific Mip Map Level Of Color Attachment Of Ping - Pong buffers
                SetupFramebuffers(mipLevel);

                // Initial horizontal blur from input to first ping-pong buffer
                {
                    VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

                    VkRenderPassBeginInfo renderPassInfo = {};
                    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    renderPassInfo.renderPass = m_Renderpasses[0].GetVKRenderpass();
                    renderPassInfo.framebuffer = m_Framebuffers[0][mipLevel].GetVKFramebuffer();
                    renderPassInfo.renderArea = { {0, 0}, {mipWidth, mipHeight} };
                    renderPassInfo.clearValueCount = 1;
                    renderPassInfo.pClearValues = &clearValue;

                    BloomPushConstant pushData = { glm::vec2(1.0f, 0.0f), mipLevel, 1 };
                    vkCmdPushConstants(commandBuffer, m_PipelineLayouts[0],
                        VK_SHADER_STAGE_FRAGMENT_BIT,
                        0, sizeof(BloomPushConstant), &pushData);

                    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipelines[0].GetVkPipeline());

                    VkBuffer vertexBuffers[] = { m_QuadMesh.VertexBuffer.GetBuffer() };
                    VkDeviceSize offsets[] = { 0 };
                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                    vkCmdBindIndexBuffer(commandBuffer, m_QuadMesh.IndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

                    auto descriptorSet = m_DescriptorSets[0].GetVkDescriptorSet();
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        m_PipelineLayouts[0], 0, 1, &descriptorSet, 0, nullptr);

                    vkCmdDrawIndexed(commandBuffer, m_QuadMesh.IndicesCount, 1, 0, 0, 0);
                    vkCmdEndRenderPass(commandBuffer);
                }

                // Ping-pong between buffers for remaining iterations
                int currentBuffer = 0;
                for (uint32_t i = 1; i < numIterations; i++)
                {
                    int targetBuffer = 1 - currentBuffer;

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
            */
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
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            //m_BloomShader.Destroy();

            for (int i = 0; i < 2; i++)
            {
                vkDestroyPipelineLayout(Context->m_Device, m_PipelineLayouts[i], nullptr);
              //  m_Pipelines[i].Destroy();
              //  m_Renderpasses[i].Destroy();

                for (auto& framebuffer : m_Framebuffers[i])
                {
                    framebuffer.Destroy();
                }

                if (m_BloomTextures[i])
                {
                    m_BloomTextures[i]->Destroy();
                    delete m_BloomTextures[i];
                    m_BloomTextures[i] = nullptr;
                }
            }
        }
    }
}
