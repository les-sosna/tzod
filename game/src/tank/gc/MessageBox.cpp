// MessageBox.cpp

#include "stdafx.h"

#include "MessageBox.h"
#include "Level.h"
#include "script.h"

#include "ui/gui.h"
#include "ui/GuiManager.h"

#include "fs/SaveFile.h"
#include "fs/MapFile.h"

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_MessageBox)
{
	ED_SERVICE("msgbox", "obj_service_msgbox");
	return true;
}

GC_MessageBox::GC_MessageBox()
  : _msgbox(NULL)
  , _option1("OK")
  , _autoClose(1)
{
	SafePtr<PropertySet> ps(NewPropertySet());
	ps->Exchange(false);
	ps->Exchange(true);
}

GC_MessageBox::GC_MessageBox(FromFile)
  : _msgbox(NULL)
{
}

GC_MessageBox::~GC_MessageBox()
{
	assert(!_msgbox.Get());
}

void GC_MessageBox::Kill()
{
	if( _msgbox.Get() )
		_msgbox->Destroy();
	GC_Service::Kill();
}

void GC_MessageBox::Serialize(SaveFile &f)
{
	GC_Service::Serialize(f);
	f.Serialize(_title);
	f.Serialize(_text);
	f.Serialize(_option1);
	f.Serialize(_option2);
	f.Serialize(_option3);
	f.Serialize(_scriptOnSelect);
	f.Serialize(_autoClose);
	if( f.loading() )
	{
		SafePtr<PropertySet> ps(NewPropertySet());
		ps->Exchange(false);
		ps->Exchange(true);
	}
}

void GC_MessageBox::MapExchange(MapFile &f)
{
	GC_Service::MapExchange(f);
	MAP_EXCHANGE_STRING(title, _title, "");
	MAP_EXCHANGE_STRING(text, _text, "");
	MAP_EXCHANGE_STRING(option1, _option1, "");
	MAP_EXCHANGE_STRING(option2, _option2, "");
	MAP_EXCHANGE_STRING(option3, _option3, "");
	MAP_EXCHANGE_STRING(on_select, _scriptOnSelect, "");
	MAP_EXCHANGE_INT(auto_close, _autoClose, 1);
	if( f.loading() )
	{
		SafePtr<PropertySet> ps(NewPropertySet());
		ps->Exchange(false);
		ps->Exchange(true);
	}
}

void GC_MessageBox::OnSelect(int n)
{
	std::stringstream buf;
	buf << "return function(self,n)";
	buf << _scriptOnSelect;
	buf << "\nend";

	if( luaL_loadstring(g_env.L, buf.str().c_str()) )
	{
		GetConsole().Printf(1, "syntax error %s", lua_tostring(g_env.L, -1));
		lua_pop(g_env.L, 1); // pop the error message from the stack
	}
	else
	{
		if( lua_pcall(g_env.L, 0, 1, 0) )
		{
			GetConsole().WriteLine(1, lua_tostring(g_env.L, -1));
			lua_pop(g_env.L, 1); // pop the error message from the stack
		}
		else
		{
			SafePtr<GC_Object> refHolder(this);
			luaT_pushobject(g_env.L, this);
			lua_pushinteger(g_env.L, n);
			if( lua_pcall(g_env.L, 2, 0, 0) )
			{
				GetConsole().WriteLine(1, lua_tostring(g_env.L, -1));
				lua_pop(g_env.L, 1); // pop the error message from the stack
			}
			else if( !IsKilled() && _autoClose )
			{
				Kill();
			}
		}
	}
}

PropertySet* GC_MessageBox::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_MessageBox::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propTitle(ObjectProperty::TYPE_STRING, "title")
  , _propText(ObjectProperty::TYPE_STRING, "text")
  , _propOption1(ObjectProperty::TYPE_STRING, "option1")
  , _propOption2(ObjectProperty::TYPE_STRING, "option2")
  , _propOption3(ObjectProperty::TYPE_STRING, "option3")
  , _propOnSelect(ObjectProperty::TYPE_STRING, "on_select")
  , _propAutoClose(ObjectProperty::TYPE_INTEGER, "auto_close")
{
	_propAutoClose.SetIntRange(0, 1);
}

int GC_MessageBox::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 7;
}

ObjectProperty* GC_MessageBox::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	default: assert(false);
	case 0: return &_propTitle;
	case 1: return &_propText;
	case 2: return &_propOption1;
	case 3: return &_propOption2;
	case 4: return &_propOption3;
	case 5: return &_propOnSelect;
	case 6: return &_propAutoClose;
	}
}

void GC_MessageBox::MyPropertySet::MyExchange(bool applyToObject)
{
	BASE::MyExchange(applyToObject);

	GC_MessageBox *tmp = static_cast<GC_MessageBox *>(GetObject());

	if( applyToObject )
	{
		tmp->_title = _propTitle.GetStringValue();
		tmp->_text = _propText.GetStringValue();
		tmp->_option1 = _propOption1.GetStringValue();
		tmp->_option2 = _propOption2.GetStringValue();
		tmp->_option3 = _propOption3.GetStringValue();
		tmp->_scriptOnSelect = _propOnSelect.GetStringValue();
		tmp->_autoClose = (0 != _propAutoClose.GetIntValue());

		if( tmp->_msgbox.Get() )
		{
			tmp->_msgbox->Destroy();
		}

		UI::ScriptMessageBox *mb = new UI::ScriptMessageBox(g_gui->GetDesktop(), 
			tmp->_title, tmp->_text, tmp->_option1, tmp->_option2, tmp->_option3);
		mb->eventSelect.bind(&GC_MessageBox::OnSelect, tmp);
		tmp->_msgbox.Set(mb);
	}
	else
	{
		_propTitle.SetStringValue(tmp->_title);
		_propText.SetStringValue(tmp->_text);
		_propOption1.SetStringValue(tmp->_option1);
		_propOption2.SetStringValue(tmp->_option2);
		_propOption3.SetStringValue(tmp->_option3);
		_propOnSelect.SetStringValue(tmp->_scriptOnSelect);
		_propAutoClose.SetIntValue(0 != tmp->_autoClose);
	}
}


// end of file
