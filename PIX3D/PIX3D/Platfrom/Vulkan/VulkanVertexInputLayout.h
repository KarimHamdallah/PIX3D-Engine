#pragma once
#include <vector>
#include <vulkan/vulkan.h>

namespace PIX3D
{
    namespace VK
    {
        enum class VertexAttributeFormat
        {
            Float,
            Float2,
            Float3,
            Float4,
            Int,
            Int2,
            Int3,
            Int4
        };

        class VulkanVertexInputLayout
        {
        public:
            VulkanVertexInputLayout& AddAttribute(VertexAttributeFormat format);

            VkVertexInputBindingDescription GetBindingDescription() const;
            std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions() const;

            static uint32_t GetFormatSize(VertexAttributeFormat format);
            static VkFormat GetVulkanFormat(VertexAttributeFormat format);

        private:
            std::vector<VertexAttributeFormat> m_Attributes;
            uint32_t m_Stride = 0;
        };
    }
}
