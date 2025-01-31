#pragma once
#include "VulkanVertexBuffer.h"
#include "VulkanIndexBuffer.h"
#include "VulkanVertexInputLayout.h"

namespace PIX3D
{
    namespace VK
    {
        struct VulkanStaticMeshData
        {
            VulkanVertexBuffer VertexBuffer;
            VulkanIndexBuffer IndexBuffer;
            VulkanVertexInputLayout VertexLayout;
            uint32_t VerticesCount = 0;
            uint32_t IndicesCount = 0;
            bool valid = false;
        };

        class VulkanStaticMeshGenerator
        {
        public:
            static VulkanStaticMeshData GenerateCube();
            static VulkanStaticMeshData GenerateQuad();
            static VulkanStaticMeshData GenerateSprite();
            static VulkanStaticMeshData GenerateGrid(uint32_t width, uint32_t height, float gridSize);

        private:
            inline static VulkanStaticMeshData CubeMesh;
            inline static VulkanStaticMeshData QuadMesh;
            inline static VulkanStaticMeshData SpriteMesh;
        };
    }
}
