#include "inc/video/DrawingContext.h"
#include "inc/video/TextureManager.h"
#include <algorithm>

DrawingContext::DrawingContext(const TextureManager &tm, IRender &render, unsigned int width, unsigned int height)
	: _tm(tm)
	, _render(render)
	, _mode(RM_UNDEFINED)
{
	_transformStack.push({ vec2d{}, 255 });
	_viewport.left = 0;
	_viewport.top = 0;
	_viewport.right = width;
	_viewport.bottom = height;
	_render.OnResizeWnd(width, height);
}

void DrawingContext::PushClippingRect(RectRB rect)
{
	rect.left += (int) _transformStack.top().offset.x;
	rect.top += (int) _transformStack.top().offset.y;
	rect.right += (int) _transformStack.top().offset.x;
	rect.bottom += (int) _transformStack.top().offset.y;

	if( _clipStack.empty() )
	{
		_clipStack.push(rect);
		_render.SetScissor(&rect);
	}
	else
	{
		RectRB tmp = _clipStack.top();
		tmp.left = std::min(std::max(tmp.left, rect.left), rect.right);
		tmp.top = std::min(std::max(tmp.top, rect.top), rect.bottom);
		tmp.right = std::max(std::min(tmp.right, rect.right), rect.left);
		tmp.bottom = std::max(std::min(tmp.bottom, rect.bottom), rect.top);
		assert(tmp.right >= tmp.left && tmp.bottom >= tmp.top);
		_clipStack.push(tmp);
		_render.SetScissor(&tmp);
	}
}

void DrawingContext::PopClippingRect()
{
	assert(!_clipStack.empty());
	_clipStack.pop();
	if( _clipStack.empty() )
	{
		_render.SetScissor(&_viewport);
	}
	else
	{
		_render.SetScissor(&_clipStack.top());
	}
}

void DrawingContext::PushTransform(vec2d offset, float opacityCombined)
{
	assert(!_transformStack.empty());
	_transformStack.push({ _transformStack.top().offset + offset, static_cast<uint32_t>(opacityCombined * 255 + .5f) });
}

void DrawingContext::PopTransform()
{
	assert(_transformStack.size() > 1);
	_transformStack.pop();
}

RectRB DrawingContext::GetVisibleRegion() const
{
	RectRB visibleRegion = _clipStack.empty() ? _viewport : _clipStack.top();
	visibleRegion.left -= (int)_transformStack.top().offset.x;
	visibleRegion.top -= (int)_transformStack.top().offset.y;
	visibleRegion.right -= (int)_transformStack.top().offset.x;
	visibleRegion.bottom -= (int)_transformStack.top().offset.y;
	return visibleRegion;
}

static SpriteColor ApplyOpacity(SpriteColor color, uint8_t opacity)
{
	color.r = (uint16_t(color.r) * opacity) >> 8;
	color.g = (uint16_t(color.g) * opacity) >> 8;
	color.b = (uint16_t(color.b) * opacity) >> 8;
	color.a = (uint16_t(color.a) * opacity) >> 8;
	return color;
}

void DrawingContext::DrawSprite(FRECT dst, size_t sprite, SpriteColor color, unsigned int frame)
{
	color = ApplyOpacity(color, _transformStack.top().opacity);

	if (color.a == 0)
		return;

	const LogicalTexture &lt = _tm.GetSpriteInfo(sprite);
	const FRECT &rt = lt.uvFrames[frame];

	MyVertex *v = _render.DrawQuad(_tm.GetDeviceTexture(sprite));

	if (_mode == RM_INTERFACE)
	{
		dst.left += _transformStack.top().offset.x;
		dst.top += _transformStack.top().offset.y;
		dst.right += _transformStack.top().offset.x;
		dst.bottom += _transformStack.top().offset.y;
	}

	v[0].color = color;
	v[0].u = rt.left;
	v[0].v = rt.top;
	v[0].x = dst.left;
	v[0].y = dst.top;

	v[1].color = color;
	v[1].u = rt.right;
	v[1].v = rt.top;
	v[1].x = dst.right;
	v[1].y = dst.top;

	v[2].color = color;
	v[2].u = rt.right;
	v[2].v = rt.bottom;
	v[2].x = dst.right;
	v[2].y = dst.bottom;

	v[3].color = color;
	v[3].u = rt.left;
	v[3].v = rt.bottom;
	v[3].x = dst.left;
	v[3].y = dst.bottom;
}

