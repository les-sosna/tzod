// gui_editor.cpp

#include "stdafx.h"

#include "gui_editor.h"
#include "GuiManager.h"

#include "Text.h"
#include "Edit.h"
#include "Combo.h"
#include "List.h"
#include "Scroll.h"
#include "Button.h"
#include "Button.h"

#include "gc/Object.h"
#include "gc/2dSprite.h"
#include "gc/Camera.h"

#include "config/Config.h"
#include "config/Language.h"

#include "core/Console.h"

#include "Level.h"
#include "Macros.h"


namespace UI
{
///////////////////////////////////////////////////////////////////////////////

NewMapDlg::NewMapDlg(Window *parent)
  : Dialog(parent, 256, 256)
{
	Text *header = new Text(this, 128, 20, "Новая карта:", alignTextCT);
	header->SetTexture("font_default");
	header->Resize(header->GetTextureWidth(), header->GetTextureHeight());

	new Text(this, 40, 75, "Ширина", alignTextLT);
	_width = new Edit(this, 60, 90, 80);
	_width->SetInt(g_conf->ed_width->GetInt());

	new Text(this, 40, 115, "Высота", alignTextLT);
	_height = new Edit(this, 60, 130, 80);
	_height->SetInt(g_conf->ed_height->GetInt());

	(new Button(this, 20, 200, "ОК"))->eventClick.bind(&NewMapDlg::OnOK, this);
	(new Button(this, 140, 200, "Отмена"))->eventClick.bind(&NewMapDlg::OnCancel, this);

	GetManager()->SetFocusWnd(_width);
}

NewMapDlg::~NewMapDlg()
{
}

void NewMapDlg::OnOK()
{
	g_conf->ed_width->SetInt( __max(LEVEL_MINSIZE, __min(LEVEL_MAXSIZE, _width->GetInt())) );
	g_conf->ed_height->SetInt( __max(LEVEL_MINSIZE, __min(LEVEL_MAXSIZE, _height->GetInt())) );
	script_exec(g_env.L, "reset(); newmap(conf.ed_width, conf.ed_height)");
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

void PropertyList::Container::OnRawChar(int c)
{
	GetParent()->OnRawChar(c); // pass messages through
}

PropertyList::PropertyList(Window *parent, float x, float y, float w, float h)
  : Dialog(parent, w, h, false)
{
	Move(x, y);
	_psheet = new Container(this);

	_scrollBar = new ScrollBar(this, 0, 0, h);
	_scrollBar->Move(w - _scrollBar->GetWidth(), 0);
	_scrollBar->eventScroll.bind(&PropertyList::OnScroll, this);
	_scrollBar->SetLimit(100);

	Resize(w, h);
	SetEasyMove(true);
	ClipChildren(true);
}

void PropertyList::Exchange(bool applyToObject)
{
	int focus = -1;

	if( applyToObject )
	{
		_ASSERT(_ps);
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
				_ASSERT( dynamic_cast<Edit*>(ctrl) );
				int n;
				n = static_cast<Edit*>(ctrl)->GetInt();
				if( n < prop->GetIntMin() || n > prop->GetIntMax() )
				{
					g_console->printf("WARNING: value %s out of range [%d, %d]\n",
						prop->GetName().c_str(), prop->GetIntMin(), prop->GetIntMax());
					n = __max(prop->GetIntMin(), __min(prop->GetIntMax(), n));
				}
				prop->SetIntValue(n);
				break;
			case ObjectProperty::TYPE_FLOAT:
				_ASSERT( dynamic_cast<Edit*>(ctrl) );
				float f;
				f = static_cast<Edit*>(ctrl)->GetFloat();
				if( f < prop->GetFloatMin() || f > prop->GetFloatMax() )
				{
					g_console->printf("WARNING: value %s out of range [%g, %g]\n",
						prop->GetName().c_str(), prop->GetFloatMin(), prop->GetFloatMax());
					f = __max(prop->GetFloatMin(), __min(prop->GetFloatMax(), f));
				}
				prop->SetFloatValue(f);
				break;
			case ObjectProperty::TYPE_STRING:
				_ASSERT( dynamic_cast<Edit*>(ctrl) );
				const char *s;
				s = static_cast<Edit*>(ctrl)->GetText().c_str();
				prop->SetStringValue(s);
				break;
			case ObjectProperty::TYPE_MULTISTRING:
				_ASSERT( dynamic_cast<ComboBox*>(ctrl) );
				int index;
				index = static_cast<ComboBox*>(ctrl)->GetCurSel();
				prop->SetCurrentIndex(index);
				break;
			default:
				_ASSERT(FALSE);
			}
		}
		_ps->Exchange(true);
	}


