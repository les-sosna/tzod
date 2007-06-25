// notify.h

#pragma once


enum NotyfyType 
{
	// GC_Object
	NOTIFY_OBJECT_KILL,
	NOTIFY_OBJECT_UPDATE_INDICATOR,

	// GC_Actor
	NOTIFY_ACTOR_MOVE,

	// Rigid body
	NOTIFY_RIGIDBODY_DESTROY,

	// GC_Player
	NOTIFY_PLAYER_SETCONTROLLER,

	// GC_Vehicle
	NOTIFY_DAMAGE_FILTER,
};

///////////////////////////////////////////////////////////////////////////////
// end of file
