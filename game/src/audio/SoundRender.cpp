#include "SoundRender.h"
#include <fs/FileSystem.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <iterator>
#include <utility>

namespace
{

class SoundDummy : public Sound
{
public:
	virtual void SetPos(vec2d) {}
	virtual void SetPlaying(bool) {}
	virtual void SetVolume(float) {}
	virtual void SetPitch(float) {}
};

class SoundImpl : public Sound
{
public:
	SoundImpl(ALuint source)
		: _source(source)
	{
	}
	
	virtual ~SoundImpl()
	{
		alDeleteSources(1, &_source);
	}

	virtual void SetPos(vec2d pos) override
	{
		alSource3f(_source, AL_POSITION, pos.x, pos.y, 0.0f);
	}
	
	virtual void SetPlaying(bool playing) override
	{
		if (playing)
		{
			ALint state = 0;
			alGetSourcei(_source, AL_SOURCE_STATE, &state);
			if (AL_PLAYING != state) // do not restart if already playing
				alSourcePlay(_source);
		}
		else
		{
			alSourcePause(_source);
		}
	}
	
	virtual void SetVolume(float volume) override
	{
		alSourcef(_source, AL_GAIN, volume);
	}
	
	virtual void SetPitch(float pitch) override
	{
		alSourcef(_source, AL_PITCH, pitch /* * g_conf.sv_speed.GetFloat() * 0.01f*/);
	}
	
private:
	ALuint _source;
};

struct FileState
{
    std::shared_ptr<FS::Stream> s;
    long position = 0;
};

struct FormatDesc
{
    ALenum format;
    ALsizei freq;
};

} // namespace


static size_t read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	auto *state = (FileState *) datasource;
	return state->s->Read(ptr, size, nmemb);
}

static int seek_func(void *datasource, ogg_int64_t offset, int whence)
{
	auto *state = (FileState *) datasource;
	state->s->Seek(offset, whence);
	return 0;
}

static long tell_func(void *datasource)
{
	auto *state = (FileState *) datasource;
	return static_cast<long>(state->s->Tell());
}

static void LoadOggVorbis(std::shared_ptr<FS::Stream> stream, FormatDesc &outFormatDesc, std::vector<char> &outData)
{
    FileState state;
    state.s = stream;

	ov_callbacks cb;
	cb.read_func  = read_func;
	cb.seek_func  = seek_func;
	cb.close_func = nullptr;
	cb.tell_func  = tell_func;

	OggVorbis_File vf;
	if( int result = ov_open_callbacks(&state, &vf, nullptr, 0, cb) )
	{
		switch( result )
		{
		case OV_EREAD: throw std::runtime_error("A read from media returned an error");
		case OV_ENOTVORBIS: throw std::runtime_error("Bitstream does not contain any Vorbis data");
		case OV_EVERSION: throw std::runtime_error("Vorbis version mismatch");
		case OV_EBADHEADER: throw std::runtime_error("Invalid Vorbis bitstream header");
		case OV_EFAULT: throw std::runtime_error("Internal logic fault; indicates a bug or heap/stack corruption");
		}
		throw std::runtime_error("unknown error opening ov stream");
	}

	try
	{
		vorbis_info *pinfo = ov_info(&vf, -1);
		if( nullptr == pinfo )
		{
			throw std::runtime_error("could not get info from ov stream");
		}


		outFormatDesc.format = pinfo->channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
		outFormatDesc.freq   = pinfo->rate;

        ogg_int64_t nSamples = ov_pcm_total(&vf, -1);
		size_t size = static_cast<size_t>(nSamples * pinfo->channels * 2);
		outData.resize(size);

		int bitstream = 0;
		size_t total = 0;

		while( total < size )
		{
			long ret = ov_read(&vf, &outData[total], size - total, 0, 2, 1, &bitstream);
			if( 0 == ret )
			{
				break; // eof
			}
			if( ret < 0 )
			{
				// error in stream
				switch( ret )
				{
				case OV_HOLE:
					throw std::runtime_error("garbage between pages, loss of sync followed by recapture, or a corrupt page");
				case OV_EBADLINK:
					throw std::runtime_error("invalid stream section or the requested link is corrupt");
				case OV_EINVAL:
					throw std::runtime_error("initial file headers couldn't be read or are corrupt");
				}
				throw std::runtime_error("unknown error in ov stream");
			}
			else
			{
				total += ret;
			}
		}
	}
	catch( const std::exception& )
	{
		ov_clear(&vf);
		throw;
	}
	ov_clear(&vf);
}

