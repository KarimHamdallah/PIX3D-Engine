#pragma once
#include "Scene.h"

namespace PIX3D
{
	class SceneSerializer
	{
	public:
		static bool SaveScene_Text(Scene* scene,const std::filesystem::path& path);
		static bool LoadScene_Text(Scene* scene,const std::filesystem::path& path);
	};
}
