#include "inc/editor/Config.h"
#include "inc/editor/EditorMain.h"
#include "inc/editor/EditorWorldView.h"
#include "GameClassVis.h"
#include <cbind/ConfigBinding.h>
#include <ctx/EditorContext.h>
#include <gc/TypeSystem.h>
#include <loc/Language.h>
#include <plat/Keys.h>
#include <ui/Button.h>
#include <ui/DataSource.h>
#include <ui/DataSourceAdapters.h>
#include <ui/LayoutContext.h>
#include <ui/List.h>
#include <ui/ListBox.h>
#include <ui/Rectangle.h>
#include <ui/StackLayout.h>
#include <ui/Text.h>
#include <sstream>

namespace
{
	class LayerDisplay final
		: public UI::LayoutData<std::string_view>
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

	class HelpBox final
		: public UI::Window
	{
		TUPLE_CHILDREN(UI::Rectangle, UI::Text);
	public:
		explicit HelpBox(const LangCache& lang)
		{
			Resize(384, 256);
			std::get<0>(_children).SetTexture("ui/list");
			std::get<1>(_children).SetText(ConfBind(lang.f1_help_editor));
			std::get<1>(_children).SetAlign(alignTextLT);
		}

		UI::WindowLayout GetChildLayout(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, const UI::Window& child) const override
		{
			return UI::WindowLayout{ MakeRectWH(lc.GetPixelSize()), 1, true };
		}
	};
}

EditorMain::EditorMain(UI::TimeStepManager &manager,
                       TextureManager &texman,
                       std::shared_ptr<EditorContext> editorContext,
                       WorldView &worldView,
                       EditorConfig &conf,
                       LangCache &lang,
                       EditorCommands commands,
                       Plat::ConsoleBuffer &logger)
	: _conf(conf)
	, _lang(lang)
	, _commands(std::move(commands))
{
	_editorWorldView = std::make_shared<EditorWorldView>(manager, texman, editorContext, worldView, conf, lang, logger);
	AddFront(_editorWorldView);
	SetFocus(_editorWorldView.get());

	_helpBox = std::make_shared<HelpBox>(lang);
	_helpBox->SetVisible(false);
	AddFront(_helpBox);

	auto gameClassVis = std::make_shared<GameClassVis>(worldView);
	gameClassVis->Resize(64, 64);
	gameClassVis->SetGameClass(std::make_shared<UI::ListDataSourceBinding>(0));

	using namespace UI::DataSourceAliases;

	_modeSelect = std::make_shared<UI::CheckBox>();
	_modeSelect->SetText("Select"_txt);
	_modeSelect->eventClick = [] { assert("not implemented"); };

	_modeErase = std::make_shared<UI::CheckBox>();
	_modeErase->SetText("Erase"_txt);

	auto play = std::make_shared<UI::Button>();
	play->SetText("Play"_txt);
	play->SetWidth(64);
	play->eventClick = _commands.playMap;

	_typeSelector = std::make_shared<DefaultListBox>();
	_typeSelector->GetList()->SetItemTemplate(gameClassVis);
	for (unsigned int i = 0; i < RTTypes::Inst().GetTypeCount(); ++i)
	{
		if (!RTTypes::Inst().GetTypeInfoByIndex(i).service)
		{
			auto &typeInfo = RTTypes::Inst().GetTypeInfoByIndex(i);
			_typeSelector->GetData()->AddItem(typeInfo.name);
			_typeSelectorTypes.push_back(RTTypes::Inst().GetTypeByIndex(i));
		}
	}
	int selectedTypeIndex = std::min(_typeSelector->GetData()->GetItemCount() - 1, std::max(0, _conf.object.GetInt()));
	_typeSelector->GetList()->SetCurSel(selectedTypeIndex);
	_typeSelector->GetList()->eventChangeCurSel = std::bind(&EditorMain::OnSelectType, this, std::placeholders::_1);
	OnSelectType(selectedTypeIndex);

	_toolbar = std::make_shared<UI::StackLayout>();
	_toolbar->AddFront(play);
	_toolbar->AddFront(_modeSelect);
	_toolbar->AddFront(_modeErase);
	_toolbar->AddFront(_typeSelector);
	AddFront(_toolbar);

	_layerDisp = std::make_shared<UI::Text>();
	_layerDisp->SetAlign(alignTextRT);
	_layerDisp->SetText(std::make_shared<LayerDisplay>(_lang, _typeSelector->GetList()));
	AddFront(_layerDisp);

	assert(!_conf.uselayers.eventChange);
	_conf.uselayers.eventChange = std::bind(&EditorMain::OnChangeUseLayers, this);
	OnChangeUseLayers();
}

