// FileSystemImpl.cpp

#include "FileSystemImpl.h"

#include <cassert>

///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32

#include <Windows.h>
#include <utf8.h>
#include <algorithm>
#include <sstream>

static std::wstring s2w(const std::string s)
{
	std::wstring w;
	utf8::utf8to16(s.begin(), s.end(), std::back_inserter(w));
	return w;
}

static std::string w2s(const std::wstring w)
{
	std::string s;
	utf8::utf16to8(w.begin(), w.end(), std::back_inserter(s));
	return s;
}
static std::string StrFromErr(DWORD dwMessageId)
{
	LPWSTR msgBuf = nullptr;
	DWORD msgSize = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                   nullptr, dwMessageId, 0, (LPWSTR) &msgBuf, 0, nullptr);
	while (msgBuf && msgSize)
	{
		if (msgBuf[msgSize - 1] == L'\n' || msgBuf[msgSize - 1] == L'\r')
		{
			--msgSize;
		}
		else
		{
			break;
		}
	}

	if (msgBuf)
	{
		std::string result;
		utf8::utf16to8(msgBuf, msgBuf + msgSize, std::back_inserter(result));
		LocalFree(msgBuf);
		return result;
	}
	else
	{
		std::ostringstream ss;
		ss << "Unknown error (" << dwMessageId << ")";
		return ss.str();
	}
}

FS::OSFileSystem::OSFile::OSFile(std::wstring &&fileName, FileMode mode)
  : _mode(mode)
  , _mapped(false)
  , _streamed(false)
{
	assert(_mode);

	std::replace(fileName.begin(), fileName.end(), L'/', L'\\');

	DWORD dwDesiredAccess = 0;
	DWORD dwShareMode = FILE_SHARE_READ;
	DWORD dwCreationDisposition;

	if( _mode & ModeWrite )
	{
		dwDesiredAccess |= FILE_WRITE_DATA;
		dwShareMode = 0;
		dwCreationDisposition = CREATE_ALWAYS;
	}

	if( _mode & ModeRead )
	{
		dwDesiredAccess |= FILE_READ_DATA;
		dwCreationDisposition = (_mode & ModeWrite) ? OPEN_ALWAYS : OPEN_EXISTING;
	}

	_file.h = ::CreateFileW(fileName.c_str(), // lpFileName
	    dwDesiredAccess,
	    dwShareMode,
	    nullptr,                           // lpSecurityAttributes
	    dwCreationDisposition,
	    FILE_FLAG_SEQUENTIAL_SCAN,      // dwFlagsAndAttributes
	    nullptr);                          // hTemplateFile

	if( INVALID_HANDLE_VALUE == _file.h )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}
}

FS::OSFileSystem::OSFile::~OSFile()
{
}

std::shared_ptr<FS::MemMap> FS::OSFileSystem::OSFile::QueryMap()
{
	assert(!_mapped && !_streamed);
	std::shared_ptr<MemMap> result = std::make_shared<OSMemMap>(shared_from_this(), _file.h);
	_mapped = true;
	return result;
}

std::shared_ptr<FS::Stream> FS::OSFileSystem::OSFile::QueryStream()
{
	assert(!_mapped && !_streamed);
	_streamed = true;
	std::shared_ptr<Stream> result = std::make_shared<OSStream>(shared_from_this(), _file.h);
	return result;
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

FS::OSFileSystem::OSFile::OSStream::OSStream(std::shared_ptr<OSFile> parent, HANDLE hFile)
  : _file(parent)
  , _hFile(hFile)
{
	Seek(0, SEEK_SET);
}

FS::OSFileSystem::OSFile::OSStream::~OSStream()
{
    _file->Unstream();
}

size_t FS::OSFileSystem::OSFile::OSStream::Read(void *dst, size_t size, size_t count)
{
	DWORD bytesRead;
	if( !ReadFile(_hFile, dst, size * count, &bytesRead, nullptr) )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}
	if( bytesRead % size )
	{
		throw std::runtime_error("unexpected end of file");
	}
	return bytesRead / size;
}

