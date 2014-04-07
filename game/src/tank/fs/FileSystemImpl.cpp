// FileSystemImpl.cpp

#include "FileSystemImpl.h"
#include "functions.h"

#include <vector>


static std::string StrFromErr(DWORD dwMessageId)
{
	LPVOID lpMsgBuf = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, dwMessageId, 0, (LPTSTR)&lpMsgBuf, 0, NULL);
	std::string result((LPCTSTR)lpMsgBuf);
	LocalFree(lpMsgBuf);
	return result;
}

///////////////////////////////////////////////////////////////////////////////

SafePtr<FS::FileSystem> FS::FileSystem::Create(const std::string &nodeName)
{
	return new FileSystem(nodeName);
}

FS::FileSystem::FileSystem(const std::string &nodeName)
  : _nodeName(nodeName)
  , _parent(NULL)
{
}

FS::FileSystem::~FileSystem(void)
{
	// unmount all children
	while( !_children.empty() )
		_children.begin()->second->Unmount();
}

const std::string FS::FileSystem::GetFullPath(void) const
{
	std::string fullPath = GetNodeName();
	for( const FileSystem *fs = GetParent(); fs; fs = fs->GetParent() )
	{
		fullPath = fs->GetNodeName() + '/' + fullPath;
	}
	return fullPath;
}

bool FS::FileSystem::MountTo(FileSystem *parent)
{
	assert(!GetNodeName().empty() && GetNodeName() != "/");
	assert(!_parent); // may be is already mounted somewhere? only one parent is allowed

	// check if node with the same name is already exists
	StrToFileSystemMap::iterator it = parent->_children.find(_nodeName);
	if( parent->_children.end() != it )
		return false; // node already exists

	// mount
	_parent = parent;
	_parent->_children[_nodeName] = this;

	return true;
}

void FS::FileSystem::Unmount()
{
	assert(_parent); // not mounted?
	assert(_parent->GetFileSystem(GetNodeName(), false, true) == this);

	AddRef(); // protect this object from deletion
	_parent->_children.erase(GetNodeName());
	_parent = NULL;
	Release();
}

bool FS::FileSystem::IsValid() const
{
	return _parent ? _parent->IsValid() : true;
}

SafePtr<FS::File> FS::FileSystem::Open(const std::string &fileName, FileMode mode)
{
	std::string::size_type pd = fileName.rfind('/');
	if( std::string::npos != pd ) // was a path delimiter found?
	{
		return GetFileSystem(fileName.substr(0, pd + 1))->RawOpen(fileName.substr(pd + 1), mode);
	}
	return RawOpen(fileName, mode);
}

void FS::FileSystem::EnumAllFiles(std::set<std::string> &files, const std::string &mask)
{
	// base file system can't contain any files
}

SafePtr<FS::File> FS::FileSystem::RawOpen(const std::string &fileName, FileMode mode)
{
	throw std::runtime_error("Base file system can't contain any files");
	return NULL;
}

