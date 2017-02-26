#pragma once
#include "Window.h"

namespace UI
{
	template <class T> struct RenderData;

	class Rating : public Window
	{
	public:
		Rating(LayoutManager &manager, TextureManager &texman);

		void SetMaxRating(unsigned int maxRating) { _maxRating = maxRating; }
		void SetRating(std::shared_ptr<RenderData<unsigned int>> rating);

		// Window
		void Draw(const DataContext &dc, const StateContext &sc, const LayoutContext &lc, const InputContext &ic, RenderContext &rc, TextureManager &texman) const override;
		vec2d GetContentSize(TextureManager &texman, const DataContext &dc, float scale) const override;

	private:
		size_t _texture;
		unsigned int _maxRating = 3;
		std::shared_ptr<RenderData<unsigned int>> _rating;
	};
}
