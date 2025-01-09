#include "PixEditor.h"
#include <Platfrom/Vulkan/VulkanHelper.h>

void PixEditor::OnStart()
{
	auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();
	auto specs = Engine::GetApplicationSpecs();


	m_Mesh.Load("res/helmet/DamagedHelmet.gltf");

	// Shader
	m_TriangleShader.LoadFromFile("../PIX3D/res/vk shaders/triangle.vert", "../PIX3D/res/vk shaders/triangle.frag");

	// index buffer
	{
		m_IndexBuffer.Create();
		const std::vector<uint32_t> indices = 
		{
			0, 1, 2,  // first triangle
			2, 3, 0   // second triangle
		};
		
		m_IndexBuffer.FillData(indices.data(), sizeof(uint32_t) * indices.size());
	}

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

	// texture loading
	{
		m_ColorAttachmentTexture = new VK::VulkanTexture();
		m_ColorAttachmentTexture->Create();
		m_ColorAttachmentTexture->CreateColorAttachment(specs.Width, specs.Height, VK::TextureFormat::RGBA16F);


		m_BloomBrightnessAttachmentTexture = new VK::VulkanTexture();
		m_BloomBrightnessAttachmentTexture->Create();
		m_BloomBrightnessAttachmentTexture->CreateColorAttachment(specs.Width, specs.Height, VK::TextureFormat::RGBA16F);
	}

	// renderpass and framebuffers
	{
		m_Renderpass
			.Init(Context->m_Device)

			// firts color attachment (for bloom brightness texture)

			.AddColorAttachment(
				m_ColorAttachmentTexture->GetVKormat(),
				VK_SAMPLE_COUNT_1_BIT,
				VK_ATTACHMENT_LOAD_OP_CLEAR,
				VK_ATTACHMENT_STORE_OP_STORE,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)

			// Second color attachment (for bloom brightness texture)

			.AddColorAttachment(
				m_BloomBrightnessAttachmentTexture->GetVKormat(),
				VK_SAMPLE_COUNT_1_BIT,
				VK_ATTACHMENT_LOAD_OP_CLEAR,
				VK_ATTACHMENT_STORE_OP_STORE,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)

			// Depth attachment

			.AddDepthAttachment(
				Context->m_SupportedDepthFormat,
				VK_SAMPLE_COUNT_1_BIT,
				VK_ATTACHMENT_LOAD_OP_CLEAR,
				VK_ATTACHMENT_STORE_OP_DONT_CARE,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)

			// Subpass with 2 color attachments and 1 depth attachment

			.AddSubpass(VK_PIPELINE_BIND_POINT_GRAPHICS)
			.Build();

		m_Framebuffers.resize(Context->m_SwapChainImages.size());

		for (uint32_t i = 0; i < Context->m_SwapChainImages.size(); i++)
		{
			m_Framebuffers[i]
				.Init(Context->m_Device, m_Renderpass.GetVKRenderpass(), specs.Width, specs.Height)
				.AddAttachment(m_ColorAttachmentTexture)
				.AddAttachment(m_BloomBrightnessAttachmentTexture)
				.AddAttachment(Context->m_DepthAttachmentTexture)
				.Build();
		}
	}

	// pipeline layout (descriptor sets or push constants)
	VkPipelineLayout pipelineLayout = nullptr;
	{
		VkDescriptorSetLayout layouts[] =
		{
			m_CameraDescriptorSetLayout.GetVkDescriptorSetLayout(),
			m_Mesh.m_MaterialDescriptorSetLayout.GetVkDescriptorSetLayout()
		};


		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 2; // No descriptor sets
		pipelineLayoutInfo.pSetLayouts = &layouts[0]; // Descriptor set layouts
		pipelineLayoutInfo.pushConstantRangeCount = 0; // No push constants
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Push constant ranges

		if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
			PIX_ASSERT_MSG(false, "Failed to create pipeline layout!");
	}

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
		.AddColorBlendState(false, 2)
		.SetPipelineLayout(pipelineLayout)
		.Build();


	{ // Create Command Buffers

		m_NumImages = Context->m_SwapChainImages.size();
		m_CommandBuffers.resize(m_NumImages);
		VK::VulkanHelper::CreateCommandBuffers(Context->m_Device, Context->m_CommandPool, m_NumImages, m_CommandBuffers.data());
	}



    { // Record Command Buffers

		VkClearColorValue ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		VkClearValue ClearValue[3];

		ClearValue[0].color = ClearColor;
		ClearValue[0].depthStencil = { 1.0f, 0 };

		ClearValue[1].color = ClearColor;
		ClearValue[1].depthStencil = { 1.0f, 0 };

		ClearValue[2].color = ClearColor;
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

		for (uint32_t i = 0; i < m_CommandBuffers.size(); i++)
		{

			VK::VulkanHelper::BeginCommandBuffer(m_CommandBuffers[i], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

			RenderPassBeginInfo.framebuffer = m_Framebuffers[i].GetVKFramebuffer();
			vkCmdBeginRenderPass(m_CommandBuffers[i], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline.GetVkPipeline());

			auto camera_descriptor_set = m_CameraDescriptorSets[i].GetVkDescriptorSet();
			vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &camera_descriptor_set, 0, nullptr);

			VkBuffer vertexBuffers[] = { m_Mesh.GetVertexBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(m_CommandBuffers[i], m_Mesh.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

			for (const auto& subMesh : m_Mesh.m_SubMeshes)
			{
				if (subMesh.MaterialIndex >= 0)
				{
					auto material_descriptor_set = m_Mesh.m_DescriptorSets[subMesh.MaterialIndex].GetVkDescriptorSet();
					vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &material_descriptor_set, 0, nullptr);
				}
				// Draw the submesh using its base vertex and index offsets
				vkCmdDrawIndexed(m_CommandBuffers[i],
					subMesh.IndicesCount,    // Index count for this submesh
					1,                       // Instance count
					subMesh.BaseIndex,      // First index
					subMesh.BaseVertex,     // Vertex offset
					0);                     // First instance
			}

			vkCmdEndRenderPass(m_CommandBuffers[i]);

			VkResult res = vkEndCommandBuffer(m_CommandBuffers[i]);
			VK_CHECK_RESULT(res, "vkEndCommandBuffer");
        }
        PIX_DEBUG_INFO("Command buffers recorded");
    }

	Cam.Init({0.0f, 0.0f, 5.0f});

	m_FullScreenQuadRenderpass.Init(specs.Width, specs.Height, m_ColorAttachmentTexture);
}

void PixEditor::OnUpdate(float dt)
{
	Cam.Update(dt);

    auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

    uint32_t ImageIndex = Context->m_Queue.AcquireNextImage();

	// Update Camera Uniform Buffer
	_CameraUniformBuffer cameraData = {};
	cameraData.proj = Cam.GetProjectionMatrix();
	cameraData.view = Cam.GetViewMatrix();

	m_CameraUniformBuffers[ImageIndex].UpdateData(&cameraData, sizeof(_CameraUniformBuffer));


    Context->m_Queue.SubmitAsync(m_CommandBuffers[ImageIndex]);
    Context->m_Queue.Present(ImageIndex);


	m_FullScreenQuadRenderpass.Render();
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
