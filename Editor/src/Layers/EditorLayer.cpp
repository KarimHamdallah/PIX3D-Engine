#include "EditorLayer.h"
#include <imgui.h>
#include <Scripting Engine/ScriptingEngine.h>
#include <Scripting Engine/ScriptGlue.h>
#include <random>

void EditorLayer::OnStart()
{
    // Initialize the scene
    m_Scene = new PIX3D::Scene("IBL Scene");
    m_Scene->OnStart();

    // Initialize editor widgets
    m_LightningWidget = new LightningWidget(m_Scene);
    m_HierarchyWidget = new HierarchyWidget(m_Scene);
    m_InspectorWidget = new InspectorWidget(m_Scene, m_HierarchyWidget);
    m_MaterialWidget = new MaterialWidget(m_Scene, m_HierarchyWidget);
    m_AssetWidget = new AssetWidget();


    ////////// Run Time ////////////
    m_PlayIcon = new VK::VulkanTexture();
    m_PlayIcon->LoadFromFile("res/icons/PlayButton.png", false);

    m_StopIcon = new VK::VulkanTexture();
    m_StopIcon->LoadFromFile("res/icons/PauseButton.png", false);

    auto& CurrentProj = Engine::GetCurrentProjectRef();
    int x = 0;

    // Init Scripting Engine
    {
        std::filesystem::path corePath = "../PIX3D/Resources/Debug/net8.0/PIXScriptCore.dll";
        std::filesystem::path gamePath = "../PIX3D/Resources/Debug/net8.0/ExampleGame.dll";

        if (!ScriptEngine::Init(corePath))
        {
            PIX_DEBUG_ERROR("Failed to initialize scripting system!");
            return;
        }

        if (!ScriptEngine::LoadAppAssembly(gamePath))
        {
            PIX_DEBUG_ERROR("Failed to Load game script!");
            return;
        }
        
        ScriptGlue::SetScene(m_Scene);

        ScriptGlue::RegisterFunctions();
        ScriptEngine::OnRuntimeStart(m_Scene);
    }

    m_TerrainGrassTexture = PIX3D::AssetManager::Get().LoadTexture("res/terrain.png", true, true);
    m_GrassTexture = PIX3D::AssetManager::Get().LoadTexture("res/grass.png", true, true, true);

    // Setup Grass
    {
        // Setup (do this once)
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(0.8f, 1.0f);



        const float terrainSize = 100.0f;
        const float startOffset = -terrainSize / 2.0f;  // Start at -50
        const float GrassSpacing = terrainSize / GrassCountX;  // This will be 0.5f (100/200)

        {
            m_GrassTransformationMatrices.reserve(VK::VulkanSceneRenderer::s_GrassSpriteRenderpass.MAX_GRASS_COUNT);
            for (size_t z = 0; z < GrassCountZ; z++)
            {
                for (size_t x = 0; x < GrassCountX; x++)
                {
                    float randomSpacing = 0.8f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 0.2f));
                    TransformComponent transf;

                    // Calculate base position to cover -50 to +50 range
                    transf.m_Position.x = startOffset + (x * GrassSpacing * randomSpacing);
                    transf.m_Position.z = startOffset + (z * GrassSpacing * randomSpacing);
                    transf.m_Position.y = 0.5f;

                    for (size_t rot = 0; rot < 4; rot++)
                    {
                        switch (rot)
                        {
                        case 0: transf.m_Rotation.y = 0; break;
                        case 1: transf.m_Rotation.y = 45; break;
                        case 2: transf.m_Rotation.y = 90; break;
                        case 3: transf.m_Rotation.y = 135; break;
                        default:
                            break;
                        }
                        m_GrassTransformationMatrices.push_back(transf.GetTransformMatrix());
                    }
                }
            }

            // Update GPU Buffer
            VK::VulkanSceneRenderer::s_GrassSpriteRenderpass.GrassPositionsBuffer.
                UpdateData(
                    m_GrassTransformationMatrices.data(),
                    m_GrassTransformationMatrices.size() * sizeof(glm::mat4)
                );
        }
    }
}

