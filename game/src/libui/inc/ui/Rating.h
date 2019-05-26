#pragma once
#include "Texture.h"
#include "Window.h"

namespace UI
{
	template <class T> struct RenderData;

	class Rating : public Window
	{
	public:
		void SetMaxRating(unsigned int maxRating) { _maxRating = maxRating; }
		void SetRating(std::shared_ptr<RenderData<unsigned int>> rating);

		// Window
		void Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman, float time, bool hovered) const override;
		vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale, const LayoutConstraints &layoutConstraints) const override;

	private:
		Texture _texture = "ui/star";
		unsigned int _maxRating = 3;
		std::shared_ptr<RenderData<unsigned int>> _rating;
	};
}
