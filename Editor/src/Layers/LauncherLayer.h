#pragma once
#include "Layer.h"

using namespace PIX3D;

struct ProjectInfo
{
	std::string name;
	std::string path;
	std::string thumbnailPath;
};


class LauncherLayer : public Layer
{
public:
	struct RecentProject
	{
		std::string name;
		std::filesystem::path projectPath;
		std::filesystem::path projectFilePath;
	};

public:
	virtual void OnStart() override;
	virtual void OnUpdate(float dt) override;
	virtual void OnDestroy() override;
	virtual void OnKeyPressed(uint32_t key) {}
private:
	void RenderNewProjectDialog();
	bool RenderProjectTile(const char* name, GL::GLTexture& texture, bool isNewProject, float tileSize);
	void RenderRecentProjectTiles(float tileSize, float padding, int& itemCount, int tilesPerRow);
	bool RenderProjectTileDoubleClick(const char* name, GL::GLTexture& texture, float tileSize);
	void ScanForRecentProjects();
private:

	GL::GLTexture m_EngineLogoTexture;
	GL::GLTexture m_AddProjectTexture;
	GL::GLTexture m_LoadProjectTexture;
	GL::GLTexture m_DefaultProjectThumbnail;

	bool m_ShowNewProjectDialog = false;
	char m_ProjectName[256] = "Example Game";
	char m_ProjectPath[256] = "";
	bool m_ProjectLoaded = false;

	std::vector<RecentProject> m_RecentProjects;
	std::filesystem::path m_RecentProjectsPath;
};
