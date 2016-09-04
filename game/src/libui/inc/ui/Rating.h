#pragma once
#include "Window.h"

namespace UI
{
	class Rating : public Window
	{
	public:
		Rating(LayoutManager &manager, TextureManager &texman);

		void SetMaxRating(unsigned int maxRating) { _maxRating = maxRating; }
		void SetRating(unsigned int rating) { _rating = rating; }

		// Window
		void Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;
		vec2d GetContentSize(TextureManager &texman, const StateContext &sc, float scale) const override;

	private:
		size_t _texture;
		unsigned int _maxRating = 3;
		unsigned int _rating = 0;
	};
}
