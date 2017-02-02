#pragma once
#include <string>

class AppState;
class AppConfig;
class DMCampaign;

namespace FS
{
	class FileSystem;
}

class AppController
{
public:
	AppController(FS::FileSystem &fs);
	void Step(AppState &appState, AppConfig &appConfig, float dt);
//    void NewGameDM(TzodApp &app, const std::string &mapName, const DMSettings &settings);
	void StartDMCampaignMap(AppState &appState, AppConfig &appConfig, DMCampaign &dmCampaign, unsigned int tier, unsigned int map);

private:
	FS::FileSystem &_fs;
};
