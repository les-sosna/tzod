#include "inc/editor/Config.h"
#include "inc/editor/EditorWorldView.h"
#include "PropertyList.h"
#include <ctx/EditorContext.h>
#include <gc/Object.h>
#include <gc/Pickup.h>
#include <gc/RigidBody.h>
#include <gc/TypeSystem.h>
#include <gc/WorldCfg.h>
#include <gv/ThemeManager.h>
#include <plat/ConsoleBuffer.h>
#include <plat/Input.h>
#include <plat/Keys.h>
#include <render/WorldView.h>
#include <render/RenderScheme.h>
#include <ui/Combo.h>
#include <ui/DataSource.h>
#include <ui/GuiManager.h>
#include <ui/InputContext.h>
#include <ui/Text.h>
#include <ui/LayoutContext.h>
#include <ui/ScrollView.h>
#include <ui/StateContext.h>
#include <video/TextureManager.h>

///////////////////////////////////////////////////////////////////////////////

static bool PtInRigidBody(const GC_RigidBodyStatic &rbs, vec2d delta)
{
	if (fabs(Vec2dDot(delta, rbs.GetDirection())) > rbs.GetHalfLength())
		return false;
	if (fabs(Vec2dCross(delta, rbs.GetDirection())) > rbs.GetHalfWidth())
		return false;
	return true;
}

static bool PtInObject(const GC_MovingObject &mo, vec2d pt)
{
	vec2d delta = pt - mo.GetPos();
	if (auto rbs = dynamic_cast<const GC_RigidBodyStatic*>(&mo))
		return PtInRigidBody(*rbs, delta);
	vec2d halfSize = RTTypes::Inst().GetTypeInfo(mo.GetType()).size / 2;
	return PtOnFRect(MakeRectRB(-halfSize, halfSize), delta);
}

GC_MovingObject* EditorWorldView::PickEdObject(const RenderScheme &rs, World &world, const vec2d &pt) const
{
	int layer = -1;
	if (_conf.uselayers.Get())
	{
		layer = RTTypes::Inst().GetTypeInfo(_currentType).layer;
	}

	GC_MovingObject* zLayers[Z_COUNT];
	memset(zLayers, 0, sizeof(zLayers));

	std::vector<ObjectList*> receive;
	world.grid_moving.OverlapPoint(receive, pt / WORLD_LOCATION_SIZE);
	for( auto rit = receive.begin(); rit != receive.end(); ++rit )
	{
		ObjectList *ls = *rit;
		for( auto it = ls->begin(); it != ls->end(); it = ls->next(it) )
		{
			auto mo = static_cast<GC_MovingObject*>(ls->at(it));
			if (RTTypes::Inst().IsRegistered(mo->GetType()) && PtInObject(*mo, pt))
			{
				enumZOrder maxZ = Z_NONE;
				for (auto &view: rs.GetViews(*mo, true, false))
				{
					maxZ = std::max(maxZ, view.zfunc->GetZ(world, *mo));
				}

				if( Z_NONE != maxZ )
				{
					for( unsigned int i = 0; i < RTTypes::Inst().GetTypeCount(); ++i )
					{
						if( mo->GetType() == RTTypes::Inst().GetTypeByIndex(i)
							&& (-1 == layer || RTTypes::Inst().GetTypeInfoByIndex(i).layer == layer) )
						{
							zLayers[maxZ] = mo;
						}
					}
				}
			}
		}
	}

	for( int z = Z_COUNT; z--; )
	{
		if (zLayers[z])
			return zLayers[z];
	}

	return nullptr;
}

EditorWorldView::EditorWorldView(UI::TimeStepManager &manager,
                                 TextureManager &texman,
                                 EditorContext &editorContext,
                                 WorldView &worldView,
                                 EditorConfig &conf,
                                 LangCache &lang,
                                 Plat::ConsoleBuffer &logger)
	: UI::TimeStepping(manager)
	, _conf(conf)
	, _virtualPointer(Center(editorContext.GetOriginalBounds()))
	, _defaultCamera(_virtualPointer)
	, _world(editorContext.GetWorld())
	, _worldView(worldView)
	, _quickActions(logger, _world)
{
	_propList = std::make_shared<PropertyList>(texman, editorContext.GetWorld(), conf, logger, lang);
	_propList->SetVisible(false);
	AddFront(_propList);

	SetTimeStep(true);
}

EditorWorldView::~EditorWorldView()
{
}

