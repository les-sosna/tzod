// gui_network.cpp

#include "stdafx.h"

#include "gui_network.h"
#include "gui_maplist.h"

#include "GuiManager.h"

#include "Text.h"
#include "Edit.h"
#include "Button.h"

#include "config/Config.h"


namespace UI
{
///////////////////////////////////////////////////////////////////////////////

CreateServerDlg::CreateServerDlg(Window *parent)
  : Dialog(parent, 0, 0, 770, 550)
{
	Move( (parent->GetWidth() - GetWidth()) / 2, (parent->GetHeight() - GetHeight()) / 2 );


	float x1 = 16;
	float x2 = x1 + 550;
	float x3 = x2 + 16;

	//
	// map list
	//

	new Text(this, 16, 16, "Выберите карту", alignTextLT);

	_maps = new MapList(this, x1, 32, x2 - x1, 192);
	GetManager()->SetFocusWnd(_maps);


	//
	// settings
	//

	{
		float y =  16;

		_nightMode = new CheckBox(this, x3, y, "Ночной режим");
		_nightMode->SetCheck( g_conf.cl_nightmode->Get() );


		new Text(this, x3, y+=30, "Скорость игры, %", alignTextLT);
		_gameSpeed = new Edit(this, x3+20, y+=15, 80);
		_gameSpeed->SetInt(g_conf.cl_speed->GetInt());

		new Text(this, x3, y+=30, "Лимит фрагов", alignTextLT);
		_fragLimit = new Edit(this, x3+20, y+=15, 80);
		_fragLimit->SetInt(g_conf.cl_fraglimit->GetInt());

		new Text(this, x3, y+=30, "Лимит времени", alignTextLT);
		_timeLimit = new Edit(this, x3+20, y+=15, 80);
		_timeLimit->SetInt(g_conf.cl_timelimit->GetInt());

		new Text(this, x3+30, y+=40, "(0 - нет лимита)", alignTextLT);
	}




}

CreateServerDlg::~CreateServerDlg()
{
}


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
