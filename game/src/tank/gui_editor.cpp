// gui_editor.cpp

#include "gui_editor.h"
#include "GuiManager.h"

#include "Text.h"
#include "Edit.h"
#include "Combo.h"
#include "List.h"
#include "Scroll.h"
#include "Button.h"
#include "DataSourceAdapters.h"
#include "ListBase.h"

#include "constants.h"
#include "script.h"
#include "DefaultCamera.h"
#include "ClientBase.h"

#include "gc/2dSprite.h"
#include "gc/Camera.h"
#include "gc/Object.h"
#include "gc/World.h"
#include "gc/Macros.h"

#include "config/Config.h"
#include "config/Language.h"

#include "render/WorldView.h"

#include <GLFW/glfw3.h>

#include <ui/ConsoleBuffer.h>
UI::ConsoleBuffer& GetConsole();

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

NewMapDlg::NewMapDlg(Window *parent)
  : Dialog(parent, 256, 256)
{
	Text *header = Text::Create(this, 128, 20, g_lang.newmap_title.Get(), alignTextCT);
	header->SetFont("font_default");

	Text::Create(this, 40, 75, g_lang.newmap_width.Get(), alignTextLT);
	_width = Edit::Create(this, 60, 90, 80);
	_width->SetInt(g_conf.ed_width.GetInt());

	Text::Create(this, 40, 115, g_lang.newmap_height.Get(), alignTextLT);
	_height = Edit::Create(this, 60, 130, 80);
	_height->SetInt(g_conf.ed_height.GetInt());

	Button::Create(this, g_lang.common_ok.Get(), 20, 200)->eventClick = std::bind(&NewMapDlg::OnOK, this);
	Button::Create(this, g_lang.common_cancel.Get(), 140, 200)->eventClick = std::bind(&NewMapDlg::OnCancel, this);

	GetManager()->SetFocusWnd(_width);
}

void NewMapDlg::OnOK()
{
//	SAFE_DELETE(g_client);
	g_conf.sv_nightmode.Set(false);
	g_conf.ed_width.SetInt(std::max(LEVEL_MINSIZE, std::min(LEVEL_MAXSIZE, _width->GetInt())));
	g_conf.ed_height.SetInt(std::max(LEVEL_MINSIZE, std::min(LEVEL_MAXSIZE, _height->GetInt())));
	script_exec(g_env.L, "newmap(conf.ed_width, conf.ed_height)");
	Close(_resultOK);
}

void NewMapDlg::OnCancel()
{
	Close(_resultCancel);
}

///////////////////////////////////////////////////////////////////////////////
// PropertyList class implementation

PropertyList::Container::Container(Window *parent)
  : Window(parent)
{
}

//bool PropertyList::Container::OnRawChar(int c)
//{
//	return GetParent()->OnRawChar(c); // pass messages through
//}

PropertyList::PropertyList(Window *parent, float x, float y, float w, float h, World &world)
  : Dialog(parent, w, h, false)
  , _world(world)
{
	Move(x, y);
	_psheet = new Container(this);

	_scrollBar = ScrollBarVertical::Create(this, 0, 0, h);
	_scrollBar->Move(w - _scrollBar->GetWidth(), 0);
	_scrollBar->eventScroll = std::bind(&PropertyList::OnScroll, this, std::placeholders::_1);
//	_scrollBar->SetLimit(100);

	Resize(w, h);
	SetEasyMove(true);
	SetClipChildren(true);
}

