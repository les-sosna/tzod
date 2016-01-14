#pragma once

namespace UI
{
	class Window;

	namespace detail
	{
		struct Resident;
	}

	class WindowWeakPtr
	{
	public:
		explicit WindowWeakPtr(Window *p);
        WindowWeakPtr();
		~WindowWeakPtr();

		Window* operator->() const;
		Window* Get() const;
		void Set(Window *p);

	private:
		WindowWeakPtr(const WindowWeakPtr&) = delete;
		WindowWeakPtr& operator = (const WindowWeakPtr&) = delete;
		detail::Resident *_resident;
	};

}