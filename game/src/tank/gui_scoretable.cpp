// gui_scoretable.cpp

#include "gui_scoretable.h"

#include "gc/Player.h"
#include "gc/World.h"
#include "gc/Macros.h"
#include "config/Config.h"
#include "config/Language.h"

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

namespace UI
{

ScoreTable::ScoreTable(Window *parent, World &world)
  : Window(parent)
  , _font(GetManager().GetTextureManager().FindSprite("font_default"))
  , _world(world)
{
	SetTexture("scoretbl", true);
	SetDrawBorder(false);
}

void ScoreTable::OnParentSize(float width, float height)
{
	Move(std::floor((width - GetWidth()) / 2), std::floor((height - GetHeight()) / 2));
}

void ScoreTable::DrawChildren(DrawingContext &dc, float sx, float sy) const
{
	std::vector<GC_Player*> players;
	FOREACH( _world.GetList(LIST_players), GC_Player, player )
	{
		players.push_back(player);
	}

	int max_score = 0;
	if( !players.empty() )
	{
		for( int i = players.size(); --i;)
		{
			for( int j = 0; j < i; ++j )
			{
				if( players[j]->GetScore() < players[j+1]->GetScore() )
				{
					std::swap(players[j+1], players[j]);
				}
			}
		}
		max_score = players[0]->GetScore();
	}


	if( g_conf.sv_timelimit.GetFloat() )
	{
		std::ostringstream text;
		int timeleft = int(g_conf.sv_timelimit.GetFloat() * 60.0f - _world._time);
		if( timeleft > 0 )
			text << g_lang.score_time_left.Get() << " " << (timeleft / 60) << ":" << std::setfill('0') << std::setw(2) << (timeleft % 60);
		else
			text << g_lang.score_time_limit_hit.Get();
		dc.DrawBitmapText(sx + SCORE_LIMITS_LEFT, sy + SCORE_TIMELIMIT_TOP, _font, 0xffffffff, text.str());
	}

	if( g_conf.sv_fraglimit.GetInt() )
	{
		std::ostringstream text;
		int scoreleft = g_conf.sv_fraglimit.GetInt() - max_score;
		if( scoreleft > 0 )
			text << g_lang.score_frags_left.Get() << " " << scoreleft;
		else
			text << g_lang.score_frag_limit_hit.Get();
		dc.DrawBitmapText(sx + SCORE_LIMITS_LEFT, sy + SCORE_FRAGLIMIT_TOP, _font, 0xffffffff, text.str());
	}

	float h = GetManager().GetTextureManager().GetCharHeight(_font);
	for( size_t i = 0; i < players.size(); ++i )
	{
		if( i < 8 )
		{
			dc.DrawBitmapText(sx + SCORE_POS_NAME, sy + SCORE_NAMES_TOP + (h - 1) * (float) i, _font, 0xffffffff, players[i]->GetNick());

			std::ostringstream text;
			text << (int) (i + 1);
			dc.DrawBitmapText(sx + SCORE_POS_NUMBER, sy + SCORE_NAMES_TOP + (h - 1) * (float) i, _font, 0xffffffff, text.str());
			text.str(std::string());
			text << players[i]->GetScore();
			dc.DrawBitmapText(sx + GetWidth() - SCORE_POS_SCORE, sy + SCORE_NAMES_TOP + (h - 1) * (float) i, _font, 0xffffffff, text.str(), alignTextRT);
		}
		else
		{
			dc.DrawBitmapText(sx + SCORE_POS_NAME, sy + SCORE_NAMES_TOP + (h - 1) * (float) i, _font, 0xffffffff, "......");
			break;
		}
	}
}

} // end of namespace UI
