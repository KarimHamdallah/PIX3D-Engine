#include "VulkanFullScreenQuadRenderpass.h"
#include <Engine/Engine.hpp>
#include "VulkanHelper.h"


namespace PIX3D
{
	namespace VK
	{
		void VulkanFullScreenQuadRenderpass::Init(uint32_t width, uint32_t height, VulkanTexture* ColorAttachment)
		{
			auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();


			// load shader
			m_Shader.LoadFromFile("../PIX3D/res/vk shaders/full_screen_quad.vert", "../PIX3D/res/vk shaders/full_screen_quad.frag");

			// qud vertex and index buffers

			const std::vector<float> vertices =
			{
				// positions          // tex coords
				-1.0f, -1.0f, 0.0f,  0.0f, 1.0f,  // bottom left
				 1.0f, -1.0f, 0.0f,  1.0f, 1.0f,  // bottom right
				 1.0f,  1.0f, 0.0f,  1.0f, 0.0f,  // top right
				-1.0f,  1.0f, 0.0f,  0.0f, 0.0f   // top left
			};

			m_VertexBuffer.Create();
			m_VertexBuffer.FillData(vertices.data(), vertices.size() * sizeof(float));

			const std::vector<uint32_t> indices =
			{
				0, 1, 2,  // first triangle
				2, 3, 0   // second triangle
			};

			m_IndexBuffer.Create();
			m_IndexBuffer.FillData(indices.data(), indices.size() * sizeof(uint32_t));

			// vertex input
			{
				m_VertexInputLayout
					.AddAttribute(VK::VertexAttributeFormat::Float3)  // position
					.AddAttribute(VK::VertexAttributeFormat::Float2);  // texcoords
			}

			auto VertexBindingDescription = m_VertexInputLayout.GetBindingDescription();
			auto VertexAttributeDescriptions = m_VertexInputLayout.GetAttributeDescriptions();


			// descriptor layout -- descripe shader resources
			m_DescriptorSetLayout
				.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.Build();

			// descriptor set -- save resources at specific bindings -- matches shader layout
			m_DescriptorSet.Init(m_DescriptorSetLayout)
				.AddTexture(0, *ColorAttachment)
				.Build();


			// renderpass -- descripe attachment format and layout
			m_Renderpass
				.Init(Context->m_Device)

				.AddColorAttachment(
					Context->m_SwapChainSurfaceFormat.format,
					VK_SAMPLE_COUNT_1_BIT,
					VK_ATTACHMENT_LOAD_OP_CLEAR,
					VK_ATTACHMENT_STORE_OP_STORE,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
				.AddSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
				.Build();

			// renderpass framebuffers
			m_Framebuffers.resize(Context->m_SwapChainImages.size());

			for (size_t i = 0; i < m_Framebuffers.size(); i++)
			{
				m_Framebuffers[i].Init(Context->m_Device, m_Renderpass.GetVKRenderpass(), width, height)
					.AddSwapChainAttachment(i)
					.Build();
			}


			// pipeline layout (descriptor sets or push constants)
			VkPipelineLayout pipelineLayout = nullptr;
			{
				VkDescriptorSetLayout layouts[] =
				{
					m_DescriptorSetLayout.GetVkDescriptorSetLayout(),
				};


				VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
				pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutInfo.setLayoutCount = 1; // No descriptor sets
				pipelineLayoutInfo.pSetLayouts = &layouts[0]; // Descriptor set layouts
				pipelineLayoutInfo.pushConstantRangeCount = 0; // No push constants
				pipelineLayoutInfo.pPushConstantRanges = nullptr; // Push constant ranges

				if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
					PIX_ASSERT_MSG(false, "Failed to create pipeline layout!");
			}

			// graphics pipeline
			m_GraphicsPipeline.Init(Context->m_Device, m_Renderpass.GetVKRenderpass())
				.AddShaderStages(m_Shader.GetVertexShader(), m_Shader.GetFragmentShader())
				.AddVertexInputState(&VertexBindingDescription, VertexAttributeDescriptions.data(), 1, VertexAttributeDescriptions.size())
				.AddViewportState(width, height)
				.AddInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
				.AddRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
				.AddMultisampleState(VK_SAMPLE_COUNT_1_BIT)
				.AddDepthStencilState(false, false)
				.AddColorBlendState(false, 1)
				.SetPipelineLayout(pipelineLayout)
				.Build();

			{ // Create Command Buffers			
				m_CommandBuffers.resize(Context->m_SwapChainImages.size());
				VK::VulkanHelper::CreateCommandBuffers(Context->m_Device, Context->m_CommandPool, Context->m_SwapChainImages.size(), m_CommandBuffers.data());
			}


			{ // Record Command Buffers

				VkClearColorValue ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
				VkClearValue ClearValue;
				ClearValue.color = ClearColor;

				VkRenderPassBeginInfo RenderPassBeginInfo =
				{
					.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
					.pNext = NULL,
					.renderPass = m_Renderpass.GetVKRenderpass(),
					.renderArea = {
						.offset = {
							.x = 0,
							.y = 0
						},
						.extent = {
							.width = width,
							.height = height
						}
					},
					.clearValueCount = 1,
					.pClearValues = &ClearValue
				};

				for (uint32_t i = 0; i < m_CommandBuffers.size(); i++)
				{

					VK::VulkanHelper::BeginCommandBuffer(m_CommandBuffers[i], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

					RenderPassBeginInfo.framebuffer = m_Framebuffers[i].GetVKFramebuffer();
					vkCmdBeginRenderPass(m_CommandBuffers[i], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

					vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline.GetVkPipeline());

					auto descriptor_set = m_DescriptorSet.GetVkDescriptorSet();
					vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptor_set, 0, nullptr);

					VkBuffer vertexBuffers[] = { m_VertexBuffer.GetBuffer() };
					VkDeviceSize offsets[] = { 0 };
					vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, vertexBuffers, offsets);
					vkCmdBindIndexBuffer(m_CommandBuffers[i], m_IndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

						// Draw the submesh using its base vertex and index offsets
					vkCmdDrawIndexed(m_CommandBuffers[i],
						6,    // Index count for this submesh
						1,                       // Instance count
						0,      // First index
						0,     // Vertex offset
						0);                     // First instance

					vkCmdEndRenderPass(m_CommandBuffers[i]);

					VkResult res = vkEndCommandBuffer(m_CommandBuffers[i]);
					VK_CHECK_RESULT(res, "vkEndCommandBuffer");
				}
				PIX_DEBUG_INFO("Command buffers recorded");
			}
		}

		void VulkanFullScreenQuadRenderpass::Render()
		{
			auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

			uint32_t ImageIndex = Context->m_Queue.AcquireNextImage();
			Context->m_Queue.SubmitAsync(m_CommandBuffers[ImageIndex]);
			Context->m_Queue.Present(ImageIndex);
		}

		void VulkanFullScreenQuadRenderpass::Destroy()
		{
			// TODO:: Destroy Vulkan Objects
		}
	}
}
