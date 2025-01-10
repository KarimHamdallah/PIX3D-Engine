#include "VulkanStaticMeshFactory.h"
#include <vector>

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

    const std::vector<float> QuadVertices =
    {
        // positions          // tex coords
        -1.0f, -1.0f,  0.0f, 1.0f,  // bottom left
         1.0f, -1.0f,  1.0f, 1.0f,  // bottom right
         1.0f,  1.0f,  1.0f, 0.0f,  // top right
        -1.0f,  1.0f,  0.0f, 0.0f   // top left
    };

    const std::vector<uint32_t> QuadIndices =
    {
        0, 1, 2,  // first triangle
        2, 3, 0   // second triangle
    };
}

namespace PIX3D
{
    namespace VK
    {
        VulkanStaticMeshData VulkanStaticMeshGenerator::GenerateCube()
        {
            if (!CubeMesh.valid)
            {
                // Setup vertex buffer
                CubeMesh.VertexBuffer.Create();
                CubeMesh.VertexBuffer.FillData(CubeVertices.data(), CubeVertices.size() * sizeof(float));

                // Setup vertex layout
                CubeMesh.VertexLayout
                    .AddAttribute(VertexAttributeFormat::Float3); // position only

                CubeMesh.VerticesCount = static_cast<uint32_t>(CubeVertices.size() / 3);
                CubeMesh.IndicesCount = 0;
                CubeMesh.valid = true;
            }
            return CubeMesh;
        }

        VulkanStaticMeshData VulkanStaticMeshGenerator::GenerateQuad()
        {
            if (!QuadMesh.valid)
            {
                // Setup vertex buffer
                QuadMesh.VertexBuffer.Create();
                QuadMesh.VertexBuffer.FillData(QuadVertices.data(), QuadVertices.size() * sizeof(float));

                // Setup index buffer
                QuadMesh.IndexBuffer.Create();
                QuadMesh.IndexBuffer.FillData(QuadIndices.data(), QuadIndices.size() * sizeof(uint32_t));

                // Setup vertex layout for position (xy) and texture coordinates (uv)
                QuadMesh.VertexLayout
                    .AddAttribute(VertexAttributeFormat::Float2)  // position (xy)
                    .AddAttribute(VertexAttributeFormat::Float2); // texcoords (uv)

                QuadMesh.VerticesCount = 4;
                QuadMesh.IndicesCount = 6;
                QuadMesh.valid = true;
            }
            return QuadMesh;
        }
    }
}
