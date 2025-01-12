#include "VulkanSceneRenderer.h"
#include <Engine/Engine.hpp>
#include "VulkanHelper.h"


namespace PIX3D
{
	namespace VK
	{
		void VulkanSceneRenderer::Init(uint32_t width, uint32_t height)
		{

			///////////////////// Default Textures //////////////////////////

			// Default Albedo Texture
			{
				// RGBA8 Texture
				std::vector<uint8_t> data =
				{
					255, 0, 255, 255,
					255, 0, 255, 255,
					255, 0, 255, 255,
					255, 0, 255, 255
				};

				s_DefaultAlbedoTexture = new VulkanTexture();

				s_DefaultAlbedoTexture->Create();
				s_DefaultAlbedoTexture->LoadFromData(data.data(), 2, 2, TextureFormat::RGBA8);
			}

			// Default Normal Texture
			{
				// RGBA8 Texture
				std::vector<uint8_t> data =
				{
					127, 127, 255, 255,
					127, 127, 255, 255,
					127, 127, 255, 255,
					127, 127, 255, 255
				};

				s_DefaultNormalTexture = new VulkanTexture();

				s_DefaultNormalTexture->Create();
				s_DefaultNormalTexture->LoadFromData(data.data(), 2, 2, TextureFormat::RGBA8);
			}

			// Default White Texture
			{
				// RGBA8 Texture
				std::vector<uint8_t> data =
				{
					255, 255, 255, 255,
					255, 255, 255, 255,
					255, 255, 255, 255,
					255, 255, 255, 255
				};

				s_DefaultWhiteTexture = new VulkanTexture();

				s_DefaultWhiteTexture->Create();
				s_DefaultWhiteTexture->LoadFromData(data.data(), 2, 2, TextureFormat::RGBA8);
			}

			// Default Black Texture
			{
				// RGBA8 Texture
				std::vector<uint8_t> data =
				{
					0, 0, 0, 255,
					0, 0, 0, 255,
					0, 0, 0, 255,
					0, 0, 0, 255
				};

				s_DefaultBlackTexture = new VulkanTexture();

				s_DefaultBlackTexture->Create();
				s_DefaultBlackTexture->LoadFromData(data.data(), 2, 2, TextureFormat::RGBA8);
			}

			////////////////// Vulkan Static Mesh Material Descriptor Set Layout ///////////////////////

			s_VulkanStaticMeshMaterialDescriptorSetLayout
				.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.Build();



			/////////////// Environment Maps //////////////////

			s_Cubemap = new VulkanHdrCubemap();
			s_Cubemap->LoadHdrToCubemapGPU("res/hdr/barcelona_rooftop.hdr", 1024);

			s_IrraduianceCubemap = new VulkanIrradianceCubemap();
			s_IrraduianceCubemap->Generate(s_Cubemap, 32);

			s_PrefilterCubemap = new VulkanPrefilteredCubemap();
			s_PrefilterCubemap->Generate(s_Cubemap, 128);

			s_BrdfLutTexture = new VulkanBrdfLutTexture();
			s_BrdfLutTexture->Generate(512);

			/////////////////////// Environment DescriptorSets ////////////////////////////

			s_EnvironmetDescriptorSetLayout
				.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.Build();

			s_EnvironmetDescriptorSet
				.Init(s_EnvironmetDescriptorSetLayout)
				.AddCubemap(0, s_IrraduianceCubemap->m_ImageView, s_IrraduianceCubemap->m_Sampler)
				.AddCubemap(1, s_PrefilterCubemap->m_ImageView, s_PrefilterCubemap->m_Sampler)
				.AddTexture(2, s_BrdfLutTexture->GetImageView(), s_BrdfLutTexture->GetSampler())
				.Build();

			//////////////////////// Setup Camera Shader Set Data ///////////////////////////

			auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

			s_CameraUniformBuffers.resize(Context->m_SwapChainImages.size());
			for (size_t i = 0; i < s_CameraUniformBuffers.size(); i++)
			{
				s_CameraUniformBuffers[i].Create(sizeof(_CameraUniformBuffer));
			}

			s_CameraDescriptorSetLayout
				.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
				.Build();

			s_CameraDescriptorSets.resize(Context->m_SwapChainImages.size());

			for (size_t i = 0; i < s_CameraDescriptorSets.size(); i++)
			{
				s_CameraDescriptorSets[i]
					.Init(s_CameraDescriptorSetLayout)
					.AddUniformBuffer(0, s_CameraUniformBuffers[i])
					.Build();
			}

			//////////////////////////////////////  Main Render Pass   ////////////////////////////////////////////

			{
				//////////////////////// Shader ///////////////////////////

				s_MainRenderpass.Shader.LoadFromFile("../PIX3D/res/vk shaders/3d_model.vert", "../PIX3D/res/vk shaders/3d_model.frag");

				/////////////// Color Attachments //////////////////////

				s_MainRenderpass.ColorAttachmentTexture = new VK::VulkanTexture();
				s_MainRenderpass.ColorAttachmentTexture->Create();
				s_MainRenderpass.ColorAttachmentTexture->CreateColorAttachment(width, height, VK::TextureFormat::RGBA16F);

				s_MainRenderpass.ColorAttachmentTexture->TransitionImageLayout(
					s_MainRenderpass.ColorAttachmentTexture->GetVKormat(),
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				);

				s_MainRenderpass.BloomBrightnessAttachmentTexture = new VK::VulkanTexture();
				s_MainRenderpass.BloomBrightnessAttachmentTexture->Create();
				s_MainRenderpass.BloomBrightnessAttachmentTexture->CreateColorAttachment(width, height, VK::TextureFormat::RGBA16F, BLOOM_DOWN_SAMPLES);

				s_MainRenderpass.BloomBrightnessAttachmentTexture->TransitionImageLayout(
					s_MainRenderpass.BloomBrightnessAttachmentTexture->GetVKormat(),
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				);

				/////////////// Renderpass //////////////////////

				s_MainRenderpass.Renderpass
					.Init(Context->m_Device)

					.AddColorAttachment(
						s_MainRenderpass.ColorAttachmentTexture->GetVKormat(),
						VK_SAMPLE_COUNT_1_BIT,
						VK_ATTACHMENT_LOAD_OP_LOAD,
						VK_ATTACHMENT_STORE_OP_STORE,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)

					.AddColorAttachment(
						s_MainRenderpass.BloomBrightnessAttachmentTexture->GetVKormat(),
						VK_SAMPLE_COUNT_1_BIT,
						VK_ATTACHMENT_LOAD_OP_LOAD,
						VK_ATTACHMENT_STORE_OP_STORE,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)

					.AddDepthAttachment(
						Context->m_SupportedDepthFormat,
						VK_SAMPLE_COUNT_1_BIT,
						VK_ATTACHMENT_LOAD_OP_LOAD,
						VK_ATTACHMENT_STORE_OP_DONT_CARE,
						VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
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

				s_MainRenderpass.Framebuffer
					.Init(Context->m_Device, s_MainRenderpass.Renderpass.GetVKRenderpass(), width, height)
					.AddAttachment(s_MainRenderpass.ColorAttachmentTexture)
					.AddAttachment(s_MainRenderpass.BloomBrightnessAttachmentTexture)
					.AddAttachment(Context->m_DepthAttachmentTexture)
					.Build();

				/////////////// Pipeline Layout //////////////////////

				VkPushConstantRange pushConstant{};
				pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
				pushConstant.offset = 0;
				pushConstant.size = sizeof(_ModelMatrixPushConstant);

				VkDescriptorSetLayout layouts[] =
				{
					s_CameraDescriptorSetLayout.GetVkDescriptorSetLayout(),
					VulkanSceneRenderer::GetVulkanStaticMeshMaterialDescriptorSetLayout(),
					s_EnvironmetDescriptorSetLayout.GetVkDescriptorSetLayout()
				};


				VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
				pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutInfo.setLayoutCount = 3;
				pipelineLayoutInfo.pSetLayouts = layouts;
				pipelineLayoutInfo.pushConstantRangeCount = 1;
				pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

				if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr, &s_MainRenderpass.PipelineLayout) != VK_SUCCESS)
					PIX_ASSERT_MSG(false, "Failed to create pipeline layout!");


				/////////////// Graphics Pipeline //////////////////////

				auto VertexBindingDescription = VulkanStaticMeshVertex::GetBindingDescription();
				auto VertexAttributeDescriptions = VulkanStaticMeshVertex::GetAttributeDescriptions();

				// graphics pipeline
				s_MainRenderpass.GraphicsPipeline.Init(Context->m_Device, s_MainRenderpass.Renderpass.GetVKRenderpass())
					.AddShaderStages(s_MainRenderpass.Shader.GetVertexShader(), s_MainRenderpass.Shader.GetFragmentShader())
					.AddVertexInputState(&VertexBindingDescription, VertexAttributeDescriptions.data(), 1, VertexAttributeDescriptions.size())
					.AddViewportState(width, height)
					.AddInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
					.AddRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
					.AddMultisampleState(VK_SAMPLE_COUNT_1_BIT)
					.AddDepthStencilState(true, true)
					.AddColorBlendState(true, 2)
					.SetPipelineLayout(s_MainRenderpass.PipelineLayout)
					.Build();


				/////////////// Command Buffers //////////////////////

				s_MainRenderpass.CommandBuffers.resize(Context->m_SwapChainImages.size());
				VK::VulkanHelper::CreateCommandBuffers(Context->m_Device, Context->m_CommandPool, Context->m_SwapChainImages.size(), s_MainRenderpass.CommandBuffers.data());
			}





