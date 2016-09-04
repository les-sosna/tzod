#pragma once
#include "Window.h"

namespace UI
{
	template<class T> struct DataSource;

	class Rectangle : public Window
	{
	public:
		explicit Rectangle(LayoutManager &manager);

		void SetBackColor(std::shared_ptr<DataSource<SpriteColor>> color);
		const DataSource<SpriteColor>& GetBackColor() const { return *_backColor; }

		void SetBorderColor(std::shared_ptr<DataSource<SpriteColor>> color) { _borderColor = color; }
		const DataSource<SpriteColor>& GetBorderColor() const { return *_borderColor; }

		void SetDrawBorder(bool enable) { _drawBorder = enable; }
		bool GetDrawBorder() const { return _drawBorder; }

		void SetDrawBackground(bool enable) { _drawBackground = enable; }
		bool GetDrawBackground() const { return _drawBackground; }

		void SetTexture(TextureManager &texman, const char *tex, bool fitSize);
		void SetTextureStretchMode(StretchMode stretchMode);
		float GetTextureWidth(TextureManager &texman)  const;
		float GetTextureHeight(TextureManager &texman) const;

		unsigned int GetFrame() const { return _frame; }
		void SetFrame(unsigned int n) { _frame = n; }

		// Window
		void Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;

	private:
		std::shared_ptr<DataSource<SpriteColor>> _backColor;
		std::shared_ptr<DataSource<SpriteColor>> _borderColor;
		size_t _texture = -1;
		StretchMode _textureStretchMode = StretchMode::Stretch;
		unsigned int _frame = 0;

		struct
		{
			bool _drawBorder : 1;
			bool _drawBackground : 1;
		};
	};
}
