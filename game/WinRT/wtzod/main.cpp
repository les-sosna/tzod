#include "pch.h"
#include "FrameworkView.h"

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;

namespace wtzod
{
	ref class AppSource sealed : IFrameworkViewSource
	{
	public:
		// IFrameworkViewSource
		virtual IFrameworkView^ CreateView()
		{
			return ref new FrameworkView();
		}
	};
}

[MTAThread]
int main(Array<String^> ^args)
{
	CoreApplication::Run(ref new wtzod::AppSource());
	return 0;
}
