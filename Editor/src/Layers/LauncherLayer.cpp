#include "LauncherLayer.h"
#include <imgui.h>
#include "EditorLayer.h"

void LauncherLayer::OnStart()
{
    m_EngineLogoTexture.LoadFromFile("res/logo.png");
    m_LoadProjectTexture.LoadFromFile("res/folder_puls.png");
    m_AddProjectTexture.LoadFromFile("res/plus_icon.png");
    m_DefaultProjectThumbnail.LoadFromFile("res/default_project_thumbnail.png");

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("res/Nexa-Heavy.ttf", 18.0f);

    // default prject path
    {
        // Default path set to "../" (one level up from current working directory)
        std::filesystem::path defaultPath = "../";

        // Get the absolute path of the default path
        std::filesystem::path absolutePath = std::filesystem::absolute(defaultPath);

        // Convert it to string to store it in your m_ProjectPath
        std::string fullPath = absolutePath.string();

        // Set the project path to the resolved absolute path
        strncpy_s(m_ProjectPath, fullPath.c_str(), sizeof(m_ProjectPath) - 1);
        m_ProjectPath[sizeof(m_ProjectPath) - 1] = '\0'; // Null-terminate the string
        
        m_RecentProjectsPath = std::string(m_ProjectPath);
    }

    ScanForRecentProjects();
}

void LauncherLayer::ScanForRecentProjects()
{
    m_RecentProjects.clear();
    
    if (!std::filesystem::exists(m_RecentProjectsPath))
    {
        std::filesystem::create_directories(m_RecentProjectsPath);
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(m_RecentProjectsPath))
    {
        if (entry.is_directory())
        {
            // Look for .pixproj file in this directory
            for (const auto& file : std::filesystem::directory_iterator(entry.path()))
            {
                if (file.path().extension() == ProjectExtension)
                {
                    RecentProject project;
                    project.name = entry.path().filename().string();
                    project.projectPath = entry.path();
                    project.projectFilePath = file.path();
                    m_RecentProjects.push_back(project);
                    break;  // Found the project file, move to next directory
                }
            }
        }
    }
}

void LauncherLayer::RenderRecentProjectTiles(float tileSize, float padding, int& itemCount, int tilesPerRow)
{
    for (const auto& project : m_RecentProjects)
    {
        // Add new row if needed
        if (itemCount % tilesPerRow != 0)
        {
            ImGui::SameLine(0, padding);
        }

        // Use m_LoadProjectTexture as the icon for recent projects
        if (RenderProjectTileDoubleClick(project.name.c_str(), m_DefaultProjectThumbnail, tileSize))
        {
            // Clicked on a recent project tile
            auto& CurrentProj = Engine::GetCurrentProjectRef();
            if (Project::LoadProject(CurrentProj, project.projectFilePath))
                m_ProjectLoaded = true;
        }

        itemCount++;
    }
}

