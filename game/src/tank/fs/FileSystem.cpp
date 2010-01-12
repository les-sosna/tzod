// FileSystem.cpp

#include "stdafx.h"
#include "FileSystem.h"
#include "functions.h"

namespace FS {

///////////////////////////////////////////////////////////////////////////////

MemMap::MemMap(const SafePtr<File> &parent)
  : _file(parent)
{
}

MemMap::~MemMap()
{
	_file->Unmap();
}

///////////////////////////////////////////////////////////////////////////////

Stream::Stream(const SafePtr<File> &parent)
  : _file(parent)
{
}

Stream::~Stream()
{
	_file->Unstream();
}

///////////////////////////////////////////////////////////////////////////////
// IFile implementation

File::File()
{
}

File::~File()
{
}

///////////////////////////////////////////////////////////////////////////////

SafePtr<FileSystem> FileSystem::Create(const string_t &nodeName)
{
	return WrapRawPtr(new FileSystem(nodeName));
}

FileSystem::FileSystem(const string_t &nodeName)
  : _nodeName(nodeName)
  , _parent(NULL)
{
}

FileSystem::~FileSystem(void)
{
	// unmount all children
	while( !_children.empty() )
		_children.begin()->second->Unmount();
}

const string_t FileSystem::GetFullPath(void) const
{
	string_t fullPath = GetNodeName();
	for( const FileSystem *fs = GetParent(); fs; fs = fs->GetParent() )
	{
		fullPath = fs->GetNodeName() + DELIMITER + fullPath;
	}
	return fullPath;
}

bool FileSystem::MountTo(FileSystem *parent)
{
	assert(!GetNodeName().empty() && GetNodeName() != TEXT("/"));
	assert(!_parent); // may be is already mounted somewhere? only one parent is allowed

	// check if node with the same name is already exists
	StrToFileSystemMap::iterator it = parent->_children.find(_nodeName);
	if( parent->_children.end() != it )
		return false; // node already exists

	// mount
	_parent = parent;
	_parent->_children[_nodeName] = WrapRawPtr(this);

	return true;
}

void FileSystem::Unmount()
{
	assert(_parent); // not mounted?
	assert(_parent->GetFileSystem(GetNodeName(), false, true) == this);

	AddRef(); // protect this object from deletion
	_parent->_children.erase(GetNodeName());
	_parent = NULL;
	Release();
}

bool FileSystem::IsValid() const
{
	return _parent ? _parent->IsValid() : true;
}

SafePtr<File> FileSystem::Open(const string_t &fileName, FileMode mode)
{
	string_t::size_type pd = fileName.rfind(DELIMITER);
	if( string_t::npos != pd ) // was a path delimiter found?
	{
		return GetFileSystem(fileName.substr(0, pd + 1))->RawOpen(fileName.substr(pd + 1), mode);
	}
	return RawOpen(fileName, mode);
}

void FileSystem::EnumAllFiles(std::set<string_t> &files, const string_t &mask)
{
	// base file system can't contain any files
}

SafePtr<File> FileSystem::RawOpen(const string_t &fileName, FileMode mode)
{
	throw std::runtime_error("Base file system can't contain any files");
	return NULL;
}

SafePtr<FileSystem> FileSystem::GetFileSystem(const string_t &path, bool create, bool nothrow)
{
	assert(!path.empty());

	// skip delimiters at the beginning
	string_t::size_type offset = 0;
	while( offset < path.length() && path[offset] == DELIMITER )
		++offset;

	if( path.length() == offset )
		return WrapRawPtr(this); // path consists of delimiters only

	string_t::size_type p = path.find(DELIMITER, offset);

	StrToFileSystemMap::const_iterator it = _children.find(
		path.substr(offset, string_t::npos != p ? p - offset : p));
	if( _children.end() == it )
	{
		if( nothrow )
			return NULL;
		else
			throw std::runtime_error("node not found in base file system");
	}

	if( string_t::npos != p )
		return it->second->GetFileSystem(path.substr(p), create, nothrow);

	return it->second;
}

///////////////////////////////////////////////////////////////////////////////

OSFileSystem::OSFile::OSFile(const string_t &fileName, FileMode mode)
  : _mode(mode)
  , _mapped(false)
  , _streamed(false)
{
	assert(_mode);

	// replace all '/' by '\'
	string_t tmp = fileName;
	for( string_t::iterator it = tmp.begin(); tmp.end() != it; ++it )
	{
		if( DELIMITER == *it )
		{
			*it = TEXT('\\');
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

OSFileSystem::OSFile::~OSFile()
{
}

SafePtr<MemMap> OSFileSystem::OSFile::QueryMap()
{
	assert(!_mapped && !_streamed);
	SafePtr<MemMap> result(new OSMemMap(WrapRawPtr(this), _file.h));
	_mapped = true;
	return result;
}

SafePtr<Stream> OSFileSystem::OSFile::QueryStream()
{
	assert(!_mapped && !_streamed);
	_streamed = true;
	SafePtr<Stream> result(new OSStream(WrapRawPtr(this), _file.h));
	return result;
}

void OSFileSystem::OSFile::Unmap()
{
	assert(_mapped && !_streamed);
	_mapped = false;
}

void OSFileSystem::OSFile::Unstream()
{
	assert(_streamed && !_mapped);
	_streamed = false;
}

///////////////////////////////////////////////////////////////////////////////

OSFileSystem::OSFile::OSStream::OSStream(const SafePtr<File> &parent, HANDLE hFile)
  : Stream(parent)
  , _hFile(hFile)
{
	Seek(0, SEEK_SET);
}

bool OSFileSystem::OSFile::OSStream::IsEof()
{
	unsigned long position = SetFilePointer(_hFile, 0, NULL, FILE_CURRENT);
	unsigned long size = GetFileSize(_hFile, NULL);
	if( INVALID_FILE_SIZE == position || INVALID_FILE_SIZE == size )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}
	return position >= size;
}

unsigned long OSFileSystem::OSFile::OSStream::Read(void *dst, unsigned long blockSize, unsigned long numBlocks)
{
	DWORD bytesRead;
	if( !ReadFile(_hFile, dst, blockSize*numBlocks, &bytesRead, NULL) )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}
	if( bytesRead % blockSize )
	{
		throw std::runtime_error(StrFromErr(ERROR_HANDLE_EOF));
	}
	return bytesRead / blockSize;
}

void OSFileSystem::OSFile::OSStream::Write(const void *src, unsigned long byteCount)
{
	DWORD written;
	BOOL result = WriteFile(_hFile, src, byteCount, &written, NULL);
	if( !result || written != byteCount )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}
}

unsigned long long OSFileSystem::OSFile::OSStream::Seek(long long amount, unsigned int origin)
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
	return result.QuadPart;
}