	// clear old controls
	while( _psheet->GetFirstChild() )
		_psheet->GetFirstChild()->Destroy();
	_ctrls.clear();

	// create new controls
	if( _ps )
	{
		_ps->Exchange(false);
		float y = 5;
		for( int i = 0; i < _ps->GetCount(); ++i )
		{
			ObjectProperty *prop = _ps->GetProperty(i);
			
			std::stringstream labelTextBuffer;
			labelTextBuffer << prop->GetName().c_str();

			Text *label = new Text(_psheet, 5, y, "", alignTextLT);
			y += label->GetHeight();
			y += 5;

			Window *ctrl = NULL;

			switch( prop->GetType() )
			{
			case ObjectProperty::TYPE_INTEGER:
				ctrl = new Edit(_psheet, 32, y, _psheet->GetWidth() - 64);
				static_cast<Edit*>(ctrl)->SetInt(prop->GetIntValue());
				labelTextBuffer << " (" << prop->GetIntMin() << " - " << prop->GetIntMax() << ")";
				break;
			case ObjectProperty::TYPE_FLOAT:
				ctrl = new Edit(_psheet, 32, y, _psheet->GetWidth() - 64);
				static_cast<Edit*>(ctrl)->SetFloat(prop->GetFloatValue());
				labelTextBuffer << " (" << prop->GetFloatMin() << " - " << prop->GetFloatMax() << ")";
				break;
			case ObjectProperty::TYPE_STRING:
				ctrl = new Edit(_psheet, 32, y, _psheet->GetWidth() - 64);
				static_cast<Edit*>(ctrl)->SetText(prop->GetStringValue().c_str());
				labelTextBuffer << " (string)";
				break;
			case ObjectProperty::TYPE_MULTISTRING:
				ctrl = new ComboBox(_psheet, 32, y, _psheet->GetWidth() - 64);
				for( size_t index = 0; index < prop->GetListSize(); ++index )
				{
					static_cast<ComboBox*>(ctrl)->GetList()->AddItem(prop->GetListValue(index).c_str());
				}
				static_cast<ComboBox*>(ctrl)->SetCurSel(prop->GetCurrentIndex());
				static_cast<ComboBox*>(ctrl)->GetList()->AlignHeightToContent();
				break;
			default:
				_ASSERT(FALSE);
			} // end of switch( prop->GetType() )

			label->SetText(labelTextBuffer.str().c_str());

			if( focus == i )
			{
				if( Edit *edit = dynamic_cast<Edit*>(ctrl) )
				{
					edit->SetSel(0, -1);
				}
				GetManager()->SetFocusWnd(ctrl);
			}

			_ASSERT(NULL != ctrl);
			_ctrls.push_back(ctrl);
			y += ctrl->GetHeight();
			y += 10;
		}

		_psheet->Resize(_psheet->GetWidth(), y);
		_scrollBar->SetLimit(y - GetHeight());
	}
	else
	{
		_psheet->Resize(_psheet->GetWidth(), 0);
		_scrollBar->SetLimit(0);
	}
	OnScroll(_scrollBar->GetPos());
}

void PropertyList::ConnectTo(const SafePtr<PropertySet> &ps)
{
	if( _ps == ps ) return;
	_ps = ps;
	Exchange(false);
}

void PropertyList::OnScroll(float pos)
{
	_psheet->Move(0, -floorf(_scrollBar->GetPos()));
}

void PropertyList::OnSize(float width, float height)
{
	_scrollBar->Resize(_scrollBar->GetWidth(), height);
	_scrollBar->Move(width - _scrollBar->GetWidth(), 0);
	_scrollBar->SetLimit(_psheet->GetHeight() - height);
	_psheet->Resize(_scrollBar->GetX(), _psheet->GetHeight());
}

void PropertyList::OnRawChar(int c)
{
	switch(c)
	{
	case VK_RETURN:
		Exchange(true);
		_ps->SaveToConfig();
		break;
	case VK_ESCAPE:
		g_conf->ed_showproperties->Set(false);
		Show(false);
		break;
	default:
		GetParent()->OnRawChar(c);
	}
}

