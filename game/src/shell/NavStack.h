#pragma once
#include <ui/PointerInput.h>
#include <ui/Window.h>
#include <ui/WindowIterator.h>
#include <vector>

class NavStack final
	: public UI::WindowContainer
	, private UI::Managerful
	, private UI::PointerSink
{
public:
	explicit NavStack(UI::TimeStepManager &manager);

	void SetFlowDirection(UI::FlowDirection flowDirection) { _flowDirection = flowDirection; }
	UI::FlowDirection GetFlowDirection() const { return _flowDirection; }
	void SetSpacing(float spacing) { _spacing = spacing; }
	void PopNavStack(UI::Window *wnd = nullptr);
	void PushNavStack(std::shared_ptr<UI::Window> wnd);

	UI::Window* GetNavFront() const;
	float GetNavigationDepth() const;

	template <class T>
	bool IsOnTop() const
	{
		return !!dynamic_cast<T*>(GetNavFront());
	}

	template <class T>
	bool IsOnStack() const
	{
		if (auto navFront = GetNavFront())
		{
			for (auto wnd : *this)
			{
				if (dynamic_cast<const T*>(wnd))
					return true;
				if (wnd == navFront)
					break;
			}
		}
		return false;
	}

	// UI::Window
	UI::PointerSink* GetPointerSink() override { return GetNavFront() ? this : nullptr; }
	UI::WindowLayout GetChildLayout(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;

private:
	enum class State
	{
		GoingForward,
		GoingBack,
	};

	UI::FlowDirection _flowDirection = UI::FlowDirection::Horizontal;
	State _state = State::GoingForward;
	float _spacing = 0;
	float _navTransitionStartTime = 0;
	float _foldTime = 0.25f;

	vec2d GetNavStackPixelSize(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc) const;
	float GetTransitionTimeLeft() const;
	float GetChildOpacity(const UI::Window& child) const;
	bool GetChildEnabled(const UI::Window& child) const;

	// PointerSink
	void OnPointerMove(const UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, bool captured) override;
	bool OnPointerDown(const UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, int button) override;
	void OnPointerUp(const UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, int button) override;
	void OnTap(const UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override;
};
