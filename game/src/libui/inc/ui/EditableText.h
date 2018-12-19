#pragma once
#include "PointerInput.h"
#include "Texture.h"
#include "Window.h"
#include <string_view>

namespace UI
{
	class EditableText
		: public Window
		, private PointerSink
		, private KeyboardSink
		, private TextSink
	{
	public:
		EditableText();

		int GetTextLength() const;

		std::string_view GetText() const;
		void SetText(std::string text);

		void SetInt(int value);
		int  GetInt() const;

		void  SetFloat(float value);
		float GetFloat() const;

		void SetSel(int begin, int end); // -1 means end of string
		int GetSelStart() const;
		int GetSelEnd() const;
		int GetSelMin() const;
		int GetSelMax() const;
		int GetSelLength() const;

		std::function<void()> eventChange;

		// Window
		FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;
		std::shared_ptr<Window> GetFocus() const override;
		void Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const override;
		bool HasPointerSink() const override { return true; }
		PointerSink* GetPointerSink() override { return this; }
		bool HasKeyboardSink() const override { return true; }
		KeyboardSink *GetKeyboardSink() override;
		bool HasTextSink() const override { return true; }
		TextSink* GetTextSink() override { return this; }
		vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const override;

	private:
		std::shared_ptr<Window> _fakeCursorPlaceholder = std::make_shared<Window>();
		std::string _text;
		int _selStart = 0;
		int _selEnd = 0;
		Texture _font = "font_small";
		Texture _cursor = "ui/editcursor";
		Texture _selection = "ui/editsel";

		int HitTest(TextureManager &texman, vec2d px, float scale) const;
		FRECT GetCursorRect(TextureManager &texman, const LayoutContext &lc) const;

		// TextSink
		bool OnChar(int c) override;
		void OnPaste(std::string_view text) override;
		std::string_view OnCopy() const override;
		std::string OnCut() override;

		// KeyboardSink
		bool OnKeyPressed(InputContext &ic, Plat::Key key) override;

		// PointerSink
		bool OnPointerDown(InputContext &ic, LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button) override;
		void OnPointerMove(InputContext &ic, LayoutContext &lc, TextureManager &texman, PointerInfo pi, bool captured) override;
		void OnTap(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override;
	};

} // namespace UI
