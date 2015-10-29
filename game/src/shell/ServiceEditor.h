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
	virtual void AddListener(UI::ListDataSourceListener *listener);
	virtual void RemoveListener(UI::ListDataSourceListener *listener);
	virtual int GetItemCount() const;
	virtual int GetSubItemCount(int index) const;
	virtual size_t GetItemData(int index) const;
	virtual const std::string& GetItemText(int index, int sub) const;
	virtual int FindItem(const std::string &text) const;

	// ObjectListener<World>
	virtual void OnGameStarted() override {}
	virtual void OnGameFinished() override {}
	virtual void OnNewObject(GC_Object &obj) override;
	virtual void OnKill(GC_Object &obj) override;

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
