#pragma once
#include <ui/Dialog.h>
#include <string>
#include <memory>

class LangCache;
class TextureManager;
namespace FS
{
	class FileSystem;
}

namespace UI
{
	class ListDataSourceDefault;
	class ListBox;
	class Edit;
	template<class, class> class ListAdapter;
}

class GetFileNameDlg : public UI::Dialog
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ListBox> DefaultListBox;
	std::shared_ptr<DefaultListBox> _files;
	std::shared_ptr<UI::Edit> _fileName;
	std::string _ext;
	std::shared_ptr<FS::FileSystem> _folder;

	bool _changing;

public:
	struct Params
	{
		std::string blank;
		std::string title;
		std::string extension;
		std::shared_ptr<FS::FileSystem> folder;
	};

	GetFileNameDlg(UI::LayoutManager &manager, TextureManager &texman, const Params &param, LangCache &lang);
	virtual ~GetFileNameDlg();

	bool IsBlank() const;
	std::string GetFileName() const;
	std::string GetFileTitle() const;

protected:
	void OnSelect(int index);
	void OnChangeName();
	bool OnKeyPressed(UI::InputContext &ic, UI::Key key);

	void OnOK();
};
