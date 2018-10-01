#include "inc/editor/Editor.h"
#include "inc/editor/Config.h"
#include "PropertyList.h"
#include "GameClassVis.h"
#include <cbind/ConfigBinding.h>
#include <ctx/EditorContext.h>
#include <gc/Object.h>
#include <gc/Pickup.h>
#include <gc/RigidBody.h>
#include <gc/TypeSystem.h>
#include <gc/WorldCfg.h>
#include <gv/ThemeManager.h>
#include <loc/Language.h>
#include <plat/Input.h>
#include <plat/Keys.h>
#include <render/WorldView.h>
#include <render/RenderScheme.h>
#include <ui/Button.h>
#include <ui/Combo.h>
#include <ui/ConsoleBuffer.h>
#include <ui/DataSource.h>
#include <ui/GuiManager.h>
#include <ui/InputContext.h>
#include <ui/Text.h>
#include <ui/List.h>
#include <ui/ListBox.h>
#include <ui/DataSource.h>
#include <ui/DataSourceAdapters.h>
#include <ui/LayoutContext.h>
#include <ui/ScrollView.h>
#include <ui/StackLayout.h>
#include <ui/StateContext.h>
#include <video/TextureManager.h>

#include <sstream>


///////////////////////////////////////////////////////////////////////////////

static bool PtInRigidBody(const GC_RigidBodyStatic &rbs, vec2d delta)
{
	if (fabs(Vec2dDot(delta, rbs.GetDirection())) > rbs.GetHalfLength())
		return false;
	if (fabs(Vec2dCross(delta, rbs.GetDirection())) > rbs.GetHalfWidth())
		return false;
	return true;
}

static bool PtOnActor(const GC_Actor &actor, vec2d pt)
{
	vec2d delta = pt - actor.GetPos();
	if (auto rbs = dynamic_cast<const GC_RigidBodyStatic*>(&actor))
		return PtInRigidBody(*rbs, delta);
	vec2d halfSize = RTTypes::Inst().GetTypeInfo(actor.GetType()).size / 2;
	return PtOnFRect(MakeRectRB(-halfSize, halfSize), delta);
}

namespace
{
	class LayerDisplay final
		: public UI::LayoutData<std::string_view>
	{
	public:
		LayerDisplay(LangCache &lang, std::shared_ptr<UI::List> typeSelector)
			: _lang(lang)
			, _typeSelector(std::move(typeSelector))
		{}

		// LayoutData<std::string_view>
		std::string_view GetLayoutValue(const UI::DataContext &dc) const override
		{
			int index = _typeSelector->GetCurSel();
			if (_cachedIndex != index)
			{
				std::ostringstream oss;
				oss << _lang.layer.Get()
					<< RTTypes::Inst().GetTypeInfo(static_cast<ObjectType>(_typeSelector->GetData()->GetItemData(index))).layer
					<< ": ";
				_cachedString = oss.str();
				_cachedIndex = index;
			}
			return _cachedString;
		}

	private:
		LangCache &_lang;
		std::shared_ptr<UI::List> _typeSelector;
		mutable int _cachedIndex = -1;
		mutable std::string _cachedString;
	};
}

