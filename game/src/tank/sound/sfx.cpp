// sfx.cpp

#include "sfx.h"

#include "constants.h"
#include "globals.h"
#include "core/debug.h"

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

static void ogg_load_vorbis(const char *filename, FormatDesc *fd, std::vector<char> &data)
{
    FileState state;
    state.s = g_fs->Open(filename)->QueryStream();

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

static void LoadOggVorbis(bool init, enumSoundTemplate sound, const char *filename)
{
	if( init )
	{
		try
		{
			FormatDesc fd;
			std::vector<char> data;
			ogg_load_vorbis(filename, &fd, data);
            
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


bool InitSound(bool init)
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
        LoadOggVorbis(init, SND_BoomStandard,   DIR_SOUND"/explosions/standard.ogg"    );
        LoadOggVorbis(init, SND_BoomBig,        DIR_SOUND"/explosions/big.ogg"         );
        LoadOggVorbis(init, SND_WallDestroy,    DIR_SOUND"/explosions/wall.ogg"        );
        
        LoadOggVorbis(init, SND_Hit1,           DIR_SOUND"/projectiles/hit1.ogg"       );
        LoadOggVorbis(init, SND_Hit3,           DIR_SOUND"/projectiles/hit2.ogg"       );
        LoadOggVorbis(init, SND_Hit5,           DIR_SOUND"/projectiles/hit3.ogg"       );
        LoadOggVorbis(init, SND_AC_Hit1,        DIR_SOUND"/projectiles/ac_hit_1.ogg"   );
        LoadOggVorbis(init, SND_AC_Hit2,        DIR_SOUND"/projectiles/ac_hit_2.ogg"   );
        LoadOggVorbis(init, SND_AC_Hit3,        DIR_SOUND"/projectiles/ac_hit_3.ogg"   );
        LoadOggVorbis(init, SND_RocketFly,      DIR_SOUND"/projectiles/rocketfly.ogg"  ); //
        LoadOggVorbis(init, SND_DiskHit,        DIR_SOUND"/projectiles/DiskHit.ogg"    ); //
        LoadOggVorbis(init, SND_BfgFlash,       DIR_SOUND"/projectiles/bfgflash.ogg"   ); //
        LoadOggVorbis(init, SND_PlazmaHit,      DIR_SOUND"/projectiles/plazmahit.ogg"  );
        LoadOggVorbis(init, SND_BoomBullet,     DIR_SOUND"/projectiles/bullet.ogg"     ); //
        
        LoadOggVorbis(init, SND_TargetLock,     DIR_SOUND"/turrets/activate.ogg"       );
        LoadOggVorbis(init, SND_TuretRotate,    DIR_SOUND"/turrets/rotate.ogg"         );
        LoadOggVorbis(init, SND_TuretWakeUp,    DIR_SOUND"/turrets/arming.ogg"         );
        LoadOggVorbis(init, SND_TuretWakeDown,  DIR_SOUND"/turrets/unarming.ogg"       );
        
        LoadOggVorbis(init, SND_RocketShoot,    DIR_SOUND"/pickup/rocketshoot.ogg"     ); //
        LoadOggVorbis(init, SND_Shoot,          DIR_SOUND"/pickup/shoot.ogg"           ); //
        LoadOggVorbis(init, SND_MinigunFire,    DIR_SOUND"/pickup/MinigunFire.ogg"     );
        LoadOggVorbis(init, SND_WeapReload,     DIR_SOUND"/pickup/reload.ogg"          );
        LoadOggVorbis(init, SND_ACShoot,        DIR_SOUND"/pickup/ac_shoot.ogg"        );
        LoadOggVorbis(init, SND_AC_Reload,      DIR_SOUND"/pickup/ac_reload.ogg"       );
        LoadOggVorbis(init, SND_Pickup,         DIR_SOUND"/pickup/pickup.ogg"          );
        LoadOggVorbis(init, SND_B_Start,        DIR_SOUND"/pickup/b_start.ogg"         );
        LoadOggVorbis(init, SND_B_Loop,         DIR_SOUND"/pickup/b_loop.ogg"          );
        LoadOggVorbis(init, SND_B_End,          DIR_SOUND"/pickup/b_end.ogg"           );
        LoadOggVorbis(init, SND_w_Pickup,       DIR_SOUND"/pickup/w_pickup.ogg"        ); //
        LoadOggVorbis(init, SND_Bolt,           DIR_SOUND"/pickup/boltshoot.ogg"       );
        LoadOggVorbis(init, SND_DiskFire,       DIR_SOUND"/pickup/ripper.ogg"          ); //
        LoadOggVorbis(init, SND_puRespawn,      DIR_SOUND"/pickup/puRespawn.ogg"       );
        LoadOggVorbis(init, SND_TowerRotate,    DIR_SOUND"/pickup/tower_rotate.ogg"    );
        LoadOggVorbis(init, SND_ShockActivate,  DIR_SOUND"/pickup/shockactivate.ogg"   ); //
        LoadOggVorbis(init, SND_BfgInit,        DIR_SOUND"/pickup/bfginit.ogg"         );
        LoadOggVorbis(init, SND_BfgFire,        DIR_SOUND"/pickup/bfgfire.ogg"         );
        LoadOggVorbis(init, SND_PlazmaFire,     DIR_SOUND"/pickup/plazma1.ogg"         );
        LoadOggVorbis(init, SND_RamEngine,      DIR_SOUND"/pickup/ram_engine.ogg"      ); //
        LoadOggVorbis(init, SND_InvEnd,         DIR_SOUND"/pickup/inv_end.ogg"         );
        LoadOggVorbis(init, SND_Inv,            DIR_SOUND"/pickup/inv.ogg"             );
        LoadOggVorbis(init, SND_InvHit1,        DIR_SOUND"/pickup/inv_hit1.ogg"        );
        LoadOggVorbis(init, SND_InvHit2,        DIR_SOUND"/pickup/inv_hit2.ogg"        );
        
        LoadOggVorbis(init, SND_Impact1,        DIR_SOUND"/vehicle/impact1.ogg"        );
        LoadOggVorbis(init, SND_Impact2,        DIR_SOUND"/vehicle/impact2.ogg"        );
        LoadOggVorbis(init, SND_Slide1,         DIR_SOUND"/vehicle/slide1.ogg"         );
        LoadOggVorbis(init, SND_TankMove,       DIR_SOUND"/vehicle/tank_move.ogg"      );
        
        LoadOggVorbis(init, SND_Screenshot,     DIR_SOUND"/misc/screenshot.ogg"      ); //
        LoadOggVorbis(init, SND_Limit,          DIR_SOUND"/misc/limit.ogg"           );
        LoadOggVorbis(init, SND_LightSwitch,    DIR_SOUND"/misc/light1.ogg"          ); //
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
	InitSound(false);
    //	g_music = NULL;
    //	SAFE_DELETE(g_soundManager);
}

// end of file
