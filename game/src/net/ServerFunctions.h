// ServerFunctions.h

#pragma once

enum ServerFunction
{
	SV_POST_TEXTMESSAGE,  // std::string
	SV_POST_CONTROL,      // ControlPacket
	SV_POST_PLAYERREADY,  // bool
	SV_POST_ADDBOT,       // BotDesc
	SV_POST_PLAYERINFO,   // PlayerDesc
};



// end of file
