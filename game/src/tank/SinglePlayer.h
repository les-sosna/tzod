// SinglePlayer.h

#pragma once

#include "network/ControlPacket.h"
#include "ClientBase.h"
#include <vector>

namespace FS
{
	class Stream;
};

class SinglePlayerClient : public ClientBase
{
public:
	SinglePlayerClient(Level *level, std::shared_ptr<FS::Stream> stream, unsigned long seed);

	virtual bool SupportPause() const;
	virtual bool SupportEditor() const;
	virtual bool SupportSave() const;
	virtual bool IsLocal() const;
	virtual void SendControl(const ControlPacket &cp);
	virtual bool RecvControl(std::vector<ControlPacket> &result);
    const char* GetActiveProfile() const;

private:
	ControlPacket _cp;
};


// end of file
