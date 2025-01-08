#include "VulkanTexture.h"
#include <stdexcept>
#include <Engine/Engine.hpp>
#include "VulkanHelper.h"

#include <Utils/stb_image.h>

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

        bool VulkanTexture::LoadFromFile(const std::filesystem::path& FilePath, bool IsSRGB)
        {
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
            CreateImage(m_Width, m_Height, vulkanFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            // Transition image layout for copy
            TransitionImageLayout(vulkanFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            // Copy data from staging buffer to image
            CopyBufferToImage(stagingBuffer.m_Buffer, m_Width, m_Height);

            // Transition to shader read layout
            TransitionImageLayout(vulkanFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            // Create image view and sampler
            CreateImageView(vulkanFormat);
            CreateSampler();

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
            imageInfo.mipLevels = 1;
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
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_Device, &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create texture image view!");
            }
        }

        void VulkanTexture::CreateSampler()
        {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = 16.0f;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = 0.0f;

            if (vkCreateSampler(m_Device, &samplerInfo, nullptr, &m_Sampler) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create texture sampler!");
            }
        }

        void VulkanTexture::TransitionImageLayout(VkFormat Format, VkImageLayout OldLayout, VkImageLayout NewLayout)
        {
            VkCommandBuffer CommandBuffer = VulkanHelper::BeginSingleTimeCommands(m_Device, m_CmdPool);

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = OldLayout;
            barrier.newLayout = NewLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = m_Image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
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
            else if (OldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && NewLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
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

        VkFormat VulkanTexture::GetVulkanFormat(TextureFormat Format)
        {
            switch (Format)
            {
            case TextureFormat::R8: return VK_FORMAT_R8_UNORM;
            case TextureFormat::RG8: return VK_FORMAT_R8G8_UNORM;
            case TextureFormat::RGB8: return VK_FORMAT_R8G8B8_UNORM;
            case TextureFormat::RGBA8: return VK_FORMAT_R8G8B8A8_UNORM;
            case TextureFormat::RGBA16F: return VK_FORMAT_R16G16B16A16_SFLOAT;
            case TextureFormat::RGBA32F: return VK_FORMAT_R32G32B32A32_SFLOAT;
            default: throw std::runtime_error("Unsupported texture format!");
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
    }
}
