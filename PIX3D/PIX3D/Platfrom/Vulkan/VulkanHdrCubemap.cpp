#include "VulkanHdrCubemap.h"
#include <Engine/Engine.hpp>
#include "VulkanHelper.h"

namespace PIX3D
{
    namespace VK
    {
        bool VulkanHdrCubemap::LoadHdrToCubemapGPU(const std::filesystem::path& hdrPath, uint32_t cubemapSize, uint32_t mips)
        {
            // Load HDRI Texture

            m_EquirectangularMap = new VulkanTexture();
            m_EquirectangularMap->Create();
            m_EquirectangularMap->LoadFromHDRFile(hdrPath);

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
                VulkanCubemapHelper::CreateImageView(m_Image,m_Format, m_ImageView);
                VulkanCubemapHelper::CreateSampler(m_MipLevels - 1, m_Sampler);
            }


            {
                // Load Shader Of Mapping Equirectangluar Map To Cube

                auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

                //////////////////////// Shader ///////////////////////////

                VulkanShader EquirectangularMapToCubeShader;
                EquirectangularMapToCubeShader.LoadFromFile("../PIX3D/res/vk shaders/equirectangluarmap_to_cube.vert", "../PIX3D/res/vk shaders/equirectangluarmap_to_cube.frag");

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
                  .AddTexture(0, *m_EquirectangularMap)
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
                
                std::vector<VulkanFramebuffer> m_Framebuffers;
                m_Framebuffers.resize(6);

                for (size_t i = 0; i < m_Framebuffers.size(); i++)
                {
                    m_Framebuffers[i]
                        .Init(Context->m_Device, Renderpass.GetVKRenderpass(), m_Size, m_Size)
                        .AddAttachment(m_Image, m_Format, (uint32_t)i, 0)
                        .Build();
                }

                // Create Pipeline And Pipeline Layout For This And Add Push Constant For view_projection matrix
                VkPushConstantRange pushConstant{};
                pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                pushConstant.offset = 0;
                pushConstant.size = sizeof(glm::mat4);

                VkDescriptorSetLayout layouts[] = { DescriptorSetLayout.GetVkDescriptorSetLayout() };

                VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
                pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
                pipelineLayoutInfo.setLayoutCount = 1;
                pipelineLayoutInfo.pSetLayouts = layouts;
                pipelineLayoutInfo.pushConstantRangeCount = 1;
                pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

                VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
                if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr, &PipelineLayout) != VK_SUCCESS)
                    PIX_ASSERT_MSG(false, "Failed to create pipeline layout!");

                /////////////// Graphics Pipeline //////////////////////

