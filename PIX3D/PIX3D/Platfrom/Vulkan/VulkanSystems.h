#pragma once
#include <Utils/entt.hpp>
#include <Scene/Scene.h>
#include "VulkanSceneRenderer.h"
#include <Asset/AssetManager.h>
#include <Core/Core.h>

namespace PIX3D
{
    class StaticMeshRendererSystem
    {
    public:
        static void Render(Scene* scene)
        {
            auto view = scene->m_Registry.view<TagComponent, TransformComponent, StaticMeshComponent>();
            view.each([scene](TagComponent& tag, TransformComponent& transform, StaticMeshComponent& mesh)
                {
                    if (mesh.m_AssetID != 0) // valid asset
                    {
                        VulkanStaticMesh* _StaticMesh = AssetManager::Get().GetStaticMesh(mesh.m_AssetID);
                        PIX_ASSERT(_StaticMesh);
                        if (_StaticMesh)
                        {
                            bool Selected = scene->IsSelectedEntity(tag.m_UUID);
                            if (Selected)
                            {
                                VK::VulkanSceneRenderer::RenderMeshOutline(scene, *_StaticMesh, transform.GetTransformMatrix());
                                VK::VulkanSceneRenderer::RenderMesh(scene, *_StaticMesh, transform.GetTransformMatrix());
                            }
                            else
                                VK::VulkanSceneRenderer::RenderMesh(scene, *_StaticMesh, transform.GetTransformMatrix());
                        }
                    }
                });
        }
    };

    class SpriteRendererSystem
    {
    public:
        static void Render(entt::registry& registry)
        {
            auto view = registry.view<TransformComponent, SpriteComponent>();
            view.each([](TransformComponent& transform, SpriteComponent& sprite)
                {
                    VK::VulkanSceneRenderer::RenderTexturedQuad(sprite.m_Material, transform.GetTransformMatrix());
                });
        }
    };

    class SpriteAnimatorSystem
    {
    public:
        static void Update(entt::registry& registry, float deltaTime)
        {
            auto view = registry.view<SpriteAnimatorComponent>();
            view.each([deltaTime](SpriteAnimatorComponent& animator)
                {
                    if (!animator.m_IsPlaying || !animator.m_Material)
                        return;

                    animator.m_CurrentTime += deltaTime;
                    if (animator.m_CurrentTime >= animator.m_FrameTime)
                    {
                        animator.m_CurrentTime = 0.0f;
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

                        // Update material with new frame offset
                        float frameWidth = 1.0f / animator.m_FrameCount;
                        animator.m_Material->m_Data->uv_offset.x = frameWidth * animator.m_CurrentFrame;
                        animator.m_Material->m_Data->uv_scale.x = frameWidth;
                        animator.m_Material->m_Data->apply_uv_scale_and_offset = 1;
                        animator.m_Material->UpdateBuffer();
                    }
                });
        }

        static void Render(entt::registry& registry)
        {
            auto view = registry.view<TransformComponent, SpriteAnimatorComponent>();
            view.each([](TransformComponent& transform, SpriteAnimatorComponent& animator)
                {
                    if (animator.m_Material)
                    {
                        VK::VulkanSceneRenderer::RenderTexturedQuad(animator.m_Material, transform.GetTransformMatrix());
                    }
                });
        }
    };
}