void PropertyList::DoExchange(bool applyToObject)
{
	int focus = -1;

	if( applyToObject )
	{
		assert(_ps);
		for( int i = 0; i < _ps->GetCount(); ++i )
		{
			ObjectProperty *prop = _ps->GetProperty(i);
			Window         *ctrl = _ctrls[i];

			if( GetManager()->GetFocusWnd() == ctrl )
			{
				focus = i;
			}

			switch( prop->GetType() )
			{
			case ObjectProperty::TYPE_INTEGER:
				assert( dynamic_cast<Edit*>(ctrl) );
				int n;
				n = static_cast<Edit*>(ctrl)->GetInt();
				if( n < prop->GetIntMin() || n > prop->GetIntMax() )
				{
					GetConsole().Printf(1, "WARNING: value %s out of range [%d, %d]",
						prop->GetName().c_str(), prop->GetIntMin(), prop->GetIntMax());
					n = std::max(prop->GetIntMin(), std::min(prop->GetIntMax(), n));
				}
				prop->SetIntValue(n);
				break;
			case ObjectProperty::TYPE_FLOAT:
				assert( dynamic_cast<Edit*>(ctrl) );
				float f;
				f = static_cast<Edit*>(ctrl)->GetFloat();
				if( f < prop->GetFloatMin() || f > prop->GetFloatMax() )
				{
					GetConsole().Printf(1, "WARNING: value %s out of range [%g, %g]",
						prop->GetName().c_str(), prop->GetFloatMin(), prop->GetFloatMax());
					f = std::max(prop->GetFloatMin(), std::min(prop->GetFloatMax(), f));
				}
				prop->SetFloatValue(f);
				break;
			case ObjectProperty::TYPE_STRING:
				assert( dynamic_cast<Edit*>(ctrl) );
				prop->SetStringValue(static_cast<Edit*>(ctrl)->GetText());
				break;
			case ObjectProperty::TYPE_MULTISTRING:
				assert( dynamic_cast<ComboBox*>(ctrl) );
				int index;
				index = static_cast<ComboBox*>(ctrl)->GetCurSel();
				prop->SetCurrentIndex(index);
				break;
			default:
				assert(false);
			}
		}
		_ps->Exchange(_world, true);
	}


	// clear old controls
	while( _psheet->GetFirstChild() )
		_psheet->GetFirstChild()->Destroy();
	_ctrls.clear();

	// create new controls
	float y = 0;
	if( _ps )
	{
		y += 5;
		_ps->Exchange(_world, false);
		for( int i = 0; i < _ps->GetCount(); ++i )
		{
			ObjectProperty *prop = _ps->GetProperty(i);
			
			std::stringstream labelTextBuffer;
			labelTextBuffer << prop->GetName();

			Text *label = Text::Create(_psheet, 5, y, "", alignTextLT);
			y += label->GetHeight();
			y += 5;

			Window *ctrl = NULL;

			switch( prop->GetType() )
			{
			case ObjectProperty::TYPE_INTEGER:
				ctrl = Edit::Create(_psheet, 32, y, _psheet->GetWidth() - 64);
				static_cast<Edit*>(ctrl)->SetInt(prop->GetIntValue());
				labelTextBuffer << " (" << prop->GetIntMin() << " - " << prop->GetIntMax() << ")";
				break;
			case ObjectProperty::TYPE_FLOAT:
				ctrl = Edit::Create(_psheet, 32, y, _psheet->GetWidth() - 64);
				static_cast<Edit*>(ctrl)->SetFloat(prop->GetFloatValue());
				labelTextBuffer << " (" << prop->GetFloatMin() << " - " << prop->GetFloatMax() << ")";
				break;
			case ObjectProperty::TYPE_STRING:
				ctrl = Edit::Create(_psheet, 32, y, _psheet->GetWidth() - 64);
				static_cast<Edit*>(ctrl)->SetText(prop->GetStringValue());
				labelTextBuffer << " (string)";
				break;
			case ObjectProperty::TYPE_MULTISTRING:
				typedef ListAdapter<ListDataSourceDefault, ComboBox> DefaultComboBox;
				ctrl = DefaultComboBox::Create(_psheet);
				ctrl->Move(32, y);
				static_cast<DefaultComboBox *>(ctrl)->Resize(_psheet->GetWidth() - 64);
				for( size_t index = 0; index < prop->GetListSize(); ++index )
				{
					static_cast<DefaultComboBox *>(ctrl)->GetData()->AddItem(prop->GetListValue(index));
				}
				static_cast<DefaultComboBox*>(ctrl)->SetCurSel(prop->GetCurrentIndex());
				static_cast<DefaultComboBox*>(ctrl)->GetList()->AlignHeightToContent();
				break;
			default:
				assert(false);
			} // end of switch( prop->GetType() )

			label->SetText(labelTextBuffer.str());

			if( focus == i )
			{
				if( Edit *edit = dynamic_cast<Edit*>(ctrl) )
				{
					edit->SetSel(0, -1);
				}
				GetManager()->SetFocusWnd(ctrl);
			}

			assert(NULL != ctrl);
			_ctrls.push_back(ctrl);
			y += ctrl->GetHeight();
			y += 10;
		}
	}
	_psheet->Resize(_psheet->GetWidth(), y);
	_scrollBar->SetDocumentSize(y);
	OnScroll(_scrollBar->GetPos());
}