			//////////////////////////////////////  Sky Box Pass   ////////////////////////////////////////////

			{
				//////////////////////// Shader ///////////////////////////

				s_SkyBoxPass.Shader.LoadFromFile("../PIX3D/res/vk shaders/skybox.vert", "../PIX3D/res/vk shaders/skybox.frag");

				/////////////// Renderpass //////////////////////

				s_SkyBoxPass.Renderpass
					.Init(Context->m_Device)

					.AddColorAttachment(
						s_MainRenderpass.ColorAttachmentTexture->GetVKormat(),
						VK_SAMPLE_COUNT_1_BIT,
						VK_ATTACHMENT_LOAD_OP_CLEAR,
						VK_ATTACHMENT_STORE_OP_STORE,
						VK_IMAGE_LAYOUT_UNDEFINED,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)

					.AddColorAttachment(
						s_MainRenderpass.BloomBrightnessAttachmentTexture->GetVKormat(),
						VK_SAMPLE_COUNT_1_BIT,
						VK_ATTACHMENT_LOAD_OP_CLEAR,
						VK_ATTACHMENT_STORE_OP_STORE,
						VK_IMAGE_LAYOUT_UNDEFINED,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)

					.AddDepthAttachment(
						Context->m_SupportedDepthFormat,
						VK_SAMPLE_COUNT_1_BIT,
						VK_ATTACHMENT_LOAD_OP_CLEAR,
						VK_ATTACHMENT_STORE_OP_STORE,
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

				s_SkyBoxPass.Framebuffer
					.Init(Context->m_Device, s_SkyBoxPass.Renderpass.GetVKRenderpass(), width, height)
					.AddAttachment(s_MainRenderpass.ColorAttachmentTexture)
					.AddAttachment(s_MainRenderpass.BloomBrightnessAttachmentTexture)
					.AddAttachment(Context->m_DepthAttachmentTexture)
					.Build();



				/////////////////// Descriptor Set Layout ///////////////////////

				s_SkyBoxPass.DescriptorSetLayout
					.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.Build();

				/////////////////// Descriptor Set //////////////////////////////

				s_SkyBoxPass.DescriptorSet
					.Init(s_SkyBoxPass.DescriptorSetLayout)
					.AddCubemap(0, s_Cubemap->m_ImageView, s_Cubemap->m_Sampler)
					.Build();

				/////////////// Pipeline Layout //////////////////////

				VkPushConstantRange pushConstant{};
				pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
				pushConstant.offset = 0;
				pushConstant.size = sizeof(_ModelMatrixPushConstant);

				VkDescriptorSetLayout layouts[] =
				{
					s_CameraDescriptorSetLayout.GetVkDescriptorSetLayout(),
					s_SkyBoxPass.DescriptorSetLayout.GetVkDescriptorSetLayout()
				};


				VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
				pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutInfo.setLayoutCount = 2; // No descriptor sets
				pipelineLayoutInfo.pSetLayouts = layouts; // Descriptor set layouts
				pipelineLayoutInfo.pushConstantRangeCount = 1;
				pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

				if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr, &s_SkyBoxPass.PipelineLayout) != VK_SUCCESS)
					PIX_ASSERT_MSG(false, "Failed to create pipeline layout!");


