#pragma once
#include <glm/glm.hpp>
#include <Platfrom/GL/GLTexture.h>

namespace PIX3D
{
	struct Sprite
	{
		Sprite();

		GL::GLTexture Texture;
		glm::vec4 Color = { 1.0, 1.0, 1.0, 1.0 };
		float TilingFactor = 1.0f;
	};
}