void LauncherLayer::RenderNewProjectDialog()
{
    ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver); // Set a fixed size for the dialog
    if (ImGui::Begin("New Project", &m_ShowNewProjectDialog, ImGuiWindowFlags_NoCollapse))
    {
        // Use the larger font
        ImGuiIO& io = ImGui::GetIO();
        ImGui::PushFont(io.Fonts->Fonts[0]); // Assuming the larger font is the second font loaded

        // Title Section
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.0f, 0.9f, 0.0f, 1.0f });
        ImGui::TextWrapped("Create New Project");
        ImGui::PopStyleColor();
        ImGui::Separator();

        // Project Name Input
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.9f, 0.9f, 0.0f, 1.0f });
        ImGui::Text("Project Name:");
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.9f, 0.5f, 0.1f, 1.0f });
        ImGui::InputText("##ProjectName", m_ProjectName, sizeof(m_ProjectName));
        ImGui::PopStyleColor(2);
        ImGui::Spacing();

        // Project Path Input
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.9f, 0.9f, 0.0f, 1.0f });
        ImGui::Text("Location:");
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 70.0f); // Adjust width to fit browse button
        ImGui::PushStyleColor(ImGuiCol_Text, { 0.9f, 0.5f, 0.1f, 1.0f });
        ImGui::InputText("##ProjectPath", m_ProjectPath, sizeof(m_ProjectPath));
        ImGui::PopStyleColor(2);
        ImGui::PopItemWidth();

        ImGui::SameLine();
        if (ImGui::Button("Browse", ImVec2(60, 0))) // Browse button
        {
            auto* PlatformLayer = PIX3D::Engine::GetPlatformLayer();
            std::filesystem::path SelectedPath = PlatformLayer->OpenDialogue(FileDialougeFilter::PATH);
            if (!SelectedPath.string().empty())
            {
                strncpy_s(m_ProjectPath, SelectedPath.string().c_str(), sizeof(m_ProjectPath) - 1);
                m_ProjectPath[sizeof(m_ProjectPath) - 1] = '\0'; // Null-terminate the string
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Buttons Section
        float buttonWidth = 120.0f;
        float spacing = 10.0f;
        float totalButtonWidth = 2 * buttonWidth + spacing;

        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalButtonWidth) / 2); // Center buttons
        
        // Validate project path
        bool isValidPath = std::filesystem::exists(m_ProjectPath) && std::filesystem::is_directory(m_ProjectPath);

        // Disable "Create Project" button if path is invalid
        ImGui::BeginDisabled(!isValidPath);  // Disables button if path is invalid

        if (ImGui::Button("Create Project", ImVec2(buttonWidth, 0)))
        {
            auto& CurrentProj = Engine::GetCurrentProjectRef();
            CurrentProj.m_ProjectName = m_ProjectName;
            CurrentProj.m_ProjectPath = m_ProjectPath;
            CurrentProj.m_ProjectPath /= CurrentProj.m_ProjectName;
            CurrentProj.m_AssetPath = CurrentProj.m_ProjectPath / "Assets";

            CurrentProj.CreateProjectFiles();

            LayerManager::Get().GoToLayer(new EditorLayer());
        }

        ImGui::EndDisabled();  // End the disabled block

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
        {
            m_ShowNewProjectDialog = false;
        }

        ImGui::PopFont();
    }
    ImGui::End();
}


