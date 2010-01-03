// Sound.cpp

#include "stdafx.h"
#include "Sound.h"

#include "level.h"
#include "macros.h"

#include "video/RenderBase.h"

#include "config/Config.h"

#include "fs/SaveFile.h"

/////////////////////////////////////////////////////////////

int GC_Sound::_countMax;
int GC_Sound::_countActive  = 0;
int GC_Sound::_countWaiting = 0;

IMPLEMENT_SELF_REGISTRATION(GC_Sound)
{
	return true;
}

GC_Sound::GC_Sound(enumSoundTemplate sound, enumSoundMode mode, const vec2d &pos)
  : GC_Actor()
  , _memberOf(this)
  , _soundTemplate(sound)
  , _freezed(false)
{
#ifndef NOSOUND
	if( !g_soundManager )
	{
		_mode = SMODE_STOP;
		return;
	}

	g_soundManager->GetDirectSound()->DuplicateSoundBuffer(
		g_pSounds[sound]->GetBuffer(0), &_soundBuffer );
	///////////////////////
	_soundBuffer->GetFrequency(&_dwNormalFrequency);
	_dwCurrentFrequency = _dwNormalFrequency;
	_soundBuffer->SetCurrentPosition(_dwPosition = 0);
	///////////////////////
	MoveTo(pos);
	///////////////////////
	SetVolume(1.0f);
	if( 100 != g_conf.sv_speed.GetInt() )
	{
		SetSpeed(1.0f);
	}

	_mode = SMODE_UNKNOWN;
	SetMode(mode);

	if( g_level->GetEditorMode() )
		Freeze(true);
#endif
}

GC_Sound::GC_Sound(FromFile)
  : GC_Actor(FromFile())
  , _memberOf(this)
  , _mode(SMODE_STOP)
{
}

GC_Sound::~GC_Sound()
{
#if !defined NOSOUND
	assert(!_soundBuffer);
	assert(SMODE_STOP == _mode);
#endif
}

void GC_Sound::Kill()
{
#if !defined NOSOUND
	if( _soundBuffer )
	{
		SetMode(SMODE_STOP);
		_soundBuffer.Release();
	}
#endif

	GC_Actor::Kill();
}

void GC_Sound::SetMode(enumSoundMode mode)
{
	assert(!IsKilled());

#ifndef NOSOUND

	if( !g_soundManager ) return;
	if( mode == _mode ) return;

	switch (mode)
	{
	case SMODE_PLAY:
		assert(SMODE_UNKNOWN == _mode);
		if( _countActive == _countMax )
		{
			FOREACH_R( g_level->GetList(LIST_sounds), GC_Sound, pSound )
			{
				if( SMODE_PLAY == pSound->_mode )
				{
					pSound->Kill();
					break;
				}
			}

			if( _countActive == _countMax )
			{
				FOREACH_R( g_level->GetList(LIST_sounds), GC_Sound, pSound )
				{
					if( SMODE_LOOP == pSound->_mode )
					{
						pSound->SetMode(SMODE_WAIT);
						break;
					}
				}
			}
		}
		////////////////////////////
		assert(_countActive < _countMax);
		/////////////////////////////
		++_countActive;
		_mode = SMODE_PLAY;
		if( !_freezed )
			_soundBuffer->Play(0, 0, 0);
		break;
	case SMODE_LOOP:
		assert(SMODE_PLAY != _mode);
		if( _countActive == _countMax )
		{
			if( SMODE_WAIT != _mode )
			{
				_mode = SMODE_WAIT;
				++_countWaiting;
			}
		}
		else
		{
			if( SMODE_WAIT == _mode )
				--_countWaiting;

			++_countActive;
			_mode = SMODE_LOOP;
			if( !_freezed )
				_soundBuffer->Play(0, 0, DSBPLAY_LOOPING);
		}
		break;
	case SMODE_STOP:
		if( SMODE_UNKNOWN != _mode )
		{
			if( SMODE_WAIT == _mode )
				--_countWaiting;
			else
				--_countActive;

			if( _countActive < _countMax && 0 < _countWaiting )
			{
				FOREACH( g_level->GetList(LIST_sounds), GC_Sound, pSound )
				{
					if( SMODE_WAIT == pSound->_mode )
					{
						pSound->SetMode(SMODE_LOOP);
						break;
					}
				}
			}
			_soundBuffer->Stop();
		}
		_mode = SMODE_STOP;
		break;
	case SMODE_WAIT:
		assert(SMODE_LOOP == _mode);
		assert(_countActive > 0);
		--_countActive;
		++_countWaiting;
		_mode = SMODE_WAIT;
		_soundBuffer->Stop();
		break;
	default:
		assert(false);
	}
#endif
}

