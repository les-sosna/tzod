// BackgroundIntro.h

#pragma once

#include "network/ControlPacket.h"
#include "ClientBase.h"

class IntroClient : public ClientBase
{
public:
	virtual bool IsLocal() const;
	virtual void SendControl(const ControlPacket &cp);
	virtual bool RecvControl(ControlPacketVector &result);
};


// end of file
