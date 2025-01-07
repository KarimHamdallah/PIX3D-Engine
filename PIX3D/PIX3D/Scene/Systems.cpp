#include "Systems.h"
#include "Components.h"
#include <Platfrom/GL/GLPixelRenderer2D.h>
#include <Platfrom/GL/GLRenderer.h>

namespace PIX3D
{

    void StaticMeshRendererSystem::Render(Scene* scene, entt::registry& registry, GL::IBLMaps& ibl_maps, int point_lights_count)
    {

        // Query all entities that have both TransformComponent and StaticMeshComponent
        auto view = registry.view<TransformComponent, StaticMeshComponent>();

        // Iterate over all matching entities
        view.each([&](TransformComponent& transform, StaticMeshComponent& mesh)
            {
                PIX3D::GL::GLRenderer::RenderMesh(scene, transform.GetTransformMatrix(), mesh.m_Mesh, ibl_maps, point_lights_count);
            });
    }

    void SpriteRendererSystem::Render(entt::registry& registry)
    {
        // Query all entities that have both TransformComponent and SpriteComponent
        auto view = registry.view<TransformComponent, SpriteComponent>();

        // Iterate over all matching entities
        view.each([&](TransformComponent& transform, SpriteComponent& sprite)
            {
                // Assuming you have a RenderSprite method in your renderer
                if(sprite.m_Texture.IsValid())
                    PIX3D::GL::GLPixelRenderer2D::DrawTexturedQuad(sprite.m_Texture, transform, sprite.m_Color, sprite.m_TilingFactor, sprite.flip);
                else
                    PIX3D::GL::GLPixelRenderer2D::DrawSmoothQuad(transform, sprite.m_Color);
            });
    }

    void SpriteAnimatorSystem::Update(entt::registry& registry, float dt)
    {
        auto view = registry.view<TransformComponent, SpriteAnimatorComponent>();

        view.each([dt](TransformComponent& transform, SpriteAnimatorComponent& animator)
            {
                if (!animator.m_IsPlaying)
                    return;

                animator.m_CurrentTime += dt;

                if (animator.m_CurrentTime >= animator.m_FrameTime)
                {
                    animator.m_CurrentTime -= animator.m_FrameTime;
                    animator.m_CurrentFrame++;

                    if (animator.m_CurrentFrame >= animator.m_FrameCount)
                    {
                        if (animator.m_Loop)
                            animator.m_CurrentFrame = 0;
                        else
                        {
                            animator.m_CurrentFrame = animator.m_FrameCount - 1;
                            animator.m_IsPlaying = false;
                        }
                    }
                }
            });
    }

    void SpriteAnimatorSystem::Render(entt::registry& registry)
    {
        auto view = registry.view<TransformComponent, SpriteAnimatorComponent>();

        view.each([](TransformComponent& transform, SpriteAnimatorComponent& animator)
            {
                if (!animator.m_SpriteSheet.IsValid())
                    return;

                // Calculate UV offset for current frame
                float frameWidth = 1.0f / animator.m_FrameCount;
                float uOffset = frameWidth * animator.m_CurrentFrame;

                // Draw the current frame
                PIX3D::GL::GLPixelRenderer2D::DrawTexturedQuadUV(
                    animator.m_SpriteSheet,
                    transform,
                    { uOffset, 0.0f },                    // UV offset
                    { frameWidth, 1.0f },                 // UV scale
                    animator.m_Color,
                    animator.m_TilingFactor, animator.m_Flip
                );
            });
    }
}
