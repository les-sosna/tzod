#pragma once
#include <ui/Dialog.h>

class ConfCache;
class LangCache;
namespace UI
{
	class Edit;
}

class NewMapDlg : public UI::Dialog
{
	ConfCache &_conf;
	UI::Edit *_width;
	UI::Edit *_height;

public:
	NewMapDlg(Window *parent, ConfCache &conf, LangCache &lang);

	void OnOK();
	void OnCancel();
};
