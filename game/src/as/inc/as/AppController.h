#pragma once
#include <string>
#include <memory>

class AppState;
class AppConfig;
class DMCampaign;
class WorldCache;
class EditorContext;

namespace FS
{
	class FileSystem;
}

class AppController final
{
public:
	AppController(FS::FileSystem &fs);
	~AppController();
	void Step(AppState &appState, AppConfig &appConfig, float dt, bool &outConfigChanged);
//	void NewGameDM(TzodApp &app, const std::string &mapName, const DMSettings &settings);
	void StartDMCampaignMap(AppState &appState, AppConfig &appConfig, DMCampaign &dmCampaign, unsigned int tier, unsigned int map);
	void SetEditorMode(AppState &appState, bool editorMode);
	bool GetEditorModeAvailable() const { return !!_savedEditorContext; }
	void StartNewMapEditor();

	WorldCache& GetWorldCache() { return *_worldCache; }

private:
	FS::FileSystem &_fs;
	std::unique_ptr<WorldCache> _worldCache;
	std::shared_ptr<EditorContext> _savedEditorContext;
};
