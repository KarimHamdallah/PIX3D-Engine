#include "VulkanMainRenderpass.h"
#include <Core/Core.h>
#include <Engine/Engine.hpp>
#include "VulkanSceneRenderer.h"
#include "VulkanHelper.h"

namespace PIX3D
{
	namespace VK
	{
		void VulkanMainRenderpass::Init(uint32_t width, uint32_t height)
		{
			auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

			//////////////////////// Shader ///////////////////////////

			m_Shader.LoadFromFile("../PIX3D/res/vk shaders/3d_model.vert", "../PIX3D/res/vk shaders/3d_model.frag");

			//////////////////////// Camera Uniform Buffer ///////////////////////////

			m_CameraUniformBuffers.resize(Context->m_SwapChainImages.size());
			for (size_t i = 0; i < m_CameraUniformBuffers.size(); i++)
			{
				m_CameraUniformBuffers[i].Create(sizeof(_CameraUniformBuffer));
			}

			//////////////////////// Camera Descriptor layout And Sets ///////////////////////////


			m_CameraDescriptorSetLayout
				.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
				.Build();

			m_CameraDescriptorSets.resize(Context->m_SwapChainImages.size());

			for (size_t i = 0; i < m_CameraDescriptorSets.size(); i++)
			{
				m_CameraDescriptorSets[i]
					.Init(m_CameraDescriptorSetLayout)
					.AddUniformBuffer(0, m_CameraUniformBuffers[i])
					.Build();
			}

			/////////////// Color Attachments //////////////////////

			m_ColorAttachmentTexture = new VK::VulkanTexture();
			m_ColorAttachmentTexture->Create();
			m_ColorAttachmentTexture->CreateColorAttachment(width, height, VK::TextureFormat::RGBA16F);


			m_BloomBrightnessAttachmentTexture = new VK::VulkanTexture();
			m_BloomBrightnessAttachmentTexture->Create();
			m_BloomBrightnessAttachmentTexture->CreateColorAttachment(width, height, VK::TextureFormat::RGBA16F, BLOOM_DOWN_SAMPLES);

			m_BloomBrightnessAttachmentTexture->TransitionImageLayout(
				m_BloomBrightnessAttachmentTexture->GetVKormat(),
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			);

			/////////////// Renderpass //////////////////////

			m_Renderpass
				.Init(Context->m_Device)

				.AddColorAttachment(
					m_ColorAttachmentTexture->GetVKormat(),
					VK_SAMPLE_COUNT_1_BIT,
					VK_ATTACHMENT_LOAD_OP_CLEAR,
					VK_ATTACHMENT_STORE_OP_STORE,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)

				.AddColorAttachment(
					m_BloomBrightnessAttachmentTexture->GetVKormat(),
					VK_SAMPLE_COUNT_1_BIT,
					VK_ATTACHMENT_LOAD_OP_CLEAR,
					VK_ATTACHMENT_STORE_OP_STORE,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)

				.AddDepthAttachment(
					Context->m_SupportedDepthFormat,
					VK_SAMPLE_COUNT_1_BIT,
					VK_ATTACHMENT_LOAD_OP_CLEAR,
					VK_ATTACHMENT_STORE_OP_DONT_CARE,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)

				.AddSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)

				// Initial synchronization dependency
				.AddDependency(
					VK_SUBPASS_EXTERNAL,                            // srcSubpass
					0,                                              // dstSubpass
					VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,          // srcStageMask
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
					VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,    // dstStageMask
					VK_ACCESS_MEMORY_READ_BIT,                     // srcAccessMask
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
					VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,  // dstAccessMask
					VK_DEPENDENCY_BY_REGION_BIT                    // dependencyFlags
				)
				// Final synchronization dependency for shader read transition
				.AddDependency(
					0,                                              // srcSubpass
					VK_SUBPASS_EXTERNAL,                           // dstSubpass
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // srcStageMask
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,         // dstStageMask
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,          // srcAccessMask
					VK_ACCESS_SHADER_READ_BIT,                     // dstAccessMask
					VK_DEPENDENCY_BY_REGION_BIT                    // dependencyFlags
				)
				.Build();

			/////////////// Framebuffer //////////////////////

			m_Framebuffer
				.Init(Context->m_Device, m_Renderpass.GetVKRenderpass(), width, height)
				.AddAttachment(m_ColorAttachmentTexture)
				.AddAttachment(m_BloomBrightnessAttachmentTexture)
				.AddAttachment(Context->m_DepthAttachmentTexture)
				.Build();

			/////////////// Pipeline Layout //////////////////////

			VkDescriptorSetLayout layouts[] =
			{
				m_CameraDescriptorSetLayout.GetVkDescriptorSetLayout(),
				VulkanSceneRenderer::GetVulkanStaticMeshMaterialDescriptorSetLayout()
			};


			VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 2; // No descriptor sets
			pipelineLayoutInfo.pSetLayouts = layouts; // Descriptor set layouts
			pipelineLayoutInfo.pushConstantRangeCount = 0; // No push constants
			pipelineLayoutInfo.pPushConstantRanges = nullptr; // Push constant ranges

