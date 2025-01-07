#pragma once
#include "VulkanGraphicsContext.h"

namespace PIX3D
{
	namespace VK
	{
		class VulkanHelper
		{
		public:
			static VkCommandBuffer BeginSingleTimeCommands(VkDevice Device, VkCommandPool CmdPool)
			{
				VkCommandBuffer CommandBuffer;

				// Allocate the command buffer
				VkCommandBufferAllocateInfo AllocInfo = {};
				AllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				AllocInfo.pNext = NULL;
				AllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
				AllocInfo.commandBufferCount = 1;
				AllocInfo.commandPool = CmdPool;

				VkResult res = vkAllocateCommandBuffers(Device, &AllocInfo, &CommandBuffer);
				VK_CHECK_RESULT(res, "vkAllocateCommandBuffers");

				// Begin the command buffer
				VkCommandBufferBeginInfo BeginInfo = {};
				BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				BeginInfo.pNext = NULL;
				BeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
				BeginInfo.pInheritanceInfo = NULL;

				res = vkBeginCommandBuffer(CommandBuffer, &BeginInfo);
				VK_CHECK_RESULT(res, "vkBeginCommandBuffer");

				return CommandBuffer;
			}

			static void EndSingleTimeCommands(VkDevice Device, VkQueue Queue, VkCommandPool CmdPool, VkCommandBuffer CommandBuffer)
			{
				VkResult res = vkEndCommandBuffer(CommandBuffer);
				VK_CHECK_RESULT(res, "vkEndCommandBuffer");

				VkSubmitInfo SubmitInfo = {};
				SubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				SubmitInfo.pNext = NULL,
				SubmitInfo.waitSemaphoreCount = 0;
				SubmitInfo.pWaitSemaphores = NULL;
				SubmitInfo.pWaitDstStageMask = NULL;
				SubmitInfo.commandBufferCount = 1;
				SubmitInfo.pCommandBuffers = &CommandBuffer;
				SubmitInfo.signalSemaphoreCount = 0;
				SubmitInfo.pSignalSemaphores = NULL;

				res = vkQueueSubmit(Queue, 1, &SubmitInfo, VK_NULL_HANDLE);
				VK_CHECK_RESULT(res, "vkQueueSubmit");

				res = vkQueueWaitIdle(Queue);
				VK_CHECK_RESULT(res, "vkQueueWaitIdle");

				vkFreeCommandBuffers(Device, CmdPool, 1, &CommandBuffer);
			}


			static VkRenderPass CreateSimpleRenderPass(VkDevice Device, VkFormat SwapChainSurfaceFormat)
			{
				VkAttachmentDescription AttachDesc = {
					.flags = 0,
					.format = SwapChainSurfaceFormat,
					.samples = VK_SAMPLE_COUNT_1_BIT,
					.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
					.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
				};

				VkAttachmentReference AttachRef = 
				{
					.attachment = 0,
					.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
				};

				VkSubpassDescription SubpassDesc = 
				{
					.flags = 0,
					.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
					.inputAttachmentCount = 0,
					.pInputAttachments = NULL,
					.colorAttachmentCount = 1,
					.pColorAttachments = &AttachRef,
					.pResolveAttachments = NULL,
					.pDepthStencilAttachment = NULL,
					.preserveAttachmentCount = 0,
					.pPreserveAttachments = NULL
				};

				VkRenderPassCreateInfo RenderPassCreateInfo = 
				{
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
					.pNext = NULL,
					.flags = 0,
					.attachmentCount = 1,
					.pAttachments = &AttachDesc,
					.subpassCount = 1,
					.pSubpasses = &SubpassDesc,
					.dependencyCount = 0,
					.pDependencies = NULL
				};

				VkRenderPass RenderPass;

				VkResult res = vkCreateRenderPass(Device, &RenderPassCreateInfo, NULL, &RenderPass);
				VK_CHECK_RESULT(res, "vkCreateRenderPass");

				PIX_DEBUG_SUCCESS("Created a simple render pass");

				return RenderPass;
			}

