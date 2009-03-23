// CommonTypes.cpp

#include "stdafx.h"
#include "CommonTypes.h"

VARIANT_IMPLEMENT_TYPE(PlayerReady) RAW;

VARIANT_IMPLEMENT_TYPE(GameInfo) RAW;

VARIANT_IMPLEMENT_TYPE(PlayerDesc)
{
	return s
		& value.nick
		& value.skin
		& value.cls
		& value.team
		& value.score;
}

VARIANT_IMPLEMENT_TYPE(PlayerDescEx)
{
	return s & (PlayerDesc&) value & value.id;
}

VARIANT_IMPLEMENT_TYPE(BotDesc)
{
	return s & (PlayerDesc&) value & value.level;
}

// end of file
