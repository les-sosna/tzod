#pragma once
#include "Navigation.h"
#include "Window.h"
#include "ListBase.h"
#include <functional>

class TextureManager;

namespace UI
{

class List
	: public Window
	, private PointerSink
	, private NavigationSink
{
public:
	explicit List(ListDataSource* dataSource);
	virtual ~List();

	ListDataSource* GetData() const;

	vec2d GetItemSize(TextureManager &texman, float scale) const;
	int HitTest(vec2d pxPos, TextureManager &texman, float scale) const; // returns item index or -1

	int  GetCurSel() const;
	void SetCurSel(int sel);

	void SetFlowDirection(FlowDirection flowDirection) { _flowDirection = flowDirection; }

	std::shared_ptr<Window> GetItemTemplate() const { return _itemTemplate; }
	void SetItemTemplate(std::shared_ptr<Window> itemTemplate);

	// list events
	std::function<void(int)> eventChangeCurSel;
	std::function<void(int)> eventClickItem;

	// Window
	vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const override;
	PointerSink* GetPointerSink() override { return this; }
	NavigationSink *GetNavigationSink() override { return this; }
	void Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const override;

protected:
	// callback interface
	class ListCallbackImpl : public ListDataSourceListener
	{
	public:
		ListCallbackImpl(List *list);
	private:
		virtual void OnDeleteAllItems();
		virtual void OnDeleteItem(int idx);
		virtual void OnAddItem();
		List *_list;
	};

	ListCallbackImpl _callbacks;

private:
	List(const List &) = delete;
	List& operator=(const List &) = delete;

	std::shared_ptr<Window> _itemTemplate;
	FlowDirection _flowDirection = FlowDirection::Vertical;

	ListDataSource *_data;

	int _curSel;

	int GetNextIndex(Navigate navigate) const;

	// PointerSink
	bool OnPointerDown(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID) override;
	void OnTap(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override;

	// NavigationSink
	bool CanNavigate(Navigate navigate, const DataContext &dc) const override;
	void OnNavigate(Navigate navigate, NavigationPhase phase, const DataContext &dc) override;
};

} // namespace UI
