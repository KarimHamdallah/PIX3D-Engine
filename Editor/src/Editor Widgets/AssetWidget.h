#pragma once
#include "Core/UUID.h"
#include <string>
#include <filesystem>
#include <Platfrom/Vulkan/VulkanTexture.h>

namespace PIX3D
{
    class AssetWidget
    {
    public:
        AssetWidget();
        ~AssetWidget();

        void OnRender();
        void PostFrameProcesses();

    private:
        void RenderAssetGrid();
        void RenderAssetContextMenu();
        void HandleDragDrop();

        bool m_ShowImportDialog = false;
        std::string m_SelectedPath;
        UUID m_SelectedAsset;


        VK::VulkanTexture* m_DefaultStaticMeshPreview = nullptr;
    };
}
