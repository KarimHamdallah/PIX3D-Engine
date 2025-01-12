#include "PixEditor.h"
#include <Platfrom/Vulkan/VulkanHelper.h>

void PixEditor::OnStart()
{
	auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();
	auto specs = Engine::GetApplicationSpecs();

	m_Mesh.Load("res/helmet/DamagedHelmet.gltf");
	
	Cam.Init({0.0f, 0.0f, 5.0f});

    /*

	uint32_t Count = 200000000;

	std::cout << "\n\n";

	{
		float* data = new float[Count];
		
		{
			PIX3D::Timer timer("CPU Generate 200 Millons Square Numbers");
			for (size_t i = 0; i < Count; i++)
			{
				data[i] = i * i;
			}
		}

		std::cout << "CPU RESULT: \n\n";
		for (size_t i = 0; i < 10; i++)
		{
			std::cout << "square " << i << ": " << data[i] << std::endl;
		}
		delete[] data;
	}

	{
		m_ComputeShader.LoadComputeShaderFromFile("../PIX3D/res/vk shaders/square.comp");

        // Create storage buffer
        m_ComputeStorageBuffer.Create(Count * sizeof(float));

        // Create descriptor set layout with proper binding
        m_ComputeDescriptorSetLayout
            .AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
            .Build();

        // Initialize descriptor set and bind storage buffer
        m_ComputeDescriptorSet
            .Init(m_ComputeDescriptorSetLayout.GetVkDescriptorSetLayout())
            .AddShaderStorageBuffer(0, m_ComputeStorageBuffer)
            .Build();

        auto* Context = static_cast<VK::VulkanGraphicsContext*>(Engine::GetGraphicsContext());

        VkDescriptorSetLayout _layouts[] =
        {
            m_ComputeDescriptorSetLayout.GetVkDescriptorSetLayout()
        };

        // Create pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = _layouts;

        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        if (vkCreatePipelineLayout(Context->m_Device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            PIX_ASSERT_MSG(false, "Failed to create pipeline layout!");
        }

        // Build compute pipeline
        m_ComputePipeline
            .Init(Context->m_Device)
            .AddComputeShader(m_ComputeShader.GetComputeShader())
            .SetPipelineLayout(pipelineLayout)
            .SetWorkGroupSize(256, 1, 1)
            .Build();

        {
            PIX3D::Timer timer("GPU Generate 200 Million Square Numbers");

            // Begin command buffer
            VkCommandBuffer cmdBuffer = VK::VulkanHelper::BeginSingleTimeCommands(Context->m_Device, Context->m_CommandPool);

            // Pipeline barrier before compute to ensure buffer is ready
            VkBufferMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.buffer = m_ComputeStorageBuffer.GetBuffer();
            barrier.size = VK_WHOLE_SIZE;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            vkCmdPipelineBarrier(
                cmdBuffer,
                VK_PIPELINE_STAGE_HOST_BIT,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                0,
                0, nullptr,
                1, &barrier,
                0, nullptr
            );

            // Bind pipeline and descriptor set
            vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_ComputePipeline.GetPipeline());
            VkDescriptorSet descSet = m_ComputeDescriptorSet.GetVkDescriptorSet();
            vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                pipelineLayout, 0, 1, &descSet, 0, nullptr);

            // Dispatch compute shader
            uint32_t workGroupCount = (Count + 255) / 256;
            vkCmdDispatch(cmdBuffer, 256, 1, 1);
            // Pipeline barrier after compute to ensure memory is visible
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_HOST_READ_BIT;

            vkCmdPipelineBarrier(
                cmdBuffer,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_HOST_BIT,
                0,
                0, nullptr,
                1, &barrier,
                0, nullptr
            );

            // Submit and wait
            VK::VulkanHelper::EndSingleTimeCommands(Context->m_Device, Context->m_Queue.m_Queue, Context->m_CommandPool, cmdBuffer);

            // Ensure GPU work is complete
            //vkDeviceWaitIdle(Context->m_Device);
        }
	}

	std::cout << "\n";
	std::cout << "GPU RESULT: \n\n";
	float* data = static_cast<float*>(m_ComputeStorageBuffer.GetMappedData());
	for (size_t i = 0; i < 10; i++)
	{
		std::cout << "square " << i << ": " << data[i] << std::endl;
	}
    */
}

void PixEditor::OnUpdate(float dt)
{
    if (Input::IsKeyPressed(KeyCode::RightShift))
        Engine::GetPlatformLayer()->ShowCursor(false);
    else if (Input::IsKeyPressed(KeyCode::Escape))
        Engine::GetPlatformLayer()->ShowCursor(true);

	Cam.Update(dt);

    VK::VulkanSceneRenderer::Begin(Cam);

    VK::VulkanSceneRenderer::RenderSkyBox();
    VK::VulkanSceneRenderer::RenderMesh(m_Mesh);

    VK::VulkanSceneRenderer::End();
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