			static std::vector<VkFramebuffer> CreateSwapChainFrameBuffers(VulkanGraphicsContext* Context, VkRenderPass RenderPass, uint32_t Width, uint32_t Height)
			{
				std::vector<VkFramebuffer> frameBuffers;
				frameBuffers.resize(Context->m_SwapChainImages.size());

				VkResult res;

				for (uint32_t i = 0; i < Context->m_SwapChainImages.size(); i++)
				{

					VkFramebufferCreateInfo fbCreateInfo = {};
					fbCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
					fbCreateInfo.renderPass = RenderPass;
					fbCreateInfo.attachmentCount = 1;
					fbCreateInfo.pAttachments = &Context->m_SwapChainImageViews[i];
					fbCreateInfo.width = Width;
					fbCreateInfo.height = Height;
					fbCreateInfo.layers = 1;

					res = vkCreateFramebuffer(Context->m_Device, &fbCreateInfo, NULL, &frameBuffers[i]);
					VK_CHECK_RESULT(res, "vkCreateFramebuffer");
				}

				PIX_DEBUG_SUCCESS("Framebuffers created");

				return frameBuffers;
			}


			static  void BeginCommandBuffer(VkCommandBuffer commandbuffer, VkCommandBufferUsageFlags usageFlags = 0)
			{
				PIX_ASSERT_MSG(commandbuffer, "Command buffer must not be null");

				VkCommandBufferBeginInfo beginInfo{};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.pNext = nullptr;
				beginInfo.flags = usageFlags; // Usage flags (e.g., VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)
				beginInfo.pInheritanceInfo = nullptr; // Not used for primary command buffers

				VkResult res = vkBeginCommandBuffer(commandbuffer, &beginInfo);
				if (res != VK_SUCCESS)
				{
					VK_CHECK_RESULT(res ,"vkBeginCommandBuffer failed");
				}
			}

			static void CreateCommandBuffers(VkDevice device, VkCommandPool commandpool, uint32_t count, VkCommandBuffer* commandbuffer)
			{

				VkCommandBufferAllocateInfo cmdBufAllocInfo =
				{
				   .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				   .pNext = NULL,
				   .commandPool = commandpool,
				   .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				   .commandBufferCount = count
				};

				VkResult res = vkAllocateCommandBuffers(device, &cmdBufAllocInfo, commandbuffer);
				VK_CHECK_RESULT(res, "vkAllocateCommandBuffers");

				PIX_DEBUG_INFO_FORMAT("{} command buffers created", count);
			}


			static VkSemaphore CreateVulkanSemaphore(VkDevice Device)
			{
				VkSemaphoreCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

				VkSemaphore semaphore;
				VkResult res = vkCreateSemaphore(Device, &createInfo, NULL, &semaphore);
				VK_CHECK_RESULT(res, "vkCreateSemaphore error");
				return semaphore;
			}

			static VkImageView CreateImageView(VkDevice Device, VkImage Image, VkFormat Format, VkImageAspectFlags AspectFlags,
				VkImageViewType ViewType, uint32_t LayerCount, uint32_t mipLevels)
			{
				VkImageViewCreateInfo ViewInfo =
				{
					.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
					.pNext = NULL,
					.flags = 0,
					.image = Image,
					.viewType = ViewType,
					.format = Format,
					.components = {
						.r = VK_COMPONENT_SWIZZLE_IDENTITY,
						.g = VK_COMPONENT_SWIZZLE_IDENTITY,
						.b = VK_COMPONENT_SWIZZLE_IDENTITY,
						.a = VK_COMPONENT_SWIZZLE_IDENTITY
					},
					.subresourceRange = {
						.aspectMask = AspectFlags,
						.baseMipLevel = 0,
						.levelCount = mipLevels,
						.baseArrayLayer = 0,
						.layerCount = LayerCount
					}
				};

				VkImageView ImageView;
				VkResult res = vkCreateImageView(Device, &ViewInfo, NULL, &ImageView);
				VK_CHECK_RESULT(res, "vkCreateImageView");
				return ImageView;
			}
		private:

		};
	}
}
