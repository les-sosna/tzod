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
#include <ctx/GameContextBase.h>
#include <fs/FileSystem.h>
#include <ui/ConsoleBuffer.h>

SoundView::SoundView(FS::FileSystem &fs, UI::ConsoleBuffer &logger, AppState &appState)
	: AppStateListener(appState)
#ifdef _WIN32
	, _soundRender(new SoundRenderXA2(logger))
#else
	, _soundRender(new SoundRenderOAL())
#endif
{
	LoadBuffer(fs, logger, SoundTemplate::BoomStandard, "explosions/standard.ogg");
	LoadBuffer(fs, logger, SoundTemplate::BoomBig, "explosions/big.ogg");
	LoadBuffer(fs, logger, SoundTemplate::WallDestroy, "explosions/wall.ogg");

	LoadBuffer(fs, logger, SoundTemplate::Hit1, "projectiles/hit1.ogg");
	LoadBuffer(fs, logger, SoundTemplate::Hit3, "projectiles/hit2.ogg");
	LoadBuffer(fs, logger, SoundTemplate::Hit5, "projectiles/hit3.ogg");
	LoadBuffer(fs, logger, SoundTemplate::AC_Hit1, "projectiles/ac_hit_1.ogg");
	LoadBuffer(fs, logger, SoundTemplate::AC_Hit2, "projectiles/ac_hit_2.ogg");
	LoadBuffer(fs, logger, SoundTemplate::AC_Hit3, "projectiles/ac_hit_3.ogg");
	LoadBuffer(fs, logger, SoundTemplate::RocketFly, "projectiles/rocketfly.ogg"); //
	LoadBuffer(fs, logger, SoundTemplate::DiskHit, "projectiles/DiskHit.ogg"); //
	LoadBuffer(fs, logger, SoundTemplate::BfgFlash, "projectiles/bfgflash.ogg"); //
	LoadBuffer(fs, logger, SoundTemplate::PlazmaHit, "projectiles/plazmahit.ogg");
	LoadBuffer(fs, logger, SoundTemplate::BoomBullet, "projectiles/bullet.ogg"); //

	LoadBuffer(fs, logger, SoundTemplate::TargetLock, "turrets/activate.ogg");
	LoadBuffer(fs, logger, SoundTemplate::TuretRotate, "turrets/rotate.ogg");
	LoadBuffer(fs, logger, SoundTemplate::TuretWakeUp, "turrets/arming.ogg");
	LoadBuffer(fs, logger, SoundTemplate::TuretWakeDown, "turrets/unarming.ogg");

	LoadBuffer(fs, logger, SoundTemplate::RocketShoot, "pickup/rocketshoot.ogg"); //
	LoadBuffer(fs, logger, SoundTemplate::Shoot, "pickup/Shoot.ogg"); //
	LoadBuffer(fs, logger, SoundTemplate::MinigunFire, "pickup/MinigunFire.ogg");
	LoadBuffer(fs, logger, SoundTemplate::WeapReload, "pickup/reload.ogg");
	LoadBuffer(fs, logger, SoundTemplate::ACShoot, "pickup/ac_shoot.ogg");
	LoadBuffer(fs, logger, SoundTemplate::AC_Reload, "pickup/ac_reload.ogg");
	LoadBuffer(fs, logger, SoundTemplate::Pickup, "pickup/pickup.ogg");
	LoadBuffer(fs, logger, SoundTemplate::B_Start, "pickup/b_start.ogg");
	LoadBuffer(fs, logger, SoundTemplate::B_Loop, "pickup/b_loop.ogg");
	LoadBuffer(fs, logger, SoundTemplate::B_End, "pickup/b_end.ogg");
	LoadBuffer(fs, logger, SoundTemplate::w_Pickup, "pickup/w_pickup.ogg"); //
	LoadBuffer(fs, logger, SoundTemplate::Bolt, "pickup/boltshoot.ogg");
	LoadBuffer(fs, logger, SoundTemplate::DiskFire, "pickup/ripper.ogg"); //
	LoadBuffer(fs, logger, SoundTemplate::puRespawn, "pickup/puRespawn.ogg");
	LoadBuffer(fs, logger, SoundTemplate::TowerRotate, "pickup/tower_rotate.ogg");
	LoadBuffer(fs, logger, SoundTemplate::ShockActivate, "pickup/shockactivate.ogg"); //
	LoadBuffer(fs, logger, SoundTemplate::BfgInit, "pickup/bfginit.ogg");
	LoadBuffer(fs, logger, SoundTemplate::BfgFire, "pickup/bfgfire.ogg");
	LoadBuffer(fs, logger, SoundTemplate::PlazmaFire, "pickup/plazma1.ogg");
	LoadBuffer(fs, logger, SoundTemplate::RamEngine, "pickup/ram_engine.ogg"); //
	LoadBuffer(fs, logger, SoundTemplate::InvEnd, "pickup/inv_end.ogg");
	LoadBuffer(fs, logger, SoundTemplate::Inv, "pickup/inv.ogg");
	LoadBuffer(fs, logger, SoundTemplate::InvHit1, "pickup/inv_hit1.ogg");
	LoadBuffer(fs, logger, SoundTemplate::InvHit2, "pickup/inv_hit2.ogg");

	LoadBuffer(fs, logger, SoundTemplate::Impact1, "vehicle/impact1.ogg");
	LoadBuffer(fs, logger, SoundTemplate::Impact2, "vehicle/impact2.ogg");
	LoadBuffer(fs, logger, SoundTemplate::Slide1, "vehicle/slide1.ogg");
	LoadBuffer(fs, logger, SoundTemplate::TankMove, "vehicle/tank_move.ogg");

	LoadBuffer(fs, logger, SoundTemplate::Screenshot, "misc/screenshot.ogg"); //
	LoadBuffer(fs, logger, SoundTemplate::Limit, "misc/limit.ogg");
	LoadBuffer(fs, logger, SoundTemplate::LightSwitch, "misc/light1.ogg"); //
	LoadBuffer(fs, logger, SoundTemplate::Beep, "misc/beep.ogg"); // http://soundbible.com/1133-Beep-Ping.html

	OnGameContextChanged();
}

SoundView::~SoundView()
{
}

void SoundView::LoadBuffer(FS::FileSystem &fs, UI::ConsoleBuffer &logger, SoundTemplate st, const char *fileName)
try
{
	FormatDesc fd;
	std::vector<char> data;
	LoadOggVorbis(fs.Open(fileName)->QueryStream(), fd, data);
	_soundRender->LoadBuffer(st, data.data(), data.size(), fd);
}
catch (const std::exception &e)
{
	logger.Format(1) << "Could not load '" << fileName << "' - " << e.what();
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
