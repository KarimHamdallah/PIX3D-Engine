#include "VulkanPrefilteredCubeMap.h"
#include <Engine/Engine.hpp>
#include "../VulkanHelper.h"

namespace PIX3D
{
    namespace VK
    {
        bool VulkanPrefilteredCubemap::Generate(VulkanHdrCubemap* cubemap, uint32_t cubemapSize, uint32_t mips)
        {
            // Create Cubemap Texture [ usage >> Color Attachment Texture To Draw Into It ]
            {
                m_Size = cubemapSize;
                m_Format = VK_FORMAT_R16G16B16A16_SFLOAT;
                m_BytePerPixel = 8;
                m_MipLevels = mips;

                auto usage =
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                    | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                    | VK_IMAGE_USAGE_SAMPLED_BIT;

                // Create the image with mip levels
                VulkanCubemapHelper::CreateImage(m_Size, m_Size, m_Format, m_MipLevels,
                    VK_IMAGE_TILING_OPTIMAL,
                    usage,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Image, m_ImageMemory);

                // Create image view with mip levels
                VulkanCubemapHelper::CreateImageView(m_Image, m_Format, m_ImageView, 0, m_MipLevels);
                VulkanCubemapHelper::CreateSampler(m_MipLevels - 1, m_Sampler);
            }


            {
                // Load Shader Of Mapping Equirectangluar Map To Cube

                auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

                //////////////////////// Shader ///////////////////////////

                VulkanShader PrefilterCubeMapGeneratorShader;
                PrefilterCubeMapGeneratorShader.LoadFromFile("../PIX3D/res/vk shaders/ibl_specular_map.vert", "../PIX3D/res/vk shaders/ibl_specular_map.frag");

                // Load Cube Mesh Vertex Buffer And Extract Vertex Input Data

                VulkanStaticMeshData CubeMesh = VulkanStaticMeshGenerator::GenerateCube();

                auto bindingDesc = CubeMesh.VertexLayout.GetBindingDescription();
                auto attributeDesc = CubeMesh.VertexLayout.GetAttributeDescriptions();

                // Create Descriptor Set Layout : Single Image Sampler For Equirectangluar Map

                 // Descriptor layout
                VulkanDescriptorSetLayout DescriptorSetLayout;
                DescriptorSetLayout
                    .AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
                    .Build();

                // Create Descriptor Set From Layout

                VulkanDescriptorSet DescriptorSet;
                DescriptorSet
                    .Init(DescriptorSetLayout)
                    .AddCubemap(0, *cubemap)
                    .Build();

                // Create Renderpass >> How To Make It Render To Cube Map Face!!!

                VulkanRenderPass Renderpass;

                Renderpass
                    .Init(Context->m_Device)
                    .AddColorAttachment(
                        m_Format,
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

                // Create 6 Framebuffer Each One Write To Each Layer (face) Of CubeMap

                const uint32_t FaceCount = 6;
                const uint32_t MipCount = 5;

                VulkanFramebuffer m_Framebuffers[FaceCount][MipCount];

                const unsigned int maxMipLevels = 5;
                for (size_t face = 0; face < FaceCount; face++)
                {
                    for (size_t mip = 0; mip < MipCount; mip++)
                    {
                        unsigned int mipSize = m_Size * std::pow(0.5, mip);

                        m_Framebuffers[face][mip]
                            .Init(Context->m_Device, Renderpass.GetVKRenderpass(), mipSize, mipSize)
                            .AddAttachment(m_Image, m_Format, (uint32_t)face, mip)
                            .Build();
                    }
                }

                struct _PushConstant
                {
                    glm::mat4 view_proj;
                    float roughness;
                };

                // Create Pipeline And Pipeline Layout For This And Add Push Constant For view_projection matrix
                VkPushConstantRange Vert_pushConstant{};
                Vert_pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                Vert_pushConstant.offset = 0;
                Vert_pushConstant.size = sizeof(_PushConstant);

                VkDescriptorSetLayout layouts[] = { DescriptorSetLayout.GetVkDescriptorSetLayout() };
                VkPushConstantRange push_constants[] = { Vert_pushConstant };

                VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
                pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutInfo.setLayoutCount = 1;
                pipelineLayoutInfo.pSetLayouts = layouts;
                pipelineLayoutInfo.pushConstantRangeCount = 1;
                pipelineLayoutInfo.pPushConstantRanges = push_constants;

                VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
                if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr, &PipelineLayout) != VK_SUCCESS)
                    PIX_ASSERT_MSG(false, "Failed to create pipeline layout!");

                /////////////// Graphics Pipeline //////////////////////

                VulkanGraphicsPipeline GraphicsPipeline;
                GraphicsPipeline.Init(Context->m_Device, Renderpass.GetVKRenderpass())
                    .AddShaderStages(PrefilterCubeMapGeneratorShader.GetVertexShader(), PrefilterCubeMapGeneratorShader.GetFragmentShader())
                    .AddVertexInputState(&bindingDesc, attributeDesc.data(), 1, attributeDesc.size())
                    .AddViewportState(m_Size, m_Size)
                    .AddInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
                    .AddRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE)
                    .AddMultisampleState(VK_SAMPLE_COUNT_1_BIT)
                    .AddDepthStencilState(true, true)
                    .AddColorBlendState(true, 1)
                    .SetPipelineLayout(PipelineLayout)
                    .Build();

