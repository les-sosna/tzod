#include "inc/as/AppController.h"
#include "inc/as/AppCfg.h"
#include "inc/as/AppState.h"
#include <ctx/GameContext.h>
#include <fs/FileSystem.h>

AppController::AppController(FS::FileSystem &fs)
    : _fs(fs)
{
}

void AppController::NewGameDM(AppState &appState, const std::string &mapName, const DMSettings &settings)
{
    std::string path = std::string(DIR_MAPS) + '/' + mapName + ".map";
    std::shared_ptr<FS::Stream> stream = _fs.Open(path)->QueryStream();
    std::unique_ptr<GameContext> gc(new GameContext(*stream, settings));
    appState.SetGameContext(std::move(gc));
}
