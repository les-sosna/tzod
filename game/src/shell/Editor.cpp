#include "Editor.h"
#include "PropertyList.h"
#include "inc/shell/Config.h"
#include "inc/shell/detail/DefaultCamera.h"
#include <app/ThemeManager.h>
#include <gc/Object.h>
#include <gc/Pickup.h>
#include <gc/Player.h>
#include <gc/RigidBody.h>
#include <gc/TypeSystem.h>
#include <gc/World.h>
#include <gc/WorldCfg.h>
#include <gc/Macros.h>
#include <gclua/lObjUtil.h>
#include <loc/Language.h>
#include <render/WorldView.h>
#include <render/RenderScheme.h>
#include <ui/GuiManager.h>
#include <ui/Text.h>
#include <ui/Edit.h>
#include <ui/Combo.h>
#include <ui/ConsoleBuffer.h>
#include <ui/List.h>
#include <ui/Scroll.h>
#include <ui/Button.h>
#include <ui/DataSourceAdapters.h>
#include <ui/ListBase.h>
#include <ui/UIInput.h>
#include <video/TextureManager.h>

#include <GLFW/glfw3.h>

#include <sstream>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

NewMapDlg::NewMapDlg(Window *parent, ConfCache &conf)
  : Dialog(parent, 256, 256)
  , _conf(conf)
{
	UI::Text *header = UI::Text::Create(this, 128, 20, g_lang.newmap_title.Get(), alignTextCT);
	header->SetFont("font_default");

	UI::Text::Create(this, 40, 75, g_lang.newmap_width.Get(), alignTextLT);
	_width = UI::Edit::Create(this, 60, 90, 80);
	_width->SetInt(_conf.ed_width.GetInt());

	UI::Text::Create(this, 40, 115, g_lang.newmap_height.Get(), alignTextLT);
	_height = UI::Edit::Create(this, 60, 130, 80);
	_height->SetInt(_conf.ed_height.GetInt());

	UI::Button::Create(this, g_lang.common_ok.Get(), 20, 200)->eventClick = std::bind(&NewMapDlg::OnOK, this);
	UI::Button::Create(this, g_lang.common_cancel.Get(), 140, 200)->eventClick = std::bind(&NewMapDlg::OnCancel, this);

	GetManager().SetFocusWnd(_width);
}

void NewMapDlg::OnOK()
{
	_conf.ed_width.SetInt(std::max(LEVEL_MINSIZE, std::min(LEVEL_MAXSIZE, _width->GetInt())));
	_conf.ed_height.SetInt(std::max(LEVEL_MINSIZE, std::min(LEVEL_MAXSIZE, _height->GetInt())));

	Close(_resultOK);
}

void NewMapDlg::OnCancel()
{
	Close(_resultCancel);
}

///////////////////////////////////////////////////////////////////////////////

ServiceListDataSource::ServiceListDataSource(World &world)
  : _listener(nullptr)
  , _world(world)
{
	_world.eWorld.AddListener(*this);
}

ServiceListDataSource::~ServiceListDataSource()
{
	_world.eWorld.RemoveListener(*this);
}

void ServiceListDataSource::AddListener(UI::ListDataSourceListener *listener)
{
	_listener = listener;
}

void ServiceListDataSource::RemoveListener(UI::ListDataSourceListener *listener)
{
	assert(_listener && _listener == listener);
	_listener = nullptr;
}

int ServiceListDataSource::GetItemCount() const
{
	return _world.GetList(LIST_services).size();
}

int ServiceListDataSource::GetSubItemCount(int index) const
{
	return 2;
}

size_t ServiceListDataSource::GetItemData(int index) const
{
	auto it = _world.GetList(LIST_services).begin();
	for( int i = 0; i < index; ++i )
	{
		it = _world.GetList(LIST_services).next(it);
		assert(_world.GetList(LIST_services).end() != it);
	}
	return (size_t) _world.GetList(LIST_services).at(it);
}

