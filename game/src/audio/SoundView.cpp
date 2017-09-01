#include "SoundHarness.h"
#include "OggVorbis.h"
#include "SoundTemplates.h"
#include "inc/audio/SoundRender.h"
#ifdef _WIN32
#include "inc/audio/SoundRenderXA2.h"
#else
#include "inc/audio/SoundRenderOAL.h"
#endif
#include "inc/audio/SoundView.h"
#include <as/AppState.h>
#include <fs/FileSystem.h>
#include <ctx/GameContextBase.h>

SoundView::SoundView(FS::FileSystem &fs, UI::ConsoleBuffer &logger, AppState &appState)
	: AppStateListener(appState)
#ifdef _WIN32
	, _soundRender(new SoundRenderXA2(logger))
#else
	, _soundRender(new SoundRenderOAL())
#endif
{
	LoadBuffer(fs, SoundTemplate::BoomStandard, "explosions/standard.ogg");
	LoadBuffer(fs, SoundTemplate::BoomBig, "explosions/big.ogg");
	LoadBuffer(fs, SoundTemplate::WallDestroy, "explosions/wall.ogg");

	LoadBuffer(fs, SoundTemplate::Hit1, "projectiles/hit1.ogg");
	LoadBuffer(fs, SoundTemplate::Hit3, "projectiles/hit2.ogg");
	LoadBuffer(fs, SoundTemplate::Hit5, "projectiles/hit3.ogg");
	LoadBuffer(fs, SoundTemplate::AC_Hit1, "projectiles/ac_hit_1.ogg");
	LoadBuffer(fs, SoundTemplate::AC_Hit2, "projectiles/ac_hit_2.ogg");
	LoadBuffer(fs, SoundTemplate::AC_Hit3, "projectiles/ac_hit_3.ogg");
	LoadBuffer(fs, SoundTemplate::RocketFly, "projectiles/rocketfly.ogg"); //
	LoadBuffer(fs, SoundTemplate::DiskHit, "projectiles/DiskHit.ogg"); //
	LoadBuffer(fs, SoundTemplate::BfgFlash, "projectiles/bfgflash.ogg"); //
	LoadBuffer(fs, SoundTemplate::PlazmaHit, "projectiles/plazmahit.ogg");
	LoadBuffer(fs, SoundTemplate::BoomBullet, "projectiles/bullet.ogg"); //

	LoadBuffer(fs, SoundTemplate::TargetLock, "turrets/activate.ogg");
	LoadBuffer(fs, SoundTemplate::TuretRotate, "turrets/rotate.ogg");
	LoadBuffer(fs, SoundTemplate::TuretWakeUp, "turrets/arming.ogg");
	LoadBuffer(fs, SoundTemplate::TuretWakeDown, "turrets/unarming.ogg");

	LoadBuffer(fs, SoundTemplate::RocketShoot, "pickup/rocketshoot.ogg"); //
	LoadBuffer(fs, SoundTemplate::Shoot, "pickup/Shoot.ogg"); //
	LoadBuffer(fs, SoundTemplate::MinigunFire, "pickup/MinigunFire.ogg");
	LoadBuffer(fs, SoundTemplate::WeapReload, "pickup/reload.ogg");
	LoadBuffer(fs, SoundTemplate::ACShoot, "pickup/ac_shoot.ogg");
	LoadBuffer(fs, SoundTemplate::AC_Reload, "pickup/ac_reload.ogg");
	LoadBuffer(fs, SoundTemplate::Pickup, "pickup/pickup.ogg");
	LoadBuffer(fs, SoundTemplate::B_Start, "pickup/b_start.ogg");
	LoadBuffer(fs, SoundTemplate::B_Loop, "pickup/b_loop.ogg");
	LoadBuffer(fs, SoundTemplate::B_End, "pickup/b_end.ogg");
	LoadBuffer(fs, SoundTemplate::w_Pickup, "pickup/w_pickup.ogg"); //
	LoadBuffer(fs, SoundTemplate::Bolt, "pickup/boltshoot.ogg");
	LoadBuffer(fs, SoundTemplate::DiskFire, "pickup/ripper.ogg"); //
	LoadBuffer(fs, SoundTemplate::puRespawn, "pickup/puRespawn.ogg");
	LoadBuffer(fs, SoundTemplate::TowerRotate, "pickup/tower_rotate.ogg");
	LoadBuffer(fs, SoundTemplate::ShockActivate, "pickup/shockactivate.ogg"); //
	LoadBuffer(fs, SoundTemplate::BfgInit, "pickup/bfginit.ogg");
	LoadBuffer(fs, SoundTemplate::BfgFire, "pickup/bfgfire.ogg");
	LoadBuffer(fs, SoundTemplate::PlazmaFire, "pickup/plazma1.ogg");
	LoadBuffer(fs, SoundTemplate::RamEngine, "pickup/ram_engine.ogg"); //
	LoadBuffer(fs, SoundTemplate::InvEnd, "pickup/inv_end.ogg");
	LoadBuffer(fs, SoundTemplate::Inv, "pickup/inv.ogg");
	LoadBuffer(fs, SoundTemplate::InvHit1, "pickup/inv_hit1.ogg");
	LoadBuffer(fs, SoundTemplate::InvHit2, "pickup/inv_hit2.ogg");

	LoadBuffer(fs, SoundTemplate::Impact1, "vehicle/impact1.ogg");
	LoadBuffer(fs, SoundTemplate::Impact2, "vehicle/impact2.ogg");
	LoadBuffer(fs, SoundTemplate::Slide1, "vehicle/slide1.ogg");
	LoadBuffer(fs, SoundTemplate::TankMove, "vehicle/tank_move.ogg");

	LoadBuffer(fs, SoundTemplate::Screenshot, "misc/screenshot.ogg"); //
	LoadBuffer(fs, SoundTemplate::Limit, "misc/limit.ogg");
	LoadBuffer(fs, SoundTemplate::LightSwitch, "misc/light1.ogg"); //
	LoadBuffer(fs, SoundTemplate::Beep, "misc/beep.ogg"); // http://soundbible.com/1133-Beep-Ping.html

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
		_soundHarness.reset(new SoundHarness(*_soundRender, *gc, gc->GetGameplay()));
	}
}