bool PropertyList::OnMouseWheel(float x, float y, float z)
{
	_scrollBar->SetPos( _scrollBar->GetPos() - z * 10.0f );
	OnScroll(_scrollBar->GetPos());
	return true;
}

///////////////////////////////////////////////////////////////////////////////

ServiceList::ServiceList(Window *parent, float x, float y, float w, float h)
  : Dialog(parent, h, w, false)
  , _margins(5)
{
	_labelService = new Text(this, _margins, _margins, g_lang->service_type->Get(), alignTextLT);
	_labelName = new Text(this, w/2, _margins, g_lang->service_name->Get(), alignTextLT);

	_list = new List(this, _margins, _margins + _labelService->GetY() + _labelService->GetTextHeight(), 1, 1);
	_list->SetBorder(true);
	_list->eventChangeCurSel.bind(&ServiceList::OnSelectService, this);

	_btnCreate = new Button(this, 0, 0, g_lang->service_create->Get());
	_btnCreate->eventClick.bind(&ServiceList::OnCreateService, this);

	_combo = new ComboBox(this, _margins, _margins, 1);
	List *ls = _combo->GetList();
	for( int i = 0; i < Level::GetTypeCount(); ++i )
	{
		if( Level::GetTypeInfoByIndex(i).service )
		{
			const char *desc0 = Level::GetTypeInfoByIndex(i).desc;
			const char *desc = g_lang.GetRoot()->GetStr(desc0, desc0)->Get();
			ls->AddItem(desc, Level::GetTypeByIndex(i));
		}
	}
	ls->SetTabPos(1, 128);
	ls->AlignHeightToContent();
	ls->Sort();

	Move(x, y);
	Resize(w, h);
	SetEasyMove(true);
}

void ServiceList::UpdateList()
{
//	ULONG_PTR sel = (-1 != _list->GetCurSel()) ? _list->GetItemData(_list->GetCurSel()) : 0;
	_list->DeleteAllItems();
	FOREACH(g_level->GetList(LIST_services), GC_Object, service)
	{
		const char *desc0 = g_level->GetTypeInfo(service->GetType()).desc;
		int i = _list->AddItem(g_lang.GetRoot()->GetStr(desc0, desc0)->Get(), (ULONG_PTR) service);
		const char *name = service->GetName();
		_list->SetItemText(i, 1, name ? name : "");
	}
}

EditorLayout* ServiceList::GetEditorLayout() const
{
	_ASSERT(dynamic_cast<EditorLayout*>(GetParent()));
	return static_cast<EditorLayout*>(GetParent());
}

void ServiceList::OnCreateService()
{
	if( -1 != _combo->GetCurSel() )
	{
		ObjectType type = (ObjectType) _combo->GetList()->GetItemData(_combo->GetCurSel());
		GC_Object *service = g_level->CreateObject(type, 0, 0);
		UpdateList();
	}
}

void ServiceList::OnSelectService(int i)
{
	if( -1 == i )
	{
		GetEditorLayout()->SelectNone();
	}
	else
	{
		GetEditorLayout()->Select((GC_Object *) _list->GetItemData(i), true);
	}
}

void ServiceList::OnSize(float width, float height)
{
	_btnCreate->Move(width - _btnCreate->GetWidth() - _margins,
		height - _btnCreate->GetHeight() - _margins);

	_combo->Resize(width - _margins * 3 - _btnCreate->GetWidth(), _combo->GetHeight());
	_combo->Move(_margins, _btnCreate->GetY() + (_btnCreate->GetHeight() - _combo->GetHeight()) / 2);

	_list->Resize(width - 2*_list->GetX(), _btnCreate->GetY() - _list->GetY() - _margins);
	_list->SetTabPos(1, _list->GetWidth() / 2);
}

void ServiceList::OnRawChar(int c)
{
	switch(c)
	{
//	case VK_RETURN:
//		Exchange(true);
//		_ps->SaveToConfig();
//		break;

	case 'S':
	case VK_ESCAPE:
		g_conf->ed_showservices->Set(false);
		Show(false);
		break;
	default:
		GetParent()->OnRawChar(c);
	}
}


///////////////////////////////////////////////////////////////////////////////

