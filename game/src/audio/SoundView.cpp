#include "SoundHarness.h"
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
#include <plat/ConsoleBuffer.h>
#include <wavfile/WavFile.h>

SoundView::SoundView(FS::FileSystem &fs, Plat::ConsoleBuffer &logger, AppState &appState)
	: AppStateListener(appState)
#ifdef _WIN32
	, _soundRender(new SoundRenderXA2(logger))
#else
	, _soundRender(new SoundRenderOAL())
#endif
{
	LoadBuffer(fs, logger, SoundTemplate::BoomStandard, "explosions/standard");
	LoadBuffer(fs, logger, SoundTemplate::BoomBig, "explosions/big");
	LoadBuffer(fs, logger, SoundTemplate::WallDestroy, "explosions/wall");

	LoadBuffer(fs, logger, SoundTemplate::Hit1, "projectiles/hit1");
	LoadBuffer(fs, logger, SoundTemplate::Hit3, "projectiles/hit2");
	LoadBuffer(fs, logger, SoundTemplate::Hit5, "projectiles/hit3");
	LoadBuffer(fs, logger, SoundTemplate::AC_Hit1, "projectiles/ac_hit_1");
	LoadBuffer(fs, logger, SoundTemplate::AC_Hit2, "projectiles/ac_hit_2");
	LoadBuffer(fs, logger, SoundTemplate::AC_Hit3, "projectiles/ac_hit_3");
	LoadBuffer(fs, logger, SoundTemplate::RocketFly, "projectiles/rocketfly"); //
	LoadBuffer(fs, logger, SoundTemplate::DiskHit, "projectiles/DiskHit"); //
	LoadBuffer(fs, logger, SoundTemplate::BfgFlash, "projectiles/bfgflash"); //
	LoadBuffer(fs, logger, SoundTemplate::PlazmaHit, "projectiles/plazmahit");
	LoadBuffer(fs, logger, SoundTemplate::BoomBullet, "projectiles/bullet"); //

	LoadBuffer(fs, logger, SoundTemplate::TargetLock, "turrets/activate");
	LoadBuffer(fs, logger, SoundTemplate::TuretRotate, "turrets/rotate");
	LoadBuffer(fs, logger, SoundTemplate::TuretWakeUp, "turrets/arming");
	LoadBuffer(fs, logger, SoundTemplate::TuretWakeDown, "turrets/unarming");

	LoadBuffer(fs, logger, SoundTemplate::RocketShoot, "pickup/rocketshoot"); //
	LoadBuffer(fs, logger, SoundTemplate::Shoot, "pickup/Shoot"); //
	LoadBuffer(fs, logger, SoundTemplate::MinigunFire, "pickup/MinigunFire");
	LoadBuffer(fs, logger, SoundTemplate::WeapReload, "pickup/reload");
	LoadBuffer(fs, logger, SoundTemplate::ACShoot, "pickup/ac_shoot");
	LoadBuffer(fs, logger, SoundTemplate::AC_Reload, "pickup/ac_reload");
	LoadBuffer(fs, logger, SoundTemplate::Pickup, "pickup/pickup");
	LoadBuffer(fs, logger, SoundTemplate::B_Start, "pickup/b_start");
	LoadBuffer(fs, logger, SoundTemplate::B_Loop, "pickup/b_loop");
	LoadBuffer(fs, logger, SoundTemplate::B_End, "pickup/b_end");
	LoadBuffer(fs, logger, SoundTemplate::w_Pickup, "pickup/w_pickup"); //
	LoadBuffer(fs, logger, SoundTemplate::Bolt, "pickup/boltshoot");
	LoadBuffer(fs, logger, SoundTemplate::DiskFire, "pickup/ripper"); //
	LoadBuffer(fs, logger, SoundTemplate::puRespawn, "pickup/puRespawn");
	LoadBuffer(fs, logger, SoundTemplate::TowerRotate, "pickup/tower_rotate");
	LoadBuffer(fs, logger, SoundTemplate::ShockActivate, "pickup/shockactivate"); //
	LoadBuffer(fs, logger, SoundTemplate::BfgInit, "pickup/bfginit");
	LoadBuffer(fs, logger, SoundTemplate::BfgFire, "pickup/bfgfire");
	LoadBuffer(fs, logger, SoundTemplate::PlazmaFire, "pickup/plazma1");
	LoadBuffer(fs, logger, SoundTemplate::RamEngine, "pickup/ram_engine"); //
	LoadBuffer(fs, logger, SoundTemplate::InvEnd, "pickup/inv_end");
	LoadBuffer(fs, logger, SoundTemplate::Inv, "pickup/inv");
	LoadBuffer(fs, logger, SoundTemplate::InvHit1, "pickup/inv_hit1");
	LoadBuffer(fs, logger, SoundTemplate::InvHit2, "pickup/inv_hit2");

	LoadBuffer(fs, logger, SoundTemplate::Impact1, "vehicle/impact1");
	LoadBuffer(fs, logger, SoundTemplate::Impact2, "vehicle/impact2");
	LoadBuffer(fs, logger, SoundTemplate::Slide1, "vehicle/slide1");
	LoadBuffer(fs, logger, SoundTemplate::TankMove, "vehicle/tank_move");

	LoadBuffer(fs, logger, SoundTemplate::Screenshot, "misc/screenshot"); //
	LoadBuffer(fs, logger, SoundTemplate::Limit, "misc/limit");
	LoadBuffer(fs, logger, SoundTemplate::LightSwitch, "misc/light1"); //
	LoadBuffer(fs, logger, SoundTemplate::Beep, "misc/beep"); // http://soundbible.com/1133-Beep-Ping.html

	OnGameContextChanged();
}

SoundView::~SoundView()
{
}

void SoundView::LoadBuffer(FS::FileSystem &fs, Plat::ConsoleBuffer &logger, SoundTemplate st, const char *fileName)
try
{
	FormatDesc fd;
	std::vector<char> data;
	fd.channels = 1;
	LoadWavPcm(*fs.Open(std::string(fileName).append(".wav"))->QueryStream(), fd.frequency, data);
	_soundRender->LoadBuffer(st, data.data(), data.size(), fd);
}
catch (const std::exception &e)
{
	logger.Format(1) << "Could not load '" << fileName << ".wav' - " << e.what();
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
	if (auto gc = GetAppState().GetGameContext())
	{
		_soundHarness.reset(new SoundHarness(*_soundRender, *gc, gc->GetGameplay()));
	}
}
