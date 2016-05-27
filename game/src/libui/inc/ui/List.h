#pragma once
#include "Window.h"
#include "ListBase.h"
#include <functional>

class TextureManager;

namespace UI
{
class ScrollBarVertical;

class List
	: public Window
	, private PointerSink
	, private KeyboardSink
{
public:
	List(LayoutManager &manager, TextureManager &texman, ListDataSource* dataSource);
	virtual ~List();

	ListDataSource* GetData() const;

	float GetScrollPos() const;
	void SetScrollPos(float pos);

	float GetItemHeight() const;
	float GetNumLinesVisible() const;
	void AlignHeightToContent(float maxHeight = 512);
	int HitTest(float y); // returns index of item

	int  GetCurSel() const;
	void SetCurSel(int sel, bool scroll = false);

	void SetTabPos(int index, float pos);

	// list events
	std::function<void(int)> eventChangeCurSel;
	std::function<void(int)> eventClickItem;

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
	void OnSize(float width, float height) override;
	void Draw(bool hovered, bool focused, bool enabled, vec2d size, InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;

private:
	List(const List &) = delete;
	List& operator=(const List &) = delete;

	ListDataSource *_data;
	std::vector<float> _tabs;

	std::shared_ptr<ScrollBarVertical> _scrollBar;

	int _curSel;
	int _hotItem;

	size_t _font;
	size_t _selection;

	// PointerSink
	void OnPointerMove(InputContext &ic, vec2d pointerPosition, PointerType pointerType, unsigned int pointerID) override;
	void OnMouseLeave() override;
	void OnPointerDown(InputContext &ic, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID) override;
	void OnMouseWheel(vec2d pointerPosition, float z) override;
	void OnTap(InputContext &ic, vec2d pointerPosition) override;

	// KeyboardSink
	bool OnKeyPressed(InputContext &ic, Key key) override;
};

} // namespace UI
