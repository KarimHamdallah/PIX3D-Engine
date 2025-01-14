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


    const std::vector<float> SpriteVertices =
    {
        // positions         // circle NDC coords    // texcoords
         0.5f,  0.5f, 0.0f,   1.0f,  1.0f,   1.0f,  1.0f,   // top right
         0.5f, -0.5f, 0.0f,   1.0f, -1.0f,   1.0f,  0.0f,   // bottom right
        -0.5f, -0.5f, 0.0f,  -1.0f, -1.0f,   0.0f,  0.0f,   // bottom left
        -0.5f,  0.5f, 0.0f,  -1.0f,  1.0f,   0.0f,  1.0f    // top left
    };

    const std::vector<uint32_t> SpriteIndices =
    {
        0, 1, 3,  // first triangle
        1, 2, 3   // second triangle
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


        VulkanStaticMeshData VulkanStaticMeshGenerator::GenerateSprite()
        {
            if (!SpriteMesh.valid)
            {
                // Setup vertex buffer
                SpriteMesh.VertexBuffer.Create();
                SpriteMesh.VertexBuffer.FillData(SpriteVertices.data(), SpriteVertices.size() * sizeof(float));

                // Setup index buffer
                SpriteMesh.IndexBuffer.Create();
                SpriteMesh.IndexBuffer.FillData(SpriteIndices.data(), SpriteIndices.size() * sizeof(uint32_t));

                // Setup vertex layout
                // position (xyz), circle NDC coords (xy), texcoords (uv)
                SpriteMesh.VertexLayout
                    .AddAttribute(VertexAttributeFormat::Float3)  // position
                    .AddAttribute(VertexAttributeFormat::Float2)  // circle NDC coords
                    .AddAttribute(VertexAttributeFormat::Float2); // texcoords

                SpriteMesh.VerticesCount = 4;
                SpriteMesh.IndicesCount = 6;
                SpriteMesh.valid = true;
            }
            return SpriteMesh;
        }
    }
}
