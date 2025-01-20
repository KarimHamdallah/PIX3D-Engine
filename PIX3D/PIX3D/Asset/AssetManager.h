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

        void SerializeRegistry(const std::string& filepath);
        void DeserializeRegistry(const std::string& filepath);

        void Destroy();

        inline static AssetManager& Get()
        {
            if (!s_Instance)
                s_Instance = new AssetManager();
            return *s_Instance;
        }

    public:
        std::unordered_map<uint64_t, VulkanStaticMesh*> m_StaticMeshes;
        inline static AssetManager* s_Instance = nullptr;
    };
}
