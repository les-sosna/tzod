#pragma once
#include "StateContext.h"
#include <video/RenderBase.h>
#include <map>
#include <memory>
#include <string>

namespace UI
{
	class DataContext;
	class StateContext;

	template <class ValueType>
	struct LayoutData
	{
		virtual ValueType GetValue(const DataContext &dc) const = 0;
	};

	template <class ValueType>
	struct RenderData
	{
		virtual ValueType GetValue(const DataContext &dc, const StateContext &sc) const = 0;
	};

	namespace detail
	{
		template <class T>
		struct StaticConstants {};
	}

	template <class T>
	class StaticValue
		: public LayoutData<T>
		, public RenderData<T>
		, public detail::StaticConstants<T>
	{
	public:
		template <class V>
		StaticValue(V && value)
			: _value(std::forward<V>(value))
		{}

		// LayoutData<T>
		T GetValue(const DataContext &dc) const override
		{
			return _value;
		}

		// RenderData<T>
		T GetValue(const DataContext &dc, const StateContext &sc) const override
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

	template <class T>
	class StateBinding : public RenderData<T>
	{
	public:
		typedef std::map<std::string, T> MapType;

		template <class T_, class M_>
		StateBinding(T_ &&defaultValue, M_ &&valueMap)
			: _defaultValue(std::forward<T_>(defaultValue))
			, _valueMap(std::forward<M_>(valueMap))
		{}

		// RenderData<T>
		T GetValue(const DataContext &dc, const StateContext &sc) const override
		{
			auto found = _valueMap.find(sc.GetState());
			return _valueMap.end() != found ? found->second : _defaultValue;
		}

	private:
		T _defaultValue;
		MapType _valueMap;
	};

	class StaticText
		: public LayoutData<std::string_view>
		, public RenderData<std::string_view>
	{
	public:
		StaticText(std::string_view text) : _text(std::move(text)) {}

		// LayoutData<std::string_view>
		std::string_view GetValue(const DataContext &dc) const override;

		// RenderData<std::string_view>
		std::string_view GetValue(const DataContext &dc, const StateContext &sc) const override;

	private:
		std::string _text;
	};

	class ListDataSourceBinding
		: public LayoutData<std::string_view>
		, public RenderData<std::string_view>
	{
	public:
		explicit ListDataSourceBinding(int column): _column(column) {}

		// LayoutData<std::string_view>
		std::string_view GetValue(const DataContext &dc) const override;

		// RenderData<std::string_view>
		std::string_view GetValue(const DataContext &dc, const StateContext &sc) const override;

	private:
		int _column;
	};


	namespace DataSourceAliases
	{
		inline std::shared_ptr<StaticText> operator"" _txt(const char* str, size_t len)
		{
			return std::make_shared<StaticText>(std::string(str, str + len));
		}

		inline std::shared_ptr<StaticValue<SpriteColor>> operator"" _rgba(unsigned long long n)
		{
			assert(n <= 0xffffffff); // The number should fit into 32 bits
			return std::make_shared<StaticValue<SpriteColor>>(static_cast<SpriteColor>(n & 0xffffffff));
		}
	}
}
