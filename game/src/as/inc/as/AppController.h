#pragma once
#include <string>

class AppState;
struct DMSettings;
namespace FS
{
    class FileSystem;
}

class AppController
{
public:
    AppController(FS::FileSystem &fs);
    void NewGameDM(AppState &appState, const std::string &mapName, const DMSettings &settings);

private:
    FS::FileSystem &_fs;
};