const std::string& ServiceListDataSource::GetItemText(int index, int sub) const
{
	GC_Object *s = (GC_Object *) GetItemData(index);
	const char *name;
	switch( sub )
	{
	case 0:
		return g_lang->GetStr(RTTypes::Inst().GetTypeInfo(s->GetType()).desc).Get();
	case 1:
		name = s->GetName(_world);
		_nameCache = name ? name : "";
		return _nameCache;
	}
	assert(false);
	_nameCache = "";
	return _nameCache;
}

int ServiceListDataSource::FindItem(const std::string &text) const
{
	return -1;
}

void ServiceListDataSource::OnNewObject(GC_Object &obj)
{
	if (obj.GetType() == GC_Player::GetTypeStatic())
		_listener->OnAddItem();
}

void ServiceListDataSource::OnKill(GC_Object &obj)
{
	if (obj.GetType() == GC_Player::GetTypeStatic())
	{
		ObjectList &list = _world.GetList(LIST_services);
		int found = -1;
		int idx = 0;
		for( auto it = list.begin(); it != list.end(); it = list.next(it) )
		{
			if( list.at(it) == &obj )
			{
				found = idx;
				break;
			}
			++idx;
		}
		assert(-1 != found);

		_listener->OnDeleteItem(found);
	}
}

///////////////////////////////////////////////////////////////////////////////

ServiceEditor::ServiceEditor(Window *parent, float x, float y, float w, float h, World &world, ConfCache &conf)
  : Dialog(parent, h, w, false)
  , _listData(world)
  , _margins(5)
  , _world(world)
  , _conf(conf)
{
	_labelService = UI::Text::Create(this, _margins, _margins, g_lang.service_type.Get(), alignTextLT);
	_labelName = UI::Text::Create(this, w/2, _margins, g_lang.service_name.Get(), alignTextLT);

	_list = UI::List::Create(this, &_listData, 0, 0, 0, 0);
	_list->Move(_margins, _margins + _labelService->GetY() + _labelService->GetHeight());
	_list->SetDrawBorder(true);
	_list->eventChangeCurSel = std::bind(&ServiceEditor::OnSelectService, this, std::placeholders::_1);

	_btnCreate = UI::Button::Create(this, g_lang.service_create.Get(), 0, 0);
	_btnCreate->eventClick = std::bind(&ServiceEditor::OnCreateService, this);

	_combo = DefaultComboBox::Create(this);
	_combo->Move(_margins, _margins);
	for( int i = 0; i < RTTypes::Inst().GetTypeCount(); ++i )
	{
		if( RTTypes::Inst().GetTypeInfoByIndex(i).service )
		{
			const char *desc0 = RTTypes::Inst().GetTypeInfoByIndex(i).desc;
			_combo->GetData()->AddItem(g_lang->GetStr(desc0).Get(), RTTypes::Inst().GetTypeByIndex(i));
		}
	}
	_combo->GetData()->Sort();

	UI::List *ls = _combo->GetList();
	ls->SetTabPos(1, 128);
	ls->AlignHeightToContent();

	Move(x, y);
	Resize(w, h);
	SetEasyMove(true);

	assert(!GetEditorLayout()->eventOnChangeSelection);
	GetEditorLayout()->eventOnChangeSelection = std::bind(&ServiceEditor::OnChangeSelectionGlobal, this, std::placeholders::_1);
}

ServiceEditor::~ServiceEditor()
{
	assert(GetEditorLayout()->eventOnChangeSelection);
	GetEditorLayout()->eventOnChangeSelection = nullptr;
}

void ServiceEditor::OnChangeSelectionGlobal(GC_Object *obj)
{
	if( obj )
	{
		for( int i = 0; i < _list->GetData()->GetItemCount(); ++i )
		{
			if( _list->GetData()->GetItemData(i) == (size_t) obj )
			{
				_list->SetCurSel(i, true);
				break;
			}
		}
	}
}

EditorLayout* ServiceEditor::GetEditorLayout() const
{
	assert(dynamic_cast<EditorLayout*>(GetParent()));
	return static_cast<EditorLayout*>(GetParent());
}

