#pragma once
#include "Window.h"

namespace UI
{
	class WindowIterator : public std::iterator<
		std::random_access_iterator_tag,
		std::shared_ptr<Window>,
		int,
		std::shared_ptr<Window>*, // pointer
		std::shared_ptr<Window>> // reference
	{
	public:
		WindowIterator(const UI::Window &parent, int index)
			: _parent(&parent)
			, _index(index)
		{}

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

		constexpr bool operator==(WindowIterator &other) const
		{
			assert(_parent == other._parent);
			return _index == other._index;
		}

		constexpr bool operator!=(WindowIterator &other) const
		{
			assert(_parent == other._parent);
			return _index != other._index;
		}

		constexpr bool operator<(WindowIterator &other) const
		{
			assert(_parent == other._parent);
			return _index < other._index;
		}

		constexpr bool operator>(WindowIterator &other) const
		{
			assert(_parent == other._parent);
			return _index > other._index;
		}

		constexpr bool operator<=(WindowIterator &other) const
		{
			assert(_parent == other._parent);
			return _index <= other._index;
		}

		constexpr bool operator>=(WindowIterator &other) const
		{
			assert(_parent == other._parent);
			return _index >= other._index;
		}

	private:
		const UI::Window *_parent;
		int _index;
	};
}

namespace std
{
	inline auto begin(const UI::Window &wnd)
	{
//		return wnd.GetChildren().begin();
		return UI::WindowIterator(wnd, 0);
	}

	inline auto end(const UI::Window &wnd)
	{
//		return wnd.GetChildren().end();
		return UI::WindowIterator(wnd, wnd.GetChildrenCount());
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
