// BackgroundIntro.cpp

#include "BackgroundIntro.h"

IntroClient::IntroClient(World *levelController)
	: ClientBase(levelController)
{
}

bool IntroClient::SupportPause() const
{
	return false;
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

bool IntroClient::RecvControl(std::vector<ControlPacket> &result)
{
	result.clear();
	return true;
}

const char* IntroClient::GetActiveProfile() const
{
    return NULL;
}


// end of file