void DrawingContext::DrawBorder(const FRECT &dst, size_t sprite, SpriteColor color, unsigned int frame)
{
	color = ApplyOpacity(color, _transformStack.top().opacity);

	if (color.a == 0)
		return;

	const LogicalTexture &lt = _tm.GetSpriteInfo(sprite);
	const DEV_TEXTURE &devtex = _tm.GetDeviceTexture(sprite);

	FRECT uvFrame = lt.uvFrames[frame];
	float uvFrameWidth = WIDTH(uvFrame);
	float uvFrameHeight = HEIGHT(uvFrame);

	const float uvBorderWidth = lt.pxBorderSize * uvFrameWidth / lt.pxFrameWidth;
	const float uvBorderHeight = lt.pxBorderSize * uvFrameHeight / lt.pxFrameHeight;

	const float left = dst.left + _transformStack.top().offset.x;
	const float top = dst.top + _transformStack.top().offset.y;
	const float right = dst.right + _transformStack.top().offset.x;
	const float bottom = dst.bottom + _transformStack.top().offset.y;

	MyVertex *v;
	IRender &render = _render;

	// left edge
	v = render.DrawQuad(devtex);
	v[0].color = color;
	v[0].u = uvFrame.left - uvBorderWidth;
	v[0].v = uvFrame.top;
	v[0].x = left;
	v[0].y = top + lt.pxBorderSize;
	v[1].color = color;
	v[1].u = uvFrame.left;
	v[1].v = uvFrame.top;
	v[1].x = left + lt.pxBorderSize;
	v[1].y = top + lt.pxBorderSize;
	v[2].color = color;
	v[2].u = uvFrame.left;
	v[2].v = uvFrame.bottom;
	v[2].x = left + lt.pxBorderSize;
	v[2].y = bottom - lt.pxBorderSize;
	v[3].color = color;
	v[3].u = uvFrame.left - uvBorderWidth;
	v[3].v = uvFrame.bottom;
	v[3].x = left;
	v[3].y = bottom - lt.pxBorderSize;

	// right edge
	v = render.DrawQuad(devtex);
	v[0].color = color;
	v[0].u = uvFrame.right;
	v[0].v = uvFrame.top;
	v[0].x = right - lt.pxBorderSize;
	v[0].y = top + lt.pxBorderSize;
	v[1].color = color;
	v[1].u = uvFrame.right + uvBorderWidth;
	v[1].v = uvFrame.top;
	v[1].x = right;
	v[1].y = top + lt.pxBorderSize;
	v[2].color = color;
	v[2].u = uvFrame.right + uvBorderWidth;
	v[2].v = uvFrame.bottom;
	v[2].x = right;
	v[2].y = bottom - lt.pxBorderSize;
	v[3].color = color;
	v[3].u = uvFrame.right;
	v[3].v = uvFrame.bottom;
	v[3].x = right - lt.pxBorderSize;
	v[3].y = bottom - lt.pxBorderSize;

	// top edge
	v = render.DrawQuad(devtex);
	v[0].color = color;
	v[0].u = uvFrame.left;
	v[0].v = uvFrame.top - uvBorderHeight;
	v[0].x = left + lt.pxBorderSize;
	v[0].y = top;
	v[1].color = color;
	v[1].u = uvFrame.right;
	v[1].v = uvFrame.top - uvBorderHeight;
	v[1].x = right - lt.pxBorderSize;
	v[1].y = top;
	v[2].color = color;
	v[2].u = uvFrame.right;
	v[2].v = uvFrame.top;
	v[2].x = right - lt.pxBorderSize;
	v[2].y = top + lt.pxBorderSize;
	v[3].color = color;
	v[3].u = uvFrame.left;
	v[3].v = uvFrame.top;
	v[3].x = left + lt.pxBorderSize;
	v[3].y = top + lt.pxBorderSize;

	// bottom edge
	v = render.DrawQuad(devtex);
	v[0].color = color;
	v[0].u = uvFrame.left;
	v[0].v = uvFrame.bottom;
	v[0].x = left + lt.pxBorderSize;
	v[0].y = bottom - lt.pxBorderSize;
	v[1].color = color;
	v[1].u = uvFrame.right;
	v[1].v = uvFrame.bottom;
	v[1].x = right - lt.pxBorderSize;
	v[1].y = bottom - lt.pxBorderSize;
	v[2].color = color;
	v[2].u = uvFrame.right;
	v[2].v = uvFrame.bottom + uvBorderHeight;
	v[2].x = right - lt.pxBorderSize;
	v[2].y = bottom;
	v[3].color = color;
	v[3].u = uvFrame.left;
	v[3].v = uvFrame.bottom + uvBorderHeight;
	v[3].x = left + lt.pxBorderSize;
	v[3].y = bottom;

	// left top corner
	v = render.DrawQuad(devtex);
	v[0].color = color;
	v[0].u = uvFrame.left - uvBorderWidth;
	v[0].v = uvFrame.top - uvBorderHeight;
	v[0].x = left;
	v[0].y = top;
	v[1].color = color;
	v[1].u = uvFrame.left;
	v[1].v = uvFrame.top - uvBorderHeight;
	v[1].x = left + lt.pxBorderSize;
	v[1].y = top;
	v[2].color = color;
	v[2].u = uvFrame.left;
	v[2].v = uvFrame.top;
	v[2].x = left + lt.pxBorderSize;
	v[2].y = top + lt.pxBorderSize;
	v[3].color = color;
	v[3].u = uvFrame.left - uvBorderWidth;
	v[3].v = uvFrame.top;
	v[3].x = left;
	v[3].y = top + lt.pxBorderSize;

	// right top corner
	v = render.DrawQuad(devtex);
	v[0].color = color;
	v[0].u = uvFrame.right;
	v[0].v = uvFrame.top - uvBorderHeight;
	v[0].x = right - lt.pxBorderSize;
	v[0].y = top;
	v[1].color = color;
	v[1].u = uvFrame.right + uvBorderWidth;
	v[1].v = uvFrame.top - uvBorderHeight;
	v[1].x = right;
	v[1].y = top;
	v[2].color = color;
	v[2].u = uvFrame.right + uvBorderWidth;
	v[2].v = uvFrame.top;
	v[2].x = right;
	v[2].y = top + lt.pxBorderSize;
	v[3].color = color;
	v[3].u = uvFrame.right;
	v[3].v = uvFrame.top;
	v[3].x = right - lt.pxBorderSize;
	v[3].y = top + lt.pxBorderSize;

	// right bottom corner
	v = render.DrawQuad(devtex);
	v[0].color = color;
	v[0].u = uvFrame.right;
	v[0].v = uvFrame.bottom;
	v[0].x = right - lt.pxBorderSize;
	v[0].y = bottom - lt.pxBorderSize;
	v[1].color = color;
	v[1].u = uvFrame.right + uvBorderWidth;
	v[1].v = uvFrame.bottom;
	v[1].x = right;
	v[1].y = bottom - lt.pxBorderSize;
	v[2].color = color;
	v[2].u = uvFrame.right + uvBorderWidth;
	v[2].v = uvFrame.bottom + uvBorderHeight;
	v[2].x = right;
	v[2].y = bottom;
	v[3].color = color;
	v[3].u = uvFrame.right;
	v[3].v = uvFrame.bottom + uvBorderHeight;
	v[3].x = right - lt.pxBorderSize;
	v[3].y = bottom;

	// left bottom corner
	v = render.DrawQuad(devtex);
	v[0].color = color;
	v[0].u = uvFrame.left - uvBorderWidth;
	v[0].v = uvFrame.bottom;
	v[0].x = left;
	v[0].y = bottom - lt.pxBorderSize;
	v[1].color = color;
	v[1].u = uvFrame.left;
	v[1].v = uvFrame.bottom;
	v[1].x = left + lt.pxBorderSize;
	v[1].y = bottom - lt.pxBorderSize;
	v[2].color = color;
	v[2].u = uvFrame.left;
	v[2].v = uvFrame.bottom + uvBorderHeight;
	v[2].x = left + lt.pxBorderSize;
	v[2].y = bottom;
	v[3].color = color;
	v[3].u = uvFrame.left - uvBorderWidth;
	v[3].v = uvFrame.bottom + uvBorderHeight;
	v[3].x = left;
	v[3].y = bottom;
}

