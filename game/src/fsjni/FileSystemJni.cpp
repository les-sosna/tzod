#include "inc/fsjni/FileSystemJni.h"
#include <cassert>

using namespace FS;

FileSystemJni::OSFile::OSFile(const std::string &fileName, FileMode mode)
    : _mode(mode)
    , _mapped(false)
    , _streamed(false)
{
    throw std::runtime_error("not implemented");
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
    Seek(0, SEEK_SET);
}

FileSystemJni::OSFile::OSStream::~OSStream()
{
    _file->Unstream();
}

size_t FileSystemJni::OSFile::OSStream::Read(void *dst, size_t size, size_t count)
{
    throw std::runtime_error("not implemented");
    return 0;
}

void FileSystemJni::OSFile::OSStream::Write(const void *, size_t)
{
    assert(false); // read only file system
}

void FileSystemJni::OSFile::OSStream::Seek(long long amount, unsigned int origin)
{
    throw std::runtime_error("not implemented");
}

long long FileSystemJni::OSFile::OSStream::Tell() const
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

FileSystemJni::OSFile::OSMemMap::OSMemMap(std::shared_ptr<OSFile> parent)
    : _file(parent)
{
    throw std::runtime_error("not implemented");
}

FileSystemJni::OSFile::OSMemMap::~OSMemMap()
{
    _file->Unmap();
}

char* FileSystemJni::OSFile::OSMemMap::GetData()
{
    return _data.empty() ? nullptr : &_data[0];
}

unsigned long FileSystemJni::OSFile::OSMemMap::GetSize() const
{
    return _data.size();
}

void FileSystemJni::OSFile::OSMemMap::SetSize(unsigned long size)
{
    _data.resize(size);
}

///////////////////////////////////////////////////////////////////////////////

FileSystemJni::FileSystemJni(std::string rootDirectory)
	: _rootDirectory(std::move(rootDirectory))
{
    throw std::runtime_error("not implemented");
}

std::vector<std::string> FileSystemJni::EnumAllFiles(std::string_view mask)
{
	std::vector<std::string> files;
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
    return std::make_shared<OSFile>(PathCombine(_rootDirectory, fileName), mode);
}

std::shared_ptr<FileSystem> FileSystemJni::GetFileSystem(std::string_view path, bool create, bool nothrow)
{
	if( auto fs = FileSystem::GetFileSystem(path, create, true /*nothrow*/) )
    {
        return fs;
    }

    assert(!path.empty());

    // skip delimiters at the beginning
    auto offset = path.find_first_not_of('/');
    assert(std::string::npos != offset);

    auto p = path.find('/', offset);
    auto dirName = path.substr(offset, std::string::npos != p ? p - offset : p);
    auto tmpDir = PathCombine(_rootDirectory, dirName);

//    if( !exists(tmpDir) )
    {
        if( create )
        {
            if( nothrow )
                return nullptr;
            else
                throw std::runtime_error("Could not create directory: " + tmpDir + ": Read only file system");
        }
        else
        {
            if( nothrow )
                return nullptr;
            else
				throw std::runtime_error(tmpDir + ": Path not found");
        }
    }
//    else if( !isdir() )
//    {
//        if( nothrow )
//            return nullptr;
//        else
//			throw std::runtime_error(tmpDir + ": Not a directory");
//    }

    // at this point the directory was either found or created
	auto child = std::make_shared<FileSystemJni>(PathCombine(_rootDirectory, dirName));
    Mount(dirName, child);
    if( std::string::npos != p )
        return child->GetFileSystem(path.substr(p), create, nothrow); // process the rest of the path
    return child; // last path node was processed
}
