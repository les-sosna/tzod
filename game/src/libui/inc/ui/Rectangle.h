#pragma once
#include "Window.h"

namespace UI
{
	template<class T> struct RenderData;

	class Rectangle : public Window
	{
	public:
		Rectangle();

		void SetBackColor(std::shared_ptr<RenderData<SpriteColor>> color);
		const RenderData<SpriteColor>& GetBackColor() const { return *_backColor; }

		void SetBorderColor(std::shared_ptr<RenderData<SpriteColor>> color);
		const RenderData<SpriteColor>& GetBorderColor() const { return *_borderColor; }

		void SetDrawBorder(bool enable) { _drawBorder = enable; }
		bool GetDrawBorder() const { return _drawBorder; }

		void SetDrawBackground(bool enable) { _drawBackground = enable; }
		bool GetDrawBackground() const { return _drawBackground; }

		void SetTexture(TextureManager &texman, const char *tex);
		void SetTextureStretchMode(StretchMode stretchMode);
		float GetTextureWidth(TextureManager &texman)  const;
		float GetTextureHeight(TextureManager &texman) const;

		RenderData<unsigned int>* GetFrame() const { return _frame.get(); }
		void SetFrame(std::shared_ptr<RenderData<unsigned int>> frame) { _frame = std::move(frame); }

		// Window
		void Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman) const override;

	private:
		std::shared_ptr<RenderData<SpriteColor>> _backColor;
		std::shared_ptr<RenderData<SpriteColor>> _borderColor;
		std::shared_ptr<RenderData<unsigned int>> _frame;
		StretchMode _textureStretchMode = StretchMode::Stretch;
		std::string _textureName;
		mutable size_t _cachedTextureId = -1;

		size_t GetTextureId(TextureManager &texman) const;
		bool HasTexture() const { return -1 != _cachedTextureId; }

		struct
		{
			bool _drawBorder : 1;
			bool _drawBackground : 1;
		};
	};
}
