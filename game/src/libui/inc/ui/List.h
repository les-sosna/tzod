#pragma once
#include "Navigation.h"
#include "PointerInput.h"
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

	vec2d GetItemSize(TextureManager &texman, float scale, const LayoutConstraints &layoutConstraints) const;
	int HitTest(vec2d pxPos, const LayoutContext &lc, TextureManager &texman) const; // returns item index or -1

	int  GetCurSel() const;
	void SetCurSel(int sel);

	void SetFlowDirection(FlowDirection flowDirection) { _flowDirection = flowDirection; }
	FlowDirection GetFlowDirection() const { return _flowDirection; }

	void SetEnableNavigation(bool enableNavigation) { _enableNavigation = enableNavigation; }
	bool GetEnableNavigation() const { return _enableNavigation; }

	std::shared_ptr<Window> GetItemTemplate() const { return _itemTemplate; }
	void SetItemTemplate(std::shared_ptr<Window> itemTemplate);

	// list events
	std::function<void(int)> eventChangeCurSel;
	std::function<void(int)> eventClickItem;

	// Window
	WindowLayout GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;
	vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const override;
	bool HasPointerSink() const override { return true; }
	PointerSink* GetPointerSink() override { return this; }
	bool HasNavigationSink() const override { return _enableNavigation; }
	NavigationSink *GetNavigationSink() override { return this; }
	std::shared_ptr<Window> GetFocus() const override;
	void Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, float time, bool hovered) const override;

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
	bool _enableNavigation = true;

	int GetNextIndex(Navigate navigate) const;

	// PointerSink
	bool OnPointerDown(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button) override;
	void OnTap(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override;

	// NavigationSink
	bool CanNavigate(TextureManager& texman, const InputContext &ic, const LayoutContext& lc, const DataContext& dc, Navigate navigate) const override;
	void OnNavigate(TextureManager& texman, const InputContext &ic, const LayoutContext& lc, const DataContext& dc, Navigate navigate, NavigationPhase phase) override;
};

} // namespace UI
