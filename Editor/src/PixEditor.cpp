#include "PixEditor.h"
#include <Platfrom/Vulkan/VulkanHelper.h>

void PixEditor::OnStart()
{
	auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();
	auto specs = Engine::GetApplicationSpecs();

	// Shader
	m_TriangleShader.LoadFromFile("../PIX3D/res/vk shaders/triangle.vert", "../PIX3D/res/vk shaders/triangle.frag");

	// vertex buffer
	{
		m_VertexBuffer.Create();
		const std::vector<float> vertices = 
		{
			// positions          // tex coords
			-0.5f, -0.5f, 0.0f,  0.0f, 1.0f,  // bottom left
			 0.5f, -0.5f, 0.0f,  1.0f, 1.0f,  // bottom right
			 0.5f,  0.5f, 0.0f,  1.0f, 0.0f,  // top right
			-0.5f,  0.5f, 0.0f,  0.0f, 0.0f   // top left
		};
		m_VertexBuffer.FillData(vertices.data(), sizeof(float) * vertices.size());
	}

	// vertex input
	{
		m_VertexInputLayout
			.AddAttribute(VK::VertexAttributeFormat::Float3)  // position
			.AddAttribute(VK::VertexAttributeFormat::Float2);  // texcoords
	}
	auto VertexBindingDescription = m_VertexInputLayout.GetBindingDescription();
	auto VertexAttributeDescriptions = m_VertexInputLayout.GetAttributeDescriptions();

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

	// texture loading
	{
		m_Texture.Create();
		m_Texture.LoadFromFile("res/samurai.png", true);
	}

	// descriptor layout -- descripe shader resources
	{
		m_DescriptorSetLayout
			.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
			.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.Build();
	}
	
	// descriptor set -- save resources at specific bindings -- matches shader layout
	{
		m_DescriptorSets.resize(Context->m_SwapChainImages.size());

		for (size_t i = 0; i < m_DescriptorSets.size(); i++)
		{
			m_DescriptorSets[i]
				.Init(m_DescriptorSetLayout)
				.AddUniformBuffer(0, m_CameraUniformBuffers[i])
				.AddTexture(1, m_Texture)
				.Build();
		}
	}

	// renderpass and framebuffers
	{
		m_Renderpass = VK::VulkanHelper::CreateSimpleRenderPass(Context->m_Device, Context->m_SwapChainSurfaceFormat.format);
		m_FrameBuffers = VK::VulkanHelper::CreateSwapChainFrameBuffers(Context, m_Renderpass, specs.Width, specs.Height);
	}

	// pipeline layout (descriptor sets or push constants)
	VkPipelineLayout pipelineLayout = nullptr;
	{
		auto layout = m_DescriptorSetLayout.GetVkDescriptorSetLayout();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1; // No descriptor sets
		pipelineLayoutInfo.pSetLayouts = &layout; // Descriptor set layouts
		pipelineLayoutInfo.pushConstantRangeCount = 0; // No push constants
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Push constant ranges

		if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
			PIX_ASSERT_MSG(false, "Failed to create pipeline layout!");
	}

	// graphics pipeline
	m_GraphicsPipeline.Init(Context->m_Device, m_Renderpass)
		.AddShaderStages(m_TriangleShader.GetVertexShader(), m_TriangleShader.GetFragmentShader())
		.AddVertexInputState(&VertexBindingDescription, VertexAttributeDescriptions.data(), 1, VertexAttributeDescriptions.size())
		.AddViewportState(800.0f, 600.0f)
		.AddInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
		.AddRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
		.AddMultisampleState(VK_SAMPLE_COUNT_1_BIT)
		.AddColorBlendState(false)
		.SetPipelineLayout(pipelineLayout)
		.Build();


	{ // Create Command Buffers

		m_NumImages = Context->m_SwapChainImages.size();
		m_CommandBuffers.resize(m_NumImages);
		VK::VulkanHelper::CreateCommandBuffers(Context->m_Device, Context->m_CommandPool, m_NumImages, m_CommandBuffers.data());
	}



    { // Record Command Buffers

		VkClearColorValue ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		VkClearValue ClearValue;
		ClearValue.color = ClearColor;

		VkRenderPassBeginInfo RenderPassBeginInfo = 
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.pNext = NULL,
			.renderPass = m_Renderpass,
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
			.clearValueCount = 1,
			.pClearValues = &ClearValue
		};

		for (uint32_t i = 0; i < m_CommandBuffers.size(); i++)
		{

			VK::VulkanHelper::BeginCommandBuffer(m_CommandBuffers[i], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

			RenderPassBeginInfo.framebuffer = m_FrameBuffers[i];
			vkCmdBeginRenderPass(m_CommandBuffers[i], &RenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline.GetVkPipeline());

			auto descriptor_set = m_DescriptorSets[i].GetVkDescriptorSet();
			vkCmdBindDescriptorSets(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptor_set, 0, nullptr);

			VkBuffer vertexBuffers[] = { m_VertexBuffer.GetBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(m_CommandBuffers[i], m_IndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

			vkCmdDrawIndexed(m_CommandBuffers[i], 6, 1, 0, 0, 0);  // 6 indices for quad

			vkCmdEndRenderPass(m_CommandBuffers[i]);

			VkResult res = vkEndCommandBuffer(m_CommandBuffers[i]);
			VK_CHECK_RESULT(res, "vkEndCommandBuffer");
        }
        PIX_DEBUG_INFO("Command buffers recorded");
    }

	Cam.Init({0.0f, 0.0f, 5.0f});
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
