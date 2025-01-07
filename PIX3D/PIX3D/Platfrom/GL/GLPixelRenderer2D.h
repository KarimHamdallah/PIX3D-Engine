#pragma once
#include "GLVertexArray.h"
#include "GLTexture.h"
#include "GLShader.h"
#include <Graphics/Camera3D.h>
#include <Graphics/Camera2D.h>

namespace PIX3D
{
	namespace GL
	{
		class GLPixelRenderer2D
		{
		public:
			static void Init();
			static void Begin(const Camera2D& cam);
			static void Begin(const Camera3D& cam);
			static void End();
			static void Destory();

			static void DrawSmoothRoundedQuad(float x, float y, float size_x, float size_y, glm::vec4 color, float roundness = 0.5f, float smoothness = 0.01f);
			static void DrawSmoothQuad(float x, float y, float size_x, float size_y, glm::vec4 color, float smoothness = 0.01f);
			static void DrawSmoothCircle(float x, float y, float size, glm::vec4 color, float smoothness = 0.01f);
			static void DrawTexturedQuad(GLTexture texture, const glm::vec3& position, const glm::vec3& scale, glm::vec4 color, float tiling_factor = 1.0f);
			
			static void DrawSmoothQuad(const glm::mat4& transformation, glm::vec4 color, float smoothness = 0.01f);
			static void DrawTexturedQuad(GLTexture texture, const glm::mat4& transformation, glm::vec4 color, float tiling_factor = 1.0f, bool flip = false);
			static void DrawTexturedQuadUV(GLTexture texture,const glm::mat4& transformation,const glm::vec2& uv_offset,const glm::vec2& uv_scale, glm::vec4 color, float tiling_factor = 1.0f, bool flip = false);

			inline static GLTexture GetPointLightGizmoIcon() { return s_PointLightGizmoIcon; }
		private:

			// Smooth Circle
			inline static GLVertexArray s_QuadVertexArray;
			inline static GLVertexBuffer s_QuadVertexBuffer;
			inline static GLIndexBuffer s_QuadIndexBuffer;
			
			inline static GLShader s_QuadShader;

			inline static glm::mat4 s_ProjectionMatrix;

			// Gizmos
			inline static GLTexture s_PointLightGizmoIcon;
		};
	}
}
