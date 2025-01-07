#pragma once
#include <PIX3D.h>

using namespace PIX3D;

class LightningWidget
{
public:
	LightningWidget(Scene* scene)
		: m_Scene(scene)
	{}
	~LightningWidget() {}

	void OnRender();

public:
	Scene* m_Scene;
};
