#pragma once
#include "AppStateListener.h"
#include <memory>

class GameViewHarness;

class GameView : AppStateListener
{
public:
    GameView(AppState &appState);
    ~GameView();

    GameViewHarness* GetHarness() const { return _harness.get(); }

private:
    std::unique_ptr<GameViewHarness> _harness;
    // AppStateListener
    void OnGameContextChanging() override;
    void OnGameContextChanged() override;
};
