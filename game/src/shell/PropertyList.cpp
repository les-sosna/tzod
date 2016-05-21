#include "PropertyList.h"
#include "inc/shell/Config.h"
#include <gc/Object.h>
#include <gc/TypeSystem.h>
#include <gc/WorldCfg.h>
#include <ui/Combo.h>
#include <ui/ConsoleBuffer.h>
#include <ui/DataSourceAdapters.h>
#include <ui/Edit.h>
#include <ui/List.h>
#include <ui/ListBase.h>
#include <ui/Scroll.h>
#include <ui/Text.h>
#include <ui/GuiManager.h>
#include <ui/Keys.h>
#include <video/TextureManager.h>
#include <algorithm>

PropertyList::Container::Container(UI::LayoutManager &manager)
  : UI::Window(manager)
{
}

PropertyList::PropertyList(UI::LayoutManager &manager, TextureManager &texman, float x, float y, float w, float h, World &world, ConfCache &conf, UI::ConsoleBuffer &logger)
  : Dialog(manager, texman, w, h, false)
  , _world(world)
  , _conf(conf)
  , _logger(logger)
{
	Move(x, y);
	_psheet = std::make_shared<Container>(manager);
	AddFront(_psheet);

	_scrollBar = std::make_shared<UI::ScrollBarVertical>(manager, texman);
	_scrollBar->SetHeight(h);
	_scrollBar->Move(w - _scrollBar->GetWidth(), 0);
	_scrollBar->eventScroll = std::bind(&PropertyList::OnScroll, this, std::placeholders::_1);
//	_scrollBar->SetLimit(100);
	AddFront(_scrollBar);

	OnSize(w, h);
	SetEasyMove(true);
	SetClipChildren(true);
}

