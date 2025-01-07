#pragma once
#include "GLShader.h"
#include "GLStorageBuffer.h"
#include "GLUniformBuffer.h"
#include "IBL/GLHdrCubemap.h"
#include <Graphics/Camera3D.h>
#include <Platfrom/GL/IBL/IBLCubemapsGenerator.h>
#include <Graphics/Mesh.h>
#include "GLMainRenderpass.h"
#include "GLBloompass.h"
#include "GLPostProcessingpass.h"
#include "GLDirShadowpass.h"
#include <Scene/Scene.h>

namespace PIX3D
{
	namespace GL
	{
		// uniform buffer
		struct CameraUnifromBufferData
		{
			glm::mat4 CameraProj;
			glm::mat4 CameraView;
			glm::mat4 SkyboxView;
		};

		class GLRenderer
		{

		public:
			static void Init(uint32_t width, uint32_t height);
			static void Resize(uint32_t width, uint32_t height);
			static void Destory();

			static void Begin(Scene* scene, Camera3D& cam, const glm::vec4& clear_color = {1.0f, 1.0f, 1.0f, 1.0f});
			static void RenderMesh(Scene* scene, const glm::mat4& model, StaticMesh& mesh, IBLMaps& ibl_maps, int point_lights_count = 0);
			static void RenderHdrSkybox(const glm::mat4& model, const GLHdriCubemap& hdrcubemap);
			static void End();

			inline static GLTexture GetDefaultAlbedoTexture() { return s_DefaultAlbedoTexture; }
			inline static GLTexture GetDefaultNormalTexture() { return s_DefaultNormalTexture; }
			
			inline static GLTexture GetDefaultWhiteTexture() { return s_DefaultWhiteTexture; }
			inline static GLTexture GetDefaultBlackTexture() { return s_DefaultBlackTexture; }

			inline static uint32_t GetMainRenderpassColorAttachment() { return s_MainRenderpass.GetColorAttachment(); }
			inline static uint32_t GetMainRenderpassBloomBrightnessColorAttachment() { return s_MainRenderpass.GetBloomColorAttachment(); }
			inline static uint32_t GetBloompassOutputColorAttachment() { return s_Bloompass.GetOutputTexture(); }
			inline static uint32_t GetPostProcessingpassOutputColorAttachment() { return s_PostProcessingpass.GetColorAttachment(); }

			inline static float GetBloomThreshold() { return s_BloomThreshold; }
			inline static void SetBloomThreshold(float threshold) { s_BloomThreshold = threshold; }

		public:
			inline static glm::vec3 s_CameraPosition;
			inline static glm::mat4 s_CameraView;

			inline static GLShader s_Model3DShader;
			inline static GLShader s_SkyBoxShader;
			inline static PIX3D::GL::GLUniformBuffer s_CameraUnifromBuffer;

			inline static GLTexture s_DefaultAlbedoTexture;
			inline static GLTexture s_DefaultNormalTexture;
			inline static GLTexture s_DefaultWhiteTexture;
			inline static GLTexture s_DefaultBlackTexture;

			// PBR Pipeline
			inline static GLDirShadowpass s_DirShadowpass;
			inline static GLMainRenderpass s_MainRenderpass;
			inline static GLBloompass s_Bloompass;
			inline static GLPostProcessingpass s_PostProcessingpass;
			inline static float s_BloomThreshold = 1.0f;
		};
	}
}
