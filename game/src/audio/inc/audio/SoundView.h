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

namespace Plat
{
	class ConsoleBuffer;
}

class SoundView final
	: private AppStateListener
{
public:
	SoundView(FS::FileSystem &fs, Plat::ConsoleBuffer &logger, AppState &appState);
	~SoundView();
	void Step();

private:
	std::unique_ptr<SoundRender> _soundRender;
	std::unique_ptr<SoundHarness> _soundHarness;
	void LoadBuffer(FS::FileSystem &fs, Plat::ConsoleBuffer &logger, SoundTemplate st, const char *fileName);

	// AppStateListener
	void OnGameContextChanging() override;
	void OnGameContextChanged() override;
};
