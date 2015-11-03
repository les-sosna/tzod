#include "inc/gv/GameView.h"
#include "inc/gv/GameViewHarness.h"
#include <app/AppState.h>
#include <app/GameContext.h>

GameView::GameView(AppState &appState)
    : AppStateListener(appState)
    , _pxWidth(0)
    , _pxHeight(0)
{
    OnGameContextChanged();
}

GameView::~GameView()
{
}

void GameView::SetCanvasSize(int pxWidth, int pxHeight)
{
    if (_pxWidth != pxWidth || _pxHeight != pxHeight)
    {
        _pxWidth = pxWidth;
        _pxHeight = pxHeight;
        if (_harness)
        {
            _harness->SetCanvasSize(_pxWidth, _pxHeight);
        }
    }
}

void GameView::Step(float dt)
{
    if (_harness)
    {
        _harness->Step(dt);
    }
}

void GameView::Render(DrawingContext &drawingContext, const WorldView &worldView)
{
    if (_harness)
    {
        _harness->RenderGame(drawingContext, worldView, vec2d(0,0), 1.0f);
    }
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
        if (_pxHeight != 0 && _pxWidth != 0)
        {
            _harness->SetCanvasSize(_pxWidth, _pxHeight);
        }
    }
}
