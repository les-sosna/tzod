#pragma once
#include "Window.h"
#include <iterator>
#include <memory>

namespace UI
{
	namespace detail
	{
		template <bool IsShared, bool IsConst>
		struct ReferenceTypeHelper {};

		template <>
		struct ReferenceTypeHelper<false /*IsShared*/, false /*IsConst*/>
		{
		protected:
			using pointer = Window*;
			using reference = Window *;
			using ParentType = Window *;
			static reference OperatorDereference(ParentType p, int i) { return &p->GetChild(i); }
		};

		template <>
		struct ReferenceTypeHelper<false /*IsShared*/, true /*IsConst*/>
		{
		protected:
			using pointer = const Window *;
			using reference = const Window *;
			using ParentType = const Window *;
			static reference OperatorDereference(ParentType p, int i) { return &p->GetChild(i); }
		};

		template <>
		struct ReferenceTypeHelper<true /*IsShared*/, true /*IsConst*/>
		{
		protected:
			using pointer = std::shared_ptr<const Window>;
			using reference = std::shared_ptr<const Window>;
			using ParentType = std::shared_ptr<const Window>;
			static reference OperatorDereference(const ParentType &p, int i) { return p->GetChild(p, i); }
		};

		template <>
		struct ReferenceTypeHelper<true /*IsShared*/, false /*IsConst*/>
		{
		protected:
			using pointer = std::shared_ptr<Window>;
			using reference = std::shared_ptr<Window>;
			using ParentType = std::shared_ptr<Window>;
			static reference OperatorDereference(const ParentType& p, int i) { return p->GetChild(p, i); }
		};
	}

	template <bool IsShared, bool IsConst>
	class WindowIterator
		: public detail::ReferenceTypeHelper<IsShared, IsConst>
	{
	public:
		using value_type = void;
		using pointer = typename detail::ReferenceTypeHelper<IsShared, IsConst>::pointer;
		using reference = typename detail::ReferenceTypeHelper<IsShared, IsConst>::reference;
		using difference_type = int;
		using iterator_category = std::random_access_iterator_tag;

		WindowIterator(typename detail::ReferenceTypeHelper<IsShared, IsConst>::ParentType parent, int index)
			: _parent(std::move(parent))
			, _index(index)
		{
			assert(_parent);
		}

		template <bool S, bool C>
		bool operator==(const WindowIterator<S, C> &other) const
		{
			assert(_parent == other._parent);
			return _index == other._index;
		}

		template <bool S, bool C>
		bool operator!=(const WindowIterator<S, C>&other) const
		{
			assert(_parent == other._parent);
			return _index != other._index;
		}

		template <bool S, bool C>
		bool operator<(const WindowIterator<S, C>&other) const
		{
			assert(_parent == other._parent);
			return _index < other._index;
		}

		template <bool S, bool C>
		bool operator>(const WindowIterator<S, C>&other) const
		{
			assert(_parent == other._parent);
			return _index > other._index;
		}

		template <bool S, bool C>
		bool operator<=(const WindowIterator<S, C>&other) const
		{
			assert(_parent == other._parent);
			return _index <= other._index;
		}

		template <bool S, bool C>
		bool operator>=(const WindowIterator<S, C>&other) const
		{
			assert(_parent == other._parent);
			return _index >= other._index;
		}

		template <bool S, bool C>
		difference_type operator-(const WindowIterator<S, C>&other) const
		{
			assert(_parent == other._parent);
			return _index - other._index;
		}

		WindowIterator operator+(difference_type offset) const
		{
			return { _parent, _index + offset };
		}

		WindowIterator operator-(difference_type offset) const
		{
			return { _parent, _index - offset };
		}

		WindowIterator& operator+=(difference_type offset)
		{
			_index += offset;
			return *this;
		}

		WindowIterator& operator-=(difference_type offset)
		{
			_index -= offset;
			return *this;
		}

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

		typename detail::ReferenceTypeHelper<IsShared, IsConst>::reference operator*() const
		{
			return detail::ReferenceTypeHelper<IsShared, IsConst>::OperatorDereference(_parent, _index);
		}

	protected:
		typename detail::ReferenceTypeHelper<IsShared, IsConst>::ParentType _parent;
		int _index;
	};

	inline auto begin(Window& wnd) // ref
	{
		return WindowIterator<false/*IsShared*/, false/*IsConst*/>(&wnd, 0);
	}
	inline auto begin(std::shared_ptr<Window> wnd) // shared
	{
		return WindowIterator<true/*IsShared*/, false/*IsConst*/>(std::move(wnd), 0);
	}
	inline auto begin(const Window& wnd) // const ref
	{
		return WindowIterator<false/*IsShared*/, true/*IsConst*/>(&wnd, 0);
	}
	inline auto begin(std::shared_ptr<const Window> wnd) // const shared
	{
		return WindowIterator<true/*IsShared*/, true/*IsConst*/>(std::move(wnd), 0);
	}

	inline auto end(Window& wnd) // ref
	{
		return WindowIterator<false/*IsShared*/, false/*IsConst*/>(&wnd, static_cast<int>(wnd.GetChildrenCount()));
	}
	inline auto end(std::shared_ptr<Window> wnd) // shared
	{
		auto childrenCount = wnd->GetChildrenCount();
		return WindowIterator<true/*IsShared*/, false/*IsConst*/>(std::move(wnd), childrenCount);
	}
	inline auto end(const Window& wnd) // const ref
	{
		return WindowIterator<false/*IsShared*/, true/*IsConst*/>(&wnd, static_cast<int>(wnd.GetChildrenCount()));
	}
	inline auto end(std::shared_ptr<const Window> wnd) // const shared
	{
		auto childrenCount = wnd->GetChildrenCount();
		return WindowIterator<true/*IsShared*/, true/*IsConst*/>(std::move(wnd), static_cast<int>(childrenCount));
	}

	inline auto rbegin(std::shared_ptr<Window> wnd)
	{
		return std::make_reverse_iterator(end(wnd));
	}
	inline auto rbegin(std::shared_ptr<const Window> wnd)
	{
		return std::make_reverse_iterator(end(wnd));
	}
	inline auto rbegin(Window &wnd)
	{
		return std::make_reverse_iterator(end(wnd));
	}
	inline auto rbegin(const Window &wnd)
	{
		return std::make_reverse_iterator(end(wnd));
	}
	inline auto rend(std::shared_ptr<Window> wnd)
	{
		return std::make_reverse_iterator(begin(wnd));
	}
	inline auto rend(std::shared_ptr<const Window> wnd)
	{
		return std::make_reverse_iterator(begin(wnd));
	}
	inline auto rend(Window &wnd)
	{
		return std::make_reverse_iterator(begin(wnd));
	}
	inline auto rend(const Window &wnd)
	{
		return std::make_reverse_iterator(begin(wnd));
	}

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
}