void EditorLayer::OnUpdate(float dt)
{
    m_Time += dt;

    // Update the scene
    if (m_IsPlaying)
    {
        m_Scene->OnRunTimeUpdate(dt);
    }
    else
    {
        m_Scene->OnUpdate(dt);
    }

    // Handle input for mouse cursor visibility
    if (PIX3D::Input::IsKeyPressed(PIX3D::KeyCode::RightShift))
        ShowMouseCursor = false;
    else if (PIX3D::Input::IsKeyPressed(PIX3D::KeyCode::Escape))
        ShowMouseCursor = true;

    // Check for widget toggles using IsKeyPressedOnce
    if (Input::IsKeyPressedOnce(KeyCode::KB_1))
        m_ShowLightningWidget = !m_ShowLightningWidget;

    if (Input::IsKeyPressedOnce(KeyCode::KB_2))
        m_ShowHierarchyWidget = !m_ShowHierarchyWidget;

    if (Input::IsKeyPressedOnce(KeyCode::KB_3))
        m_ShowInspectorWidget = !m_ShowInspectorWidget;

    if (Input::IsKeyPressedOnce(KeyCode::KB_4))
        m_ShowMaterialWidget = !m_ShowMaterialWidget;

    if (Input::IsKeyPressedOnce(KeyCode::KB_5))
        m_ShowAssetWidget = !m_ShowAssetWidget;

    PIX3D::Engine::GetPlatformLayer()->ShowCursor(ShowMouseCursor);

    // Render the scene
    if (m_IsPlaying)
    {
        m_Scene->OnRunTimeRender();
    }
    else
    {
        m_Scene->OnRender();
    }

    VK::VulkanSceneRenderer::RenderTerrain(m_TerrainGrassTexture);


    uint32_t ImageIndex = VK::VulkanSceneRenderer::s_ImageIndex;

    VK::VulkanSceneRenderer::RenderTexturedQuadInstanced(m_GrassTexture, GrassCountX * GrassCountZ * 4, m_Time, m_WindStrength, m_WindDirection);

    // Bloom Pass
    if (VK::VulkanSceneRenderer::s_PostProcessingRenderpass.m_Data.BloomEnabled)
    {
        VK::VulkanSceneRenderer::s_BloomPass.RecordCommandBuffer(
            VK::VulkanSceneRenderer::s_MainRenderpass.BloomBrightnessAttachmentTexture,
            VK::VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[ImageIndex]);
    }

    // Full Screen Quad
    VK::VulkanSceneRenderer::s_PostProcessingRenderpass.RecordCommandBuffer(
        VK::VulkanSceneRenderer::s_MainRenderpass.ColorAttachmentTexture,
        VK::VulkanSceneRenderer::s_BloomPass.GetFinalBloomTexture(),
        VK::VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[ImageIndex],
        ImageIndex);

    VK::VulkanSceneRenderer::EndRecordCommandBuffer();


    // ImGui Pass

    VK::VulkanImGuiPass::BeginRecordCommandbuffer(ImageIndex);
    VK::VulkanImGuiPass::BeginFrame();

    // Render UI
    RenderMenuBar();
    RenderWidgets();
    RenderToolbar();

        if (ImGui::Begin("Wind Settings"))
        {
            // Wind strength slider
            ImGui::SliderFloat("Wind Strength", &m_WindStrength, 0.0f, 1.0f, "%.2f");

            // Wind direction as a 2D vector
            ImGui::Text("Wind Direction");
            ImGui::SliderFloat2("##WindDir", &m_WindDirection.x, -1.0f, 1.0f, "%.2f");

            // Optional: Add a normalize button for wind direction
            if (ImGui::Button("Normalize Direction")) {
                m_WindDirection = glm::normalize(m_WindDirection);
            }

            ImGui::End();
        }

    VK::VulkanImGuiPass::EndFrame();
    VK::VulkanImGuiPass::EndRecordCommandbufferAndSubmit(ImageIndex);

    auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

    Context->m_Queue.SubmitAsyncBuffers(
        {
            VK::VulkanSceneRenderer::s_MainRenderpass.CommandBuffers[ImageIndex],
            VK::VulkanImGuiPass::m_CommandBuffers[ImageIndex]
        });
    Context->m_Queue.Present(VK::VulkanSceneRenderer::s_ImageIndex);

    m_InspectorWidget->PostFrameProcesses();
    m_MaterialWidget->PostFrameProcesses();
}

void EditorLayer::OnDestroy()
{
    delete m_LightningWidget;
    m_LightningWidget = nullptr;
    delete m_HierarchyWidget;
    m_HierarchyWidget = nullptr;
    delete m_InspectorWidget;
    m_InspectorWidget = nullptr;
    delete m_MaterialWidget;
    m_MaterialWidget = nullptr;
    delete m_AssetWidget;
    m_AssetWidget = nullptr;
    delete m_Scene;
    m_Scene = nullptr;
}