void PropertyList::ConnectTo(const SafePtr<PropertySet> &ps)
{
	if( _ps == ps ) return;
	_ps = ps;
	DoExchange(false);
}

void PropertyList::OnScroll(float pos)
{
	_psheet->Move(0, -floorf(_scrollBar->GetPos()));
}

void PropertyList::OnSize(float width, float height)
{
	_scrollBar->Resize(_scrollBar->GetWidth(), height);
	_scrollBar->Move(width - _scrollBar->GetWidth(), 0);
	_scrollBar->SetPageSize(height);
	_psheet->Resize(_scrollBar->GetX(), _psheet->GetHeight());
}

bool PropertyList::OnRawChar(int c)
{
	switch(c)
	{
	case GLFW_KEY_ENTER:
		DoExchange(true);
		_ps->SaveToConfig();
		break;
	case GLFW_KEY_ESCAPE:
		g_conf.ed_showproperties.Set(false);
		SetVisible(false);
		break;
	default:
		return Dialog::OnRawChar(c);
	}
	return true;
}

bool PropertyList::OnMouseWheel(float x, float y, float z)
{
	_scrollBar->SetPos( _scrollBar->GetPos() - z * 10.0f );
	OnScroll(_scrollBar->GetPos());
	return true;
}

///////////////////////////////////////////////////////////////////////////////

ServiceListDataSource::ServiceListDataSource(World &world)
  : _listener(NULL)
  , _world(world)
{
	assert(!_world._serviceListener);
	_world._serviceListener = this;
}

ServiceListDataSource::~ServiceListDataSource()
{
	assert(this == _world._serviceListener);
	_world._serviceListener = NULL;
}

void ServiceListDataSource::AddListener(ListDataSourceListener *listener)
{
	_listener = listener;
}

void ServiceListDataSource::RemoveListener(ListDataSourceListener *listener)
{
	assert(_listener && _listener == listener);
	_listener = NULL;
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
		return g_lang->GetRoot()->GetStr(RTTypes::Inst().GetTypeInfo(s->GetType()).desc, "")->Get();
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

void ServiceListDataSource::OnCreate(GC_Object *obj)
{
	_listener->OnAddItem();
}

void ServiceListDataSource::OnKill(GC_Object *obj)
{
	ObjectList &list = _world.GetList(LIST_services);
	int found = -1;
	int idx = 0;
	for( auto it = list.begin(); it != list.end(); it = list.next(it) )
	{
		if( list.at(it) == obj )
		{
			found = idx;
			break;
		}
		++idx;
	}
	assert(-1 != found);

	_listener->OnDeleteItem(found);
}

///////////////////////////////////////////////////////////////////////////////

ServiceEditor::ServiceEditor(Window *parent, float x, float y, float w, float h, World &world)
  : Dialog(parent, h, w, false)
  , _listData(world)
  , _margins(5)
  , _world(world)
{
	_labelService = Text::Create(this, _margins, _margins, g_lang.service_type.Get(), alignTextLT);
	_labelName = Text::Create(this, w/2, _margins, g_lang.service_name.Get(), alignTextLT);

	_list = List::Create(this, &_listData, 0, 0, 0, 0);
	_list->Move(_margins, _margins + _labelService->GetY() + _labelService->GetHeight());
	_list->SetDrawBorder(true);
	_list->eventChangeCurSel = std::bind(&ServiceEditor::OnSelectService, this, std::placeholders::_1);

	_btnCreate = Button::Create(this, g_lang.service_create.Get(), 0, 0);
	_btnCreate->eventClick = std::bind(&ServiceEditor::OnCreateService, this);

	_combo = DefaultComboBox::Create(this);
	_combo->Move(_margins, _margins);
	for( int i = 0; i < RTTypes::Inst().GetTypeCount(); ++i )
	{
		if( RTTypes::Inst().GetTypeInfoByIndex(i).service )
		{
			const char *desc0 = RTTypes::Inst().GetTypeInfoByIndex(i).desc;
			_combo->GetData()->AddItem(g_lang->GetRoot()->GetStr(desc0, "")->Get(), RTTypes::Inst().GetTypeByIndex(i));
		}
	}
	_combo->GetData()->Sort();

	List *ls = _combo->GetList();
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
		GC_Object *service = RTTypes::Inst().CreateObject(_world, type, 0, 0);
		GetEditorLayout()->SelectNone();
		GetEditorLayout()->Select(service, true);
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
		g_conf.ed_showservices.Set(false);
		SetVisible(false);
		break;
	default:
		return Dialog::OnRawChar(c);
	}
	return true;
}