void ServiceEditor::OnCreateService()
{
	if( -1 != _combo->GetCurSel() )
	{
		ObjectType type = (ObjectType) _combo->GetData()->GetItemData(_combo->GetCurSel());
		GC_Service &service = RTTypes::Inst().CreateService(_world, type);
		GetEditorLayout()->SelectNone();
		GetEditorLayout()->Select(&service, true);
	}
}

void ServiceEditor::OnSelectService(int i)
{
	if( -1 == i )
	{
		GetEditorLayout()->SelectNone();
	}
	else
	{
		GetEditorLayout()->Select((GC_Object *) _list->GetData()->GetItemData(i), true);
	}
}

void ServiceEditor::OnSize(float width, float height)
{
	_btnCreate->Move(width - _btnCreate->GetWidth() - _margins,
		height - _btnCreate->GetHeight() - _margins);

	_combo->Resize(width - _margins * 3 - _btnCreate->GetWidth());
	_combo->Move(_margins, _btnCreate->GetY() + floor((_btnCreate->GetHeight() - _combo->GetHeight()) / 2 + 0.5f));

	_list->Resize(width - 2*_list->GetX(), _btnCreate->GetY() - _list->GetY() - _margins);
	_list->SetTabPos(1, floor(_list->GetWidth() / 2));
}

bool ServiceEditor::OnRawChar(int c)
{
	switch(c)
	{
	case GLFW_KEY_S:
	case GLFW_KEY_ESCAPE:
		_conf.ed_showservices.Set(false);
		SetVisible(false);
		break;
	default:
		return Dialog::OnRawChar(c);
	}
	return true;
}


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
					for( int i = 0; i < RTTypes::Inst().GetTypeCount(); ++i )
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


EditorLayout::EditorLayout(Window *parent, World &world, WorldView &worldView, const DefaultCamera &defaultCamera, lua_State *globL, ConfCache &conf, UI::ConsoleBuffer &logger)
  : Window(parent)
  , _conf(conf)
  , _logger(logger)
  , _defaultCamera(defaultCamera)
  , _fontSmall(GetManager().GetTextureManager().FindSprite("font_small"))
  , _selectionRect(GetManager().GetTextureManager().FindSprite("ui/selection"))
  , _selectedObject(nullptr)
  , _isObjectNew(false)
  , _click(true)
  , _mbutton(0)
  , _world(world)
  , _worldView(worldView)
  , _globL(globL)
{
	SetTexture(nullptr, false);

	_help = UI::Text::Create(this, 10, 10, g_lang.f1_help_editor.Get(), alignTextLT);
	_help->SetVisible(false);

	_propList = new PropertyList(this, 5, 5, 512, 256, _world, _conf, _logger);
	_propList->SetVisible(false);

	_serviceList = new ServiceEditor(this, 5, 300, 512, 256, _world, _conf);
	_serviceList->SetVisible(_conf.ed_showservices.Get());

	_layerDisp = UI::Text::Create(this, 0, 0, "", alignTextRT);

	_typeList = DefaultComboBox::Create(this);
	_typeList->Resize(256);
	for( int i = 0; i < RTTypes::Inst().GetTypeCount(); ++i )
	{
		if( RTTypes::Inst().GetTypeInfoByIndex(i).service ) continue;
		const char *desc0 = RTTypes::Inst().GetTypeInfoByIndex(i).desc;
		_typeList->GetData()->AddItem(g_lang->GetStr(desc0).Get(), RTTypes::Inst().GetTypeByIndex(i));
	}
	_typeList->GetData()->Sort();
	UI::List *ls = _typeList->GetList();
	ls->SetTabPos(1, 128);
	ls->AlignHeightToContent();
	_typeList->eventChangeCurSel = std::bind(&EditorLayout::OnChangeObjectType, this, std::placeholders::_1);
	_typeList->SetCurSel(std::min(_typeList->GetData()->GetItemCount() - 1, std::max(0, _conf.ed_object.GetInt())));

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
			_propList->ConnectTo(_selectedObject->GetProperties(_world));
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

		_propList->ConnectTo(nullptr);
		_propList->SetVisible(false);
	}

	if( eventOnChangeSelection )
		eventOnChangeSelection(_selectedObject);
}

