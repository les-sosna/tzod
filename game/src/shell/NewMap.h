#pragma once
#include <ui/Dialog.h>

class ConfCache;
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
	NewMapDlg(Window *parent, ConfCache &conf);

	void OnOK();
	void OnCancel();
};
