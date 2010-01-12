// gui_scoretable.cpp

#include "stdafx.h"
#include "GuiManager.h"
#include "gui_scoretable.h"

#include "video/TextureManager.h"

#include "gc/Player.h"

#include "config/Config.h"
#include "config/Language.h"

#include "Level.h"
#include "Macros.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

ScoreTable::ScoreTable(Window *parent)
  : Window(parent)
  , _font(GetManager()->GetTextureManager()->FindSprite("font_default"))
{
	SetTexture("scoretbl", true);
	SetDrawBorder(false);
	SetTimeStep(true);
}

void ScoreTable::OnParentSize(float width, float height)
{
	Move((width - GetWidth()) * 0.5f, (height - GetHeight()) * 0.5f);
}

void ScoreTable::DrawChildren(const DrawingContext *dc, float sx, float sy) const
{
	assert(g_level);

	std::vector<GC_Player*> players;
	FOREACH( g_level->GetList(LIST_players), GC_Player, player )
	{
		if( player->IsKilled() ) continue;
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
		int timeleft = int(g_conf.sv_timelimit.GetFloat() * 60.0f - g_level->_time);
		if( timeleft > 0 )
		{
			wsprintf(text, g_lang.score_time_left_xx.Get().c_str(), timeleft / 60, timeleft % 60);
		}
		else
			wsprintf(text, g_lang.score_time_limit_hit.Get().c_str());
		dc->DrawBitmapText(sx + SCORE_LIMITS_LEFT, sy + SCORE_TIMELIMIT_TOP, _font, 0xffffffff, text);
	}

	if( g_conf.sv_fraglimit.GetInt() )
	{
		int scoreleft = g_conf.sv_fraglimit.GetInt() - max_score;
		if( scoreleft > 0 )
			wsprintf(text, g_lang.score_frags_left_x.Get().c_str(), scoreleft);
		else
			wsprintf(text, g_lang.score_frag_limit_hit.Get().c_str());
		dc->DrawBitmapText(sx + SCORE_LIMITS_LEFT, sy + SCORE_FRAGLIMIT_TOP, _font, 0xffffffff, text);
	}

	float h = dc->GetCharHeight(_font);
	for( size_t i = 0; i < players.size(); ++i )
	{
		if( i < 8 )
		{
			dc->DrawBitmapText(sx + SCORE_POS_NAME, sy + SCORE_NAMES_TOP + (h - 1) * (float) i, _font, 0xffffffff, players[i]->GetNick());

			wsprintf(text, "%d", i + 1);
			dc->DrawBitmapText(sx + SCORE_POS_NUMBER, sy + SCORE_NAMES_TOP + (h - 1) * (float) i, _font, 0xffffffff, text);

			wsprintf(text, "%d", players[i]->GetScore());
			dc->DrawBitmapText(sx + GetWidth() - SCORE_POS_SCORE, sy + SCORE_NAMES_TOP + (h - 1) * (float) i, _font, 0xffffffff, text, alignTextRT);
		}
		else
		{
			dc->DrawBitmapText(sx + SCORE_POS_NAME, sy + SCORE_NAMES_TOP + (h - 1) * (float) i, _font, 0xffffffff, "......");
			break;
		}
	}
}

void ScoreTable::OnTimeStep(float dt)
{
	bool tab = g_env.envInputs.IsKeyPressed(DIK_TAB);
	SetVisible(!g_level->IsEmpty() && !g_level->GetEditorMode() && (tab || g_level->_limitHit));
}


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

