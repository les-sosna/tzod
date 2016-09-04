#pragma once
#include <string>

class AppState;
class ConfCache;
class DMCampaign;
struct DMSettings;

namespace FS
{
    class FileSystem;
}

class AppController
{
public:
    AppController(FS::FileSystem &fs);
    void Step(ConfCache &conf, DMCampaign &dmCampaign, AppState &appState, float dt);
    void NewGameDM(AppState &appState, const std::string &mapName, const DMSettings &settings);

private:
    FS::FileSystem &_fs;
};
