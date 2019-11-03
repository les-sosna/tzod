#include "pch.h"
#include "FrameworkView.h"
#include <app/tzod.h>
#include <fswin/FileSystemWin32.h>
#include <plat/ConsoleBuffer.h>
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
		FrameworkViewSource(FS::FileSystem &fs, Plat::ConsoleBuffer &logger, TzodApp &app)
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
		Plat::ConsoleBuffer &_logger;
		TzodApp &_app;
	};
}

[MTAThread]
int main(Array<String^> ^args)
{
	::SetCurrentDirectoryW(L"StoreData");
	auto fs = std::make_shared<FS::FileSystemWin32>(L"data");
	fs->Mount("user", std::make_shared<FS::FileSystemWin32>(ApplicationData::Current->LocalFolder->Path->Data()));

	Plat::ConsoleBuffer logger(100, 500);
	TzodApp app(*fs, logger);

	CoreApplication::Run(ref new wtzod::FrameworkViewSource(*fs, logger, app));
	return 0;
}
