#include "PixEditor.h"
#include <Platfrom/Vulkan/VulkanHelper.h>
#include <imgui.h>

void PixEditor::OnStart()
{
    auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();
    auto specs = Engine::GetApplicationSpecs();

    m_Mesh.Load("res/helmet/DamagedHelmet.gltf");

    Cam.Init({ 0.0f, 0.0f, 5.0f });

    VK::VulkanTexture* Texture = new VK::VulkanTexture();
    Texture->Create();
    Texture->LoadFromFile("res/samurai.png");

    m_Material.Create(Texture);

    // Set sprite properties
    m_Material.m_Data->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    m_Material.m_Data->tiling_factor = 1.0f;
    m_Material.m_Data->flip = 1;

    // Render sprite

    m_SpriteTransform.m_Position.x = 5;
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
    VK::VulkanSceneRenderer::RenderTexturedQuad(&m_Material, m_SpriteTransform);

    VK::VulkanSceneRenderer::End();


    VK::VulkanImGuiPass::BeginFrame();

    ImGui::Begin("Debug Window");

    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Image((ImTextureID)m_Material.m_Texture->GetImGuiDescriptorSet(), {256, 256});


    if (ImGui::Button("Click me!"))
    {
        // Handle button click
    }

    ImGui::End();

    VK::VulkanImGuiPass::EndFrame();

    VK::VulkanSceneRenderer::Submit();
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
