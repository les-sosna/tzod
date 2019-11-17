#include "inc/video/RenderContext.h"
#include "inc/video/TextureManager.h"
#include <algorithm>

RenderContext::RenderContext(const TextureManager &tm, IRender &render, unsigned int width, unsigned int height)
	: _tm(tm)
	, _render(render)
	, _currentTransform{ vec2d{}, 1, 0xff }
	, _mode(RM_UNDEFINED)
{
	_transformStack.push(_currentTransform);
	_clipStack.push(RectRB{ 0, 0, (int)width, (int)height });
}

void RenderContext::PushClippingRect(RectRB rect)
{
	assert(rect.right >= rect.left && rect.bottom >= rect.top);

	rect.left += (int)_currentTransform.offset.x;
	rect.top += (int)_currentTransform.offset.y;
	rect.right += (int)_currentTransform.offset.x;
	rect.bottom += (int)_currentTransform.offset.y;

	RectRB newClip = RectClamp(_clipStack.top(), rect);
	assert(newClip.right >= newClip.left && newClip.bottom >= newClip.top);

	_clipStack.push(newClip);
	_render.SetScissor(newClip);
}

void RenderContext::PopClippingRect()
{
	_clipStack.pop();
	_render.SetScissor(_clipStack.top());
}

void RenderContext::PushTransform(vec2d offset, float opacityCombined)
{
	assert(!_currentTransform.hardware);
	_transformStack.push({ _currentTransform.offset + offset, 1, static_cast<uint32_t>(opacityCombined * 255 + .5f), false /*hardware*/ });
	_currentTransform = _transformStack.top();
}

void RenderContext::PushWorldTransform(vec2d offset, float scale)
{
	_transformStack.push({ _currentTransform.offset + offset, _currentTransform.scale * scale, _currentTransform.opacity, true /*hardware*/ });
	_currentTransform = _transformStack.top();
	_render.SetTransform(_currentTransform.offset, _currentTransform.scale);
}

void RenderContext::PopTransform()
{
	assert(_transformStack.size() > 1);
	bool wasHardware = _currentTransform.hardware;
	_transformStack.pop();
	_currentTransform = _transformStack.top();
	if (_currentTransform.hardware)
	{
		_render.SetTransform(_currentTransform.offset, _currentTransform.scale);
	}
	else if (wasHardware)
	{
		_render.SetTransform({}, 1);
	}
}

FRECT RenderContext::GetVisibleRegion() const
{
	FRECT visibleRegion = RectOffset(RectToFRect(_clipStack.top()), -_currentTransform.offset);
	return visibleRegion / _currentTransform.scale;
}

static SpriteColor ApplyOpacity(SpriteColor color, uint8_t opacity)
{
	auto colorAG = (((color.color & 0xff00ff00) >> 8) * opacity) & 0xff00ff00;
	auto colorBR = (((color.color & 0x00ff00ff) * opacity) >> 8) & 0x00ff00ff;
	return colorAG | colorBR;
}

