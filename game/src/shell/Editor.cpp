#include "Editor.h"
#include "ConfigBinding.h"
#include "PropertyList.h"
#include "GameClassVis.h"
#include "inc/shell/Config.h"
#include <gc/Object.h>
#include <gc/Pickup.h>
#include <gc/RigidBody.h>
#include <gc/TypeSystem.h>
#include <gc/WorldCfg.h>
#include <gv/ThemeManager.h>
#include <loc/Language.h>
#include <render/WorldView.h>
#include <render/RenderScheme.h>
#include <ui/Combo.h>
#include <ui/ConsoleBuffer.h>
#include <ui/DataSource.h>
#include <ui/GuiManager.h>
#include <ui/InputContext.h>
#include <ui/Text.h>
#include <ui/List.h>
#include <ui/ListBox.h>
#include <ui/DataSourceAdapters.h>
#include <ui/Keys.h>
#include <ui/LayoutContext.h>
#include <ui/ScrollView.h>
#include <ui/StackLayout.h>
#include <ui/StateContext.h>
#include <ui/UIInput.h>
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

static bool PtInPickup(const GC_Pickup &pickup, vec2d delta)
{
	float r = pickup.GetRadius();
	if (Vec2dDot(delta, delta) > r*r)
		return false;
	return true;
}

static bool PtInDefaultRadius(vec2d delta)
{
	float r = 8;
	if (Vec2dDot(delta, delta) > r*r)
		return false;
	return true;
}

static bool PtInActor(const GC_Actor &actor, vec2d pt)
{
	vec2d delta = pt - actor.GetPos();
	if (PtInDefaultRadius(delta))
		return true;
	if (auto rbs = dynamic_cast<const GC_RigidBodyStatic*>(&actor))
		return PtInRigidBody(*rbs, delta);
	if (auto pickup = dynamic_cast<const GC_Pickup*>(&actor))
		return PtInPickup(*pickup, delta);
	return false;
}