				/////////////// Graphics Pipeline //////////////////////
 
				s_SkyBoxPass.CubeMesh = VulkanStaticMeshGenerator::GenerateCube();

				auto VertexBindingDescription = s_SkyBoxPass.CubeMesh.VertexLayout.GetBindingDescription();
				auto VertexAttributeDescriptions = s_SkyBoxPass.CubeMesh.VertexLayout.GetAttributeDescriptions();

				// graphics pipeline
				s_SkyBoxPass.GraphicsPipeline.Init(Context->m_Device, s_SkyBoxPass.Renderpass.GetVKRenderpass())
					.AddShaderStages(s_SkyBoxPass.Shader.GetVertexShader(), s_SkyBoxPass.Shader.GetFragmentShader())
					.AddVertexInputState(&VertexBindingDescription, VertexAttributeDescriptions.data(), 1, VertexAttributeDescriptions.size())
					.AddViewportState(width, height)
					.AddInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
					.AddRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
					.AddMultisampleState(VK_SAMPLE_COUNT_1_BIT)
					.AddDepthStencilState(true, false, VK_COMPARE_OP_LESS_OR_EQUAL)
					.AddColorBlendState(false, 2)
					.SetPipelineLayout(s_SkyBoxPass.PipelineLayout)
					.Build();
			}





