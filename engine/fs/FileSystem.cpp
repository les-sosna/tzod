#include "inc/fs/FileSystem.h"
#include <cassert>
#include <stdexcept>

using namespace FS;

void FileSystem::Mount(std::string_view nodeName, std::shared_ptr<FileSystem> fs)
{
	assert(!nodeName.empty() && std::string::npos == nodeName.find('/'));
	_children.emplace(nodeName, fs);
}

std::shared_ptr<FS::File> FileSystem::Open(std::string_view path, FileMode mode, bool nothrow)
{
	auto pd = path.rfind('/');
	if( pd && std::string::npos != pd ) // was a path delimiter found?
	{
		auto fs = GetFileSystem(path.substr(0, pd), false, nothrow);
		return fs ? fs->RawOpen(path.substr(pd + 1), mode, nothrow) : nullptr;
	}
	return RawOpen(path, mode, nothrow);
}

std::vector<std::string> FileSystem::EnumAllFiles(std::string_view mask)
{
	// base file system can't contain any files
	return std::vector<std::string>();
}

std::shared_ptr<FS::FileSystem> FileSystem::GetFileSystem(std::string_view path, bool create, bool nothrow)
{
	assert(!path.empty());

	// skip delimiters at the beginning
	auto offset = path.find_first_not_of('/');
	assert(std::string::npos != offset);

	auto p = path.find('/', offset);
	auto dirName = path.substr(offset, std::string::npos != p ? p - offset : p);

	auto it = _children.find(dirName);
	if( _children.end() == it )
	{
		if( nothrow )
			return nullptr;
		else
			throw std::runtime_error("node not found in base file system");
	}

	if( std::string::npos != p )
		return it->second->GetFileSystem(path.substr(p), create, nothrow);

	return it->second;
}

std::string FS::PathCombine(std::string_view first, std::string_view second)
{
	std::string result;
	assert(!second.empty());
	if (first.empty())
	{
		result = second;
	}
	else
	{
		result.reserve(first.size() + second.size() + 1);
		result.append(first).append(1, '/').append(second);
	}
	return result;
}