void PropertyList::DoExchange(bool applyToObject, TextureManager &texman)
{
	int focus = -1;

	if( applyToObject )
	{
		assert(_ps);
		for( int i = 0; i < _ps->GetCount(); ++i )
		{
			ObjectProperty *prop = _ps->GetProperty(i);
			auto &ctrl = _ctrls[i];

			if( _psheet->GetFocus() == ctrl )
			{
				focus = i;
			}

			switch( prop->GetType() )
			{
			case ObjectProperty::TYPE_INTEGER:
				assert( dynamic_cast<UI::Edit*>(ctrl.get()) );
				int n;
				n = static_cast<UI::Edit*>(ctrl.get())->GetInt();
				if( n < prop->GetIntMin() || n > prop->GetIntMax() )
				{
					_logger.Printf(1, "WARNING: value %s out of range [%d, %d]",
						prop->GetName().c_str(), prop->GetIntMin(), prop->GetIntMax());
					n = std::max(prop->GetIntMin(), std::min(prop->GetIntMax(), n));
				}
				prop->SetIntValue(n);
				break;
			case ObjectProperty::TYPE_FLOAT:
				assert( dynamic_cast<UI::Edit*>(ctrl.get()) );
				float f;
				f = static_cast<UI::Edit*>(ctrl.get())->GetFloat();
				if( f < prop->GetFloatMin() || f > prop->GetFloatMax() )
				{
					_logger.Printf(1, "WARNING: value %s out of range [%g, %g]",
						prop->GetName().c_str(), prop->GetFloatMin(), prop->GetFloatMax());
					f = std::max(prop->GetFloatMin(), std::min(prop->GetFloatMax(), f));
				}
				prop->SetFloatValue(f);
				break;
			case ObjectProperty::TYPE_STRING:
				assert( dynamic_cast<UI::Edit*>(ctrl.get()) );
				prop->SetStringValue(static_cast<UI::Edit*>(ctrl.get())->GetText());
				break;
			case ObjectProperty::TYPE_MULTISTRING:
				assert( dynamic_cast<UI::ComboBox*>(ctrl.get()) );
				int index;
				index = static_cast<UI::ComboBox*>(ctrl.get())->GetCurSel();
				prop->SetCurrentIndex(index);
				break;
			case ObjectProperty::TYPE_SKIN:
			case ObjectProperty::TYPE_TEXTURE:
				assert( dynamic_cast<UI::ComboBox*>(ctrl.get()) );
				index = static_cast<UI::ComboBox*>(ctrl.get())->GetCurSel();
				prop->SetStringValue(static_cast<UI::ComboBox*>(ctrl.get())->GetData()->GetItemText(index, 0));
				break;
			default:
				assert(false);
			}
		}
		_ps->Exchange(_world, true);
	}


	// clear old controls
	_psheet->UnlinkAllChildren();
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

			std::ostringstream labelTextBuffer;
			labelTextBuffer << prop->GetName();

			auto label = UI::Text::Create(_psheet.get(), texman, 5, y, "", alignTextLT);
			y += label->GetHeight();
			y += 5;

			std::shared_ptr<Window> ctrl;

			switch( prop->GetType() )
			{
			case ObjectProperty::TYPE_INTEGER:
				ctrl = std::make_shared<UI::Edit>(GetManager(), texman);
				ctrl->Move(32, y);
				ctrl->SetWidth(_psheet->GetWidth() - 64);
				_psheet->AddFront(ctrl);
				std::static_pointer_cast<UI::Edit>(ctrl)->SetInt(prop->GetIntValue());
				labelTextBuffer << " (" << prop->GetIntMin() << " - " << prop->GetIntMax() << ")";
				break;
			case ObjectProperty::TYPE_FLOAT:
				ctrl = std::make_shared<UI::Edit>(GetManager(), texman);
				ctrl->Move(32, y);
				ctrl->SetWidth(_psheet->GetWidth() - 64);
				_psheet->AddFront(ctrl);
				std::static_pointer_cast<UI::Edit>(ctrl)->SetFloat(prop->GetFloatValue());
				labelTextBuffer << " (" << prop->GetFloatMin() << " - " << prop->GetFloatMax() << ")";
				break;
			case ObjectProperty::TYPE_STRING:
				ctrl = std::make_shared<UI::Edit>(GetManager(), texman);
				ctrl->Move(32, y);
				ctrl->SetWidth(_psheet->GetWidth() - 64);
				_psheet->AddFront(ctrl);
				std::static_pointer_cast<UI::Edit>(ctrl)->SetText(prop->GetStringValue());
				labelTextBuffer << " (string)";
				break;
			case ObjectProperty::TYPE_MULTISTRING:
			case ObjectProperty::TYPE_SKIN:
			case ObjectProperty::TYPE_TEXTURE:
				typedef UI::ListAdapter<UI::ListDataSourceDefault, UI::ComboBox> DefaultComboBox;
				ctrl = DefaultComboBox::Create(_psheet.get(), texman);
				ctrl->Move(32, y);
				static_cast<DefaultComboBox *>(ctrl.get())->Resize(_psheet->GetWidth() - 64);
				if (prop->GetType() == ObjectProperty::TYPE_MULTISTRING)
				{
					for( size_t index = 0; index < prop->GetListSize(); ++index )
					{
						static_cast<DefaultComboBox *>(ctrl.get())->GetData()->AddItem(prop->GetListValue(index));
					}
					static_cast<DefaultComboBox*>(ctrl.get())->SetCurSel(prop->GetCurrentIndex());
				}
				else
				{
					std::vector<std::string> names;
					const char *filter = prop->GetType() == ObjectProperty::TYPE_SKIN ? "skin/" : nullptr;
					texman.GetTextureNames(names, filter, true);
					for( auto &name: names )
					{
						// only allow using textures which are less than half of a cell
						const LogicalTexture &lt = texman.GetSpriteInfo(texman.FindSprite(name));
						if( lt.pxFrameWidth <= LOCATION_SIZE / 2 && lt.pxFrameHeight <= LOCATION_SIZE / 2 )
						{
							int index = static_cast<DefaultComboBox *>(ctrl.get())->GetData()->AddItem(name);
							if (name == prop->GetStringValue())
								static_cast<DefaultComboBox*>(ctrl.get())->SetCurSel(index);
						}
					}
				}
				static_cast<DefaultComboBox*>(ctrl.get())->GetList()->AlignHeightToContent();
				break;
			default:
				assert(false);
			} // end of switch( prop->GetType() )

			label->SetText(labelTextBuffer.str());

			if( focus == i )
			{
				if(auto edit = std::dynamic_pointer_cast<UI::Edit>(ctrl) )
				{
					edit->SetSel(0, -1);
				}
				_psheet->SetFocus(ctrl);
			}

			assert(nullptr != ctrl);
			_ctrls.push_back(ctrl);
			y += ctrl->GetHeight();
			y += 10;
		}
	}
	_psheet->Resize(_psheet->GetWidth(), y);
	_scrollBar->SetDocumentSize(y);
	OnScroll(_scrollBar->GetPos());
}