GC_Actor* EditorLayout::PickEdObject(const RenderScheme &rs, World &world, const vec2d &pt) const
{
	int layer = -1;
	if (_conf.uselayers.Get())
	{
		layer = RTTypes::Inst().GetTypeInfo(GetCurrentType()).layer;
	}

	GC_Actor* zLayers[Z_COUNT];
	memset(zLayers, 0, sizeof(zLayers));

	std::vector<ObjectList*> receive;
	world.grid_actors.OverlapPoint(receive, pt / WORLD_LOCATION_SIZE);
	for( auto rit = receive.begin(); rit != receive.end(); ++rit )
	{
		ObjectList *ls = *rit;
		for( auto it = ls->begin(); it != ls->end(); it = ls->next(it) )
		{
			auto actor = static_cast<GC_Actor*>(ls->at(it));
			if (RTTypes::Inst().IsRegistered(actor->GetType()) && PtOnActor(*actor, pt))
			{
				enumZOrder maxZ = Z_NONE;
				for (auto &view: rs.GetViews(*actor, true, false))
				{
					maxZ = std::max(maxZ, view.zfunc->GetZ(world, *actor));
				}

				if( Z_NONE != maxZ )
				{
					for( unsigned int i = 0; i < RTTypes::Inst().GetTypeCount(); ++i )
					{
						if( actor->GetType() == RTTypes::Inst().GetTypeByIndex(i)
							&& (-1 == layer || RTTypes::Inst().GetTypeInfoByIndex(i).layer == layer) )
						{
							zLayers[maxZ] = actor;
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


EditorLayout::EditorLayout(UI::TimeStepManager &manager,
                           TextureManager &texman,
                           EditorContext &editorContext,
                           WorldView &worldView,
                           EditorConfig &conf,
                           LangCache &lang,
                           EditorCommands commands,
                           UI::ConsoleBuffer &logger)
	: UI::TimeStepping(manager)
	, _conf(conf)
	, _lang(lang)
	, _commands(std::move(commands))
	, _virtualPointer(Center(editorContext.GetOriginalBounds()))
	, _defaultCamera(_virtualPointer)
	, _world(editorContext.GetWorld())
	, _worldView(worldView)
	, _quickActions(logger, _world)
{
	_help = std::make_shared<UI::Text>();
	_help->Move(10, 10);
	_help->SetText(ConfBind(_lang.f1_help_editor));
	_help->SetAlign(alignTextLT);
	_help->SetVisible(false);
	AddFront(_help);

	_propList = std::make_shared<PropertyList>(texman, _world, conf, logger, lang);
	_propList->SetVisible(false);
	AddFront(_propList);

	auto gameClassVis = std::make_shared<GameClassVis>(_worldView);
	gameClassVis->Resize(64, 64);
	gameClassVis->SetGameClass(std::make_shared<UI::ListDataSourceBinding>(0));

	using namespace UI::DataSourceAliases;

	_modeSelect = std::make_shared<UI::CheckBox>();
	_modeSelect->SetText("Select"_txt);

	_modeErase = std::make_shared<UI::CheckBox>();
	_modeErase->SetText("Erase"_txt);

	auto play = std::make_shared<UI::Button>();
	play->SetText("Play"_txt);
	play->SetWidth(64);
	play->eventClick = _commands.playMap;

	_typeSelector = std::make_shared<DefaultListBox>();
	_typeSelector->GetList()->SetItemTemplate(gameClassVis);

	for( unsigned int i = 0; i < RTTypes::Inst().GetTypeCount(); ++i )
	{
		if (!RTTypes::Inst().GetTypeInfoByIndex(i).service)
		{
			auto &typeInfo = RTTypes::Inst().GetTypeInfoByIndex(i);
			_typeSelector->GetData()->AddItem(typeInfo.name, RTTypes::Inst().GetTypeByIndex(i));
		}
	}
	_typeSelector->GetList()->SetCurSel(std::min(_typeSelector->GetData()->GetItemCount() - 1, std::max(0, _conf.object.GetInt())));

	_toolbar = std::make_shared<UI::StackLayout>();
	_toolbar->AddFront(play);
	_toolbar->AddFront(_modeSelect);
	_toolbar->AddFront(_modeErase);
	_toolbar->AddFront(_typeSelector);
	AddFront(_toolbar);

	_layerDisp = std::make_shared<UI::Text>();
	_layerDisp->SetAlign(alignTextRT);
	_layerDisp->SetText(std::make_shared<LayerDisplay>(_lang, _typeSelector->GetList()));
	AddFront(_layerDisp);

	assert(!_conf.uselayers.eventChange);
	_conf.uselayers.eventChange = std::bind(&EditorLayout::OnChangeUseLayers, this);
	OnChangeUseLayers();

	SetTimeStep(true);
}

EditorLayout::~EditorLayout()
{
	_conf.uselayers.eventChange = nullptr;
}

void EditorLayout::Select(GC_Object *object, bool bSelect)
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

void EditorLayout::SelectNone()
{
	if( _selectedObject )
	{
		Select(_selectedObject, false);
	}
}

void EditorLayout::EraseAt(vec2d worldPos)
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

void EditorLayout::CreateAt(vec2d worldPos, bool defaultProperties)
{
	auto &typeInfo = RTTypes::Inst().GetTypeInfo(GetCurrentType());
	vec2d alignedPos = AlignToGrid(typeInfo, worldPos);
	if (CanCreateAt(alignedPos))
	{
		// create object
		GC_Actor &newobj = RTTypes::Inst().CreateActor(_world, GetCurrentType(), alignedPos);
		std::shared_ptr<PropertySet> properties = newobj.GetProperties(_world);

		if (!defaultProperties)
		{
			LoadFromConfig(_conf, *properties);
			properties->Exchange(_world, true);
		}

		_recentlyCreatedObject = &newobj;
	}
}

void EditorLayout::ActionOrCreateAt(vec2d worldPos, bool defaultProperties)
{
	if( GC_Actor *actor = PickEdObject(_worldView.GetRenderScheme(), _world, worldPos) )
	{
		_quickActions.DoAction(*actor);

		if( _recentlyCreatedObject == actor )
		{
			SaveToConfig(_conf, *actor->GetProperties(_world));
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

void EditorLayout::ActionOrSelectAt(vec2d worldPos)
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

bool EditorLayout::CanCreateAt(vec2d worldPos) const
{
	return PtInFRect(_world.GetBounds(), worldPos) &&
		!PickEdObject(_worldView.GetRenderScheme(), _world, worldPos);
}

static FRECT GetSelectionRect(const GC_Actor &actor)
{
	vec2d halfSize = RTTypes::Inst().GetTypeInfo(actor.GetType()).size / 2;
	return MakeRectRB(actor.GetPos() - halfSize, actor.GetPos() + halfSize);
}

FRECT EditorLayout::GetNavigationOrigin() const
{
	if (GC_Actor *actor = PickEdObject(_worldView.GetRenderScheme(), _world, _virtualPointer))
	{
		return GetSelectionRect(*actor);
	}
	else
	{
		auto &typeInfo = RTTypes::Inst().GetTypeInfo(GetCurrentType());
		vec2d alignedPos = AlignToGrid(typeInfo, _virtualPointer);
		return MakeRectWH(alignedPos, vec2d{});
	}
}

EditorLayout::WorldCursor EditorLayout::GetCursor() const
{
	WorldCursor cursor = {};

	vec2d worldPos = _virtualPointer;// CanvasToWorld(lc, ic.GetMousePos());

	if (GC_Actor *actor = PickEdObject(_worldView.GetRenderScheme(), _world, worldPos))
	{
		cursor.bounds = GetSelectionRect(*actor);
		cursor.cursorType = WorldCursor::Type::Action;
	}
	else if (!_modeSelect->GetCheck())
	{
		auto &typeInfo = RTTypes::Inst().GetTypeInfo(GetCurrentType());
		vec2d mouseAligned = AlignToGrid(typeInfo, worldPos);
		cursor.bounds = MakeRectWH(mouseAligned - typeInfo.size / 2, typeInfo.size);
		cursor.cursorType = CanCreateAt(mouseAligned) ? WorldCursor::Type::Create : WorldCursor::Type::Obstructed;
	}

	return cursor;
}

void EditorLayout::ChooseNextType()
{
	_typeSelector->GetList()->SetCurSel(std::clamp(_typeSelector->GetList()->GetCurSel() + 1,
		0, _typeSelector->GetList()->GetData()->GetItemCount() - 1));
}

void EditorLayout::ChoosePrevType()
{
	_typeSelector->GetList()->SetCurSel(std::clamp(_typeSelector->GetList()->GetCurSel() - 1,
		0, _typeSelector->GetList()->GetData()->GetItemCount() - 1));
}

void EditorLayout::EnsureVisible(const UI::LayoutContext &lc, FRECT worldRect)
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

void EditorLayout::OnTimeStep(const UI::InputContext &ic, float dt)
{
	_defaultCamera.HandleMovement(ic.GetInput(), _world.GetBounds(), dt);

	// Workaround: we do not get notifications when the object is killed
	if (!_selectedObject)
	{
		_propList->ConnectTo(nullptr);
		_propList->SetVisible(false);
	}
}

void EditorLayout::OnScroll(TextureManager &texman, const UI::InputContext &ic, const UI::LayoutContext &lc, const UI::DataContext &dc, vec2d scrollOffset, bool precise)
{
	if (!precise)
	{
		scrollOffset *= WORLD_BLOCK_SIZE;
	}
	_defaultCamera.Move(scrollOffset, _world.GetBounds());
}

void EditorLayout::OnPointerMove(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, bool captured)
{
	_virtualPointer = CanvasToWorld(lc, pi.position);

	if (_capturedButton)
	{
		vec2d worldPos = CanvasToWorld(lc, pi.position);
		if (2 == _capturedButton)
		{
			EraseAt(worldPos);
		}
		else if (1 == _capturedButton)
		{
			// keep default properties if Ctrl key is not pressed
			bool defaultProperties = !ic.GetInput().IsKeyPressed(Plat::Key::LeftCtrl) && !ic.GetInput().IsKeyPressed(Plat::Key::RightCtrl);
			CreateAt(worldPos, defaultProperties);
		}
	}
}

void EditorLayout::OnPointerUp(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, int button)
{
	if (_capturedButton == button )
	{
		_capturedButton = 0;
	}
}

bool EditorLayout::OnPointerDown(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, int button)
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
		if (_modeSelect->GetCheck())
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

	return capture;
}

void EditorLayout::OnTap(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition)
{
	if (_modeSelect->GetCheck())
	{
		ActionOrSelectAt(CanvasToWorld(lc, pointerPosition));
	}
	else
	{
		ActionOrCreateAt(CanvasToWorld(lc, pointerPosition), true);
	}
}

bool EditorLayout::OnKeyPressed(UI::InputContext &ic, Plat::Key key)
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
			if (GC_Actor *actor = PickEdObject(_worldView.GetRenderScheme(), _world, alignedPointerPos))
			{
				actor->Kill(_world);
			}
		}
		break;
	case Plat::Key::GamepadY:
		ActionOrSelectAt(Center(GetNavigationOrigin()));
		break;
	case Plat::Key::F1:
		_help->SetVisible(!_help->GetVisible());
		break;
	case Plat::Key::F9:
		_conf.uselayers.Set(!_conf.uselayers.Get());
		break;
	case Plat::Key::G:
		_conf.drawgrid.Set(!_conf.drawgrid.Get());
		break;
	case Plat::Key::LeftBracket:
	case Plat::Key::GamepadLeftShoulder:
		ChoosePrevType();
		break;
	case Plat::Key::RightBracket:
	case Plat::Key::GamepadRightShoulder:
		ChooseNextType();
		break;
	default:
		return false;
	}
	return true;
}

bool EditorLayout::CanNavigate(UI::Navigate navigate, const UI::LayoutContext &lc, const UI::DataContext &dc) const
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

void EditorLayout::OnNavigate(UI::Navigate navigate, UI::NavigationPhase phase, const UI::LayoutContext &lc, const UI::DataContext &dc)
{
	if (phase != UI::NavigationPhase::Started)
	{
		return;
	}

	auto &typeInfo = RTTypes::Inst().GetTypeInfo(GetCurrentType());

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

FRECT EditorLayout::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const
{
	float scale = lc.GetScale();
	vec2d size = lc.GetPixelSize();

	if (_layerDisp.get() == &child)
	{
		return UI::CanvasLayout(vec2d{ size.x / scale - 5, 6 }, _layerDisp->GetSize(), scale);
	}
	if (_toolbar.get() == &child)
	{
		return FRECT{ size.x - _toolbar->GetContentSize(texman, dc, scale, DefaultLayoutConstraints(lc)).x, 0, size.x, size.y };
	}
	if (_propList.get() == &child)
	{
		float pxWidth = std::floor(100 * lc.GetScale());
		float pxRight = _toolbar ? GetChildRect(texman, lc, dc, *_toolbar).left : lc.GetPixelSize().x;
		return FRECT{ pxRight - pxWidth, 0, pxRight, lc.GetPixelSize().y };
	}

	return UI::Window::GetChildRect(texman, lc, dc, child);
}

void EditorLayout::OnChangeUseLayers()
{
	_layerDisp->SetVisible(_conf.uselayers.Get());
}

void EditorLayout::Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const
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
	if( auto selectedActor = PtrDynCast<const GC_Actor>(_selectedObject) )
	{
		FRECT sel = GetSelectionRect(*selectedActor);

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
			if (GC_Actor *actor = PickEdObject(_worldView.GetRenderScheme(), _world, Center(cursor.bounds)))
			{
				auto obstacleRect = GetSelectionRect(*actor);
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
	vec2d worldPos = Center(GetNavigationOrigin());// CanvasToWorld(lc, ic.GetMousePos());
	std::stringstream buf;
	buf << "x=" << floor(worldPos.x + 0.5f) << "; y=" << floor(worldPos.y + 0.5f);
	rc.DrawBitmapText(vec2d{ std::floor(lc.GetPixelSize().x / 2 + 0.5f), 1 },
		lc.GetScale(), _fontSmall.GetTextureId(texman), 0xffffffff, buf.str(), alignTextCT);
}

vec2d EditorLayout::CanvasToWorld(const UI::LayoutContext &lc, vec2d canvasPos) const
{
	vec2d eye = _defaultCamera.GetEye();
	float zoom = _defaultCamera.GetZoom() * lc.GetScale();
	vec2d worldTransformOffset = ComputeWorldTransformOffset(MakeRectWH(lc.GetPixelSize()), eye, zoom);

	return (canvasPos - worldTransformOffset) / zoom;
}

FRECT EditorLayout::CanvasToWorld(vec2d worldTransformOffset, float worldTransformScale, FRECT canvasRect) const
{
	return RectOffset(canvasRect, -worldTransformOffset) / worldTransformScale;
}

vec2d EditorLayout::WorldToCanvas(vec2d worldTransformOffset, float worldTransformScale, vec2d worldPos) const
{
	return worldPos * worldTransformScale + worldTransformOffset;
}

FRECT EditorLayout::WorldToCanvas(vec2d worldTransformOffset, float worldTransformScale, FRECT worldRect) const
{
	return RectOffset(worldRect * worldTransformScale, worldTransformOffset);
}

ObjectType EditorLayout::GetCurrentType() const
{
	int selectedIndex = std::max(0, _typeSelector->GetList()->GetCurSel()); // ignore -1
	_conf.object.SetInt(selectedIndex);
	return static_cast<ObjectType>(_typeSelector->GetData()->GetItemData(selectedIndex));
}
