#include "inc/fsjni/FileSystemJni.h"
#include <android/asset_manager.h>
#include <cassert>

using namespace FS;

void AAssetDirDeleter::operator()(AAssetDir *assetDir)
{
    AAssetDir_close(assetDir);
}

void AAssetDeleter::operator()(AAsset *asset)
{
    AAsset_close(asset);
}

FileSystemJni::OSFile::OSFile(AAssetPtr asset)
    : _asset(std::move(asset))
{
    assert(_asset);
}

FileSystemJni::OSFile::~OSFile()
{
}

std::shared_ptr<MemMap> FileSystemJni::OSFile::QueryMap()
{
    assert(!_mapped && !_streamed);
	std::shared_ptr<MemMap> result = std::make_shared<OSMemMap>(shared_from_this());
    _mapped = true;
    return result;
}

std::shared_ptr<Stream> FileSystemJni::OSFile::QueryStream()
{
    assert(!_mapped && !_streamed);
    _streamed = true;
    return std::make_shared<OSStream>(shared_from_this());
}

void FileSystemJni::OSFile::Unmap()
{
    assert(_mapped && !_streamed);
    _mapped = false;
}

void FileSystemJni::OSFile::Unstream()
{
    assert(_streamed && !_mapped);
    _streamed = false;
}

///////////////////////////////////////////////////////////////////////////////

FileSystemJni::OSFile::OSStream::OSStream(std::shared_ptr<OSFile> parent)
    : _file(parent)
{
}

FileSystemJni::OSFile::OSStream::~OSStream()
{
    _file->Unstream();
}

size_t FileSystemJni::OSFile::OSStream::Read(void *dst, size_t size, size_t count)
{
    auto bytesRead = AAsset_read(_file->_asset.get(), dst, size * count);
    if (bytesRead < 0)
        throw std::runtime_error("Asset read error");
    if( bytesRead % size )
        throw std::runtime_error("Unexpected end of asset data");
    return bytesRead / size;
}

void FileSystemJni::OSFile::OSStream::Write(const void *, size_t)
{
    assert(false); // read only file system
}

void FileSystemJni::OSFile::OSStream::Seek(long long amount, unsigned int origin)
{
    AAsset_seek(_file->_asset.get(), amount, origin);
}

long long FileSystemJni::OSFile::OSStream::Tell() const
{
    return AAsset_seek(_file->_asset.get(), 0, SEEK_CUR);
}

///////////////////////////////////////////////////////////////////////////////

FileSystemJni::OSFile::OSMemMap::OSMemMap(std::shared_ptr<OSFile> parent)
    : _file(parent)
{
}

FileSystemJni::OSFile::OSMemMap::~OSMemMap()
{
    _file->Unmap();
}

const void* FileSystemJni::OSFile::OSMemMap::GetData() const
{
    return AAsset_getBuffer(_file->_asset.get());
}

unsigned long FileSystemJni::OSFile::OSMemMap::GetSize() const
{
    return static_cast<unsigned long>(AAsset_getLength(_file->_asset.get()));
}

void FileSystemJni::OSFile::OSMemMap::SetSize(unsigned long /*size*/)
{
    throw std::runtime_error("Operation not supported");
}

///////////////////////////////////////////////////////////////////////////////

FileSystemJni::FileSystemJni(AAssetManager *assetManager, std::string rootDirectory)
    : _assetManager(assetManager)
    , _rootDirectory(std::move(rootDirectory))
    , _assetDir(AAssetManager_openDir(_assetManager, _rootDirectory.c_str()))
{
    // If directory does not exist it'll appear empty. Since we can only list files but not
    // subdirectories without going to Java there is no good way to validate the returned pointer.
    // Assume the directory exists until we actually fail to open a file.
    if (!_assetDir)
        throw std::runtime_error(_rootDirectory + ": failed to open directory");
}

FileSystemJni::~FileSystemJni()
{
}

std::vector<std::string> FileSystemJni::EnumAllFiles(std::string_view mask)
{
	std::vector<std::string> files;
    AAssetDir_rewind(_assetDir.get());
    while (const char *file = AAssetDir_getNextFileName(_assetDir.get()))
        files.emplace_back(file);
	return files;
}

static std::string PathCombine(std::string_view first, std::string_view second)
{
	std::string result;
	result.reserve(first.size() + second.size() + 1);
	result.append(first).append(1, '/').append(second);
	return result;
}

std::shared_ptr<File> FileSystemJni::RawOpen(std::string_view fileName, FileMode mode)
{
    assert(FileMode::ModeRead == mode);
    AAssetPtr asset(AAssetManager_open(_assetManager, PathCombine(_rootDirectory, fileName).c_str(), AASSET_MODE_UNKNOWN));
    if (!asset)
        throw std::runtime_error(std::string(fileName) + ": Asset not found");
    return std::make_shared<OSFile>(std::move(asset));
}

std::shared_ptr<FileSystem> FileSystemJni::GetFileSystem(std::string_view path, bool create, bool nothrow)
{
	if( auto fs = FileSystem::GetFileSystem(path, create, true /*nothrow*/) )
    {
        return fs;
    }

    assert(!create);
    assert(!path.empty());

    // skip delimiters at the beginning
    auto offset = path.find_first_not_of('/');
    assert(std::string::npos != offset);

    auto p = path.find('/', offset);
    auto dirName = path.substr(offset, std::string::npos != p ? p - offset : p);

    auto child = std::make_shared<FileSystemJni>(_assetManager, PathCombine(_rootDirectory, dirName));
    Mount(dirName, child);
    if( std::string::npos != p )
        return child->GetFileSystem(path.substr(p), create, nothrow); // process the rest of the path
    return child; // last path node was processed
}
