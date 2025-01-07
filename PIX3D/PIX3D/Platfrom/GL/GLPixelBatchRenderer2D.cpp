#include "GLPixelBatchRenderer2D.h"
#include <Engine/Engine.hpp>
#include "GLCommands.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <Engine/Engine.hpp>

namespace
{
	std::vector<glm::vec4> vertices =
	{
		// positions
		glm::vec4( 1.0f,  1.0f, 0.0f, 1.0f),  // top right
		glm::vec4( 1.0f, -1.0f, 0.0f, 1.0f),  // bottom right
		glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f),  // bottom left
		glm::vec4(-1.0f,  1.0f, 0.0f, 1.0f)   // top left 
	};

	std::vector<glm::vec2> coords =
	{
		// positions
		glm::vec2( 1.0f,  1.0f),  // top right
		glm::vec2( 1.0f, -1.0f),  // bottom right
		glm::vec2(-1.0f, -1.0f),  // bottom left
		glm::vec2(-1.0f, 1.0f )   // top left 
	};
}

namespace PIX3D
{
	namespace GL
	{
		void GLPixelBatchRenderer2D::Init()
		{
			// VBO

			s_BatchedVertices.resize(MAX_BATCHED_QUADS * VERTICES_PER_QUAD);

			s_BatchVertexBuffer.Create();
			s_BatchVertexBuffer.Fill(BufferData::CreatFrom<BatchVertex>(s_BatchedVertices), GLVertexBufferUsage::DYNAMIC_DRAW);

			std::vector<uint32_t> indices(MAX_BATCHED_QUADS * 6);

			uint32_t offset = 0;
			for (uint32_t i = 0; i < MAX_BATCHED_QUADS; ++i)
			{
				indices[i * 6 + 0] = offset + 0; // Top-right
				indices[i * 6 + 1] = offset + 1; // Bottom-right
				indices[i * 6 + 2] = offset + 3; // Top-left
				indices[i * 6 + 3] = offset + 1; // Bottom-right
				indices[i * 6 + 4] = offset + 2; // Bottom-left
				indices[i * 6 + 5] = offset + 3; // Top-left

				offset += 4; // Move to the next quad's vertices
			}

			s_BatchIndexBuffer.Create();
			s_BatchIndexBuffer.Fill(indices);

			s_BatchVertexArray.Create();
			{
				GLVertexAttrib position;
				position.ShaderLocation = 0;
				position.Type = GLShaderAttribType::FLOAT;
				position.Count = 3;
				position.Offset = 0;


				GLVertexAttrib coords;
				coords.ShaderLocation = 1;
				coords.Type = GLShaderAttribType::FLOAT;
				coords.Count = 2;
				coords.Offset = 3 * sizeof(float);

				GLVertexAttrib color;
				color.ShaderLocation = 2;
				color.Type = GLShaderAttribType::FLOAT;
				color.Count = 4;
				color.Offset = 3 * sizeof(float) + 2 * sizeof(float);


				GLVertexAttrib circle_quad;
				circle_quad.ShaderLocation = 3;
				circle_quad.Type = GLShaderAttribType::FLOAT;
				circle_quad.Count = 1;
				circle_quad.Offset = 3 * sizeof(float) + 2 * sizeof(float) + 4 * sizeof(float);

				s_BatchVertexArray.AddVertexAttrib(position);
				s_BatchVertexArray.AddVertexAttrib(coords);
				s_BatchVertexArray.AddVertexAttrib(color);
				s_BatchVertexArray.AddVertexAttrib(circle_quad);
			}
			s_BatchVertexArray.AddVertexBuffer(s_BatchVertexBuffer, sizeof(BatchVertex));
			s_BatchVertexArray.AddIndexBuffer(s_BatchIndexBuffer);

			s_PixelBatchRendererShader.LoadFromFile("../PIX3D/res/gl shaders/pixel_batch_renderer.vert", "../PIX3D/res/gl shaders/pixel_batch_renderer.frag");
		}

		void GLPixelBatchRenderer2D::Begin()
		{
			auto specs = PIX3D::Engine::GetApplicationSpecs();
			s_OrthographicMatrix = glm::ortho(
				0.0f,                   // Left
				static_cast<float>(specs.Width),  // Right
				static_cast<float>(specs.Height), // Bottom (flipped)
				0.0f,                   // Top
				-1.0f, 1.0f             // Near and far planes
			);

			s_CurrentVertexIndex = 0;
			s_BatchCount = 0;
		}

