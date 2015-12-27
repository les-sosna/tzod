#include "SoundHarness.h"
#include "OggVorbis.h"
#include "SoundTemplates.h"
#include "inc/audio/SoundRender.h"
#ifdef WIN32
#include "inc/audio/SoundRenderXA2.h"
#endif
#include "inc/audio/SoundView.h"
#include <as/AppState.h>
#include <fs/FileSystem.h>
#include <ctx/GameContextBase.h>

SoundView::SoundView(AppState &appState, FS::FileSystem &fs)
	: AppStateListener(appState)
#ifdef WIN32
	, _soundRender(new SoundRenderXA2())
#endif
{
	LoadBuffer(fs, SND_BoomStandard, "explosions/standard.ogg");
	LoadBuffer(fs, SND_BoomBig, "explosions/big.ogg");
	LoadBuffer(fs, SND_WallDestroy, "explosions/wall.ogg");

	LoadBuffer(fs, SND_Hit1, "projectiles/hit1.ogg");
	LoadBuffer(fs, SND_Hit3, "projectiles/hit2.ogg");
	LoadBuffer(fs, SND_Hit5, "projectiles/hit3.ogg");
	LoadBuffer(fs, SND_AC_Hit1, "projectiles/ac_hit_1.ogg");
	LoadBuffer(fs, SND_AC_Hit2, "projectiles/ac_hit_2.ogg");
	LoadBuffer(fs, SND_AC_Hit3, "projectiles/ac_hit_3.ogg");
	LoadBuffer(fs, SND_RocketFly, "projectiles/rocketfly.ogg"); //
	LoadBuffer(fs, SND_DiskHit, "projectiles/DiskHit.ogg"); //
	LoadBuffer(fs, SND_BfgFlash, "projectiles/bfgflash.ogg"); //
	LoadBuffer(fs, SND_PlazmaHit, "projectiles/plazmahit.ogg");
	LoadBuffer(fs, SND_BoomBullet, "projectiles/bullet.ogg"); //

	LoadBuffer(fs, SND_TargetLock, "turrets/activate.ogg");
	LoadBuffer(fs, SND_TuretRotate, "turrets/rotate.ogg");
	LoadBuffer(fs, SND_TuretWakeUp, "turrets/arming.ogg");
	LoadBuffer(fs, SND_TuretWakeDown, "turrets/unarming.ogg");

	LoadBuffer(fs, SND_RocketShoot, "pickup/rocketshoot.ogg"); //
	LoadBuffer(fs, SND_Shoot, "pickup/Shoot.ogg"); //
	LoadBuffer(fs, SND_MinigunFire, "pickup/MinigunFire.ogg");
	LoadBuffer(fs, SND_WeapReload, "pickup/reload.ogg");
	LoadBuffer(fs, SND_ACShoot, "pickup/ac_shoot.ogg");
	LoadBuffer(fs, SND_AC_Reload, "pickup/ac_reload.ogg");
	LoadBuffer(fs, SND_Pickup, "pickup/pickup.ogg");
	LoadBuffer(fs, SND_B_Start, "pickup/b_start.ogg");
	LoadBuffer(fs, SND_B_Loop, "pickup/b_loop.ogg");
	LoadBuffer(fs, SND_B_End, "pickup/b_end.ogg");
	LoadBuffer(fs, SND_w_Pickup, "pickup/w_pickup.ogg"); //
	LoadBuffer(fs, SND_Bolt, "pickup/boltshoot.ogg");
	LoadBuffer(fs, SND_DiskFire, "pickup/ripper.ogg"); //
	LoadBuffer(fs, SND_puRespawn, "pickup/puRespawn.ogg");
	LoadBuffer(fs, SND_TowerRotate, "pickup/tower_rotate.ogg");
	LoadBuffer(fs, SND_ShockActivate, "pickup/shockactivate.ogg"); //
	LoadBuffer(fs, SND_BfgInit, "pickup/bfginit.ogg");
	LoadBuffer(fs, SND_BfgFire, "pickup/bfgfire.ogg");
	LoadBuffer(fs, SND_PlazmaFire, "pickup/plazma1.ogg");
	LoadBuffer(fs, SND_RamEngine, "pickup/ram_engine.ogg"); //
	LoadBuffer(fs, SND_InvEnd, "pickup/inv_end.ogg");
	LoadBuffer(fs, SND_Inv, "pickup/inv.ogg");
	LoadBuffer(fs, SND_InvHit1, "pickup/inv_hit1.ogg");
	LoadBuffer(fs, SND_InvHit2, "pickup/inv_hit2.ogg");

	LoadBuffer(fs, SND_Impact1, "vehicle/impact1.ogg");
	LoadBuffer(fs, SND_Impact2, "vehicle/impact2.ogg");
	LoadBuffer(fs, SND_Slide1, "vehicle/slide1.ogg");
	LoadBuffer(fs, SND_TankMove, "vehicle/tank_move.ogg");

	LoadBuffer(fs, SND_Screenshot, "misc/screenshot.ogg"); //
	LoadBuffer(fs, SND_Limit, "misc/limit.ogg");
	LoadBuffer(fs, SND_LightSwitch, "misc/light1.ogg"); //

	OnGameContextChanged();
}

SoundView::~SoundView()
{
}

void SoundView::LoadBuffer(FS::FileSystem &fs, SoundTemplate st, const char *fileName)
try
{
	FormatDesc fd;
	std::vector<char> data;
	LoadOggVorbis(fs.Open(fileName)->QueryStream(), fd, data);
	_soundRender->LoadBuffer(st, data.data(), data.size(), fd);
}
catch (const std::exception&)
{
	std::throw_with_nested(std::runtime_error(std::string("could not load '") + fileName + "'"));
}

void SoundView::Step()
{
	_soundRender->Step();
	if (_soundHarness)
		_soundHarness->Step();
	
//	if( _scriptEnvironment.music )
//		_scriptEnvironment.music->HandleBufferFilling();
}

void SoundView::OnGameContextChanging()
{
	_soundHarness.reset();
}

void SoundView::OnGameContextChanged()
{
	if (GameContextBase *gc = GetAppState().GetGameContext())
	{
		_soundHarness.reset(new SoundHarness(*_soundRender, gc->GetWorld()));
	}
}
