#pragma once
#include "../Layer.h"
#include "FrostedGlassMaskRenderpass.h"
#include "FrostedGlassPostProcessPass.h"
#include "FrostedGlassBlurPass.h"

using namespace PIX3D;

class FrostedGlassExample : public Layer
{
public:
    virtual void OnStart() override;
    virtual void OnUpdate(float dt) override;
    virtual void OnDestroy() override;
    virtual void OnKeyPressed(uint32_t key) override;


private:
    Scene* m_Scene;
    FrostedGlassMaskRenderpass* m_FrostedGlassMaskRenderpass;
    VulkanStaticMesh* m_CubeMesh;
    TransformComponent m_CubeTransform;

    VK::FrostedGlassPostProcessPass* m_FrostedGlassPostProcessPass;
    VK::FrostedGlassBlurPass* m_FrostedGlassBlurPass;
};
