#pragma once
#include <string>
#include <memory>

class AppState;
class AppConfig;
class DMCampaign;
class MapCollection;
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
	void Step(AppState &appState, AppConfig &appConfig, float dt, bool *outConfigChanged);
//	void NewGameDM(TzodApp &app, const std::string &mapName, const DMSettings &settings);
	void StartDMCampaignMap(AppState &appState, MapCollection& mapCollection, AppConfig &appConfig, DMCampaign &dmCampaign, unsigned int tier, unsigned int map);
	void PlayCurrentMap(AppState &appState, MapCollection& mapCollection);
	void StartNewMapEditor(AppState& appState, MapCollection& mapCollection, int width, int height, std::string_view existingMapNameOptional);
	void SaveAndExitEditor(AppState& appState, MapCollection& mapCollection);

private:
	FS::FileSystem &_fs;

	void SaveCurrentMap(EditorContext &editorContext, MapCollection& mapCollection);
};