EditorLayout::EditorLayout(Window *parent)
  : Window(parent)
{
	SetTexture(NULL);

	_help = new Text(this, 10, 10,
		"F1                   - справка\n"
		"F5                   - включить/выключить редактор\n"
		"F8                   - настройки карты\n"
		"F9                   - включить/выключить слои\n"
		"G                    - показать/спрятать сетку\n"
		"ESC                  - выход в главное меню\n"
		"Delete               - удалить выделенный объект\n"
		"Enter                - свойства выделенного объекта\n"
		"Стрелки              - движение камеры\n"
		"Колёсико мышки       - выбрать тип объекта\n"
		"Левая кнопка мышки   - создать объект; действие над объектом\n"
		"Правая кнопка мышки  - удалить объект\n"
		"\nУдерживайте Ctrl, чтобы создать объект с параметрами по умолчанию."
		, alignTextLT);
	_help->Show(false);

	_propList = new PropertyList(this, 5, 5, 512, 256);
	_propList->Show(false);

	_serviceList = new ServiceList(this, 5, 300, 512, 256);
	_serviceList->Show(g_conf->ed_showservices->Get());

	_layerDisp = new Text(this, 0, 0, "", alignTextRT);

	_typeList = new ComboBox(this, 0, 0, 256);
	List *ls = _typeList->GetList();
	for( int i = 0; i < Level::GetTypeCount(); ++i )
	{
		if( Level::GetTypeInfoByIndex(i).service ) continue;
		const char *desc0 = Level::GetTypeInfoByIndex(i).desc;
		const char *desc = g_lang.GetRoot()->GetStr(desc0, desc0)->Get();
		ls->AddItem(desc, Level::GetTypeByIndex(i));
	}
	ls->SetTabPos(1, 128);
	ls->AlignHeightToContent();
	ls->Sort();
	_typeList->eventChangeCurSel.bind(&EditorLayout::OnChangeObject, this);
	_typeList->SetCurSel( g_conf->ed_object->GetInt() );

	_selectionRect = new Window(this, 0, 0, "selection");
	_selectionRect->SetBorder(true);
	_selectionRect->Show(false);
	_selectionRect->BringToBack();

	_selectedObject = NULL;
	_isObjectNew = false;
	_click = true;
	_mbutton = 0;

	_ASSERT(!g_conf->ed_uselayers->eventChange);
	g_conf->ed_uselayers->eventChange.bind(&EditorLayout::OnChangeUseLayers, this);
	OnChangeUseLayers();
}

EditorLayout::~EditorLayout()
{
	g_conf->ed_uselayers->eventChange.clear();
}

void EditorLayout::OnKillSelected(GC_Object *sender, void *param)
{
	Select(sender, false);
}

void EditorLayout::OnMoveSelected(GC_Object *sender, void *param)
{
	_ASSERT(_selectedObject == sender);
}

void EditorLayout::Select(GC_Object *object, bool bSelect)
{
	_ASSERT(NULL != object);

	if( bSelect )
	{
		if( _selectedObject != object )
		{
			if( _selectedObject )
			{
				Select(_selectedObject, false);
			}

			_selectedObject = object;
			if( _selectedObject )
			{
				_propList->ConnectTo(_selectedObject->GetProperties());
				if( g_conf->ed_showproperties->Get() )
				{
					_propList->Show(true);
				}
			}
		}
	}
	else
	{
		_ASSERT(object == _selectedObject);
		_selectedObject = NULL;
		_isObjectNew = false;

		_propList->ConnectTo(NULL);
		_selectionRect->Show(false);
		_propList->Show(false);
	}
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
		_typeList->SetCurSel( __max(0, _typeList->GetCurSel() - 1) );
	}
	if( z < 0 )
	{
		_typeList->SetCurSel( __min(_typeList->GetList()->GetSize()-1, _typeList->GetCurSel() + 1) );
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
		ReleaseCapture();
	}
	return true;
}

