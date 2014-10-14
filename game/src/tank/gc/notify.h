// notify.h

#pragma once


enum NotifyType
{
	// GC_Object
	NOTIFY_OBJECT_KILL,

	// GC_Actor
	NOTIFY_ACTOR_MOVE,

	// Rigid body
	NOTIFY_RIGIDBODY_DESTROY,

	// GC_Vehicle
	NOTIFY_DAMAGE_FILTER,
};

///////////////////////////////////////////////////////////////////////////////
// end of file
