#include "VulkanTexture.h"
#include <stdexcept>
#include <Engine/Engine.hpp>
#include "VulkanHelper.h"

#include <Utils/stb_image.h>
#include <imgui_impl_vulkan.h>

namespace PIX3D
{
    namespace VK
    {
        void VulkanTexture::Create()
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            m_Device = Context->m_Device;
            m_PhysicalDevice = Context->m_PhysDevice.GetSelected().m_physDevice;
            m_Queue = Context->m_Queue.GetVkQueue();
            m_CmdPool = Context->m_CommandPool;
        }

        bool VulkanTexture::CreateColorAttachment(uint32_t Width, uint32_t Height, TextureFormat Format, uint32_t MipLevels, bool ClampToEdge)
        {
            m_Width = Width;
            m_Height = Height;
            m_Format = Format;
            m_MipLevels = MipLevels;
            m_SamplerClampToEdge = ClampToEdge;

            auto usage = IsDepthFormat(GetVulkanFormat(Format))
                ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                : (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

            // Create the image with mip levels
            CreateImage(Width, Height, GetVulkanFormat(Format),
                VK_IMAGE_TILING_OPTIMAL,
                usage | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            // Create image view with mip levels
            CreateImageView(GetVulkanFormat(Format));
            CreateSampler(static_cast<float>(m_MipLevels - 1));

            return true;
        }

        bool VulkanTexture::LoadFromFile(const std::filesystem::path& FilePath, uint32_t miplevels, bool IsSRGB)
        {
            m_MipLevels = miplevels;

            int texWidth, texHeight, texChannels;
            stbi_uc* pixels = stbi_load(FilePath.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

            if (!pixels) {
                throw std::runtime_error("Failed to load texture image!");
            }

            m_Width = static_cast<uint32_t>(texWidth);
            m_Height = static_cast<uint32_t>(texHeight);

            // Always use RGBA8 format for loaded images, but handle SRGB if requested
            m_Format = TextureFormat::RGBA8;
            VkFormat vulkanFormat = IsSRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

            VkDeviceSize imageSize = m_Width * m_Height * 4; // 4 bytes per RGBA pixel

            // Create staging buffer
            BufferAndMemory stagingBuffer = CreateBuffer(
                imageSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_PhysicalDevice
            );

            // Copy pixel data to staging buffer
            void* data;
            vkMapMemory(m_Device, stagingBuffer.m_Memory, 0, imageSize, 0, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
            vkUnmapMemory(m_Device, stagingBuffer.m_Memory);

            // Free original pixel data
            stbi_image_free(pixels);

            // Create the image

            VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

            if (m_MipLevels > 1)
                usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

            CreateImage(m_Width, m_Height, vulkanFormat, VK_IMAGE_TILING_OPTIMAL,
                usage,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            // Transition image layout for copy
            TransitionImageLayout(vulkanFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            // Copy data from staging buffer to image
            CopyBufferToImage(stagingBuffer.m_Buffer, m_Width, m_Height);

            if (m_MipLevels > 1)
            {
                auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

                VkCommandBuffer Commandbuffer = VulkanHelper::BeginSingleTimeCommands(Context->m_Device, Context->m_CommandPool);
                GenerateMipmaps(Commandbuffer);
                VulkanHelper::EndSingleTimeCommands(Context->m_Device, Context->m_Queue.m_Queue, Context->m_CommandPool, Commandbuffer);
            }
            else
                TransitionImageLayout(vulkanFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            // Create image view and sampler
            CreateImageView(vulkanFormat);
            CreateSampler(static_cast<float>(m_MipLevels - 1));

            // Cleanup staging buffer
            stagingBuffer.Destroy(m_Device);

            return true;
        }

        bool VulkanTexture::LoadFromHDRFile(const std::filesystem::path& FilePath, TextureFormat format, uint32_t miplevels)
        {
            m_MipLevels = miplevels;

            int texWidth, texHeight, texChannels;
            PIX_ASSERT_MSG((FilePath.extension().string() == ".hdr"), "file is not .hdr file!");

            stbi_set_flip_vertically_on_load(true);
            float* pixels = stbi_loadf(FilePath.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            stbi_set_flip_vertically_on_load(false);

            PIX_ASSERT_MSG(pixels, "failed to load hdr file!");

            m_Width = static_cast<uint32_t>(texWidth);
            m_Height = static_cast<uint32_t>(texHeight);

            // Always use RGBA8 format for loaded images, but handle SRGB if requested
            m_Format = format;
            VkFormat vulkanFormat = GetVulkanFormat(format);

            VkDeviceSize imageSize = m_Width * m_Height * GetBytesPerPixel(format); // 4 bytes per RGBA pixel for example

            // Create staging buffer
            BufferAndMemory stagingBuffer = CreateBuffer(
                imageSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_PhysicalDevice
            );

            // Copy pixel data to staging buffer
            void* data;
            vkMapMemory(m_Device, stagingBuffer.m_Memory, 0, imageSize, 0, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
            vkUnmapMemory(m_Device, stagingBuffer.m_Memory);

            // Free original pixel data
            stbi_image_free(pixels);

            // Create the image

            VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

            if (m_MipLevels > 1)
                usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

            CreateImage(m_Width, m_Height, vulkanFormat, VK_IMAGE_TILING_OPTIMAL,
                usage,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            // Transition image layout for copy
            TransitionImageLayout(vulkanFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            // Copy data from staging buffer to image
            CopyBufferToImage(stagingBuffer.m_Buffer, m_Width, m_Height);

            if (m_MipLevels > 1)
            {
                auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

                VkCommandBuffer Commandbuffer = VulkanHelper::BeginSingleTimeCommands(Context->m_Device, Context->m_CommandPool);
                GenerateMipmaps(Commandbuffer);
                VulkanHelper::EndSingleTimeCommands(Context->m_Device, Context->m_Queue.m_Queue, Context->m_CommandPool, Commandbuffer);
            }
            else
                TransitionImageLayout(vulkanFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            // Create image view and sampler
            CreateImageView(vulkanFormat);
            CreateSampler(static_cast<float>(m_MipLevels - 1));

            // Cleanup staging buffer
            stagingBuffer.Destroy(m_Device);

            return true;
        }

        bool VulkanTexture::LoadFromData(void* Data, uint32_t Width, uint32_t Height, TextureFormat Format)
        {
            if (!Data || Width == 0 || Height == 0)
                return false;

            m_Width = Width;
            m_Height = Height;
            m_Format = Format;

            VkFormat vulkanFormat = GetVulkanFormat(Format);
            VkDeviceSize imageSize = Width * Height * GetBytesPerPixel(Format);

            // Create staging buffer
            BufferAndMemory stagingBuffer = CreateBuffer(
                imageSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                m_PhysicalDevice
            );

            // Copy data to staging buffer
            void* mappedData = nullptr;
            VkResult res = vkMapMemory(m_Device, stagingBuffer.m_Memory, 0, imageSize, 0, &mappedData);
            VK_CHECK_RESULT(res, "Failed to map staging buffer memory");

            memcpy(mappedData, Data, static_cast<size_t>(imageSize));
            vkUnmapMemory(m_Device, stagingBuffer.m_Memory);

            // Create the image
            CreateImage(Width, Height, vulkanFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            // Transition image layout for copy
            TransitionImageLayout(vulkanFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            // Copy data from staging buffer to image
            CopyBufferToImage(stagingBuffer.m_Buffer, Width, Height);

            // Transition to shader read layout
            TransitionImageLayout(vulkanFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            // Create image view and sampler
            CreateImageView(vulkanFormat);
            CreateSampler();

            // Cleanup staging buffer
            stagingBuffer.Destroy(m_Device);

            return true;
        }

        void VulkanTexture::Destroy()
        {
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

        BufferAndMemory VulkanTexture::CreateBuffer(VkDeviceSize Size,
            VkBufferUsageFlags Usage,
            VkMemoryPropertyFlags Properties,
            VkPhysicalDevice PhysDevice)
        {
            BufferAndMemory Result;

            VkBufferCreateInfo BufferInfo{};
            BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            BufferInfo.size = Size;
            BufferInfo.usage = Usage;
            BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VkResult res = vkCreateBuffer(m_Device, &BufferInfo, nullptr, &Result.m_Buffer);
            VK_CHECK_RESULT(res, "Failed to create buffer");

            VkMemoryRequirements MemRequirements;
            vkGetBufferMemoryRequirements(m_Device, Result.m_Buffer, &MemRequirements);

            VkMemoryAllocateInfo AllocInfo{};
            AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            AllocInfo.allocationSize = MemRequirements.size;
            AllocInfo.memoryTypeIndex = FindMemoryType(PhysDevice,
                MemRequirements.memoryTypeBits,
                Properties);

            res = vkAllocateMemory(m_Device, &AllocInfo, nullptr, &Result.m_Memory);
            VK_CHECK_RESULT(res, "Failed to allocate buffer memory");

            Result.m_AllocationSize = AllocInfo.allocationSize;

            vkBindBufferMemory(m_Device, Result.m_Buffer, Result.m_Memory, 0);

            return Result;
        }

        uint32_t VulkanTexture::FindMemoryType(VkPhysicalDevice PhysDevice,
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



        void VulkanTexture::CreateImage(uint32_t Width, uint32_t Height, VkFormat Format,
            VkImageTiling Tiling, VkImageUsageFlags Usage, VkMemoryPropertyFlags Properties)
        {
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = Width;
            imageInfo.extent.height = Height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = m_MipLevels;
            imageInfo.arrayLayers = 1;
            imageInfo.format = Format;
            imageInfo.tiling = Tiling;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = Usage;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

            if (vkCreateImage(m_Device, &imageInfo, nullptr, &m_Image) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create image!");
            }

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(m_Device, m_Image, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = FindMemoryType(m_PhysicalDevice, memRequirements.memoryTypeBits, Properties);

            if (vkAllocateMemory(m_Device, &allocInfo, nullptr, &m_ImageMemory) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to allocate image memory!");
            }

            vkBindImageMemory(m_Device, m_Image, m_ImageMemory, 0);
        }

        void VulkanTexture::CreateImageView(VkFormat Format)
        {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = m_Image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = Format;
            viewInfo.subresourceRange.aspectMask = IsDepthFormat(GetVKormat()) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = m_MipLevels;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_Device, &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create texture image view!");
            }
        }

        void VulkanTexture::CreateSampler(float MaxLod)
        {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = m_SamplerClampToEdge ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = m_SamplerClampToEdge ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = m_SamplerClampToEdge ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = 16.0f;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = MaxLod;

            if (vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create texture sampler!");
            }
        }

        void VulkanTexture::GenerateMipmaps(VkCommandBuffer commandBuffer)
        {
            // Check if image format supports linear blitting
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, GetVKormat(), &formatProperties);

            if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
                PIX_ASSERT_MSG(false, "Texture image format does not support linear blitting!");

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image = m_Image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = 1;

            int32_t mipWidth = m_Width;
            int32_t mipHeight = m_Height;

            for (uint32_t i = 1; i < m_MipLevels; i++)
            {
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                vkCmdPipelineBarrier(commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);

                VkImageBlit blit{};
                blit.srcOffsets[0] = { 0, 0, 0 };
                blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount = 1;
                blit.dstOffsets[0] = { 0, 0, 0 };
                blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel = i;
                blit.dstSubresource.baseArrayLayer = 0;
                blit.dstSubresource.layerCount = 1;

                vkCmdBlitImage(commandBuffer,
                    m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &blit,
                    VK_FILTER_LINEAR);

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);

                if (mipWidth > 1) mipWidth /= 2;
                if (mipHeight > 1) mipHeight /= 2;
            }

            barrier.subresourceRange.baseMipLevel = m_MipLevels - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);
        }

        void VulkanTexture::TransitionImageLayout(VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout, VkCommandBuffer ExistingCommandBuffer)
        {
            VkCommandBuffer CommandBuffer;
            bool useExistingCommandBuffer = (ExistingCommandBuffer != VK_NULL_HANDLE);

            if (!useExistingCommandBuffer)
                CommandBuffer = VulkanHelper::BeginSingleTimeCommands(m_Device, m_CmdPool);
            else
                CommandBuffer = ExistingCommandBuffer;

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = OldLayout;
            barrier.newLayout = NewLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = m_Image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = m_MipLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

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
                VulkanHelper::EndSingleTimeCommands(m_Device, m_Queue, m_CmdPool, CommandBuffer);
        }

        void VulkanTexture::CopyBufferToImage(VkBuffer Buffer, uint32_t Width, uint32_t Height)
        {
            VkCommandBuffer CommandBuffer = VulkanHelper::BeginSingleTimeCommands(m_Device, m_CmdPool);

            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = { Width, Height, 1 };

            vkCmdCopyBufferToImage(CommandBuffer, Buffer, m_Image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            VulkanHelper::EndSingleTimeCommands(m_Device, m_Queue, m_CmdPool, CommandBuffer);
        }

        void VulkanTexture::CopyFromTexture(VulkanTexture* sourceTexture, VkCommandBuffer existingCommandBuffer)
        {
            // Ensure the dimensions match
            if (m_Width != sourceTexture->GetWidth() || m_Height != sourceTexture->GetHeight())
                PIX_ASSERT_MSG(false, "Source and destination textures must have the same dimensions!");

            // Ensure mip levels match
            if (m_MipLevels != sourceTexture->m_MipLevels)
                PIX_ASSERT_MSG(false,  "Source and destination textures must have the same number of mip levels!");

            VkCommandBuffer commandBuffer;
            bool useExistingCommandBuffer = (existingCommandBuffer != VK_NULL_HANDLE);

            if (!useExistingCommandBuffer)
                commandBuffer = VulkanHelper::BeginSingleTimeCommands(m_Device, m_CmdPool);
            else
                commandBuffer = existingCommandBuffer;

            // Transition source image to transfer source layout
            sourceTexture->TransitionImageLayout(
                sourceTexture->GetVKormat(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                commandBuffer
            );

            // Transition destination image to transfer destination layout
            TransitionImageLayout(
                GetVKormat(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                commandBuffer
            );

            std::vector<VkImageCopy> copyRegions;
            copyRegions.reserve(m_MipLevels);

            // Set up the copy regions for each mip level
            for (uint32_t mipLevel = 0; mipLevel < m_MipLevels; mipLevel++)
            {
                VkImageCopy copyRegion{};
                copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.srcSubresource.mipLevel = mipLevel;
                copyRegion.srcSubresource.baseArrayLayer = 0;
                copyRegion.srcSubresource.layerCount = 1;
                copyRegion.srcOffset = { 0, 0, 0 };

                copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                copyRegion.dstSubresource.mipLevel = mipLevel;
                copyRegion.dstSubresource.baseArrayLayer = 0;
                copyRegion.dstSubresource.layerCount = 1;
                copyRegion.dstOffset = { 0, 0, 0 };

                // Calculate mip level dimensions
                uint32_t mipWidth = std::max(m_Width >> mipLevel, 1u);
                uint32_t mipHeight = std::max(m_Height >> mipLevel, 1u);

                copyRegion.extent = {
                    mipWidth,
                    mipHeight,
                    1
                };

                copyRegions.push_back(copyRegion);
            }

            // Execute the copy command for all mip levels at once
            vkCmdCopyImage(
                commandBuffer,
                sourceTexture->GetVKImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                static_cast<uint32_t>(copyRegions.size()),
                copyRegions.data()
            );

            // Transition images back to shader read layout
            sourceTexture->TransitionImageLayout(
                sourceTexture->GetVKormat(),
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                commandBuffer
            );

            TransitionImageLayout(
                GetVKormat(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                commandBuffer
            );

            if (!useExistingCommandBuffer)
                VulkanHelper::EndSingleTimeCommands(m_Device, m_Queue, m_CmdPool, commandBuffer);
        }

        VkFormat VulkanTexture::GetVulkanFormat(TextureFormat Format) const
        {
            switch (Format)
            {
            case TextureFormat::R8: return VK_FORMAT_R8_UNORM;
            case TextureFormat::RG8: return VK_FORMAT_R8G8_UNORM;
            case TextureFormat::RGB8: return VK_FORMAT_R8G8B8_UNORM;
            case TextureFormat::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
            case TextureFormat::RGBA8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
            case TextureFormat::RGB16F: return VK_FORMAT_R16G16B16_SFLOAT;
            case TextureFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
            case TextureFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;

            // Depth formats
            case TextureFormat::DEPTH16: return VK_FORMAT_D16_UNORM;
            case TextureFormat::DEPTH24: return VK_FORMAT_X8_D24_UNORM_PACK32; // Depth24 with 8 unused bits
            case TextureFormat::DEPTH32: return VK_FORMAT_D32_SFLOAT;
            case TextureFormat::DEPTH24_STENCIL8: return VK_FORMAT_D24_UNORM_S8_UINT;
            case TextureFormat::DEPTH32_STENCIL8: return VK_FORMAT_D32_SFLOAT_S8_UINT;

            default:
                PIX_ASSERT_MSG(false, "Unsupported texture format!");
            }
        }

        uint32_t VulkanTexture::GetBytesPerPixel(TextureFormat Format)
        {
            switch (Format)
            {
            case TextureFormat::R8: return 1;
            case TextureFormat::RG8: return 2;
            case TextureFormat::RGB8: return 3;
            case TextureFormat::RGBA8: return 4;
            case TextureFormat::RGBA16F: return 8;
            case TextureFormat::RGBA32F: return 16;
            default: throw std::runtime_error("Unsupported texture format!");
            }
        }

        VkDescriptorSet VulkanTexture::GetImGuiDescriptorSet()
        {
            if (!m_ImGuiDescriptorset)
            {
                m_ImGuiDescriptorset = ImGui_ImplVulkan_AddTexture(
                    GetSampler(),
                    GetImageView(),
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                );
            }

            return m_ImGuiDescriptorset;
        }
    }
}
