#include "VulkanImGuiPass.h"
#include <Engine/Engine.hpp>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include "VulkanHelper.h"
#include "VulkanTexture.h"

namespace
{
    void SetDarkThemeColors()
    {
        ImGuiStyle& style = ImGui::GetStyle();

        style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        style.Colors[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.07f, 1.00f);
        style.Colors[ImGuiCol_ChildBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
        style.Colors[ImGuiCol_PopupBg] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
        style.Colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
        style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
        style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
        style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        style.Colors[ImGuiCol_CheckMark] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
        style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.08f, 0.50f, 0.72f, 1.00f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.38f, 0.38f, 0.38f, 1.00f);
        style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
        style.Colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.67f, 0.67f, 0.67f, 0.39f);
        style.Colors[ImGuiCol_Separator] = style.Colors[ImGuiCol_Border];
        style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.41f, 0.42f, 0.44f, 1.00f);
        style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.29f, 0.30f, 0.31f, 0.67f);
        style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        style.Colors[ImGuiCol_Tab] = ImVec4(0.08f, 0.08f, 0.09f, 0.83f);
        style.Colors[ImGuiCol_TabHovered] = ImVec4(0.33f, 0.34f, 0.36f, 0.83f);
        style.Colors[ImGuiCol_TabActive] = ImVec4(0.23f, 0.23f, 0.24f, 1.00f);
        style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
        style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.13f, 0.14f, 0.15f, 1.00f);
        style.Colors[ImGuiCol_DockingPreview] = /*ImVec4(0.26f, 0.59f, 0.98f, 0.70f);*/ ImVec4(0.83f, 0.83f, 0.83f, 0.7f);
        style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        style.Colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.11f, 0.64f, 0.92f, 1.00f);
        style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
        //style.GrabRounding = style.FrameRounding = 2.3f;
    }
}


