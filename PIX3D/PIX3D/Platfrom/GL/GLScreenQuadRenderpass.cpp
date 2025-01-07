#include "GLScreenQuadRenderpass.h"
#include "GLCommands.h"
#include "GLStaticMeshFactory.h"
#include <glad/glad.h>
#include <Engine/Engine.hpp>

namespace PIX3D
{
	namespace GL
	{

        void GLScreenQuadRenderpass::Init()
        {
            s_ScreenQuadShader.LoadFromFile("../PIX3D/res/gl shaders/screen_quad.vert", "../PIX3D/res/gl shaders/screen_quad.frag");
        }

        void GLScreenQuadRenderpass::Destroy()
        {
            s_ScreenQuadShader.Destroy();
        }

        void GLScreenQuadRenderpass::Render(uint32_t texturehandle)
        {
            // bind Texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texturehandle);

            s_ScreenQuadShader.Bind();

            GLStaticMeshData QuadMesh = GLStaticMeshGenerator::GenerateQuad();
            QuadMesh.VertexArray.Bind();
            GLCommands::DrawArrays(Primitive::TRIANGLES, QuadMesh.VerticesCount);
        }
	}
}
