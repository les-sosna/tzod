// gui_editor.cpp

#include "stdafx.h"

#include "gui_editor.h"

#include "ui/Text.h"
#include "ui/Edit.h"
#include "ui/Combo.h"
#include "ui/List.h"
#include "ui/Scroll.h"

#include "gc/Object.h"
#include "gc/2dSprite.h"
#include "gc/Camera.h"

#include "config/Config.h"

#include "Level.h"


namespace UI
{

///////////////////////////////////////////////////////////////////////////////
// PropertyList class implementation

PropertyList::Container::Container(Window *parent) : Window(parent)
{
}

void PropertyList::Container::OnRawChar(int c)
{
	GetParent()->OnRawChar(c); // pass messages through
}

PropertyList::PropertyList(Window *parent, float x, float y, float w, float h)
  : Dialog(parent, x, y, w, h)
{
	_psheet = new Container(this);

	_scrollBar = new ScrollBar(this, 0, 0, h);
	_scrollBar->Move(w - _scrollBar->GetWidth(), 0);
	_scrollBar->eventScroll.bind(&PropertyList::OnScroll, this);
	_scrollBar->SetLimit(100);

	Resize(w, h);
	SetEasyMove(true);
	ClipChildren(true);
}

void PropertyList::Commit()
{
	_ASSERT(_ps);
	for( int i = 0; i < _ps->GetCount(); ++i )
	{
		ObjectProperty *prop = _ps->GetProperty(i);
		Window *ctrl = _ctrls[i];

		switch( prop->GetType() )
		{
		case ObjectProperty::TYPE_INTEGER:
			_ASSERT( dynamic_cast<Edit*>(ctrl) );
			prop->SetValueInt(static_cast<Edit*>(ctrl)->GetInt());
			break;
		case ObjectProperty::TYPE_STRING:
			_ASSERT( dynamic_cast<Edit*>(ctrl) );
			prop->SetValue(static_cast<Edit*>(ctrl)->GetText());
			break;
		case ObjectProperty::TYPE_MULTISTRING:
			_ASSERT( dynamic_cast<ComboBox*>(ctrl) );
			prop->SetCurrentIndex( static_cast<ComboBox*>(ctrl)->GetCurSel() );
			break;
		default:
			_ASSERT(FALSE);
		}
	}
	_ps->Exchange(true);

	SafePtr<PropertySet> tmp = _ps;
	_ps = NULL;
	ConnectTo(tmp);
}

void PropertyList::ConnectTo(const SafePtr<PropertySet> &ps)
{
	if( _ps == ps ) return;

	// clear old controlls
	while( _psheet->GetFirstChild() )
		_psheet->GetFirstChild()->Destroy();
	_ctrls.clear();

	// create new controlls
	if( ps )
	{
		float y = 5;
		for( int i = 0; i < ps->GetCount(); ++i )
		{
			ObjectProperty *prop = ps->GetProperty(i);
			y += (new Text(_psheet, 5, y, prop->GetName().c_str(), alignTextLT))->GetHeight();
			y += 5;

			Window *ctrl = NULL;

			switch( prop->GetType() )
			{
			case ObjectProperty::TYPE_INTEGER:
				ctrl = new Edit(_psheet, 32, y, _psheet->GetWidth() - 64 );
				static_cast<Edit*>(ctrl)->SetInt(prop->GetValueInt());
				break;
			case ObjectProperty::TYPE_STRING:
				ctrl = new Edit(_psheet, 32, y, _psheet->GetWidth() - 64 );
				static_cast<Edit*>(ctrl)->SetText(prop->GetValue().c_str());
				break;
			case ObjectProperty::TYPE_MULTISTRING:
				ctrl = new ComboBox(_psheet, 32, y, _psheet->GetWidth() - 64 );
				for( size_t index = 0; index < prop->GetSetSize(); ++index )
				{
					static_cast<ComboBox*>(ctrl)->GetList()->AddItem(prop->GetSetValue(index).c_str());
				}
				static_cast<ComboBox*>(ctrl)->SetCurSel(prop->GetCurrentIndex());
				break;
			default:
				_ASSERT(FALSE);
			} // end of switch( prop->GetType() )

			_ASSERT(NULL != ctrl);
			_ctrls.push_back(ctrl);
			y += ctrl->GetHeight();
			y += 10;
		}

		_psheet->Resize(_psheet->GetWidth(), y);
		_scrollBar->SetLimit(y - GetHeight());
	}

	_ps = ps;
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
		Commit();
		break;
	case VK_ESCAPE:
		Show(false);
		break;
	default:
		GetParent()->OnRawChar(c);
	}
}

///////////////////////////////////////////////////////////////////////////////

EditorLayout::EditorLayout(Window *parent) : Window(parent)
{
	SetTexture(NULL);
	_propList = new PropertyList(this, 5, 5, 256, 256);

	_typeList = new ComboBox(this, 0, 0, 256);
	List *ls = _typeList->GetList();
	for( int i = 0; i < Level::GetTypeCount(); ++i )
	{
		ls->AddItem(Level::GetTypeDesc(i), Level::GetType(i));
	}
	_typeList->SetCurSel( g_conf.ed_object->GetInt() );
	ls->SetTabPos(1, 128);
	ls->AlignHeightToContent();
	_typeList->eventChangeCurSel.bind(&EditorLayout::OnChangeObject, this);
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

			// мы здесь не используем protect, поскольку отписываемся вручную
//			object->Subscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &EditorLayout::OnKillSelected, true, false);
//			object->Subscribe(NOTIFY_OBJECT_MOVE, this, (NOTIFYPROC) &EditorLayout::OnMoveSelected, false, false);

		//	_selectionRect->Adjust(object);
		//	_selectionRect->Show(true);

			_selectedObject = object;
			if( _selectedObject )
				_propList->ConnectTo(_selectedObject->GetProperties());
		}
	}
	else
	{
		_ASSERT(object == _selectedObject);
//		_selectedObject->Unsubscribe(this);
		_selectedObject = NULL;
	//	_selectionRect->Show(false);
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

bool EditorLayout::OnMouseDown(float x, float y, int button)
{
	vec2d mouse;

	if( g_level && GC_Camera::GetWorldMousePos(mouse) )
	{
		float align = Level::GetTypeInfo(g_conf.ed_object->GetInt()).align;
		float offset = Level::GetTypeInfo(g_conf.ed_object->GetInt()).offset;

		vec2d pt;

		pt.x = __min(g_level->_sx - align, __max(align - offset, mouse.x));
		pt.y = __min(g_level->_sy - align, __max(align - offset, mouse.y));
		pt.x -= fmodf(pt.x + align * 0.5f - offset, align) - align * 0.5f;
		pt.y -= fmodf(pt.y + align * 0.5f - offset, align) - align * 0.5f;

		if( GC_Object *object = g_level->PickEdObject(mouse) )
		{
			if( 1 == button )
			{
				if( _selectedObject == object )
				{
					object->EditorAction();
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
				Select(  g_level->CreateObject(
					Level::GetType(g_conf.ed_object->GetInt()), pt.x, pt.y), true  );
			}
		}
	}

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
		_propList->Show(true);
		break;
	default:
		GetParent()->OnRawChar(c);
	}
}

void EditorLayout::OnSize(float width, float height)
{
	_typeList->Move(width - _typeList->GetWidth() - 5, 5);
}

void EditorLayout::OnShow(bool show)
{
	if( !show )
	{
		_propList->ConnectTo(NULL);
		_selectedObject = NULL;
	}
}

void EditorLayout::OnChangeObject(int index)
{
	g_conf.ed_object->SetInt(index);
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