void EditorWorldView::Select(GC_Object *object, bool bSelect)
{
	assert(object);

	if( bSelect )
	{
		if( _selectedObject != object )
		{
			if( _selectedObject )
			{
				Select(_selectedObject, false);
			}

			_selectedObject = object;
			_propList->ConnectTo(_selectedObject->GetProperties(_world));
			_propList->SetVisible(true);
			SetFocus(_propList);
		}
	}
	else
	{
		assert(object == _selectedObject);
		_selectedObject = nullptr;

		_propList->ConnectTo(nullptr);
		_propList->SetVisible(false);
		SetFocus(nullptr);
	}
}

void EditorWorldView::SelectNone()
{
	if( _selectedObject )
	{
		Select(_selectedObject, false);
	}
}

void EditorWorldView::EraseAt(vec2d worldPos)
{
	while (GC_Object *object = PickEdObject(_worldView.GetRenderScheme(), _world, worldPos))
	{
		if (_selectedObject == object)
		{
			Select(object, false);
		}
		object->Kill(_world);
	}
}

static vec2d AlignToGrid(const RTTypes::EdItem &typeInfo, vec2d worldPos)
{
	vec2d offset = vec2d{ typeInfo.offset, typeInfo.offset };
	vec2d halfAlign = vec2d{ typeInfo.align, typeInfo.align } / 2;
	return Vec2dFloor((worldPos + halfAlign - offset) / typeInfo.align) * typeInfo.align + offset;
}

void EditorWorldView::CreateAt(vec2d worldPos, bool defaultProperties)
{
	auto &typeInfo = RTTypes::Inst().GetTypeInfo(_currentType);
	vec2d alignedPos = AlignToGrid(typeInfo, worldPos);
	if (CanCreateAt(alignedPos))
	{
		// create object
		GC_MovingObject &newobj = RTTypes::Inst().CreateObject(_world, _currentType, alignedPos);
		std::shared_ptr<PropertySet> properties = newobj.GetProperties(_world);

		if (!defaultProperties)
		{
			LoadFromConfig(_conf, *properties);
			properties->Exchange(_world, true);
		}

		_recentlyCreatedObject = &newobj;
	}
}

void EditorWorldView::ActionOrCreateAt(vec2d worldPos, bool defaultProperties)
{
	if( GC_MovingObject *mo = PickEdObject(_worldView.GetRenderScheme(), _world, worldPos) )
	{
		_quickActions.DoAction(*mo);

		if( _recentlyCreatedObject == mo )
		{
			SaveToConfig(_conf, *mo->GetProperties(_world));
		}
		else
		{
			_recentlyCreatedObject = nullptr;
		}
	}
	else
	{
		CreateAt(worldPos, defaultProperties);
	}
}

void EditorWorldView::ActionOrSelectAt(vec2d worldPos)
{
	if (GC_Object *object = PickEdObject(_worldView.GetRenderScheme(), _world, worldPos))
	{
		if (_selectedObject == object)
		{
			_quickActions.DoAction(*object);
			_propList->DoExchange(false);
		}
		else
		{
			Select(object, true);
		}
	}
	else
	{
		SelectNone();
	}
}

bool EditorWorldView::CanCreateAt(vec2d worldPos) const
{
	return PtInFRect(_world.GetBounds(), worldPos) &&
		!PickEdObject(_worldView.GetRenderScheme(), _world, worldPos);
}

static FRECT GetSelectionRect(const GC_MovingObject &mo)
{
	vec2d halfSize = RTTypes::Inst().GetTypeInfo(mo.GetType()).size / 2;
	return MakeRectRB(mo.GetPos() - halfSize, mo.GetPos() + halfSize);
}

FRECT EditorWorldView::GetNavigationOrigin() const
{
	if (GC_MovingObject *mo = PickEdObject(_worldView.GetRenderScheme(), _world, _virtualPointer))
	{
		return GetSelectionRect(*mo);
	}
	else
	{
		auto &typeInfo = RTTypes::Inst().GetTypeInfo(_currentType);
		vec2d alignedPos = AlignToGrid(typeInfo, _virtualPointer);
		return MakeRectWH(alignedPos, vec2d{});
	}
}

