// ControlPacket.h

#pragma once
#include <cstdint>

struct VehicleState;

const unsigned int STATE_MOVEFORWARD   = 0x0001;
const unsigned int STATE_MOVEBACK      = 0x0002;
const unsigned int STATE_ROTATELEFT    = 0x0004;
const unsigned int STATE_ROTATERIGHT   = 0x0008;
const unsigned int STATE_FIRE          = 0x0010;
const unsigned int STATE_ALLOWDROP     = 0x0020;
const unsigned int STATE_TOWERLEFT     = 0x0040;
const unsigned int STATE_TOWERRIGHT    = 0x0080;
const unsigned int STATE_TOWERCENTER   = 0x0100;
const unsigned int STATE_ENABLELIGHT   = 0x0200;

const unsigned int MODE_EXPLICITTOWER  = 0x1000;
const unsigned int MODE_EXPLICITBODY   = 0x2000;
const unsigned int MODE_BODY_HINT_CW   = 0x4000;
const unsigned int MODE_BODY_HINT_CCW  = 0x8000;


#pragma pack(push)
#pragma pack(2)
struct ControlPacket
{
	uint16_t wControlState;
	uint16_t weap;  // angle, if explicit
	uint16_t body;  // angle, if explicit
#ifdef NETWORK_DEBUG
	uint32_t checksum;
	uint32_t frame;
#endif
	//--------------------------
	ControlPacket();
	//--------------------------
	void fromvs(const VehicleState &vs);
	void tovs(VehicleState &vs) const;
};
#pragma pack(pop)

//typedef std::vector<ControlPacket> ControlPacketVector;

//#include "Variant.h"
//VARIANT_DECLARE_TYPE(ControlPacket);
//VARIANT_DECLARE_TYPE(ControlPacketVector);

// end of file
