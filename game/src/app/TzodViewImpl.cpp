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

static TextureManager InitTextureManager(FS::FileSystem &fs, Plat::ConsoleBuffer &logger, IRender &render)
{
	TextureManager textureManager(render);
	try
	{
		textureManager.LoadPackage(render, fs, ParseDirectory(fs, DIR_SPRITES));
	}
	catch(...)
	{
		textureManager.UnloadAllTextures(render);
		throw;
	}
	return textureManager;
}

TzodViewImpl::TzodViewImpl(FS::FileSystem &fs, Plat::AppWindowCommandClose* cmdClose, Plat::ConsoleBuffer &logger, Plat::Input &input, IRender& render, TzodApp &app)
	: textureManager(InitTextureManager(fs, logger, render))
	, timeStepManager()
	, desktop(std::make_shared<Desktop>(
		timeStepManager,
		textureManager,
		app.GetAppState(),
		app.GetAppConfig(),
		app.GetAppController(),
		fs,
		app.GetShellConfig(),
		app.GetLang(),
		app.GetDMCampaign(),
		logger,
		cmdClose))
	, uiInputRenderingController(input, textureManager, timeStepManager, desktop)
#ifndef NOSOUND
	, soundView(app.GetShellConfig().s_enabled.Get() ? std::make_unique<SoundView>(*fs.GetFileSystem(DIR_SOUND), logger, app.GetAppState()) : nullptr)
#endif
{
}

TzodViewImpl::~TzodViewImpl()
{
}
