#pragma once
#include <as/AppStateListener.h>
#include <memory>

enum SoundTemplate;
class SoundHarness;
struct SoundRender;

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
	void LoadBuffer(FS::FileSystem &fs, SoundTemplate st, const char *fileName);

	// AppStateListener
	void OnGameContextChanging() override;
	void OnGameContextChanged() override;
};