static GC_Actor* PickEdObject(const RenderScheme &rs, World &world, const vec2d &pt, int layer)
{
	GC_Actor* zLayers[Z_COUNT];
	memset(zLayers, 0, sizeof(zLayers));

	std::vector<ObjectList*> receive;
	world.grid_actors.OverlapPoint(receive, pt / LOCATION_SIZE);
	for( auto rit = receive.begin(); rit != receive.end(); ++rit )
	{
		ObjectList *ls = *rit;
		for( auto it = ls->begin(); it != ls->end(); it = ls->next(it) )
		{
			auto *object = static_cast<GC_Actor*>(ls->at(it));
			if (PtInActor(*object, pt))
			{
				enumZOrder maxZ = Z_NONE;
				if( const ObjectViewsSelector::ViewCollection *views = rs.GetViews(*object, true, false) )
				{
					for (auto &view: *views)
					{
						maxZ = std::max(maxZ, view.zfunc->GetZ(world, *object));
					}
				}

				if( Z_NONE != maxZ )
				{
					for( unsigned int i = 0; i < RTTypes::Inst().GetTypeCount(); ++i )
					{
						if( object->GetType() == RTTypes::Inst().GetTypeByIndex(i)
							&& (-1 == layer || RTTypes::Inst().GetTypeInfoByIndex(i).layer == layer) )
						{
							zLayers[maxZ] = object;
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


EditorLayout::EditorLayout(UI::LayoutManager &manager,
                           TextureManager &texman,
                           World &world,
                           WorldView &worldView,
                           ConfCache &conf,
                           LangCache &lang,
                           UI::ConsoleBuffer &logger)
  : Window(manager)
  , _conf(conf)
  , _lang(lang)
  , _logger(logger)
  , _fontSmall(texman.FindSprite("font_small"))
  , _texSelection(texman.FindSprite("ui/selection"))
  , _selectedObject(nullptr)
  , _isObjectNew(false)
  , _click(true)
  , _mbutton(0)
  , _world(world)
  , _worldView(worldView)
  , _quickActions(logger, world)
{
	_help = std::make_shared<UI::Text>(manager, texman);
	_help->Move(10, 10);
	_help->SetText(ConfBind(_lang.f1_help_editor));
	_help->SetAlign(alignTextLT);
	_help->SetVisible(false);
	AddFront(_help);

	_propList = std::make_shared<PropertyList>(manager, texman, 120.f, 240.f, _world, _conf, _logger);
	_propList->SetVisible(false);
	AddFront(_propList);

	_layerDisp = std::make_shared<UI::Text>(manager, texman);
	_layerDisp->SetAlign(alignTextRT);
	AddFront(_layerDisp);

	auto gameClassVis = std::make_shared<GameClassVis>(manager, texman, _worldView);
	gameClassVis->Resize(64, 64);
	gameClassVis->SetGameClass(std::make_shared<UI::ListDataSourceBinding>(0));

	_typeSelector = std::make_shared<DefaultListBox>(manager, texman);
	_typeSelector->GetScrollView()->SetHorizontalScrollEnabled(true);
	_typeSelector->GetScrollView()->SetVerticalScrollEnabled(false);
	_typeSelector->GetList()->SetFlowDirection(UI::FlowDirection::Horizontal);
	_typeSelector->GetList()->SetItemTemplate(gameClassVis);
	AddFront(_typeSelector);

	for( unsigned int i = 0; i < RTTypes::Inst().GetTypeCount(); ++i )
	{
		if (!RTTypes::Inst().GetTypeInfoByIndex(i).service)
		{
			auto &typeInfo = RTTypes::Inst().GetTypeInfoByIndex(i);
			_typeSelector->GetData()->AddItem(typeInfo.name, RTTypes::Inst().GetTypeByIndex(i));
		}
	}
	_typeSelector->GetList()->eventChangeCurSel = std::bind(&EditorLayout::OnChangeObjectType, this, std::placeholders::_1);
	_typeSelector->GetList()->SetCurSel(std::min(_typeSelector->GetData()->GetItemCount() - 1, std::max(0, _conf.ed_object.GetInt())));

	assert(!_conf.ed_uselayers.eventChange);
	_conf.ed_uselayers.eventChange = std::bind(&EditorLayout::OnChangeUseLayers, this);
	OnChangeUseLayers();
}

EditorLayout::~EditorLayout()
{
	_conf.ed_uselayers.eventChange = nullptr;
}

void EditorLayout::OnKillSelected(World &world, GC_Object *sender, void *param)
{
	Select(sender, false);
}

void EditorLayout::OnMoveSelected(World &world, GC_Object *sender, void *param)
{
	assert(_selectedObject == sender);
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
			_propList->ConnectTo(_selectedObject->GetProperties(_world), GetManager().GetTextureManager());
			if( _conf.ed_showproperties.Get() )
			{
				_propList->SetVisible(true);
			}
		}
	}
	else
	{
		assert(object == _selectedObject);
		_selectedObject = nullptr;
		_isObjectNew = false;

		_propList->ConnectTo(nullptr, GetManager().GetTextureManager());
		_propList->SetVisible(false);
	}
}

void EditorLayout::SelectNone()
{
	if( _selectedObject )
	{
		Select(_selectedObject, false);
	}
}

void EditorLayout::OnTimeStep(UI::LayoutManager &manager, float dt)
{
    _defaultCamera.HandleMovement(manager.GetInputContext().GetInput(), _world._bounds);
}

void EditorLayout::OnScroll(TextureManager &texman, const UI::InputContext &ic, const UI::LayoutContext &lc, const UI::StateContext &sc, vec2d pointerPosition, vec2d offset)
{
    _defaultCamera.Move(offset, _world._bounds);
}

void EditorLayout::OnPointerMove(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, UI::PointerType pointerType, unsigned int pointerID, bool captured)
{
	if( _mbutton )
	{
		OnPointerDown(ic, lc, texman, pointerPosition, _mbutton, pointerType, pointerID);
	}
}

void EditorLayout::OnPointerUp(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID)
{
	if( _mbutton == button )
	{
		_click = true;
		_mbutton = 0;
	}
}

bool EditorLayout::OnPointerDown(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID)
{
	bool capture = false;
	if( 0 == _mbutton )
	{
		capture = true;
		_mbutton = button;
	}

	if( _mbutton != button )
	{
		return capture;
	}

	SetFocus(nullptr);

	vec2d mouse = CanvasToWorld(lc, pointerPosition);

	ObjectType type = static_cast<ObjectType>(_typeSelector->GetData()->GetItemData(_conf.ed_object.GetInt()) );

	int layer = -1;
	if( _conf.ed_uselayers.Get() )
	{
		layer = RTTypes::Inst().GetTypeInfo(_typeSelector->GetData()->GetItemData(_typeSelector->GetList()->GetCurSel())).layer;
	}

	if( GC_Object *object = PickEdObject(_worldView.GetRenderScheme(), _world, mouse, layer) )
	{
		if( 1 == button )
		{
			if( _click && _selectedObject == object )
			{
				_quickActions.DoAction(*object);

				_propList->DoExchange(false, GetManager().GetTextureManager());
				if( _isObjectNew )
					SaveToConfig(_conf, *object->GetProperties(_world));
			}
			else
			{
				Select(object, true);
			}
		}

		if( 2 == button )
		{
			if( _selectedObject == object )
			{
				Select(object, false);
			}
			object->Kill(_world);
		}
	}
	else if( 1 == button )
	{
		float align = RTTypes::Inst().GetTypeInfo(type).align;
		vec2d offset = vec2d{ 1, 1 } * RTTypes::Inst().GetTypeInfo(type).offset;
		vec2d halfAlign = vec2d{ align, align } / 2;
		vec2d pt = Vec2dFloor((mouse + halfAlign - offset) / align) * align + offset;

		if (PtInFRect(_world._bounds, pt))
		{
			// create object
			GC_Actor &newobj = RTTypes::Inst().CreateActor(_world, type, pt.x, pt.y);
			std::shared_ptr<PropertySet> properties = newobj.GetProperties(_world);

			// set default properties if Ctrl key is not pressed
			if (ic.GetInput().IsKeyPressed(UI::Key::LeftCtrl) ||
				ic.GetInput().IsKeyPressed(UI::Key::RightCtrl))
			{
				LoadFromConfig(_conf, *properties);
				properties->Exchange(_world, true);
			}

			Select(&newobj, true);
			_isObjectNew = true;
		}
	}

	_click = false;

	return capture;
}

bool EditorLayout::OnKeyPressed(UI::InputContext &ic, UI::Key key)
{
	switch(key)
	{
	case UI::Key::Enter:
		if( _selectedObject )
		{
			_propList->SetVisible(true);
			_conf.ed_showproperties.Set(true);
		}
		break;
	case UI::Key::Delete:
		if( _selectedObject )
		{
			GC_Object *o = _selectedObject;
			Select(_selectedObject, false);
			o->Kill(_world);
		}
		break;
	case UI::Key::F1:
		_help->SetVisible(!_help->GetVisible());
		break;
	case UI::Key::F9:
		_conf.ed_uselayers.Set(!_conf.ed_uselayers.Get());
		break;
	case UI::Key::G:
		_conf.ed_drawgrid.Set(!_conf.ed_drawgrid.Get());
		break;
	case UI::Key::Escape:
		if( _selectedObject )
		{
			Select(_selectedObject, false);
		}
		else
		{
			return false;
		}
		break;
	default:
		return false;
	}
	return true;
}

FRECT EditorLayout::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::StateContext &sc, const UI::Window &child) const
{
	float scale = lc.GetScale();
	vec2d size = lc.GetPixelSize();

	if (_layerDisp.get() == &child)
	{
		return UI::CanvasLayout(vec2d{ size.x / scale - 5, 6 }, _layerDisp->GetSize(), scale);
	}
	else if (_typeSelector.get() == &child)
	{
		return FRECT{ 0, size.y - _typeSelector->GetContentSize(texman, sc, scale).y, size.x, size.y };
	}

	return UI::Window::GetChildRect(texman, lc, sc, child);
}

void EditorLayout::OnChangeObjectType(int index)
{
	_conf.ed_object.SetInt(index);

	std::ostringstream buf;
	buf << _lang.layer.Get() << RTTypes::Inst().GetTypeInfo(_typeSelector->GetData()->GetItemData(index)).layer << ": ";
	_layerDisp->SetText(std::make_shared<UI::StaticText>(buf.str()));
}

void EditorLayout::OnChangeUseLayers()
{
	_layerDisp->SetVisible(_conf.ed_uselayers.Get());
}

static FRECT GetSelectionRect(const GC_Actor &actor)
{
	vec2d halfSize = RTTypes::Inst().GetTypeInfo(actor.GetType()).size / 2;
	if (halfSize.sqr() < 16*16*2 - 1)
	{
		if (auto *pickup = dynamic_cast<const GC_Pickup*>(&actor))
			halfSize = { pickup->GetRadius(), pickup->GetRadius() };
		else if (auto *rbs = dynamic_cast<const GC_RigidBodyStatic*>(&actor))
			halfSize = { rbs->GetRadius(), rbs->GetRadius() };
		else
			halfSize = { 16, 16 };
	}
	FRECT result = {
		actor.GetPos().x - halfSize.x,
		actor.GetPos().y - halfSize.y,
		actor.GetPos().x + halfSize.x,
		actor.GetPos().y + halfSize.y
	};
	return result;
}

void EditorLayout::Draw(const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const
{
	// World
	RectRB viewport{ 0, 0, (int)lc.GetPixelSize().x, (int)lc.GetPixelSize().y };
	vec2d eye = _defaultCamera.GetEye();
	float zoom = _defaultCamera.GetZoom() * lc.GetScale();
	_worldView.Render(dc, _world, viewport, eye, zoom, true, _conf.ed_drawgrid.Get(), _world.GetNightMode());

	// Selection
	if( auto *s = dynamic_cast<const GC_Actor *>(_selectedObject) )
	{
		FRECT rt = GetSelectionRect(*s); // in world coord
		FRECT sel = WorldToCanvas(lc, rt);

		dc.DrawSprite(sel, _texSelection, 0xffffffff, 0);
		dc.DrawBorder(sel, _texSelection, 0xffffffff, 0);
	}

	// Mouse coordinates
	vec2d mouse = CanvasToWorld(lc, ic.GetMousePos());
	std::stringstream buf;
	buf<<"x="<<floor(mouse.x+0.5f)<<"; y="<<floor(mouse.y+0.5f);
	dc.DrawBitmapText(vec2d{ std::floor(lc.GetPixelSize().x / 2 + 0.5f), 1 },
		lc.GetScale(), _fontSmall, 0xffffffff, buf.str(), alignTextCT);
}

vec2d EditorLayout::CanvasToWorld(const UI::LayoutContext &lc, vec2d canvasPos) const
{
	vec2d eye = _defaultCamera.GetEye();
	float zoom = _defaultCamera.GetZoom() * lc.GetScale();
	return (canvasPos - lc.GetPixelSize() / 2) / zoom + eye;
}

vec2d EditorLayout::WorldToCanvas(const UI::LayoutContext &lc, vec2d worldPos) const
{
	vec2d eye = _defaultCamera.GetEye();
	float zoom = _defaultCamera.GetZoom() * lc.GetScale();
	return (worldPos - eye) * zoom + lc.GetPixelSize() / 2;
}

FRECT EditorLayout::WorldToCanvas(const UI::LayoutContext &lc, FRECT worldRect) const
{
	vec2d eye = _defaultCamera.GetEye();
	float zoom = _defaultCamera.GetZoom() * lc.GetScale();
	vec2d offset = (vec2d{ worldRect.left, worldRect.top } - eye) * zoom + lc.GetPixelSize() / 2;
	return MakeRectWH(offset, Size(worldRect) * zoom);
}