namespace PIX3D
{
    namespace VK
    {
        void VulkanImGuiPass::Init(uint32_t width, uint32_t height)
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
            ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)Context->m_NativeWindowHandle, true);

            // Create Render Passes
            CreateRenderPasses();

            // Initialize Vulkan backend
            ImGui_ImplVulkan_InitInfo init_info = {};
            init_info.Instance = Context->m_Instance;
            init_info.PhysicalDevice = Context->m_PhysDevice.GetSelected().m_physDevice;
            init_info.Device = Context->m_Device;
            init_info.QueueFamily = Context->m_QueueFamily;
            init_info.Queue = Context->m_Queue.m_Queue;
            init_info.PipelineCache = VK_NULL_HANDLE;
            init_info.DescriptorPool = m_DescriptorPool;
            init_info.RenderPass = m_LoadRenderPass; // Use load renderpass as default
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

            // Create Command Buffers
            m_CommandBuffers.resize(Context->m_SwapChainImages.size());
            VK::VulkanHelper::CreateCommandBuffers(Context->m_Device, Context->m_CommandPool, Context->m_SwapChainImages.size(), m_CommandBuffers.data());

            SetDarkThemeColors();
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
            for (auto framebuffer : m_LoadFramebuffers)
            {
                vkDestroyFramebuffer(Context->m_Device, framebuffer, nullptr);
            }

            for (auto framebuffer : m_ClearFramebuffers)
            {
                vkDestroyFramebuffer(Context->m_Device, framebuffer, nullptr);
            }

            if (m_LoadRenderPass != VK_NULL_HANDLE)
            {
                vkDestroyRenderPass(Context->m_Device, m_LoadRenderPass, nullptr);
                m_LoadRenderPass = VK_NULL_HANDLE;
            }

            if (m_ClearRenderPass != VK_NULL_HANDLE)
            {
                vkDestroyRenderPass(Context->m_Device, m_ClearRenderPass, nullptr);
                m_ClearRenderPass = VK_NULL_HANDLE;
            }

            if (m_DescriptorPool != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorPool(Context->m_Device, m_DescriptorPool, nullptr);
                m_DescriptorPool = VK_NULL_HANDLE;
            }

            for (size_t i = 0; i < m_CommandBuffers.size(); i++)
            {
                vkFreeCommandBuffers(Context->m_Device, Context->m_CommandPool, 1, &m_CommandBuffers[i]);
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

        void VulkanImGuiPass::CreateRenderPasses()
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            // Create Load RenderPass (for overlay)
            {
                VkAttachmentDescription attachment = {};
                attachment.format = Context->m_SwapChainSurfaceFormat.format;
                attachment.samples = VK_SAMPLE_COUNT_1_BIT;
                attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
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

                std::array<VkSubpassDependency, 2> dependencies = {};

                dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
                dependencies[0].dstSubpass = 0;
                dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                dependencies[1].srcSubpass = 0;
                dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
                dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                VkRenderPassCreateInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                info.attachmentCount = 1;
                info.pAttachments = &attachment;
                info.subpassCount = 1;
                info.pSubpasses = &subpass;
                info.dependencyCount = dependencies.size();
                info.pDependencies = dependencies.data();

                VK_CHECK_RESULT(vkCreateRenderPass(Context->m_Device, &info, nullptr, &m_LoadRenderPass));
            }

            // Create Clear RenderPass (for standalone UI)
            {
                VkAttachmentDescription attachment = {};
                attachment.format = Context->m_SwapChainSurfaceFormat.format;
                attachment.samples = VK_SAMPLE_COUNT_1_BIT;
                attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                VkAttachmentReference color_attachment = {};
                color_attachment.attachment = 0;
                color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkSubpassDescription subpass = {};
                subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = 1;
                subpass.pColorAttachments = &color_attachment;

                std::array<VkSubpassDependency, 2> dependencies = {};

                dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
                dependencies[0].dstSubpass = 0;
                dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                dependencies[1].srcSubpass = 0;
                dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
                dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

                VkRenderPassCreateInfo info = {};
                info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                info.attachmentCount = 1;
                info.pAttachments = &attachment;
                info.subpassCount = 1;
                info.pSubpasses = &subpass;
                info.dependencyCount = dependencies.size();
                info.pDependencies = dependencies.data();

                VK_CHECK_RESULT(vkCreateRenderPass(Context->m_Device, &info, nullptr, &m_ClearRenderPass));
            }
        }

        void VulkanImGuiPass::OnResize(uint32_t width, uint32_t height)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            // Cleanup old framebuffers
            for (auto framebuffer : m_LoadFramebuffers)
            {
                vkDestroyFramebuffer(Context->m_Device, framebuffer, nullptr);
            }
            for (auto framebuffer : m_ClearFramebuffers)
            {
                vkDestroyFramebuffer(Context->m_Device, framebuffer, nullptr);
            }

            m_LoadFramebuffers.clear();
            m_ClearFramebuffers.clear();

            // Create new framebuffers
            m_LoadFramebuffers.resize(Context->m_SwapChainImages.size());
            m_ClearFramebuffers.resize(Context->m_SwapChainImages.size());

            for (size_t i = 0; i < Context->m_SwapChainImages.size(); i++)
            {
                VkImageView attachment = Context->m_SwapChainImageViews[i];

                // Create Load Framebuffer
                {
                    VkFramebufferCreateInfo info = {};
                    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                    info.renderPass = m_LoadRenderPass;
                    info.attachmentCount = 1;
                    info.pAttachments = &attachment;
                    info.width = width;
                    info.height = height;
                    info.layers = 1;
                    VK_CHECK_RESULT(vkCreateFramebuffer(Context->m_Device, &info, nullptr, &m_LoadFramebuffers[i]));
                }

                // Create Clear Framebuffer
                {
                    VkFramebufferCreateInfo info = {};
                    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                    info.renderPass = m_ClearRenderPass;
                    info.attachmentCount = 1;
                    info.pAttachments = &attachment;
                    info.width = width;
                    info.height = height;
                    info.layers = 1;
                    VK_CHECK_RESULT(vkCreateFramebuffer(Context->m_Device, &info, nullptr, &m_ClearFramebuffers[i]));
                }
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

        void VulkanImGuiPass::StartDockSpace()
        {
            static bool DockSpace_Open = true;
            static bool opt_fullscreen = true;
            static bool opt_padding = false;
            static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

            // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
            // because it would be confusing to have two docking targets within each others.
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
            if (opt_fullscreen)
            {
                const ImGuiViewport* viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->WorkPos);
                ImGui::SetNextWindowSize(viewport->WorkSize);
                ImGui::SetNextWindowViewport(viewport->ID);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
                window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
            }
            else
            {
                dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
            }

            // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
            // and handle the pass-thru hole, so we ask Begin() to not render a background.
            if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
                window_flags |= ImGuiWindowFlags_NoBackground;

            // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
            // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
            // all active windows docked into it will lose their parent and become undocked.
            // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
            // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
            if (!opt_padding)
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("DockSpace Demo", &DockSpace_Open, window_flags);
            if (!opt_padding)
                ImGui::PopStyleVar();

            if (opt_fullscreen)
                ImGui::PopStyleVar(2);

            // Submit the DockSpace
            ImGuiIO& io = ImGui::GetIO();
            //style.WindowMinSize.x = 450.0f;
            if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
            {
                ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
            }
        }

        void VulkanImGuiPass::EndDockSpace()
        {
            ImGui::End();
        }

        void VulkanImGuiPass::Render(VkCommandBuffer commandBuffer, uint32_t imageIndex, bool useLoadRenderPass)
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();
            auto specs = Engine::GetApplicationSpecs();

            VkRenderPassBeginInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            info.renderPass = useLoadRenderPass ? m_LoadRenderPass : m_ClearRenderPass;
            info.framebuffer = useLoadRenderPass ? m_LoadFramebuffers[imageIndex] : m_ClearFramebuffers[imageIndex];
            info.renderArea.extent.width = specs.Width;
            info.renderArea.extent.height = specs.Height;
            info.renderArea.offset = { 0, 0 };

            VkClearValue clearValue = {};
            clearValue.color = { 0.0f, 0.0f, 0.0f, 1.0f };

            if (!useLoadRenderPass)
            {
                info.clearValueCount = 1;
                info.pClearValues = &clearValue;
            }
            else
            {
                info.clearValueCount = 0;
                info.pClearValues = nullptr;
            }

            vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
            vkCmdEndRenderPass(commandBuffer);
        }

        void VulkanImGuiPass::BeginRecordCommandbuffer()
        {
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            /////////////// Acquire Next Frame Image /////////////////

            s_ImageIndex = Context->m_Queue.AcquireNextImage();

            ////////////////// Begin Record CommandBuffer ////////////////

            VK::VulkanHelper::BeginCommandBuffer(m_CommandBuffers[s_ImageIndex], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
        }

        void VulkanImGuiPass::EndRecordCommandbufferAndSubmit()
        {
            VK::VulkanImGuiPass::Render(m_CommandBuffers[s_ImageIndex], s_ImageIndex, false);
            auto* Context = (VulkanGraphicsContext*)Engine::GetGraphicsContext();

            ////////////////// End Record CommandBuffer ////////////////

            VkResult res = vkEndCommandBuffer(m_CommandBuffers[s_ImageIndex]);
            VK_CHECK_RESULT(res, "vkEndCommandBuffer");

            Context->m_Queue.SubmitAsync(m_CommandBuffers[s_ImageIndex]);
            Context->m_Queue.Present(s_ImageIndex);
        }
    }
}
