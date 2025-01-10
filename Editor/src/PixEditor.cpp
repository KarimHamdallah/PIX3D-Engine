#include "PixEditor.h"
#include <Platfrom/Vulkan/VulkanHelper.h>

void PixEditor::OnStart()
{
	auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();
	auto specs = Engine::GetApplicationSpecs();


	m_Mesh.Load("res/helmet/DamagedHelmet.gltf");

	// Shader
	m_TriangleShader.LoadFromFile("../PIX3D/res/vk shaders/triangle.vert", "../PIX3D/res/vk shaders/triangle.frag");

	// camera uniform buffer
	{
		m_CameraUniformBuffers.resize(Context->m_SwapChainImages.size());
		for (size_t i = 0; i < m_CameraUniformBuffers.size(); i++)
		{
			m_CameraUniformBuffers[i].Create(sizeof(_CameraUniformBuffer));
		}
	}

	// descriptor layout -- descripe shader resources
	{
		m_CameraDescriptorSetLayout
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.Build();
	}
	
	// descriptor set -- save resources at specific bindings -- matches shader layout
	{
		m_CameraDescriptorSets.resize(Context->m_SwapChainImages.size());

		for (size_t i = 0; i < m_CameraDescriptorSets.size(); i++)
		{
			m_CameraDescriptorSets[i]
				.Init(m_CameraDescriptorSetLayout)
				.AddUniformBuffer(0, m_CameraUniformBuffers[i])
				.Build();
		}
	}


	/////////////// Color Attachments //////////////////////



		m_ColorAttachmentTexture = new VK::VulkanTexture();
		m_ColorAttachmentTexture->Create();
		m_ColorAttachmentTexture->CreateColorAttachment(specs.Width, specs.Height, VK::TextureFormat::RGBA16F);


		m_BloomBrightnessAttachmentTexture = new VK::VulkanTexture();
		m_BloomBrightnessAttachmentTexture->Create();
		m_BloomBrightnessAttachmentTexture->CreateColorAttachment(specs.Width, specs.Height, VK::TextureFormat::RGBA16F, 6);


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
				.Init(Context->m_Device, m_Renderpass.GetVKRenderpass(), specs.Width, specs.Height)
				.AddAttachment(m_ColorAttachmentTexture)
				.AddAttachment(m_BloomBrightnessAttachmentTexture)
				.AddAttachment(Context->m_DepthAttachmentTexture)
				.Build();

	/////////////// Pipeline Layout //////////////////////

		VkDescriptorSetLayout layouts[] =
		{
			m_CameraDescriptorSetLayout.GetVkDescriptorSetLayout(),
			m_Mesh.m_MaterialDescriptorSetLayout.GetVkDescriptorSetLayout()
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
		.AddShaderStages(m_TriangleShader.GetVertexShader(), m_TriangleShader.GetFragmentShader())
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

	
	Cam.Init({0.0f, 0.0f, 5.0f});

	
	m_FullScreenQuadRenderpass.Init(specs.Width, specs.Height, m_ColorAttachmentTexture);
}

void PixEditor::OnUpdate(float dt)
{
	auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();
	auto specs = Engine::GetApplicationSpecs();

	uint32_t ImageIndex = Context->m_Queue.AcquireNextImage();

	// update
	{
		Cam.Update(dt);

		// Update Camera Uniform Buffer
		_CameraUniformBuffer cameraData = {};
		cameraData.proj = Cam.GetProjectionMatrix();
		cameraData.view = Cam.GetViewMatrix();

		m_CameraUniformBuffers[ImageIndex].UpdateData(&cameraData, sizeof(_CameraUniformBuffer));
	}



	// render

	{
		VK::VulkanHelper::BeginCommandBuffer(m_CommandBuffers[ImageIndex], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
		
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
			vkCmdBeginRenderPass(m_CommandBuffers[ImageIndex], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(m_CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline.GetVkPipeline());

			auto camera_descriptor_set = m_CameraDescriptorSets[ImageIndex].GetVkDescriptorSet();
			vkCmdBindDescriptorSets(m_CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1, &camera_descriptor_set, 0, nullptr);

			VkBuffer vertexBuffers[] = { m_Mesh.GetVertexBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(m_CommandBuffers[ImageIndex], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(m_CommandBuffers[ImageIndex], m_Mesh.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			for (const auto& subMesh : m_Mesh.m_SubMeshes)
			{
				if (subMesh.MaterialIndex >= 0)
				{
					auto material_descriptor_set = m_Mesh.m_DescriptorSets[subMesh.MaterialIndex].GetVkDescriptorSet();
					vkCmdBindDescriptorSets(m_CommandBuffers[ImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 1, 1, &material_descriptor_set, 0, nullptr);
				}
				// Draw the submesh using its base vertex and index offsets
				vkCmdDrawIndexed(m_CommandBuffers[ImageIndex],
					subMesh.IndicesCount,    // Index count for this submesh
					1,                       // Instance count
					subMesh.BaseIndex,      // First index
					subMesh.BaseVertex,     // Vertex offset
					0);                     // First instance
			}

			vkCmdEndRenderPass(m_CommandBuffers[ImageIndex]);
		}

		m_FullScreenQuadRenderpass.RecordCommandBuffer(m_CommandBuffers[ImageIndex], ImageIndex);

		VkResult res = vkEndCommandBuffer(m_CommandBuffers[ImageIndex]);
		VK_CHECK_RESULT(res, "vkEndCommandBuffer");
	}
	
	Context->m_Queue.SubmitAsync(m_CommandBuffers[ImageIndex]);
	Context->m_Queue.Present(ImageIndex);
}

void PixEditor::OnDestroy()
{
}

void PixEditor::OnResize(uint32_t width, uint32_t height)
{

}

void PixEditor::OnKeyPressed(uint32_t key)
{
}
