#include "VulkanVertexInputLayout.h"

namespace PIX3D
{
    namespace VK
    {
        VulkanVertexInputLayout& VulkanVertexInputLayout::AddAttribute(VertexAttributeFormat format)
        {
            m_Attributes.push_back(format);
            m_Stride += GetFormatSize(format);
            return *this;
        }

        VkVertexInputBindingDescription VulkanVertexInputLayout::GetBindingDescription() const
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = m_Stride;
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bindingDescription;
        }

        std::vector<VkVertexInputAttributeDescription> VulkanVertexInputLayout::GetAttributeDescriptions() const
        {
            std::vector<VkVertexInputAttributeDescription> attributes(m_Attributes.size());
            uint32_t offset = 0;

            for (size_t i = 0; i < m_Attributes.size(); i++)
            {
                attributes[i].binding = 0;
                attributes[i].location = i;
                attributes[i].format = GetVulkanFormat(m_Attributes[i]);
                attributes[i].offset = offset;
                offset += GetFormatSize(m_Attributes[i]);
            }

            return attributes;
        }

        uint32_t VulkanVertexInputLayout::GetFormatSize(VertexAttributeFormat format)
        {
            switch (format)
            {
            case VertexAttributeFormat::Float:  return sizeof(float);
            case VertexAttributeFormat::Float2: return sizeof(float) * 2;
            case VertexAttributeFormat::Float3: return sizeof(float) * 3;
            case VertexAttributeFormat::Float4: return sizeof(float) * 4;
            case VertexAttributeFormat::Int:    return sizeof(int);
            case VertexAttributeFormat::Int2:   return sizeof(int) * 2;
            case VertexAttributeFormat::Int3:   return sizeof(int) * 3;
            case VertexAttributeFormat::Int4:   return sizeof(int) * 4;
            default: return 0;
            }
        }

        VkFormat VulkanVertexInputLayout::GetVulkanFormat(VertexAttributeFormat format)
        {
            switch (format)
            {
            case VertexAttributeFormat::Float:  return VK_FORMAT_R32_SFLOAT;
            case VertexAttributeFormat::Float2: return VK_FORMAT_R32G32_SFLOAT;
            case VertexAttributeFormat::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
            case VertexAttributeFormat::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;
            case VertexAttributeFormat::Int:    return VK_FORMAT_R32_SINT;
            case VertexAttributeFormat::Int2:   return VK_FORMAT_R32G32_SINT;
            case VertexAttributeFormat::Int3:   return VK_FORMAT_R32G32B32_SINT;
            case VertexAttributeFormat::Int4:   return VK_FORMAT_R32G32B32A32_SINT;
            default: return VK_FORMAT_UNDEFINED;
            }
        }
    }
}
