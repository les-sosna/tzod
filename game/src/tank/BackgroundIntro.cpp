// BackgroundIntro.cpp

#include "stdafx.h"
#include "BackgroundIntro.h"

IntroClient::IntroClient(ILevelController *levelController)
	: ClientBase(levelController)
{
}

bool IntroClient::SupportEditor() const
{
	return false;
}

bool IntroClient::SupportSave() const
{
	return false;
}

bool IntroClient::IsLocal() const
{
	return true;
}

void IntroClient::SendControl(const ControlPacket &cp)
{
}

bool IntroClient::RecvControl(ControlPacketVector &result)
{
	result.clear();
	return true;
}

const char* IntroClient::GetActiveProfile() const
{
    return NULL;
}


// end of file
