#include "PixEditor.h"
#include <Platfrom/Vulkan/VulkanHelper.h>

void PixEditor::OnStart()
{
	auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();
	auto specs = Engine::GetApplicationSpecs();


	// vertex buffer
	m_VertexBuffer.Create();

	// Define triangle vertices
	const std::vector<float> vertices = 
	{
		 // pos               // color
		 0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  // Bottom center, red
		-0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // Top right, green
		 0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f   // Top left, blue
	};

	// Fill vertex buffer with triangle data
	m_VertexBuffer.FillData(vertices.data(), sizeof(float) * vertices.size());

	m_ShaderStorageBuffer.Create();

	{ // Create Command Buffers

        m_NumImages = Context->m_SwapChainImages.size();
		m_CommandBuffers.resize(m_NumImages);
		VK::VulkanHelper::CreateCommandBuffers(Context->m_Device, Context->m_CommandPool, m_NumImages, m_CommandBuffers.data());

		m_Renderpass = VK::VulkanHelper::CreateSimpleRenderPass(Context->m_Device, Context->m_SwapChainSurfaceFormat.format);
		m_FrameBuffers = VK::VulkanHelper::CreateSwapChainFrameBuffers(Context, m_Renderpass, specs.Width, specs.Height);
	}

	// Shader

	m_TriangleShader.LoadFromFile("../PIX3D/res/vk shaders/triangle.vert", "../PIX3D/res/vk shaders/triangle.frag");


	// Define vertex input binding and attributes
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = 6 * sizeof(float);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

	// Position attribute
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = 0;

	// Color attribute
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = 3 * sizeof(float);

	// Define a pipeline layout (descriptor sets or push constants)
	VkPipelineLayout pipelineLayout = nullptr;
	{
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0; // No descriptor sets
		pipelineLayoutInfo.pSetLayouts = nullptr; // Descriptor set layouts
		pipelineLayoutInfo.pushConstantRangeCount = 0; // No push constants
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Push constant ranges

		if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
			PIX_ASSERT_MSG(false, "Failed to create pipeline layout!");
	}

	// graphics pipeline
	m_GraphicsPipeline.Init(Context->m_Device, m_Renderpass)
		.AddShaderStages(m_TriangleShader.GetVertexShader(), m_TriangleShader.GetFragmentShader())
		.AddVertexInputState(&bindingDescription, attributeDescriptions.data(), 1, attributeDescriptions.size())
		.AddViewportState(800.0f, 600.0f)
		.AddInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE)
		.AddRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
		.AddMultisampleState(VK_SAMPLE_COUNT_1_BIT)
		.AddColorBlendState(false)
		.SetPipelineLayout(pipelineLayout)
		.Build();

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
			
			// draw triangle
			
			// Bind graphics pipeline
			vkCmdBindPipeline(m_CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline.GetVkPipeline());

			// Bind vertex buffer
			VkBuffer vertexBuffers[] = { m_VertexBuffer.GetBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(m_CommandBuffers[i], 0, 1, vertexBuffers, offsets);

			uint32_t VertexCount = 3;
			uint32_t InstanceCount = 1;
			uint32_t FirstVertex = 0;
			uint32_t FirstInstance = 0;

			vkCmdDraw(m_CommandBuffers[i], VertexCount, InstanceCount, FirstVertex, FirstInstance);

			
			vkCmdEndRenderPass(m_CommandBuffers[i]);

			VkResult res = vkEndCommandBuffer(m_CommandBuffers[i]);
			VK_CHECK_RESULT(res, "vkEndCommandBuffer");
        }

        PIX_DEBUG_INFO("Command buffers recorded");
    }
}

void PixEditor::OnUpdate(float dt)
{
    auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

    uint32_t ImageIndex = Context->m_Queue.AcquireNextImage();
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
