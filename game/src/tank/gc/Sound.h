// Sound.h

#pragma once

#include "Actor.h"

#include <SoundTemplates.h>
#ifndef NOSOUND
#include <al.h>
#endif

/////////////////////////////////////////////////////////////

enum enumSoundMode
{
    SMODE_UNKNOWN = 0,
    SMODE_PLAY,  // the GC_Soung object will be destroyed at the end
    SMODE_LOOP,
    SMODE_STOP,  // pause with the resource releasing
    SMODE_WAIT,  // forced pause
};

class GC_Sound : public GC_Actor
{
	DECLARE_SELF_REGISTRATION(GC_Sound);
	MemberOfGlobalList<LIST_sounds> _memberOf;

	enumSoundTemplate   _soundTemplate;
#ifndef NOSOUND
    ALuint _source;
#endif
    
protected:
	bool          _freezed;
	enumSoundMode _mode;
    float _speed;
	void SetMode(enumSoundMode mode);

public:
	float _volume;  // 0 - min;  1 - max

public:
	GC_Sound(Level &world, enumSoundTemplate sound, enumSoundMode mode, const vec2d &pos);
	GC_Sound(FromFile);
	virtual ~GC_Sound();
	virtual void Serialize(Level &world, SaveFile &f);

	void KillWhenFinished(Level &world);
	virtual void MoveTo(Level &world, const vec2d &pos) override;

	void Pause(bool pause);
	void Freeze(bool freeze);

	void SetSpeed(float speed);
	void SetVolume(float vol);
	void UpdateVolume();  // should be called each time the conf.s_volume changes

public:
	static int _countMax;
	static int _countActive;
	static int _countWaiting;
};

/////////////////////////////////////////////////////////////
// got destroyed together with the target object
class GC_Sound_link : public GC_Sound
{
	DECLARE_SELF_REGISTRATION(GC_Sound_link);

protected:
	ObjPtr<GC_Actor> _object;

public:
	GC_Sound_link(Level &world, enumSoundTemplate sound, enumSoundMode mode, GC_Actor *object);
	GC_Sound_link(FromFile);
	virtual void Serialize(Level &world, SaveFile &f);
	virtual void TimeStepFixed(Level &world, float dt);

public:
	bool CheckObject(const GC_Object *object) const
	{
		return _object == object;
	}
};

/////////////////////////////////////////////////////////////

#if !defined NOSOUND
#define PLAY(s, pos)  (new GC_Sound(world, (s), SMODE_PLAY, (pos)))
#else
#define PLAY(s, pos) ((void) 0) // no sound
#endif

// end of file
