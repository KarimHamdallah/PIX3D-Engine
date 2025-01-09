#include "VulkanStaticMesh.h"
#include <Core/Core.h>
#include <fstream>
#include <Engine/Engine.hpp>
#include <Platfrom/Vulkan/VulkanHelper.h>

namespace PIX3D
{
    void VulkanStaticMesh::Load(const std::string& path, float scale)
    {
        m_Path = path;
        m_Scale = scale;

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path,
            aiProcess_Triangulate |
            aiProcess_FlipUVs |
            aiProcess_PreTransformVertices |
            aiProcess_GenNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_JoinIdenticalVertices
        );

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            PIX_ASSERT_MSG(false, importer.GetErrorString());
        }

        ProcessNode(scene->mRootNode, scene);

        // Load Into GPU
        LoadGpuData();
    }

    void VulkanStaticMesh::FillMaterialBuffer()
    {
        std::vector<VulkanMaterialGPUShader_InfoBufferData> info;

        for (auto& mat : m_Materials)
        {
            VulkanMaterialGPUShader_InfoBufferData data;

            data.UseAlbedoTexture = mat.UseAlbedoTexture;
            data.UseNormalTexture = mat.UseNormalTexture;
            data.UseMetallicRoughnessTexture = mat.UseMetallicRoughnessTexture;
            data.useAoTexture = mat.useAoTexture;
            data.UseEmissiveTexture = mat.UseEmissiveTexture;

            data.BaseColor = mat.BaseColor;
            data.EmissiveColor = mat.EmissiveColor;

            data.Metalic = mat.Metalic;
            data.Roughness = mat.Roughness;
            data.Ao = mat.Ao;
            data.UseIBL = mat.UseIBL;

            info.push_back(data);
        }

        m_MaterialInfoBuffer.UpdateData(info.data(), info.size() * sizeof(MaterialGPUShader_InfoBufferData));
    }

    void VulkanStaticMesh::Destroy()
    {
        m_VertexBuffer.Destroy();
        m_IndexBuffer.Destroy();
        m_MaterialInfoBuffer.Destroy();

        for (auto& material : m_Materials)
        {
            if (material.UseAlbedoTexture) material.AlbedoTexture->Destroy();
            if (material.UseNormalTexture) material.NormalTexture->Destroy();
            if (material.UseMetallicRoughnessTexture) material.MetalRoughnessTexture->Destroy();
            if (material.useAoTexture) material.AoTexture->Destroy();
            if (material.UseEmissiveTexture) material.EmissiveTexture->Destroy();
        }

        // free descriptor sets
        /*
        for (auto& descriptorSet : m_DescriptorSets)
        {
            descriptorSet.Destroy();
        }
        */
    }

    void VulkanStaticMesh::ProcessNode(aiNode* node, const aiScene* scene)
    {
        // process all the node's meshes (if any)
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            m_SubMeshes.push_back(ProcessMesh(mesh, scene));
        }

        // then do the same for each of its children
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            ProcessNode(node->mChildren[i], scene);
        }
    }

    VulkanStaticSubMesh VulkanStaticMesh::ProcessMesh(aiMesh* mesh, const aiScene* scene)
    {
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            VulkanStaticMeshVertex vertex;

            vertex.Position.x = mesh->mVertices[i].x * m_Scale;
            vertex.Position.y = mesh->mVertices[i].y * m_Scale;
            vertex.Position.z = mesh->mVertices[i].z * m_Scale;

            vertex.Normal.x = mesh->mNormals[i].x;
            vertex.Normal.y = mesh->mNormals[i].y;
            vertex.Normal.z = mesh->mNormals[i].z;

            if (mesh->HasTangentsAndBitangents())
            {
                vertex.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
                vertex.BiTangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
            }

            if (mesh->mTextureCoords[0])
            {
                vertex.TexCoords.x = mesh->mTextureCoords[0][i].x;
                vertex.TexCoords.y = mesh->mTextureCoords[0][i].y;
            }
            else
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);

            m_Vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                m_Indices.push_back(face.mIndices[j]);
        }

        // Load Material
        bool HasMaterial = mesh->mMaterialIndex >= 0;
        int MaterialIndex = -1;

        if (HasMaterial)
        {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            VulkanBaseColorMaterial mat;
            mat.Name = material->GetName().C_Str();

            // Get Base Color
            aiColor4D DiffuseColor;
            material->Get(AI_MATKEY_COLOR_DIFFUSE, DiffuseColor);
            mat.BaseColor = { DiffuseColor.r, DiffuseColor.g, DiffuseColor.b, 1.0f };
            mat.EmissiveColor = { 0.0f, 0.0f, 0.0f, 1.0f };

            // Fill Metalic/Roughness
            float Metallic = 0.0f;
            if (AI_SUCCESS == aiGetMaterialFloat(material, AI_MATKEY_METALLIC_FACTOR, &Metallic))
                mat.Metalic = Metallic;

            float Roughness = 1.0f;
            if (AI_SUCCESS == aiGetMaterialFloat(material, AI_MATKEY_ROUGHNESS_FACTOR, &Roughness))
                mat.Roughness = Roughness;

            // Load Textures
           // Load AlbedoMap
            {
                aiString path;
                material->GetTexture(aiTextureType_DIFFUSE, 0, &path);
                std::filesystem::path FullPath = (m_Path.parent_path() / path.C_Str()).string();

                if (FileExists(FullPath.string())) // AlbedoMap Found
                {
                    mat.UseAlbedoTexture = true;
                    
                    mat.AlbedoTexture = new VK::VulkanTexture();
                    mat.AlbedoTexture->Create();
                    
                    mat.AlbedoTexture->LoadFromFile(FullPath.string(), true);
                }
                else // Default Albedo
                {
                    mat.UseAlbedoTexture = false;
                    mat.AlbedoTexture = VK::VulkanSceneRenderer::GetDefaultAlbedoTexture();
                }
            }

            // Load NormalMap
            {
                aiString path;

                auto ext = m_Path.extension().string();

                aiTextureType type = aiTextureType_NORMALS;

                if (ext == ".obj")
                    type = aiTextureType_HEIGHT;


                material->GetTexture(type, 0, &path);
                std::filesystem::path FullPath = (m_Path.parent_path() / path.C_Str()).string();

                if (FileExists(FullPath.string())) // NormalMap Found
                {
                    mat.UseNormalTexture = true;

                    mat.NormalTexture = new VK::VulkanTexture();
                    mat.NormalTexture->Create();

                    mat.NormalTexture->LoadFromFile(FullPath.string());
                }
                else
                {
                    mat.UseNormalTexture = false;
                    mat.NormalTexture = VK::VulkanSceneRenderer::GetDefaultNormalTexture();
                }
            }

            // Load MetalRoughnessMap
            {
                aiString path;

                auto ext = m_Path.extension().string();

                aiTextureType type = aiTextureType_METALNESS;

                if (ext == ".obj")
                    type = aiTextureType_SPECULAR;

                material->GetTexture(type, 0, &path);
                std::filesystem::path FullPath = (m_Path.parent_path() / path.C_Str()).string();

                if (FileExists(FullPath.string())) // RoughnessMap Found
                {
                    mat.UseMetallicRoughnessTexture = true;
                    
                    mat.MetalRoughnessTexture = new VK::VulkanTexture();
                    mat.MetalRoughnessTexture->Create();

                    mat.MetalRoughnessTexture->LoadFromFile(FullPath.string());
                }
                else
                {
                    mat.UseMetallicRoughnessTexture = false;
                    mat.MetalRoughnessTexture = VK::VulkanSceneRenderer::GetDefaultBlackTexture();
                }
            }

            // Load AoMap
            {
                aiString path;
                material->GetTexture(aiTextureType_LIGHTMAP, 0, &path);
                std::filesystem::path FullPath = (m_Path.parent_path() / path.C_Str()).string();

                if (FileExists(FullPath.string())) // RoughnessMap Found
                {
                    mat.useAoTexture = true;

                    mat.AoTexture = new VK::VulkanTexture();
                    mat.AoTexture->Create();

                    mat.AoTexture->LoadFromFile(FullPath.string());
                }
                else
                {
                    mat.useAoTexture = false;
                    mat.AoTexture = VK::VulkanSceneRenderer::GetDefaultWhiteTexture();
                }
            }

            // Load EmissiveMap
            {
                aiString path;
                material->GetTexture(aiTextureType_EMISSIVE, 0, &path);
                std::filesystem::path FullPath = (m_Path.parent_path() / path.C_Str()).string();

                if (FileExists(FullPath.string())) // RoughnessMap Found
                {
                    mat.UseEmissiveTexture = true;

                    mat.EmissiveTexture = new VK::VulkanTexture();
                    mat.EmissiveTexture->Create();

                    mat.EmissiveTexture->LoadFromFile(FullPath.string());
                }
                else
                {
                    mat.UseEmissiveTexture = false;
                    mat.EmissiveTexture = VK::VulkanSceneRenderer::GetDefaultBlackTexture();
                }
            }

            m_Materials.push_back(mat);
            MaterialIndex = m_Materials.size() - 1;
        }

        VulkanStaticSubMesh subMesh;
        subMesh.BaseVertex = m_BaseVertex;
        subMesh.BaseIndex = m_BaseIndex;
        subMesh.VerticesCount = mesh->mNumVertices;
        subMesh.IndicesCount = mesh->mNumFaces * 3;
        subMesh.MaterialIndex = MaterialIndex;

        m_BaseVertex += mesh->mNumVertices;
        m_BaseIndex += mesh->mNumFaces * 3;

        return subMesh;
    }

    void VulkanStaticMesh::LoadGpuData()
    {
        auto* Context = (VK::VulkanGraphicsContext*)Engine::GetGraphicsContext();

        m_VertexBuffer.Create();
        m_VertexBuffer.FillData(m_Vertices.data(), sizeof(StaticMeshVertex) * m_Vertices.size());

        m_IndexBuffer.Create();
        m_IndexBuffer.FillData(m_Indices.data(), sizeof(uint32_t) * m_Indices.size());

        m_MaterialInfoBuffer.Create(m_Materials.size() * sizeof(MaterialGPUShader_InfoBufferData));
        FillMaterialBuffer();

        // create descriptor layout (set = 1) -- material set
        m_MaterialDescriptorSetLayout
            .AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
            .Build();

        // create descriptor sets for each material
        m_DescriptorSets.resize(m_Materials.size());

        for (size_t i = 0; i < m_Materials.size(); i++)
        {
            auto& material = m_Materials[i];

            m_DescriptorSets[i].Init(m_MaterialDescriptorSetLayout)
                .AddTexture(0, *material.AlbedoTexture)
                .Build();
        }
    }
}
