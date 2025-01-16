#pragma once

#include <PIX3D.h>
#include "Layers/LauncherLayer.h"
#include "Layers/EditorLayer.h"

using namespace PIX3D;

class PixEditor : public PIX3D::Application
{
public:

public:
	virtual void OnStart() override;
	virtual void OnUpdate(float dt) override;
	virtual void OnDestroy() override;
	virtual void OnResize(uint32_t width, uint32_t height) override;
	virtual void OnKeyPressed(uint32_t key) override;

private:
	LayerManager m_LayerManager;
};