void EditorLayout::SelectNone()
{
	if( _selectedObject )
	{
		Select(_selectedObject, false);
	}
}

bool EditorLayout::OnMouseWheel(float x, float y, float z)
{
	if( z > 0 )
	{
		_typeList->SetCurSel( std::max(0, _typeList->GetCurSel() - 1) );
	}
	if( z < 0 )
	{
		_typeList->SetCurSel( std::min(_typeList->GetData()->GetItemCount()-1, _typeList->GetCurSel() + 1) );
	}
	return true;
}

bool EditorLayout::OnMouseMove(float x, float y)
{
	if( _mbutton )
	{
		OnMouseDown(x, y, _mbutton);
	}
	return true;
}

bool EditorLayout::OnMouseUp(float x, float y, int button)
{
	if( _mbutton == button )
	{
		_click = true;
		_mbutton = 0;
		GetManager().SetCapture(nullptr);
	}
	return true;
}

bool EditorLayout::OnMouseDown(float x, float y, int button)
{
	if( 0 == _mbutton )
	{
		GetManager().SetCapture(this);
		_mbutton = button;
	}

	if( _mbutton != button )
	{
		return true;
	}


    vec2d mouse(x / _defaultCamera.GetZoom() + _defaultCamera.GetPos().x,
                y / _defaultCamera.GetZoom() + _defaultCamera.GetPos().y);

    ObjectType type = static_cast<ObjectType>(
        _typeList->GetData()->GetItemData(_conf.ed_object.GetInt()) );

    float align = RTTypes::Inst().GetTypeInfo(type).align;
    float offset = RTTypes::Inst().GetTypeInfo(type).offset;

    vec2d pt;
    pt.x = std::min(_world._sx - align, std::max(align - offset, mouse.x));
    pt.y = std::min(_world._sy - align, std::max(align - offset, mouse.y));
    pt.x -= fmod(pt.x + align * 0.5f - offset, align) - align * 0.5f;
    pt.y -= fmod(pt.y + align * 0.5f - offset, align) - align * 0.5f;

    int layer = -1;
    if( _conf.ed_uselayers.Get() )
    {
        layer = RTTypes::Inst().GetTypeInfo(_typeList->GetData()->GetItemData(_typeList->GetCurSel())).layer;
    }

    if( GC_Object *object = PickEdObject(_worldView.GetRenderScheme(), _world, mouse, layer) )
    {
        if( 1 == button )
        {
            if( _click && _selectedObject == object )
            {
                auto &selTypeInfo = RTTypes::Inst().GetTypeInfo(object->GetType());

                lua_getglobal(_globL, "editor_actions");
                if( lua_isnil(_globL, -1) )
                {
                    _logger.WriteLine(1, "There was no editor module loaded");
                }
                else
                {
                    lua_getfield(_globL, -1, selTypeInfo.name);
                    if (lua_isnil(_globL, -1))
                    {
                        // no action available for this object
                        lua_pop(_globL, 1);
                    }
                    else
                    {
                        luaT_pushobject(_globL, object);
                        if( lua_pcall(_globL, 1, 0, 0) )
                        {
                            _logger.WriteLine(1, lua_tostring(_globL, -1));
                            lua_pop(_globL, 1); // pop error message
                        }
                    }
                }
                lua_pop(_globL, 1); // pop editor_actions

                _propList->DoExchange(false);
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
    else
    {
        if( 1 == button )
        {
            // create object
            GC_Actor &newobj = RTTypes::Inst().CreateActor(_world, type, pt.x, pt.y);
			std::shared_ptr<PropertySet> properties = newobj.GetProperties(_world);

            // set default properties if Ctrl key is not pressed
            if( GetManager().GetInput().IsKeyPressed(GLFW_KEY_LEFT_CONTROL) ||
                GetManager().GetInput().IsKeyPressed(GLFW_KEY_RIGHT_CONTROL) )
            {
                LoadFromConfig(_conf, *properties);
                properties->Exchange(_world, true);
            }

			Select(&newobj, true);
            _isObjectNew = true;
        }
    }

	_click = false;
	return true;
}

bool EditorLayout::OnFocus(bool focus)
{
	return true;
}

bool EditorLayout::OnRawChar(int c)
{
	switch(c)
	{
	case GLFW_KEY_ENTER:
		if( _selectedObject )
		{
			_propList->SetVisible(true);
			_conf.ed_showproperties.Set(true);
		}
		break;
	case GLFW_KEY_S:
		_serviceList->SetVisible(!_serviceList->GetVisible());
		_conf.ed_showservices.Set(_serviceList->GetVisible());
		break;
	case GLFW_KEY_DELETE:
		if( _selectedObject )
		{
			GC_Object *o = _selectedObject;
			Select(_selectedObject, false);
			o->Kill(_world);
		}
		break;
	case GLFW_KEY_F1:
		_help->SetVisible(!_help->GetVisible());
		break;
	case GLFW_KEY_F8:
	//	dlg = new MapSettingsDlg(this, _world);
	//	SetDrawBackground(true);
	//	dlg->eventClose = std::bind(&Desktop::OnCloseChild, this, std::placeholders::_1);
	//	_nModalPopups++;
		break;

	case GLFW_KEY_F9:
		_conf.ed_uselayers.Set(!_conf.ed_uselayers.Get());
		break;
	case GLFW_KEY_G:
		_conf.ed_drawgrid.Set(!_conf.ed_drawgrid.Get());
		break;
	case GLFW_KEY_ESCAPE:
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

void EditorLayout::OnSize(float width, float height)
{
	_typeList->Move(width - _typeList->GetWidth() - 5, 5);
	_layerDisp->Move(width - _typeList->GetWidth() - 5, 6);
}

void EditorLayout::OnVisibleChange(bool visible, bool inherited)
{
	if( !visible )
	{
		SelectNone();
	}
}

void EditorLayout::OnChangeObjectType(int index)
{
	_conf.ed_object.SetInt(index);

	std::ostringstream buf;
	buf << g_lang.layer.Get() << RTTypes::Inst().GetTypeInfo(_typeList->GetData()->GetItemData(index)).layer << ": ";
	_layerDisp->SetText(buf.str());
}

void EditorLayout::OnChangeUseLayers()
{
	_layerDisp->SetVisible(_conf.ed_uselayers.Get());
}

static FRECT GetSelectionRect(const GC_Actor &actor)
{
	float halfSize = 8;
	if (auto *pickup = dynamic_cast<const GC_Pickup*>(&actor))
		halfSize = pickup->GetRadius();
	else if (auto *rbs = dynamic_cast<const GC_RigidBodyStatic*>(&actor))
		halfSize = rbs->GetRadius();
	FRECT result = {
		actor.GetPos().x - halfSize,
		actor.GetPos().y - halfSize,
		actor.GetPos().x + halfSize,
		actor.GetPos().y + halfSize
	};
	return result;
}

void EditorLayout::DrawChildren(DrawingContext &dc, float sx, float sy) const
{
	CRect viewport(0, 0, (int) GetWidth(), (int) GetHeight());
	vec2d eye(_defaultCamera.GetPos().x + GetWidth() / 2, _defaultCamera.GetPos().y + GetHeight() / 2);
	float zoom = _defaultCamera.GetZoom();
	_worldView.Render(dc, _world, viewport, eye, zoom, true, _conf.ed_drawgrid.Get(), _world.GetNightMode());

	dc.SetMode(RM_INTERFACE);

	if( auto *s = dynamic_cast<const GC_Actor *>(_selectedObject) )
	{
		FRECT rt = GetSelectionRect(*s);

		FRECT sel = {
			(rt.left - _defaultCamera.GetPos().x) * _defaultCamera.GetZoom(),
			(rt.top - _defaultCamera.GetPos().y) * _defaultCamera.GetZoom(),
			(rt.left - _defaultCamera.GetPos().x) * _defaultCamera.GetZoom() + WIDTH(rt) * _defaultCamera.GetZoom(),
			(rt.top - _defaultCamera.GetPos().y) * _defaultCamera.GetZoom() + HEIGHT(rt) * _defaultCamera.GetZoom()
		};
		dc.DrawSprite(&sel, _selectionRect, 0xffffffff, 0);
		dc.DrawBorder(&sel, _selectionRect, 0xffffffff, 0);
	}
    vec2d mouse = GetManager().GetInput().GetMousePos() / _defaultCamera.GetZoom() + _defaultCamera.GetPos();

    std::stringstream buf;
    buf<<"x="<<floor(mouse.x+0.5f)<<"; y="<<floor(mouse.y+0.5f);
    dc.DrawBitmapText(sx+floor(GetWidth()/2+0.5f), sy+1, _fontSmall, 0xffffffff, buf.str(), alignTextCT);

	Window::DrawChildren(dc, sx, sy);
}

///////////////////////////////////////////////////////////////////////////////

static size_t FindTheme(const ThemeManager &themeManager, const std::string &name)
{
	for (size_t i = 0; i < themeManager.GetThemeCount(); ++i)
	{
		if (themeManager.GetThemeName(i) == name)
		{
			return i;
		}
	}
	return 0;
}

MapSettingsDlg::MapSettingsDlg(Window *parent, World &world, const ThemeManager &themeManager)
  : Dialog(parent, 512, 512)
  , _world(world)
{
	SetEasyMove(true);

	UI::Text *title = UI::Text::Create(this, GetWidth() / 2, 16, g_lang.map_title.Get(), alignTextCT);
	title->SetFont("font_default");


	float x1 = 20;
	float x2 = x1 + 12;

	float y = 32;

	UI::Text::Create(this, x1, y += 20, g_lang.map_author.Get(), alignTextLT);
	_author = UI::Edit::Create(this, x2, y += 15, 256);
	_author->SetText(world._infoAuthor);

	UI::Text::Create(this, x1, y += 20, g_lang.map_email.Get(), alignTextLT);
	_email = UI::Edit::Create(this, x2, y += 15, 256);
	_email->SetText(world._infoEmail);

	UI::Text::Create(this, x1, y += 20, g_lang.map_url.Get(), alignTextLT);
	_url = UI::Edit::Create(this, x2, y += 15, 256);
	_url->SetText(world._infoUrl);

	UI::Text::Create(this, x1, y += 20, g_lang.map_desc.Get(), alignTextLT);
	_desc = UI::Edit::Create(this, x2, y += 15, 256);
	_desc->SetText(world._infoDesc);

	UI::Text::Create(this, x1, y += 20, g_lang.map_init_script.Get(), alignTextLT);
	_onInit = UI::Edit::Create(this, x2, y += 15, 256);
	_onInit->SetText(world._infoOnInit);

	UI::Text::Create(this, x1, y += 20, g_lang.map_theme.Get(), alignTextLT);
	_theme = DefaultComboBox::Create(this);
	_theme->Move(x2, y += 15);
	_theme->Resize(256);
	for( size_t i = 0; i < themeManager.GetThemeCount(); i++ )
	{
		_theme->GetData()->AddItem(themeManager.GetThemeName(i));
	}
	_theme->SetCurSel(FindTheme(themeManager, world._infoTheme));
	_theme->GetList()->AlignHeightToContent();


	//
	// OK & Cancel
	//
	UI::Button::Create(this, g_lang.common_ok.Get(), 304, 480)->eventClick = std::bind(&MapSettingsDlg::OnOK, this);
	UI::Button::Create(this, g_lang.common_cancel.Get(), 408, 480)->eventClick = std::bind(&MapSettingsDlg::OnCancel, this);
}

MapSettingsDlg::~MapSettingsDlg()
{
}

void MapSettingsDlg::OnOK()
{
	_world._infoAuthor = _author->GetText();
	_world._infoEmail = _email->GetText();
	_world._infoUrl = _url->GetText();
	_world._infoDesc = _desc->GetText();
	_world._infoOnInit = _onInit->GetText();

	int i = _theme->GetCurSel();
	if( 0 != i )
	{
		_world._infoTheme = _theme->GetData()->GetItemText(i, 0);
	}
	else
	{
		_world._infoTheme.clear();
	}

	Close(_resultOK);
}

void MapSettingsDlg::OnCancel()
{
	Close(_resultCancel);
}
