#include "AssetManager.h"
#include <fstream>
#include "Utils/json.hpp"
#include <Core/Core.h>

namespace PIX3D
{
    VulkanStaticMesh* AssetManager::LoadStaticMesh(const std::string& path, float scale)
    {
        // Check if mesh is already loaded
        for (const auto& pair : m_StaticMeshes)
        {
            if (pair.second->GetPath() == path)
                return pair.second;
        }

        // Create and load new mesh
        auto* mesh = new VulkanStaticMesh();
        mesh->Load(path, scale);
        m_StaticMeshes[mesh->GetUUID()] = mesh;
        return mesh;
    }

    VulkanStaticMesh* AssetManager::LoadStaticMeshUUID(PIX3D::UUID uuid, const std::string& path, float scale)
    {
        // Check if mesh is already loaded
        for (const auto& pair : m_StaticMeshes)
        {
            if (pair.second->GetPath() == path)
                return pair.second;
        }

        // Create and load new mesh
        auto* mesh = new VulkanStaticMesh();
        mesh->Load(path, scale);
        mesh->SetUUID(uuid);
        m_StaticMeshes[mesh->GetUUID()] = mesh;
        return mesh;
    }

    VulkanStaticMesh* AssetManager::GetStaticMesh(const UUID& uuid)
    {
        auto it = m_StaticMeshes.find(uuid);
        if (it != m_StaticMeshes.end())
            return it->second;
        return nullptr;
    }

    void AssetManager::UnloadStaticMesh(const UUID& uuid)
    {
        auto it = m_StaticMeshes.find(uuid);
        if (it != m_StaticMeshes.end())
        {
            delete it->second;  // Delete the mesh
            m_StaticMeshes.erase(it);
        }
    }

    void AssetManager::SerializeRegistry(const std::string& filepath)
    {
        json j;
        j["staticMeshes"] = json::array();

        for (const auto& pair : m_StaticMeshes)
        {
            json meshJson;
            meshJson["uuid"] = (uint64_t)pair.first;
            pair.second->Serialize(meshJson);
            j["staticMeshes"].push_back(meshJson);
        }

        std::ofstream file(filepath);
        if (file.is_open())
        {
            file << std::setw(4) << j << std::endl;
            file.close();
        }
    }

    void AssetManager::DeserializeRegistry(const std::string& filepath)
    {
        std::ifstream file(filepath);
        if (!file.is_open())
            return;

        try
        {
            json j;
            file >> j;
            file.close();

            for (const auto& meshJson : j["staticMeshes"])
            {
                std::string path = meshJson["path"].get<std::string>();
                float scale = meshJson["scale"].get<float>();
                LoadStaticMesh(path, scale);
            }
        }
        catch (const std::exception& e)
        {
            // Handle JSON parsing errors
            PIX_DEBUG_ERROR_FORMAT("Failed to parse asset registry: {0}", e.what());
        }
    }

    void AssetManager::Destroy()
    {
        // Clean up all meshes
        for (const auto& pair : m_StaticMeshes)
        {
            pair.second->Destroy();
            delete pair.second;
        }
        m_StaticMeshes.clear();
    }
}
