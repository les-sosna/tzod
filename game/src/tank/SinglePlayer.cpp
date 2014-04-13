// SinglePlayer.cpp

#include "SinglePlayer.h"
#include "script.h"
#include "config/Config.h"

#include <vector>

SinglePlayerClient::SinglePlayerClient(Level *level, std::shared_ptr<FS::Stream> stream, unsigned long seed)
	: ClientBase(level)
{
	level->Clear();
	level->Seed(seed);
	level->SetEditorMode(false);
	level->Import(stream);
	if( !script_exec(g_env.L, level->_infoOnInit.c_str()) )
	{
		level->Clear();
		throw std::runtime_error("init script error");
	}
    
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

bool SinglePlayerClient::RecvControl(std::vector<ControlPacket> &result)
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
