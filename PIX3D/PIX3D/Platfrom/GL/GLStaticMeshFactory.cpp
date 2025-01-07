#include "GLStaticMeshFactory.h"

namespace
{
    std::vector<float> CubeVertices =
    {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    std::vector<float> QuadVertices = 
    {
        // positions   // textureCoordinates
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
}

namespace PIX3D
{
    namespace GL
    {
        GLStaticMeshData PIX3D::GL::GLStaticMeshGenerator::GenerateCube()
        {
            if (!CubeMesh.vaild)
            {
                CubeMesh.VertexBuffer.Create();
                CubeMesh.VertexBuffer.Fill({ CubeVertices.data(), (uint32_t)(CubeVertices.size() * sizeof(float)) });

                CubeMesh.VertexArray.Create();
                CubeMesh.VertexArray.AddVertexBuffer(CubeMesh.VertexBuffer, 3 * sizeof(float));

                {
                    GL::GLVertexAttrib Position;
                    Position.ShaderLocation = 0;
                    Position.Type = GL::GLShaderAttribType::FLOAT;
                    Position.Count = 3;
                    Position.Offset = 0;

                    CubeMesh.VertexArray.AddVertexAttrib(Position);
                }
                CubeMesh.VerticesCount = CubeVertices.size() / 3;
                CubeMesh.IndicesCount = 0;
                CubeMesh.vaild = true;
            }
            
            return CubeMesh;
        }

        GLStaticMeshData PIX3D::GL::GLStaticMeshGenerator::GenerateQuad()
        {
            if (!QuadMesh.vaild)
            {
                QuadMesh.VertexBuffer.Create();
                QuadMesh.VertexBuffer.Fill({ QuadVertices.data(), (uint32_t)(QuadVertices.size() * sizeof(float)) });

                QuadMesh.VertexArray.Create();
                QuadMesh.VertexArray.AddVertexBuffer(QuadMesh.VertexBuffer, 4 * sizeof(float));

                {
                    GL::GLVertexAttrib Position;
                    Position.ShaderLocation = 0;
                    Position.Type = GL::GLShaderAttribType::FLOAT;
                    Position.Count = 2;
                    Position.Offset = 0;

                    GL::GLVertexAttrib TexCoords;
                    TexCoords.ShaderLocation = 1;
                    TexCoords.Type = GL::GLShaderAttribType::FLOAT;
                    TexCoords.Count = 2;
                    TexCoords.Offset = 2 * sizeof(float);

                    QuadMesh.VertexArray.AddVertexAttrib(Position);
                    QuadMesh.VertexArray.AddVertexAttrib(TexCoords);
                }

                QuadMesh.VerticesCount = QuadVertices.size() / 4;
                QuadMesh.IndicesCount = 0;
                QuadMesh.vaild = true;
            }

            return QuadMesh;
        }
    }
}
