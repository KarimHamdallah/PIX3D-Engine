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
				s_DefaultAlbedoTexture->LoadFromData(data.data(), 2, 2, VK_FORMAT_R8G8B8A8_UNORM);
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
				s_DefaultNormalTexture->LoadFromData(data.data(), 2, 2, VK_FORMAT_R8G8B8A8_UNORM);
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
				s_DefaultWhiteTexture->LoadFromData(data.data(), 2, 2, VK_FORMAT_R8G8B8A8_UNORM);
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
				s_DefaultBlackTexture->LoadFromData(data.data(), 2, 2, VK_FORMAT_R8G8B8A8_UNORM);
			}

			////////////////// Vulkan Static Mesh Material Descriptor Set Layout ///////////////////////

			s_VulkanStaticMeshMaterialDescriptorSetLayout
				.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.AddBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.AddBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.Build();

			s_PointLightsDescriptorSetLayout
				.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.Build();

			/////////////// Environment Maps //////////////////

			// Load And Setup Scene Environment Maps
			{
				s_Environment.Cubemap = new VK::VulkanHdrCubemap();
				s_Environment.Cubemap->LoadHdrToCubemapGPU("res/hdr/barcelona_rooftop.hdr", s_Environment.EnvironmentMapSize);

				s_Environment.IrraduianceCubemap = new VK::VulkanIrradianceCubemap();
				s_Environment.IrraduianceCubemap->Generate(s_Environment.Cubemap, 32);

				s_Environment.PrefilterCubemap = new VK::VulkanPrefilteredCubemap();
				s_Environment.PrefilterCubemap->Generate(s_Environment.Cubemap, 128);

				s_BrdfLutTexture = new VulkanBrdfLutTexture();
				s_BrdfLutTexture->Generate(512);
			}


			/////////////////////// Environment DescriptorSets ////////////////////////////

			s_EnvironmetDescriptorSetLayout
				.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
				.Build();


			VK::VulkanSceneRenderer::s_EnvironmetDescriptorSet
				.Init(VK::VulkanSceneRenderer::s_EnvironmetDescriptorSetLayout)
				.AddCubemap(0, s_Environment.IrraduianceCubemap->m_ImageView, s_Environment.IrraduianceCubemap->m_Sampler)
				.AddCubemap(1, s_Environment.PrefilterCubemap->m_ImageView, s_Environment.PrefilterCubemap->m_Sampler)
				.AddTexture(2, VK::VulkanSceneRenderer::s_BrdfLutTexture->GetImageView(), VK::VulkanSceneRenderer::s_BrdfLutTexture->GetSampler())
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
				s_MainRenderpass.OutlineShader.LoadFromFile("../PIX3D/res/vk shaders/outline.vert", "../PIX3D/res/vk shaders/outline.frag");

				/////////////// Color Attachments //////////////////////

				s_MainRenderpass.ColorAttachmentTexture = new VK::VulkanTexture();
				s_MainRenderpass.ColorAttachmentTexture->CreateColorAttachment(width, height, VK_FORMAT_R16G16B16A16_SFLOAT);

				VulkanTextureHelper::TransitionImageLayout
				(
					s_MainRenderpass.ColorAttachmentTexture->GetImage(),
					s_MainRenderpass.ColorAttachmentTexture->GetFormat(),
					0, 1,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				);

				s_MainRenderpass.BloomBrightnessAttachmentTexture = new VK::VulkanTexture();
				s_MainRenderpass.BloomBrightnessAttachmentTexture->CreateColorAttachment(width, height, VK_FORMAT_R16G16B16A16_SFLOAT, BLOOM_DOWN_SAMPLES);

				VulkanTextureHelper::TransitionImageLayout
				(
					s_MainRenderpass.BloomBrightnessAttachmentTexture->GetImage(),
					s_MainRenderpass.BloomBrightnessAttachmentTexture->GetFormat(),
					0, BLOOM_DOWN_SAMPLES,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				);

				/////////////// Renderpass //////////////////////

				s_MainRenderpass.Renderpass
					.Init(Context->m_Device)

					.AddColorAttachment(
						s_MainRenderpass.ColorAttachmentTexture->GetFormat(),
						VK_SAMPLE_COUNT_1_BIT,
						VK_ATTACHMENT_LOAD_OP_LOAD,
						VK_ATTACHMENT_STORE_OP_STORE,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)

					.AddColorAttachment(
						s_MainRenderpass.BloomBrightnessAttachmentTexture->GetFormat(),
						VK_SAMPLE_COUNT_1_BIT,
						VK_ATTACHMENT_LOAD_OP_LOAD,
						VK_ATTACHMENT_STORE_OP_STORE,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)

					.AddDepthAttachment(
						Context->m_SupportedDepthFormat,
						VK_SAMPLE_COUNT_1_BIT,
						VK_ATTACHMENT_LOAD_OP_LOAD,
						VK_ATTACHMENT_STORE_OP_STORE,
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
				pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
				pushConstant.offset = 0;
				pushConstant.size = sizeof(_ModelMatrixPushConstant);

				VkDescriptorSetLayout layouts[] =
				{
					s_CameraDescriptorSetLayout.GetVkDescriptorSetLayout(),
					VulkanSceneRenderer::GetVulkanStaticMeshMaterialDescriptorSetLayout(),
					s_EnvironmetDescriptorSetLayout.GetVkDescriptorSetLayout(),
					s_PointLightsDescriptorSetLayout.GetVkDescriptorSetLayout()
				};


				VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
				pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutInfo.setLayoutCount = 4;
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
					.AddRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
					.AddMultisampleState(VK_SAMPLE_COUNT_1_BIT)
					.AddDepthStencilState(true, true)
					.AddColorBlendState(true, 2)
					.SetPipelineLayout(s_MainRenderpass.PipelineLayout)
					.Build();

				VkDescriptorSetLayout OutLineLayouts[] =
				{
					s_CameraDescriptorSetLayout.GetVkDescriptorSetLayout(),
				};

				VkPipelineLayoutCreateInfo outlinepipelineLayoutInfo = {};
				outlinepipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				outlinepipelineLayoutInfo.setLayoutCount = 1;
				outlinepipelineLayoutInfo.pSetLayouts = layouts;
				outlinepipelineLayoutInfo.pushConstantRangeCount = 1;
				outlinepipelineLayoutInfo.pPushConstantRanges = &pushConstant;

				if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr, &s_MainRenderpass.OutlinePipelineLayout) != VK_SUCCESS)
					PIX_ASSERT_MSG(false, "Failed to create pipeline layout!");


				// outline graphics pipeline
				s_MainRenderpass.OutlineGraphicsPipeline.Init(Context->m_Device, s_MainRenderpass.Renderpass.GetVKRenderpass())
					.AddShaderStages(s_MainRenderpass.OutlineShader.GetVertexShader(), s_MainRenderpass.OutlineShader.GetFragmentShader())
					.AddVertexInputState(&VertexBindingDescription, VertexAttributeDescriptions.data(), 1, VertexAttributeDescriptions.size())
					.AddViewportState(width, height)
					.AddInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
					.AddRasterizationState(VK_POLYGON_MODE_LINE, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
					.AddMultisampleState(VK_SAMPLE_COUNT_1_BIT)
					.AddDepthStencilState(true, false)
					.AddColorBlendState(true, 2)
					.SetPipelineLayout(s_MainRenderpass.OutlinePipelineLayout)
					.Build();

				/////////////// Command Buffers //////////////////////

				s_MainRenderpass.CommandBuffers.resize(Context->m_SwapChainImages.size());
				VK::VulkanHelper::CreateCommandBuffers(Context->m_Device, Context->m_CommandPool, Context->m_SwapChainImages.size(), s_MainRenderpass.CommandBuffers.data());
			}


			//////////////////////////////////////  Clear Pass   ////////////////////////////////////////////
			
			// Create Clear Renderpass
			s_ClearRenderpass.Renderpass
				.Init(Context->m_Device)
				.AddColorAttachment(
					s_MainRenderpass.ColorAttachmentTexture->GetFormat(),
					VK_SAMPLE_COUNT_1_BIT,
					VK_ATTACHMENT_LOAD_OP_CLEAR,
					VK_ATTACHMENT_STORE_OP_STORE,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
				.AddColorAttachment(
					s_MainRenderpass.BloomBrightnessAttachmentTexture->GetFormat(),
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

			// Create Framebuffer
			s_ClearRenderpass.Framebuffer
				.Init(Context->m_Device, s_ClearRenderpass.Renderpass.GetVKRenderpass(), width, height)
				.AddAttachment(s_MainRenderpass.ColorAttachmentTexture)
				.AddAttachment(s_MainRenderpass.BloomBrightnessAttachmentTexture)
				.AddAttachment(Context->m_DepthAttachmentTexture)
				.Build();



			//////////////////////////////////////  Sky Box Pass   ////////////////////////////////////////////

			{
				//////////////////////// Shader ///////////////////////////

				s_SkyBoxPass.Shader.LoadFromFile("../PIX3D/res/vk shaders/skybox.vert", "../PIX3D/res/vk shaders/skybox.frag");

				/////////////// Renderpass //////////////////////

				s_SkyBoxPass.Renderpass
					.Init(Context->m_Device)

					.AddColorAttachment(
						s_MainRenderpass.ColorAttachmentTexture->GetFormat(),
						VK_SAMPLE_COUNT_1_BIT,
						VK_ATTACHMENT_LOAD_OP_LOAD,
						VK_ATTACHMENT_STORE_OP_STORE,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)

					.AddColorAttachment(
						s_MainRenderpass.BloomBrightnessAttachmentTexture->GetFormat(),
						VK_SAMPLE_COUNT_1_BIT,
						VK_ATTACHMENT_LOAD_OP_LOAD,
						VK_ATTACHMENT_STORE_OP_STORE,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)

					.AddDepthAttachment(
						Context->m_SupportedDepthFormat,
						VK_SAMPLE_COUNT_1_BIT,
						VK_ATTACHMENT_LOAD_OP_LOAD,
						VK_ATTACHMENT_STORE_OP_STORE,
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
					.AddCubemap(0, s_Environment.Cubemap->m_ImageView, s_Environment.Cubemap->m_Sampler)
					.Build();

				/////////////// Pipeline Layout //////////////////////

				VkPushConstantRange pushConstant{};
				pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
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





			//////////////////////////////////////  Sprite Pass   //////////////////////////////////////////////////////////////


			{
			//////////////////////// Shader ///////////////////////////

			s_SpriteRenderpass.Shader.LoadFromFile("../PIX3D/res/vk shaders/sprite.vert", "../PIX3D/res/vk shaders/sprite.frag");

			//////////////////////// Descriptor Layout ///////////////////////

			s_SpriteRenderpass.DescriptorSetLayout
				.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT) // For texture
				.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // For texture
				.Build();

			//////////////////////// Pipeline Layout ///////////////////////

			VkPushConstantRange pushConstant{};
			pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			pushConstant.offset = 0;
			pushConstant.size = sizeof(glm::mat4);  // Only model matrix in push constants

			VkDescriptorSetLayout layouts[] =
			{
				s_CameraDescriptorSetLayout.GetVkDescriptorSetLayout(),
				s_SpriteRenderpass.DescriptorSetLayout.GetVkDescriptorSetLayout()
			};

			VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 2;
			pipelineLayoutInfo.pSetLayouts = layouts;
			pipelineLayoutInfo.pushConstantRangeCount = 1;
			pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

			if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr, &s_SpriteRenderpass.PipelineLayout) != VK_SUCCESS)
				PIX_ASSERT_MSG(false, "Failed to create sprite pipeline layout!");

			//////////////////////// Vertex Description ///////////////////////

			s_SpriteRenderpass.SpriteMesh = VulkanStaticMeshGenerator::GenerateSprite();

			auto bindingDesc = s_SpriteRenderpass.SpriteMesh.VertexLayout.GetBindingDescription();
			auto attributeDesc = s_SpriteRenderpass.SpriteMesh.VertexLayout.GetAttributeDescriptions();

			//////////////////////// Graphics Pipeline ///////////////////////

			s_SpriteRenderpass.GraphicsPipeline.Init(Context->m_Device, s_MainRenderpass.Renderpass.GetVKRenderpass())
				.AddShaderStages(s_SpriteRenderpass.Shader.GetVertexShader(), s_SpriteRenderpass.Shader.GetFragmentShader())
				.AddVertexInputState(&bindingDesc, attributeDesc.data(), 1, attributeDesc.size())
				.AddViewportState(width, height)
				.AddInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
				.AddRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
				.AddMultisampleState(VK_SAMPLE_COUNT_1_BIT)
				.AddDepthStencilState(true, true)
				.AddColorBlendState(true, 2)  // Enable blending for transparency
				.SetPipelineLayout(s_SpriteRenderpass.PipelineLayout)
				.Build();

			}

			//////////////////////////////////////  Grass Pass   //////////////////////////////////////////////////////////////


			{
				//////////////////////// Shader ///////////////////////////

				s_GrassSpriteRenderpass.Shader.LoadFromFile
				(
					"../PIX3D/res/vk shaders/Billboard Grass Rendering/grass_billboard.vert",
					"../PIX3D/res/vk shaders/Billboard Grass Rendering/grass_billboard.frag"
				);

				//////////////////////// Descriptor Layout ///////////////////////

				s_GrassSpriteRenderpass.DescriptorSetLayout
					.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
					.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.Build();

				//////////////////////// Pipeline Layout ///////////////////////

				VkPushConstantRange pushConstant{};
				pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
				pushConstant.offset = 0;
				pushConstant.size = sizeof(_GrassPushConstant);

				VkDescriptorSetLayout layouts[] =
				{
					s_CameraDescriptorSetLayout.GetVkDescriptorSetLayout(),
					s_GrassSpriteRenderpass.DescriptorSetLayout.GetVkDescriptorSetLayout()
				};

				VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
				pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutInfo.setLayoutCount = 2;
				pipelineLayoutInfo.pSetLayouts = layouts;
				pipelineLayoutInfo.pushConstantRangeCount = 1;
				pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

				if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr, &s_GrassSpriteRenderpass.PipelineLayout) != VK_SUCCESS)
					PIX_ASSERT_MSG(false, "Failed to create sprite pipeline layout!");

				//////////////////////// Vertex Description ///////////////////////

				s_GrassSpriteRenderpass.SpriteMesh = VulkanStaticMeshGenerator::GenerateSprite();

				auto bindingDesc = s_GrassSpriteRenderpass.SpriteMesh.VertexLayout.GetBindingDescription();
				auto attributeDesc = s_GrassSpriteRenderpass.SpriteMesh.VertexLayout.GetAttributeDescriptions();

				//////////////////////// Graphics Pipeline ///////////////////////

				s_GrassSpriteRenderpass.GraphicsPipeline.Init(Context->m_Device, s_MainRenderpass.Renderpass.GetVKRenderpass())
					.AddShaderStages(s_GrassSpriteRenderpass.Shader.GetVertexShader(), s_GrassSpriteRenderpass.Shader.GetFragmentShader())
					.AddVertexInputState(&bindingDesc, attributeDesc.data(), 1, attributeDesc.size())
					.AddViewportState(width, height)
					.AddInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
					.AddRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
					.AddMultisampleState(VK_SAMPLE_COUNT_1_BIT)
					.AddDepthStencilState(true, true)
					.AddColorBlendState(false, 2)  // Enable blending for transparency
					.SetPipelineLayout(s_GrassSpriteRenderpass.PipelineLayout)
					.Build();

				s_GrassSpriteRenderpass.GrassPositionsBuffer.Create(s_GrassSpriteRenderpass.MAX_GRASS_COUNT * sizeof(glm::mat4));
			}


			/////////////////////// Terrain Pass ///////////////////

			{
				s_Terrain.TerrainMesh = PIX3D::VK::VulkanStaticMeshGenerator::GenerateGrid(100, 100, 1.0f);

				// Create and compile shaders
				s_Terrain.TerrainPass.Shader.LoadFromFile(
					"../PIX3D/res/vk shaders/terrain.vert",
					"../PIX3D/res/vk shaders/terrain.frag"
				);

				s_Terrain.DescriptorSetLayout
					.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
					.Build();

				// Create pipeline layout
				VkPushConstantRange pushConstant{};
				pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
				pushConstant.offset = 0;
				pushConstant.size = sizeof(_ModelMatrixPushConstant);

				VkDescriptorSetLayout layouts[] = {
					s_CameraDescriptorSetLayout.GetVkDescriptorSetLayout(),
					s_Terrain.DescriptorSetLayout.GetVkDescriptorSetLayout()
				};

				VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
				pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
				pipelineLayoutInfo.setLayoutCount = 2;
				pipelineLayoutInfo.pSetLayouts = layouts;
				pipelineLayoutInfo.pushConstantRangeCount = 1;
				pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

				if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr,
					&s_Terrain.TerrainPass.PipelineLayout) != VK_SUCCESS)
				{
					PIX_ASSERT_MSG(false, "Failed to create terrain pipeline layout!");
				}

				// Setup vertex description
				auto bindingDesc = VulkanVertexInputLayout()
					.AddAttribute(VertexAttributeFormat::Float3)  // position
					.AddAttribute(VertexAttributeFormat::Float2)  // texcoords
					.AddAttribute(VertexAttributeFormat::Float3)  // normal
					.GetBindingDescription();

				auto attributeDesc = VulkanVertexInputLayout()
					.AddAttribute(VertexAttributeFormat::Float3)
					.AddAttribute(VertexAttributeFormat::Float2)
					.AddAttribute(VertexAttributeFormat::Float3)
					.GetAttributeDescriptions();

				// Create graphics pipeline
				s_Terrain.TerrainPass.GraphicsPipeline.Init(Context->m_Device, s_MainRenderpass.Renderpass.GetVKRenderpass())
					.AddShaderStages(
						s_Terrain.TerrainPass.Shader.GetVertexShader(),
						s_Terrain.TerrainPass.Shader.GetFragmentShader())
					.AddVertexInputState(&bindingDesc, attributeDesc.data(), 1, attributeDesc.size())
					.AddViewportState(width, height)
					.AddInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
					.AddRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
					.AddMultisampleState(VK_SAMPLE_COUNT_1_BIT)
					.AddDepthStencilState(true, true)
					.AddColorBlendState(false, 2)
					.SetPipelineLayout(s_Terrain.TerrainPass.PipelineLayout)
					.Build();
			}


			//////////////////////////////////////  Bloom & PostProcessing Passes   ////////////////////////////////////////////

			s_BloomPass.Init(width, height);
			s_PostProcessingRenderpass.Init(width, height);
		}

		void VulkanSceneRenderer::RenderClearPass(const glm::vec4& clearColor)
		{
			auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();
			auto specs = Engine::GetApplicationSpecs();

			VkClearValue clearValues[3] = {};
			clearValues[0].color = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
			clearValues[1].color = { 0.0f, 0.0f, 0.0f, 1.0f };
			clearValues[2].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = s_ClearRenderpass.Renderpass.GetVKRenderpass();
			renderPassInfo.framebuffer = s_ClearRenderpass.Framebuffer.GetVKFramebuffer();
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = { specs.Width, specs.Height };
			renderPassInfo.clearValueCount = 3;
			renderPassInfo.pClearValues = clearValues;

			vkCmdBeginRenderPass(s_MainRenderpass.CommandBuffers[s_ImageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdEndRenderPass(s_MainRenderpass.CommandBuffers[s_ImageIndex]);
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

			s_CameraPosition = cam.GetPosition();
			s_CameraViewProjection = cam.GetProjectionMatrix() * cam.GetViewMatrix();
		}

		void VulkanSceneRenderer::End()
		{
			auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

			s_BloomPass.RecordCommandBuffer(s_MainRenderpass.BloomBrightnessAttachmentTexture, s_MainRenderpass.CommandBuffers[s_ImageIndex]);
			s_PostProcessingRenderpass.RecordCommandBuffer(s_MainRenderpass.ColorAttachmentTexture, s_BloomPass.GetFinalBloomTexture(), s_MainRenderpass.CommandBuffers[s_ImageIndex], s_ImageIndex);

			////////////////// End Record CommandBuffer ////////////////

			VkResult res = vkEndCommandBuffer(s_MainRenderpass.CommandBuffers[s_ImageIndex]);
			VK_CHECK_RESULT(res, "vkEndCommandBuffer");
		}

		void VulkanSceneRenderer::EndRecordCommandBuffer()
		{
			////////////////// End Record CommandBuffer ////////////////

			VkResult res = vkEndCommandBuffer(s_MainRenderpass.CommandBuffers[s_ImageIndex]);
			VK_CHECK_RESULT(res, "vkEndCommandBuffer");
		}

		void VulkanSceneRenderer::Submit()
		{
			auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();
			Context->m_Queue.SubmitAsync(s_MainRenderpass.CommandBuffers[s_ImageIndex]);
		}

		void VulkanSceneRenderer::RenderTerrain(VulkanTexture* texture)
		{
			auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();
			auto specs = Engine::GetApplicationSpecs();

			if (!s_Terrain.DescriptorSet.GetVkDescriptorSet())
			{
				s_Terrain.DescriptorSet.Init(s_Terrain.DescriptorSetLayout)
					.AddTexture(0, *texture)
					.Build();
			}

			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = s_MainRenderpass.Renderpass.GetVKRenderpass();
			renderPassInfo.framebuffer = s_MainRenderpass.Framebuffer.GetVKFramebuffer();
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = { specs.Width, specs.Height };
			renderPassInfo.clearValueCount = 0;
			renderPassInfo.pClearValues = nullptr;

			vkCmdBeginRenderPass(s_MainRenderpass.CommandBuffers[s_ImageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(s_MainRenderpass.CommandBuffers[s_ImageIndex],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				s_Terrain.TerrainPass.GraphicsPipeline.GetVkPipeline());

			vkCmdSetLineWidth(s_MainRenderpass.CommandBuffers[s_ImageIndex], 1.0f);

			// Set viewport and scissor
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

			// Bind camera descriptor set
			auto cameraDescriptorSet = s_CameraDescriptorSets[s_ImageIndex].GetVkDescriptorSet();
			vkCmdBindDescriptorSets(s_MainRenderpass.CommandBuffers[s_ImageIndex],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				s_Terrain.TerrainPass.PipelineLayout,
				0, 1, &cameraDescriptorSet,
				0, nullptr);

			auto descriptorSet = s_Terrain.DescriptorSet.GetVkDescriptorSet();
			vkCmdBindDescriptorSets(s_MainRenderpass.CommandBuffers[s_ImageIndex],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				s_Terrain.TerrainPass.PipelineLayout,
				1, 1, &descriptorSet,
				0, nullptr);

			// Bind vertex and index buffers
			VkBuffer vertexBuffers[] = { s_Terrain.TerrainMesh.VertexBuffer.GetBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(s_MainRenderpass.CommandBuffers[s_ImageIndex], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(s_MainRenderpass.CommandBuffers[s_ImageIndex],
				s_Terrain.TerrainMesh.IndexBuffer.GetBuffer(),
				0, VK_INDEX_TYPE_UINT32);

			// Push constants
			_ModelMatrixPushConstant pushData = {
				glm::mat4(1.0f),  // Model matrix (identity for now)
				s_CameraPosition,
				0.0f,            // Selected mesh ID
				s_BloomThreshold,
				0.0f             // Point light count
			};

			vkCmdPushConstants(s_MainRenderpass.CommandBuffers[s_ImageIndex],
				s_Terrain.TerrainPass.PipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(_ModelMatrixPushConstant),
				&pushData);

			// Draw the terrain
			vkCmdDrawIndexed(s_MainRenderpass.CommandBuffers[s_ImageIndex],
				s_Terrain.TerrainMesh.IndicesCount,
				1, 0, 0, 0);

			vkCmdEndRenderPass(s_MainRenderpass.CommandBuffers[s_ImageIndex]);
		}


		void VulkanSceneRenderer::RenderTexturedQuad(SpriteMaterial* material, const glm::mat4& transform)
		{
			if (!material)
				return;

			auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();
			auto specs = Engine::GetApplicationSpecs();

			VkRenderPassBeginInfo RenderPassBeginInfo = {
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				.pNext = NULL,
				.renderPass = s_MainRenderpass.Renderpass.GetVKRenderpass(),
				.renderArea = {
					.offset = { 0, 0 },
					.extent = { specs.Width, specs.Height }
				},
				.clearValueCount = 0,
				.pClearValues = nullptr
			};
			RenderPassBeginInfo.framebuffer = s_MainRenderpass.Framebuffer.GetVKFramebuffer();

			vkCmdBeginRenderPass(s_MainRenderpass.CommandBuffers[s_ImageIndex], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(s_MainRenderpass.CommandBuffers[s_ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, s_SpriteRenderpass.GraphicsPipeline.GetVkPipeline());

			// Set viewport and scissor
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


			// Bind camera's descriptor set
			auto _camera_descriptor_set = s_CameraDescriptorSets[s_ImageIndex].GetVkDescriptorSet();
			vkCmdBindDescriptorSets(s_MainRenderpass.CommandBuffers[s_ImageIndex],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				s_SpriteRenderpass.PipelineLayout,
				0, 1, &_camera_descriptor_set,
				0, nullptr);

			// Bind material's descriptor set (contains both storage buffer and texture)
			auto descriptorSet = material->GetVKDescriptorSet();
			vkCmdBindDescriptorSets(s_MainRenderpass.CommandBuffers[s_ImageIndex],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				s_SpriteRenderpass.PipelineLayout,
				1, 1, &descriptorSet,
				0, nullptr);

			// Push model matrix
			vkCmdPushConstants(s_MainRenderpass.CommandBuffers[s_ImageIndex],
				s_SpriteRenderpass.PipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT,
				0,
				sizeof(glm::mat4),
				&transform);

			// Bind vertex and index buffers
			VkBuffer vertexBuffers[] = { s_SpriteRenderpass.SpriteMesh.VertexBuffer.GetBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(s_MainRenderpass.CommandBuffers[s_ImageIndex], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(s_MainRenderpass.CommandBuffers[s_ImageIndex],
				s_SpriteRenderpass.SpriteMesh.IndexBuffer.GetBuffer(),
				0,
				VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(s_MainRenderpass.CommandBuffers[s_ImageIndex],
				s_SpriteRenderpass.SpriteMesh.IndicesCount,
				1, 0, 0, 0);

			vkCmdEndRenderPass(s_MainRenderpass.CommandBuffers[s_ImageIndex]);
		}

		void VulkanSceneRenderer::RenderTexturedQuadInstanced(VulkanTexture* texture, uint32_t instanceCount, float _time, float wind_strength, glm::vec2 wind_movement)
		{
			auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();
			auto specs = Engine::GetApplicationSpecs();

			VkRenderPassBeginInfo RenderPassBeginInfo = {
				.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
				.pNext = NULL,
				.renderPass = s_MainRenderpass.Renderpass.GetVKRenderpass(),
				.renderArea = {
					.offset = { 0, 0 },
					.extent = { specs.Width, specs.Height }
				},
				.clearValueCount = 0,
				.pClearValues = nullptr
			};
			RenderPassBeginInfo.framebuffer = s_MainRenderpass.Framebuffer.GetVKFramebuffer();

			vkCmdBeginRenderPass(s_MainRenderpass.CommandBuffers[s_ImageIndex], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(s_MainRenderpass.CommandBuffers[s_ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, s_GrassSpriteRenderpass.GraphicsPipeline.GetVkPipeline());

			// Set viewport and scissor
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

			// Bind camera's descriptor set
			auto camera_descriptor_set = s_CameraDescriptorSets[s_ImageIndex].GetVkDescriptorSet();
			vkCmdBindDescriptorSets(s_MainRenderpass.CommandBuffers[s_ImageIndex],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				s_GrassSpriteRenderpass.PipelineLayout,
				0, 1, &camera_descriptor_set,
				0, nullptr);

			_GrassPushConstant pushconstantdata;
			pushconstantdata.time = _time;
			pushconstantdata.wind_strength = wind_strength;
			pushconstantdata.wind_movement = wind_movement;

			// Push model matrix
			vkCmdPushConstants(s_MainRenderpass.CommandBuffers[s_ImageIndex],
				s_GrassSpriteRenderpass.PipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT,
				0,
				sizeof(_GrassPushConstant),
				&pushconstantdata);

			if (!s_GrassSpriteRenderpass.DescriptorSet.GetVkDescriptorSet())
			{
				s_GrassSpriteRenderpass.DescriptorSet.Init(s_GrassSpriteRenderpass.DescriptorSetLayout)
					.AddShaderStorageBuffer(0, s_GrassSpriteRenderpass.GrassPositionsBuffer)
					.AddTexture(1, *texture)
					.Build();
			}

			auto descriptorSet = s_GrassSpriteRenderpass.DescriptorSet.GetVkDescriptorSet();
			vkCmdBindDescriptorSets(s_MainRenderpass.CommandBuffers[s_ImageIndex],
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				s_GrassSpriteRenderpass.PipelineLayout,
				1, 1, &descriptorSet,
				0, nullptr);

			// Bind vertex and index buffers
			VkBuffer vertexBuffers[] = { s_GrassSpriteRenderpass.SpriteMesh.VertexBuffer.GetBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(s_MainRenderpass.CommandBuffers[s_ImageIndex], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(s_MainRenderpass.CommandBuffers[s_ImageIndex],
				s_GrassSpriteRenderpass.SpriteMesh.IndexBuffer.GetBuffer(),
				0,
				VK_INDEX_TYPE_UINT32);

			// Modified draw command to use instancing
			vkCmdDrawIndexed(s_MainRenderpass.CommandBuffers[s_ImageIndex],
				s_GrassSpriteRenderpass.SpriteMesh.IndicesCount,
				instanceCount,  // Number of instances to draw
				0, 0, 0);

			vkCmdEndRenderPass(s_MainRenderpass.CommandBuffers[s_ImageIndex]);
		}

		void VulkanSceneRenderer::RenderMesh(Scene* scene, VulkanStaticMesh& mesh, const glm::mat4& transform)
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
				vkCmdSetLineWidth(s_MainRenderpass.CommandBuffers[s_ImageIndex], 5.0f);


				auto _camera_descriptor_set = s_CameraDescriptorSets[s_ImageIndex].GetVkDescriptorSet();
				vkCmdBindDescriptorSets(s_MainRenderpass.CommandBuffers[s_ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, s_MainRenderpass.PipelineLayout, 0, 1, &_camera_descriptor_set, 0, nullptr);

				VkBuffer vertexBuffers[] = { mesh.GetVertexBuffer() };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(s_MainRenderpass.CommandBuffers[s_ImageIndex], 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(s_MainRenderpass.CommandBuffers[s_ImageIndex], mesh.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

				int SubMeshIndex = 0;
				for (const auto& subMesh : mesh.m_SubMeshes)
				{
					if (subMesh.MaterialIndex >= 0)
					{
						auto material_descriptor_set = mesh.m_DescriptorSets[subMesh.MaterialIndex].GetVkDescriptorSet();
						vkCmdBindDescriptorSets(s_MainRenderpass.CommandBuffers[s_ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, s_MainRenderpass.PipelineLayout, 1, 1, &material_descriptor_set, 0, nullptr);
					}

					_ModelMatrixPushConstant pushData = { transform, s_CameraPosition, (float)SubMeshIndex, s_BloomThreshold, (float)scene->m_PointLightsCount };
					vkCmdPushConstants(s_MainRenderpass.CommandBuffers[s_ImageIndex], s_MainRenderpass.PipelineLayout,
						VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
						0, sizeof(_ModelMatrixPushConstant), &pushData);

					auto environment_descriptor_set = s_EnvironmetDescriptorSet.GetVkDescriptorSet();
					vkCmdBindDescriptorSets(s_MainRenderpass.CommandBuffers[s_ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, s_MainRenderpass.PipelineLayout, 2, 1, &environment_descriptor_set, 0, nullptr);

					if (scene)
					{
						auto point_lights_descriptor_set = scene->PointLightsDescriptorSet.GetVkDescriptorSet();
						vkCmdBindDescriptorSets(s_MainRenderpass.CommandBuffers[s_ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, s_MainRenderpass.PipelineLayout, 3, 1, &point_lights_descriptor_set, 0, nullptr);
					}

					// Draw the submesh using its base vertex and index offsets
					vkCmdDrawIndexed(s_MainRenderpass.CommandBuffers[s_ImageIndex],
						subMesh.IndicesCount,    // Index count for this submesh
						1,                       // Instance count
						subMesh.BaseIndex,      // First index
						subMesh.BaseVertex,     // Vertex offset
						0);                     // First instance

					SubMeshIndex++;
				}

				vkCmdEndRenderPass(s_MainRenderpass.CommandBuffers[s_ImageIndex]);
			}
		}

		void VulkanSceneRenderer::RenderMeshOutline(Scene* scene, VulkanStaticMesh& mesh, const glm::mat4& transform)
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

				vkCmdBeginRenderPass(s_MainRenderpass.CommandBuffers[s_ImageIndex], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				vkCmdBindPipeline(s_MainRenderpass.CommandBuffers[s_ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, s_MainRenderpass.OutlineGraphicsPipeline.GetVkPipeline());

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
				vkCmdSetLineWidth(s_MainRenderpass.CommandBuffers[s_ImageIndex], 5.0f);


				auto _camera_descriptor_set = s_CameraDescriptorSets[s_ImageIndex].GetVkDescriptorSet();
				vkCmdBindDescriptorSets(s_MainRenderpass.CommandBuffers[s_ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, s_MainRenderpass.OutlinePipelineLayout, 0, 1, &_camera_descriptor_set, 0, nullptr);

				VkBuffer vertexBuffers[] = { mesh.GetVertexBuffer() };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(s_MainRenderpass.CommandBuffers[s_ImageIndex], 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(s_MainRenderpass.CommandBuffers[s_ImageIndex], mesh.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

				int SubMeshIndex = 0;
				for (const auto& subMesh : mesh.m_SubMeshes)
				{
					_ModelMatrixPushConstant pushData = { transform, s_CameraPosition, (float)SubMeshIndex, s_BloomThreshold, (float)scene->m_PointLightsCount };
					vkCmdPushConstants(s_MainRenderpass.CommandBuffers[s_ImageIndex], s_MainRenderpass.OutlinePipelineLayout,
						VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
						0, sizeof(_ModelMatrixPushConstant), &pushData);

					// Draw the submesh using its base vertex and index offsets
					vkCmdDrawIndexed(s_MainRenderpass.CommandBuffers[s_ImageIndex],
						subMesh.IndicesCount,    // Index count for this submesh
						1,                       // Instance count
						subMesh.BaseIndex,      // First index
						subMesh.BaseVertex,     // Vertex offset
						0);                     // First instance

					SubMeshIndex++;
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

				_ModelMatrixPushConstant pushData = { s_Environment.CubemapTransform, s_CameraPosition, 0.0f, s_BloomThreshold };
				vkCmdPushConstants(s_MainRenderpass.CommandBuffers[s_ImageIndex], s_SkyBoxPass.PipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
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

		void VulkanSceneRenderer::OnResize(uint32_t width, uint32_t height)
		{
			auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

			// Wait for device to finish all operations
			vkDeviceWaitIdle(Context->m_Device);

			// Resize main renderpass
			{
				// Recreate color attachment with new size
				s_MainRenderpass.ColorAttachmentTexture->Destroy();
				s_MainRenderpass.BloomBrightnessAttachmentTexture->Destroy();
				
				delete s_MainRenderpass.ColorAttachmentTexture;
				delete s_MainRenderpass.BloomBrightnessAttachmentTexture;

				s_MainRenderpass.ColorAttachmentTexture = new VK::VulkanTexture();
				s_MainRenderpass.ColorAttachmentTexture->CreateColorAttachment(width, height, VK_FORMAT_R16G16B16A16_SFLOAT);

				VulkanTextureHelper::TransitionImageLayout
				(
					s_MainRenderpass.ColorAttachmentTexture->GetImage(),
					s_MainRenderpass.ColorAttachmentTexture->GetFormat(),
					0, 1,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				);

				s_MainRenderpass.BloomBrightnessAttachmentTexture = new VK::VulkanTexture();
				s_MainRenderpass.BloomBrightnessAttachmentTexture->CreateColorAttachment(width, height, VK_FORMAT_R16G16B16A16_SFLOAT, BLOOM_DOWN_SAMPLES);

				VulkanTextureHelper::TransitionImageLayout
				(
					s_MainRenderpass.BloomBrightnessAttachmentTexture->GetImage(),
					s_MainRenderpass.BloomBrightnessAttachmentTexture->GetFormat(),
					0, BLOOM_DOWN_SAMPLES,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				);

				// Recreate framebuffer
				s_MainRenderpass.Framebuffer.Destroy();
				s_MainRenderpass.Framebuffer
					.Init(Context->m_Device, s_MainRenderpass.Renderpass.GetVKRenderpass(), width, height)
					.AddAttachment(s_MainRenderpass.ColorAttachmentTexture)
					.AddAttachment(s_MainRenderpass.BloomBrightnessAttachmentTexture)
					.AddAttachment(Context->m_DepthAttachmentTexture)
					.Build();
			}

			// Resize skybox pass
			{
				s_SkyBoxPass.Framebuffer.Destroy();
				s_SkyBoxPass.Framebuffer
					.Init(Context->m_Device, s_SkyBoxPass.Renderpass.GetVKRenderpass(), width, height)
					.AddAttachment(s_MainRenderpass.ColorAttachmentTexture)
					.AddAttachment(s_MainRenderpass.BloomBrightnessAttachmentTexture)
					.AddAttachment(Context->m_DepthAttachmentTexture)
					.Build();
			}


			// Recreate clear pass framebuffer
			s_ClearRenderpass.Framebuffer.Destroy();
			s_ClearRenderpass.Framebuffer
				.Init(Context->m_Device, s_ClearRenderpass.Renderpass.GetVKRenderpass(), width, height)
				.AddAttachment(s_MainRenderpass.ColorAttachmentTexture)
				.AddAttachment(s_MainRenderpass.BloomBrightnessAttachmentTexture)
				.AddAttachment(Context->m_DepthAttachmentTexture)
				.Build();

			// Resize bloom pass
			s_BloomPass.OnResize(width, height);

			// Resize post processing pass
			s_PostProcessingRenderpass.OnResize(width, height);

			VK::VulkanImGuiPass::OnResize(width, height);

			PIX_DEBUG_SUCCESS("Scene renderer resized successfully");
		}

		void VulkanSceneRenderer::Destroy()
		{
			auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

			// Wait for device to finish operations
			vkDeviceWaitIdle(Context->m_Device);

			// Destroy default textures
			if (s_DefaultAlbedoTexture) {
				s_DefaultAlbedoTexture->Destroy();
				delete s_DefaultAlbedoTexture;
				s_DefaultAlbedoTexture = nullptr;
			}

			if (s_DefaultNormalTexture) {
				s_DefaultNormalTexture->Destroy();
				delete s_DefaultNormalTexture;
				s_DefaultNormalTexture = nullptr;
			}

			if (s_DefaultWhiteTexture) {
				s_DefaultWhiteTexture->Destroy();
				delete s_DefaultWhiteTexture;
				s_DefaultWhiteTexture = nullptr;
			}

			if (s_DefaultBlackTexture) {
				s_DefaultBlackTexture->Destroy();
				delete s_DefaultBlackTexture;
				s_DefaultBlackTexture = nullptr;
			}

			// Destroy environment maps
			if (s_Environment.Cubemap)
			{
				s_Environment.Cubemap->Destroy();
				delete s_Environment.Cubemap;
				s_Environment.Cubemap = nullptr;
			}

			if (s_Environment.IrraduianceCubemap)
			{
				s_Environment.IrraduianceCubemap->Destroy();
				delete s_Environment.IrraduianceCubemap;
				s_Environment.IrraduianceCubemap = nullptr;
			}

			if (s_Environment.PrefilterCubemap)
			{
				s_Environment.PrefilterCubemap->Destroy();
				delete s_Environment.PrefilterCubemap;
				s_Environment.PrefilterCubemap = nullptr;
			}

			if (s_BrdfLutTexture)
			{
				s_BrdfLutTexture->Destroy();
				delete s_BrdfLutTexture;
				s_BrdfLutTexture = nullptr;
			}

			// Destroy descriptor sets and layouts
			s_VulkanStaticMeshMaterialDescriptorSetLayout.Destroy();
			s_EnvironmetDescriptorSetLayout.Destroy();
			s_EnvironmetDescriptorSet.Destroy();
			s_CameraDescriptorSetLayout.Destroy();

			for (auto& descriptorSet : s_CameraDescriptorSets) {
				descriptorSet.Destroy();
			}
			s_CameraDescriptorSets.clear();

			// Destroy uniform buffers
			for (auto& buffer : s_CameraUniformBuffers) {
				buffer.Destroy();
			}
			s_CameraUniformBuffers.clear();

			// Destroy main renderpass resources
			{
				if (s_MainRenderpass.ColorAttachmentTexture) {
					s_MainRenderpass.ColorAttachmentTexture->Destroy();
					delete s_MainRenderpass.ColorAttachmentTexture;
					s_MainRenderpass.ColorAttachmentTexture = nullptr;
				}

				if (s_MainRenderpass.BloomBrightnessAttachmentTexture) {
					s_MainRenderpass.BloomBrightnessAttachmentTexture->Destroy();
					delete s_MainRenderpass.BloomBrightnessAttachmentTexture;
					s_MainRenderpass.BloomBrightnessAttachmentTexture = nullptr;
				}

				s_MainRenderpass.Renderpass.Destroy();
				s_MainRenderpass.Framebuffer.Destroy();
				s_MainRenderpass.Shader.Destroy();
				s_MainRenderpass.GraphicsPipeline.Destroy();

				if (s_MainRenderpass.PipelineLayout != VK_NULL_HANDLE) {
					vkDestroyPipelineLayout(Context->m_Device, s_MainRenderpass.PipelineLayout, nullptr);
					s_MainRenderpass.PipelineLayout = VK_NULL_HANDLE;
				}

				// Free command buffers
				if (!s_MainRenderpass.CommandBuffers.empty()) {
					vkFreeCommandBuffers(Context->m_Device, Context->m_CommandPool,
						static_cast<uint32_t>(s_MainRenderpass.CommandBuffers.size()),
						s_MainRenderpass.CommandBuffers.data());
					s_MainRenderpass.CommandBuffers.clear();
				}
			}

			s_ClearRenderpass.Renderpass.Destroy();
			s_ClearRenderpass.Framebuffer.Destroy();

			// Destroy skybox pass resources
			{
				s_SkyBoxPass.Renderpass.Destroy();
				s_SkyBoxPass.Framebuffer.Destroy();
				s_SkyBoxPass.Shader.Destroy();
				s_SkyBoxPass.GraphicsPipeline.Destroy();
				s_SkyBoxPass.DescriptorSetLayout.Destroy();
				s_SkyBoxPass.DescriptorSet.Destroy();

				if (s_SkyBoxPass.PipelineLayout != VK_NULL_HANDLE) {
					vkDestroyPipelineLayout(Context->m_Device, s_SkyBoxPass.PipelineLayout, nullptr);
					s_SkyBoxPass.PipelineLayout = VK_NULL_HANDLE;
				}

				// Destroy cube mesh
				s_SkyBoxPass.CubeMesh.VertexBuffer.Destroy();
				s_SkyBoxPass.CubeMesh.IndexBuffer.Destroy();
			}

			// Destroy bloom and post processing passes
			s_BloomPass.Destroy();
			s_PostProcessingRenderpass.Destroy();

			s_Terrain.TerrainMesh.VertexBuffer.Destroy();
			s_Terrain.TerrainMesh.IndexBuffer.Destroy();
			s_Terrain.TerrainPass.Shader.Destroy();
			s_Terrain.TerrainPass.GraphicsPipeline.Destroy();
			if (s_Terrain.TerrainPass.PipelineLayout != VK_NULL_HANDLE)
			{
				vkDestroyPipelineLayout(Context->m_Device, s_Terrain.TerrainPass.PipelineLayout, nullptr);
				s_Terrain.TerrainPass.PipelineLayout = VK_NULL_HANDLE;
			}
		}
	}
}