		void GLPixelBatchRenderer2D::End()
		{
			// update vertex buffer data
			s_BatchVertexBuffer.Update(BufferData::CreatFrom<BatchVertex>(s_BatchedVertices));

			// bind shader
			s_PixelBatchRendererShader.Bind();

			// draw
			s_BatchVertexArray.Bind();
			GLCommands::DrawIndexed(Primitive::TRIANGLES, s_BatchCount * 6);
			
			s_DrawCalls++;
			s_TotalBatchCount += s_BatchCount;
		}

		void GLPixelBatchRenderer2D::Destory()
		{
			s_BatchVertexBuffer.Destroy();
			s_BatchIndexBuffer.Destroy();
			s_BatchVertexArray.Destroy();
			s_PixelBatchRendererShader.Destroy();
		}

		void GLPixelBatchRenderer2D::DrawQuad_TopLeft(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
		{
			if (s_BatchCount + 1 > MAX_BATCHED_QUADS)
			{
				End();
				Begin();
			}

			glm::mat4 proj =
				s_OrthographicMatrix *
				glm::translate(glm::mat4(1.0f), glm::vec3(position.x, position.y, 0.0f)) *
				glm::scale(glm::mat4(1.0f), glm::vec3(size.x, size.y, 1.0f));

			BatchVertex top_right;
			top_right.Position = proj * vertices[0];
			top_right.Coords = coords[0];
			top_right.Color = color;
			top_right.circle_quad = 1;

			BatchVertex bottom_right;
			bottom_right.Position = proj * vertices[1];
			bottom_right.Coords = coords[1];
			bottom_right.Color = color;
			bottom_right.circle_quad = 1;

			BatchVertex bottom_left;
			bottom_left.Position = proj * vertices[2];
			bottom_left.Coords = coords[2];
			bottom_left.Color = color;
			bottom_left.circle_quad = 1;

			BatchVertex top_left;
			top_left.Position = proj * vertices[3];
			top_left.Coords = coords[3];
			top_left.Color = color;
			top_left.circle_quad = 1;

			s_BatchedVertices[s_CurrentVertexIndex++] = top_right;
			s_BatchedVertices[s_CurrentVertexIndex++] = bottom_right;
			s_BatchedVertices[s_CurrentVertexIndex++] = bottom_left;
			s_BatchedVertices[s_CurrentVertexIndex++] = top_left;

			s_BatchCount++;
		}

		void GLPixelBatchRenderer2D::DrawCircle_TopLeft(const glm::vec2& position, float size, const glm::vec4& color)
		{
			if (s_BatchCount + 1 > MAX_BATCHED_QUADS)
			{
				End();
				Begin();
			}

			glm::mat4 proj =
				s_OrthographicMatrix *
				glm::translate(glm::mat4(1.0f), glm::vec3(position.x, position.y, 0.0f)) *
				glm::scale(glm::mat4(1.0f), glm::vec3(size, size, 1.0f));

			BatchVertex top_right;
			top_right.Position = proj * vertices[0];
			top_right.Coords = coords[0];
			top_right.Color = color;
			top_right.circle_quad = 0;

			BatchVertex bottom_right;
			bottom_right.Position = proj * vertices[1];
			bottom_right.Coords = coords[1];
			bottom_right.Color = color;
			bottom_right.circle_quad = 0;

			BatchVertex bottom_left;
			bottom_left.Position = proj * vertices[2];
			bottom_left.Coords = coords[2];
			bottom_left.Color = color;
			bottom_left.circle_quad = 0;

			BatchVertex top_left;
			top_left.Position = proj * vertices[3];
			top_left.Coords = coords[3];
			top_left.Color = color;
			top_left.circle_quad = 0;
			
			s_BatchedVertices[s_CurrentVertexIndex++] = top_right;
			s_BatchedVertices[s_CurrentVertexIndex++] = bottom_right;
			s_BatchedVertices[s_CurrentVertexIndex++] = bottom_left;
			s_BatchedVertices[s_CurrentVertexIndex++] = top_left;

			s_BatchCount++;
		}
	}
}
