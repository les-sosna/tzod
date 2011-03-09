// SinglePlayer.cpp

#include "stdafx.h"
#include "SinglePlayer.h"
#include "config/Config.h"
#include "globals.h"
#include "Level.h"

SinglePlayerClient::SinglePlayerClient(ILevelController *levelController, FS::Stream *stream, unsigned long seed)
	: ClientBase(levelController)
{
	g_level->init_newdm(stream, seed);
}

bool SinglePlayerClient::SupportEditor() const
{
	return true;
}

bool SinglePlayerClient::SupportSave() const
{
	return true;
}

bool SinglePlayerClient::IsLocal() const
{
	return true;
}

void SinglePlayerClient::SendControl(const ControlPacket &cp)
{
	_cp = cp;
}

bool SinglePlayerClient::RecvControl(ControlPacketVector &result)
{
	result.assign(1, _cp);
	return true;
}



// end of file