void SoundRender::LoadBuffer(FS::FileSystem &fs, SoundTemplate st, const char *fileName)
try
{
    FormatDesc fd;
    std::vector<char> data;
    LoadOggVorbis(fs.Open(fileName)->QueryStream(), fd, data);
    
    alBufferData(_buffers[st], fd.format, data.data(), data.size(), fd.freq);
    ALenum e = alGetError();
    if (AL_NO_ERROR != e)
    {
        const ALchar *msg = alGetString(e);
        throw std::runtime_error(std::string("failed to fill sound buffer with data: ") +
                                 (msg ? msg : "Unknown OpenAL error"));
    }
}
catch (const std::exception&)
{
    std::throw_with_nested(std::runtime_error(std::string("could not load '") + fileName + "'"));
}

SoundRender::SoundRender(FS::FileSystem &fs)
	: _buffers(SND_COUNT)
{
	alGenBuffers(SND_COUNT, &_buffers[0]);
	ALenum e = alGetError();
	if (AL_NO_ERROR != e)
	{
		const ALchar *msg = alGetString(e);
		throw std::runtime_error(std::string("failed to create sound buffers: ") +
								 (msg ? msg : "Unknown OpenAL error"));
	}
	
	LoadBuffer(fs, SND_BoomStandard,   "explosions/standard.ogg"    );
	LoadBuffer(fs, SND_BoomBig,        "explosions/big.ogg"         );
	LoadBuffer(fs, SND_WallDestroy,    "explosions/wall.ogg"        );
	
	LoadBuffer(fs, SND_Hit1,           "projectiles/hit1.ogg"       );
	LoadBuffer(fs, SND_Hit3,           "projectiles/hit2.ogg"       );
	LoadBuffer(fs, SND_Hit5,           "projectiles/hit3.ogg"       );
	LoadBuffer(fs, SND_AC_Hit1,        "projectiles/ac_hit_1.ogg"   );
	LoadBuffer(fs, SND_AC_Hit2,        "projectiles/ac_hit_2.ogg"   );
	LoadBuffer(fs, SND_AC_Hit3,        "projectiles/ac_hit_3.ogg"   );
	LoadBuffer(fs, SND_RocketFly,      "projectiles/rocketfly.ogg"  ); //
	LoadBuffer(fs, SND_DiskHit,        "projectiles/DiskHit.ogg"    ); //
	LoadBuffer(fs, SND_BfgFlash,       "projectiles/bfgflash.ogg"   ); //
	LoadBuffer(fs, SND_PlazmaHit,      "projectiles/plazmahit.ogg"  );
	LoadBuffer(fs, SND_BoomBullet,     "projectiles/bullet.ogg"     ); //
	
	LoadBuffer(fs, SND_TargetLock,     "turrets/activate.ogg"       );
	LoadBuffer(fs, SND_TuretRotate,    "turrets/rotate.ogg"         );
	LoadBuffer(fs, SND_TuretWakeUp,    "turrets/arming.ogg"         );
	LoadBuffer(fs, SND_TuretWakeDown,  "turrets/unarming.ogg"       );
	
	LoadBuffer(fs, SND_RocketShoot,    "pickup/rocketshoot.ogg"     ); //
	LoadBuffer(fs, SND_Shoot,          "pickup/shoot.ogg"           ); //
	LoadBuffer(fs, SND_MinigunFire,    "pickup/MinigunFire.ogg"     );
	LoadBuffer(fs, SND_WeapReload,     "pickup/reload.ogg"          );
	LoadBuffer(fs, SND_ACShoot,        "pickup/ac_shoot.ogg"        );
	LoadBuffer(fs, SND_AC_Reload,      "pickup/ac_reload.ogg"       );
	LoadBuffer(fs, SND_Pickup,         "pickup/pickup.ogg"          );
	LoadBuffer(fs, SND_B_Start,        "pickup/b_start.ogg"         );
	LoadBuffer(fs, SND_B_Loop,         "pickup/b_loop.ogg"          );
	LoadBuffer(fs, SND_B_End,          "pickup/b_end.ogg"           );
	LoadBuffer(fs, SND_w_Pickup,       "pickup/w_pickup.ogg"        ); //
	LoadBuffer(fs, SND_Bolt,           "pickup/boltshoot.ogg"       );
	LoadBuffer(fs, SND_DiskFire,       "pickup/ripper.ogg"          ); //
	LoadBuffer(fs, SND_puRespawn,      "pickup/puRespawn.ogg"       );
	LoadBuffer(fs, SND_TowerRotate,    "pickup/tower_rotate.ogg"    );
	LoadBuffer(fs, SND_ShockActivate,  "pickup/shockactivate.ogg"   ); //
	LoadBuffer(fs, SND_BfgInit,        "pickup/bfginit.ogg"         );
	LoadBuffer(fs, SND_BfgFire,        "pickup/bfgfire.ogg"         );
	LoadBuffer(fs, SND_PlazmaFire,     "pickup/plazma1.ogg"         );
	LoadBuffer(fs, SND_RamEngine,      "pickup/ram_engine.ogg"      ); //
	LoadBuffer(fs, SND_InvEnd,         "pickup/inv_end.ogg"         );
	LoadBuffer(fs, SND_Inv,            "pickup/inv.ogg"             );
	LoadBuffer(fs, SND_InvHit1,        "pickup/inv_hit1.ogg"        );
	LoadBuffer(fs, SND_InvHit2,        "pickup/inv_hit2.ogg"        );
	
	LoadBuffer(fs, SND_Impact1,        "vehicle/impact1.ogg"        );
	LoadBuffer(fs, SND_Impact2,        "vehicle/impact2.ogg"        );
	LoadBuffer(fs, SND_Slide1,         "vehicle/slide1.ogg"         );
	LoadBuffer(fs, SND_TankMove,       "vehicle/tank_move.ogg"      );
	
	LoadBuffer(fs, SND_Screenshot,     "misc/screenshot.ogg"        ); //
	LoadBuffer(fs, SND_Limit,          "misc/limit.ogg"             );
	LoadBuffer(fs, SND_LightSwitch,    "misc/light1.ogg"            ); //
}

