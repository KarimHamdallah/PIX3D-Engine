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

    // Helper function to generate grid mesh data
    void GenerateGridMeshData(uint32_t width, uint32_t height, float gridSize,
        std::vector<float>& vertices, std::vector<uint32_t>& indices)
    {
        // Calculate actual dimensions
        float halfWidth = (width * gridSize) * 0.5f;
        float halfHeight = (height * gridSize) * 0.5f;

        // Generate vertices
        for (uint32_t z = 0; z <= height; z++)
        {
            for (uint32_t x = 0; x <= width; x++)
            {
                // Position
                float xPos = (x * gridSize) - halfWidth;
                float zPos = (z * gridSize) - halfHeight;
                vertices.push_back(xPos);
                vertices.push_back(0.0f);  // Y position (height) starts at 0
                vertices.push_back(zPos);

                // UV coordinates
                vertices.push_back(static_cast<float>(x) / width);
                vertices.push_back(static_cast<float>(z) / height);

                // Normal (pointing up)
                vertices.push_back(0.0f);
                vertices.push_back(1.0f);
                vertices.push_back(0.0f);
            }
        }

        // Generate indices
        for (uint32_t z = 0; z < height; z++)
        {
            for (uint32_t x = 0; x < width; x++)
            {
                uint32_t topLeft = z * (width + 1) + x;
                uint32_t topRight = topLeft + 1;
                uint32_t bottomLeft = (z + 1) * (width + 1) + x;
                uint32_t bottomRight = bottomLeft + 1;

                // First triangle
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);

                // Second triangle
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }
    }
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

        VulkanStaticMeshData VulkanStaticMeshGenerator::GenerateGrid(uint32_t width, uint32_t height, float gridSize)
        {
            // Create a new mesh data instance for the grid
            VulkanStaticMeshData GridMesh;

            // Generate the grid data
            std::vector<float> vertices;
            std::vector<uint32_t> indices;
            GenerateGridMeshData(width, height, gridSize, vertices, indices);

            // Setup vertex buffer
            GridMesh.VertexBuffer.Create();
            GridMesh.VertexBuffer.FillData(vertices.data(), vertices.size() * sizeof(float));

            // Setup index buffer
            GridMesh.IndexBuffer.Create();
            GridMesh.IndexBuffer.FillData(indices.data(), indices.size() * sizeof(uint32_t));

            // Setup vertex layout
            GridMesh.VertexLayout
                .AddAttribute(VertexAttributeFormat::Float3)  // position (xyz)
                .AddAttribute(VertexAttributeFormat::Float2)  // texcoords (uv)
                .AddAttribute(VertexAttributeFormat::Float3); // normal (xyz)

            GridMesh.VerticesCount = static_cast<uint32_t>(vertices.size() / 8); // 8 floats per vertex (3 pos + 2 uv + 3 normal)
            GridMesh.IndicesCount = static_cast<uint32_t>(indices.size());
            GridMesh.valid = true;

            return GridMesh;
        }
    }
}