void FS::OSFileSystem::OSFile::OSStream::Write(const void *src, size_t size)
{
	DWORD written;
	BOOL result = WriteFile(_hFile, src, size, &written, nullptr);
	if( !result || written != size )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}
}

void FS::OSFileSystem::OSFile::OSStream::Seek(long long amount, unsigned int origin)
{
	DWORD dwMoveMethod;
	switch( origin )
	{
	case SEEK_SET: dwMoveMethod = FILE_BEGIN; break;
	case SEEK_CUR: dwMoveMethod = FILE_CURRENT; break;
	case SEEK_END: dwMoveMethod = FILE_END; break;
	default:
		assert(false);
	}
	LARGE_INTEGER result;
	LARGE_INTEGER liAmount;
	liAmount.QuadPart = amount;
	if( !SetFilePointerEx(_hFile, liAmount, &result, dwMoveMethod) )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}
}

long long FS::OSFileSystem::OSFile::OSStream::Tell() const
{
	LARGE_INTEGER zero = { 0 };
	LARGE_INTEGER current = { 0 };
	SetFilePointerEx(_hFile, zero, &current, FILE_CURRENT);
	return current.QuadPart;
}

///////////////////////////////////////////////////////////////////////////////

FS::OSFileSystem::OSFile::OSMemMap::OSMemMap(std::shared_ptr<OSFile> parent, HANDLE hFile)
  : _file(parent)
  , _hFile(hFile)
  , _data(nullptr)
  , _size(0)
{
	SetupMapping();
}

FS::OSFileSystem::OSFile::OSMemMap::~OSMemMap()
{
	if( _data )
	{
		UnmapViewOfFile(_data);
	}
	_file->Unmap();
}

void FS::OSFileSystem::OSFile::OSMemMap::SetupMapping()
{
	_size = GetFileSize(_hFile, nullptr);
	if( INVALID_FILE_SIZE == _size )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}

	_map.h = CreateFileMapping(_hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
	if( nullptr == _map.h )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}

	_data = MapViewOfFile(_map.h, FILE_MAP_READ, 0, 0, 0);
	if( nullptr == _data )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}
}

char* FS::OSFileSystem::OSFile::OSMemMap::GetData()
{
	return (char *) _data;
}

unsigned long FS::OSFileSystem::OSFile::OSMemMap::GetSize() const
{
	return _size;
}

void FS::OSFileSystem::OSFile::OSMemMap::SetSize(unsigned long size)
{
	BOOL bUnmapped = UnmapViewOfFile(_data);
	_data = nullptr;
	_size = 0;
	if( !bUnmapped )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}

	CloseHandle(_map.h);

	if( INVALID_SET_FILE_POINTER == SetFilePointer(_hFile, size, nullptr, FILE_BEGIN) )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}
	if( !SetEndOfFile(_hFile) )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}

	SetupMapping();
	assert(_size == size);
}

///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<FS::OSFileSystem> FS::OSFileSystem::Create(const std::string &rootDirectory)
{
	// convert to absolute path
	std::wstring tmpRel = s2w(rootDirectory);
	if (DWORD len = GetFullPathNameW(tmpRel.c_str(), 0, nullptr, nullptr))
	{
		std::wstring tmpFull(len, L'\0');
		if (DWORD len2 = GetFullPathNameW(tmpRel.c_str(), len, &tmpFull[0], nullptr))
		{
			tmpFull.resize(len2); // truncate terminating \0
			return std::make_shared<OSFileSystem>(std::move(tmpFull));
		}
	}
	throw std::runtime_error(StrFromErr(GetLastError()));
	return nullptr;
}

FS::OSFileSystem::OSFileSystem(std::wstring &&rootDirectory)
  : _rootDirectory(std::move(rootDirectory))
{
}

