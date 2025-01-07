#include "GLPipeline.h"
#include <glad/glad.h>

namespace PIX3D
{
	namespace GL
	{
		void GLPipeline::EnableMultiSample()
		{
			glEnable(GL_MULTISAMPLE);
		}
	}
}
