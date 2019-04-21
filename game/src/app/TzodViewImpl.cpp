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
		if (textureManager.LoadPackage(render, ParsePackage(FILE_TEXTURES, fs.Open(FILE_TEXTURES)->QueryMap(), fs)) <= 0)
			logger.Printf(1, "WARNING: no textures loaded");
		if (textureManager.LoadPackage(render, ParseDirectory(DIR_SKINS, "skin/", fs)) <= 0)
			logger.Printf(1, "WARNING: no skins found");
	}
	catch(...)
	{
		textureManager.UnloadAllTextures(render);
		throw;
	}
	return textureManager;
}

TzodViewImpl::TzodViewImpl(FS::FileSystem &fs, Plat::ConsoleBuffer &logger, Plat::Input &input, IRender& render, TzodApp &app)
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
		logger))
	, uiInputRenderingController(input, textureManager, timeStepManager, desktop)
#ifndef NOSOUND
	, soundView(app.GetShellConfig().s_enabled.Get() ? std::make_unique<SoundView>(*fs.GetFileSystem(DIR_SOUND), logger, app.GetAppState()) : nullptr)
#endif
{
}

TzodViewImpl::~TzodViewImpl()
{
}
