// Sound.h

#pragma once

#include "Actor.h"
#include "core/ComPtr.h"

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

private:
	ComPtr<IDirectSoundBuffer> _soundBuffer;
	enumSoundTemplate   _soundTemplate;
	DWORD _dwNormalFrequency;
	DWORD _dwCurrentFrequency;
	DWORD _dwPosition;

protected:
	bool          _freezed;
	enumSoundMode _mode;
	void SetMode(enumSoundMode mode);

public:
	float _volume;  // 0 - min;  1 - max

public:
	GC_Sound(enumSoundTemplate sound, enumSoundMode mode, const vec2d &pos);
	GC_Sound(FromFile);
	virtual ~GC_Sound();
	virtual void Kill();
	virtual void Serialize(SaveFile &f);

	void KillWhenFinished();
	virtual void MoveTo(const vec2d &pos);

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
	SafePtr<GC_Actor> _object;

public:
	GC_Sound_link(enumSoundTemplate sound, enumSoundMode mode, GC_Actor *object);
	GC_Sound_link(FromFile);
	virtual void Kill();
	virtual void Serialize(SaveFile &f);
	virtual void TimeStepFixed(float dt);

public:
	bool CheckObject(const GC_Object *object) const
	{
		return _object == object;
	}
};

/////////////////////////////////////////////////////////////

#if !defined NOSOUND
#define PLAY(s, pos)  (new GC_Sound((s), SMODE_PLAY, (pos)))
#else
#define PLAY // no sound
#endif

// end of file
