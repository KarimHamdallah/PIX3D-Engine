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

        private:
            inline static VulkanStaticMeshData CubeMesh;
            inline static VulkanStaticMeshData QuadMesh;
        };
    }
}
