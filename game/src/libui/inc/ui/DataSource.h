#pragma once
#include <video/RenderBase.h>
#include <map>
#include <memory>

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


	struct TextSource
	{
		virtual const std::string& GetText(const StateContext &sc) const = 0;
	};

	class StaticText : public TextSource
	{
	public:
		StaticText(std::string text) : _text(std::move(text)) {}

		// TextSource
		const std::string& GetText(const StateContext &sc) const override;

	private:
		std::string _text;
	};

	class ListDataSourceBinding : public TextSource
	{
	public:
		explicit ListDataSourceBinding(int column): _column(column) {}

		// UI::TextSource
		const std::string& GetText(const StateContext &sc) const override;

	private:
		int _column;
	};


	namespace DataSourceAliases
	{
		inline std::shared_ptr<TextSource> operator"" _txt(const char* str, size_t len)
		{
			return std::make_shared<StaticText>(std::string(str, str + len));
		}

		inline std::shared_ptr<StaticColor> operator"" _rgba(unsigned long long n)
		{
			assert(n <= 0xffffffff); // The number should fit into 32 bits
			return std::make_shared<StaticColor>(static_cast<SpriteColor>(n & 0xffffffff));
		}
	}
}