			if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS)
				PIX_ASSERT_MSG(false, "Failed to create pipeline layout!");


			/////////////// Graphics Pipeline //////////////////////

			auto VertexBindingDescription = VulkanStaticMeshVertex::GetBindingDescription();
			auto VertexAttributeDescriptions = VulkanStaticMeshVertex::GetAttributeDescriptions();

			// graphics pipeline
			m_GraphicsPipeline.Init(Context->m_Device, m_Renderpass.GetVKRenderpass())
				.AddShaderStages(m_Shader.GetVertexShader(), m_Shader.GetFragmentShader())
				.AddVertexInputState(&VertexBindingDescription, VertexAttributeDescriptions.data(), 1, VertexAttributeDescriptions.size())
				.AddViewportState(800.0f, 600.0f)
				.AddInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
				.AddRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
				.AddMultisampleState(VK_SAMPLE_COUNT_1_BIT)
				.AddDepthStencilState(true, true)
				.AddColorBlendState(true, 2)
				.SetPipelineLayout(m_PipelineLayout)
				.Build();


			/////////////// Command Buffers //////////////////////

			m_CommandBuffers.resize(Context->m_SwapChainImages.size());
			VK::VulkanHelper::CreateCommandBuffers(Context->m_Device, Context->m_CommandPool, Context->m_SwapChainImages.size(), m_CommandBuffers.data());
		}

		void VulkanMainRenderpass::BeginRender(Camera3D& cam, uint32_t image_index)
		{
			/////////////// Update Camera Uniform Buffer /////////////////

			_CameraUniformBuffer cameraData = {};
			cameraData.proj = cam.GetProjectionMatrix();
			cameraData.view = cam.GetViewMatrix();

			m_CameraUniformBuffers[image_index].UpdateData(&cameraData, sizeof(_CameraUniformBuffer));

			////////////////// Begin Record CommandBuffer ////////////////

			VK::VulkanHelper::BeginCommandBuffer(m_CommandBuffers[image_index], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
		}

		void VulkanMainRenderpass::EndRender(uint32_t image_index)
		{
			////////////////// End Record CommandBuffer ////////////////

			VkResult res = vkEndCommandBuffer(m_CommandBuffers[image_index]);
			VK_CHECK_RESULT(res, "vkEndCommandBuffer");
		}


		void VulkanMainRenderpass::RenderMesh(VulkanStaticMesh& mesh, uint32_t image_index)
		{
			auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();
			auto specs = Engine::GetApplicationSpecs();

			// render


				// main renderpass record command buffers
			{

				VkClearColorValue ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
				VkClearValue ClearValue[3];

				ClearValue[0].color = ClearColor;

				ClearValue[1].color = ClearColor;

				ClearValue[2].depthStencil = { 1.0f, 0 };

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
							.width = specs.Width,
							.height = specs.Height
						}
					},
					.clearValueCount = 3,
					.pClearValues = ClearValue
				};


				RenderPassBeginInfo.framebuffer = m_Framebuffer.GetVKFramebuffer();
				vkCmdBeginRenderPass(m_CommandBuffers[image_index], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				vkCmdBindPipeline(m_CommandBuffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline.GetVkPipeline());

				VkViewport viewport{};
				viewport.x = 0.0f;
				viewport.y = (float)specs.Height;
				viewport.width = (float)specs.Width;
				viewport.height = -(float)specs.Height;
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;

				VkRect2D scissor{};
				scissor.offset = { 0, 0 };
				scissor.extent = { specs.Width, specs.Height };

				vkCmdSetViewport(m_CommandBuffers[image_index], 0, 1, &viewport);
				vkCmdSetScissor(m_CommandBuffers[image_index], 0, 1, &scissor);

				auto camera_descriptor_set = m_CameraDescriptorSets[image_index].GetVkDescriptorSet();
				vkCmdBindDescriptorSets(m_CommandBuffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &camera_descriptor_set, 0, nullptr);

				VkBuffer vertexBuffers[] = { mesh.GetVertexBuffer() };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(m_CommandBuffers[image_index], 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(m_CommandBuffers[image_index], mesh.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

				for (const auto& subMesh : mesh.m_SubMeshes)
				{
					if (subMesh.MaterialIndex >= 0)
					{
						auto material_descriptor_set = mesh.m_DescriptorSets[subMesh.MaterialIndex].GetVkDescriptorSet();
						vkCmdBindDescriptorSets(m_CommandBuffers[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 1, 1, &material_descriptor_set, 0, nullptr);
					}
					// Draw the submesh using its base vertex and index offsets
					vkCmdDrawIndexed(m_CommandBuffers[image_index],
						subMesh.IndicesCount,    // Index count for this submesh
						1,                       // Instance count
						subMesh.BaseIndex,      // First index
						subMesh.BaseVertex,     // Vertex offset
						0);                     // First instance
				}

				vkCmdEndRenderPass(m_CommandBuffers[image_index]);
			}
		}
	}
}
