#include "VulkanFullScreenQuadRenderpass.h"
#include <Engine/Engine.hpp>
#include "VulkanHelper.h"


namespace PIX3D
{
	namespace VK
	{
		void VulkanFullScreenQuadRenderpass::Init(uint32_t width, uint32_t height, VulkanTexture* color_attachment)
		{
			m_Width = width;
			m_Height = height;

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
				.AddTexture(0, *color_attachment)
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
				// Add initial dependency to wait for previous pass
				.AddDependency(
					VK_SUBPASS_EXTERNAL,
					0,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,          // Previous pass reading
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // Our writing
					VK_ACCESS_SHADER_READ_BIT,                      // Previous shader reads
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,           // Our writes
					VK_DEPENDENCY_BY_REGION_BIT
				)
				// Add final dependency for presentation
				.AddDependency(
					0,
					VK_SUBPASS_EXTERNAL,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // Our writes
					VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // Present
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_ACCESS_MEMORY_READ_BIT,
					VK_DEPENDENCY_BY_REGION_BIT
				)
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

				if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
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
				.AddColorBlendState(true, 1)
				.SetPipelineLayout(m_PipelineLayout)
				.Build();
		}

		void VulkanFullScreenQuadRenderpass::RecordCommandBuffer(VkCommandBuffer commandbuffer, uint32_t ImageIndex)
		{
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
						.width = m_Width,
						.height = m_Height
					}
				},
				.clearValueCount = 1,
				.pClearValues = &ClearValue
			};

			RenderPassBeginInfo.framebuffer = m_Framebuffers[ImageIndex].GetVKFramebuffer();
			vkCmdBeginRenderPass(commandbuffer, &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline.GetVkPipeline());

			auto descriptor_set = m_DescriptorSet.GetVkDescriptorSet();
			vkCmdBindDescriptorSets(commandbuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &descriptor_set, 0, nullptr);

			VkBuffer vertexBuffers[] = { m_VertexBuffer.GetBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandbuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandbuffer, m_IndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

			// Draw the submesh using its base vertex and index offsets
			vkCmdDrawIndexed(commandbuffer,
				6,    // Index count for this submesh
				1,                       // Instance count
				0,      // First index
				0,     // Vertex offset
				0);                     // First instance

			vkCmdEndRenderPass(commandbuffer);
		}

		void VulkanFullScreenQuadRenderpass::Destroy()
		{
			// TODO:: Destroy Vulkan Objects
		}
	}
}
