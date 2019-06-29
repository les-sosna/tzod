#pragma once
#include "PointerInput.h"
#include "Window.h"
#include "Rectangle.h"
#include "ScrollView.h"

namespace UI
{
	class EditableText;

class Edit
	: public Window
	, private PointerSink
	, private StateGen
{
public:
	Edit();

	EditableText& GetEditable() { return *_editable; }

	// Window
	PointerSink* GetPointerSink() override { return this; }
	unsigned int GetChildrenCount() const override;
	std::shared_ptr<const Window> GetChild(const std::shared_ptr<const Window>& owner, unsigned int index) const override;
	const Window& GetChild(unsigned int index) const override;
	WindowLayout GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;
	vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const override;
	const StateGen* GetStateGen() const override { return this; }
	std::shared_ptr<const Window> GetFocus(const std::shared_ptr<const Window>& owner) const override;
	const Window* GetFocus() const override;

private:
	using EditBoxChildren = std::tuple<Rectangle, ScrollView>;
	EditBoxChildren _children;
	std::shared_ptr<EditableText> _editable;

	// PointerSink
	bool OnPointerDown(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button) override;
	void OnPointerMove(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, PointerInfo pi, bool captured) override;
	void OnTap(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override;

	// StateGen
	void PushState(StateContext &sc, const LayoutContext &lc, const InputContext &ic, bool hovered) const override;
};

} // namespace UI
