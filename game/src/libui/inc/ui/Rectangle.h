#pragma once
#include "Window.h"

namespace UI
{
	class Rectangle : public Window
	{
	public:
		explicit Rectangle(LayoutManager &manager);

		void SetBackColor(SpriteColor color) { _backColor = color; }
		SpriteColor GetBackColor() const { return _backColor; }

		void SetBorderColor(SpriteColor color) { _borderColor = color; }
		SpriteColor GetBorderColor() const { return _borderColor; }

		void SetDrawBorder(bool enable) { _drawBorder = enable; }
		bool GetDrawBorder() const { return _drawBorder; }

		void SetDrawBackground(bool enable) { _drawBackground = enable; }
		bool GetDrawBackground() const { return _drawBackground; }

		void SetTexture(TextureManager &texman, const char *tex, bool fitSize);
		void SetTextureStretchMode(StretchMode stretchMode);
		float GetTextureWidth(TextureManager &texman)  const;
		float GetTextureHeight(TextureManager &texman) const;

		unsigned int GetFrameCount() const;
		unsigned int GetFrame() const { return _frame; }
		void SetFrame(unsigned int n) { assert(-1 == _texture || n < GetFrameCount()); _frame = n; }

		// Window
		void Draw(const LayoutContext &lc, InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;

	private:
		SpriteColor  _backColor = 0xffffffff;
		SpriteColor  _borderColor = 0xffffffff;
		size_t       _texture = -1;
		StretchMode  _textureStretchMode = StretchMode::Stretch;
		unsigned int _frame = 0;

		struct
		{
			bool _drawBorder : 1;
			bool _drawBackground : 1;
		};
	};
}