void LauncherLayer::OnUpdate(float dt)
{
    ImGuiLayer::StartDockSpace();

    // Logo and Header Bar
    {
        ImGui::SetNextWindowDockID(ImGui::GetID("MyDockSpace"), ImGuiCond_FirstUseEver);

        ImGui::Begin("Header", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse);

        auto AppSpecs = Engine::GetApplicationSpecs();
        float headerHeight = AppSpecs.Height * 0.3f;

        ImGui::GetWindowDrawList()->AddRectFilled(
            ImGui::GetWindowPos(),
            ImVec2(ImGui::GetWindowPos().x + AppSpecs.Width,
                ImGui::GetWindowPos().y + headerHeight),
            IM_COL32(0, 0, 0, 255));

        if (m_LoadProjectTexture.IsValid())
        {
            auto Size = ImGui::GetContentRegionAvail();
            ImGui::Image((ImTextureID)m_EngineLogoTexture.GetHandle(), Size);
        }

        ImGui::End();
    }

    {
        ImGui::Begin("PIX3D Launcher", nullptr, /*ImGuiWindowFlags_NoTitleBar*/0);

        // Top Bar
        /*
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 8));

            if (ImGui::Button("Your Projects"))
            {
                // LoadRecentProjects();
            }

            ImGui::SameLine();
            if (ImGui::Button("About")) {}

            ImGui::SameLine();
            if (ImGui::Button("Settings")) {}

            ImGui::SameLine();
            ImGui::PushItemWidth(200);
            static char search[128] = "";
            ImGui::InputText("##Search", search, sizeof(search), ImGuiInputTextFlags_AutoSelectAll);
            ImGui::PopItemWidth();

            ImGui::PopStyleVar();
            ImGui::Separator();
        }
        */

        // Projects grid
        {
            const float tileSize = 100.0f; // You can change this value or make it configurable
            const float padding = 10.0f;

            // Calculate how many tiles can fit in a row
            float availWidth = ImGui::GetContentRegionAvail().x;
            int tilesPerRow = static_cast<int>((availWidth - padding) / (tileSize + padding));
            tilesPerRow = std::max(tilesPerRow, 1); // At least one tile per row

            int itemCount = 0;

            // New Project Tile
            RenderProjectTile("New Project", m_AddProjectTexture, true, tileSize);
            itemCount++;

            // Add SameLine with proper spacing for grid layout
            if (itemCount % tilesPerRow != 0)
            {
                ImGui::SameLine(0, padding);
            }

            // Browse Project Tile
            if (RenderProjectTile("Open Project", m_LoadProjectTexture, false, tileSize))
            {
                // Open Project
                {
                    auto* PlatformLayer = PIX3D::Engine::GetPlatformLayer();
                    std::filesystem::path ProjectPath = PlatformLayer->OpenDialogue(FileDialougeFilter::PIXPROJ);

                    if (!ProjectPath.empty())
                    {
                        // Found a project file, try to load it
                        auto& CurrentProj = Engine::GetCurrentProjectRef();
                        PIX_ASSERT_MSG(PIX3D::Project::LoadProject(CurrentProj, ProjectPath), "failed to load project!");

                        // Successfully loaded project, switch to editor
                        m_ProjectLoaded = true;
                    }
                }
            }
            itemCount++;

            // Render Recent Projects Section
            if (!m_RecentProjects.empty())
            {
                ImGui::Spacing();
                ImGui::Spacing();

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.0f, 1.0f));
                ImGui::Text("Recent Projects");
                ImGui::PopStyleColor();

                ImGui::Spacing();

                // Start new row for recent projects
                itemCount = 0;  // Reset count for proper grid alignment
                RenderRecentProjectTiles(tileSize * 1.2f, padding, itemCount, tilesPerRow);
            }
        }

        ImGui::End();
    }


    if (m_ShowNewProjectDialog)
    {
        RenderNewProjectDialog();
    }

    ImGuiLayer::EndDockSpace();

    if(m_ProjectLoaded)
        LayerManager::Get().GoToLayer(new EditorLayer());
}

void LauncherLayer::OnDestroy()
{
    m_EngineLogoTexture.Destroy();
    m_LoadProjectTexture.Destroy();
    m_AddProjectTexture.Destroy();
}

