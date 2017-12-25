#pragma once
#include "Window.h"
#include <iterator>
#include <memory>

namespace UI
{
	template <class ParentType>
	class WindowIteratorBase
	{
	public:
		WindowIteratorBase(ParentType &parent, int index)
			: _parent(&parent)
			, _index(index)
		{}

		constexpr bool operator==(WindowIteratorBase &other) const
		{
			assert(_parent == other._parent);
			return _index == other._index;
		}

		constexpr bool operator!=(WindowIteratorBase &other) const
		{
			assert(_parent == other._parent);
			return _index != other._index;
		}

		constexpr bool operator<(WindowIteratorBase &other) const
		{
			assert(_parent == other._parent);
			return _index < other._index;
		}

		constexpr bool operator>(WindowIteratorBase &other) const
		{
			assert(_parent == other._parent);
			return _index > other._index;
		}

		constexpr bool operator<=(WindowIteratorBase &other) const
		{
			assert(_parent == other._parent);
			return _index <= other._index;
		}

		constexpr bool operator>=(WindowIteratorBase &other) const
		{
			assert(_parent == other._parent);
			return _index >= other._index;
		}

	protected:
		ParentType *_parent;
		int _index;
	};

	class WindowConstIterator
		: public WindowIteratorBase<const Window>
	{
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = std::shared_ptr<const Window>;
		using difference_type = int;
		using pointer = std::shared_ptr<const Window>*;
		using reference = std::shared_ptr<const Window>;

		using WindowIteratorBase::WindowIteratorBase;

		WindowConstIterator& operator++() // pre
		{
			++_index;
			return *this;
		}

		WindowConstIterator operator++(int) // post
		{
			WindowConstIterator current(*this);
			++_index;
			return current;
		}

		WindowConstIterator& operator--() // pre
		{
			--_index;
			return *this;
		}

		WindowConstIterator operator--(int) // post
		{
			WindowConstIterator current(*this);
			--_index;
			return current;
		}

		value_type operator*() const
		{
			return _parent->GetChild(_index);
		}

		constexpr pointer operator->() const
		{
			assert(false); // cannot return pointer to temporary object
			return nullptr;
		}
	};

	class WindowIterator
		: public WindowIteratorBase<Window>
	{
	public:
		using iterator_category = std::random_access_iterator_tag;
		using value_type = std::shared_ptr<Window>;
		using difference_type = int;
		using pointer = std::shared_ptr<Window>*;
		using reference = std::shared_ptr<Window>;

		using WindowIteratorBase::WindowIteratorBase;

		WindowIterator& operator++() // pre
		{
			++_index;
			return *this;
		}

		WindowIterator operator++(int) // post
		{
			WindowIterator current(*this);
			++_index;
			return current;
		}

		WindowIterator& operator--() // pre
		{
			--_index;
			return *this;
		}

		WindowIterator operator--(int) // post
		{
			WindowIterator current(*this);
			--_index;
			return current;
		}

		value_type operator*() const
		{
			return _parent->GetChild(_index);
		}

		constexpr pointer operator->() const
		{
			assert(false); // cannot return pointer to temporary object
			return nullptr;
		}
	}; 
}

namespace std
{
	inline UI::WindowIterator begin(UI::Window &wnd)
	{
//		return wnd.GetChildren().begin();
		return UI::WindowIterator(wnd, 0);
	}

	inline UI::WindowIterator end(UI::Window &wnd)
	{
//		return wnd.GetChildren().end();
		return UI::WindowIterator(wnd, wnd.GetChildrenCount());
	}

	inline UI::WindowConstIterator begin(const UI::Window &wnd)
	{
//		return wnd.GetChildren().begin();
		return UI::WindowConstIterator(wnd, 0);
	}

	inline UI::WindowConstIterator end(const UI::Window &wnd)
	{
//		return wnd.GetChildren().end();
		return UI::WindowConstIterator(wnd, wnd.GetChildrenCount());
	}

	inline auto rbegin(UI::Window &wnd)
	{
		return std::make_reverse_iterator(end(wnd));
	}

	inline auto rend(UI::Window &wnd)
	{
		return std::make_reverse_iterator(begin(wnd));
	}

	inline auto rbegin(const UI::Window &wnd)
	{
		return std::make_reverse_iterator(end(wnd));
	}

	inline auto rend(const UI::Window &wnd)
	{
		return std::make_reverse_iterator(begin(wnd));
	}
}

namespace UI
{
	namespace detail
	{
		template <typename T>
		struct reversion_wrapper { T& iterable; };

		template <typename T>
		auto begin(reversion_wrapper<T> w) { return rbegin(w.iterable); }

		template <typename T>
		auto end(reversion_wrapper<T> w) { return rend(w.iterable); }
	}

	template <typename T>
	detail::reversion_wrapper<T> reverse(T&& iterable) { return { iterable }; }
}