EditorMain::~EditorMain()
{
	_conf.uselayers.eventChange = nullptr;
}

void EditorMain::OnSelectType(int selectionIndex)
{
	_conf.object.SetInt(selectionIndex);
	if (selectionIndex != -1)
		_editorWorldView->SetCurrentType(_typeSelectorTypes[selectionIndex]);
}

void EditorMain::OnChangeUseLayers()
{
	_layerDisp->SetVisible(_conf.uselayers.Get());
}

void EditorMain::ChooseNextType()
{
	_typeSelector->GetList()->SetCurSel(std::clamp(_typeSelector->GetList()->GetCurSel() + 1,
		0, _typeSelector->GetList()->GetData()->GetItemCount() - 1));
}

void EditorMain::ChoosePrevType()
{
	_typeSelector->GetList()->SetCurSel(std::clamp(_typeSelector->GetList()->GetCurSel() - 1,
		0, _typeSelector->GetList()->GetData()->GetItemCount() - 1));
}

bool EditorMain::OnKeyPressed(const Plat::Input &input, const UI::InputContext &ic, Plat::Key key)
{
	switch (key)
	{
	case Plat::Key::F1:
		_helpBox->SetVisible(!_helpBox->GetVisible());
		break;
	case Plat::Key::F5:
	case Plat::Key::GamepadView:
		_commands.playMap();
		break;
	case Plat::Key::F9:
		_conf.uselayers.Set(!_conf.uselayers.Get());
		break;
	case Plat::Key::LeftBracket:
	case Plat::Key::GamepadLeftShoulder:
		ChoosePrevType();
		break;
	case Plat::Key::RightBracket:
	case Plat::Key::GamepadRightShoulder:
		ChooseNextType();
		break;

	default:
		return false;
	}
	return true;
}

bool EditorMain::CanNavigate(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, UI::Navigate navigate) const
{
	switch (navigate)
	{
	case UI::Navigate::Back:
		return _helpBox->GetVisible();

	default:
		return false;
	}
}

void EditorMain::OnNavigate(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, UI::Navigate navigate, UI::NavigationPhase phase)
{
	if (phase != UI::NavigationPhase::Started)
	{
		return;
	}

	switch (navigate)
	{
	case UI::Navigate::Back:
		if (_helpBox->GetVisible())
		{
			_helpBox->SetVisible(false);
		}
		break;
	default:
		break;
	}
}

UI::WindowLayout EditorMain::GetChildLayout(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const
{
	float scale = lc.GetScaleCombined();
	vec2d size = lc.GetPixelSize();

	if (_editorWorldView.get() == &child)
	{
		return UI::WindowLayout{ MakeRectWH(size.x - _toolbar->GetContentSize(texman, dc, scale, DefaultLayoutConstraints(lc)).x, size.y), 1, true };
	}
	if (_layerDisp.get() == &child)
	{
		return UI::WindowLayout{ UI::CanvasLayout(vec2d{ size.x / scale - 5, 6 }, _layerDisp->GetSize(), scale), 1, true };
	}
	if (_toolbar.get() == &child)
	{
		return UI::WindowLayout{ FRECT{ size.x - _toolbar->GetContentSize(texman, dc, scale, DefaultLayoutConstraints(lc)).x, 0, size.x, size.y }, 1, true };
	}
	if (_helpBox.get() == &child)
	{
		vec2d pxHelpBoxSize = _helpBox->GetContentSize(texman, dc, lc.GetScaleCombined(), DefaultLayoutConstraints(lc));
		return UI::WindowLayout{ MakeRectWH(Vec2dFloor((lc.GetPixelSize() - pxHelpBoxSize) / 2), pxHelpBoxSize), 1, true };
	}
	assert(false);
	return {};
}

vec2d EditorMain::GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const
{
	return layoutConstraints.maxPixelSize;
}

