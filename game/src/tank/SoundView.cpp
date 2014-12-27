#include "SoundView.h"
#include "SoundHarness.h"
#include "SoundRender.h"
#include "GameContext.h"

SoundView::SoundView(AppState &appState, FS::FileSystem &fs)
	: AppStateListener(appState)
	, _soundRender(new SoundRender(fs))
{
}

SoundView::~SoundView()
{
}

void SoundView::Step()
{
	_soundRender->Step();
	if (_soundHarness)
		_soundHarness->Step();
	
//	if( _scriptEnvironment.music )
//		_scriptEnvironment.music->HandleBufferFilling();
}

void SoundView::OnGameContextChanging()
{
	_soundHarness.reset();
}

void SoundView::OnGameContextChanged()
{
	if (GameContextBase *gc = GetAppState().GetGameContext())
	{
		_soundHarness.reset(new SoundHarness(*_soundRender, gc->GetWorld()));
	}
}