bool LauncherLayer::RenderProjectTile(const char* name, GL::GLTexture& texture, bool isNewProject, float tileSize)
{
    const float titleHeight = 40.0f;
    const float imageSize = tileSize * 0.5f;
    bool clicked = false;

    ImGui::PushID(name);

    // Store the cursor position for the entire tile button
    ImVec2 startPos = ImGui::GetCursorScreenPos();
    ImVec2 endPos = ImVec2(startPos.x + tileSize, startPos.y + tileSize + titleHeight);

    // Make the entire tile clickable with invisible button
    ImGui::InvisibleButton(name, ImVec2(tileSize, tileSize + titleHeight));
    clicked = ImGui::IsItemClicked();
    bool hovered = ImGui::IsItemHovered();

    // Draw tile background
    ImGui::GetWindowDrawList()->AddRectFilled(
        startPos,
        endPos,
        IM_COL32(45, 45, 45, 255),  // Dark gray background
        8.0f  // Rounded corners
    );

    // Calculate center position for the image
    float imageX = startPos.x + (tileSize - imageSize) * 0.5f;
    float imageY = startPos.y + (tileSize - imageSize) * 0.5f;

    // Draw darker rectangle around image area
    ImGui::GetWindowDrawList()->AddRectFilled(
        ImVec2(imageX - 5, imageY - 5),
        ImVec2(imageX + imageSize + 5, imageY + imageSize + 5),
        IM_COL32(30, 30, 30, 255),  // Darker gray for inner rectangle
        4.0f  // Rounded corners
    );

    // Draw the image
    ImGui::GetWindowDrawList()->AddImage(
        (ImTextureID)texture.GetHandle(),
        ImVec2(imageX, imageY),
        ImVec2(imageX + imageSize, imageY + imageSize)
    );

    // Project name
    ImVec2 textSize = ImGui::CalcTextSize(name);
    float textX = startPos.x + (tileSize - textSize.x) * 0.5f;
    float textY = startPos.y + tileSize + (titleHeight - textSize.y) * 0.5f;

    ImGui::GetWindowDrawList()->AddText(
        ImVec2(textX, textY),
        IM_COL32(200, 200, 200, 255),
        name
    );

    // Hover effect for the entire tile
    if (hovered)
    {
        ImGui::GetWindowDrawList()->AddRect(
            startPos,
            endPos,
            IM_COL32(0, 122, 204, 255),  // Bright blue highlight
            8.0f,   // Rounded corners
            0,      // Flags
            2.0f    // Thickness
        );
    }

    ImGui::PopID();

    if (clicked && isNewProject)
    {
        //LayerManager::Get().GoToLayer(new EditorLayer());
        m_ShowNewProjectDialog = true;
    }

    return clicked;
}

bool LauncherLayer::RenderProjectTileDoubleClick(const char* name, GL::GLTexture& texture, float tileSize)
{
    const float titleHeight = 40.0f;
    const float imageSize = tileSize * 0.5f;
    bool activated = false;

    ImGui::PushID(name);

    // Store the cursor position for the entire tile button
    ImVec2 startPos = ImGui::GetCursorScreenPos();
    ImVec2 endPos = ImVec2(startPos.x + tileSize, startPos.y + tileSize + titleHeight);

    // Make the entire tile clickable with invisible button
    ImGui::InvisibleButton(name, ImVec2(tileSize, tileSize + titleHeight));
    bool hovered = ImGui::IsItemHovered();
    activated = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && hovered;

    // Draw tile background
    ImGui::GetWindowDrawList()->AddRectFilled(
        startPos,
        endPos,
        IM_COL32(45, 45, 45, 255),  // Dark gray background
        8.0f  // Rounded corners
    );

    // Calculate center position for the image
    float imageX = startPos.x + (tileSize - imageSize) * 0.5f;
    float imageY = startPos.y + (tileSize - imageSize) * 0.5f;

    // Draw darker rectangle around image area
    ImGui::GetWindowDrawList()->AddRectFilled(
        ImVec2(imageX - 5, imageY - 5),
        ImVec2(imageX + imageSize + 5, imageY + imageSize + 5),
        IM_COL32(30, 30, 30, 255),  // Darker gray for inner rectangle
        4.0f  // Rounded corners
    );

    // Draw the image
    ImGui::GetWindowDrawList()->AddImage(
        (ImTextureID)texture.GetHandle(),
        ImVec2(imageX, imageY),
        ImVec2(imageX + imageSize, imageY + imageSize)
    );

    // Project name
    ImVec2 textSize = ImGui::CalcTextSize(name);
    float textX = startPos.x + (tileSize - textSize.x) * 0.5f;
    float textY = startPos.y + tileSize + (titleHeight - textSize.y) * 0.5f;

    ImGui::GetWindowDrawList()->AddText(
        ImVec2(textX, textY),
        IM_COL32(200, 200, 200, 255),
        name
    );

    // Hover effect for the entire tile
    if (hovered)
    {
        ImGui::GetWindowDrawList()->AddRect(
            startPos,
            endPos,
            IM_COL32(0, 122, 204, 255),  // Bright blue highlight
            8.0f,   // Rounded corners
            0,      // Flags
            2.0f    // Thickness
        );
    }

    ImGui::PopID();

    return activated;
}
