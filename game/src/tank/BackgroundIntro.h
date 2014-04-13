// BackgroundIntro.h

#pragma once

#include "network/ControlPacket.h"
#include "ClientBase.h"

class IntroClient : public ClientBase
{
public:
	explicit IntroClient(Level *levelController);

	virtual bool SupportPause() const;
	virtual bool SupportEditor() const;
	virtual bool SupportSave() const;
	virtual bool IsLocal() const;
	virtual void SendControl(const ControlPacket &cp);
	virtual bool RecvControl(std::vector<ControlPacket> &result);
    virtual const char* GetActiveProfile() const;
};


// end of file
