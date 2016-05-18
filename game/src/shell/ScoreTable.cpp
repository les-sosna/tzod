#include "ScoreTable.h"

#include <ctx/GameContext.h>
#include <ctx/Deathmatch.h>
#include <gc/Player.h>
#include <gc/World.h>
#include <gc/Macros.h>
#include <loc/Language.h>
#include <ui/GuiManager.h>
#include <video/TextureManager.h>
#include <video/DrawingContext.h>

#include <sstream>
#include <iomanip>


#define SCORE_POS_NUMBER     16
#define SCORE_POS_NAME       48
#define SCORE_POS_SCORE      16 // from the right side
#define SCORE_LIMITS_LEFT    64
#define SCORE_TIMELIMIT_TOP  16
#define SCORE_FRAGLIMIT_TOP  36
#define SCORE_NAMES_TOP      64
#define SCORE_ROW_HEIGHT     24

ScoreTable::ScoreTable(UI::LayoutManager &manager, TextureManager &texman, World &world, Deathmatch &deathmatch, LangCache &lang)
  : UI::Window(manager)
  , _font(texman.FindSprite("font_default"))
  , _world(world)
  , _deathmatch(deathmatch)
  , _lang(lang)
{
	SetTexture(texman, "scoretbl", true);
	SetDrawBorder(false);
}

void ScoreTable::Draw(vec2d size, DrawingContext &dc, TextureManager &texman) const
{
	Window::Draw(size, dc, texman);

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

	if( _deathmatch.GetTimeLimit() > 0 )
	{
		std::ostringstream text;
		int timeleft = int(_deathmatch.GetTimeLimit() - _world.GetTime());
		if( timeleft > 0 )
			text << _lang.score_time_left.Get() << " " << (timeleft / 60) << ":" << std::setfill('0') << std::setw(2) << (timeleft % 60);
		else
			text << _lang.score_time_limit_hit.Get();
		dc.DrawBitmapText(SCORE_LIMITS_LEFT, SCORE_TIMELIMIT_TOP, _font, 0xffffffff, text.str());
	}

	if( _deathmatch.GetFragLimit() > 0 )
	{
		std::ostringstream text;
		int scoreleft = _deathmatch.GetFragLimit() - max_score;
		if( scoreleft > 0 )
			text << _lang.score_frags_left.Get() << " " << scoreleft;
		else
			text << _lang.score_frag_limit_hit.Get();
		dc.DrawBitmapText(SCORE_LIMITS_LEFT, SCORE_FRAGLIMIT_TOP, _font, 0xffffffff, text.str());
	}

	float h = texman.GetCharHeight(_font);
	for( size_t i = 0; i < players.size(); ++i )
	{
		if( i < 8 )
		{
			dc.DrawBitmapText(SCORE_POS_NAME, SCORE_NAMES_TOP + (h - 1) * (float) i, _font, 0xffffffff, players[i]->GetNick());

			std::ostringstream text;
			text << (int) (i + 1);
			dc.DrawBitmapText(SCORE_POS_NUMBER, SCORE_NAMES_TOP + (h - 1) * (float) i, _font, 0xffffffff, text.str());
			text.str(std::string());
			text << players[i]->GetScore();
			dc.DrawBitmapText(size.x - SCORE_POS_SCORE, SCORE_NAMES_TOP + (h - 1) * (float) i, _font, 0xffffffff, text.str(), alignTextRT);
		}
		else
		{
			dc.DrawBitmapText(SCORE_POS_NAME, SCORE_NAMES_TOP + (h - 1) * (float) i, _font, 0xffffffff, "......");
			break;
		}
	}
}
