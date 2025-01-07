#pragma once
#include "GLShader.h"
#include "GLStorageBuffer.h"
#include <Graphics/Camera3D.h>

namespace PIX3D
{
	namespace GL
	{
		class GLDirShadowpass
		{
		public:
			GLDirShadowpass() = default;
			~GLDirShadowpass();

			void Init(uint32_t res = 1024);

			void BeginRender(Camera3D& cam, const glm::vec3& lightdir);
			void EndRender();

		private:
			std::vector<glm::mat4> GetLightSpaceMatrices(Camera3D& cam, const glm::vec3& lightdir);
			std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& projview);
			std::vector<glm::vec4> GetFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view);
			glm::mat4 GetLightSpaceMatrix(const float nearPlane, const float farPlane, Camera3D& cam, const glm::vec3& lightdir);

		public:
			GLShader m_SimpleDepthShader;
			uint32_t m_Framebuffer = 0; // Shaodwpass Framebuffer Handle
			uint32_t m_LightDepthMaps = 0; // Texture Array Handle
			std::vector<float> m_ShadowCascadeLevels;
			uint32_t m_LightSpaceMatricesUniformbuffer = 0;
			GLStorageBuffer m_LightSpaceMatricesBuffer;
			uint32_t m_DepthResolution;
		};
	}
}
