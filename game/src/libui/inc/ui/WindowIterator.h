#pragma once
#include "Window.h"
#include <iterator>
#include <memory>

namespace UI
{
	class WindowIteratorBase
	{
	public:
		using difference_type = int;
		using iterator_category = std::random_access_iterator_tag;

		constexpr WindowIteratorBase(const Window &parent, int index)
			: _parent(&parent)
			, _index(index)
		{}

		constexpr bool operator==(const WindowIteratorBase &other) const
		{
			assert(_parent == other._parent);
			return _index == other._index;
		}

		constexpr bool operator!=(const WindowIteratorBase &other) const
		{
			assert(_parent == other._parent);
			return _index != other._index;
		}

		constexpr bool operator<(const WindowIteratorBase &other) const
		{
			assert(_parent == other._parent);
			return _index < other._index;
		}

		constexpr bool operator>(const WindowIteratorBase &other) const
		{
			assert(_parent == other._parent);
			return _index > other._index;
		}

		constexpr bool operator<=(const WindowIteratorBase &other) const
		{
			assert(_parent == other._parent);
			return _index <= other._index;
		}

		constexpr bool operator>=(const WindowIteratorBase &other) const
		{
			assert(_parent == other._parent);
			return _index >= other._index;
		}

		constexpr difference_type operator-(const WindowIteratorBase &other) const
		{
			assert(_parent == other._parent);
			return _index - other._index;
		}

	protected:
		const Window *_parent;
		int _index;
	};

	class WindowConstIterator
		: public WindowIteratorBase
	{
	public:
		using value_type = std::shared_ptr<const Window>;
		using pointer = std::shared_ptr<const Window>*;
		using reference = std::shared_ptr<const Window>;

		using WindowIteratorBase::WindowIteratorBase;

		WindowConstIterator& operator+=(int offset)
		{
			_index += offset;
			return *this;
		}

		WindowConstIterator& operator-=(int offset)
		{
			_index -= offset;
			return *this;
		}

		constexpr WindowConstIterator operator+(int offset) const
		{
			return WindowConstIterator(*_parent, _index + offset);
		}

//		constexpr WindowConstIterator operator-(int offset) const
//		{
//			return WindowConstIterator(*_parent, _index - offset);
//		}

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

		pointer operator->() const
		{
			assert(false); // cannot return pointer to temporary object
			return nullptr;
		}
	};

	class WindowIterator
		: public WindowIteratorBase
	{
	public:
		using value_type = std::shared_ptr<Window>;
		using pointer = std::shared_ptr<Window>*;
		using reference = std::shared_ptr<Window>;

		constexpr WindowIterator(Window &parent, int index)
			: WindowIteratorBase(parent, index)
		{}

		operator WindowConstIterator() const
		{
			return WindowConstIterator(*_parent, _index);
		}

		WindowIterator& operator+=(int offset)
		{
			_index += offset;
			return *this;
		}

		WindowIterator& operator-=(int offset)
		{
			_index -= offset;
			return *this;
		}

		constexpr WindowIterator operator+(int offset) const
		{
			return WindowIterator(*const_cast<Window*>(_parent), _index + offset);
		}

//		constexpr WindowIterator operator-(int offset) const
//		{
//			return WindowIterator(*const_cast<Window*>(_parent), _index - offset);
//		}

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
			return const_cast<Window*>(_parent)->GetChild(_index);
		}

		pointer operator->() const
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
		return UI::WindowIterator(wnd, 0);
	}

	inline UI::WindowIterator end(UI::Window &wnd)
	{
		return UI::WindowIterator(wnd, wnd.GetChildrenCount());
	}

	inline UI::WindowConstIterator begin(const UI::Window &wnd)
	{
		return UI::WindowConstIterator(wnd, 0);
	}

	inline UI::WindowConstIterator end(const UI::Window &wnd)
	{
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
		constexpr auto begin(reversion_wrapper<T> w) { return rbegin(w.iterable); }

		template <typename T>
		constexpr auto end(reversion_wrapper<T> w) { return rend(w.iterable); }
	}

	template <typename T>
	detail::reversion_wrapper<T> reverse(T&& iterable) { return { iterable }; }

	inline WindowConstIterator FindWindowChild(const UI::Window &wnd, const UI::Window &child)
	{
		for (auto it = begin(wnd); it != end(wnd); ++it)
		{
			if ((*it).get() == &child)
			{
				return it;
			}
		}
		return end(wnd);
	}
}
