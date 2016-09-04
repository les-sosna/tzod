#pragma once
#include <video/RenderBase.h>
#include <map>
#include <memory>
#include <string>

namespace UI
{
	class StateContext;

	template <class ValueType>
	struct DataSource
	{
		virtual ValueType GetValue(const StateContext &sc) const = 0;
	};

	class StaticColor : public DataSource<SpriteColor>
	{
	public:
		StaticColor(SpriteColor color) : _color(color) {}

		// DataSource<SpriteColor>
		SpriteColor GetValue(const StateContext &sc) const override;

	private:
		SpriteColor _color;
	};

	class ColorMap : public DataSource<SpriteColor>
	{
	public:
		typedef std::map<std::string, SpriteColor> ColorMapType;
		ColorMap(SpriteColor defaultColor, ColorMapType &&colorMap)
			: _defaultColor(defaultColor)
			, _colorMap(std::move(colorMap))
		{}

		// DataSource<SpriteColor>
		SpriteColor GetValue(const StateContext &sc) const override;

	private:
		SpriteColor _defaultColor;
		ColorMapType _colorMap;
	};


	class StaticText : public DataSource<const std::string&>
	{
	public:
		StaticText(std::string text) : _text(std::move(text)) {}

		// DataSource<const std::string&>
		const std::string& GetValue(const StateContext &sc) const override;

	private:
		std::string _text;
	};

	class ListDataSourceBinding : public DataSource<const std::string&>
	{
	public:
		explicit ListDataSourceBinding(int column): _column(column) {}

		// DataSource<const std::string&>
		const std::string& GetValue(const StateContext &sc) const override;

	private:
		int _column;
	};


	namespace DataSourceAliases
	{
		inline std::shared_ptr<DataSource<const std::string&>> operator"" _txt(const char* str, size_t len)
		{
			return std::make_shared<StaticText>(std::string(str, str + len));
		}

		inline std::shared_ptr<DataSource<SpriteColor>> operator"" _rgba(unsigned long long n)
		{
			assert(n <= 0xffffffff); // The number should fit into 32 bits
			return std::make_shared<StaticColor>(static_cast<SpriteColor>(n & 0xffffffff));
		}
	}
}