void GC_Sound::Pause(bool pause)
{
#if !defined NOSOUND
	assert(SMODE_PLAY != _mode);
	if( !IsKilled() )
		SetMode(pause ? SMODE_STOP : SMODE_LOOP);
#endif
}

void GC_Sound::UpdateVolume()
{
#if !defined NOSOUND
	if( _soundBuffer )
	{
		_soundBuffer->SetVolume(DSBVOLUME_MIN
			+ int((float) (g_conf.s_volume.GetInt() - DSBVOLUME_MIN) * _volume));
	}
#endif
}

void GC_Sound::SetVolume(float vol)
{
#if !defined NOSOUND
	assert(0 <= vol);
	assert(1 >= vol);
	_volume = vol;
	UpdateVolume();
#endif
}

void GC_Sound::SetSpeed(float speed)
{
#if !defined NOSOUND
	if( !g_soundManager ) return;
	_dwCurrentFrequency = int((float)_dwNormalFrequency * speed
		* g_conf.sv_speed.GetFloat() * 0.01f);
	_soundBuffer->SetFrequency(_dwCurrentFrequency);
#endif
}

void GC_Sound::MoveTo(const vec2d &pos)
{
	GC_Actor::MoveTo(pos);
#if !defined NOSOUND
//	if( g_soundManager )
//		_soundBuffer->SetPan(int(pos.x - g_env.camera_x - g_render->GetWidth() / 2));
#endif
}

void GC_Sound::Serialize(SaveFile &f)
{
	GC_Actor::Serialize(f);

#if !defined NOSOUND
	assert(f.loading() || _freezed);  // freeze it before saving!
	/////////////////////////////////////
	f.Serialize(_freezed);
	f.Serialize(_dwNormalFrequency);
	f.Serialize(_dwCurrentFrequency);
	f.Serialize(_dwPosition);
	f.Serialize(_volume);
	f.Serialize(_mode);
	f.Serialize(_soundTemplate);
	/////////////////////////////////////
	if( f.loading() && g_soundManager )
	{
		g_soundManager->GetDirectSound()->DuplicateSoundBuffer(
			g_pSounds[_soundTemplate]->GetBuffer(0), &_soundBuffer);

		_soundBuffer->SetCurrentPosition( _dwPosition );
		_soundBuffer->SetFrequency(_dwCurrentFrequency);

		MoveTo(GetPos()); // update pan
		UpdateVolume();

		switch (_mode)
		{
		case SMODE_PLAY:
		case SMODE_LOOP:
			++_countActive;
			break;
		case SMODE_WAIT:
			++_countWaiting;
			break;
		}
	}
#endif
}

void GC_Sound::KillWhenFinished()
{
#if !defined NOSOUND
	if( !g_soundManager || _freezed ) return;

	if( SMODE_PLAY == _mode )
	{
		DWORD dwStatus;
		_soundBuffer->GetStatus( &dwStatus );
		if( !(dwStatus & DSBSTATUS_PLAYING) )
			Kill();
	}
#endif
}

void GC_Sound::Freeze(bool freeze)
{
#if !defined NOSOUND
	if( !g_soundManager ) return;

	_freezed = freeze;
	if( IsKilled() )
		return;

	if( freeze )
	{
		_soundBuffer->GetCurrentPosition(&_dwPosition, NULL);
		if( SMODE_STOP != _mode ) _soundBuffer->Stop();
	}
	else
	{
		_soundBuffer->SetCurrentPosition(_dwPosition);
		switch (_mode)
		{
			case SMODE_PLAY:
				_soundBuffer->Play(0, 0, 0);
				break;
			case SMODE_LOOP:
				_soundBuffer->Play(0, 0, DSBPLAY_LOOPING);
				break;
		}
	}
#endif
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Sound_link)
{
	return true;
}

GC_Sound_link::GC_Sound_link(enumSoundTemplate sound, enumSoundMode mode, GC_Actor *object)
   : GC_Sound(sound, mode, object->GetPos())
   , _object(object)
{
	///////////////////////
#if !defined NOSOUND
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);
#endif
}

GC_Sound_link::GC_Sound_link(FromFile)
  : GC_Sound(FromFile())
{
}

void GC_Sound_link::Kill()
{
	_object = NULL;
	GC_Sound::Kill();
}

void GC_Sound_link::Serialize(SaveFile &f)
{
	GC_Sound::Serialize(f);
	f.Serialize(_object);
}

void GC_Sound_link::TimeStepFixed(float dt)
{
	assert(_object);

	if( _object->IsKilled() )
		Kill();
	else
		MoveTo(_object->GetPos());

	GC_Sound::TimeStepFixed(dt);
}


// end of file
