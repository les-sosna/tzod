#pragma once
#include "AppState.h"
#include <memory>

class SoundHarness;
class SoundRender;
namespace FS
{
	class FileSystem;
}

class SoundView : private AppStateListener
{
public:
	SoundView(AppState &appState, FS::FileSystem &fs);
	~SoundView();
	void Step();

private:
	std::unique_ptr<SoundHarness> _soundHarness;
	std::unique_ptr<SoundRender> _soundRender;

	// AppStateListener
	virtual void OnGameContextChanging() override;
	virtual void OnGameContextChanged() override;
};