                // Prepare 6 glm::LookAt matrices for each face and Projection

                glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
                
                /*
                glm::mat4 captureViews[] =
                {
                    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // right
                    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // left
                    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)), // up
                    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)), // down
                    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // back
                    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)) // forawad
                };
                glm::mat4 faceRotations[] =
                {
                    glm::mat4(1.0f),
                    glm::mat4(1.0f),
                    glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
                    glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
                    glm::mat4(1.0f),
                    glm::mat4(1.0f)
                };
                */

                glm::mat4 captureViews[] =
                {
                    // Right
                    glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                    // Left 
                    glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                    // Top
                    glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                    // Bottom
                    glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
                    // Front 
                    glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                    // Back
                    glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
                };

                // Render 6 Times in loop Each Time We Do Resetup Framebuffer To Target Next Cubemap Layer (face)

                VkCommandBuffer CommandBuffer = VulkanHelper::BeginSingleTimeCommands(Context->m_Device, Context->m_CommandPool);

                for (size_t i = 0; i < FaceCount; i++)
                {
                    for (size_t _mip = 0; _mip < MipCount; _mip++)
                    {
                        unsigned int mipSize = m_Size * std::pow(0.5, _mip);

                        VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

                        VkRenderPassBeginInfo renderPassInfo = {};
                        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                        renderPassInfo.renderPass = Renderpass.GetVKRenderpass();
                        renderPassInfo.framebuffer = m_Framebuffers[i][_mip].GetVKFramebuffer();
                        renderPassInfo.renderArea = { {0, 0}, {mipSize, mipSize} };
                        renderPassInfo.clearValueCount = 1;
                        renderPassInfo.pClearValues = &clearValue;

                        glm::mat4 view_proj = captureProjection * captureViews[i];
                        float roughness = (float)_mip / (float)(maxMipLevels - 1);

                        _PushConstant __data = { view_proj, roughness };

                        vkCmdPushConstants(CommandBuffer, PipelineLayout,
                            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                            0, sizeof(_PushConstant), &__data);


                        vkCmdBeginRenderPass(CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                        vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline.GetVkPipeline());

                        VkViewport viewport{};
                        viewport.x = 0.0f;
                        viewport.y = 0.0f;
                        viewport.width = (float)mipSize;
                        viewport.height = (float)mipSize;
                        viewport.minDepth = 0.0f;
                        viewport.maxDepth = 1.0f;

                        VkRect2D scissor{};
                        scissor.offset = { 0, 0 };
                        scissor.extent = { mipSize, mipSize };

                        vkCmdSetViewport(CommandBuffer, 0, 1, &viewport);
                        vkCmdSetScissor(CommandBuffer, 0, 1, &scissor);

                        VkBuffer vertexBuffers[] = { CubeMesh.VertexBuffer.GetBuffer() };
                        VkDeviceSize offsets[] = { 0 };
                        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, vertexBuffers, offsets);

                        auto descriptorSet = DescriptorSet.GetVkDescriptorSet();
                        vkCmdBindDescriptorSets(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            PipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

                        vkCmdDraw(CommandBuffer, CubeMesh.VerticesCount, 1, 0, 0);
                        vkCmdEndRenderPass(CommandBuffer);
                    }
                }

                VulkanHelper::EndSingleTimeCommands(Context->m_Device, Context->m_Queue.m_Queue, Context->m_CommandPool, CommandBuffer);


                // Wait for device
                vkDeviceWaitIdle(Context->m_Device);

                // Cleanup shader
                PrefilterCubeMapGeneratorShader.Destroy();

                // Cleanup pipeline and layout
                GraphicsPipeline.Destroy();
                if (PipelineLayout != VK_NULL_HANDLE)
                {
                    vkDestroyPipelineLayout(Context->m_Device, PipelineLayout, nullptr);
                }

                // Cleanup render pass
                Renderpass.Destroy();

                for (size_t face = 0; face < FaceCount; face++)
                {
                    for (size_t mip = 0; mip < MipCount; mip++)
                    {
                        m_Framebuffers[face][mip].Destroy();
                    }
                }

                // Cleanup descriptor resources
                DescriptorSet.Destroy();
                DescriptorSetLayout.Destroy();
            }

            VulkanCubemapHelper::TransitionImageLayout(m_Image, m_MipLevels, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, nullptr);

            return true;
        }



        void VulkanPrefilteredCubemap::Destroy()
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            VkDevice m_Device = Context->m_Device;

            if (m_Device != VK_NULL_HANDLE)
            {
                if (m_Sampler != VK_NULL_HANDLE)
                {
                    vkDestroySampler(m_Device, m_Sampler, nullptr);
                    m_Sampler = VK_NULL_HANDLE;
                }
                if (m_ImageView != VK_NULL_HANDLE)
                {
                    vkDestroyImageView(m_Device, m_ImageView, nullptr);
                    m_ImageView = VK_NULL_HANDLE;
                }
                if (m_Image != VK_NULL_HANDLE)
                {
                    vkDestroyImage(m_Device, m_Image, nullptr);
                    m_Image = VK_NULL_HANDLE;
                }
                if (m_ImageMemory != VK_NULL_HANDLE)
                {
                    vkFreeMemory(m_Device, m_ImageMemory, nullptr);
                    m_ImageMemory = VK_NULL_HANDLE;
                }
            }
        }
    }
}
