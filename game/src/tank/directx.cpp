// directx.cpp
//------------------------------------------------------------------------

#include "stdafx.h"

#include "directx.h"

#include "Macros.h"

#include "core/Debug.h"

#include "sound/sfx.h"
#include "sound/MusicPlayer.h"

///////////////////////////////////////////////////////////////////////////////////////////

#if !defined NOSOUND

HRESULT InitDirectSound(HWND hWnd, bool init)
{
	HRESULT hr = S_OK;

	if( init )
	{
		assert(!g_soundManager);
		TRACE("Init direct sound...");
		g_soundManager = new CSoundManager();
		if( FAILED(hr = g_soundManager->Initialize(hWnd, DSSCL_PRIORITY, 2, 44100, 16)) )
		{
			if( FAILED(hr = g_soundManager->Initialize(hWnd, DSSCL_NORMAL, 2, 44100, 16)) )
			{
				TRACE("ERROR: direct sound init failed");
				SAFE_DELETE(g_soundManager);
			}
		}
	}
	else
	{
		TRACE("free direct sound...");
	}

	if( !g_soundManager ) return hr;


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
		if( init ) FreeDirectSound();
		throw;
	}

	return S_OK;
}

void FreeDirectSound()
{
	InitDirectSound(NULL, 0);
	g_music = NULL;
	SAFE_DELETE(g_soundManager);
}

#endif

///////////////////////////////////////////////////////////////////////////////
// end of file
