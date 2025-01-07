#include "Sprite.h"
#include <Platfrom/GL/GLRenderer.h>

PIX3D::Sprite::Sprite()
{
	Texture = GL::GLRenderer::GetDefaultWhiteTexture();
}
