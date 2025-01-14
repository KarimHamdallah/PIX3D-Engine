#include "VulkanImGuiPass.h"
#include <Engine/Engine.hpp>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include "VulkanHelper.h"

namespace PIX3D
{
	namespace VK
	{
        void VulkanImGuiPass::Init(void* window_handle, uint32_t width, uint32_t height)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            // Create Descriptor Pool
            CreateDescriptorPool();

            // Setup Dear ImGui context
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

            // Setup style
            ImGui::StyleColorsDark();

            // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
            ImGuiStyle& style = ImGui::GetStyle();
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                style.WindowRounding = 0.0f;
                style.Colors[ImGuiCol_WindowBg].w = 1.0f;
            }

            // Initialize GLFW backend
            ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)window_handle, true);

            // Create Render Pass
            CreateRenderPass();

            // Initialize Vulkan backend
            ImGui_ImplVulkan_InitInfo init_info = {};
            init_info.Instance = Context->m_Instance;
            init_info.PhysicalDevice = Context->m_PhysDevice.GetSelected().m_physDevice;
            init_info.Device = Context->m_Device;
            init_info.QueueFamily = Context->m_QueueFamily;
            init_info.Queue = Context->m_Queue.m_Queue;
            init_info.PipelineCache = VK_NULL_HANDLE;
            init_info.DescriptorPool = m_DescriptorPool;
            init_info.RenderPass = m_RenderPass;
            init_info.Subpass = 0;
            init_info.MinImageCount = Context->m_SwapChainImages.size();
            init_info.ImageCount = Context->m_SwapChainImages.size();
            init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
            init_info.Allocator = nullptr;

            ImGui_ImplVulkan_Init(&init_info);

            // Upload fonts
            VkCommandBuffer commandBuffer = VulkanHelper::BeginSingleTimeCommands(Context->m_Device, Context->m_CommandPool);
            ImGui_ImplVulkan_CreateFontsTexture();
            VulkanHelper::EndSingleTimeCommands(Context->m_Device, Context->m_Queue.m_Queue, Context->m_CommandPool, commandBuffer);

            // Create framebuffers
            OnResize(width, height);
        }

        void VulkanImGuiPass::Destroy()
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();
            vkDeviceWaitIdle(Context->m_Device);

            // Cleanup ImGui
            ImGui_ImplVulkan_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();

            // Cleanup Vulkan resources
            for (auto framebuffer : m_Framebuffers)
            {
                vkDestroyFramebuffer(Context->m_Device, framebuffer, nullptr);
            }

            if (m_RenderPass != VK_NULL_HANDLE)
            {
                vkDestroyRenderPass(Context->m_Device, m_RenderPass, nullptr);
                m_RenderPass = VK_NULL_HANDLE;
            }

            if (m_DescriptorPool != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorPool(Context->m_Device, m_DescriptorPool, nullptr);
                m_DescriptorPool = VK_NULL_HANDLE;
            }
        }

        void VulkanImGuiPass::CreateDescriptorPool()
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            VkDescriptorPoolSize pool_sizes[] =
            {
                { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
            };

            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 1000;
            pool_info.poolSizeCount = std::size(pool_sizes);
            pool_info.pPoolSizes = pool_sizes;

            VK_CHECK_RESULT(vkCreateDescriptorPool(Context->m_Device, &pool_info, nullptr, &m_DescriptorPool));
        }

        void VulkanImGuiPass::CreateRenderPass()
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            VkAttachmentDescription attachment = {};
            attachment.format = Context->m_SwapChainSurfaceFormat.format;
            attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Load previous contents
            attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference color_attachment = {};
            color_attachment.attachment = 0;
            color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass = {};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &color_attachment;

            VkSubpassDependency dependency = {};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            VkRenderPassCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            info.attachmentCount = 1;
            info.pAttachments = &attachment;
            info.subpassCount = 1;
            info.pSubpasses = &subpass;
            info.dependencyCount = 1;
            info.pDependencies = &dependency;

            VK_CHECK_RESULT(vkCreateRenderPass(Context->m_Device, &info, nullptr, &m_RenderPass));
        }

        void VulkanImGuiPass::OnResize(uint32_t width, uint32_t height)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            // Cleanup old framebuffers
            for (auto framebuffer : m_Framebuffers)
            {
                vkDestroyFramebuffer(Context->m_Device, framebuffer, nullptr);
            }
            m_Framebuffers.clear();

            // Create new framebuffers
            m_Framebuffers.resize(Context->m_SwapChainImages.size());
            for (size_t i = 0; i < Context->m_SwapChainImages.size(); i++)
            {
                VkImageView attachment = Context->m_SwapChainImageViews[i];

                VkFramebufferCreateInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                info.renderPass = m_RenderPass;
                info.attachmentCount = 1;
                info.pAttachments = &attachment;
                info.width = width;
                info.height = height;
                info.layers = 1;

                VK_CHECK_RESULT(vkCreateFramebuffer(Context->m_Device, &info, nullptr, &m_Framebuffers[i]));
            }
        }

        void VulkanImGuiPass::BeginFrame()
        {
            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
        }

        void VulkanImGuiPass::EndFrame()
        {
            ImGui::Render();

            ImGuiIO& io = ImGui::GetIO();
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }
        }

        void VulkanImGuiPass::Render(VkCommandBuffer commandBuffer, uint32_t imageIndex)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();
            auto specs = Engine::GetApplicationSpecs();


            VkRenderPassBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            info.renderPass = m_RenderPass;
            info.framebuffer = m_Framebuffers[imageIndex];
            info.renderArea.extent.width = specs.Width;
            info.renderArea.extent.height = specs.Height;
            info.renderArea.offset = { 0, 0 };
            info.clearValueCount = 0;
            info.pClearValues = nullptr;

            vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
            vkCmdEndRenderPass(commandBuffer);
        }
	}
}
