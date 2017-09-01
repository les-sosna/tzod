#pragma once
#include <as/AppStateListener.h>
#include <memory>

enum class SoundTemplate;
class SoundHarness;
struct SoundRender;

namespace FS
{
	class FileSystem;
}

namespace UI
{
	class ConsoleBuffer;
}

class SoundView : private AppStateListener
{
public:
	SoundView(FS::FileSystem &fs, UI::ConsoleBuffer &logger, AppState &appState);
	~SoundView();
	void Step();

private:
	std::unique_ptr<SoundRender> _soundRender;
	std::unique_ptr<SoundHarness> _soundHarness;
	void LoadBuffer(FS::FileSystem &fs, UI::ConsoleBuffer &logger, SoundTemplate st, const char *fileName);

	// AppStateListener
	void OnGameContextChanging() override;
	void OnGameContextChanged() override;
};
