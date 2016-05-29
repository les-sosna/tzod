#pragma once

#include <video/RenderBase.h>

#include <cassert>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <deque>

class DrawingContext;
class TextureManager;

namespace UI
{

class InputContext;
class LayoutManager;
enum class Key;
enum class PointerType;

enum class StretchMode
{
	Stretch,
	Fill,
};

struct PointerSink
{
	virtual bool OnPointerDown(InputContext &ic, vec2d size, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID) { return false; }
	virtual void OnPointerUp(InputContext &ic, vec2d size, vec2d pointerPosition, int button, PointerType pointerType, unsigned int pointerID) {}
	virtual void OnPointerMove(InputContext &ic, vec2d size, vec2d pointerPosition, PointerType pointerType, unsigned int pointerID, bool captured) {}
	virtual void OnMouseWheel(InputContext &ic, vec2d size, vec2d pointerPosition, float z) {}
	virtual void OnTap(InputContext &ic, vec2d size, vec2d pointerPosition) {}
};

struct KeyboardSink
{
	virtual bool OnKeyPressed(InputContext &ic, Key key) = 0;
};

struct TextSink
{
	virtual bool OnChar(int c) = 0;
};

class Window : public std::enable_shared_from_this<Window>
{
	friend class LayoutManager;
	LayoutManager &_manager;

	std::shared_ptr<Window> _focusChild;
	std::deque<std::shared_ptr<Window>> _children;

	std::list<Window*>::iterator _timeStepReg;


	//
	// size and position
	//

	float _x = 0;
	float _y = 0;
	float _width = 0;
	float _height = 0;


	//
	// attributes
	//

	std::string  _text;

	SpriteColor  _backColor = 0xffffffff;
	SpriteColor  _borderColor = 0xffffffff;
	size_t       _texture = -1;
	StretchMode  _textureStretchMode = StretchMode::Stretch;
	unsigned int _frame = 0;

	struct
	{
		bool _isVisible      : 1;
		bool _isEnabled      : 1;
		bool _isTopMost      : 1;
		bool _isTimeStep     : 1;
		bool _drawBorder     : 1;
		bool _drawBackground : 1;
		bool _clipChildren   : 1;
	};

	void PrepareToUnlink(Window &child);

protected:
	unsigned int GetFrameCount() const;
	unsigned int GetFrame() const { return _frame; }
	void SetFrame(unsigned int n) { assert(-1 == _texture || n < GetFrameCount()); _frame = n; }

	void OnEnabledChangeInternal(bool enable, bool inherited);
	void OnVisibleChangeInternal(bool visible, bool inherited);

public:
	explicit Window(LayoutManager &manager);
	virtual ~Window();

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

	void UnlinkAllChildren();
	void UnlinkChild(Window &child);
	void AddFront(std::shared_ptr<Window> child);
	void AddBack(std::shared_ptr<Window> child);

	auto& GetChildren() const { return _children; }
	LayoutManager& GetManager() const { return _manager;  } // to remove

	virtual FRECT GetChildRect(vec2d size, const Window &child) const;

	//
	// Input
	//
	virtual PointerSink* GetPointerSink() { return nullptr; }
	virtual KeyboardSink* GetKeyboardSink() { return nullptr; }
	virtual TextSink* GetTextSink() { return nullptr; }

	//
	// Appearance
	//

	void SetBackColor(SpriteColor color)   { _backColor = color; }
	SpriteColor GetBackColor() const       { return _backColor;  }

	void SetBorderColor(SpriteColor color) { _borderColor = color; }
	SpriteColor GetBorderColor() const     { return _borderColor;  }

	void SetDrawBorder(bool enable)        { _drawBorder = enable; }
	bool GetDrawBorder() const             { return _drawBorder;   }

	void SetDrawBackground(bool enable)    { _drawBackground = enable; }
	bool GetDrawBackground() const         { return _drawBackground;   }

	void SetTexture(TextureManager &texman, const char *tex, bool fitSize);
	void SetTextureStretchMode(StretchMode stretchMode);
	float GetTextureWidth(TextureManager &texman)  const;
	float GetTextureHeight(TextureManager &texman) const;

	void SetVisible(bool show);
	bool GetVisible() const { return _isVisible; }

	void SetTopMost(bool topmost);
	bool GetTopMost() const { return _isTopMost; }

	void SetClipChildren(bool clip)  { _clipChildren = clip; }
	bool GetClipChildren() const     { return _clipChildren; }

	const std::string& GetText() const;
	void SetText(TextureManager &texman, const std::string &text);


	//
	// size & position
	//

	void Move(float x, float y);
	float GetX() const { return _x; }
	float GetY() const { return _y; }

	void Resize(float width, float height);
	void SetHeight(float height) { Resize(GetWidth(), height); }
	void SetWidth(float width) { Resize(width, GetHeight()); }
	float GetWidth()  const { return _width;  }
	float GetHeight() const { return _height; }


	//
	// Behavior
	//

	void SetEnabled(bool enable);
	bool GetEnabled() const { return _isEnabled; }

	void SetTimeStep(bool enable);
	bool GetTimeStep() const { return _isTimeStep; }

	void SetFocus(std::shared_ptr<Window> child);
	std::shared_ptr<Window> GetFocus() const;

	//
	// Events
	//
	std::function<void(void)> eventLostFocus;


	//
	// rendering
	//

	virtual void Draw(bool hovered, bool focused, bool enabled, vec2d size, InputContext &ic, DrawingContext &dc, TextureManager &texman) const;

private:

	//
	// size
	//

	virtual void OnSize(float width, float height);


	//
	// other
	//

	virtual void OnTextChange(TextureManager &texman);
	virtual void OnEnabledChange(bool enable, bool inherited);
	virtual void OnTimeStep(LayoutManager &manager, float dt);
};

inline bool NeedsFocus(Window *wnd)
{
	return wnd ? wnd->GetKeyboardSink() || wnd->GetTextSink() || NeedsFocus(wnd->GetFocus().get()) : false;
}

} // namespace UI
