#include "GLRenderer.h"
#include "GLCommands.h"
#include "GLStaticMeshFactory.h"
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <imgui.h>

#include <Engine/Engine.hpp>


namespace PIX3D
{
	namespace GL
	{
		void GLRenderer::Init(uint32_t width, uint32_t height)
		{
			// Default Albedo Texture
			{
				// RGBA8 Texture
				std::vector<uint8_t> data =
				{
					255, 0, 255, 255,
					255, 0, 255, 255,
					255, 0, 255, 255,
					255, 0, 255, 255
				};

				s_DefaultAlbedoTexture.LoadFromData(data.data(), 2, 2);
			}

			// Default Normal Texture
			{
				// RGBA8 Texture
				std::vector<uint8_t> data =
				{
					127, 127, 255, 255,
					127, 127, 255, 255,
					127, 127, 255, 255,
					127, 127, 255, 255
				};

				s_DefaultNormalTexture.LoadFromData(data.data(), 2, 2);
			}

			// Default White Texture
			{
				// RGBA8 Texture
				std::vector<uint8_t> data =
				{
					255, 255, 255, 255,
					255, 255, 255, 255,
					255, 255, 255, 255,
					255, 255, 255, 255
				};

				s_DefaultWhiteTexture.LoadFromData(data.data(), 2, 2);
			}

			// Default Black Texture
			{
				// RGBA8 Texture
				std::vector<uint8_t> data =
				{
					0, 0, 0, 255,
					0, 0, 0, 255,
					0, 0, 0, 255,
					0, 0, 0, 255
				};

				s_DefaultBlackTexture.LoadFromData(data.data(), 2, 2);
			}

			s_Model3DShader.LoadFromFile("../PIX3D/res/gl shaders/model3d.vert", "../PIX3D/res/gl shaders/model3d.frag");
			s_SkyBoxShader.LoadFromFile("../PIX3D/res/gl shaders/skybox.vert", "../PIX3D/res/gl shaders/skybox.frag");
			s_CameraUnifromBuffer.Create(0);

			s_DirShadowpass.Init(4096);
			s_MainRenderpass.Init(width, height);
			s_Bloompass.Init(width, height);
			s_PostProcessingpass.Init(width, height);
		}

		void GLRenderer::Resize(uint32_t width, uint32_t height)
		{
			s_MainRenderpass.Resize(width, height);
			s_Bloompass.Resize(width, height);
			s_PostProcessingpass.Resize(width, height);
		}

		void GLRenderer::Destory()
		{
			s_Model3DShader.Destroy();
			s_CameraUnifromBuffer.Destroy();
			s_MainRenderpass.Destroy();
			s_Bloompass.Destroy();
		}

		void GLRenderer::Begin(Scene* scene, Camera3D& cam, const glm::vec4& clear_color)
		{
			s_CameraView = cam.GetViewMatrix();

			// shadowpass
			s_DirShadowpass.BeginRender(cam, scene->m_DirLight.m_Direction);
			{
				// Query all entities that have both TransformComponent and StaticMeshComponent
				auto view = scene->m_Registry.view<TransformComponent, StaticMeshComponent>();

				// Iterate over all matching entities
				view.each([&](TransformComponent& transform, StaticMeshComponent& mesh)
					{
						s_DirShadowpass.m_SimpleDepthShader.Bind();
						s_DirShadowpass.m_SimpleDepthShader.SetMat4("model", transform.GetTransformMatrix());

						// draw meshes
						for (size_t i = 0; i < mesh.m_Mesh.m_SubMeshes.size(); i++)
						{
							auto submesh = mesh.m_Mesh.m_SubMeshes[i];

							mesh.m_Mesh.GetVertexArray().Bind();
							PIX3D::GL::GLCommands::DrawIndexedBaseVertex(PIX3D::GL::Primitive::TRIANGLES, submesh.IndicesCount, submesh.BaseIndex, submesh.BaseVertex);
						}
					});
			}
			s_DirShadowpass.EndRender();

			// save camera position to send later to shader
			s_CameraPosition = cam.GetPosition();

			// fill camera uniformbuffer
			// remove translation so skybox is always surrounding camera
			glm::mat4 SkyboxView = glm::mat4(glm::mat3(cam.GetViewMatrix()));

			CameraUnifromBufferData data;
			data.CameraProj = cam.GetProjectionMatrix();
			data.CameraView = cam.GetViewMatrix();
			data.SkyboxView = SkyboxView;

			s_CameraUnifromBuffer.Update({ &data, sizeof(data) });
			
			// bind main renderpass framebuffer
			s_MainRenderpass.BindFrameBuffer(clear_color);
		}

