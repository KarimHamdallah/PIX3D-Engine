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

    VulkanStaticMesh* AssetManager::GetStaticMesh(const PIX3D::UUID& uuid)
    {
        auto it = m_StaticMeshes.find(uuid);
        if (it != m_StaticMeshes.end())
            return it->second;
        return nullptr;
    }

    void AssetManager::UnloadStaticMesh(const PIX3D::UUID& uuid)
    {
        auto it = m_StaticMeshes.find(uuid);
        if (it != m_StaticMeshes.end())
        {
            delete it->second;  // Delete the mesh
            m_StaticMeshes.erase(it);
        }
    }

    VK::VulkanTexture* AssetManager::LoadTexture(const std::string& path, bool genMips, bool isSRGB)
    {
        // Check if texture is already loaded
        for (const auto& pair : m_Textures)
        {
            if (pair.second->GetPath() == path)
                return pair.second;
        }

        // Create and load new texture
        auto* texture = new VK::VulkanTexture();
        texture->LoadFromFile(path, genMips, isSRGB);
        m_Textures[texture->GetUUID()] = texture;
        return texture;
    }

    VK::VulkanTexture* AssetManager::LoadTextureUUID(UUID uuid, const std::string& path, bool genMips, bool isSRGB)
    {
        // Check if texture is already loaded
        for (const auto& pair : m_Textures)
        {
            if (pair.second->GetPath() == path)
                return pair.second;
        }

        // Create and load new texture
        auto* texture = new VK::VulkanTexture();
        texture->LoadFromFile(path, genMips, isSRGB);
        texture->SetUUID(uuid);
        m_Textures[texture->GetUUID()] = texture;
        return texture;
    }

    VK::VulkanTexture* AssetManager::GetTexture(const UUID& uuid)
    {
        auto it = m_Textures.find(uuid);
        if (it != m_Textures.end())
            return it->second;
        return nullptr;
    }

    void AssetManager::UnloadTexture(const UUID& uuid)
    {
        auto it = m_Textures.find(uuid);
        if (it != m_Textures.end())
        {
            it->second->Destroy();
            delete it->second;
            m_Textures.erase(it);
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

        // Clean up all textures
        for (const auto& pair : m_Textures)
        {
            pair.second->Destroy();
            delete pair.second;
        }
        m_Textures.clear();
    }
}
