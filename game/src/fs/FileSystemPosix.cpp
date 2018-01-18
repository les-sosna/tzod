#include "FileSystemPosix.h"
#include <cassert>
#include <cerrno>
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <string.h>

FS::OSFileSystem::OSFile::OSFile(const std::string &fileName, FileMode mode)
    : _mode(mode)
    , _mapped(false)
    , _streamed(false)
{
    int nMode = ((_mode & ModeWrite) ? 1:0) + ((_mode & ModeRead) ? 2:0);
    assert(nMode);
    static const char *modes[] = {"", "wb", "rb", "rb+"};
    _file.f = fopen(fileName.c_str(), modes[nMode]);
    if( !_file.f )
        throw std::runtime_error(std::string("could not open file: ") + strerror(errno));
}

FS::OSFileSystem::OSFile::~OSFile()
{
}

std::shared_ptr<FS::MemMap> FS::OSFileSystem::OSFile::QueryMap()
{
    assert(!_mapped && !_streamed);
	std::shared_ptr<MemMap> result = std::make_shared<OSMemMap>(shared_from_this());
    _mapped = true;
    return result;
}

std::shared_ptr<FS::Stream> FS::OSFileSystem::OSFile::QueryStream()
{
    assert(!_mapped && !_streamed);
    _streamed = true;
    return std::make_shared<OSStream>(shared_from_this());
}

void FS::OSFileSystem::OSFile::Unmap()
{
    assert(_mapped && !_streamed);
    _mapped = false;
}

void FS::OSFileSystem::OSFile::Unstream()
{
    assert(_streamed && !_mapped);
    _streamed = false;
}

///////////////////////////////////////////////////////////////////////////////

FS::OSFileSystem::OSFile::OSStream::OSStream(std::shared_ptr<OSFile> parent)
    : _file(parent)
{
    Seek(0, SEEK_SET);
}

FS::OSFileSystem::OSFile::OSStream::~OSStream()
{
    _file->Unstream();
}

size_t FS::OSFileSystem::OSFile::OSStream::Read(void *dst, size_t size, size_t count)
{
    size_t result = fread(dst, size, count, _file->_file.f);
    if( count != result && ferror(_file->_file.f) )
        throw std::runtime_error("read file");
    return result;
}

void FS::OSFileSystem::OSFile::OSStream::Write(const void *src, size_t size)
{
    if( 1 != fwrite(src, size, 1, _file->_file.f) )
    {
        throw std::runtime_error("file could not be written");
    }
}

void FS::OSFileSystem::OSFile::OSStream::Seek(long long amount, unsigned int origin)
{
    fseek(_file->_file.f, amount, origin);
}

long long FS::OSFileSystem::OSFile::OSStream::Tell() const
{
    return ftell(_file->_file.f);
}

///////////////////////////////////////////////////////////////////////////////

FS::OSFileSystem::OSFile::OSMemMap::OSMemMap(std::shared_ptr<OSFile> parent)
    : _file(parent)
{
    if( fseek(_file->_file.f, 0, SEEK_END) )
        throw std::runtime_error("get file size");
    long int size = ftell(_file->_file.f);
    rewind(_file->_file.f);
    if( size < 0 )
        throw std::runtime_error("get file size");
    if( size > 0 )
    {
        _data.resize(size);
        if( 1 != fread(&_data[0], size, 1, _file->_file.f) )
            throw std::runtime_error("read file");
    }
}

FS::OSFileSystem::OSFile::OSMemMap::~OSMemMap()
{
    if (!_data.empty())
    {
        fseek(_file->_file.f, 0, SEEK_SET);
        fwrite(&_data[0], _data.size(), 1, _file->_file.f);
    }
    _file->Unmap();
}

char* FS::OSFileSystem::OSFile::OSMemMap::GetData()
{
    return _data.empty() ? nullptr : &_data[0];
}

unsigned long FS::OSFileSystem::OSFile::OSMemMap::GetSize() const
{
    return _data.size();
}

void FS::OSFileSystem::OSFile::OSMemMap::SetSize(unsigned long size)
{
    _data.resize(size);
}

///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<FS::FileSystem> FS::CreateOSFileSystem(const std::string &rootDirectory)
{
    return std::make_shared<OSFileSystem>(rootDirectory);
}

FS::OSFileSystem::OSFileSystem(const std::string &rootDirectory)
    : _rootDirectory(rootDirectory)
{
}

std::vector<std::string> FS::OSFileSystem::EnumAllFiles(std::string_view mask)
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

std::shared_ptr<FS::File> FS::OSFileSystem::RawOpen(const std::string &fileName, FileMode mode)
{
    return std::make_shared<OSFile>(_rootDirectory + '/' + fileName, mode);
}

std::shared_ptr<FS::FileSystem> FS::OSFileSystem::GetFileSystem(const std::string &path, bool create, bool nothrow)
{
	if( std::shared_ptr<FileSystem> tmp = FileSystem::GetFileSystem(path, create, true) )
    {
        return tmp;
    }

    assert(!path.empty());

    // skip delimiters at the beginning
    std::string::size_type offset = path.find_first_not_of('/');
    assert(std::string::npos != offset);

    std::string::size_type p = path.find('/', offset);
    std::string dirName = path.substr(offset, std::string::npos != p ? p - offset : p);
    std::string tmpDir = _rootDirectory + '/' + dirName;

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
                    throw std::runtime_error("could not create directory");
            }
        }
        else
        {
            if( nothrow )
                return nullptr;
            else
                throw std::runtime_error(tmpDir + " - directory not found");
        }
    }
    else if( !S_ISDIR(s.st_mode) )
    {
        if( nothrow )
            return nullptr;
        else
            throw std::runtime_error("not a directory");
    }

    // at this point the directory was either found or created
	std::shared_ptr<FileSystem> child = std::make_shared<OSFileSystem>(_rootDirectory + '/' + dirName);
    Mount(dirName, child);
    if( std::string::npos != p )
        return child->GetFileSystem(path.substr(p), create, nothrow); // process the rest of the path
    return child; // last path node was processed
}
