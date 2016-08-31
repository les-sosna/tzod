#pragma once
#include "Rectangle.h"

namespace UI
{

class Button;

class ScrollBarBase : public Rectangle
{
public:
	ScrollBarBase(LayoutManager &manager, TextureManager &texman);

	void SetShowButtons(bool showButtons);
	bool GetShowButtons() const;

	virtual void SetPos(float pos);
	float GetPos() const;

	void  SetDocumentSize(float limit);
	float GetDocumentSize() const;

	void  SetLineSize(float ls);
	float GetLineSize() const;

	void  SetPageSize(float ps);
	float GetPageSize() const;

	void SetElementTextures(TextureManager &texman, const char *slider, const char *upleft, const char *downright);

	std::function<void(float)> eventScroll;

	// Window
	void Draw(const StateContext &sc, const LayoutContext &lc, const InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;
	FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const StateContext &sc, const Window &child) const override;

protected:
	virtual float Select(float x, float y) const = 0;
	float GetScrollPaneLength(const LayoutContext &lc) const;

	float _tmpBoxPos;
	std::shared_ptr<Button> _btnBox;
	std::shared_ptr<Button> _btnUpLeft;
	std::shared_ptr<Button> _btnDownRight;

private:
	void OnBoxMouseDown(float x, float y);
	void OnBoxMouseUp(float x, float y);
	void OnBoxMouseMove(float x, float y);

	void OnUpLeft();
	void OnDownRight();

	void OnLimitsChanged();

	float _pos;
	float _lineSize;
	float _pageSize;
	float _documentSize;

	bool _showButtons;
};

class ScrollBarVertical final : public ScrollBarBase
{
public:
	ScrollBarVertical(LayoutManager &manager, TextureManager &texman);

	// Window
	FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const StateContext &sc, const Window &child) const override;

protected:
	float Select(float x, float y) const override { return y; }
};

class ScrollBarHorizontal final : public ScrollBarBase
{
public:
	ScrollBarHorizontal(LayoutManager &manager, TextureManager &texman);

	// Window
	FRECT GetChildRect(TextureManager &texman, const LayoutContext &lc, const StateContext &sc, const Window &child) const override;

private:
	float Select(float x, float y) const override { return x; }
};

} // namespace UI
