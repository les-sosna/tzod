// sfx.cpp

#include "sfx.h"

#include "constants.h"
#include "globals.h"
#include "core/Debug.h"

#include <fs/FileSystem.h>

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include <al.h>
#include <alc.h>
#include <cassert>


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

static void ogg_load_vorbis(std::shared_ptr<FS::Stream> stream, FormatDesc *fd, std::vector<char> &data)
{
    FileState state;
    state.s = stream;

	ov_callbacks cb;
	cb.read_func  = read_func;
	cb.seek_func  = seek_func;
	cb.close_func = NULL;
	cb.tell_func  = tell_func;

	OggVorbis_File vf;
	if( int result = ov_open_callbacks(&state, &vf, NULL, 0, cb) )
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
		if( NULL == pinfo )
		{
			throw std::runtime_error("could not get info from ov stream");
		}


		fd->format = pinfo->channels == 2 ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
		fd->freq   = pinfo->rate;

        ogg_int64_t nSamples = ov_pcm_total(&vf, -1);
		size_t size = static_cast<size_t>(nSamples * pinfo->channels * 2);
		data.resize(size);

		int bitstream = 0;
		size_t total = 0;

		while( total < size )
		{
			long ret = ov_read(&vf, &data[total], size - total, 0, 2, 1, &bitstream);
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

static void LoadOggVorbis(FS::FileSystem *fs, bool init, enumSoundTemplate sound, const char *filename)
{
	if( init )
	{
		try
		{
			FormatDesc fd;
			std::vector<char> data;
			ogg_load_vorbis(fs->Open(filename)->QueryStream(), &fd, data);
            
            alGenBuffers(1, &g_sounds[sound]);
            if (AL_NO_ERROR != alGetError())
            {
                throw std::runtime_error("failed to create sound buffer");
            }
            
            alBufferData(g_sounds[sound], fd.format, data.data(), data.size(), fd.freq);
            ALenum err = alGetError();
            if (AL_NO_ERROR != err)
            {
                throw std::runtime_error(alGetString(err));
            }
        }
		catch( std::exception &e )
		{
			throw std::runtime_error(std::string("failed to load '") + filename + "' - " + e.what());
		}
	}
	else
	{
		alDeleteBuffers(1, &g_sounds[sound]);
	}
}


bool InitSound(FS::FileSystem *fs, bool init)
{
    static ALCdevice *device = nullptr;
    static ALCcontext *context = nullptr;
    
	if( init )
	{
        assert(!device && !context);
		TRACE("Init sound - OpenAL");
        
        device = alcOpenDevice(nullptr);
        if (!device)
        {
            TRACE("ERROR: failed to open audio device");
            return false;
        }
        
        context = alcCreateContext(device, nullptr);
        if (!context)
        {
            TRACE("ERROR: failed to create audio context");
            alcCloseDevice(device);
            return false;
        }
        
        alcMakeContextCurrent(context);
	}
	else
	{
		TRACE("Release sound");
	}
    
    
	try
	{
        LoadOggVorbis(fs, init, SND_BoomStandard,   "explosions/standard.ogg"    );
        LoadOggVorbis(fs, init, SND_BoomBig,        "explosions/big.ogg"         );
        LoadOggVorbis(fs, init, SND_WallDestroy,    "explosions/wall.ogg"        );
        
        LoadOggVorbis(fs, init, SND_Hit1,           "projectiles/hit1.ogg"       );
        LoadOggVorbis(fs, init, SND_Hit3,           "projectiles/hit2.ogg"       );
        LoadOggVorbis(fs, init, SND_Hit5,           "projectiles/hit3.ogg"       );
        LoadOggVorbis(fs, init, SND_AC_Hit1,        "projectiles/ac_hit_1.ogg"   );
        LoadOggVorbis(fs, init, SND_AC_Hit2,        "projectiles/ac_hit_2.ogg"   );
        LoadOggVorbis(fs, init, SND_AC_Hit3,        "projectiles/ac_hit_3.ogg"   );
        LoadOggVorbis(fs, init, SND_RocketFly,      "projectiles/rocketfly.ogg"  ); //
        LoadOggVorbis(fs, init, SND_DiskHit,        "projectiles/DiskHit.ogg"    ); //
        LoadOggVorbis(fs, init, SND_BfgFlash,       "projectiles/bfgflash.ogg"   ); //
        LoadOggVorbis(fs, init, SND_PlazmaHit,      "projectiles/plazmahit.ogg"  );
        LoadOggVorbis(fs, init, SND_BoomBullet,     "projectiles/bullet.ogg"     ); //
        
        LoadOggVorbis(fs, init, SND_TargetLock,     "turrets/activate.ogg"       );
        LoadOggVorbis(fs, init, SND_TuretRotate,    "turrets/rotate.ogg"         );
        LoadOggVorbis(fs, init, SND_TuretWakeUp,    "turrets/arming.ogg"         );
        LoadOggVorbis(fs, init, SND_TuretWakeDown,  "turrets/unarming.ogg"       );
        
        LoadOggVorbis(fs, init, SND_RocketShoot,    "pickup/rocketshoot.ogg"     ); //
        LoadOggVorbis(fs, init, SND_Shoot,          "pickup/shoot.ogg"           ); //
        LoadOggVorbis(fs, init, SND_MinigunFire,    "pickup/MinigunFire.ogg"     );
        LoadOggVorbis(fs, init, SND_WeapReload,     "pickup/reload.ogg"          );
        LoadOggVorbis(fs, init, SND_ACShoot,        "pickup/ac_shoot.ogg"        );
        LoadOggVorbis(fs, init, SND_AC_Reload,      "pickup/ac_reload.ogg"       );
        LoadOggVorbis(fs, init, SND_Pickup,         "pickup/pickup.ogg"          );
        LoadOggVorbis(fs, init, SND_B_Start,        "pickup/b_start.ogg"         );
        LoadOggVorbis(fs, init, SND_B_Loop,         "pickup/b_loop.ogg"          );
        LoadOggVorbis(fs, init, SND_B_End,          "pickup/b_end.ogg"           );
        LoadOggVorbis(fs, init, SND_w_Pickup,       "pickup/w_pickup.ogg"        ); //
        LoadOggVorbis(fs, init, SND_Bolt,           "pickup/boltshoot.ogg"       );
        LoadOggVorbis(fs, init, SND_DiskFire,       "pickup/ripper.ogg"          ); //
        LoadOggVorbis(fs, init, SND_puRespawn,      "pickup/puRespawn.ogg"       );
        LoadOggVorbis(fs, init, SND_TowerRotate,    "pickup/tower_rotate.ogg"    );
        LoadOggVorbis(fs, init, SND_ShockActivate,  "pickup/shockactivate.ogg"   ); //
        LoadOggVorbis(fs, init, SND_BfgInit,        "pickup/bfginit.ogg"         );
        LoadOggVorbis(fs, init, SND_BfgFire,        "pickup/bfgfire.ogg"         );
        LoadOggVorbis(fs, init, SND_PlazmaFire,     "pickup/plazma1.ogg"         );
        LoadOggVorbis(fs, init, SND_RamEngine,      "pickup/ram_engine.ogg"      ); //
        LoadOggVorbis(fs, init, SND_InvEnd,         "pickup/inv_end.ogg"         );
        LoadOggVorbis(fs, init, SND_Inv,            "pickup/inv.ogg"             );
        LoadOggVorbis(fs, init, SND_InvHit1,        "pickup/inv_hit1.ogg"        );
        LoadOggVorbis(fs, init, SND_InvHit2,        "pickup/inv_hit2.ogg"        );
        
        LoadOggVorbis(fs, init, SND_Impact1,        "vehicle/impact1.ogg"        );
        LoadOggVorbis(fs, init, SND_Impact2,        "vehicle/impact2.ogg"        );
        LoadOggVorbis(fs, init, SND_Slide1,         "vehicle/slide1.ogg"         );
        LoadOggVorbis(fs, init, SND_TankMove,       "vehicle/tank_move.ogg"      );
        
        LoadOggVorbis(fs, init, SND_Screenshot,     "misc/screenshot.ogg"        ); //
        LoadOggVorbis(fs, init, SND_Limit,          "misc/limit.ogg"             );
        LoadOggVorbis(fs, init, SND_LightSwitch,    "misc/light1.ogg"            ); //
	}
	catch( const std::exception & )
	{
		if( init ) FreeSound();
		throw;
	}
    
	return true;
}

void FreeSound()
{
	InitSound(nullptr, false);
}

// end of file
