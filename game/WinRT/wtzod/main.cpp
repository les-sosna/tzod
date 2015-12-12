#include "pch.h"
#include "FrameworkView.h"
#include <app/tzod.h>
#include <fs/FileSystem.h>
#include <ui/ConsoleBuffer.h>

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;

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

[MTAThread]
int main(Array<String^> ^args)
{
	std::shared_ptr<FS::FileSystem> fs = FS::CreateOSFileSystem("StoreData/data");
	UI::ConsoleBuffer logger(100, 500);
	TzodApp app(*fs, logger);

	CoreApplication::Run(ref new wtzod::FrameworkViewSource(*fs, logger, app));
	return 0;
}
