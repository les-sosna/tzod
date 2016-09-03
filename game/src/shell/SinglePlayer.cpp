#include "BotView.h"
#include "ConfigBinding.h"
#include "MapPreview.h"
#include "SinglePlayer.h"
#include "inc/shell/Config.h"
#include <MapFile.h>
#include <fs/FileSystem.h>
#include <gc/World.h>
#include <loc/Language.h>
#include <render/WorldView.h>
#include <video/DrawingContext.h>
#include <ui/Button.h>
#include <ui/DataSource.h>
#include <ui/LayoutContext.h>
#include <ui/List.h>
#include <ui/StackLayout.h>
#include <ui/Text.h>

static const float c_tileSize = 180;
static const float c_tileSpacing = 16;

using namespace UI::DataSourceAliases;

SinglePlayer::SinglePlayer(UI::LayoutManager &manager, TextureManager &texman, WorldView &worldView, FS::FileSystem &fs, ConfCache &conf, LangCache &lang)
	: UI::Dialog(manager, texman)
	, _conf(conf)
	, _content(std::make_shared<UI::StackLayout>(manager))
	, _tierTitle(std::make_shared<UI::Text>(manager, texman))
	, _tiles(std::make_shared<UI::List>(manager, texman, &_tilesSource))
	, _enemiesTitle(std::make_shared<UI::Text>(manager, texman))
{
	_tierTitle->SetFont(texman, "font_default");
	_tierTitle->SetText("Tier 1"_txt);
	_content->AddFront(_tierTitle);

	std::vector<std::string> maps = { "dm1", "dm2", "dm3", "dm4" };
	for (auto &mapName: maps)
	{
		_tilesSource.AddItem(mapName);
	}

	auto mp = std::make_shared<MapPreview>(manager, texman, fs, worldView, _mapCache);
	mp->Resize(c_tileSize, c_tileSize);
	mp->SetPadding(c_tileSpacing / 2);
	mp->SetMapName(std::make_shared<UI::ListDataSourceBinding>(0));

	_tiles->SetItemTemplate(mp);
	_tiles->SetFlowDirection(UI::FlowDirection::Horizontal);
	_content->AddFront(_tiles);

	_enemiesTitle->SetFont(texman, "font_default");
	_enemiesTitle->SetText("Enemies"_txt);
	_content->AddFront(_enemiesTitle);

	_enemies = std::make_shared<UI::StackLayout>(manager);
	_enemies->SetFlowDirection(UI::FlowDirection::Horizontal);
	_content->AddFront(_enemies);

	for (size_t i = 0; i < _conf.dm_bots.GetSize(); ++i)
	{
		auto botView = std::make_shared<BotView>(manager, texman);
		botView->SetBotConfig(_conf.dm_bots.GetAt(i).AsTable(), texman);
		botView->Resize(64, 64);
		_enemies->AddFront(botView);
	}

	auto buttons = std::make_shared<UI::StackLayout>(manager);
	buttons->SetFlowDirection(UI::FlowDirection::Horizontal);
	buttons->SetSpacing(c_tileSpacing);
	_content->AddFront(buttons);

	auto btn = std::make_shared<UI::Button>(manager, texman);
	btn->SetText(ConfBind(lang.dm_ok));
	btn->eventClick = std::bind(&SinglePlayer::OnOK, this);
	buttons->AddFront(btn);

	btn = std::make_shared<UI::Button>(manager, texman);
	btn->SetText(ConfBind(lang.dm_cancel));
	btn->eventClick = std::bind(&SinglePlayer::OnCancel, this);
	buttons->AddFront(btn);

	_content->SetSpacing(c_tileSpacing);
	AddFront(_content);

	Resize(800, 400);
}

void SinglePlayer::OnOK()
{
	int index = _tiles->GetCurSel();
	if (-1 != index)
	{
		_conf.cl_map.Set(_tilesSource.GetItemText(index, 0));
		Close(_resultOK);
	}
}

void SinglePlayer::OnCancel()
{
	Close(_resultCancel);
}

FRECT SinglePlayer::GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::StateContext &sc, const UI::Window &child) const
{
	if (_content.get() == &child)
	{
		vec2d pxMargins = UI::ToPx(vec2d{ c_tileSpacing, c_tileSpacing }, lc);
		return MakeRectRB(pxMargins, lc.GetPixelSize() - pxMargins);
	}

	return UI::Dialog::GetChildRect(texman, lc, sc, child);
}

vec2d SinglePlayer::GetContentSize(TextureManager &texman, const UI::StateContext &sc, float scale) const
{
	vec2d pxMargins = UI::ToPx(vec2d{ c_tileSpacing, c_tileSpacing }, scale);
	return _content->GetContentSize(texman, sc, scale) + pxMargins * 2;
}
