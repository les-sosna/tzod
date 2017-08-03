#pragma once
#include "Texture.h"
#include "Window.h"

namespace UI
{
	struct IClipboard;

	class EditableText
		: public Window
		, private PointerSink
		, private KeyboardSink
		, private TextSink
	{
	public:
		EditableText();

		int GetTextLength() const;

		const std::string& GetText() const;
		void SetText(std::string_view text);

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

		void Paste(const IClipboard &clipboard);
		void Copy(IClipboard &clipboard) const;

		std::function<void()> eventChange;

		// Window
		void Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const override;
		PointerSink* GetPointerSink() override { return this; }
		KeyboardSink *GetKeyboardSink() override;
		TextSink* GetTextSink() override { return this; }
		vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const override;

	private:
		std::string _text;
		int _selStart = 0;
		int _selEnd = 0;
		Texture _font = "font_small";
		Texture _cursor = "ui/editcursor";
		Texture _selection = "ui/editsel";

		int HitTest(TextureManager &texman, vec2d px, float scale) const;

		// TextSink
		bool OnChar(int c) override;

		// KeyboardSink
		bool OnKeyPressed(InputContext &ic, Key key) override;

		// PointerSink
		bool OnPointerDown(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID) override;
		void OnPointerMove(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, PointerType pointerType, unsigned int pointerID, bool captured) override;
		void OnTap(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override;
	};

} // namespace UI
