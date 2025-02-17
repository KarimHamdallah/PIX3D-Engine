#pragma once
#include "VulkanTexture.h"
#include "VulkanDescriptorSetLayout.h"
#include <Platfrom/Vulkan/VulkanPostProcessingRenderpass.h>
#include <Platfrom/Vulkan/VulkanBloomPass.h>

#include <Platfrom/Vulkan/IBL/VulkanIrradianceCubemap.h>
#include <Platfrom/Vulkan/IBL/VulkanPrefilteredCubeMap.h>
#include <Platfrom/Vulkan/IBL/VulkanBrdfLutTexture.h>
#include <Graphics/VulkanStaticMesh.h>
#include <Graphics/Camera3D.h>
#include <Graphics/Material.h>
#include <Scene/SceneStructures.h>
#include <Scene/Scene.h>

namespace PIX3D
{
	namespace VK
	{
		class VulkanSceneRenderer
		{
		public:
			static void Init(uint32_t width, uint32_t height);
			static void Destroy();

			static void Begin(Camera3D& cam);
			static void RenderClearPass(const glm::vec4& clearColor);
			static void RenderTerrain(VulkanTexture* texture);
			static void RenderMesh(Scene* scene, VulkanStaticMesh& mesh, const glm::mat4& transform);
			static void RenderMeshOutline(Scene* scene, VulkanStaticMesh& mesh, const glm::mat4& transform);
			static void RenderTexturedQuad(SpriteMaterial* material, const glm::mat4& transform);
			static void RenderTexturedQuadInstanced(VulkanTexture* texture, uint32_t instanceCount, float _time, float wind_strength, glm::vec2 wind_movement);
			static void RenderSkyBox();
			static void End();
			static void EndRecordCommandBuffer();
			static void Submit();

			static void OnResize(uint32_t width, uint32_t height);


			inline static VulkanTexture* GetDefaultAlbedoTexture() { return s_DefaultAlbedoTexture; }
			inline static VulkanTexture* GetDefaultNormalTexture() { return s_DefaultNormalTexture; }

			inline static VulkanTexture* GetDefaultWhiteTexture() { return s_DefaultWhiteTexture; }
			inline static VulkanTexture* GetDefaultBlackTexture() { return s_DefaultBlackTexture; }

			inline static VkDescriptorSetLayout GetVulkanStaticMeshMaterialDescriptorSetLayout() { return s_VulkanStaticMeshMaterialDescriptorSetLayout.GetVkDescriptorSetLayout(); }


		public:

			///////////////////// Default Textures //////////////////////////

			inline static VulkanTexture* s_DefaultAlbedoTexture;
			inline static VulkanTexture* s_DefaultNormalTexture;
			inline static VulkanTexture* s_DefaultWhiteTexture;
			inline static VulkanTexture* s_DefaultBlackTexture;

			///////////////////// Material Descriptor Set Layout //////////////////////////

			inline static VulkanDescriptorSetLayout s_VulkanStaticMeshMaterialDescriptorSetLayout;


			/////////////////////// Environment DescriptorSets ////////////////////////////

			inline static VulkanDescriptorSetLayout s_EnvironmetDescriptorSetLayout;
			inline static VulkanDescriptorSet s_EnvironmetDescriptorSet;
			
			///////////////////// Camera Shader Set Data //////////////////////////
			struct _CameraUniformBuffer
			{
				glm::mat4 proj;
				glm::mat4 view;
				glm::mat4 skybox_view;
			};

			inline static std::vector<VK::VulkanUniformBuffer> s_CameraUniformBuffers;
			inline static VK::VulkanDescriptorSetLayout s_CameraDescriptorSetLayout;
			inline static std::vector<VK::VulkanDescriptorSet> s_CameraDescriptorSets;

			/////////////////////////  Model Matrix Push Constant //////////////////////////////////

			struct _ModelMatrixPushConstant
			{
				glm::mat4 model;
				glm::vec3 CameraPosition;
				float MeshIndex;
				float BloomThreshold;
				float PointLightCount;
			};


			/////////////////////////  Main Render Pass //////////////////////////////////

			enum
			{
				BLOOM_DOWN_SAMPLES = 6
			};

