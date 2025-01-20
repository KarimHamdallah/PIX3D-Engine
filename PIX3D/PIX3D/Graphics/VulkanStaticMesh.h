#pragma once
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>
#include <Platfrom/Vulkan/VulkanDescriptorSetLayout.h>
#include <Platfrom/Vulkan/VulkanDescriptorSet.h>
#include <Platfrom/Vulkan/VulkanIndexBuffer.h>
#include <Asset/Asset.h>

namespace PIX3D
{
    // material data structure
    struct VulkanBaseColorMaterial
    {
        bool UseAlbedoTexture;
        bool UseNormalTexture;
        bool UseMetallicRoughnessTexture;
        bool useAoTexture;
        bool UseEmissiveTexture;

        glm::vec4 BaseColor;
        glm::vec4 EmissiveColor;

        float Metalic = 0.0f;
        float Roughness = 0.5f;
        float Ao = 1.0f;

        bool UseIBL = true;

        VK::VulkanTexture* AlbedoTexture;
        VK::VulkanTexture* NormalTexture;
        VK::VulkanTexture* MetalRoughnessTexture;
        VK::VulkanTexture* AoTexture;
        VK::VulkanTexture* EmissiveTexture;

        std::string Name;
    };

    // shader material info data structure
    struct VulkanMaterialGPUShader_InfoBufferData
    {
        alignas(4) int UseAlbedoTexture;
        alignas(4) int UseNormalTexture;
        alignas(4) int UseMetallicRoughnessTexture;
        alignas(4) int useAoTexture;
        alignas(4) int UseEmissiveTexture;

        alignas(16) glm::vec3 BaseColor;
        alignas(16) glm::vec3 EmissiveColor;

        alignas(4) float Metalic;
        alignas(4) float Roughness;
        alignas(4) float Ao;

        alignas(4) int UseIBL;
    };

    struct VulkanStaticMeshVertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec3 Tangent;
        glm::vec3 BiTangent;
        glm::vec2 TexCoords;

        static VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(VulkanStaticMeshVertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDescription;
        }

        static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions()
        {
            std::vector<VkVertexInputAttributeDescription> attributeDescriptions(5);

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(VulkanStaticMeshVertex, Position);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(VulkanStaticMeshVertex, Normal);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(VulkanStaticMeshVertex, Tangent);

            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[3].offset = offsetof(VulkanStaticMeshVertex, BiTangent);

            attributeDescriptions[4].binding = 0;
            attributeDescriptions[4].location = 4;
            attributeDescriptions[4].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[4].offset = offsetof(VulkanStaticMeshVertex, TexCoords);

            return attributeDescriptions;
        }
    };

    struct VulkanStaticSubMesh
    {
        uint32_t BaseVertex = 0;
        uint32_t BaseIndex = 0;
        uint32_t VerticesCount = 0;
        uint32_t IndicesCount = 0;
        uint32_t MaterialIndex = 0;
    };

    class VulkanStaticMesh : public Asset
    {
    public:
        VulkanStaticMesh() = default;
        void Load(const std::string& path, float scale = 1.0f);
        void Destroy();
        void FillMaterialBuffer();

        std::vector<VulkanStaticSubMesh> GetSubMeshes() const { return m_SubMeshes; }
        std::vector<VulkanBaseColorMaterial> GetMaterials() const { return m_Materials; }

        VkBuffer GetVertexBuffer() { return m_VertexBuffer.GetBuffer(); }
        VkBuffer GetIndexBuffer() { return m_IndexBuffer.GetBuffer(); }
        VkBuffer GetMaterialInfoBuffer() { return m_MaterialInfoBuffer.GetBuffer(); }

        VK::VulkanDescriptorSet& GetDescriptorSet(size_t index) { return m_DescriptorSets[index]; }

        virtual void Serialize(json& j) const override
        {
            j["path"] = m_Path.string();
            j["scale"] = m_Scale;
        }

        virtual void Deserialize(const json& j) override
        {
            std::string path = j["path"].get<std::string>();
            float scale = j["scale"].get<float>();
            Load(path, scale);
        }

    private:
        void ProcessNode(aiNode* node, const aiScene* scene);
        VulkanStaticSubMesh ProcessMesh(aiMesh* mesh, const aiScene* scene);
        void LoadGpuData();

    public:
        float m_Scale = 1.0f;

        std::vector<VulkanStaticMeshVertex> m_Vertices;
        std::vector<uint32_t> m_Indices;
        std::vector<VulkanStaticSubMesh> m_SubMeshes;
        std::vector<VulkanBaseColorMaterial> m_Materials;

        uint32_t m_BaseVertex = 0;
        uint32_t m_BaseIndex = 0;

        // Vulkan resources
        VK::VulkanVertexBuffer m_VertexBuffer;
        VK::VulkanIndexBuffer m_IndexBuffer;
        VK::VulkanShaderStorageBuffer m_MaterialInfoBuffer; // holds array of info data structure each element for each submesh accroding to submesh index

        std::vector<VK::VulkanDescriptorSet> m_DescriptorSets;
    };
}
