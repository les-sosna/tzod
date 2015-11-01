#pragma once
#include <gc/WorldEvents.h>
#include <ui/Dialog.h>
#include <ui/ListBase.h>
#include <string>

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
	ServiceListDataSource(World &world);
	~ServiceListDataSource();

private:
	mutable std::string _nameCache;
	UI::ListDataSourceListener *_listener;
	World &_world;
};

class ConfCache;
class EditorLayout;

class ServiceEditor : public UI::Dialog
{
	typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ComboBox> DefaultComboBox;

	ServiceListDataSource _listData;
	UI::List *_list;
	DefaultComboBox *_combo;
	UI::Text *_labelService;
	UI::Text *_labelName;
	UI::Button *_btnCreate;

	float _margins;
	World &_world;
	ConfCache &_conf;

public:
	ServiceEditor(UI::Window *parent, float x, float y, float w, float h, World &world, ConfCache &conf);
	virtual ~ServiceEditor();

protected:
	void OnChangeSelectionGlobal(GC_Object *obj);

	void OnCreateService();
	void OnSelectService(int i);
	EditorLayout* GetEditorLayout() const;

	virtual void OnSize(float width, float height);
	virtual bool OnKeyPressed(UI::Key key);
};