unsigned long long OSFileSystem::OSFile::OSStream::GetSize()
{
	LARGE_INTEGER result;
	if( !GetFileSizeEx(_hFile, &result) )
	{
		throw std::runtime_error(StrFromErr(GetLastError()));
	}
	return result.QuadPart;
}

///////////////////////////////////////////////////////////////////////////////

OSFileSystem::OSFile::OSMemMap::OSMemMap(const SafePtr<File> &parent, HANDLE hFile)
  : MemMap(parent)
  , _hFile(hFile)
  , _data(NULL)
  , _size(0)
{
	SetupMapping();
}

OSFileSystem::OSFile::OSMemMap::~OSMemMap()
{
	if( _data )
	{
		UnmapViewOfFile(_data);
	}
}

void OSFileSystem::OSFile::OSMemMap::SetupMapping()
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

char* OSFileSystem::OSFile::OSMemMap::GetData()
{
	return (char *) _data;
}

unsigned long OSFileSystem::OSFile::OSMemMap::GetSize() const
{
	return _size;
}

void OSFileSystem::OSFile::OSMemMap::SetSize(unsigned long size)
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

SafePtr<OSFileSystem> OSFileSystem::Create(const string_t &rootDirectory, const string_t &nodeName)
{
	return WrapRawPtr(new OSFileSystem(rootDirectory, nodeName));
}

OSFileSystem::OSFileSystem(const string_t &rootDirectory, const string_t &nodeName)
  : FileSystem(nodeName)
{
	// remember current directory to restore it later
	DWORD len = GetCurrentDirectory(0, NULL);
	std::vector<TCHAR> curDir(len);
	GetCurrentDirectory(len, &curDir[0]);

	if( SetCurrentDirectory(rootDirectory.c_str()) )
	{
		DWORD tmpLen = GetCurrentDirectory(0, NULL);
		std::vector<TCHAR> tmp(tmpLen);
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

OSFileSystem::OSFileSystem(OSFileSystem *parent, const string_t &nodeName)
  : FileSystem(nodeName)
{
	assert(parent);
	assert(string_t::npos == nodeName.find(DELIMITER));

	MountTo(parent);
	_rootDirectory = parent->_rootDirectory + TEXT('\\') + nodeName;
}

OSFileSystem::~OSFileSystem(void)
{
}

bool OSFileSystem::IsValid() const
{
	return true;
}

void OSFileSystem::EnumAllFiles(std::set<string_t> &files, const string_t &mask)
{
	// remember current directory to restore it later
	DWORD len = GetCurrentDirectory(0, NULL);
	std::vector<TCHAR> buf(len);
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

SafePtr<File> OSFileSystem::RawOpen(const string_t &fileName, FileMode mode)
{
	// combine with the root path
	return WrapRawPtr(new OSFile(_rootDirectory + TEXT('\\') + fileName, mode));
}

SafePtr<FileSystem> OSFileSystem::GetFileSystem(const string_t &path, bool create, bool nothrow)
{
	if( SafePtr<FileSystem> tmp = __super::GetFileSystem(path, create, true) )
	{
		return tmp;
	}

	assert(!path.empty());

	// skip delimiters at the beginning
	string_t::size_type offset = 0;
	while( offset < path.length() && path[offset] == DELIMITER )
		++offset;
	assert(path.length() > offset);

	string_t::size_type p = path.find(DELIMITER, offset);
	string_t dirName = path.substr(offset, string_t::npos != p ? p - offset : p);
	string_t tmpDir = _rootDirectory + TEXT('\\') + dirName;

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
		if( string_t::npos != p )
			return child->GetFileSystem(path.substr(p), create, nothrow); // process the rest of the path
		return child; // last path node was processed
	}

	if( !nothrow )
		throw std::runtime_error("object is not a directory");
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace FS
///////////////////////////////////////////////////////////////////////////////
// end of file