void DrawingContext::DrawBitmapText(vec2d origin, float scale, size_t tex, SpriteColor color, const std::string &str, enumAlignText align)
{
	color = ApplyOpacity(color, _transformStack.top().opacity);

	if (color.a == 0)
		return;

	// grep enum enumAlignText LT CT RT LC CC RC LB CB RB
	static const float dx[] = { 0, 1, 2, 0, 1, 2, 0, 1, 2 };
	static const float dy[] = { 0, 0, 0, 1, 1, 1, 2, 2, 2 };

	std::vector<size_t> lines;
	size_t maxline = 0;
	if( align )
	{
		size_t count = 0;
		for( const std::string::value_type *tmp = str.c_str(); *tmp; )
		{
			++count;
			++tmp;
			if( '\n' == *tmp || '\0' == *tmp )
			{
				if( maxline < count )
					maxline = count;
				lines.push_back(count);
				count = 0;
			}
		}
	}

	if (_mode == RM_INTERFACE)
	{
		origin += _transformStack.top().offset;
	}

	const LogicalTexture &lt = _tm.GetSpriteInfo(tex);
	IRender &render = _render;

	size_t count = 0;
	size_t line  = 0;

	vec2d pxCharSize = Vec2dFloor(vec2d{ lt.pxFrameWidth, lt.pxFrameHeight } * scale);
    float pxAdvance = std::floor((lt.pxFrameWidth - 1) * scale);

	float x0 = origin.x - std::floor(dx[align] * pxAdvance * (float) maxline / 2);
	float y0 = origin.y - std::floor(dy[align] * pxCharSize.y * (float) lines.size() / 2);

	for( const std::string::value_type *tmp = str.c_str(); *tmp; ++tmp )
	{
		if( '\n' == *tmp )
		{
			++line;
			count = 0;
		}

		if( (unsigned char) *tmp < 32 )
		{
			continue;
		}

		const FRECT &rt = lt.uvFrames[(unsigned char) *tmp - 32];
		float x = x0 + (float) ((count++) * pxAdvance);
		float y = y0 + (float) (line * pxCharSize.y);

		MyVertex *v = render.DrawQuad(_tm.GetDeviceTexture(tex));

		v[0].color = color;
		v[0].u = rt.left;
		v[0].v = rt.top;
		v[0].x = x;
		v[0].y = y;

		v[1].color = color;
		v[1].u = rt.left + WIDTH(rt);
		v[1].v = rt.top;
		v[1].x = x + pxCharSize.x;
		v[1].y = y;

		v[2].color = color;
		v[2].u = rt.left + WIDTH(rt);
		v[2].v = rt.bottom;
		v[2].x = x + pxCharSize.x;
		v[2].y = y + pxCharSize.y;

		v[3].color = color;
		v[3].u = rt.left;
		v[3].v = rt.bottom;
		v[3].x = x;
		v[3].y = y + pxCharSize.y;
	}
}

