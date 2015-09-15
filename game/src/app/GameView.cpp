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
    if (auto gameContext = dynamic_cast<GameContext*>(GetAppState().GetGameContext()))
    {
        _harness.reset(new GameViewHarness(gameContext->GetWorld(), gameContext->GetWorldController()));
    }
}
