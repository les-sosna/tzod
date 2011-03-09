// SinglePlayer.h

#pragma once

#include "network/ControlPacket.h"
#include "ClientBase.h"

namespace FS
{
	class Stream;
};

class SinglePlayerClient : public ClientBase
{
public:
	SinglePlayerClient(ILevelController *levelController, FS::Stream *stream, unsigned long seed);

	virtual bool SupportEditor() const;
	virtual bool SupportSave() const;
	virtual bool IsLocal() const;
	virtual void SendControl(const ControlPacket &cp);
	virtual bool RecvControl(ControlPacketVector &result);

private:
	ControlPacket _cp;
};


// end of file
