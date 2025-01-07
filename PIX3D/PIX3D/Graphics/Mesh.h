#pragma once
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <Platfrom/GL/GLTexture.h>
#include <Platfrom/GL/GLVertexArray.h>
#include <Platfrom/GL/GLShader.h>
#include <Platfrom/GL/GLStorageBuffer.h>
#include <filesystem>

namespace PIX3D
{
	struct BaseColorMaterial
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

		GL::GLTexture AlbedoTexture;
		GL::GLTexture NormalTexture;
		GL::GLTexture MetalRoughnessTexture;
		GL::GLTexture AoTexture;
		GL::GLTexture EmissiveTexture;
		
		std::string Name;
	};

	// Shared Buffer With Shader
	struct MaterialGPUShader_TextureBufferData
	{
		uint64_t AlbedoTexture = 0;
		uint64_t NormalTexture = 0;
		uint64_t MetalRoughnessTexture = 0;
		uint64_t AoTexture = 0;
		uint64_t EmissiveTexture = 0;
	};

	struct MaterialGPUShader_InfoBufferData
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


	struct StaticMeshVertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 BiTangent;
		glm::vec2 TexCoords;
	};

	struct StaticSubMesh
	{
		uint32_t BaseVertex = 0;
		uint32_t BaseIndex = 0;
		uint32_t VerticesCount = 0;
		uint32_t IndicesCount = 0;
		uint32_t MaterialIndex = 0;
	};

	class StaticMesh
	{
	public:
		StaticMesh() = default;

		void Load(const std::string& path, float scale = 1.0f);
		void Destroy();

		void FillMaterialBuffer();
		
		std::vector<StaticSubMesh> GetSubMeshes() const { return m_SubMeshes; }
		std::vector<BaseColorMaterial> GetMaterials() const { return m_Materials; }
		GL::GLVertexArray GetVertexArray() const { return m_VertexArray; }

		void BindMaterialBuffer(uint32_t binding_point);

	private:
		void ProcessNode(aiNode* node, const aiScene* scene);
		StaticSubMesh ProcessMesh(aiMesh* mesh, const aiScene* scene);

		void LoadGpuData();

	public:
		std::filesystem::path m_Path;
		float m_Scale = 1.0f;

		std::vector<StaticMeshVertex> m_Vertices;
		std::vector<uint32_t> m_Indices;

		std::vector<StaticSubMesh> m_SubMeshes;
		std::vector<BaseColorMaterial> m_Materials;
		GL::GLStorageBuffer m_MaterialTextureDataStorageBuffer;
		
		uint32_t m_BaseVertex, m_BaseIndex = 0;

		// Gpu Data
		GL::GLVertexBuffer m_Vertexbuffer;
		GL::GLIndexBuffer m_Indexbuffer;
		GL::GLVertexArray m_VertexArray;
		GL::GLStorageBuffer m_MaterialInfoDataStorageBuffer;
	};
}