EditorWorldView::WorldCursor EditorWorldView::GetCursor() const
{
	WorldCursor cursor = {};

	vec2d worldPos = _virtualPointer;

	if (GC_MovingObject *mo = PickEdObject(_worldView.GetRenderScheme(), _world, worldPos))
	{
		cursor.bounds = GetSelectionRect(*mo);
		cursor.cursorType = WorldCursor::Type::Action;
	}
	else if (!_selectOnly)
	{
		auto &typeInfo = RTTypes::Inst().GetTypeInfo(_currentType);
		vec2d mouseAligned = AlignToGrid(typeInfo, worldPos);
		cursor.bounds = MakeRectWH(mouseAligned - typeInfo.size / 2, typeInfo.size);
		cursor.cursorType = CanCreateAt(mouseAligned) ? WorldCursor::Type::Create : WorldCursor::Type::Obstructed;
	}

	return cursor;
}

void EditorWorldView::EnsureVisible(const UI::LayoutContext &lc, FRECT worldRect)
{
	vec2d cameraOffset = {};

	auto canvasViewport = MakeRectWH(lc.GetPixelSize());
	vec2d eye = _defaultCamera.GetEye();
	float zoom = _defaultCamera.GetZoom() * lc.GetScale();
	vec2d worldTransformOffset = ComputeWorldTransformOffset(canvasViewport, eye, zoom);
	FRECT worldViewport = CanvasToWorld(worldTransformOffset, zoom, canvasViewport);

	if (worldRect.left < worldViewport.left)
		cameraOffset.x = worldRect.left - worldViewport.left;
	else if (worldRect.right > worldViewport.right)
		cameraOffset.x = worldRect.right - worldViewport.right;

	if (worldRect.top < worldViewport.top)
		cameraOffset.y = worldRect.top - worldViewport.top;
	else if (worldRect.bottom > worldViewport.bottom)
		cameraOffset.y = worldRect.bottom - worldViewport.bottom;

	if (!cameraOffset.IsZero())
		_defaultCamera.MoveTo(_defaultCamera.GetEye() + cameraOffset);
}

void EditorWorldView::OnTimeStep(const UI::InputContext &ic, float dt)
{
	_defaultCamera.HandleMovement(ic.GetInput(), _world.GetBounds(), dt);

	// Workaround: we do not get notifications when the object is killed
	if (!_selectedObject)
	{
		_propList->ConnectTo(nullptr);
		_propList->SetVisible(false);
	}
}

void EditorWorldView::OnScroll(TextureManager &texman, const UI::InputContext &ic, const UI::LayoutContext &lc, const UI::DataContext &dc, vec2d scrollOffset, bool precise)
{
	if (precise)
	{
		_defaultCamera.Move(scrollOffset, _world.GetBounds());
	}
	else
	{
		if (scrollOffset.y > 0)
		{
			_defaultCamera.ZoomIn();
		}
		else
		{
			_defaultCamera.ZoomOut();
		}
	}
}

void EditorWorldView::OnPointerMove(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, bool captured)
{
	_virtualPointer = CanvasToWorld(lc, pi.position);

	if (_capturedButton)
	{
		vec2d worldPos = CanvasToWorld(lc, pi.position);
		switch (_capturedButton)
		{
		case 1:
			// keep default properties if Ctrl key is not pressed
			CreateAt(worldPos, !ic.GetInput().IsKeyPressed(Plat::Key::LeftCtrl) && !ic.GetInput().IsKeyPressed(Plat::Key::RightCtrl));
			break;

		case 2:
			EraseAt(worldPos);
			break;

		case 4:
			_defaultCamera.Move((pi.position - _prevPointerPosition) / _defaultCamera.GetZoom() / lc.GetScale(), _world.GetBounds());
			_prevPointerPosition = pi.position;
			break;
		}
	}
}

void EditorWorldView::OnPointerUp(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, int button)
{
	if (_capturedButton == button )
	{
		_capturedButton = 0;
	}
}

bool EditorWorldView::OnPointerDown(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, int button)
{
	if (pi.type == Plat::PointerType::Touch)
		return false; // ignore touch here to not conflict with scroll. handle tap instead

	bool capture = false;
	if( 0 == _capturedButton )
	{
		capture = true;
		_capturedButton = button;
	}

	// do not react on other buttons once we captured one
	if( _capturedButton != button )
	{
		return capture;
	}

	SetFocus(nullptr);

	vec2d worldPos = CanvasToWorld(lc, pi.position);
	if (2 == button)
	{
		EraseAt(worldPos);
	}
	else if (1 == button)
	{
		if (_selectOnly)
		{
			ActionOrSelectAt(worldPos);
		}
		else
		{
			// keep default properties if Ctrl key is not pressed
			bool defaultProperties = !ic.GetInput().IsKeyPressed(Plat::Key::LeftCtrl) && !ic.GetInput().IsKeyPressed(Plat::Key::RightCtrl);
			ActionOrCreateAt(worldPos, defaultProperties);
		}
	}
	else if (4 == button)
	{
		_prevPointerPosition = pi.position;
	}

	return capture;
}

