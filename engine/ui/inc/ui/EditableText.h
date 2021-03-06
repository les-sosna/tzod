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

		void SetFont(Texture fontTexture) { _font = std::move(fontTexture); }

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
		WindowLayout GetChildLayout(TextureManager &texman, const LayoutContext &lc, const DataContext &dc, const Window &child) const override;
		std::shared_ptr<const Window> GetFocus(const std::shared_ptr<const Window>& owner) const override;
		const Window* GetFocus() const override;
		void Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, const Plat::Input &input, float time, bool hovered) const override;
		PointerSink* GetPointerSink() override { return this; }
		KeyboardSink *GetKeyboardSink() override;
		TextSink* GetTextSink() override { return this; }
		vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const override;

	private:
		std::shared_ptr<Window> _fakeCursorPlaceholder = std::make_shared<Window>();
		std::string _text;
		int _selStart = -1;
		int _selEnd = -1;
		Texture _font = "font_small";
		Texture _cursor = "ui/editcursor";
		Texture _selection = "ui/editsel";

		int HitTest(TextureManager &texman, vec2d px, float scale) const;
		float GetCursorWidth(TextureManager &texman, float scale) const;
		FRECT GetCursorRect(TextureManager &texman, const LayoutContext &lc) const;

		// TextSink
		bool OnChar(int c) override;
		void OnPaste(std::string_view text) override;
		std::string_view OnCopy() const override;
		std::string OnCut() override;

		// KeyboardSink
		bool OnKeyPressed(const Plat::Input &input, const InputContext &ic, Plat::Key key) override;

		// PointerSink
		bool OnPointerDown(const Plat::Input &input, const  InputContext &ic, const LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button) override;
		void OnPointerMove(const Plat::Input &input, const  InputContext &ic, const LayoutContext &lc, TextureManager &texman, PointerInfo pi, bool captured) override;
		void OnTap(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override;
	};

} // namespace UI
