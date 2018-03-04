#include "pch.h"
#include "FrameworkView.h"
#include <app/tzod.h>
#include <fs/FileSystem.h>
#include <ui/ConsoleBuffer.h>
#include <utf8.h>

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Storage;

namespace wtzod
{
	ref class FrameworkViewSource sealed : IFrameworkViewSource
	{
	internal:
		FrameworkViewSource(FS::FileSystem &fs, UI::ConsoleBuffer &logger, TzodApp &app)
			: _fs(fs)
			, _logger(logger)
			, _app(app)
		{
		}

	public:
		// IFrameworkViewSource
		virtual IFrameworkView^ CreateView()
		{
			return ref new FrameworkView(_fs, _logger, _app);
		}

	private:
		FS::FileSystem &_fs;
		UI::ConsoleBuffer &_logger;
		TzodApp &_app;
	};
}

static std::string w2s(std::wstring_view w)
{
	std::string s;
	utf8::utf16to8(w.begin(), w.end(), std::back_inserter(s));
	return s;
}

[MTAThread]
int main(Array<String^> ^args)
{
	::SetCurrentDirectoryW(L"StoreData");
	std::shared_ptr<FS::FileSystem> fs = FS::CreateOSFileSystem("data");
	fs->Mount("user", FS::CreateOSFileSystem(w2s(ApplicationData::Current->LocalFolder->Path->Data())));

	UI::ConsoleBuffer logger(100, 500);
	TzodApp app(*fs, logger);

	CoreApplication::Run(ref new wtzod::FrameworkViewSource(*fs, logger, app));
	return 0;
}
