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

	namespace detail
	{
		template <class T>
		struct StaticConstants {};
	}

	template <class T>
	class StaticValue
		: public DataSource<T>
		, public detail::StaticConstants<T>
	{
	public:
		template <class V>
		StaticValue(V && value)
			: _value(std::forward<V>(value))
		{}

		// DataSource<T>
		T GetValue(const StateContext &sc) const override
		{
			return _value;
		}

	private:
		T _value;
	};

	namespace detail
	{
		template <>
		struct StaticConstants<bool>
		{
			static const std::shared_ptr<StaticValue<bool>>& True();
			static const std::shared_ptr<StaticValue<bool>>& False();
		};
	}

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
			return std::make_shared<StaticValue<SpriteColor>>(static_cast<SpriteColor>(n & 0xffffffff));
		}
	}
}
