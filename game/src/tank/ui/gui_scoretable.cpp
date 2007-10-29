// gui_scoretable.cpp

#include "stdafx.h"
#include "gui_scoretable.h"

#include "Text.h"

#include "gc/Player.h"

#include "config/Config.h"

#include "Level.h"
#include "Macros.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

ScoreTable::ScoreTable(Window *parent)
  : Window(parent, 0, 0, "scoretbl")
{
	SetBorder(false);

	_text = new Text(this, 0, 0, "", alignTextLT);
	_text->SetTexture("font_default");
	_text->Resize(_text->GetTextureWidth(), _text->GetTextureHeight());
}

ScoreTable::~ScoreTable()
{
}

void ScoreTable::Refresh()
{
	_ASSERT(g_level);

	_players.clear();
	FOREACH( g_level->GetList(LIST_players), GC_Player, player )
	{
		if( player->IsKilled() ) continue;
		_players.push_back( PlayerDesc() );
		_players.back().score = player->GetScore();
		strcpy(_players.back().nick, player->GetNick().c_str());
	}

	if( _players.empty() ) return;

	for( int i = _players.size(); --i;)
	{
		for( int j = 0; j < i; ++j )
		{
			if( _players[j].score < _players[j+1].score )
			{
				PlayerDesc tmp = _players[j+1];
				_players[j+1]  = _players[j];
				_players[j]    = tmp;
			}
		}
	}
}

void ScoreTable::OnParentSize(float width, float height)
{
	Move((width - GetWidth()) * 0.5f, (height - GetHeight()) * 0.5f);
}

void ScoreTable::DrawChildren(float sx, float sy)
{
	if( !g_level )
	{
		return;
	}

	Refresh();

	char text[256];
	if( g_conf.sv_timelimit->GetFloat() )
	{
		int timeleft = int(g_conf.sv_timelimit->GetFloat() * 60.0f - g_level->_time);
		if( timeleft > 0 )
		{
			if( timeleft % 60 < 10 )
				wsprintf(text, "Осталось времени %d:0%d", timeleft / 60, timeleft % 60);
			else
				wsprintf(text, "Осталось времени %d:%d", timeleft / 60, timeleft % 60);
		}
		else
			wsprintf(text, "Достигнут лимит времени");

		_text->SetText(text);
		_text->SetAlign(alignTextLT);
		_text->Draw(sx + SCORE_LIMITS_LEFT, sy + SCORE_TIMELIMIT_TOP);
	}

	if( g_conf.sv_fraglimit->GetInt() )
	{
		int max_score = _players.empty() ? 0 : _players[0].score;
		for( size_t i = 0; i < _players.size(); ++i )
		{
			if( _players[i].score > max_score )
				max_score = _players[i].score;
		}
		int scoreleft = g_conf.sv_fraglimit->GetInt() - max_score;
		if( scoreleft > 0 )
			wsprintf(text, "Осталось фрагов  %d", scoreleft);
		else
			wsprintf(text, "Достигнут лимит фрагов");

		_text->SetText(text);
		_text->SetAlign(alignTextLT);
		_text->Draw(sx + SCORE_LIMITS_LEFT, sy + SCORE_FRAGLIMIT_TOP);
	}

	for( size_t i = 0; i < _players.size(); ++i )
	{
		if( i < 8 )
		{
			wsprintf(text, "%d", i + 1);
			_text->SetText(text);
			_text->SetAlign(alignTextLT);
			_text->Draw(sx + SCORE_POS_NUMBER, sy + (float) (SCORE_NAMES_TOP + (_text->GetTextureHeight() - 1) * i));

			_text->SetText(_players[i].nick);
			_text->Draw(sx + SCORE_POS_NAME, sy + (float) (SCORE_NAMES_TOP + (_text->GetTextureHeight() - 1) * i));

			wsprintf(text, "%d", _players[i].score);
			_text->SetText(text);
			_text->SetAlign(alignTextRT);
			_text->Draw(sx + (float) (GetWidth() - SCORE_POS_SCORE),
				sy + (float) (SCORE_NAMES_TOP + (_text->GetTextureHeight() - 1) * i));
		}
		else
		{
			_text->SetAlign(alignTextLT);
			_text->SetText("......");
			_text->Draw(sx + SCORE_POS_NAME, sy + (float) (SCORE_NAMES_TOP + (_text->GetTextureHeight() - 1) * i));
			break;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