                VulkanGraphicsPipeline GraphicsPipeline;
                GraphicsPipeline.Init(Context->m_Device, Renderpass.GetVKRenderpass())
                    .AddShaderStages(EquirectangularMapToCubeShader.GetVertexShader(), EquirectangularMapToCubeShader.GetFragmentShader())
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
                    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)), // left
                    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)), // up
                    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)), // down
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

                for (size_t i = 0; i < m_Framebuffers.size(); i++)
                {
                    VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };

                    VkRenderPassBeginInfo renderPassInfo = {};
                    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                    renderPassInfo.renderPass = Renderpass.GetVKRenderpass();
                    renderPassInfo.framebuffer = m_Framebuffers[i].GetVKFramebuffer();
                    renderPassInfo.renderArea = { {0, 0}, {m_Size, m_Size} };
                    renderPassInfo.clearValueCount = 1;
                    renderPassInfo.pClearValues = &clearValue;

                    glm::mat4 view_proj = captureProjection * captureViews[i]; //* faceRotations[i];

                    vkCmdPushConstants(CommandBuffer, PipelineLayout,
                        VK_SHADER_STAGE_VERTEX_BIT,
                        0, sizeof(glm::mat4), &view_proj);

                    vkCmdBeginRenderPass(CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
                    vkCmdBindPipeline(CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline.GetVkPipeline());

                    VkViewport viewport{};
                    viewport.x = 0.0f;
                    viewport.y = 0.0f;
                    viewport.width = (float)m_Size;
                    viewport.height = (float)m_Size;
                    viewport.minDepth = 0.0f;
                    viewport.maxDepth = 1.0f;

                    VkRect2D scissor{};
                    scissor.offset = { 0, 0 };
                    scissor.extent = { m_Size, m_Size };

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

                VulkanHelper::EndSingleTimeCommands(Context->m_Device, Context->m_Queue.m_Queue, Context->m_CommandPool, CommandBuffer);
            }

            VulkanCubemapHelper::TransitionImageLayout(m_Image, m_MipLevels, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, nullptr);

            // TODO:: Destroy All Vulkan Objects
            return true;
        }



        void VulkanHdrCubemap::Destroy()
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

            m_EquirectangularMap->Destroy();
            delete m_EquirectangularMap;
        }





















        ////////////////////// Cubemap Helper //////////////////


        void VulkanCubemapHelper::CreateImage
        (
            uint32_t width, uint32_t height,
            VkFormat format,
            uint32_t miplevels,
            VkImageTiling tiling,
            VkImageUsageFlags usage,
            VkMemoryPropertyFlags properties,
            VkImage& Image,
            VkDeviceMemory& ImageMemory
        )
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = miplevels;
            imageInfo.arrayLayers = 6; // Cubemap has 6 faces
            imageInfo.format = format;
            imageInfo.tiling = tiling;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = usage;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;


            VK_CHECK_RESULT(vkCreateImage(Context->m_Device, &imageInfo, nullptr, &Image),
                "Failed to create cubemap image!");

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(Context->m_Device, Image, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = FindMemoryType(Context->m_PhysDevice.GetSelected().m_physDevice, memRequirements.memoryTypeBits, properties);

            VK_CHECK_RESULT(vkAllocateMemory(Context->m_Device, &allocInfo, nullptr, &ImageMemory),
                "Failed to allocate cubemap image memory!");

            vkBindImageMemory(Context->m_Device, Image, ImageMemory, 0);
        }

        void VulkanCubemapHelper::CreateImageView
        (
            VkImage Image,
            VkFormat format,
            VkImageView& ImageView,
            uint32_t mipcount
        )
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = Image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;  // Specify cubemap view type
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = IsDepthFormat(format) ?
                VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = mipcount;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 6;  // All 6 faces

            VK_CHECK_RESULT(vkCreateImageView(Context->m_Device, &viewInfo, nullptr, &ImageView),
                "Failed to create cubemap image view!");
        }

        void VulkanCubemapHelper::CreateSampler(float maxLod, VkSampler& Sampler)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = 16.0f;
            samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = maxLod;

            VK_CHECK_RESULT(vkCreateSampler(Context->m_Device, &samplerInfo, nullptr, &Sampler),
                "Failed to create cubemap sampler!");
        }

        uint32_t VulkanCubemapHelper::FindMemoryType
        (
            VkPhysicalDevice PhysDevice,
            uint32_t TypeFilter,
            VkMemoryPropertyFlags Properties
        )
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


        void  VulkanCubemapHelper::TransitionImageLayout
        (
            VkImage Image,
            uint32_t MipLevels,
            VkImageLayout OldLayout,
            VkImageLayout NewLayout,
            VkCommandBuffer ExistingCommandBuffer
        )
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();


            VkCommandBuffer CommandBuffer;
            bool useExistingCommandBuffer = (ExistingCommandBuffer != VK_NULL_HANDLE);

            if (!useExistingCommandBuffer)
                CommandBuffer = VulkanHelper::BeginSingleTimeCommands(Context->m_Device, Context->m_CommandPool);
            else
                CommandBuffer = ExistingCommandBuffer;

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = OldLayout;
            barrier.newLayout = NewLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = Image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = MipLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 6;

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else if (OldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
            else if (OldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_UNDEFINED)
            {
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.dstAccessMask = 0;
                sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                destinationStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            }
            else if (OldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            {
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
            else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else if (OldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (OldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (OldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
            {
                barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else
            {
                throw std::runtime_error("Unsupported layout transition!");
            }

            vkCmdPipelineBarrier(CommandBuffer, sourceStage, destinationStage, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            if (!useExistingCommandBuffer)
                VulkanHelper::EndSingleTimeCommands(Context->m_Device, Context->m_Queue.m_Queue, Context->m_CommandPool, CommandBuffer);
        }
    }
}