void DrawingContext::DrawSprite(size_t tex, unsigned int frame, SpriteColor color, float x, float y, vec2d dir)
{
	color = ApplyOpacity(color, _transformStack.top().opacity);

	if (color.a == 0)
		return;

	assert(frame < _tm.GetFrameCount(tex));
	const LogicalTexture &lt = _tm.GetSpriteInfo(tex);
	const FRECT &rt = lt.uvFrames[frame];
	IRender &render = _render;

	MyVertex *v = render.DrawQuad(_tm.GetDeviceTexture(tex));

	if (_mode == RM_INTERFACE)
	{
		x += _transformStack.top().offset.x;
		y += _transformStack.top().offset.y;
	}

	float width = lt.pxFrameWidth;
	float height = lt.pxFrameHeight;

	float px = lt.uvPivot.x * width;
	float py = lt.uvPivot.y * height;

	v[0].color = color;
	v[0].u = rt.left;
	v[0].v = rt.top;
	v[0].x = x - px * dir.x + py * dir.y;
	v[0].y = y - px * dir.y - py * dir.x;

	v[1].color = color;
	v[1].u = rt.right;
	v[1].v = rt.top;
	v[1].x = x + (width - px) * dir.x + py * dir.y;
	v[1].y = y + (width - px) * dir.y - py * dir.x;

	v[2].color = color;
	v[2].u = rt.right;
	v[2].v = rt.bottom;
	v[2].x = x + (width - px) * dir.x - (height - py) * dir.y;
	v[2].y = y + (width - px) * dir.y + (height - py) * dir.x;

	v[3].color = color;
	v[3].u = rt.left;
	v[3].v = rt.bottom;
	v[3].x = x - px * dir.x - (height - py) * dir.y;
	v[3].y = y - px * dir.y + (height - py) * dir.x;
}

