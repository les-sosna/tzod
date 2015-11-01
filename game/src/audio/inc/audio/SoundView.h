#pragma once
#include <app/AppStateListener.h>
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
    std::unique_ptr<SoundRender> _soundRender;
	std::unique_ptr<SoundHarness> _soundHarness;

	// AppStateListener
	void OnGameContextChanging() override;
	void OnGameContextChanged() override;
};
