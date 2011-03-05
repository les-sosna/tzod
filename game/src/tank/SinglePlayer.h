// SinglePlayer.h

#pragma once

#include "network/ControlPacket.h"

class SinglePlayerClient
{
public:
	virtual bool IsLocal() const;
	virtual void SendControl(const ControlPacket &cp);
	virtual bool RecvControl(ControlPacketVector &result);

private:
	ControlPacket _cp;
};


// end of file