void DrawingContext::DrawSprite(size_t tex, unsigned int frame, SpriteColor color, float x, float y, float width, float height, vec2d dir)
{
	color = ApplyOpacity(color, _transformStack.top().opacity);

	if (color.a == 0)
		return;

	const LogicalTexture &lt = _tm.GetSpriteInfo(tex);
	const FRECT &rt = lt.uvFrames[frame];

	MyVertex *v = _render.DrawQuad(_tm.GetDeviceTexture(tex));

	if (_mode == RM_INTERFACE)
	{
		x += _transformStack.top().offset.x;
		y += _transformStack.top().offset.y;
	}

	float px = lt.uvPivot.x * width;
	float py = lt.uvPivot.y * height;

	v[0].color = color;
	v[0].u = rt.left;
	v[0].v = rt.top;
	v[0].x = x - px * dir.x + py * dir.y;
	v[0].y = y - px * dir.y - py * dir.x;

	v[1].color = color;
	v[1].u = rt.right;
	v[1].v = rt.top;
	v[1].x = x + (width - px) * dir.x + py * dir.y;
	v[1].y = y + (width - px) * dir.y - py * dir.x;

	v[2].color = color;
	v[2].u = rt.right;
	v[2].v = rt.bottom;
	v[2].x = x + (width - px) * dir.x - (height - py) * dir.y;
	v[2].y = y + (width - px) * dir.y + (height - py) * dir.x;

	v[3].color = color;
	v[3].u = rt.left;
	v[3].v = rt.bottom;
	v[3].x = x - px * dir.x - (height - py) * dir.y;
	v[3].y = y - px * dir.y + (height - py) * dir.x;
}

void DrawingContext::DrawIndicator(size_t tex, float x, float y, float value)
{
	const LogicalTexture &lt = _tm.GetSpriteInfo(tex);
	const FRECT &rt = lt.uvFrames[0];
	IRender &render = _render;

	float px = lt.uvPivot.x * lt.pxFrameWidth;
	float py = lt.uvPivot.y * lt.pxFrameHeight;

	MyVertex *v = render.DrawQuad(_tm.GetDeviceTexture(tex));

	v[0].color = 0xffffffff;
	v[0].u = rt.left;
	v[0].v = rt.top;
	v[0].x = x - px;
	v[0].y = y - py;

	v[1].color = 0xffffffff;
	v[1].u = rt.left + WIDTH(rt) * value;
	v[1].v = rt.top;
	v[1].x = x - px + lt.pxFrameWidth * value;
	v[1].y = y - py;

	v[2].color = 0xffffffff;
	v[2].u = rt.left + WIDTH(rt) * value;
	v[2].v = rt.bottom;
	v[2].x = x - px + lt.pxFrameWidth * value;
	v[2].y = y - py + lt.pxFrameHeight;

	v[3].color = 0xffffffff;
	v[3].u = rt.left;
	v[3].v = rt.bottom;
	v[3].x = x - px;
	v[3].y = y - py + lt.pxFrameHeight;
}

