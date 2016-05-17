#pragma once
#include <ui/Dialog.h>

class ConfCache;
class LangCache;
class TextureManager;
namespace UI
{
	class Edit;
}

class NewMapDlg : public UI::Dialog
{
	ConfCache &_conf;
	std::shared_ptr<UI::Edit> _width;
	std::shared_ptr<UI::Edit> _height;

public:
	NewMapDlg(UI::LayoutManager &manager, TextureManager &texman, ConfCache &conf, LangCache &lang);

	void OnOK();
	void OnCancel();
};