void EditorWorldView::OnTap(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition)
{
	if (_selectOnly)
	{
		ActionOrSelectAt(CanvasToWorld(lc, pointerPosition));
	}
	else
	{
		ActionOrCreateAt(CanvasToWorld(lc, pointerPosition), true);
	}
}

bool EditorWorldView::OnKeyPressed(UI::InputContext &ic, Plat::Key key)
{
	switch(key)
	{
	case Plat::Key::GamepadRightTrigger:
	case Plat::Key::PageUp:
		_defaultCamera.ZoomIn();
		break;
	case Plat::Key::GamepadLeftTrigger:
	case Plat::Key::PageDown:
		_defaultCamera.ZoomOut();
		break;
	case Plat::Key::Delete:
	case Plat::Key::GamepadX:
		if( _selectedObject )
		{
			GC_Object *o = _selectedObject;
			Select(_selectedObject, false);
			o->Kill(_world);
		}
		else
		{
			vec2d alignedPointerPos = Center(GetNavigationOrigin());
			if (GC_MovingObject *mo = PickEdObject(_worldView.GetRenderScheme(), _world, alignedPointerPos))
			{
				mo->Kill(_world);
			}
		}
		break;
	case Plat::Key::GamepadY:
		ActionOrSelectAt(Center(GetNavigationOrigin()));
		break;
	case Plat::Key::G:
		_conf.drawgrid.Set(!_conf.drawgrid.Get());
		break;
	default:
		return false;
	}
	return true;
}

bool EditorWorldView::CanNavigate(UI::Navigate navigate, const UI::LayoutContext &lc, const UI::DataContext &dc) const
{
	switch (navigate)
	{
	case UI::Navigate::Back:
		return !!_selectedObject;
	case UI::Navigate::Enter:
	case UI::Navigate::Up:
	case UI::Navigate::Down:
	case UI::Navigate::Left:
	case UI::Navigate::Right:
		return true;

	default:
		return false;
	}
}

void EditorWorldView::OnNavigate(UI::Navigate navigate, UI::NavigationPhase phase, const UI::LayoutContext &lc, const UI::DataContext &dc)
{
	if (phase != UI::NavigationPhase::Started)
	{
		return;
	}

	auto &typeInfo = RTTypes::Inst().GetTypeInfo(_currentType);

	FRECT origin = GetNavigationOrigin();

	switch (navigate)
	{
	case UI::Navigate::Enter:
		ActionOrCreateAt(_virtualPointer, true);
		break;
	case UI::Navigate::Back:
		if (_selectedObject)
		{
			Select(_selectedObject, false);
		}
		break;
	case UI::Navigate::Up:
		if (Size(origin).IsZero())
			_virtualPointer.y = origin.top - typeInfo.align + 1;
		else
			_virtualPointer.y = origin.top - typeInfo.size.y / 2 + 1;
		break;
	case UI::Navigate::Down:
		if (Size(origin).IsZero())
			_virtualPointer.y = origin.bottom + typeInfo.align - 1;
		else
			_virtualPointer.y = origin.bottom + typeInfo.size.y / 2 - 1;
		break;
	case UI::Navigate::Left:
		if (Size(origin).IsZero())
			_virtualPointer.x = origin.left - typeInfo.align + 1;
		else
			_virtualPointer.x = origin.left - typeInfo.size.x / 2 + 1;
		break;
	case UI::Navigate::Right:
		if (Size(origin).IsZero())
			_virtualPointer.x = origin.right + typeInfo.align - 1;
		else
			_virtualPointer.x = origin.right + typeInfo.size.x / 2 - 1;
		break;
	default:
		break;
	}

	EnsureVisible(lc, RectExpand(GetCursor().bounds, WORLD_BLOCK_SIZE));
}

FRECT EditorWorldView::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const
{
	if (_propList.get() == &child)
	{
		float pxWidth = std::floor(100 * lc.GetScale());
		float pxRight = lc.GetPixelSize().x;
		return FRECT{ pxRight - pxWidth, 0, pxRight, lc.GetPixelSize().y };
	}
	return UI::Window::GetChildRect(texman, lc, dc, child);
}

