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
        bool VulkanTexture::CreateColorAttachment(uint32_t Width, uint32_t Height, VkFormat Format, uint32_t MipLevels, bool ClampToEdge)
        {
            m_Width = Width;
            m_Height = Height;
            m_Format = Format;
            m_MipLevels = MipLevels;
            m_SamplerClampToEdge = ClampToEdge;

            auto usage = IsDepthFormat(Format)
                ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                : (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

            // Create the image with mip levels
            auto ImageAndMemory = VulkanTextureHelper::CreateImage(Width, Height, Format,
                VK_IMAGE_TILING_OPTIMAL,
                usage | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                m_MipLevels);

            m_Image = ImageAndMemory.Image;
            m_ImageMemory == ImageAndMemory.Memory;

            // Create image view with mip levels
            m_ImageView = VulkanTextureHelper::CreateImageView(m_Image, Format, 0, MipLevels);
            m_Sampler = VulkanTextureHelper::CreateSampler(m_MipLevels, ClampToEdge);

            return true;
        }

        bool VulkanTexture::LoadFromFile(const std::filesystem::path& FilePath, bool gen_mips, bool IsSRGB)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();
            if (!Context) return false;


            int texWidth, texHeight, texChannels;
            stbi_uc* pixels = stbi_load(FilePath.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

            if (!pixels) {
                return false;
            }

            m_Path = FilePath;
            m_MipLevels = gen_mips ? VulkanTextureHelper::CalculateMipLevels(texWidth, texHeight) : 1;
            m_Width = static_cast<uint32_t>(texWidth);
            m_Height = static_cast<uint32_t>(texHeight);
            m_Format = IsSRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

            VkDeviceSize imageSize = m_Width * m_Height * 4; // 4 bytes per RGBA pixel

            // Create staging buffer
            auto stagingBuffer = VulkanTextureHelper::CreateBuffer(
                imageSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            // Copy pixel data to staging buffer
            void* data;
            VK_CHECK_RESULT(vkMapMemory(Context->m_Device, stagingBuffer.Memory, 0, imageSize, 0, &data));
            memcpy(data, pixels, static_cast<size_t>(imageSize));
            vkUnmapMemory(Context->m_Device, stagingBuffer.Memory);

            stbi_image_free(pixels);

            // Create the image with appropriate usage flags
            VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            if (m_MipLevels > 1) {
                usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            }

            auto imageAndMemory = VulkanTextureHelper::CreateImage(
                m_Width, m_Height, m_Format,
                VK_IMAGE_TILING_OPTIMAL,
                usage,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                m_MipLevels);

            m_Image = imageAndMemory.Image;
            m_ImageMemory = imageAndMemory.Memory;

            // Transition image layout for copy
            VulkanTextureHelper::TransitionImageLayout(
                m_Image, m_Format, 0, m_MipLevels,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            // Copy data from staging buffer to image
            VulkanTextureHelper::CopyBufferToImage(stagingBuffer.Buffer, m_Image, m_Width, m_Height);

            if (m_MipLevels > 1)
            {
                VkCommandBuffer commandBuffer = VulkanHelper::BeginSingleTimeCommands(Context->m_Device, Context->m_CommandPool);
                GenerateMipmaps(commandBuffer);
                VulkanHelper::EndSingleTimeCommands(Context->m_Device, Context->m_Queue.m_Queue, Context->m_CommandPool, commandBuffer);
            }
            else
            {
                VulkanTextureHelper::TransitionImageLayout(
                    m_Image, m_Format, 0, m_MipLevels,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            // Create image view and sampler
            m_ImageView = VulkanTextureHelper::CreateImageView(m_Image, m_Format, 0, m_MipLevels);
            m_Sampler = VulkanTextureHelper::CreateSampler(m_MipLevels, false);

            stagingBuffer.Destroy(Context->m_Device);
            return true;
        }

        bool VulkanTexture::LoadFromHDRFile(const std::filesystem::path& FilePath, VkFormat format, uint32_t miplevels)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();
            if (!Context || FilePath.extension() != ".hdr")
                return false;

            m_MipLevels = miplevels;

            int texWidth, texHeight, texChannels;
            stbi_set_flip_vertically_on_load(true);
            float* pixels = stbi_loadf(FilePath.string().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            stbi_set_flip_vertically_on_load(false);

            if (!pixels) {
                return false;
            }

            m_Width = static_cast<uint32_t>(texWidth);
            m_Height = static_cast<uint32_t>(texHeight);
            m_Format = format;

            VkDeviceSize pixelSize = sizeof(float) * 4; // RGBA float format
            VkDeviceSize imageSize = m_Width * m_Height * pixelSize;

            // Create staging buffer
            auto stagingBuffer = VulkanTextureHelper::CreateBuffer(
                imageSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            void* data;
            VK_CHECK_RESULT(vkMapMemory(Context->m_Device, stagingBuffer.Memory, 0, imageSize, 0, &data));
            memcpy(data, pixels, static_cast<size_t>(imageSize));
            vkUnmapMemory(Context->m_Device, stagingBuffer.Memory);

            stbi_image_free(pixels);

            VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            if (m_MipLevels > 1)
                usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

            auto imageAndMemory = VulkanTextureHelper::CreateImage(
                m_Width, m_Height, m_Format,
                VK_IMAGE_TILING_OPTIMAL,
                usage,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                m_MipLevels);

            m_Image = imageAndMemory.Image;
            m_ImageMemory = imageAndMemory.Memory;

            VulkanTextureHelper::TransitionImageLayout(
                m_Image, m_Format, 0, m_MipLevels,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VulkanTextureHelper::CopyBufferToImage(stagingBuffer.Buffer, m_Image, m_Width, m_Height);

            if (m_MipLevels > 1)
            {
                VkCommandBuffer commandBuffer = VulkanHelper::BeginSingleTimeCommands(Context->m_Device, Context->m_CommandPool);
                GenerateMipmaps(commandBuffer);
                VulkanHelper::EndSingleTimeCommands(Context->m_Device, Context->m_Queue.m_Queue, Context->m_CommandPool, commandBuffer);
            }
            else
            {
                VulkanTextureHelper::TransitionImageLayout(
                    m_Image, m_Format, 0, m_MipLevels,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            m_ImageView = VulkanTextureHelper::CreateImageView(m_Image, m_Format, 0, m_MipLevels);
            m_Sampler = VulkanTextureHelper::CreateSampler(m_MipLevels, false);

            stagingBuffer.Destroy(Context->m_Device);
            return true;
        }

        bool VulkanTexture::LoadFromData(void* Data, uint32_t Width, uint32_t Height, VkFormat Format)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();
            if (!Context || !Data || Width == 0 || Height == 0)
                return false;

            m_Width = Width;
            m_Height = Height;
            m_Format = Format;

            VkDeviceSize imageSize = Width * Height * GetBytesPerPixel(Format);

            // Create staging buffer
            auto stagingBuffer = VulkanTextureHelper::CreateBuffer(
                imageSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

            // Copy data to staging buffer
            void* mappedData = nullptr;
            VkResult res = vkMapMemory(Context->m_Device, stagingBuffer.Memory, 0, imageSize, 0, &mappedData);
            VK_CHECK_RESULT(res, "Failed to map staging buffer memory");

            memcpy(mappedData, Data, static_cast<size_t>(imageSize));
            vkUnmapMemory(Context->m_Device, stagingBuffer.Memory);

            // Create the image
            auto imageAndMemory = VulkanTextureHelper::CreateImage(
                Width, Height, Format,
                VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                1);

            m_Image = imageAndMemory.Image;
            m_ImageMemory = imageAndMemory.Memory;

            // Transition image layout for copy
            VulkanTextureHelper::TransitionImageLayout(
                m_Image, Format, 0, 1,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            // Copy data from staging buffer to image
            VulkanTextureHelper::CopyBufferToImage(stagingBuffer.Buffer, m_Image, Width, Height);

            // Transition to shader read layout
            VulkanTextureHelper::TransitionImageLayout(
                m_Image, Format, 0, 1,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            // Create image view and sampler
            m_ImageView = VulkanTextureHelper::CreateImageView(m_Image, Format);
            m_Sampler = VulkanTextureHelper::CreateSampler(1, false);

            // Cleanup staging buffer
            stagingBuffer.Destroy(Context->m_Device);

            return true;
        }

        void VulkanTexture::Destroy()
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            auto m_Device = Context->m_Device;

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

                if (m_ImGuiDescriptorset != VK_NULL_HANDLE)
                {
                    // TODO:: Remove Descriptor Set Created With ImGui
                }
            }
        }














        ///////////////// Vulkan Texture Helper ///////////////////////////////










        VulkanBufferAndMemory VulkanTextureHelper::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            VulkanBufferAndMemory Result;

            VkBufferCreateInfo BufferInfo{};
            BufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            BufferInfo.size = size;
            BufferInfo.usage = usage;
            BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            VkResult res = vkCreateBuffer(Context->m_Device, &BufferInfo, nullptr, &Result.Buffer);
            VK_CHECK_RESULT(res, "Failed to create buffer");

            VkMemoryRequirements MemRequirements;
            vkGetBufferMemoryRequirements(Context->m_Device, Result.Buffer, &MemRequirements);

            VkMemoryAllocateInfo AllocInfo{};
            AllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            AllocInfo.allocationSize = MemRequirements.size;
            AllocInfo.memoryTypeIndex = FindMemoryType(MemRequirements.memoryTypeBits, properties);

            res = vkAllocateMemory(Context->m_Device, &AllocInfo, nullptr, &Result.Memory);
            VK_CHECK_RESULT(res, "Failed to allocate buffer memory");

            Result.AllocationSize = AllocInfo.allocationSize;

            vkBindBufferMemory(Context->m_Device, Result.Buffer, Result.Memory, 0);

            return Result;
        }

        uint32_t VulkanTextureHelper::FindMemoryType(uint32_t type_filter, VkMemoryPropertyFlags properties)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            VkPhysicalDeviceMemoryProperties MemProperties;
            vkGetPhysicalDeviceMemoryProperties(Context->m_PhysDevice.GetSelected().m_physDevice, &MemProperties);

            for (uint32_t i = 0; i < MemProperties.memoryTypeCount; i++)
            {
                if ((type_filter & (1 << i)) &&
                    (MemProperties.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    return i;
                }
            }

            PIX_ASSERT_MSG(false, "Failed to find suitable memory type!");
            return 0;
        }



        VulkanImageAndMemory VulkanTextureHelper::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, uint32_t mip_count)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = mip_count;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = tiling;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = usage;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

            VulkanImageAndMemory Image;
            if (vkCreateImage(Context->m_Device, &imageInfo, nullptr, &Image.Image) != VK_SUCCESS)
                PIX_ASSERT_MSG(false, "Failed to create image!");

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(Context->m_Device, Image.Image, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

            if (vkAllocateMemory(Context->m_Device, &allocInfo, nullptr, &Image.Memory) != VK_SUCCESS)
                PIX_ASSERT_MSG(false, "Failed to allocate image memory!");

            vkBindImageMemory(Context->m_Device, Image.Image, Image.Memory, 0);

            return Image;
        }

        VkImageView VulkanTextureHelper::CreateImageView(VkImage image, VkFormat format, uint32_t base_mip, uint32_t mip_count)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = IsDepthFormat(format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = base_mip;
            viewInfo.subresourceRange.levelCount = mip_count;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            VkImageView View;
            if (vkCreateImageView(Context->m_Device, &viewInfo, nullptr, &View) != VK_SUCCESS)
                PIX_ASSERT_MSG(false,  "Failed to create texture image view!");

            return View;
        }

        VkSampler VulkanTextureHelper::CreateSampler(float mip_count, bool clamp_to_edge)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = clamp_to_edge ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = clamp_to_edge ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = clamp_to_edge ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = 16.0f;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = mip_count;

            VkSampler Sampler;
            if (vkCreateSampler(Context->m_Device, &samplerInfo, nullptr, &Sampler) != VK_SUCCESS)
                PIX_ASSERT_MSG(false, "Failed to create texture sampler!");

            return Sampler;
        }

        void VulkanTexture::GenerateMipmaps(VkCommandBuffer commandBuffer)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            // Check if image format supports linear blitting
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(Context->m_PhysDevice.GetSelected().m_physDevice, m_Format, &formatProperties);

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

        void VulkanTextureHelper::TransitionImageLayout(VkImage Image, VkFormat Format, uint32_t base_mip,uint32_t mip_count, VkImageLayout OldLayout, VkImageLayout NewLayout, VkCommandBuffer ExistingCommandBuffer)
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
            barrier.subresourceRange.baseMipLevel = base_mip;
            barrier.subresourceRange.levelCount = mip_count;
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
            else if (OldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR && NewLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
            else if (OldLayout == VK_IMAGE_LAYOUT_UNDEFINED && NewLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = 0;
                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            }
            else if (OldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
            {
                barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                barrier.dstAccessMask = 0;
                sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
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

        void VulkanTextureHelper::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            VkCommandBuffer CommandBuffer = VulkanHelper::BeginSingleTimeCommands(Context->m_Device, Context->m_CommandPool);

            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = { width, height, 1 };

            vkCmdCopyBufferToImage(CommandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            VulkanHelper::EndSingleTimeCommands(Context->m_Device, Context->m_Queue.m_Queue, Context->m_CommandPool, CommandBuffer);
        }

        void VulkanTextureHelper::CopyTextureWithAllMips(VkImage src_image, VkFormat src_format, VkImage dst_image, VkFormat dst_format, uint32_t width, uint32_t height, uint32_t mip_count, VkCommandBuffer existing_commandbuffer)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            VkCommandBuffer commandBuffer;
            bool useExistingCommandBuffer = (existing_commandbuffer != VK_NULL_HANDLE);

            if (!useExistingCommandBuffer)
                commandBuffer = VulkanHelper::BeginSingleTimeCommands(Context->m_Device, Context->m_CommandPool);
            else
                commandBuffer = existing_commandbuffer;

            // Transition source image to transfer source layout
            VulkanTextureHelper::TransitionImageLayout(src_image, src_format, 0, mip_count,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                commandBuffer
            );

            // Transition destination image to transfer destination layout
            VulkanTextureHelper::TransitionImageLayout(dst_image, dst_format, 0, mip_count,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                commandBuffer
            );

            std::vector<VkImageCopy> copyRegions;
            copyRegions.reserve(mip_count);

            // Set up the copy regions for each mip level
            for (uint32_t mipLevel = 0; mipLevel < mip_count; mipLevel++)
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
                uint32_t mipWidth = std::max(width >> mipLevel, 1u);
                uint32_t mipHeight = std::max(height >> mipLevel, 1u);

                copyRegion.extent =
                {
                    mipWidth,
                    mipHeight,
                    1
                };

                copyRegions.push_back(copyRegion);
            }

            // Execute the copy command for all mip levels at once
            vkCmdCopyImage(
                commandBuffer,
                src_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                dst_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                static_cast<uint32_t>(copyRegions.size()),
                copyRegions.data()
            );

            // Transition images back to shader read layout
            VulkanTextureHelper::TransitionImageLayout(src_image, src_format, 0, mip_count,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                commandBuffer
            );

            VulkanTextureHelper::TransitionImageLayout(dst_image, dst_format, 0, mip_count,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                commandBuffer
            );

            if (!useExistingCommandBuffer)
                VulkanHelper::EndSingleTimeCommands(Context->m_Device, Context->m_Queue.m_Queue, Context->m_CommandPool, commandBuffer);
        }

        uint32_t VulkanTextureHelper::CalculateMipLevels(uint32_t width, uint32_t height)
        {
            uint32_t MaxDimension = std::max(width, height);
            return static_cast<uint32_t>(std::floor(std::log2(MaxDimension))) + 1;
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
