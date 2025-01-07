#pragma once
#include "GLVertexArray.h"
#include "GLShader.h"
#include <glm/glm.hpp>

#define MAX_BATCHED_QUADS 10000
#define VERTICES_PER_QUAD 4

namespace PIX3D
{
	namespace GL
	{

		struct BatchVertex
		{
			glm::vec3 Position;
			glm::vec2 Coords; // For Circle Rendering in ndc inside shader
			glm::vec4 Color;
			float circle_quad; // circle = 0, quad = 1
		};

		class GLPixelBatchRenderer2D
		{
		public:
			static void Init();
			static void Begin();
			static void End();
			static void Destory();
			static void DrawCircle_TopLeft(const glm::vec2& position, float size, const glm::vec4& color);
			static void DrawQuad_TopLeft(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);

			inline static uint32_t GetTotalBatchCount() { return s_TotalBatchCount; }
			inline static uint32_t GetDrawCalls() { return s_DrawCalls; }
			inline static void ResetDrawCalls() { s_DrawCalls = 0; s_TotalBatchCount = 0; }

		private:
			inline static GLVertexBuffer s_BatchVertexBuffer;
			inline static GLIndexBuffer s_BatchIndexBuffer;
			inline static GLVertexArray s_BatchVertexArray;

			inline static std::vector<BatchVertex> s_BatchedVertices;
			
			inline static glm::mat4 s_OrthographicMatrix;
			
			inline static uint32_t s_CurrentVertexIndex = 0;
			inline static uint32_t s_BatchCount = 0;
			inline static uint32_t s_DrawCalls = 0;
			inline static uint32_t s_TotalBatchCount = 0;
			
			inline static GLShader s_PixelBatchRendererShader;
		};
	}
}
