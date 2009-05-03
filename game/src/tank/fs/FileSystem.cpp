// FileSystem.cpp

#include "stdafx.h"
#include "FileSystem.h"

namespace FS {

///////////////////////////////////////////////////////////////////////////////
// IFile implementation

File::File(/*FileSystem* host*/)
//  : _hostFileSystem(host)
{
//    assert(_hostFileSystem);
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
	assert(_parent->GetFileSystem(GetNodeName(), false) == this);

	AddRef(); // protect this object from deletion
	_parent->_children.erase(GetNodeName());
	_parent = NULL;
	Release();
}

bool FileSystem::IsValid() const
{
	return _parent ? _parent->IsValid() : true;
}

SafePtr<File> FileSystem::Open(const string_t &fileName)
{
//	if( fileName.empty() ) return NULL;

	string_t::size_type pd = fileName.rfind(DELIMITER);
	if( string_t::npos != pd ) // is a path delimiter found?
	{
		return GetFileSystem(fileName.substr(0, pd+1), false)->RawOpen(fileName.substr(pd+1));
	}
	return RawOpen(fileName);
}

void FileSystem::EnumAllFiles(std::set<string_t> &files, const string_t &mask)
{
	// base file system can't contain any files
}

SafePtr<File> FileSystem::RawOpen(const string_t &fileName)
{
	throw std::runtime_error("Base file system can't contain any files");
	return NULL;
}

SafePtr<FileSystem> FileSystem::GetFileSystem(const string_t &path, bool create)
{
	assert(!path.empty());

//    if( path[0] == DELIMITER ) //

	string_t::size_type offset = (string_t::size_type) (path[0] == DELIMITER);

	if( path.length() == offset )
		return WrapRawPtr(this); // path contains only one symbol '/'

	string_t::size_type p = path.find(DELIMITER, offset);

	StrToFileSystemMap::const_iterator it = _children.find(
		path.substr(offset, string_t::npos != p ? p - offset : p));
	if( _children.end() == it )
		throw std::runtime_error("node not found in base file system");

	if( string_t::npos != p )
		return it->second->GetFileSystem(path.substr(p), create);

	return it->second;
}

///////////////////////////////////////////////////////////////////////////////

static string_t GetLastErrorMessage()
{
	LPVOID lpMsgBuf = NULL;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, GetLastError(), 0, (LPTSTR) &lpMsgBuf, 0, NULL );
	string_t result((LPCTSTR) lpMsgBuf);
	LocalFree(lpMsgBuf);
	return result;
}

OSFileSystem::OSFile::OSFile(const string_t &fileName)
  : _data(NULL)
  , _size(0)
{
	// replace all '/' by '\'
	string_t tmp = fileName;
	for( string_t::iterator it = tmp.begin(); tmp.end() != it; ++it )
	{
		if( DELIMITER == *it )
		{
			*it = TEXT('\\');
		}
	}

	_file.h = ::CreateFile(tmp.c_str(), // lpFileName
	    FILE_READ_DATA,                 // dwDesiredAccess
	    FILE_SHARE_READ,                // dwShareMode
	    NULL,                           // lpSecurityAttributes
	    OPEN_EXISTING,                  // dwCreationDisposition
	    FILE_FLAG_NO_BUFFERING,         // dwFlagsAndAttributes
	    NULL);                          // hTemplateFile

	if( INVALID_HANDLE_VALUE == _file.h )
	{
		throw std::runtime_error(GetLastErrorMessage());
	}

	_size = GetFileSize(_file.h, NULL);
	if( INVALID_FILE_SIZE == _size )
	{
		throw std::runtime_error(GetLastErrorMessage());
	}

	_map.h = CreateFileMapping(_file.h, NULL, PAGE_READONLY, 0, 0, NULL);
	if( NULL == _map.h )
	{
		throw std::runtime_error(GetLastErrorMessage());
	}

	_data = MapViewOfFile(_map.h, FILE_MAP_READ, 0, 0, 0);
	if( NULL == _data )
	{
		throw std::runtime_error(GetLastErrorMessage());
	}
}

OSFileSystem::OSFile::~OSFile()
{
	if( _data )
	{
		UnmapViewOfFile(_data);
	}
}

char* OSFileSystem::OSFile::GetData()
{
	return (char *) _data;
}

unsigned long OSFileSystem::OSFile::GetSize() const
{
	return _size;
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
	TCHAR *curDir = new TCHAR[len];
	GetCurrentDirectory(len, curDir);

	if( SetCurrentDirectory(rootDirectory.c_str()) )
	{
		DWORD tmpLen = GetCurrentDirectory(0, NULL);
		TCHAR *tmp = new TCHAR[tmpLen];
		GetCurrentDirectory(tmpLen, tmp);
		_rootDirectory = tmp;
		delete[] tmp;

		SetCurrentDirectory(curDir);
	}
	else
	{
		// error: nodeName doesn't exists or something nasty happened
	}

	delete[] curDir;
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
		throw std::runtime_error(GetLastErrorMessage());
	}

	WIN32_FIND_DATA fd;
	HANDLE hSearch = FindFirstFile(mask.c_str(), &fd);
	if( INVALID_HANDLE_VALUE == hSearch )
	{
		if( ERROR_FILE_NOT_FOUND == GetLastError() )
		{
			return; // nothing matches
		}
		throw std::runtime_error(GetLastErrorMessage());
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
		throw std::runtime_error(GetLastErrorMessage());
	}
}

SafePtr<File> OSFileSystem::RawOpen(const string_t &fileName)
{
	// combine with the root path
	return WrapRawPtr(new OSFile(_rootDirectory + TEXT('\\') + fileName));
}

SafePtr<FileSystem> OSFileSystem::GetFileSystem(const string_t &path, bool create)
{
	try
	{
		return __super::GetFileSystem(path, create);
	}
	catch( const std::exception& )
	{
		assert(!path.empty());

		string_t::size_type offset = (string_t::size_type) (path[0] == DELIMITER);
		if( path.length() == offset )
			return WrapRawPtr(this); // path contains only one DELIMITER symbol

		string_t::size_type p = path.find( DELIMITER, offset );
		string_t dirName = path.substr(offset, string_t::npos != p ? p - offset : p);
		string_t tmpDir = (_rootDirectory + TEXT('\\') + dirName);

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
					throw std::runtime_error("could not create directory");
				}
			}
			else
			{
				throw std::runtime_error("could not create or find directory");
			}
		}

		if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			SafePtr<FileSystem> child(new OSFileSystem(this, dirName));
			if( string_t::npos != p )
				return child->GetFileSystem(path.substr(p), create); // process the rest of the path
			return child; // last path node was processed
		}

		throw;
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace FS
///////////////////////////////////////////////////////////////////////////////
// end of file