///////////////////////////////////////////////////////////////////////////////

static GC_2dSprite* PickEdObject(World &world, const vec2d &pt, int layer)
{
    GC_2dSprite* zLayers[Z_COUNT];
    memset(zLayers, 0, sizeof(zLayers));
    
    std::vector<ObjectList*> receive;
    world.grid_sprites.OverlapPoint(receive, pt / LOCATION_SIZE);
    for( auto rit = receive.begin(); rit != receive.end(); ++rit )
    {
        ObjectList *ls = *rit;
        for( auto it = ls->begin(); it != ls->end(); it = ls->next(it) )
        {
            GC_2dSprite *object = static_cast<GC_2dSprite*>(ls->at(it));
            
            FRECT frect;
            object->GetGlobalRect(frect);
            
            if( PtInFRect(frect, pt) )
            {
                for( int i = 0; i < RTTypes::Inst().GetTypeCount(); ++i )
                {
                    if( Z_NONE != object->GetZ()
                        && object->GetType() == RTTypes::Inst().GetTypeByIndex(i)
                        && (-1 == layer || RTTypes::Inst().GetTypeInfoByIndex(i).layer == layer) )
                    {
                        zLayers[object->GetZ()] = object;
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
    
    return NULL;
}

    
EditorLayout::EditorLayout(Window *parent, World &world, WorldView &worldView, const DefaultCamera &defaultCamera)
  : Window(parent)
  , _defaultCamera(defaultCamera)
  , _fontSmall(GetManager()->GetTextureManager().FindSprite("font_small"))
  , _selectionRect(GetManager()->GetTextureManager().FindSprite("ui/selection"))
  , _selectedObject(NULL)
  , _isObjectNew(false)
  , _click(true)
  , _mbutton(0)
  , _world(world)
  , _worldView(worldView)
{
	SetTexture(NULL, false);

	_help = Text::Create(this, 10, 10, g_lang.f1_help_editor.Get(), alignTextLT);
	_help->SetVisible(false);

	_propList = new PropertyList(this, 5, 5, 512, 256, _world);
	_propList->SetVisible(false);

	_serviceList = new ServiceEditor(this, 5, 300, 512, 256, _world);
	_serviceList->SetVisible(g_conf.ed_showservices.Get());

	_layerDisp = Text::Create(this, 0, 0, "", alignTextRT);

	_typeList = DefaultComboBox::Create(this);
	_typeList->Resize(256);
	for( int i = 0; i < RTTypes::Inst().GetTypeCount(); ++i )
	{
		if( RTTypes::Inst().GetTypeInfoByIndex(i).service ) continue;
		const char *desc0 = RTTypes::Inst().GetTypeInfoByIndex(i).desc;
		_typeList->GetData()->AddItem(g_lang->GetRoot()->GetStr(desc0, "")->Get(), RTTypes::Inst().GetTypeByIndex(i));
	}
	_typeList->GetData()->Sort();
	List *ls = _typeList->GetList();
	ls->SetTabPos(1, 128);
	ls->AlignHeightToContent();
	_typeList->eventChangeCurSel = std::bind(&EditorLayout::OnChangeObjectType, this, std::placeholders::_1);
	_typeList->SetCurSel(std::min(_typeList->GetData()->GetItemCount() - 1, std::max(0, g_conf.ed_object.GetInt())));

	assert(!g_conf.ed_uselayers.eventChange);
	g_conf.ed_uselayers.eventChange = std::bind(&EditorLayout::OnChangeUseLayers, this);
	OnChangeUseLayers();
}

EditorLayout::~EditorLayout()
{
	g_conf.ed_uselayers.eventChange = nullptr;
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
			if( g_conf.ed_showproperties.Get() )
			{
				_propList->SetVisible(true);
			}
		}
	}
	else
	{
		assert(object == _selectedObject);
		_selectedObject = NULL;
		_isObjectNew = false;

		_propList->ConnectTo(NULL);
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
		GetManager()->SetCapture(NULL);
	}
	return true;
}

bool EditorLayout::OnMouseDown(float x, float y, int button)
{
	if( 0 == _mbutton )
	{
		GetManager()->SetCapture(this);
		_mbutton = button;
	}

	if( _mbutton != button )
	{
		return true;
	}

    
    vec2d mouse(x / _defaultCamera.GetZoom() + _defaultCamera.GetPosX(),
                y / _defaultCamera.GetZoom() + _defaultCamera.GetPosY());
    
    ObjectType type = static_cast<ObjectType>(
        _typeList->GetData()->GetItemData(g_conf.ed_object.GetInt()) );

    float align = RTTypes::Inst().GetTypeInfo(type).align;
    float offset = RTTypes::Inst().GetTypeInfo(type).offset;

    vec2d pt;
    pt.x = std::min(_world._sx - align, std::max(align - offset, mouse.x));
    pt.y = std::min(_world._sy - align, std::max(align - offset, mouse.y));
    pt.x -= fmod(pt.x + align * 0.5f - offset, align) - align * 0.5f;
    pt.y -= fmod(pt.y + align * 0.5f - offset, align) - align * 0.5f;

    int layer = -1;
    if( g_conf.ed_uselayers.Get() )
    {
        layer = RTTypes::Inst().GetTypeInfo(_typeList->GetData()->GetItemData(_typeList->GetCurSel())).layer;
    }

    if( GC_Object *object = PickEdObject(_world, mouse, layer) )
    {
        if( 1 == button )
        {
            if( _click && _selectedObject == object )
            {
                auto &selTypeInfo = RTTypes::Inst().GetTypeInfo(object->GetType());

                lua_getglobal(g_env.L, "editor_actions");
                if( lua_isnil(g_env.L, -1) )
                {
                    GetConsole().WriteLine(1, "There was no editor module loaded");
                }
                else
                {
                    lua_getfield(g_env.L, -1, selTypeInfo.name);
                    if (lua_isnil(g_env.L, -1))
                    {
                        // no action available for this object
                        lua_pop(g_env.L, 1);
                    }
                    else
                    {
                        luaT_pushobject(g_env.L, object);
                        if( lua_pcall(g_env.L, 1, 0, 0) )
                        {
                            GetConsole().WriteLine(1, lua_tostring(g_env.L, -1));
                            lua_pop(g_env.L, 1); // pop error message
                        }
                    }
                }
                lua_pop(g_env.L, 1); // pop editor_actions
                
                _propList->DoExchange(false);
                if( _isObjectNew )
                {
                    // save properties for new object
                    object->GetProperties(_world)->SaveToConfig();
                }
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
            GC_Object *object = RTTypes::Inst().CreateObject(_world, type, pt.x, pt.y);
            SafePtr<PropertySet> properties = object->GetProperties(_world);

            // set default properties if Ctrl key is not pressed
            if( glfwGetKey(g_appWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                glfwGetKey(g_appWindow, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS )
            {
                properties->LoadFromConfig();
                properties->Exchange(_world, true);
            }

            Select(object, true);
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
			g_conf.ed_showproperties.Set(true);
		}
		break;
	case GLFW_KEY_S:
		_serviceList->SetVisible(!_serviceList->GetVisible());
		g_conf.ed_showservices.Set(_serviceList->GetVisible());
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
	case GLFW_KEY_F9:
		g_conf.ed_uselayers.Set(!g_conf.ed_uselayers.Get());
		break;
	case GLFW_KEY_G:
		g_conf.ed_drawgrid.Set(!g_conf.ed_drawgrid.Get());
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
	g_conf.ed_object.SetInt(index);

	std::ostringstream buf;
	buf << g_lang.layer.Get() << RTTypes::Inst().GetTypeInfo(_typeList->GetData()->GetItemData(index)).layer << ": ";
	_layerDisp->SetText(buf.str());
}

void EditorLayout::OnChangeUseLayers()
{
	_layerDisp->SetVisible(g_conf.ed_uselayers.Get());
}

void EditorLayout::DrawChildren(DrawingContext &dc, float sx, float sy) const
{
    FRECT viewRect;
    viewRect.left = _defaultCamera.GetPosX();
    viewRect.top = _defaultCamera.GetPosY();
    viewRect.right = viewRect.left + (float) GetWidth() / _defaultCamera.GetZoom();
    viewRect.bottom = viewRect.top + (float) GetHeight() / _defaultCamera.GetZoom();
    
    g_render->Camera(NULL, _defaultCamera.GetPosX(), _defaultCamera.GetPosY(), _defaultCamera.GetZoom(), 0);
    _worldView.Render(dc, _world, viewRect, true);
    
	g_render->SetMode(RM_INTERFACE);
    
	if( GC_2dSprite *s = dynamic_cast<GC_2dSprite *>(_selectedObject) )
	{
		FRECT rt;
		s->GetGlobalRect(rt);

		FRECT sel = {
			(rt.left - _defaultCamera.GetPosX()) * _defaultCamera.GetZoom(),
			(rt.top - _defaultCamera.GetPosY()) * _defaultCamera.GetZoom(),
			(rt.left - _defaultCamera.GetPosX()) * _defaultCamera.GetZoom() + s->GetSpriteWidth() * _defaultCamera.GetZoom(),
			(rt.top - _defaultCamera.GetPosY()) * _defaultCamera.GetZoom() + s->GetSpriteHeight() * _defaultCamera.GetZoom()
		};
		dc.DrawSprite(&sel, _selectionRect, 0xffffffff, 0);
		dc.DrawBorder(&sel, _selectionRect, 0xffffffff, 0);
	}
    vec2d mouse(GetManager()->GetMousePos().x / _defaultCamera.GetZoom() + _defaultCamera.GetPosX(),
                GetManager()->GetMousePos().y / _defaultCamera.GetZoom() + _defaultCamera.GetPosY());

    std::stringstream buf;
    buf<<"x="<<floor(mouse.x+0.5f)<<"; y="<<floor(mouse.y+0.5f);
    dc.DrawBitmapText(sx+floor(GetWidth()/2+0.5f), sy+1, _fontSmall, 0xffffffff, buf.str(), alignTextCT);
	
	Window::DrawChildren(dc, sx, sy);
}

///////////////////////////////////////////////////////////////////////////////

MapSettingsDlg::MapSettingsDlg(Window *parent, World &world)
  : Dialog(parent, 512, 512)
  , _world(world)
{
	SetEasyMove(true);

	Text *title = Text::Create(this, GetWidth() / 2, 16, g_lang.map_title.Get(), alignTextCT);
	title->SetFont("font_default");


	float x1 = 20;
	float x2 = x1 + 12;

	float y = 32;

	Text::Create(this, x1, y += 20, g_lang.map_author.Get(), alignTextLT);
	_author = Edit::Create(this, x2, y += 15, 256);
	_author->SetText(world._infoAuthor);

	Text::Create(this, x1, y += 20, g_lang.map_email.Get(), alignTextLT);
	_email = Edit::Create(this, x2, y += 15, 256);
	_email->SetText(world._infoEmail);

	Text::Create(this, x1, y += 20, g_lang.map_url.Get(), alignTextLT);
	_url = Edit::Create(this, x2, y += 15, 256);
	_url->SetText(world._infoUrl);

	Text::Create(this, x1, y += 20, g_lang.map_desc.Get(), alignTextLT);
	_desc = Edit::Create(this, x2, y += 15, 256);
	_desc->SetText(world._infoDesc);

	Text::Create(this, x1, y += 20, g_lang.map_init_script.Get(), alignTextLT);
	_onInit = Edit::Create(this, x2, y += 15, 256);
	_onInit->SetText(world._infoOnInit);

	Text::Create(this, x1, y += 20, g_lang.map_theme.Get(), alignTextLT);
	_theme = DefaultComboBox::Create(this);
	_theme->Move(x2, y += 15);
	_theme->Resize(256);
	for( size_t i = 0; i < _ThemeManager::Inst().GetThemeCount(); i++ )
	{
		_theme->GetData()->AddItem(_ThemeManager::Inst().GetThemeName(i));
	}
	_theme->SetCurSel(_ThemeManager::Inst().FindTheme(world._infoTheme));
	_theme->GetList()->AlignHeightToContent();


	//
	// OK & Cancel
	//
	Button::Create(this, g_lang.common_ok.Get(), 304, 480)->eventClick = std::bind(&MapSettingsDlg::OnOK, this);
	Button::Create(this, g_lang.common_cancel.Get(), 408, 480)->eventClick = std::bind(&MapSettingsDlg::OnCancel, this);
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
	if( !_ThemeManager::Inst().ApplyTheme(i) )
	{
//		MessageBoxT(g_env.hMainWnd, "Could not apply theme", MB_ICONERROR);
	}

	Close(_resultOK);
}

void MapSettingsDlg::OnCancel()
{
	Close(_resultCancel);
}
	
} // end of namespace UI
