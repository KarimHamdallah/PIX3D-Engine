#pragma once
#include "VulkanTexture.h"
#include "VulkanDescriptorSetLayout.h"

namespace PIX3D
{
	namespace VK
	{
		class VulkanSceneRenderer
		{
		public:
			static void Init();
			static void Destroy();

			inline static VulkanTexture* GetDefaultAlbedoTexture() { return s_DefaultAlbedoTexture; }
			inline static VulkanTexture* GetDefaultNormalTexture() { return s_DefaultNormalTexture; }

			inline static VulkanTexture* GetDefaultWhiteTexture() { return s_DefaultWhiteTexture; }
			inline static VulkanTexture* GetDefaultBlackTexture() { return s_DefaultBlackTexture; }

			inline static VkDescriptorSetLayout GetVulkanStaticMeshMaterialDescriptorSetLayout() { return s_VulkanStaticMeshMaterialDescriptorSetLayout.GetVkDescriptorSetLayout(); }

		private:
			inline static VulkanTexture* s_DefaultAlbedoTexture;
			inline static VulkanTexture* s_DefaultNormalTexture;
			inline static VulkanTexture* s_DefaultWhiteTexture;
			inline static VulkanTexture* s_DefaultBlackTexture;

			inline static VulkanDescriptorSetLayout s_VulkanStaticMeshMaterialDescriptorSetLayout;
		};
	}
}
