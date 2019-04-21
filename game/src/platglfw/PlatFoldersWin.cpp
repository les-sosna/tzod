#define WIN32_LEAN_AND_MEAN
#include <Shlobj.h>
#include <plat/Folders.h>
#include <utf8.h>

static std::string w2s(std::wstring_view w)
{
	std::string s;
	s.reserve(w.size());
	utf8::utf16to8(w.begin(), w.end(), std::back_inserter(s));
	return s;
}

std::string Plat::GetAppDataFolder()
{
	std::string result;
	PWSTR path = nullptr;
	HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, &path);
	if (FAILED(hr))
		throw std::runtime_error("Failed to get AppData folder");
	if (path)
	{
		result = w2s(path);
		CoTaskMemFree(path);
		path = nullptr;
	}
	return result;
}
