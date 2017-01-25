#pragma once
#include <ui/Window.h>
#include <vector>

class NavStack : public UI::Window
{
public:
	explicit NavStack(UI::LayoutManager &manager);

	void SetSpacing(float spacing) { _spacing = spacing; }
	void PopNavStack(UI::Window *wnd = nullptr);
	void PushNavStack(std::shared_ptr<UI::Window> wnd);

	std::shared_ptr<UI::Window> GetNavFront() const;

	template <class T>
	bool IsOnTop() const
	{
		return !!dynamic_cast<T*>(GetNavFront().get());
	}

	template <class T>
	bool IsOnStack() const
	{
		if (auto navFront = GetNavFront())
		{
			for (auto wnd : GetChildren())
			{
				if (dynamic_cast<T*>(wnd.get()))
					return true;
				if (wnd == navFront)
					break;
			}
		}
		return false;
	}

	// UI::Window
	FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::StateContext &sc, const UI::Window &child) const override;

private:
	enum class State
	{
		GoingForward,
		GoingBack,
	};

	State _state = State::GoingForward;
	float _spacing = 0;
	float _navTransitionStartTime = 0;
	float _foldTime = 0.25f;

	float GetNavStackPixelSize(TextureManager &texman, const UI::LayoutContext &lc, const UI::StateContext &sc) const;
	float GetTransitionTimeLeft() const;
};
