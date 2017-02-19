#pragma once
#include <ui/Window.h>
#include <vector>

class NavStack
	: public UI::Window
	, private UI::PointerSink
{
public:
	explicit NavStack(UI::LayoutManager &manager);

	void SetSpacing(float spacing) { _spacing = spacing; }
	void PopNavStack(UI::Window *wnd = nullptr);
	void PushNavStack(std::shared_ptr<UI::Window> wnd);

	std::shared_ptr<UI::Window> GetNavFront() const;
	float GetNavigationDepth() const;

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
	UI::PointerSink* GetPointerSink() override { return GetNavFront() ? this : nullptr; }
	FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::StateContext &sc, const UI::Window &child) const override;
	float GetChildOpacity(const Window &child) const override;

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

	// PointerSink
	void OnPointerMove(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, UI::PointerType pointerType, unsigned int pointerID, bool captured) override;
	bool OnPointerDown(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID) override;
	void OnPointerUp(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID) override;
	void OnTap(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override;
};
