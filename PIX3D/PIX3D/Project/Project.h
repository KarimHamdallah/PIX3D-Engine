#pragma once
#include <filesystem>
#include <Utils/json.hpp>
#include <fstream>

namespace
{
    bool FileExists(const std::string& name)
    {
        std::ifstream f(name.c_str());
        return f.good();
    }
}

namespace PIX3D
{
    constexpr auto ProjectExtension = ".pixproj";

    struct Project
    {
    public:

        bool IsValid()
        {
            return !m_ProjectName.empty();
        }

        std::string GetProjectName()
        {
            return m_ProjectName.string();
        }

        std::filesystem::path GetAssetsPath()
        {
            if (m_AssetPath.empty())
                m_AssetPath = m_ProjectPath / m_ProjectName / "Asset";

            return m_AssetPath;
        }

        // Serialize the project to JSON
        void ToJson(nlohmann::json& j) const
        {
            j["project_name"] = m_ProjectName.string();
            j["project_path"] = m_ProjectPath.string();
            j["asset_path"] = m_AssetPath.string();
        }

        // Deserialize the project from JSON
        void FromJson(const nlohmann::json& j)
        {
            m_ProjectName = j.value("project_name", "");
            m_ProjectPath = j.value("project_path", "");
            m_AssetPath = j.value("asset_path", "");
        }

        void CreateProjectFiles()
        {
            // Create the directories for the project and asset folder
            std::filesystem::create_directories(m_ProjectPath);           // Create the main project folder
            std::filesystem::create_directories(m_AssetPath); // Create the asset folder inside the project
            SaveProjectDataAsJson(*this); // save project data as json file
        }

        // Save Project to a .json file
        static void SaveProjectDataAsJson(const Project& project)
        {
            std::filesystem::path jsonFilePath = project.m_ProjectPath / (project.m_ProjectName.string() + ProjectExtension);
            nlohmann::json j;
            project.ToJson(j);
            std::ofstream out(jsonFilePath);
            out << j.dump(4); // Write the JSON with pretty printing (indent of 4 spaces)
            out.close();
        }

        // Load Project from a .json file
        static bool LoadProject(Project& project, const std::filesystem::path& project_file_path)
        {
            if (FileExists(project_file_path.string()))
            {
                std::ifstream in(project_file_path);
                nlohmann::json j;
                in >> j;
                project.FromJson(j);
                in.close();
                return true;
            }
            return false;
        }

    public:
        std::filesystem::path m_ProjectName = "";
        std::filesystem::path m_ProjectPath = "";
        std::filesystem::path m_AssetPath = "";
    };
}