SoundRender::~SoundRender()
{
	if (_sources.empty())
	{
		alDeleteSources((ALsizei) _sources.size(), &_sources[0]);
	}
}

void SoundRender::SetListenerPos(vec2d pos)
{
    alListener3f(AL_POSITION, pos.x, pos.y, 500.0f);
}

std::unique_ptr<Sound> SoundRender::CreateLopped(SoundTemplate sound)
{
	ALuint source = 0;
	alGenSources(1, &source);
	if (AL_NO_ERROR == alGetError())
	{
		alSourcei(source, AL_BUFFER, _buffers[sound]);
		alSourcei(source, AL_REFERENCE_DISTANCE, 70);
		alSourcei(source, AL_LOOPING, AL_TRUE);
		
		try {
			return std::unique_ptr<Sound>(new SoundImpl(source));
		}
		catch(...)
		{
			alDeleteSources(1, &source);
			throw;
		}
	}
	return std::unique_ptr<SoundDummy>(new SoundDummy());
}

void SoundRender::PlayOnce(SoundTemplate sound, vec2d pos)
{
	ALuint source = 0;
	alGenSources(1, &source);
	if (AL_NO_ERROR == alGetError())
	{
		_sources.push_back(source);
		alSourcei(source, AL_BUFFER, _buffers[sound]);
		alSourcei(source, AL_REFERENCE_DISTANCE, 70);
		alSourcei(source, AL_LOOPING, AL_FALSE);
		alSource3f(source, AL_POSITION, pos.x, pos.y, 0.0f);
		alSourcePlay(source);
	}
}

void SoundRender::Step()
{
	auto finished = _sources.end();
	for (auto it = _sources.begin(); it != finished; )
	{
		ALint state = 0;
		alGetSourcei(*it, AL_SOURCE_STATE, &state);
		if( state != AL_PLAYING )
			std::swap(*it, *--finished);
		else
			++it;
	}
	
	if (finished != _sources.end())
	{
		ALsizei count = (ALsizei) std::distance(finished, _sources.end());
		alDeleteSources(count, &*finished);
		_sources.erase(finished, _sources.end());
	}
}