			struct _ClearRenderpass
			{
				VK::VulkanRenderPass Renderpass;
				VK::VulkanFramebuffer Framebuffer;
			};

			inline static _ClearRenderpass s_ClearRenderpass;

			struct _MainRenderpass
			{
				VK::VulkanShader Shader;
				VK::VulkanShader OutlineShader;
				
				VK::VulkanVertexInputLayout VertexInputLayout;

				VkPipelineLayout PipelineLayout = nullptr;
				VK::VulkanGraphicsPipeline GraphicsPipeline;

				VkPipelineLayout OutlinePipelineLayout = nullptr;
				VK::VulkanGraphicsPipeline OutlineGraphicsPipeline;

				VK::VulkanTexture* ColorAttachmentTexture;
				VK::VulkanTexture* BloomBrightnessAttachmentTexture;

				VK::VulkanRenderPass Renderpass;
				
				VK::VulkanFramebuffer Framebuffer;

				std::vector<VkCommandBuffer> CommandBuffers;
			};

			inline static _MainRenderpass s_MainRenderpass;

			inline static VK::VulkanDescriptorSetLayout s_PointLightsDescriptorSetLayout;


			struct _SkyBoxPass
			{
				VK::VulkanShader Shader;

				VK::VulkanVertexInputLayout VertexInputLayout;

				VK::VulkanDescriptorSetLayout DescriptorSetLayout;
				VK::VulkanDescriptorSet DescriptorSet;

				VkPipelineLayout PipelineLayout = nullptr;
				VK::VulkanGraphicsPipeline GraphicsPipeline;

				VK::VulkanRenderPass Renderpass;

				VK::VulkanFramebuffer Framebuffer;

				VulkanStaticMeshData CubeMesh;
			};

			inline static _SkyBoxPass s_SkyBoxPass;

			struct _SpriteRenderpass
			{
				VK::VulkanShader Shader;

				VK::VulkanDescriptorSetLayout DescriptorSetLayout;

				VkPipelineLayout PipelineLayout = nullptr;
				VK::VulkanGraphicsPipeline GraphicsPipeline;

				VulkanStaticMeshData SpriteMesh;
			};

			inline static _SpriteRenderpass s_SpriteRenderpass;


			struct _GrassPushConstant
			{
				float time;
				float wind_strength;
				glm::vec2 wind_movement;
			};

			struct _GrassSpriteRenderpass
			{
				VK::VulkanShader Shader;

				VK::VulkanDescriptorSetLayout DescriptorSetLayout;
				VK::VulkanDescriptorSet DescriptorSet;

				VkPipelineLayout PipelineLayout = nullptr;
				VK::VulkanGraphicsPipeline GraphicsPipeline;

				VulkanStaticMeshData SpriteMesh;

				uint32_t MAX_GRASS_COUNT = 200000;
				VK::VulkanShaderStorageBuffer GrassPositionsBuffer;
			};

			inline static _GrassSpriteRenderpass s_GrassSpriteRenderpass;




			struct _Environment
			{
				VK::VulkanHdrCubemap* Cubemap;

				int EnvironmentMapSize = 1024;

				TransformData CubemapTransform;

				VK::VulkanIrradianceCubemap* IrraduianceCubemap;
				VK::VulkanPrefilteredCubemap* PrefilterCubemap;
			};
			inline static _Environment s_Environment;

			struct TerrainRenderpass
			{
				VulkanShader Shader;
				VulkanGraphicsPipeline GraphicsPipeline;
				VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
			};

			struct _Terrain
			{
				VulkanStaticMeshData TerrainMesh;
				TerrainRenderpass TerrainPass;

				VK::VulkanDescriptorSetLayout DescriptorSetLayout;
				VK::VulkanDescriptorSet DescriptorSet;
			};
			inline static _Terrain s_Terrain;

			inline static VulkanPostProcessingRenderpass s_PostProcessingRenderpass;
			inline static VulkanBloomPass s_BloomPass;
			inline static uint32_t s_ImageIndex = 0;

			inline static VulkanBrdfLutTexture* s_BrdfLutTexture;

			inline static glm::vec3 s_CameraPosition;
			inline static glm::mat4 s_CameraViewProjection;
			inline static float s_BloomThreshold = 1.0f;
		};
	}
}
