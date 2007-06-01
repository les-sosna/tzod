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

#include "Level.h"

namespace UI
{

///////////////////////////////////////////////////////////////////////////////
// PropertyList class implementation

PropertyList::PropertyList(Window *parent, float x, float y, float w, float h)
  : Dialog(parent, x, y, w, h)
{
	_psheet = new Window(this);

	_scrollBar = new ScrollBar(this, 0, 0, h);
	_scrollBar->Move(w - _scrollBar->GetWidth(), 0);
	_scrollBar->eventScroll.bind(&PropertyList::OnScroll, this);
	_scrollBar->SetLimit(100);

	Resize(w, h);
	SetEasyMove(true);
	ClipChildren(true);
}

void PropertyList::ConnectTo(const SafePtr<IPropertySet> &ps)
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
	_proplist = new PropertyList(this, 10, 10, 256, 256);
}

void EditorLayout::OnKillSelected(GC_Object *sender, void *param)
{
	Select(sender, false);
}

void EditorLayout::OnMoveSelected(GC_Object *sender, void *param)
{
	_ASSERT(_selectedObject == sender);
//	_selectionRect->Adjust(static_cast<GC_2dSprite *>(sender));
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
				_proplist->ConnectTo(_selectedObject->GetProperties());
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

bool EditorLayout::OnMouseDown(float x, float y, int button)
{
	vec2d mouse;

	if( g_level && GC_Camera::GetWorldMousePos(mouse) )
	{
		if( GC_Object *object = g_level->PickEdObject(mouse) )
		{
			Select(object, true);
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
		_proplist->Show(true);
		break;
	default:
		GetParent()->OnRawChar(c);
	}
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