SafePtr<FS::FileSystem> FS::FileSystem::GetFileSystem(const std::string &path, bool create, bool nothrow)
{
	assert(!path.empty());

	// skip delimiters at the beginning
	std::string::size_type offset = 0;
	while( offset < path.length() && path[offset] == '/' )
		++offset;

	if( path.length() == offset )
		return this; // path consists of delimiters only

	std::string::size_type p = path.find('/', offset);

	StrToFileSystemMap::const_iterator it = _children.find(
		path.substr(offset, std::string::npos != p ? p - offset : p));
	if( _children.end() == it )
	{
		if( nothrow )
			return NULL;
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

FS::OSFileSystem::OSFile::OSFile(const std::string &fileName, FileMode mode)
  : _mode(mode)
  , _mapped(false)
  , _streamed(false)
{
	assert(_mode);

	// replace all '/' by '\'
	std::string tmp = fileName;
	for( std::string::iterator it = tmp.begin(); tmp.end() != it; ++it )
	{
		if( '/' == *it )
		{
			*it = '\\';
		}
	}

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

	_file.h = ::CreateFile(tmp.c_str(), // lpFileName
	    dwDesiredAccess,
	    dwShareMode,
	    NULL,                           // lpSecurityAttributes
	    dwCreationDisposition,
	    FILE_FLAG_SEQUENTIAL_SCAN,      // dwFlagsAndAttributes
	    NULL);                          // hTemplateFile

	if( INVALID_HANDLE_VALUE == _file.h )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}
}

FS::OSFileSystem::OSFile::~OSFile()
{
}

SafePtr<FS::MemMap> FS::OSFileSystem::OSFile::QueryMap()
{
	assert(!_mapped && !_streamed);
	SafePtr<MemMap> result(new OSMemMap(this, _file.h));
	_mapped = true;
	return result;
}

SafePtr<FS::Stream> FS::OSFileSystem::OSFile::QueryStream()
{
	assert(!_mapped && !_streamed);
	_streamed = true;
	SafePtr<Stream> result(new OSStream(this, _file.h));
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

FS::OSFileSystem::OSFile::OSStream::OSStream(const SafePtr<OSFile> &parent, HANDLE hFile)
  : _file(parent)
  , _hFile(hFile)
{
	Seek(0, SEEK_SET);
}

FS::OSFileSystem::OSFile::OSStream::~OSStream()
{
    _file->Unstream();
}
    
FS::ErrorCode FS::OSFileSystem::OSFile::OSStream::Read(void *dst, size_t size)
{
	DWORD bytesRead;
	if( !ReadFile(_hFile, dst, size, &bytesRead, NULL) )
	{
		return EC_ERROR;
	}
	return bytesRead == size ? EC_OK : EC_EOF;
}

void FS::OSFileSystem::OSFile::OSStream::Write(const void *src, size_t size)
{
	DWORD written;
	BOOL result = WriteFile(_hFile, src, size, &written, NULL);
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

///////////////////////////////////////////////////////////////////////////////

FS::OSFileSystem::OSFile::OSMemMap::OSMemMap(const SafePtr<OSFile> &parent, HANDLE hFile)
  : _file(parent)
  , _hFile(hFile)
  , _data(NULL)
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
	_size = GetFileSize(_hFile, NULL);
	if( INVALID_FILE_SIZE == _size )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}

	_map.h = CreateFileMapping(_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if( NULL == _map.h )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}

	_data = MapViewOfFile(_map.h, FILE_MAP_READ, 0, 0, 0);
	if( NULL == _data )
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
	_data = NULL;
	_size = 0;
	if( !bUnmapped )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}

	CloseHandle(_map.h);

	if( INVALID_SET_FILE_POINTER == SetFilePointer(_hFile, size, NULL, FILE_BEGIN) )
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

SafePtr<FS::OSFileSystem> FS::OSFileSystem::Create(const std::string &rootDirectory, const std::string &nodeName)
{
	return new OSFileSystem(rootDirectory, nodeName);
}

FS::OSFileSystem::OSFileSystem(const std::string &rootDirectory, const std::string &nodeName)
  : FileSystem(nodeName)
{
	// remember current directory to restore it later
	DWORD len = GetCurrentDirectory(0, NULL);
	std::vector<char> curDir(len);
	GetCurrentDirectory(len, &curDir[0]);

	if( SetCurrentDirectory(rootDirectory.c_str()) )
	{
		DWORD tmpLen = GetCurrentDirectory(0, NULL);
		std::vector<char> tmp(tmpLen);
		GetCurrentDirectory(tmpLen, &tmp[0]);
		_rootDirectory = &tmp[0];

		// restore last current directory
		if( !SetCurrentDirectory(&curDir[0]) )
		{
			throw std::runtime_error(StrFromErr(GetLastError()));
		}
	}
	else
	{
		// error: nodeName doesn't exists or something nasty happened
	}
}

FS::OSFileSystem::OSFileSystem(OSFileSystem *parent, const std::string &nodeName)
  : FileSystem(nodeName)
{
	assert(parent);
	assert(std::string::npos == nodeName.find('/'));

	MountTo(parent);
	_rootDirectory = parent->_rootDirectory + TEXT('\\') + nodeName;
}

FS::OSFileSystem::~OSFileSystem(void)
{
}

bool FS::OSFileSystem::IsValid() const
{
	return true;
}

void FS::OSFileSystem::EnumAllFiles(std::set<std::string> &files, const std::string &mask)
{
	// remember current directory to restore it later
	DWORD len = GetCurrentDirectory(0, NULL);
	std::vector<char> buf(len);
	GetCurrentDirectory(len, &buf[0]);

	if( !SetCurrentDirectory(_rootDirectory.c_str()) )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}

	WIN32_FIND_DATA fd;
	HANDLE hSearch = FindFirstFile(mask.c_str(), &fd);
	if( INVALID_HANDLE_VALUE == hSearch )
	{
		if( ERROR_FILE_NOT_FOUND == GetLastError() )
		{
			// restore last current directory
			if( !SetCurrentDirectory(&buf[0]) )
			{
				throw std::runtime_error(StrFromErr(GetLastError()));
			}
			return; // nothing matches
		}
		throw std::runtime_error(StrFromErr(GetLastError()));
	}

	files.clear();
	do
	{
		if( 0 == (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
		{
			files.insert( fd.cFileName );
		}
	}
	while( FindNextFile(hSearch, &fd) );
	FindClose(hSearch);

	// restore last current directory
	if( !SetCurrentDirectory(&buf[0]) )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}
}

SafePtr<FS::File> FS::OSFileSystem::RawOpen(const std::string &fileName, FileMode mode)
{
	// combine with the root path
	return new OSFile(_rootDirectory + TEXT('\\') + fileName, mode);
}

SafePtr<FS::FileSystem> FS::OSFileSystem::GetFileSystem(const std::string &path, bool create, bool nothrow)
{
	if (SafePtr<FileSystem> tmp = FileSystem::GetFileSystem(path, create, true))
	{
		return tmp;
	}

	assert(!path.empty());

	// skip delimiters at the beginning
	std::string::size_type offset = 0;
	while( offset < path.length() && path[offset] == '/' )
		++offset;
	assert(path.length() > offset);

	std::string::size_type p = path.find('/', offset);
	std::string dirName = path.substr(offset, std::string::npos != p ? p - offset : p);
	std::string tmpDir = _rootDirectory + TEXT('\\') + dirName;

	// try to find directory
	WIN32_FIND_DATA fd = {0};
	HANDLE search = FindFirstFile(tmpDir.c_str(), &fd);
	FindClose(search);

	if( INVALID_HANDLE_VALUE == search )
	{
		if( create && CreateDirectory(tmpDir.c_str(), NULL) )
		{
			// try to find again
			HANDLE search = FindFirstFile(tmpDir.c_str(), &fd);
			FindClose(search);
			if( INVALID_HANDLE_VALUE == search )
			{
				if( nothrow )
					return NULL;
				else
					throw std::runtime_error("could not create directory");
			}
		}
		else
		{
			if( nothrow )
				return NULL;
			else
				throw std::runtime_error("could not create or find directory");
		}
	}

	if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
	{
		SafePtr<FileSystem> child(new OSFileSystem(this, dirName));
		if( std::string::npos != p )
			return child->GetFileSystem(path.substr(p), create, nothrow); // process the rest of the path
		return child; // last path node was processed
	}

	if( !nothrow )
		throw std::runtime_error("object is not a directory");
	return NULL;
}

// ----------------------------------------------------------------
#else // POSIX
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/stat.h>

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
        throw std::runtime_error("open file");
}

