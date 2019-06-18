#pragma once
#include "Window.h"
#include <memory>

namespace UI
{
	class ScrollView
		: public Window
		, private ScrollSink
	{
	public:
		ScrollView();

		void SetContent(std::shared_ptr<Window> content);
		void SetVerticalScrollEnabled(bool enabled) { _verticalScrollEnabled = enabled; }
		void SetHorizontalScrollEnabled(bool enabled) { _horizontalScrollEnabled = enabled; }

		void SetStretchContent(bool stretchContent) { _stretchContent = stretchContent; }
		bool GetStretchContent() const { return _stretchContent; }

		// Window
		unsigned int GetChildrenCount() const override { return !!_content; }
		std::shared_ptr<const Window> GetChild(const std::shared_ptr<const Window>& owner, unsigned int index) const override { return _content; }
		const Window& GetChild(unsigned int index) const override { return *_content; }
		ScrollSink* GetScrollSink() override { return this; }
		WindowLayout GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;
		vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const override;
		std::shared_ptr<const Window> GetFocus(const std::shared_ptr<const Window>& owner) const override { return _content; }
		const Window* GetFocus() const override { return _content.get(); }

	private:
		std::shared_ptr<Window> _content;
		vec2d _offset = {};
		bool _verticalScrollEnabled = true;
		bool _horizontalScrollEnabled = false;
		bool _stretchContent = false;

		// ScrollSink
		void OnScroll(TextureManager &texman, const UI::InputContext &ic, const LayoutContext &lc, const DataContext &dc, vec2d scrollOffset, bool precise) override;
		void EnsureVisible(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, FRECT pxFocusRect) override;
	};

}// namespace UI
