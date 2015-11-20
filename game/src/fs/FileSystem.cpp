#include "inc/fs/FileSystem.h"
#include <cassert>

void FS::FileSystem::Mount(const std::string &nodeName, std::shared_ptr<FileSystem> fs)
{
	assert(!nodeName.empty() && std::string::npos == nodeName.find('/'));
	_children[nodeName] = fs;
}

std::shared_ptr<FS::File> FS::FileSystem::Open(const std::string &fileName, FileMode mode)
{
	std::string::size_type pd = fileName.rfind('/');
	if( pd && std::string::npos != pd ) // was a path delimiter found?
	{
		return GetFileSystem(fileName.substr(0, pd))->RawOpen(fileName.substr(pd + 1), mode);
	}
	return RawOpen(fileName, mode);
}

std::vector<std::string> FS::FileSystem::EnumAllFiles(const std::string &mask)
{
	// base file system can't contain any files
	return std::vector<std::string>();
}

std::shared_ptr<FS::File> FS::FileSystem::RawOpen(const std::string &fileName, FileMode mode)
{
	throw std::runtime_error("Base file system can't contain any files");
	return nullptr;
}

std::shared_ptr<FS::FileSystem> FS::FileSystem::GetFileSystem(const std::string &path, bool create, bool nothrow)
{
	assert(!path.empty());

	// skip delimiters at the beginning
	std::string::size_type offset = path.find_first_not_of('/');
	assert(std::string::npos != offset);

	std::string::size_type p = path.find('/', offset);
	std::string dirName = path.substr(offset, std::string::npos != p ? p - offset : p);

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
