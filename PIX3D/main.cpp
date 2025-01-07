/*
* Copyright (c) 2024 Karim Hamdallah
*/

#include <PIX3D.h>


int main()
{
	std::cout << "Hello, World!\n";

	// Init Engine
	{
		PIX3D::EngineSpecs EngineSpecs;
		EngineSpecs.API = PIX3D::GraphicsAPI::OPENGL;

		PIX3D::Engine::Init(EngineSpecs);
	}

	// Create Application
	{
		PIX3D::ApplicationSpecs AppSpecs;
		AppSpecs.Width = 800;
		AppSpecs.Height = 600;
		AppSpecs.Title = "Drawing Application";

		// Run Application
		PIX3D::Engine::CreateApplication<PIX3D::Application>(AppSpecs);
	}


	// Destroy Engine
	PIX3D::Engine::Destroy();

	return 0;
}
