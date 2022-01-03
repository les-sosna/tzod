#include "inc/fsposix/FileSystemPosix.h"
#include <cassert>
#include <cerrno>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <string.h>
#include <stdexcept>

using namespace FS;
using namespace FS::detail;

void StdioFileDeleter::operator()(FILE *file)
{
    if (file)
        fclose(file);
}

FS::FileSystemPosix::OSFile::OSFile(StdioFile file)
    : _file(std::move(file))
    , _mapped(false)
    , _streamed(false)
{
    assert(_file);
}

FS::FileSystemPosix::OSFile::~OSFile()
{
}

std::shared_ptr<FS::MemMap> FS::FileSystemPosix::OSFile::QueryMap()
{
    assert(!_mapped && !_streamed);
	std::shared_ptr<MemMap> result = std::make_shared<OSMemMap>(shared_from_this());
    _mapped = true;
    return result;
}

std::shared_ptr<FS::Stream> FS::FileSystemPosix::OSFile::QueryStream()
{
    assert(!_mapped && !_streamed);
    _streamed = true;
    return std::make_shared<OSStream>(shared_from_this());
}

void FS::FileSystemPosix::OSFile::Unmap()
{
    assert(_mapped && !_streamed);
    _mapped = false;
}

void FS::FileSystemPosix::OSFile::Unstream()
{
    assert(_streamed && !_mapped);
    _streamed = false;
}

///////////////////////////////////////////////////////////////////////////////

FS::FileSystemPosix::OSFile::OSStream::OSStream(std::shared_ptr<OSFile> parent)
    : _file(parent)
{
    Seek(0, SEEK_SET);
}

FS::FileSystemPosix::OSFile::OSStream::~OSStream()
{
    _file->Unstream();
}

size_t FS::FileSystemPosix::OSFile::OSStream::Read(void *dst, size_t size, size_t count)
{
    size_t result = fread(dst, size, count, _file->_file.get());
    if( count != result && ferror(_file->_file.get()) )
        throw std::runtime_error("read file");
    return result;
}

void FS::FileSystemPosix::OSFile::OSStream::Write(const void *src, size_t size)
{
    if( 1 != fwrite(src, size, 1, _file->_file.get()) )
    {
        throw std::runtime_error("file could not be written");
    }
}

void FS::FileSystemPosix::OSFile::OSStream::Seek(long long amount, unsigned int origin)
{
    fseek(_file->_file.get(), amount, origin);
}

long long FS::FileSystemPosix::OSFile::OSStream::Tell() const
{
    return ftell(_file->_file.get());
}

///////////////////////////////////////////////////////////////////////////////

FS::FileSystemPosix::OSFile::OSMemMap::OSMemMap(std::shared_ptr<OSFile> parent)
    : _file(parent)
{
    if( fseek(_file->_file.get(), 0, SEEK_END) )
        throw std::runtime_error("get file size");
    long int size = ftell(_file->_file.get());
    rewind(_file->_file.get());
    if( size < 0 )
        throw std::runtime_error("get file size");
    if( size > 0 )
    {
        _data.resize(size);
        if( 1 != fread(&_data[0], size, 1, _file->_file.get()) )
            throw std::runtime_error("read file");
    }
}

FS::FileSystemPosix::OSFile::OSMemMap::~OSMemMap()
{
    if (!_data.empty())
    {
        fseek(_file->_file.get(), 0, SEEK_SET);
        fwrite(&_data[0], _data.size(), 1, _file->_file.get());
    }
    _file->Unmap();
}

const void* FS::FileSystemPosix::OSFile::OSMemMap::GetData() const
{
    return _data.empty() ? nullptr : _data.data();
}

unsigned long FS::FileSystemPosix::OSFile::OSMemMap::GetSize() const
{
    return _data.size();
}

void FS::FileSystemPosix::OSFile::OSMemMap::SetSize(unsigned long size)
{
    _data.resize(size);
}

///////////////////////////////////////////////////////////////////////////////

FS::FileSystemPosix::FileSystemPosix(std::string rootDirectory)
	: _rootDirectory(std::move(rootDirectory))
{
	struct stat sb;
	if( stat(_rootDirectory.c_str(), &sb) )
		throw std::runtime_error(_rootDirectory + ": " + strerror(errno));

	if (!S_ISDIR(sb.st_mode))
		std::runtime_error(_rootDirectory + ": Not a directory");
}

std::vector<std::string> FS::FileSystemPosix::EnumAllFiles(std::string_view mask)
{
	std::vector<std::string> files;
    if( DIR *dir = opendir(_rootDirectory.c_str()) )
    {
        try
        {
            while( const dirent *e = readdir(dir) )
            {
				if( (DT_REG == e->d_type || DT_LNK == e->d_type) && !fnmatch(std::string(mask).c_str(), e->d_name, 0) )
                {
                    files.push_back(e->d_name);
                }
            }
        }
        catch(const std::exception&)
        {
            closedir(dir);
            throw;
        }
        closedir(dir);
    }
    else
    {
        throw std::runtime_error("open directory");
    }
	return files;
}

static std::string PathCombine(std::string_view first, std::string_view second)
{
	std::string result;
	result.reserve(first.size() + second.size() + 1);
	result.append(first).append(1, '/').append(second);
	return result;
}

std::shared_ptr<FS::File> FS::FileSystemPosix::RawOpen(std::string_view fileName, FileMode mode, bool nothrow)
{
    auto fullPath = PathCombine(_rootDirectory, fileName);

    struct stat sb;
    bool hasStat = !stat(fullPath.c_str(), &sb);
    if( !hasStat && !(mode & ModeWrite) )
    {
        if (nothrow)
            return nullptr;
        else
            throw std::runtime_error(fullPath + ": " + strerror(errno));
    }

    if (hasStat && S_ISDIR(sb.st_mode))
    {
        if (nothrow)
            return nullptr;
        else
            throw std::runtime_error(fullPath + ": Is a directory");
    }

    int nMode = ((mode & ModeWrite) ? 1:0) + ((mode & ModeRead) ? 2:0);
    assert(nMode);
    constexpr const char *modes[] = {"", "wb", "rb", "rb+"};

    StdioFile file(fopen(fullPath.c_str(), modes[nMode]));
    if( !file )
    {
        if (nothrow)
            return nullptr;
        else
            throw std::runtime_error(fullPath + ": " + strerror(errno));
    }

    return std::make_shared<OSFile>(std::move(file));
}

std::shared_ptr<FS::FileSystem> FS::FileSystemPosix::GetFileSystem(std::string_view path, bool create, bool nothrow)
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

    struct stat s;
    if( stat(tmpDir.c_str(), &s) )
    {
        if( create )
        {
            if( mkdir(tmpDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) )
            {
                if( nothrow )
                    return nullptr;
                else
					throw std::runtime_error("Could not create directory: " + tmpDir + ": " + strerror(errno));
            }
        }
        else
        {
            if( nothrow )
                return nullptr;
            else
				throw std::runtime_error(tmpDir + ": Path not found");
        }
    }
    else if( !S_ISDIR(s.st_mode) )
    {
        if( nothrow )
            return nullptr;
        else
			throw std::runtime_error(tmpDir + ": Not a directory");
    }

    // at this point the directory was either found or created
	auto child = std::make_shared<FileSystemPosix>(PathCombine(_rootDirectory, dirName));
    Mount(dirName, child);
    if( std::string::npos != p )
        return child->GetFileSystem(path.substr(p), create, nothrow); // process the rest of the path
    return child; // last path node was processed
}
