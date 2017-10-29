#pragma once
#include "PointerInput.h"
#include "Window.h"

namespace UI
{
class EditableText;
class Rectangle;
class ScrollView;

class Edit
	: public Window
	, private PointerSink
	, private StateGen
{
public:
	Edit();

	const std::shared_ptr<EditableText>& GetEditable() const { return _editable; }

	// Window
	PointerSink* GetPointerSink() override { return this; }
	FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;
	vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const override;
	const StateGen* GetStateGen() const override { return this; }
	std::shared_ptr<Window> GetFocus() const override;

private:
	std::shared_ptr<Rectangle> _background;
	std::shared_ptr<ScrollView> _scrollView;
	std::shared_ptr<EditableText> _editable;

	// PointerSink
	bool OnPointerDown(InputContext &ic, LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button) override;
	void OnPointerMove(InputContext &ic, LayoutContext &lc, TextureManager &texman, PointerInfo pi, bool captured) override;
	void OnTap(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override;

	// StateGen
	void PushState(StateContext &sc, const LayoutContext &lc, const InputContext &ic) const override;
};

} // namespace UI
