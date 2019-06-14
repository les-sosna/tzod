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

static auto GetTzodTextures(FS::FileSystem& fs)
{
	auto skins = ParseDirectory(DIR_SKINS, "skin/", fs, false /*magFilter*/);
	auto textures = ParsePackage(FILE_TEXTURES, fs.Open(FILE_TEXTURES)->QueryMap(), fs);
	textures.insert(textures.end(), std::make_move_iterator(skins.begin()), std::make_move_iterator(skins.end()));
	return textures;
}

static auto InitTextureManager(FS::FileSystem &fs)
{
	TextureManager textureManager;
	textureManager.LoadPackage(fs, GetTzodTextures(fs));
	return textureManager;
}

TzodViewImpl::TzodViewImpl(FS::FileSystem &fs, Plat::AppWindowCommandClose* cmdClose, Plat::ConsoleBuffer &logger, TzodApp &app)
	: textureManager(InitTextureManager(fs))
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
	, uiInputRenderingController(textureManager, timeStepManager, desktop)
#ifndef NOSOUND
	, soundView(app.GetShellConfig().s_enabled.Get() ? std::make_unique<SoundView>(*fs.GetFileSystem(DIR_SOUND), logger, app.GetAppState()) : nullptr)
#endif
{
}

TzodViewImpl::~TzodViewImpl()
{
}