void DrawingContext::DrawLine(size_t tex, SpriteColor color,
                              float x0, float y0, float x1, float y1, float phase)
{
	color = ApplyOpacity(color, _transformStack.top().opacity);

	if (color.a == 0)
		return;

	const LogicalTexture &lt = _tm.GetSpriteInfo(tex);
	IRender &render = _render;

	MyVertex *v = render.DrawQuad(_tm.GetDeviceTexture(tex));

	float len = sqrtf((x1-x0)*(x1-x0) + (y1-y0)*(y1-y0));
	float phase1 = phase + len / lt.pxFrameWidth;
	float c = (x1-x0) / len;
	float s = (y1-y0) / len;
	float py = lt.pxFrameHeight / 2;

	v[0].color = color;
	v[0].u = phase;
	v[0].v = 0;
	v[0].x = x0 + py * s;
	v[0].y = y0 - py * c;

	v[1].color = color;
	v[1].u = phase1;
	v[1].v = 0;
	v[1].x = x0 + len * c + py * s;
	v[1].y = y0 + len * s - py * c;

	v[2].color = color;
	v[2].u = phase1;
	v[2].v = 1;
	v[2].x = x0 + len * c - py * s;
	v[2].y = y0 + len * s + py * c;

	v[3].color = color;
	v[3].u = phase;
	v[3].v = 1;
	v[3].x = x0 - py * s;
	v[3].y = y0 + py * c;
}

void DrawingContext::DrawBackground(size_t tex, FRECT bounds) const
{
	const LogicalTexture &lt = _tm.GetSpriteInfo(tex);
	IRender &render = _render;
	MyVertex *v = render.DrawQuad(_tm.GetDeviceTexture(tex));
	v[0].color = 0xffffffff;
	v[0].u = bounds.left / lt.pxFrameWidth;
	v[0].v = bounds.top / lt.pxFrameHeight;
	v[0].x = bounds.left;
	v[0].y = bounds.top;
	v[1].color = 0xffffffff;
	v[1].u = bounds.right / lt.pxFrameWidth;
	v[1].v = bounds.top / lt.pxFrameHeight;
	v[1].x = bounds.right;
	v[1].y = bounds.top;
	v[2].color = 0xffffffff;
	v[2].u = bounds.right / lt.pxFrameWidth;
	v[2].v = bounds.bottom / lt.pxFrameHeight;
	v[2].x = bounds.right;
	v[2].y = bounds.bottom;
	v[3].color = 0xffffffff;
	v[3].u = bounds.left / lt.pxFrameWidth;
	v[3].v = bounds.bottom / lt.pxFrameHeight;
	v[3].x = bounds.left;
	v[3].y = bounds.bottom;
}

static const int SINTABLE_SIZE = 32;
static const int SINTABLE_MASK = 0x1f;

static float sintable[SINTABLE_SIZE] = {
	 0.000000f, 0.195090f, 0.382683f, 0.555570f,
	 0.707106f, 0.831469f, 0.923879f, 0.980785f,
	 1.000000f, 0.980785f, 0.923879f, 0.831469f,
	 0.707106f, 0.555570f, 0.382683f, 0.195090f,
	-0.000000f,-0.195090f,-0.382683f,-0.555570f,
	-0.707106f,-0.831469f,-0.923879f,-0.980785f,
	-1.000000f,-0.980785f,-0.923879f,-0.831469f,
	-0.707106f,-0.555570f,-0.382683f,-0.195090f,
};

void DrawingContext::DrawPointLight(float intensity, float radius, vec2d pos)
{
	SpriteColor color;
	color.color = 0x00000000;
	color.a = (unsigned char) std::max(0, std::min(255, int(255.0f * intensity)));

	IRender &render = _render;
	MyVertex *v = render.DrawFan(SINTABLE_SIZE>>1);
	v[0].color = color;
	v[0].x = pos.x;
	v[0].y = pos.y;
	for( int i = 0; i < SINTABLE_SIZE>>1; i++ )
	{
		v[i+1].x = pos.x + radius * sintable[(i<<1)+(SINTABLE_SIZE>>2) & SINTABLE_MASK];
		v[i+1].y = pos.y + radius * sintable[i<<1];
		v[i+1].color.color = 0x00000000;
	}
}

