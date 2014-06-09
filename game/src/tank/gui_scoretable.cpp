// gui_scoretable.cpp

#include "gui_scoretable.h"

#include "constants.h"
#include "globals.h"
#include "Macros.h"

#include "video/TextureManager.h"
#include "gc/Player.h"
#include "gc/World.h"
#include "config/Config.h"
#include "config/Language.h"

#include <GuiManager.h>
#include <GLFW/glfw3.h>

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

ScoreTable::ScoreTable(Window *parent, World &world)
  : Window(parent)
  , _font(GetManager()->GetTextureManager().FindSprite("font_default"))
  , _world(world)
{
	SetTexture("scoretbl", true);
	SetDrawBorder(false);
	SetTimeStep(true);
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


	char text[256];
	if( g_conf.sv_timelimit.GetFloat() )
	{
		int timeleft = int(g_conf.sv_timelimit.GetFloat() * 60.0f - _world._time);
		if( timeleft > 0 )
		{
			sprintf(text, g_lang.score_time_left_xx.Get().c_str(), timeleft / 60, timeleft % 60);
		}
		else
			sprintf(text, "%s", g_lang.score_time_limit_hit.Get().c_str());
		dc.DrawBitmapText(sx + SCORE_LIMITS_LEFT, sy + SCORE_TIMELIMIT_TOP, _font, 0xffffffff, text);
	}

	if( g_conf.sv_fraglimit.GetInt() )
	{
		int scoreleft = g_conf.sv_fraglimit.GetInt() - max_score;
		if( scoreleft > 0 )
			sprintf(text, g_lang.score_frags_left_x.Get().c_str(), scoreleft);
		else
			sprintf(text, "%s", g_lang.score_frag_limit_hit.Get().c_str());
		dc.DrawBitmapText(sx + SCORE_LIMITS_LEFT, sy + SCORE_FRAGLIMIT_TOP, _font, 0xffffffff, text);
	}

	float h = dc.GetCharHeight(_font);
	for( size_t i = 0; i < players.size(); ++i )
	{
		if( i < 8 )
		{
			dc.DrawBitmapText(sx + SCORE_POS_NAME, sy + SCORE_NAMES_TOP + (h - 1) * (float) i, _font, 0xffffffff, players[i]->GetNick());

			sprintf(text, "%d", (int) (i + 1));
			dc.DrawBitmapText(sx + SCORE_POS_NUMBER, sy + SCORE_NAMES_TOP + (h - 1) * (float) i, _font, 0xffffffff, text);

			sprintf(text, "%d", players[i]->GetScore());
			dc.DrawBitmapText(sx + GetWidth() - SCORE_POS_SCORE, sy + SCORE_NAMES_TOP + (h - 1) * (float) i, _font, 0xffffffff, text, alignTextRT);
		}
		else
		{
			dc.DrawBitmapText(sx + SCORE_POS_NAME, sy + SCORE_NAMES_TOP + (h - 1) * (float) i, _font, 0xffffffff, "......");
			break;
		}
	}
}

void ScoreTable::OnTimeStep(float dt)
{
	bool tab = GLFW_PRESS == glfwGetKey(g_appWindow, GLFW_KEY_TAB);
	SetVisible(!_world.IsEmpty() && (tab || _world._limitHit));
}


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