std::vector<std::string> FS::OSFileSystem::EnumAllFiles(const std::string &mask)
{
	// query = _rootDirectory + '\\' + mask
	std::wstring query = _rootDirectory + L'\\';
	utf8::utf8to16(mask.begin(), mask.end(), std::back_inserter(query));

	WIN32_FIND_DATAW fd;
	HANDLE hSearch = FindFirstFileW(query.c_str(), &fd);
	if( INVALID_HANDLE_VALUE == hSearch )
	{
		if( ERROR_FILE_NOT_FOUND == GetLastError() )
		{
			return std::vector<std::string>(); // nothing matches
		}
		throw std::runtime_error(StrFromErr(GetLastError()));
	}

	std::vector<std::string> files;
	do
	{
		if( 0 == (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
		{
			files.push_back(w2s(fd.cFileName));
		}
	}
	while( FindNextFileW(hSearch, &fd) );
	FindClose(hSearch);
	return files;
}

std::shared_ptr<FS::File> FS::OSFileSystem::RawOpen(const std::string &fileName, FileMode mode)
{
	// combine with the root path
	return std::make_shared<OSFile>(_rootDirectory + L'\\' + s2w(fileName), mode);
}

std::shared_ptr<FS::FileSystem> FS::OSFileSystem::GetFileSystem(const std::string &path, bool create, bool nothrow)
try
{
	if (std::shared_ptr<FileSystem> tmp = FileSystem::GetFileSystem(path, create, true))
	{
		return tmp;
	}

	assert(!path.empty());

	// skip delimiters at the beginning
	std::string::size_type offset = path.find_first_not_of('/');
	assert(std::string::npos != offset);

	std::string::size_type p = path.find('/', offset);
	std::string dirName = path.substr(offset, std::string::npos != p ? p - offset : p);

	// tmpDir = _rootDirectory + '\\' + dirName
	std::wstring tmpDir = _rootDirectory + L"\\";
	utf8::utf8to16(dirName.begin(), dirName.end(), std::back_inserter(tmpDir));

	// try to find directory
	WIN32_FIND_DATAW fd = {0};
	HANDLE search = FindFirstFileW(tmpDir.c_str(), &fd);

    if (INVALID_HANDLE_VALUE != search)
    {
        FindClose(search);
    }
    else
	{
		if( create )
		{
			if (!CreateDirectoryW(tmpDir.c_str(), nullptr))
			{
				// creation failed
				if( nothrow )
					return nullptr;
				else
					throw std::runtime_error(StrFromErr(GetLastError()));
			}
			else
			{
				// try searching again to get attributes
				HANDLE search2 = FindFirstFileW(tmpDir.c_str(), &fd);
				FindClose(search2);
				if (INVALID_HANDLE_VALUE == search2)
				{
					if (nothrow)
						return nullptr;
					else
						throw std::runtime_error(StrFromErr(GetLastError()));
				}
			}
		}
		else
		{
			// directory not found
			if( nothrow )
				return nullptr;
			else
				throw std::runtime_error(StrFromErr(GetLastError()));
		}
	}

	if( 0 == (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
		throw std::runtime_error("object is not a directory");

	std::shared_ptr<FileSystem> child = std::make_shared<OSFileSystem>(_rootDirectory + L'\\' + s2w(dirName));
	Mount(dirName, child);

	if( std::string::npos != p )
		return child->GetFileSystem(path.substr(p), create, nothrow); // process the rest of the path
	return child; // last path node was processed
}
catch (const std::exception&)
{
	std::ostringstream ss;
	ss << "Failed to open directory '" << path << "'";
	std::throw_with_nested(std::runtime_error(ss.str()));
}

// ----------------------------------------------------------------
#else // POSIX
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

std::shared_ptr<FS::OSFileSystem> FS::OSFileSystem::Create(const std::string &rootDirectory)
{
    return std::make_shared<OSFileSystem>(rootDirectory);
}
FS::OSFileSystem::OSFileSystem(const std::string &rootDirectory)
    : _rootDirectory(rootDirectory)
{
}

std::vector<std::string> FS::OSFileSystem::EnumAllFiles(const std::string &mask)
{
	std::vector<std::string> files;
    if( DIR *dir = opendir(_rootDirectory.c_str()) )
    {
        try
        {
            while( const dirent *e = readdir(dir) )
            {
                if( (DT_REG == e->d_type || DT_LNK == e->d_type) && !fnmatch(mask.c_str(), e->d_name, 0) )
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

#endif // _WIN32

// end of file
