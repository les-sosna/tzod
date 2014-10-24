// Sound.cpp

#include "Sound.h"

#include "globals.h"
#include "World.h"
#include "Macros.h"
#include "SaveFile.h"

#include "config/Config.h"

#include <video/RenderBase.h>

#ifndef NOSOUND
#include <al.h>
#endif

/////////////////////////////////////////////////////////////

int GC_Sound::_countMax;
int GC_Sound::_countActive  = 0;
int GC_Sound::_countWaiting = 0;

IMPLEMENT_SELF_REGISTRATION(GC_Sound)
{
	return true;
}

IMPLEMENT_1LIST_MEMBER(GC_Sound, LIST_sounds);

GC_Sound::GC_Sound(enumSoundTemplate sound)
  : _soundTemplate(sound)
#ifndef NOSOUND
  , _source(0U)
#endif
  , _freezed(false)
  , _mode(SMODE_UNKNOWN)
  , _speed(1)
{
#ifndef NOSOUND
    alGenSources(1, &_source);
    if (AL_NO_ERROR == alGetError())
    {
        alSourcei(_source, AL_BUFFER, g_sounds[sound]);
        alSourcei(_source, AL_REFERENCE_DISTANCE, 70);
        _mode = SMODE_STOP;
    }

	SetVolume(1.0f);
	if( 100 != g_conf.sv_speed.GetInt() )
	{
		SetSpeed(1.0f);
	}

//	if( world.GetEditorMode() )
//		Freeze(true);
#endif
}

GC_Sound::GC_Sound(FromFile)
  : _mode(SMODE_UNKNOWN)
{
}

GC_Sound::~GC_Sound()
{
}

void GC_Sound::Kill(World &world)
{
#if !defined NOSOUND
	if( SMODE_UNKNOWN != _mode )
	{
		SetMode(world, SMODE_STOP);
        alDeleteSources(1, &_source);
        _mode = SMODE_UNKNOWN;
	}
#endif
    GC_Actor::Kill(world);
}

void GC_Sound::SetMode(World &world, enumSoundMode mode)
{
    assert(SMODE_UNKNOWN != mode);
#ifndef NOSOUND
	if( SMODE_UNKNOWN == _mode || mode == _mode ) return;

	switch (mode)
	{
	case SMODE_PLAY:
		assert(SMODE_STOP == _mode);
		if( _countActive == _countMax )
		{
            // FIXME: reverse
			FOREACH( world.GetList(LIST_sounds), GC_Sound, pSound )
			{
				if( SMODE_PLAY == pSound->_mode )
				{
					pSound->Kill(world);
					break;
				}
			}

			if( _countActive == _countMax )
			{
                // FIXME: reverse
				FOREACH( world.GetList(LIST_sounds), GC_Sound, pSound )
				{
					if( SMODE_LOOP == pSound->_mode )
					{
						pSound->SetMode(world, SMODE_WAIT);
						break;
					}
				}
			}
		}
		assert(_countActive < _countMax);
		++_countActive;
		_mode = SMODE_PLAY;
		if( !_freezed )
        {
            alSourcei(_source, AL_LOOPING, AL_FALSE);
			alSourcePlay(_source);
        }
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
            {
                alSourcei(_source, AL_LOOPING, AL_TRUE);
				alSourcePlay(_source);
            }
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
				FOREACH( world.GetList(LIST_sounds), GC_Sound, pSound )
				{
					if( SMODE_WAIT == pSound->_mode )
					{
						pSound->SetMode(world, SMODE_LOOP);
						break;
					}
				}
			}
            alSourcePause(_source);
		}
		_mode = SMODE_STOP;
		break;
	case SMODE_WAIT:
		assert(SMODE_LOOP == _mode);
		assert(_countActive > 0);
		--_countActive;
		++_countWaiting;
		_mode = SMODE_WAIT;
		alSourcePause(_source);
		break;
	default:
		assert(false);
	}
#endif
}

void GC_Sound::Pause(World &world, bool pause)
{
#if !defined NOSOUND
	assert(SMODE_PLAY != _mode);
	SetMode(world, pause ? SMODE_STOP : SMODE_LOOP);
#endif
}

void GC_Sound::UpdateVolume()
{
#if !defined NOSOUND
	if( SMODE_UNKNOWN != _mode )
	{
        alSourcef(_source, AL_GAIN, _volume);
//		_soundBuffer->SetVolume(DSBVOLUME_MIN
//			+ int((float) (g_conf.s_volume.GetInt() - DSBVOLUME_MIN) * _volume));
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
    _speed = speed;
#if !defined NOSOUND
	if( SMODE_UNKNOWN != _mode )
    {
        alSourcef(_source, AL_PITCH, speed * g_conf.sv_speed.GetFloat() * 0.01f);
    }
#endif
}

void GC_Sound::MoveTo(World &world, const vec2d &pos)
{
	GC_Actor::MoveTo(world, pos);
#if !defined NOSOUND
    alSource3f(_source, AL_POSITION, GetPos().x, GetPos().y, 0.0f);
#endif
}

void GC_Sound::Serialize(World &world, SaveFile &f)
{
	GC_Actor::Serialize(world, f);

#if !defined NOSOUND
	assert(f.loading() || _freezed);  // freeze it before saving!
    
    ALint offset = 0;
    if (SMODE_UNKNOWN != _mode)
    {
        alGetSourcei(_source, AL_SAMPLE_OFFSET, &offset);
    }

    f.Serialize(offset);
	f.Serialize(_freezed);
	f.Serialize(_speed);
	f.Serialize(_volume);
	f.Serialize(_mode);
	f.Serialize(_soundTemplate);

	if( f.loading() )
	{
        alGenSources(1, &_source);
        if (AL_NO_ERROR == alGetError())
        {
            alSourcei(_source, AL_BUFFER, g_sounds[_soundTemplate]);
            alSourcei(_source, AL_SAMPLE_OFFSET, offset);
        }
        else
        {
            _mode = SMODE_UNKNOWN;
        }


		MoveTo(world, GetPos()); // update pan
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
        default:
            break;
		}
	}
#endif
}

void GC_Sound::KillWhenFinished(World &world)
{
#if !defined NOSOUND
	if( SMODE_UNKNOWN == _mode || _freezed ) return;

	if( SMODE_PLAY == _mode )
	{
        ALint state = 0;
        alGetSourcei(_source, AL_SOURCE_STATE, &state);
        if( state != AL_PLAYING )
			Kill(world);
	}
#endif
}

void GC_Sound::Freeze(bool freeze)
{
	_freezed = freeze;
#if !defined NOSOUND
	if( freeze )
	{
        alSourcePause(_source);
	}
	else
	{
        alSourcePlay(_source);
	}
#endif
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Sound_link)
{
	return true;
}

IMPLEMENT_1LIST_MEMBER(GC_Sound_link, LIST_timestep);

GC_Sound_link::GC_Sound_link(enumSoundTemplate sound, GC_Actor *object)
   : GC_Sound(sound)
   , _object(object)
{
}

GC_Sound_link::GC_Sound_link(FromFile)
  : GC_Sound(FromFile())
{
}

void GC_Sound_link::Serialize(World &world, SaveFile &f)
{
	GC_Sound::Serialize(world, f);
	f.Serialize(_object);
}

void GC_Sound_link::TimeStep(World &world, float dt)
{
	if( !_object )
		Kill(world);
	else
		MoveTo(world, _object->GetPos());

	GC_Sound::TimeStep(world, dt);
}


// end of file
