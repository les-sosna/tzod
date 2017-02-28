#include "ScoreTable.h"

#include <ctx/GameContext.h>
#include <ctx/Deathmatch.h>
#include <gc/Player.h>
#include <gc/World.h>
#include <gc/Macros.h>
#include <loc/Language.h>
#include <ui/GuiManager.h>
#include <ui/LayoutContext.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>

#include <sstream>
#include <iomanip>

#define SCORE_POS_NUMBER     16
#define SCORE_POS_NAME       48
#define SCORE_POS_SCORE      16 // from the right side
#define SCORE_LIMITS_LEFT    96
#define SCORE_TIMELIMIT_TOP  16
#define SCORE_FRAGLIMIT_TOP  36
#define SCORE_NAMES_TOP      64
#define SCORE_ROW_HEIGHT     24

ScoreTable::ScoreTable(TextureManager &texman, World &world, const Deathmatch *deathmatch, LangCache &lang)
  : _font(texman.FindSprite("font_default"))
  , _texHighlight(texman.FindSprite("ui/selection"))
  , _world(world)
  , _deathmatch(deathmatch)
  , _lang(lang)
{
	Resize(420, 256);
	SetTexture(texman, "ui/list", false);
	SetDrawBorder(true);
}

void ScoreTable::Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman) const
{
	UI::Rectangle::Draw(dc, sc, lc, ic, rc, texman);

	std::vector<GC_Player*> players;
	FOREACH( _world.GetList(LIST_players), GC_Player, player )
	{
		players.push_back(player);
	}

	int max_score = 0;
	if( !players.empty() )
	{
		for( size_t i = players.size(); --i;)
		{
			for( size_t j = 0; j < i; ++j )
			{
				if( players[j]->GetScore() < players[j+1]->GetScore() )
				{
					std::swap(players[j+1], players[j]);
				}
			}
		}
		max_score = players[0]->GetScore();
	}

	if( _deathmatch && _deathmatch->GetTimeLimit() > 0 )
	{
		std::ostringstream text;
		int timeleft = (int)std::ceil(_deathmatch->GetTimeLimit() - _world.GetTime());
		if( timeleft > 0 )
			text << _lang.score_time_left.Get() << " " << (timeleft / 60) << ":" << std::setfill('0') << std::setw(2) << (timeleft % 60);
		else
			text << _lang.score_time_limit_hit.Get();
		rc.DrawBitmapText(ToPx(vec2d{ SCORE_LIMITS_LEFT, SCORE_TIMELIMIT_TOP }, lc), lc.GetScale(), _font, 0xffffffff, text.str());
	}

	if( _deathmatch && _deathmatch->GetFragLimit() > 0 )
	{
		std::ostringstream text;
		int scoreleft = _deathmatch->GetFragLimit() - max_score;
		if( scoreleft > 0 )
			text << _lang.score_frags_left.Get() << " " << scoreleft;
		else
			text << _lang.score_frag_limit_hit.Get();
		rc.DrawBitmapText(ToPx(vec2d{ SCORE_LIMITS_LEFT, SCORE_FRAGLIMIT_TOP }, lc), lc.GetScale(), _font, 0xffffffff, text.str());
	}

	const size_t maxLines = 8;

	float pxLineHeight = ToPx(texman.GetCharHeight(_font) - 1, lc);
	for (size_t i = 0; i < players.size(); ++i)
	{
		if (i == maxLines)
			break;

		vec2d pxOffset = vec2d{ 0, pxLineHeight * (float)i };
		if (players[i]->GetIsHuman())
		{
			vec2d lt = ToPx(vec2d{ SCORE_POS_NUMBER, SCORE_NAMES_TOP }, lc) + pxOffset;
			float pxWidth = lc.GetPixelSize().x - ToPx(SCORE_POS_SCORE, lc) - lt.x;
			vec2d pxMargin = ToPx(vec2d{ 3, 3 }, lc);
			FRECT pxRect = MakeRectWH(lt - pxMargin, vec2d{ pxWidth, pxLineHeight } + pxMargin * 2);
			rc.DrawSprite(pxRect, _texHighlight, 0xffffffff, 0);
			rc.DrawBorder(pxRect, _texHighlight, 0xffffffff, 0);
		}
	}

	for (size_t i = 0; i < players.size(); ++i)
	{
		vec2d pxOffset = vec2d{ 0, pxLineHeight * (float)i };
		if( i < maxLines)
		{
			rc.DrawBitmapText(ToPx(vec2d{ SCORE_POS_NAME, SCORE_NAMES_TOP }, lc) + pxOffset, lc.GetScale(), _font, 0xffffffff, players[i]->GetNick());

			std::ostringstream text;
			text << (int) (i + 1);
			rc.DrawBitmapText(ToPx(vec2d{ SCORE_POS_NUMBER, SCORE_NAMES_TOP }, lc) + pxOffset, lc.GetScale(), _font, 0xffffffff, text.str());
			text.str(std::string());
			text << players[i]->GetScore();
			rc.DrawBitmapText(vec2d{ lc.GetPixelSize().x - ToPx(SCORE_POS_SCORE, lc), ToPx(SCORE_NAMES_TOP, lc) } + pxOffset, lc.GetScale(), _font, 0xffffffff, text.str(), alignTextRT);
		}
		else
		{
			rc.DrawBitmapText(ToPx(vec2d{ SCORE_POS_NAME, SCORE_NAMES_TOP }, lc) + pxOffset, lc.GetScale(), _font, 0xffffffff, "...");
			break;
		}
	}
}
