// SinglePlayer.cpp

#include "stdafx.h"
#include "SinglePlayer.h"
#include "config/Config.h"
#include "LevelInterfaces.h"

SinglePlayerClient::SinglePlayerClient(ILevelController *levelController, FS::Stream *stream, unsigned long seed)
	: ClientBase(levelController)
{
	levelController->Clear();
	levelController->init_newdm(stream, seed);
}

bool SinglePlayerClient::SupportPause() const
{
	return true;
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

const char* SinglePlayerClient::GetActiveProfile() const
{
    if( g_conf.dm_players.GetSize() )
        return ConfPlayerLocal(g_conf.dm_players.GetTable(0)).profile.Get().c_str();
    return NULL;
}



// end of file
