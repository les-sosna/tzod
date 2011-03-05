// BackgroundIntro.cpp

#include "stdafx.h"
#include "BackgroundIntro.h"


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


// end of file