void PropertyList::ConnectTo(std::shared_ptr<PropertySet> ps, TextureManager &texman)
{
	if( _ps == ps ) return;
	_ps = std::move(ps);
	DoExchange(false, texman);
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

bool PropertyList::OnKeyPressed(UI::InputContext &ic, UI::Key key)
{
	switch(key)
	{
	case UI::Key::Enter:
		DoExchange(true, GetManager().GetTextureManager());
		SaveToConfig(_conf, *_ps);
		break;
	case UI::Key::Escape:
		_conf.ed_showproperties.Set(false);
		SetVisible(false);
		break;
	default:
		return Dialog::OnKeyPressed(ic, key);
	}
	return true;
}

void PropertyList::OnMouseWheel(float x, float y, float z)
{
	_scrollBar->SetPos(_scrollBar->GetPos() - z * 10.0f);
	OnScroll(_scrollBar->GetPos());
}

///////////////////////////////////////////////////////////////////////////////

void SaveToConfig(ConfCache &conf, const PropertySet &ps)
{
	ConfVarTable &op = conf.ed_objproperties.GetTable(RTTypes::Inst().GetTypeName(ps.GetObject()->GetType()));
	for (int i = 0; i < ps.GetCount(); ++i)
	{
		const ObjectProperty *prop = const_cast<PropertySet&>(ps).GetProperty(i);
		switch (prop->GetType())
		{
		case ObjectProperty::TYPE_INTEGER:
			op.SetNum(prop->GetName(), prop->GetIntValue());
			break;
		case ObjectProperty::TYPE_FLOAT:
			op.SetNum(prop->GetName(), prop->GetFloatValue());
			break;
		case ObjectProperty::TYPE_STRING:
		case ObjectProperty::TYPE_SKIN:
		case ObjectProperty::TYPE_TEXTURE:
			op.SetStr(prop->GetName(), prop->GetStringValue());
			break;
		case ObjectProperty::TYPE_MULTISTRING:
			op.SetNum(prop->GetName(), (int)prop->GetCurrentIndex());
			break;
		default:
			assert(false);
		}
	}
}

void LoadFromConfig(const ConfCache &conf, PropertySet &ps)
{
	ConfVarTable &op = conf.ed_objproperties.GetTable(RTTypes::Inst().GetTypeName(ps.GetObject()->GetType()));
	for (int i = 0; i < ps.GetCount(); ++i)
	{
		ObjectProperty *prop = ps.GetProperty(i);
		switch (prop->GetType())
		{
		case ObjectProperty::TYPE_INTEGER:
			prop->SetIntValue(std::min(prop->GetIntMax(),
				std::max(prop->GetIntMin(),
					op.GetNum(prop->GetName(), prop->GetIntValue()).GetInt())));
			break;
		case ObjectProperty::TYPE_FLOAT:
			prop->SetFloatValue(std::min(prop->GetFloatMax(),
				std::max(prop->GetFloatMin(),
					op.GetNum(prop->GetName(), prop->GetFloatValue()).GetFloat())));
			break;
		case ObjectProperty::TYPE_STRING:
		case ObjectProperty::TYPE_SKIN:
		case ObjectProperty::TYPE_TEXTURE:
			prop->SetStringValue(op.GetStr(prop->GetName(), prop->GetStringValue().c_str()).Get());
			break;
		case ObjectProperty::TYPE_MULTISTRING:
			prop->SetCurrentIndex(std::min((int)prop->GetListSize() - 1,
				std::max(0, op.GetNum(prop->GetName(), (int)prop->GetCurrentIndex()).GetInt())));
			break;
		default:
			assert(false);
		}
	}
}

