#include "GLGraphicsContext.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Core/Core.h>

namespace PIX3D
{
	namespace GL
	{
		void GLGraphicsContext::Init(void* NativeWindowHandle)
		{
			// Make Context
			glfwMakeContextCurrent((GLFWwindow*)NativeWindowHandle);

			// Init Glad
			PIX_ASSERT_MSG(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to initialize GLAD");

			// Set Viewport
			int w, h;
			glfwGetWindowSize((GLFWwindow*)NativeWindowHandle, &w, &h);
			glViewport(0, 0, w, h);

			glEnable(GL_DEPTH_TEST);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

		void GLGraphicsContext::SwapBuffers(void* NativeWindowHandle)
		{
			glfwSwapBuffers((GLFWwindow*)NativeWindowHandle);
		}

		void GLGraphicsContext::EnableDepthTest(bool depthtest)
		{
			if(depthtest)
				glEnable(GL_DEPTH_TEST);
			else
				glDisable(GL_DEPTH_TEST);
		}
	}
}
