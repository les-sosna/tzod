#pragma once
#include <gc/WorldEvents.h>
#include <ui/Dialog.h>
#include <ui/ListBase.h>
#include <string>

class LangCache;
namespace UI
{
	template <class, class> class ListAdapter;
	class ComboBox;
	class Button;
	class List;
	class Text;
}

class ServiceListDataSource
	: public UI::ListDataSource
	, public ObjectListener<World>
{
public:
	// UI::ListDataSource
	void AddListener(UI::ListDataSourceListener *listener) override;
	void RemoveListener(UI::ListDataSourceListener *listener) override;
	int GetItemCount() const override;
	int GetSubItemCount(int index) const override;
	size_t GetItemData(int index) const override;
	const std::string& GetItemText(int index, int sub) const override;
	int FindItem(const std::string &text) const override;

	// ObjectListener<World>
	void OnGameStarted() override {}
	void OnGameFinished() override {}
	void OnNewObject(GC_Object &obj) override;
	void OnKill(GC_Object &obj) override;

public:
	ServiceListDataSource(World &world, LangCache &lang);
	~ServiceListDataSource();

private:
	mutable std::string _nameCache;
	UI::ListDataSourceListener *_listener;
	World &_world;
	LangCache &_lang;
};

class ConfCache;
class EditorLayout;

class ServiceEditor : public UI::Dialog
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ComboBox> DefaultComboBox;

	ServiceListDataSource _listData;
	std::shared_ptr<UI::List> _list;
	std::shared_ptr<DefaultComboBox> _combo;
	std::shared_ptr<UI::Text> _labelService;
	std::shared_ptr<UI::Text> _labelName;
	std::shared_ptr<UI::Button> _btnCreate;

	float _margins;
	World &_world;
	ConfCache &_conf;
	LangCache &_lang;

public:
	ServiceEditor(UI::LayoutManager &manager, float x, float y, float w, float h, World &world, ConfCache &conf, LangCache &lang);
	virtual ~ServiceEditor();

	void OnChangeSelectionGlobal(GC_Object *obj);

protected:
	void OnCreateService();
	void OnSelectService(int i);
	EditorLayout* GetEditorLayout() const;

	virtual void OnSize(float width, float height);
	virtual bool OnKeyPressed(UI::Key key);
};
