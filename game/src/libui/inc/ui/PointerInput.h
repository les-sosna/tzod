#pragma once
#include <math/MyMath.h>

class TextureManager;

namespace UI
{
	class InputContext;
	class LayoutContext;

	enum class PointerType
	{
		Unknown,
		Mouse,
		Touch,
	};

	struct PointerInfo
	{
		vec2d position;
		PointerType type;
		unsigned int id;
	};

	struct PointerSink
	{
		virtual bool OnPointerDown(InputContext &ic, LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button) { return false; }
		virtual void OnPointerUp(InputContext &ic, LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button) {}
		virtual void OnPointerMove(InputContext &ic, LayoutContext &lc, TextureManager &texman, PointerInfo pi, bool captured) {}
		virtual void OnTap(InputContext &ic, LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) {}
	};
}
