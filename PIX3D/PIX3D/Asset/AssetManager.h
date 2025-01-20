#pragma once
#include <string>
#include "Core/UUID.h"
#include <unordered_map>
#include "Graphics/VulkanStaticMesh.h"

namespace PIX3D
{
    class AssetManager
    {
    public:
        // Static Mesh management
        VulkanStaticMesh* LoadStaticMesh(const std::string& path, float scale = 1.0f);
        VulkanStaticMesh* LoadStaticMeshUUID(PIX3D::UUID uuid, const std::string& path, float scale = 1.0f);
        VulkanStaticMesh* GetStaticMesh(const UUID& uuid);
        void UnloadStaticMesh(const UUID& uuid);

        // Texture management
        VK::VulkanTexture* LoadTexture(const std::string& path, bool genMips = true, bool isSRGB = false);
        VK::VulkanTexture* LoadTextureUUID(PIX3D::UUID uuid, const std::string& path, bool genMips = true, bool isSRGB = true);
        VK::VulkanTexture* GetTexture(const PIX3D::UUID& uuid);
        void UnloadTexture(const PIX3D::UUID& uuid);

        void Destroy();

        inline static AssetManager& Get()
        {
            if (!s_Instance)
                s_Instance = new AssetManager();
            return *s_Instance;
        }

    public:
        std::unordered_map<uint64_t, VulkanStaticMesh*> m_StaticMeshes;
        std::unordered_map<uint64_t, VK::VulkanTexture*> m_Textures;
        inline static AssetManager* s_Instance = nullptr;
    };
}