void DrawingContext::DrawSpotLight(float intensity, float radius, vec2d pos, vec2d dir, float offset, float aspect)
{
	SpriteColor color;
	color.color = 0x00000000;
	color.a = (unsigned char) std::max(0, std::min(255, int(255.0f * intensity)));

	IRender &render = _render;
	MyVertex *v = render.DrawFan(SINTABLE_SIZE);
	v[0].color = color;
	v[0].x = pos.x;
	v[0].y = pos.y;
	for( int i = 0; i < SINTABLE_SIZE; i++ )
	{
		float x = offset + radius * sintable[i+(SINTABLE_SIZE>>2) & SINTABLE_MASK];
		float y =          radius * sintable[i] * aspect;
		v[i+1].x = pos.x + x*dir.x - y*dir.y;
		v[i+1].y = pos.y + y*dir.x + x*dir.y;
		v[i+1].color.color = 0x00000000;
	}
}

void DrawingContext::DrawDirectLight(float intensity, float radius, vec2d pos, vec2d dir, float length)
{
	SpriteColor color;
	color.color = 0x00000000;
	color.a = (unsigned char) std::max(0, std::min(255, int(255.0f * intensity)));

	IRender &render = _render;
	MyVertex *v = render.DrawFan((SINTABLE_SIZE>>2)+4);
	v[0].color = color;
	v[0].x = pos.x;
	v[0].y = pos.y;
	for( int i = 0; i <= SINTABLE_SIZE>>2; i++ )
	{
		float x = radius * sintable[i<<1];
		float y = radius * sintable[(i<<1)+(SINTABLE_SIZE>>2) & SINTABLE_MASK];
		v[i+1].x = pos.x - x*dir.x - y*dir.y;
		v[i+1].y = pos.y - x*dir.y + y*dir.x;
		v[i+1].color.color = 0x00000000;
	}

	v[(SINTABLE_SIZE>>2)+2].color.color = 0x00000000;
	v[(SINTABLE_SIZE>>2)+2].x = pos.x + length * dir.x + radius*dir.y;
	v[(SINTABLE_SIZE>>2)+2].y = pos.y + length * dir.y - radius*dir.x;

	v[(SINTABLE_SIZE>>2)+3].color = color;
	v[(SINTABLE_SIZE>>2)+3].x = pos.x + length * dir.x;
	v[(SINTABLE_SIZE>>2)+3].y = pos.y + length * dir.y;

	v[(SINTABLE_SIZE>>2)+4].color.color = 0x00000000;
	v[(SINTABLE_SIZE>>2)+4].x = pos.x + length * dir.x - radius*dir.y;
	v[(SINTABLE_SIZE>>2)+4].y = pos.y + length * dir.y + radius*dir.x;

	v = render.DrawFan((SINTABLE_SIZE>>2)+1);
	v[0].color = color;
	v[0].x = pos.x + length * dir.x;
	v[0].y = pos.y + length * dir.y;
	for( int i = 0; i <= SINTABLE_SIZE>>2; i++ )
	{
		float x = radius * sintable[i<<1] + length;
		float y = radius * sintable[(i<<1)+(SINTABLE_SIZE>>2) & SINTABLE_MASK];
		v[i+1].x = pos.x + x*dir.x - y*dir.y;
		v[i+1].y = pos.y + x*dir.y + y*dir.x;
		v[i+1].color.color = 0x00000000;
	}
}

void DrawingContext::Camera(RectRB viewport, float x, float y, float scale)
{
	viewport.left += (int) _transformStack.top().offset.x;
	viewport.top += (int) _transformStack.top().offset.y;
	viewport.right += (int) _transformStack.top().offset.x;
	viewport.bottom += (int) _transformStack.top().offset.y;
	_render.Camera(&viewport, x, y, scale);
}

void DrawingContext::SetAmbient(float ambient)
{
	_render.SetAmbient(ambient);
}

void DrawingContext::SetMode(RenderMode mode)
{
	if (_mode != mode)
	{
		_render.SetMode(mode);
		_mode = mode;
	}
}
