#pragma once
#include "GLVertexArray.h"

namespace PIX3D
{
	namespace GL
	{
		struct GLStaticMeshData
		{
			GLVertexArray VertexArray;
			GLVertexBuffer VertexBuffer;
			GLIndexBuffer IndexBuffer;
			uint32_t VerticesCount = 0;
			uint32_t IndicesCount = 0;
			bool vaild = false;
		};

		class GLStaticMeshGenerator
		{
		public:
			static GLStaticMeshData GenerateCube();
			static GLStaticMeshData GenerateQuad();
		private:
			inline static GLStaticMeshData CubeMesh;
			inline static GLStaticMeshData QuadMesh;
		};
	}
}
