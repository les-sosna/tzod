#pragma once
#include <ui/Dialog.h>
#include <string>
#include <memory>

class LangCache;
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

class GetFileNameDlg final
	: public UI::Dialog
{
public:
	struct Params
	{
		std::string blank;
		std::string title;
		std::string extension;
		std::shared_ptr<FS::FileSystem> folder;
	};

	GetFileNameDlg(const Params &param, LangCache &lang);
	virtual ~GetFileNameDlg();

	bool IsBlank() const;
	std::string GetFileName() const;
	std::string_view GetFileTitle() const;

protected:
	void OnSelect(int index);
	void OnChangeName();
	bool OnKeyPressed(const Plat::Input &input, const UI::InputContext &ic, Plat::Key key);

	void OnOK();

private:
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ListBox> DefaultListBox;
	std::shared_ptr<DefaultListBox> _files;
	std::shared_ptr<UI::Edit> _fileName;
	std::string _ext;
	std::shared_ptr<FS::FileSystem> _folder;

	bool _changing;
};
