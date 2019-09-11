#pragma once
#include <math/MyMath.h>

class TextureManager;

namespace Plat
{
	enum class PointerType;
}

namespace UI
{
	class InputContext;
	class LayoutContext;

	struct PointerInfo
	{
		vec2d position;
		Plat::PointerType type;
		unsigned int id;
	};

	struct PointerSink
	{
		virtual bool OnPointerDown(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button) { return true; }
		virtual void OnPointerUp(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, PointerInfo pi, int button) {}
		virtual void OnPointerMove(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, PointerInfo pi, bool captured) {}
		virtual void OnTap(const InputContext &ic, const LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) {}
	};
}