FS::OSFileSystem::OSFile::~OSFile()
{
}

SafePtr<FS::MemMap> FS::OSFileSystem::OSFile::QueryMap()
{
    assert(!_mapped && !_streamed);
    SafePtr<MemMap> result(new OSMemMap(this));
    _mapped = true;
    return result;
}

SafePtr<FS::Stream> FS::OSFileSystem::OSFile::QueryStream()
{
    assert(!_mapped && !_streamed);
    _streamed = true;
    return SafePtr<Stream>(new OSStream(this));
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

FS::OSFileSystem::OSFile::OSStream::OSStream(const SafePtr<OSFile> &parent)
    : _file(parent)
{
    Seek(0, SEEK_SET);
}

FS::OSFileSystem::OSFile::OSStream::~OSStream()
{
    _file->Unstream();
}

FS::ErrorCode FS::OSFileSystem::OSFile::OSStream::Read(void *dst, size_t size)
{
    if( 1 == fread(dst, size, 1, _file->_file.f) )
        return EC_OK;
    return feof(_file->_file.f) ? EC_EOF : EC_ERROR;
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

///////////////////////////////////////////////////////////////////////////////

FS::OSFileSystem::OSFile::OSMemMap::OSMemMap(const SafePtr<OSFile> &parent)
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
    fseek(_file->_file.f, 0, SEEK_SET);
    fwrite(&_data[0], _data.size(), 1, _file->_file.f);
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

SafePtr<FS::OSFileSystem> FS::OSFileSystem::Create(const std::string &rootDirectory, const std::string &nodeName)
{
    return new OSFileSystem(rootDirectory, nodeName);
}

FS::OSFileSystem::OSFileSystem(const std::string &rootDirectory, const std::string &nodeName)
    : FileSystem(nodeName)
    , _rootDirectory(rootDirectory)
{
}

FS::OSFileSystem::OSFileSystem(OSFileSystem *parent, const std::string &nodeName)
    : FileSystem(nodeName)
{
    assert(parent);
    assert(std::string::npos == nodeName.find('/'));
    
    MountTo(parent);
    _rootDirectory = parent->_rootDirectory + '/' + nodeName;
}

FS::OSFileSystem::~OSFileSystem(void)
{
}

bool FS::OSFileSystem::IsValid() const
{
    return true;
}

void FS::OSFileSystem::EnumAllFiles(std::set<std::string> &files, const std::string &mask)
{
    if( DIR *dir = opendir(_rootDirectory.c_str()) )
    {
        try
        {
            files.clear();
            while( const dirent *e = readdir(dir) )
            {
                if( (DT_REG == e->d_type || DT_LNK == e->d_type) && !fnmatch(mask.c_str(), e->d_name, 0) )
                {
                    files.insert(e->d_name);
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
}

SafePtr<FS::File> FS::OSFileSystem::RawOpen(const std::string &fileName, FileMode mode)
{
    return new OSFile(_rootDirectory + '/' + fileName, mode);
}

SafePtr<FS::FileSystem> FS::OSFileSystem::GetFileSystem(const std::string &path, bool create, bool nothrow)
{
    if( SafePtr<FileSystem> tmp = FileSystem::GetFileSystem(path, create, true) )
    {
        return tmp;
    }
    
    assert(!path.empty());
    
    // skip delimiters at the beginning
    std::string::size_type offset = 0;
    while( offset < path.length() && path[offset] == '/' )
        ++offset;
    assert(path.length() > offset);
    
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
                    return NULL;
                else
                    throw std::runtime_error("could not create directory");
            }
        }
        else
        {
            if( nothrow )
                return NULL;
            else
                throw std::runtime_error("directory not found");
        }
    }
    else if( !S_ISDIR(s.st_mode) )
    {
        if( nothrow )
            return NULL;
        else
            throw std::runtime_error("not a directory");
    }
    
    // at this point the directory was either found or created
    SafePtr<FileSystem> child(new OSFileSystem(this, dirName));
    if( std::string::npos != p )
        return child->GetFileSystem(path.substr(p), create, nothrow); // process the rest of the path
    return child; // last path node was processed
}

#endif // _WIN32

// end of file