void RenderContext::DrawSprite(FRECT dst, size_t sprite, SpriteColor color, unsigned int frame)
{
	color = ApplyOpacity(color, _currentTransform.opacity);

	if (color.a == 0)
		return;

	const LogicalTexture &lt = _tm.GetSpriteInfo(sprite);
	const FRECT &rt = lt.uvFrames[frame];

	MyVertex *v = _render.DrawQuad(_tm.GetDeviceTexture(sprite));

	if (!_currentTransform.hardware)
	{
		dst = RectOffset(dst, _currentTransform.offset);
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

void RenderContext::DrawBorder(FRECT dst, size_t sprite, SpriteColor color, unsigned int frame)
{
	color = ApplyOpacity(color, _currentTransform.opacity);

	if (color.a == 0)
		return;

	const LogicalTexture &lt = _tm.GetSpriteInfo(sprite);
	const DEV_TEXTURE &devtex = _tm.GetDeviceTexture(sprite);

	FRECT uvFrame = lt.uvFrames[frame];
	float uvFrameWidth = WIDTH(uvFrame);
	float uvFrameHeight = HEIGHT(uvFrame);

	const float uvBorderWidth = lt.pxBorderSize * uvFrameWidth / lt.pxFrameWidth;
	const float uvBorderHeight = lt.pxBorderSize * uvFrameHeight / lt.pxFrameHeight;

	if (!_currentTransform.hardware)
	{
		dst = RectOffset(dst, _currentTransform.offset);
	}

	MyVertex *v;
	IRender &render = _render;

	// left edge
	v = render.DrawQuad(devtex);
	v[0].color = color;
	v[0].u = uvFrame.left - uvBorderWidth;
	v[0].v = uvFrame.top;
	v[0].x = dst.left;
	v[0].y = dst.top + lt.pxBorderSize;
	v[1].color = color;
	v[1].u = uvFrame.left;
	v[1].v = uvFrame.top;
	v[1].x = dst.left + lt.pxBorderSize;
	v[1].y = dst.top + lt.pxBorderSize;
	v[2].color = color;
	v[2].u = uvFrame.left;
	v[2].v = uvFrame.bottom;
	v[2].x = dst.left + lt.pxBorderSize;
	v[2].y = dst.bottom - lt.pxBorderSize;
	v[3].color = color;
	v[3].u = uvFrame.left - uvBorderWidth;
	v[3].v = uvFrame.bottom;
	v[3].x = dst.left;
	v[3].y = dst.bottom - lt.pxBorderSize;

	// right edge
	v = render.DrawQuad(devtex);
	v[0].color = color;
	v[0].u = uvFrame.right;
	v[0].v = uvFrame.top;
	v[0].x = dst.right - lt.pxBorderSize;
	v[0].y = dst.top + lt.pxBorderSize;
	v[1].color = color;
	v[1].u = uvFrame.right + uvBorderWidth;
	v[1].v = uvFrame.top;
	v[1].x = dst.right;
	v[1].y = dst.top + lt.pxBorderSize;
	v[2].color = color;
	v[2].u = uvFrame.right + uvBorderWidth;
	v[2].v = uvFrame.bottom;
	v[2].x = dst.right;
	v[2].y = dst.bottom - lt.pxBorderSize;
	v[3].color = color;
	v[3].u = uvFrame.right;
	v[3].v = uvFrame.bottom;
	v[3].x = dst.right - lt.pxBorderSize;
	v[3].y = dst.bottom - lt.pxBorderSize;

	// top edge
	v = render.DrawQuad(devtex);
	v[0].color = color;
	v[0].u = uvFrame.left;
	v[0].v = uvFrame.top - uvBorderHeight;
	v[0].x = dst.left + lt.pxBorderSize;
	v[0].y = dst.top;
	v[1].color = color;
	v[1].u = uvFrame.right;
	v[1].v = uvFrame.top - uvBorderHeight;
	v[1].x = dst.right - lt.pxBorderSize;
	v[1].y = dst.top;
	v[2].color = color;
	v[2].u = uvFrame.right;
	v[2].v = uvFrame.top;
	v[2].x = dst.right - lt.pxBorderSize;
	v[2].y = dst.top + lt.pxBorderSize;
	v[3].color = color;
	v[3].u = uvFrame.left;
	v[3].v = uvFrame.top;
	v[3].x = dst.left + lt.pxBorderSize;
	v[3].y = dst.top + lt.pxBorderSize;

	// bottom edge
	v = render.DrawQuad(devtex);
	v[0].color = color;
	v[0].u = uvFrame.left;
	v[0].v = uvFrame.bottom;
	v[0].x = dst.left + lt.pxBorderSize;
	v[0].y = dst.bottom - lt.pxBorderSize;
	v[1].color = color;
	v[1].u = uvFrame.right;
	v[1].v = uvFrame.bottom;
	v[1].x = dst.right - lt.pxBorderSize;
	v[1].y = dst.bottom - lt.pxBorderSize;
	v[2].color = color;
	v[2].u = uvFrame.right;
	v[2].v = uvFrame.bottom + uvBorderHeight;
	v[2].x = dst.right - lt.pxBorderSize;
	v[2].y = dst.bottom;
	v[3].color = color;
	v[3].u = uvFrame.left;
	v[3].v = uvFrame.bottom + uvBorderHeight;
	v[3].x = dst.left + lt.pxBorderSize;
	v[3].y = dst.bottom;

	// left top corner
	v = render.DrawQuad(devtex);
	v[0].color = color;
	v[0].u = uvFrame.left - uvBorderWidth;
	v[0].v = uvFrame.top - uvBorderHeight;
	v[0].x = dst.left;
	v[0].y = dst.top;
	v[1].color = color;
	v[1].u = uvFrame.left;
	v[1].v = uvFrame.top - uvBorderHeight;
	v[1].x = dst.left + lt.pxBorderSize;
	v[1].y = dst.top;
	v[2].color = color;
	v[2].u = uvFrame.left;
	v[2].v = uvFrame.top;
	v[2].x = dst.left + lt.pxBorderSize;
	v[2].y = dst.top + lt.pxBorderSize;
	v[3].color = color;
	v[3].u = uvFrame.left - uvBorderWidth;
	v[3].v = uvFrame.top;
	v[3].x = dst.left;
	v[3].y = dst.top + lt.pxBorderSize;

	// right top corner
	v = render.DrawQuad(devtex);
	v[0].color = color;
	v[0].u = uvFrame.right;
	v[0].v = uvFrame.top - uvBorderHeight;
	v[0].x = dst.right - lt.pxBorderSize;
	v[0].y = dst.top;
	v[1].color = color;
	v[1].u = uvFrame.right + uvBorderWidth;
	v[1].v = uvFrame.top - uvBorderHeight;
	v[1].x = dst.right;
	v[1].y = dst.top;
	v[2].color = color;
	v[2].u = uvFrame.right + uvBorderWidth;
	v[2].v = uvFrame.top;
	v[2].x = dst.right;
	v[2].y = dst.top + lt.pxBorderSize;
	v[3].color = color;
	v[3].u = uvFrame.right;
	v[3].v = uvFrame.top;
	v[3].x = dst.right - lt.pxBorderSize;
	v[3].y = dst.top + lt.pxBorderSize;

	// right bottom corner
	v = render.DrawQuad(devtex);
	v[0].color = color;
	v[0].u = uvFrame.right;
	v[0].v = uvFrame.bottom;
	v[0].x = dst.right - lt.pxBorderSize;
	v[0].y = dst.bottom - lt.pxBorderSize;
	v[1].color = color;
	v[1].u = uvFrame.right + uvBorderWidth;
	v[1].v = uvFrame.bottom;
	v[1].x = dst.right;
	v[1].y = dst.bottom - lt.pxBorderSize;
	v[2].color = color;
	v[2].u = uvFrame.right + uvBorderWidth;
	v[2].v = uvFrame.bottom + uvBorderHeight;
	v[2].x = dst.right;
	v[2].y = dst.bottom;
	v[3].color = color;
	v[3].u = uvFrame.right;
	v[3].v = uvFrame.bottom + uvBorderHeight;
	v[3].x = dst.right - lt.pxBorderSize;
	v[3].y = dst.bottom;

	// left bottom corner
	v = render.DrawQuad(devtex);
	v[0].color = color;
	v[0].u = uvFrame.left - uvBorderWidth;
	v[0].v = uvFrame.bottom;
	v[0].x = dst.left;
	v[0].y = dst.bottom - lt.pxBorderSize;
	v[1].color = color;
	v[1].u = uvFrame.left;
	v[1].v = uvFrame.bottom;
	v[1].x = dst.left + lt.pxBorderSize;
	v[1].y = dst.bottom - lt.pxBorderSize;
	v[2].color = color;
	v[2].u = uvFrame.left;
	v[2].v = uvFrame.bottom + uvBorderHeight;
	v[2].x = dst.left + lt.pxBorderSize;
	v[2].y = dst.bottom;
	v[3].color = color;
	v[3].u = uvFrame.left - uvBorderWidth;
	v[3].v = uvFrame.bottom + uvBorderHeight;
	v[3].x = dst.left;
	v[3].y = dst.bottom;
}

void RenderContext::DrawBitmapText(vec2d origin, float scale, size_t tex, SpriteColor color, std::string_view str, enumAlignText align)
{
	DrawBitmapText(MakeRectWH(origin, {}), scale, tex, color, str, align);
}

void RenderContext::DrawBitmapText(FRECT rect, float scale, size_t tex, SpriteColor color, std::string_view str, enumAlignText align)
{
	color = ApplyOpacity(color, _currentTransform.opacity);

	if (color.a == 0)
		return;

	// grep enum enumAlignText LT CT RT LC CC RC LB CB RB
	static const float dx[] = { 0, 1, 2, 0, 1, 2, 0, 1, 2 };
	static const float dy[] = { 0, 0, 0, 1, 1, 1, 2, 2, 2 };

	int lineCount = 0;
	size_t maxline = 0;
	if( align )
	{
		size_t count = 0;
		for(auto tmp = str.cbegin(); tmp != str.cend(); )
		{
			++count;
			++tmp;
			if( str.cend() == tmp || '\n' == *tmp )
			{
				if( maxline < count )
					maxline = count;
				lineCount++;
				count = 0;
			}
		}
	}

	LogicalTexture lt = _tm.GetSpriteInfo(tex);
	IRender &render = _render;

	int count = 0;
	int line  = 0;

	vec2d pxCharSize = Vec2dFloor(vec2d{ lt.pxFrameWidth, lt.pxFrameHeight } * scale);
	float pxAdvance = std::floor((lt.pxFrameWidth - 1) * scale);

	vec2d pxTextSize = { pxAdvance * (float)maxline, pxCharSize.y * (float)lineCount };

	float x0 = rect.left + std::floor(dx[align] * (WIDTH(rect) - pxTextSize.x) / 2);
	float y0 = rect.top + std::floor(dy[align] * (HEIGHT(rect) - pxTextSize.y) / 2);

	if (!_currentTransform.hardware)
	{
		x0 += _currentTransform.offset.x;
		y0 += _currentTransform.offset.y;
	}

	for(auto tmp = str.cbegin(); tmp != str.cend(); ++tmp )
	{
		if( '\n' == *tmp )
		{
			++line;
			count = 0;
		}

		int frame = (int)(unsigned char)*tmp - lt.leadChar;
		if( frame < 0 || frame >= lt.uvFrames.size() )
		{
			continue;
		}

		const FRECT &rt = lt.uvFrames[frame];
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

void RenderContext::DrawSprite(size_t tex, unsigned int frame, SpriteColor color, vec2d pos, vec2d dir)
{
	color = ApplyOpacity(color, _currentTransform.opacity);

	if (color.a == 0)
		return;

	assert(frame < _tm.GetFrameCount(tex));
	const LogicalTexture &lt = _tm.GetSpriteInfo(tex);
	const FRECT &rt = lt.uvFrames[frame];
	IRender &render = _render;

	MyVertex *v = render.DrawQuad(_tm.GetDeviceTexture(tex));

	if (!_currentTransform.hardware)
	{
		pos += _currentTransform.offset;
	}

	float width = lt.pxFrameWidth;
	float height = lt.pxFrameHeight;

	float px = lt.pxPivot.x;
	float py = lt.pxPivot.y;

	v[0].color = color;
	v[0].u = rt.left;
	v[0].v = rt.top;
	v[0].x = pos.x - px * dir.x + py * dir.y;
	v[0].y = pos.y - px * dir.y - py * dir.x;

	v[1].color = color;
	v[1].u = rt.right;
	v[1].v = rt.top;
	v[1].x = pos.x + (width - px) * dir.x + py * dir.y;
	v[1].y = pos.y + (width - px) * dir.y - py * dir.x;

	v[2].color = color;
	v[2].u = rt.right;
	v[2].v = rt.bottom;
	v[2].x = pos.x + (width - px) * dir.x - (height - py) * dir.y;
	v[2].y = pos.y + (width - px) * dir.y + (height - py) * dir.x;

	v[3].color = color;
	v[3].u = rt.left;
	v[3].v = rt.bottom;
	v[3].x = pos.x - px * dir.x - (height - py) * dir.y;
	v[3].y = pos.y - px * dir.y + (height - py) * dir.x;
}

void RenderContext::DrawSprite(size_t tex, unsigned int frame, SpriteColor color, vec2d pos, float width, float height, vec2d dir)
{
	color = ApplyOpacity(color, _currentTransform.opacity);

	if (color.a == 0)
		return;

	const LogicalTexture &lt = _tm.GetSpriteInfo(tex);
	const FRECT &rt = lt.uvFrames[frame];

	MyVertex *v = _render.DrawQuad(_tm.GetDeviceTexture(tex));

	if (!_currentTransform.hardware)
	{
		pos += _currentTransform.offset;
	}

	float px = lt.pxPivot.x * width / lt.pxFrameWidth;
	float py = lt.pxPivot.y * height / lt.pxFrameHeight;

	v[0].color = color;
	v[0].u = rt.left;
	v[0].v = rt.top;
	v[0].x = pos.x - px * dir.x + py * dir.y;
	v[0].y = pos.y - px * dir.y - py * dir.x;

	v[1].color = color;
	v[1].u = rt.right;
	v[1].v = rt.top;
	v[1].x = pos.x + (width - px) * dir.x + py * dir.y;
	v[1].y = pos.y + (width - px) * dir.y - py * dir.x;

	v[2].color = color;
	v[2].u = rt.right;
	v[2].v = rt.bottom;
	v[2].x = pos.x + (width - px) * dir.x - (height - py) * dir.y;
	v[2].y = pos.y + (width - px) * dir.y + (height - py) * dir.x;

	v[3].color = color;
	v[3].u = rt.left;
	v[3].v = rt.bottom;
	v[3].x = pos.x - px * dir.x - (height - py) * dir.y;
	v[3].y = pos.y - px * dir.y + (height - py) * dir.x;
}

void RenderContext::DrawIndicator(size_t tex, vec2d pos, float value)
{
	SpriteColor color = ApplyOpacity(0xffffffff, _currentTransform.opacity);

	const LogicalTexture &lt = _tm.GetSpriteInfo(tex);
	const FRECT &rt = lt.uvFrames[0];
	IRender &render = _render;

	float px = lt.pxPivot.x;
	float py = lt.pxPivot.y;

	MyVertex *v = render.DrawQuad(_tm.GetDeviceTexture(tex));

	v[0].color = color;
	v[0].u = rt.left;
	v[0].v = rt.top;
	v[0].x = pos.x - px;
	v[0].y = pos.y - py;

	v[1].color = color;
	v[1].u = rt.left + WIDTH(rt) * value;
	v[1].v = rt.top;
	v[1].x = pos.x - px + lt.pxFrameWidth * value;
	v[1].y = pos.y - py;

	v[2].color = color;
	v[2].u = rt.left + WIDTH(rt) * value;
	v[2].v = rt.bottom;
	v[2].x = pos.x - px + lt.pxFrameWidth * value;
	v[2].y = pos.y - py + lt.pxFrameHeight;

	v[3].color = color;
	v[3].u = rt.left;
	v[3].v = rt.bottom;
	v[3].x = pos.x - px;
	v[3].y = pos.y - py + lt.pxFrameHeight;
}

void RenderContext::DrawLine(size_t tex, SpriteColor color, vec2d begin, vec2d end, float phase)
{
	color = ApplyOpacity(color, _currentTransform.opacity);

	if (color.a == 0)
		return;

	const LogicalTexture &lt = _tm.GetSpriteInfo(tex);
	IRender &render = _render;

	MyVertex *v = render.DrawQuad(_tm.GetDeviceTexture(tex));

	float len = (begin - end).len();
	float phase1 = phase + len / lt.pxFrameWidth;
	float c = (end.x - begin.x) / len;
	float s = (end.y - begin.y) / len;
	float py = lt.pxFrameHeight / 2;

	v[0].color = color;
	v[0].u = phase;
	v[0].v = 0;
	v[0].x = begin.x + py * s;
	v[0].y = begin.y - py * c;

	v[1].color = color;
	v[1].u = phase1;
	v[1].v = 0;
	v[1].x = begin.x + len * c + py * s;
	v[1].y = begin.y + len * s - py * c;

	v[2].color = color;
	v[2].u = phase1;
	v[2].v = 1;
	v[2].x = begin.x + len * c - py * s;
	v[2].y = begin.y + len * s + py * c;

	v[3].color = color;
	v[3].u = phase;
	v[3].v = 1;
	v[3].x = begin.x - py * s;
	v[3].y = begin.y + py * c;
}

void RenderContext::DrawBackground(size_t tex, FRECT bounds) const
{
	SpriteColor color = ApplyOpacity(0xffffffff, _currentTransform.opacity);

	const LogicalTexture &lt = _tm.GetSpriteInfo(tex);
	IRender &render = _render;
	MyVertex *v = render.DrawQuad(_tm.GetDeviceTexture(tex));
	v[0].color = color;
	v[0].u = bounds.left / lt.pxFrameWidth;
	v[0].v = bounds.top / lt.pxFrameHeight;
	v[0].x = bounds.left;
	v[0].y = bounds.top;
	v[1].color = color;
	v[1].u = bounds.right / lt.pxFrameWidth;
	v[1].v = bounds.top / lt.pxFrameHeight;
	v[1].x = bounds.right;
	v[1].y = bounds.top;
	v[2].color = color;
	v[2].u = bounds.right / lt.pxFrameWidth;
	v[2].v = bounds.bottom / lt.pxFrameHeight;
	v[2].x = bounds.right;
	v[2].y = bounds.bottom;
	v[3].color = color;
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

void RenderContext::DrawPointLight(float intensity, float radius, vec2d pos)
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

void RenderContext::DrawSpotLight(float intensity, float radius, vec2d pos, vec2d dir, float offset, float aspect)
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

void RenderContext::DrawDirectLight(float intensity, float radius, vec2d pos, vec2d dir, float length)
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

void RenderContext::SetAmbient(float ambient)
{
	_render.SetAmbient(ambient);
}

void RenderContext::SetMode(RenderMode mode)
{
	if (_mode != mode)
	{
		_render.SetMode(mode);
		_mode = mode;
	}
}
