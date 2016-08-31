#pragma once
#include "Window.h"
#include "ListBase.h"
#include <functional>

class TextureManager;

namespace UI
{

class List
	: public Window
	, private PointerSink
	, private KeyboardSink
{
public:
	List(LayoutManager &manager, TextureManager &texman, ListDataSource* dataSource);
	virtual ~List();

	ListDataSource* GetData() const;

	vec2d GetItemSize(TextureManager &texman, float scale) const;
	int HitTest(vec2d pxPos, TextureManager &texman, float scale) const; // returns item index or -1

	int  GetCurSel() const;
	void SetCurSel(int sel, bool scroll = false);

	void SetFlowDirection(FlowDirection flowDirection) { _flowDirection = flowDirection; }

	void SetItemTemplate(std::shared_ptr<Window> itemTemplate);

	// list events
	std::function<void(int)> eventChangeCurSel;
	std::function<void(int)> eventClickItem;

	// Window
    vec2d GetContentSize(TextureManager &texman, const StateContext &sc, float scale) const override;

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

protected:
	// Window
	PointerSink* GetPointerSink() override { return this; }
	KeyboardSink *GetKeyboardSink() override { return this; }
	void Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;

private:
	List(const List &) = delete;
	List& operator=(const List &) = delete;

	std::shared_ptr<Window> _itemTemplate;
	FlowDirection _flowDirection = FlowDirection::Vertical;

	ListDataSource *_data;
	std::vector<float> _tabs;

	int _curSel;

	size_t _font;
	size_t _selection;

	// PointerSink
	bool OnPointerDown(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID) override;
	void OnTap(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override;

	// KeyboardSink
	bool OnKeyPressed(InputContext &ic, Key key) override;
};

} // namespace UI
