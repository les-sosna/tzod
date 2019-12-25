#include "TzodViewImpl.h"
#include "inc/app/tzod.h"
#include <as/AppConstants.h>
#include <fs/FileSystem.h>
#include <shell/Config.h>
#include <shell/Desktop.h>
#include <plat/ConsoleBuffer.h>
#ifndef NOSOUND
# include <audio/SoundView.h>
#endif

static auto InitTextureManager(FS::FileSystem &fs, ImageCache &imageCache)
{
	TextureManager textureManager;
	try
	{
		textureManager.LoadPackage(fs, imageCache, ParseDirectory(fs, DIR_SPRITES));
	}
	catch(const std::runtime_error&)
	{
		textureManager.UnloadAllTextures();
		throw;
	}
	return textureManager;
}

TzodViewImpl::TzodViewImpl(FS::FileSystem &fs, Plat::AppWindowCommandClose* cmdClose, Plat::ConsoleBuffer &logger, TzodApp &app)
	: textureManager(InitTextureManager(fs, imageCache))
	, timeStepManager()
	, desktop(std::make_shared<Desktop>(
		timeStepManager,
		textureManager,
		app.GetAppState(),
		app.GetMapCollection(),
		app.GetAppConfig(),
		app.GetAppController(),
		fs,
		app.GetShellConfig(),
		app.GetLang(),
		app.GetDMCampaign(),
		logger,
		cmdClose))
	, uiInputRenderingController(fs, textureManager, timeStepManager, desktop)
#ifndef NOSOUND
	, soundView(app.GetShellConfig().s_enabled.Get() ? std::make_unique<SoundView>(*fs.GetFileSystem(DIR_SOUND), logger, app.GetAppState()) : nullptr)
#endif
{
}

TzodViewImpl::~TzodViewImpl()
{
}
