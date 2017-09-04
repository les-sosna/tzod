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
	class LayerDisplay : public UI::LayoutData<std::string_view>
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
	world.grid_actors.OverlapPoint(receive, pt / LOCATION_SIZE);
	for( auto rit = receive.begin(); rit != receive.end(); ++rit )
	{
		ObjectList *ls = *rit;
		for( auto it = ls->begin(); it != ls->end(); it = ls->next(it) )
		{
			auto actor = static_cast<GC_Actor*>(ls->at(it));
			if (RTTypes::Inst().IsRegistered(actor->GetType()) && PtOnActor(*actor, pt))
			{
				enumZOrder maxZ = Z_NONE;
				if(const ObjectViewsSelector::ViewCollection *views = rs.GetViews(*actor, true, false) )
				{
					for (auto &view: *views)
					{
						maxZ = std::max(maxZ, view.zfunc->GetZ(world, *actor));
					}
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


EditorLayout::EditorLayout(UI::LayoutManager &manager,
                           EditorContext &editorContext,
                           WorldView &worldView,
                           EditorConfig &conf,
                           LangCache &lang,
                           UI::ConsoleBuffer &logger)
  : UI::TimeStepping(manager)
  , _conf(conf)
  , _lang(lang)
  , _defaultCamera(Center(editorContext.GetOriginalBounds()))
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

	_propList = std::make_shared<PropertyList>(manager.GetTextureManager(), _world, conf, logger, lang);
	_propList->SetVisible(false);
	AddFront(_propList);

	auto gameClassVis = std::make_shared<GameClassVis>(_worldView);
	gameClassVis->Resize(64, 64);
	gameClassVis->SetGameClass(std::make_shared<UI::ListDataSourceBinding>(0));

	_typeSelector = std::make_shared<DefaultListBox>();
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
	_typeSelector->GetList()->SetCurSel(std::min(_typeSelector->GetData()->GetItemCount() - 1, std::max(0, _conf.object.GetInt())));

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
			if( _conf.showproperties.Get() )
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

		_propList->ConnectTo(nullptr);
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

void EditorLayout::CreateAt(vec2d worldPos, bool defaultProperties)
{
	vec2d alignedPos = AlignToGrid(worldPos);
	if (PtInFRect(_world._bounds, alignedPos) &&
		!PickEdObject(_worldView.GetRenderScheme(), _world, alignedPos))
	{
		// create object
		GC_Actor &newobj = RTTypes::Inst().CreateActor(_world, GetCurrentType(), alignedPos);
		std::shared_ptr<PropertySet> properties = newobj.GetProperties(_world);

		if (!defaultProperties)
		{
			LoadFromConfig(_conf, *properties);
			properties->Exchange(_world, true);
		}

		Select(&newobj, true);
		_isObjectNew = true;
	}
}

void EditorLayout::ActionOrSelectOrCreateAt(vec2d worldPos, bool defaultProperties)
{
	if( GC_Object *object = PickEdObject(_worldView.GetRenderScheme(), _world, worldPos) )
	{
		if( _selectedObject == object )
		{
			_quickActions.DoAction(*object);
			
			_propList->DoExchange(false);
			if( _isObjectNew )
				SaveToConfig(_conf, *object->GetProperties(_world));
		}
		else
		{
			Select(object, true);
		}
	}
	else
	{
		CreateAt(worldPos, defaultProperties);
	}
}

vec2d EditorLayout::AlignToGrid(vec2d worldPos) const
{
	auto &typeInfo = RTTypes::Inst().GetTypeInfo(GetCurrentType());
	vec2d offset = vec2d{ typeInfo.offset, typeInfo.offset };
	vec2d halfAlign = vec2d{ typeInfo.align, typeInfo.align } / 2;
	return Vec2dFloor((worldPos + halfAlign - offset) / typeInfo.align) * typeInfo.align + offset;
}

void EditorLayout::OnTimeStep(UI::LayoutManager &manager, float dt)
{
	_defaultCamera.HandleMovement(manager.GetInputContext().GetInput(), _world._bounds, dt);

	// Workaround: we do not get notifications when the object is killed
	if (!_selectedObject)
	{
		_propList->ConnectTo(nullptr);
		_propList->SetVisible(false);
	}
}

void EditorLayout::OnScroll(TextureManager &texman, const UI::InputContext &ic, const UI::LayoutContext &lc, const UI::DataContext &dc, vec2d pointerPosition, vec2d offset)
{
	_defaultCamera.Move(offset, _world._bounds);
}

void EditorLayout::OnPointerMove(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, UI::PointerType pointerType, unsigned int pointerID, bool captured)
{
	if (_capturedButton)
	{
		vec2d worldPos = CanvasToWorld(lc, pointerPosition);
		if (2 == _capturedButton)
		{
			EraseAt(worldPos);
		}
		else if (1 == _capturedButton)
		{
			// keep default properties if Ctrl key is not pressed
			bool defaultProperties = !ic.GetInput().IsKeyPressed(UI::Key::LeftCtrl) && !ic.GetInput().IsKeyPressed(UI::Key::RightCtrl);
			CreateAt(worldPos, defaultProperties);
		}
	}
}

void EditorLayout::OnPointerUp(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID)
{
	if (_capturedButton == button )
	{
		_capturedButton = 0;
	}
}

bool EditorLayout::OnPointerDown(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID)
{
	if (pointerType == UI::PointerType::Touch)
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

	vec2d worldPos = CanvasToWorld(lc, pointerPosition);
	if (2 == button)
	{
		EraseAt(worldPos);
	}
	else if (1 == button)
	{
		// keep default properties if Ctrl key is not pressed
		bool defaultProperties = !ic.GetInput().IsKeyPressed(UI::Key::LeftCtrl) && !ic.GetInput().IsKeyPressed(UI::Key::RightCtrl);
		ActionOrSelectOrCreateAt(worldPos, defaultProperties);
	}

	return capture;
}

void EditorLayout::OnTap(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition)
{
	ActionOrSelectOrCreateAt(CanvasToWorld(lc, pointerPosition), true);
}

bool EditorLayout::OnKeyPressed(UI::InputContext &ic, UI::Key key)
{
	switch(key)
	{
	case UI::Key::Enter:
		if( _selectedObject )
		{
			_propList->SetVisible(true);
			_conf.showproperties.Set(true);
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
		_conf.uselayers.Set(!_conf.uselayers.Get());
		break;
	case UI::Key::G:
		_conf.drawgrid.Set(!_conf.drawgrid.Get());
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

FRECT EditorLayout::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const
{
	float scale = lc.GetScale();
	vec2d size = lc.GetPixelSize();

	if (_layerDisp.get() == &child)
	{
		return UI::CanvasLayout(vec2d{ size.x / scale - 5, 6 }, _layerDisp->GetSize(), scale);
	}
	if (_typeSelector.get() == &child)
	{
		return FRECT{ 0, size.y - _typeSelector->GetContentSize(texman, dc, scale).y, size.x, size.y };
	}
	if (_propList.get() == &child)
	{
		float pxWidth = std::floor(100 * lc.GetScale());
		float pxBottom = _typeSelector ? GetChildRect(texman, lc, dc, *_typeSelector).top : lc.GetPixelSize().y;
		return FRECT{ lc.GetPixelSize().x - pxWidth, 0, lc.GetPixelSize().x, pxBottom };
	}

	return UI::Window::GetChildRect(texman, lc, dc, child);
}

void EditorLayout::OnChangeUseLayers()
{
	_layerDisp->SetVisible(_conf.uselayers.Get());
}

static FRECT GetSelectionRect(const GC_Actor &actor)
{
	vec2d halfSize = RTTypes::Inst().GetTypeInfo(actor.GetType()).size / 2;
	return MakeRectRB(actor.GetPos() - halfSize, actor.GetPos() + halfSize);
}

void EditorLayout::Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time) const
{
	// World
	RectRB viewport{ 0, 0, (int)lc.GetPixelSize().x, (int)lc.GetPixelSize().y };
	vec2d eye = _defaultCamera.GetEye();
	float zoom = _defaultCamera.GetZoom() * lc.GetScale();
	_worldView.Render(rc, _world, viewport, eye, zoom, true, _conf.drawgrid.Get(), _world.GetNightMode());

	// Selection
	if( auto selectedActor = PtrDynCast<const GC_Actor>(_selectedObject) )
	{
		FRECT rt = GetSelectionRect(*selectedActor); // in world coord
		FRECT sel = WorldToCanvas(lc, rt);

		rc.DrawSprite(sel, _texSelection.GetTextureId(texman), 0xffffffff, 0);
		rc.DrawBorder(sel, _texSelection.GetTextureId(texman), 0xffffffff, 0);
	}

	// Mouse coordinates
	vec2d mouse = CanvasToWorld(lc, ic.GetMousePos());
	std::stringstream buf;
	buf<<"x="<<floor(mouse.x+0.5f)<<"; y="<<floor(mouse.y+0.5f);
	rc.DrawBitmapText(vec2d{ std::floor(lc.GetPixelSize().x / 2 + 0.5f), 1 },
		lc.GetScale(), _fontSmall.GetTextureId(texman), 0xffffffff, buf.str(), alignTextCT);
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

ObjectType EditorLayout::GetCurrentType() const
{
	int selectedIndex = std::max(0, _typeSelector->GetList()->GetCurSel()); // ignore -1
	_conf.object.SetInt(selectedIndex);
	return static_cast<ObjectType>(_typeSelector->GetData()->GetItemData(selectedIndex));
}
