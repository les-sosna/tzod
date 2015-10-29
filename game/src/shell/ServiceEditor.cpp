#include "ServiceEditor.h"
#include "inc/shell/Config.h"
#include <gc/Player.h>
#include <gc/TypeSystem.h>
#include <gc/World.h>
#include <loc/Language.h>
#include <ui/Button.h>
#include <ui/DataSourceAdapters.h>
#include <ui/Combo.h>
#include <ui/List.h>
#include <ui/Text.h>

#include <GLFW/glfw3.h>

#include "Editor.h" // WTF! cross-reference

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
	for (int i = 0; i < index; ++i)
	{
		it = _world.GetList(LIST_services).next(it);
		assert(_world.GetList(LIST_services).end() != it);
	}
	return (size_t)_world.GetList(LIST_services).at(it);
}

const std::string& ServiceListDataSource::GetItemText(int index, int sub) const
{
	GC_Object *s = (GC_Object *)GetItemData(index);
	const char *name;
	switch (sub)
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
		for (auto it = list.begin(); it != list.end(); it = list.next(it))
		{
			if (list.at(it) == &obj)
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
	_labelName = UI::Text::Create(this, w / 2, _margins, g_lang.service_name.Get(), alignTextLT);

	_list = UI::List::Create(this, &_listData, 0, 0, 0, 0);
	_list->Move(_margins, _margins + _labelService->GetY() + _labelService->GetHeight());
	_list->SetDrawBorder(true);
	_list->eventChangeCurSel = std::bind(&ServiceEditor::OnSelectService, this, std::placeholders::_1);

	_btnCreate = UI::Button::Create(this, g_lang.service_create.Get(), 0, 0);
	_btnCreate->eventClick = std::bind(&ServiceEditor::OnCreateService, this);

	_combo = DefaultComboBox::Create(this);
	_combo->Move(_margins, _margins);
	for (int i = 0; i < RTTypes::Inst().GetTypeCount(); ++i)
	{
		if (RTTypes::Inst().GetTypeInfoByIndex(i).service)
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
	if (obj)
	{
		for (int i = 0; i < _list->GetData()->GetItemCount(); ++i)
		{
			if (_list->GetData()->GetItemData(i) == (size_t)obj)
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
	if (-1 != _combo->GetCurSel())
	{
		ObjectType type = (ObjectType)_combo->GetData()->GetItemData(_combo->GetCurSel());
		GC_Service &service = RTTypes::Inst().CreateService(_world, type);
		GetEditorLayout()->SelectNone();
		GetEditorLayout()->Select(&service, true);
	}
}

void ServiceEditor::OnSelectService(int i)
{
	if (-1 == i)
	{
		GetEditorLayout()->SelectNone();
	}
	else
	{
		GetEditorLayout()->Select((GC_Object *)_list->GetData()->GetItemData(i), true);
	}
}

void ServiceEditor::OnSize(float width, float height)
{
	_btnCreate->Move(width - _btnCreate->GetWidth() - _margins,
		height - _btnCreate->GetHeight() - _margins);

	_combo->Resize(width - _margins * 3 - _btnCreate->GetWidth());
	_combo->Move(_margins, _btnCreate->GetY() + floor((_btnCreate->GetHeight() - _combo->GetHeight()) / 2 + 0.5f));

	_list->Resize(width - 2 * _list->GetX(), _btnCreate->GetY() - _list->GetY() - _margins);
	_list->SetTabPos(1, floor(_list->GetWidth() / 2));
}

bool ServiceEditor::OnRawChar(int c)
{
	switch (c)
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
