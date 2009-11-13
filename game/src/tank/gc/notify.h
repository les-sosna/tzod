// notify.h

#pragma once


enum NotifyType
{
	// GC_Object
	NOTIFY_OBJECT_KILL,

	// GC_Actor
	NOTIFY_ACTOR_MOVE,

	// GC_Pickup
	NOTIFY_PICKUP_DISAPPEAR,

	// Rigid body
	NOTIFY_RIGIDBODY_DESTROY,

	// GC_Player
	NOTIFY_PLAYER_SETCONTROLLER,

	// GC_Vehicle
	NOTIFY_DAMAGE_FILTER,
};

///////////////////////////////////////////////////////////////////////////////
// end of file