			//////////////////////////////////////  Bloom & PostProcessing Passes   ////////////////////////////////////////////

			s_BloomPass.Init(width, height, s_MainRenderpass.BloomBrightnessAttachmentTexture);
			s_PostProcessingRenderpass.Init(width, height, s_MainRenderpass.ColorAttachmentTexture, s_BloomPass.GetFinalBloomTexture());
		}

		void VulkanSceneRenderer::Destroy()
		{
			// TODO:: Handel

			s_DefaultAlbedoTexture->Destroy();
			s_DefaultNormalTexture->Destroy();
			s_DefaultWhiteTexture->Destroy();
			s_DefaultBlackTexture->Destroy();

			delete s_DefaultAlbedoTexture;
			delete s_DefaultNormalTexture;
			delete s_DefaultWhiteTexture;
			delete s_DefaultBlackTexture;
		}

		void VulkanSceneRenderer::Begin(Camera3D& cam)
		{
			auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();


			/////////////// Acquire Next Frame Image /////////////////

			s_ImageIndex = Context->m_Queue.AcquireNextImage();

			/////////////// Update Camera Shader Set Data /////////////////

			_CameraUniformBuffer cameraData = {};
			cameraData.proj = cam.GetProjectionMatrix();
			cameraData.view = cam.GetViewMatrix();
			cameraData.skybox_view = glm::mat4(glm::mat3(cam.GetViewMatrix()));

			s_CameraUniformBuffers[s_ImageIndex].UpdateData(&cameraData, sizeof(_CameraUniformBuffer));

			////////////////// Begin Record CommandBuffer ////////////////

			VK::VulkanHelper::BeginCommandBuffer(s_MainRenderpass.CommandBuffers[s_ImageIndex], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
		}

		void VulkanSceneRenderer::End()
		{
			auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

			s_BloomPass.RecordCommandBuffer(s_MainRenderpass.CommandBuffers[s_ImageIndex], BLOOM_BLUR_ITERATIONS);
			s_PostProcessingRenderpass.RecordCommandBuffer(s_MainRenderpass.CommandBuffers[s_ImageIndex], s_ImageIndex);

			////////////////// End Record CommandBuffer ////////////////

			VkResult res = vkEndCommandBuffer(s_MainRenderpass.CommandBuffers[s_ImageIndex]);
			VK_CHECK_RESULT(res, "vkEndCommandBuffer");

			Context->m_Queue.SubmitAsync(s_MainRenderpass.CommandBuffers[s_ImageIndex]);
			Context->m_Queue.Present(s_ImageIndex);
		}

		void VulkanSceneRenderer::RenderMesh(VulkanStaticMesh& mesh)
		{
			auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();
			auto specs = Engine::GetApplicationSpecs();

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
					.renderPass = s_MainRenderpass.Renderpass.GetVKRenderpass(),
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
					//.clearValueCount = 3,
					//.pClearValues = ClearValue
					.clearValueCount = 0,
					.pClearValues = nullptr
				};


				RenderPassBeginInfo.framebuffer = s_MainRenderpass.Framebuffer.GetVKFramebuffer();

				_ModelMatrixPushConstant pushData = { glm::mat4(1.0f) };
				vkCmdPushConstants(s_MainRenderpass.CommandBuffers[s_ImageIndex], s_MainRenderpass.PipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT,
					0, sizeof(_ModelMatrixPushConstant), &pushData);


				vkCmdBeginRenderPass(s_MainRenderpass.CommandBuffers[s_ImageIndex], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				vkCmdBindPipeline(s_MainRenderpass.CommandBuffers[s_ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, s_MainRenderpass.GraphicsPipeline.GetVkPipeline());

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

				vkCmdSetViewport(s_MainRenderpass.CommandBuffers[s_ImageIndex], 0, 1, &viewport);
				vkCmdSetScissor(s_MainRenderpass.CommandBuffers[s_ImageIndex], 0, 1, &scissor);

				auto _camera_descriptor_set = s_CameraDescriptorSets[s_ImageIndex].GetVkDescriptorSet();
				vkCmdBindDescriptorSets(s_MainRenderpass.CommandBuffers[s_ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, s_MainRenderpass.PipelineLayout, 0, 1, &_camera_descriptor_set, 0, nullptr);

				VkBuffer vertexBuffers[] = { mesh.GetVertexBuffer() };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(s_MainRenderpass.CommandBuffers[s_ImageIndex], 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(s_MainRenderpass.CommandBuffers[s_ImageIndex], mesh.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

				for (const auto& subMesh : mesh.m_SubMeshes)
				{
					if (subMesh.MaterialIndex >= 0)
					{
						auto material_descriptor_set = mesh.m_DescriptorSets[subMesh.MaterialIndex].GetVkDescriptorSet();
						vkCmdBindDescriptorSets(s_MainRenderpass.CommandBuffers[s_ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, s_MainRenderpass.PipelineLayout, 1, 1, &material_descriptor_set, 0, nullptr);
					}


					auto environment_descriptor_set = s_EnvironmetDescriptorSet.GetVkDescriptorSet();
					vkCmdBindDescriptorSets(s_MainRenderpass.CommandBuffers[s_ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, s_MainRenderpass.PipelineLayout, 2, 1, &environment_descriptor_set, 0, nullptr);

					// Draw the submesh using its base vertex and index offsets
					vkCmdDrawIndexed(s_MainRenderpass.CommandBuffers[s_ImageIndex],
						subMesh.IndicesCount,    // Index count for this submesh
						1,                       // Instance count
						subMesh.BaseIndex,      // First index
						subMesh.BaseVertex,     // Vertex offset
						0);                     // First instance
				}

				vkCmdEndRenderPass(s_MainRenderpass.CommandBuffers[s_ImageIndex]);
			}
		}

		void VulkanSceneRenderer::RenderSkyBox()
		{
			auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();
			auto specs = Engine::GetApplicationSpecs();

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
					.renderPass = s_SkyBoxPass.Renderpass.GetVKRenderpass(),
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

				_ModelMatrixPushConstant pushData = { glm::mat4(1.0f) };
				vkCmdPushConstants(s_MainRenderpass.CommandBuffers[s_ImageIndex], s_SkyBoxPass.PipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT,
					0, sizeof(_ModelMatrixPushConstant), &pushData);

				RenderPassBeginInfo.framebuffer = s_SkyBoxPass.Framebuffer.GetVKFramebuffer();
				vkCmdBeginRenderPass(s_MainRenderpass.CommandBuffers[s_ImageIndex], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				vkCmdBindPipeline(s_MainRenderpass.CommandBuffers[s_ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, s_SkyBoxPass.GraphicsPipeline.GetVkPipeline());

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

				vkCmdSetViewport(s_MainRenderpass.CommandBuffers[s_ImageIndex], 0, 1, &viewport);
				vkCmdSetScissor(s_MainRenderpass.CommandBuffers[s_ImageIndex], 0, 1, &scissor);

				auto _camera_descriptor_set = s_CameraDescriptorSets[s_ImageIndex].GetVkDescriptorSet();
				vkCmdBindDescriptorSets(s_MainRenderpass.CommandBuffers[s_ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, s_SkyBoxPass.PipelineLayout, 0, 1, &_camera_descriptor_set, 0, nullptr);

				VkBuffer vertexBuffers[] = { s_SkyBoxPass.CubeMesh.VertexBuffer.GetBuffer() };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(s_MainRenderpass.CommandBuffers[s_ImageIndex], 0, 1, vertexBuffers, offsets);

				
				auto cubemap_texture_descriptor_set = s_SkyBoxPass.DescriptorSet.GetVkDescriptorSet();
				vkCmdBindDescriptorSets(s_MainRenderpass.CommandBuffers[s_ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, s_SkyBoxPass.PipelineLayout, 1, 1, &cubemap_texture_descriptor_set, 0, nullptr);

				vkCmdDraw(s_MainRenderpass.CommandBuffers[s_ImageIndex], s_SkyBoxPass.CubeMesh.VerticesCount, 1, 0, 0);

				vkCmdEndRenderPass(s_MainRenderpass.CommandBuffers[s_ImageIndex]);
			}
		}
	}
}
