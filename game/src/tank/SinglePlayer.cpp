// SinglePlayer.cpp

#include "stdafx.h"
#include "SinglePlayer.h"
#include "config/Config.h"

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
