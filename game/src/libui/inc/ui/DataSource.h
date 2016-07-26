#pragma once
#include <video/RenderBase.h>
#include <map>

namespace UI
{
	class StateContext;

	struct ColorSource
	{
		virtual SpriteColor GetColor(const StateContext &sc) const = 0;
	};

	class StaticColor : public ColorSource
	{
	public:
		StaticColor(SpriteColor color) : _color(color) {}

		// ColorSource
		SpriteColor GetColor(const StateContext &sc) const override;

	private:
		SpriteColor _color;
	};

	class ColorMap : public ColorSource
	{
	public:
		typedef std::map<std::string, SpriteColor> ColorMapType;
		ColorMap(SpriteColor defaultColor, ColorMapType &&colorMap)
			: _defaultColor(defaultColor)
			, _colorMap(std::move(colorMap))
		{}

		// ColorSource
		SpriteColor GetColor(const StateContext &sc) const override;

	private:
		SpriteColor _defaultColor;
		ColorMapType _colorMap;
	};
}