void EditorLayer::OnKeyPressed(uint32_t key)
{
    PIX3D::KeyCode keycode = (PIX3D::KeyCode)key;

    if (keycode == PIX3D::KeyCode::LeftControl)
    {
        // Save Scene
        if (PIX3D::Input::IsKeyPressed(PIX3D::KeyCode::S))
        {
            SaveSceneDialogue();
        }

        // Load Scene
        if (PIX3D::Input::IsKeyPressed(PIX3D::KeyCode::D))
        {
            LoadSceneDialogue();
        }
    }
}

void EditorLayer::SaveSceneDialogue()
{
    auto* PlatformLayer = PIX3D::Engine::GetPlatformLayer();

    std::filesystem::path ScenePath = PlatformLayer->SaveDialogue(FileDialougeFilter::PIXSCENE);
    if (!ScenePath.string().empty())
    {
        PIX3D::SceneSerializer serializer(m_Scene);
        serializer.Serialize(ScenePath.string());
    }
}

void EditorLayer::LoadSceneDialogue()
{
    auto* PlatformLayer = PIX3D::Engine::GetPlatformLayer();

    std::filesystem::path ScenePath = PlatformLayer->OpenDialogue(FileDialougeFilter::PIXSCENE);
    if (!ScenePath.string().empty() && ScenePath.extension().string() == ".pixscene")
    {
        PIX3D::SceneSerializer serializer(m_Scene);
        serializer.Deserialize(ScenePath.string());
    }
}

void EditorLayer::RenderMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save Scene ...", "Cntrl + S"))
            {
                SaveSceneDialogue();
            }

            if (ImGui::MenuItem("Load Scene ...", "Cntrl + D"))
            {
                LoadSceneDialogue();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window"))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, m_ShowLightningWidget ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            if (ImGui::MenuItem("Lightning"))
            {
                m_ShowLightningWidget = !m_ShowLightningWidget;
            }
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Text, m_ShowHierarchyWidget ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            if (ImGui::MenuItem("Hierarchy"))
            {
                m_ShowHierarchyWidget = !m_ShowHierarchyWidget;
            }
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Text, m_ShowInspectorWidget ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            if (ImGui::MenuItem("Inspector"))
            {
                m_ShowInspectorWidget = !m_ShowInspectorWidget;
            }
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Text, m_ShowMaterialWidget ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            if (ImGui::MenuItem("Material"))
            {
                m_ShowMaterialWidget = !m_ShowMaterialWidget;
            }
            ImGui::PopStyleColor();

            ImGui::PushStyleColor(ImGuiCol_Text, m_ShowAssetWidget ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            if (ImGui::MenuItem("Asset"))
            {
                m_ShowAssetWidget = !m_ShowAssetWidget;
            }
            ImGui::PopStyleColor();

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void EditorLayer::RenderWidgets()
{
    if (m_ShowLightningWidget)
        m_LightningWidget->OnRender();
    if (m_ShowHierarchyWidget)
        m_HierarchyWidget->OnRender();
    if (m_ShowInspectorWidget)
        m_InspectorWidget->OnRender();
    if (m_ShowMaterialWidget)
        m_MaterialWidget->OnRender();
    if (m_ShowAssetWidget)
        m_AssetWidget->OnRender();
}

void EditorLayer::RenderToolbar()
{
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar;

    ImGui::Begin("Debug", nullptr, window_flags);
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));

        // Play Button
        ImVec4 buttonColor = m_IsPlaying ? ImVec4(0.3f, 0.8f, 0.3f, 1.0f) : ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);

        if (ImGui::ImageButton("Play", (ImTextureID)m_PlayIcon->GetImGuiDescriptorSet(),
            ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1)))
        {
            if (!m_IsPlaying)
            {
                m_IsPlaying = true;

                // Store Current Edior Scene
                m_TempScene = PIX3D::Scene::CopyScene(m_Scene);

                // Call OnCreate For All Scripts
                m_Scene->OnRunTimeStart();
            }
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();

        // Stop Button
        if (m_IsPlaying)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 1.0f));
            if (ImGui::ImageButton("Stop", (ImTextureID)m_StopIcon->GetImGuiDescriptorSet(),
                ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1)))
            {
                m_IsPlaying = false;
                
                // Call OnDestroy For All Scripts
                m_Scene->OnRunTimeEnd();

                // Destroy Scene
                delete m_Scene;
                m_Scene = nullptr;

                m_Scene = PIX3D::Scene::CopyScene(m_TempScene);

                delete m_TempScene;
                m_TempScene = nullptr;
            }
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
            ImGui::ImageButton("Stop", (ImTextureID)m_StopIcon->GetImGuiDescriptorSet(),
                ImVec2(24, 24), ImVec2(0, 0), ImVec2(1, 1));
            ImGui::PopStyleColor();
        }

        ImGui::PopStyleVar();
    }
    ImGui::End();
}