bool EditorLayout::OnMouseDown(float x, float y, int button)
{
	if( 0 == _mbutton )
	{
		SetCapture();
		_mbutton = button;
	}

	if( _mbutton != button )
	{
		return true;
	}

	vec2d mouse;
	if( g_level && GC_Camera::GetWorldMousePos(mouse) )
	{
		ObjectType type = static_cast<ObjectType>(
			_typeList->GetList()->GetItemData(g_conf->ed_object->GetInt()) );

		float align = Level::GetTypeInfo(type).align;
		float offset = Level::GetTypeInfo(type).offset;

		vec2d pt;
		pt.x = __min(g_level->_sx - align, __max(align - offset, mouse.x));
		pt.y = __min(g_level->_sy - align, __max(align - offset, mouse.y));
		pt.x -= fmodf(pt.x + align * 0.5f - offset, align) - align * 0.5f;
		pt.y -= fmodf(pt.y + align * 0.5f - offset, align) - align * 0.5f;

		int layer = -1;
		if( g_conf->ed_uselayers->Get() )
		{
			layer = Level::GetTypeInfo(_typeList->GetList()->GetItemData(_typeList->GetCurSel())).layer;
		}

		if( GC_Object *object = g_level->PickEdObject(mouse, layer) )
		{
			if( 1 == button )
			{
				if( _click && _selectedObject == object )
				{
					object->EditorAction();
					_propList->Exchange(false);
					if( _isObjectNew )
					{
						// save properties for new object
						object->GetProperties()->SaveToConfig();
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
				object->Kill();
			}
		}
		else
		{
			if( 1 == button )
			{
				// create object
				GC_Object *object = g_level->CreateObject(type, pt.x, pt.y);
				SafePtr<PropertySet> properties = object->GetProperties();

				// set default properties if Ctrl key is not pressed
				if( 0 == (GetAsyncKeyState(VK_CONTROL) & 0x8000) )
				{
					properties->LoadFromConfig();
					properties->Exchange(true);
				}

				Select(object, true);
				_isObjectNew = true;
			}
		}
	}

	_click = false;
	return true;
}

bool EditorLayout::OnFocus(bool focus)
{
	return true;
}

void EditorLayout::OnRawChar(int c)
{
	switch(c)
	{
	case VK_RETURN:
		if( _selectedObject )
		{
			_propList->Show(true);
			g_conf->ed_showproperties->Set(true);
		}
		break;
	case 'S':
		_serviceList->Show(!_serviceList->IsVisible());
		g_conf->ed_showservices->Set(_serviceList->IsVisible());
		break;
	case VK_DELETE:
		if( _selectedObject )
		{
			GC_Object *o = _selectedObject;
			Select(_selectedObject, false);
			o->Kill();
		}
		break;
	case VK_F1:
		_help->Show(!_help->IsVisible());
		break;
	case VK_F9:
		g_conf->ed_uselayers->Set(!g_conf->ed_uselayers->Get());
		break;
	case 'G':
		g_conf->ed_drawgrid->Set(!g_conf->ed_drawgrid->Get());
		break;
	case VK_ESCAPE:
		if( _selectedObject )
		{
			Select(_selectedObject, false);
		}
		else
		{
			GetParent()->OnRawChar(c);
		}
		break;
	default:
		GetParent()->OnRawChar(c);
	}
}

void EditorLayout::OnSize(float width, float height)
{
	_typeList->Move(width - _typeList->GetWidth() - 5, 5);
	_layerDisp->Move(width - _typeList->GetWidth() - 5, 6);
}

void EditorLayout::OnShow(bool show)
{
	if( !show )
	{
		_propList->ConnectTo(NULL);
		_propList->Show(false);
		_selectedObject = NULL;
	}
}

void EditorLayout::OnChangeObject(int index)
{
	g_conf->ed_object->SetInt(index);

	std::ostringstream buf;
	buf << "слой " << Level::GetTypeInfo(_typeList->GetList()->GetItemData(index)).layer << ": ";
	_layerDisp->SetText(buf.str().c_str());
}

void EditorLayout::OnChangeUseLayers()
{
	_layerDisp->Show(g_conf->ed_uselayers->Get());
}

void EditorLayout::DrawChildren(float sx, float sy)
{
	if( GC_2dSprite *s = dynamic_cast<GC_2dSprite *>(_selectedObject) )
	{
		_ASSERT(g_level);
		GC_Camera *camera = NULL;

		FOREACH( g_level->GetList(LIST_cameras), GC_Camera, c )
		{
			if( c->IsActive() )
			{
				camera = c;
				break;
			}
		}

		_ASSERT(camera);
		RECT viewport;
		camera->GetViewport(viewport);

		_selectionRect->Show(true);
		_selectionRect->Move(
			(float) viewport.left + s->GetPos().x - s->GetPivot().x - camera->GetPos().x,
			(float) viewport.top + s->GetPos().y - s->GetPivot().y - camera->GetPos().y );
		_selectionRect->Resize(s->GetSpriteWidth(), s->GetSpriteHeight());
	}
	else
	{
		_selectionRect->Show(false);
	}
	Window::DrawChildren(sx, sy);
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
