#pragma once
#include <string>
#include <memory>

class AppState;
class AppConfig;
class DMCampaign;
class WorldCache;

namespace FS
{
	class FileSystem;
}

class AppController
{
public:
	AppController(FS::FileSystem &fs);
	~AppController();
	void Step(AppState &appState, AppConfig &appConfig, float dt);
//	void NewGameDM(TzodApp &app, const std::string &mapName, const DMSettings &settings);
	void StartDMCampaignMap(AppState &appState, AppConfig &appConfig, DMCampaign &dmCampaign, unsigned int tier, unsigned int map);

	WorldCache& GetWorldCache() { return *_worldCache; }

private:
	FS::FileSystem &_fs;
	std::unique_ptr<WorldCache> _worldCache;
};