		void GLRenderer::RenderMesh(Scene* scene, const glm::mat4& model, StaticMesh& mesh, IBLMaps& ibl_maps, int point_lights_count)
		{
            s_Model3DShader.Bind();

			mesh.BindMaterialBuffer(2);

			s_Model3DShader.SetMat4("model", model);

			s_Model3DShader.SetVec3("u_CameraPosition", s_CameraPosition);
			s_Model3DShader.SetFloat("u_BloomBrightnessCutoff", s_BloomThreshold);
			s_Model3DShader.SetInt("u_PointLightsCount", point_lights_count);

			s_Model3DShader.SetVec3("u_DirLightDirection", scene->m_DirLight.m_Direction);
			s_Model3DShader.SetVec4("u_DirLightColor", scene->m_DirLight.m_Color);
			s_Model3DShader.SetFloat("u_DirLightIntensity", scene->m_DirLight.m_Intensity);
			s_Model3DShader.SetBool("u_HasDirLight", scene->m_HasDirectionalLight);

			s_Model3DShader.SetMat4("u_View", s_CameraView);
			for (size_t i = 0; i < s_DirShadowpass.m_ShadowCascadeLevels.size(); ++i)
			{
				s_Model3DShader.SetFloat("u_CascadePlaneDistances[" + std::to_string(i) + "]", s_DirShadowpass.m_ShadowCascadeLevels[i]);
			}

			// IBL stuff
			glActiveTexture(GL_TEXTURE0 + 0);
			s_Model3DShader.SetInt("u_DiffuseIrradianceMap", 0);
			ibl_maps.IrradianceMap.Bind();

			glActiveTexture(GL_TEXTURE0 + 1);
			s_Model3DShader.SetInt("u_PrefilteredEnvMap", 1);
			ibl_maps.PrefilterMap.Bind();

			glActiveTexture(GL_TEXTURE0 + 2);
			s_Model3DShader.SetInt("u_BrdfConvolutionMap", 2);
			glBindTexture(GL_TEXTURE_2D, IBLCubemapsGenerator::GetBrdfLUTTextureHandle());

			// draw meshes
			auto submeshes = mesh.GetSubMeshes();
            for (size_t i = 0; i < submeshes.size(); i++)
            {
				auto mat = mesh.GetMaterials()[i];


				s_Model3DShader.SetInt("AlbedoTexture", 3);
				mat.AlbedoTexture.Bind(3);

				s_Model3DShader.SetInt("NormalTexture", 4);
				mat.NormalTexture.Bind(4);

				s_Model3DShader.SetInt("MetalRoughnessTexture", 5);
				mat.MetalRoughnessTexture.Bind(5);


				s_Model3DShader.SetInt("AoTexture", 6);
				mat.AoTexture.Bind(6);


				s_Model3DShader.SetInt("EmissiveTexture", 7);
				mat.EmissiveTexture.Bind(7);

				s_Model3DShader.SetInt("u_DirShadowMap", 8);
				glActiveTexture(GL_TEXTURE0 + 8);
				glBindTexture(GL_TEXTURE_2D_ARRAY, s_DirShadowpass.m_LightDepthMaps);

				auto submesh = submeshes[i];
			
				mesh.GetVertexArray().Bind();
				s_Model3DShader.SetFloat("u_MeshIndex", (float)(i));
				PIX3D::GL::GLCommands::DrawIndexedBaseVertex(PIX3D::GL::Primitive::TRIANGLES, submesh.IndicesCount, submesh.BaseIndex, submesh.BaseVertex);
            }
		}

		void GLRenderer::RenderHdrSkybox(const glm::mat4& model, const GLHdriCubemap& hdrcubemap)
		{
			GLStaticMeshData CubeMesh = GLStaticMeshGenerator::GenerateCube();

			// default depth buffer value is 1.0
			// skybox depth is 1.0 everywhere
			// need equality to make sure skybox passes depth test in default value places
			glDepthFunc(GL_LEQUAL);

			s_SkyBoxShader.Bind();
			s_SkyBoxShader.SetMat4("model", model);
			s_SkyBoxShader.SetFloat("u_BloomBrightnessCutoff", s_BloomThreshold);

			glActiveTexture(GL_TEXTURE0);
			s_SkyBoxShader.SetInt("u_CubemapTexture", 0);
			hdrcubemap.Bind();

			// draw mesh
			CubeMesh.VertexArray.Bind();
			GLCommands::DrawArrays(Primitive::TRIANGLES, CubeMesh.VerticesCount);

			glDepthFunc(GL_LESS); // go back to default depth comparison

		}

		void GLRenderer::End()
		{
			s_Bloompass.Render(s_MainRenderpass.GetBloomColorAttachment());
			s_PostProcessingpass.Render(s_MainRenderpass.GetColorAttachment(), s_Bloompass.GetOutputTexture());
		}
	}
}