void EditorWorldView::Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const
{
	// World
	RectRB viewport{ 0, 0, (int)lc.GetPixelSize().x, (int)lc.GetPixelSize().y };
	vec2d eye = _defaultCamera.GetEye();
	float zoom = _defaultCamera.GetZoom() * lc.GetScale();
	vec2d worldTransformOffset = ComputeWorldTransformOffset(MakeRectWH(lc.GetPixelSize()), eye, zoom);

	rc.PushClippingRect(viewport);
	rc.PushWorldTransform(worldTransformOffset, zoom);
	WorldViewRenderOptions options;
	options.editorMode = true;
	options.drawGrid = _conf.drawgrid.Get();
	options.nightMode = _world.GetNightMode();
	options.visualizeField = false;
	_worldView.Render(rc, _world, options);

	// Selection
	if( auto selectedObject = PtrDynCast<const GC_MovingObject>(_selectedObject) )
	{
		FRECT sel = GetSelectionRect(*selectedObject);

		rc.DrawSprite(sel, _texSelection.GetTextureId(texman), 0xffffffff, 0);
		rc.DrawBorder(sel, _texSelection.GetTextureId(texman), 0xffffffff, 0);
	}

	// World cursor
	if (ic.GetFocused())
	{
		auto cursor = GetCursor();
		auto cursorColor = SpriteColor{};
		switch (cursor.cursorType)
		{
			case WorldCursor::Type::Create:     cursorColor = 0xff00ff00; break;
			case WorldCursor::Type::Obstructed: cursorColor = 0x8f008f00; break;
			case WorldCursor::Type::Action:     cursorColor = 0xff00ffff; break;
			case WorldCursor::Type::None: break;
		}
		rc.DrawBorder(cursor.bounds, _texSelection.GetTextureId(texman), cursorColor, 0);

		if (cursor.cursorType == WorldCursor::Type::Obstructed)
		{
			if (GC_MovingObject *mo = PickEdObject(_worldView.GetRenderScheme(), _world, Center(cursor.bounds)))
			{
				auto obstacleRect = GetSelectionRect(*mo);
				rc.DrawSprite(obstacleRect, _texSelection.GetTextureId(texman), 0xff0000ff, 0);
				rc.DrawBorder(obstacleRect, _texSelection.GetTextureId(texman), 0x88000088, 0);
			}
		}
	}

#ifndef NDEBUG
	FRECT dst = { _virtualPointer.x - 2, _virtualPointer.y - 2, _virtualPointer.x + 2, _virtualPointer.y + 2 };
	rc.DrawSprite(dst, 0U, 0xffffffff, 0U);
#endif

	rc.PopTransform();
	rc.PopClippingRect();

	// Mouse coordinates
	vec2d worldPos = Center(GetNavigationOrigin());
	std::stringstream buf;
	buf << "x=" << floor(worldPos.x + 0.5f) << "; y=" << floor(worldPos.y + 0.5f);
	rc.DrawBitmapText(vec2d{ std::floor(lc.GetPixelSize().x / 2 + 0.5f), 1 },
		lc.GetScale(), _fontSmall.GetTextureId(texman), 0xffffffff, buf.str(), alignTextCT);
}

vec2d EditorWorldView::CanvasToWorld(const UI::LayoutContext &lc, vec2d canvasPos) const
{
	vec2d eye = _defaultCamera.GetEye();
	float zoom = _defaultCamera.GetZoom() * lc.GetScale();
	vec2d worldTransformOffset = ComputeWorldTransformOffset(MakeRectWH(lc.GetPixelSize()), eye, zoom);

	return (canvasPos - worldTransformOffset) / zoom;
}

FRECT EditorWorldView::CanvasToWorld(vec2d worldTransformOffset, float worldTransformScale, FRECT canvasRect) const
{
	return RectOffset(canvasRect, -worldTransformOffset) / worldTransformScale;
}

vec2d EditorWorldView::WorldToCanvas(vec2d worldTransformOffset, float worldTransformScale, vec2d worldPos) const
{
	return worldPos * worldTransformScale + worldTransformOffset;
}

FRECT EditorWorldView::WorldToCanvas(vec2d worldTransformOffset, float worldTransformScale, FRECT worldRect) const
{
	return RectOffset(worldRect * worldTransformScale, worldTransformOffset);
}
