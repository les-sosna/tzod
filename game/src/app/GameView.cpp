#include "inc/app/AppState.h"
#include "inc/app/GameView.h"
#include "inc/app/GameViewHarness.h"
#include "inc/app/GameContext.h"

GameView::GameView(AppState &appState)
    : AppStateListener(appState)
{
    OnGameContextChanged();
}

GameView::~GameView()
{
}

void GameView::OnGameContextChanging()
{
    _harness.reset();
}

void GameView::OnGameContextChanged()
{
    if (GameContextBase *gc = GetAppState().GetGameContext())
    {
        _harness.reset(new GameViewHarness(gc->GetWorld()));
    }
}
