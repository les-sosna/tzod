// FileSystem.cpp

#include "stdafx.h"
#include "FileSystem.h"

///////////////////////////////////////////////////////////////////////////////
// IFile implementation

IFile::IFile(/*IFileSystem* host*/)
//  : _hostFileSystem(host)
{
//    _ASSERT(_hostFileSystem);
}

IFile::~IFile()
{
}

///////////////////////////////////////////////////////////////////////////////

SafePtr<IFileSystem> IFileSystem::Create(const string_t &nodeName)
{
	return WrapRawPtr(new IFileSystem(nodeName));
}

IFileSystem::IFileSystem(const string_t &nodeName)
  : _nodeName(nodeName)
  , _parent(NULL)
{
}

IFileSystem::~IFileSystem(void)
{
	// unmount all children
	while( !_children.empty() )
		_children.begin()->second->Unmount();
}

const string_t IFileSystem::GetFullPath(void) const
{
	string_t fullPath = GetNodeName();
	for( SafePtr<IFileSystem> fs = GetParent(); fs; fs = fs->GetParent() )
	{
		fullPath = fs->GetNodeName() + DELIMITER + fullPath;
	}
	return fullPath;
}

bool IFileSystem::MountTo(IFileSystem *parent)
{
	_ASSERT(!GetNodeName().empty() && GetNodeName() != TEXT("/"));
	_ASSERT(!_parent); // may be is already mounted somewhere?
	                   // only one parent is allowed

	// check if node with the same name is already exists
	StrToFileSystemMap::iterator it = parent->_children.find(_nodeName);
	if( parent->_children.end() != it )
		return false; // node already exists

	// mount
	_parent = parent;
	_parent->_children[_nodeName] = this;

	return true;
}

void IFileSystem::Unmount()
{
	_ASSERT(_parent); // not mounted?
	_ASSERT(_parent->GetFileSystem(GetNodeName()) == this);

	AddRef(); // protect this object from deletion
	_parent->_children.erase(GetNodeName());
	_parent = NULL;
	Release();
}

bool IFileSystem::IsValid() const
{
	return _parent ? _parent->IsValid() : true;
}

SafePtr<IFile> IFileSystem::Open(const string_t &fileName)
{
	if( fileName.empty() ) return NULL;

	string_t::size_type pd = fileName.rfind(DELIMITER);
	if( string_t::npos != pd ) // is a path delimiter found?
	{
		if( SafePtr<IFileSystem> fs = GetFileSystem(fileName.substr(0, pd+1)) )
			return fs->RawOpen(fileName.substr(pd+1));
		return NULL;
	}
	return RawOpen(fileName);
}

bool IFileSystem::EnumAllFiles(std::set<string_t> &files, const string_t &mask)
{
	return true; // base file system can't contain any files
}

SafePtr<IFile> IFileSystem::RawOpen(const string_t &fileName)
{
	return NULL; // base file system can't contain any files
}

SafePtr<IFileSystem> IFileSystem::GetFileSystem(const string_t &path)
{
	_ASSERT(!path.empty());

//    if( path[0] == DELIMITER ) //

	string_t::size_type offset = (string_t::size_type) (path[0] == DELIMITER);

	if( path.length() == offset )
		return WrapRawPtr(this); // path contains only one symbol '/'

	string_t::size_type p = path.find( DELIMITER, offset );

	StrToFileSystemMap::const_iterator it = _children.find(
		path.substr(offset, string_t::npos != p ? p - offset : p));
	if( _children.end() == it )
		return NULL; // node not found

	if( string_t::npos != p )
		return it->second->GetFileSystem(path.substr(p));

	return it->second;
}

///////////////////////////////////////////////////////////////////////////////

OSFileSystem::OSFile::OSFile(const TCHAR *fileName)
{
	// replace all '/' by '\'
	string_t tmp = fileName;
	for( string_t::iterator it = tmp.begin(); tmp.end() != it; ++it )
	{
		if( DELIMITER == *it ) *it = TEXT('\\');
	}

	_handle = ::CreateFile(
		tmp.c_str(),
		FILE_READ_ACCESS,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);
}

OSFileSystem::OSFile::~OSFile()
{
	if( INVALID_HANDLE_VALUE != _handle )
	{
		CloseHandle(_handle);
	}
}

size_t OSFileSystem::OSFile::Read(void *data, size_t size)
{
	DWORD bytesRead = 0;
	ReadFile(_handle, data, size, &bytesRead, NULL);
	return bytesRead;
}

bool OSFileSystem::OSFile::Write(const void *data, size_t size)
{
	DWORD bytesWritten = 0;
	WriteFile(_handle, data, size, &bytesWritten, NULL);
	return bytesWritten == size;
}

bool OSFileSystem::OSFile::Seek(long offset, int origin)
{
	DWORD method = 0;
	switch( origin )
	{
	case SEEK_SET: method = FILE_BEGIN;   break;
	case SEEK_CUR: method = FILE_CURRENT; break;
	case SEEK_END: method = FILE_END;     break;
	default:
		_ASSERT(FALSE);
	}
	return INVALID_SET_FILE_POINTER != SetFilePointer(_handle, offset, NULL, method);
}

size_t OSFileSystem::OSFile::Tell() const
{
	return SetFilePointer(_handle, 0, NULL, FILE_CURRENT);
}

bool OSFileSystem::OSFile::IsOpen() const
{
	return INVALID_HANDLE_VALUE != _handle;
}

///////////////////////////////////////

SafePtr<OSFileSystem> OSFileSystem::Create(const string_t &rootDirectory, const string_t &nodeName)
{
	return WrapRawPtr(new OSFileSystem(rootDirectory, nodeName));
}

OSFileSystem::OSFileSystem(const string_t &rootDirectory, const string_t &nodeName)
  : IFileSystem(nodeName)
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
  : IFileSystem(nodeName)
{
	_ASSERT(parent);
	_ASSERT(string_t::npos == nodeName.find(DELIMITER));

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

bool OSFileSystem::EnumAllFiles(std::set<string_t> &files, const string_t &mask)
{
	bool result = true;

	// remember current directory to restore it later
	DWORD len = GetCurrentDirectory(0, NULL);
	TCHAR *curDir = new TCHAR[len];
	GetCurrentDirectory(len, curDir);

	if( SetCurrentDirectory(_rootDirectory.c_str()) )
	{
		WIN32_FIND_DATA fd;
		HANDLE hSearch = FindFirstFile(mask.c_str(), &fd);
		if( INVALID_HANDLE_VALUE != hSearch )
		{
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
		}
		else
		{
			if( ERROR_FILE_NOT_FOUND != GetLastError() )
			{
				result = false;
			}
		}
		SetCurrentDirectory(curDir);
	}
	else
	{
		result = false;
	}

	delete[] curDir;
	return result;
}

SafePtr<IFile> OSFileSystem::RawOpen(const string_t &fileName)
{
	// combine with the root path
	string_t tmp = _rootDirectory + TEXT('\\') + fileName;

	// open file and return pointer if succeeded
	SafePtr<OSFile> f(new OSFile(tmp.c_str()));
	if( f->IsOpen() )
		return f;

	return NULL;  // open file failed
}

SafePtr<IFileSystem> OSFileSystem::GetFileSystem(const string_t &path)
{
	if( SafePtr<IFileSystem> child = IFileSystem::GetFileSystem(path) )
		return child;

	_ASSERT(!path.empty());

	string_t::size_type offset = (string_t::size_type) (path[0] == DELIMITER);
	if( path.length() == offset )
		return WrapRawPtr(this); // path contains only one DELIMITER symbol

	string_t::size_type p = path.find( DELIMITER, offset );
	string_t dirName = path.substr(offset, string_t::npos != p ? p - offset : p);

	// try to find directory
	WIN32_FIND_DATA fd = {0};
	HANDLE search = FindFirstFile((_rootDirectory + TEXT('\\') + dirName).c_str(), &fd);
	if( INVALID_HANDLE_VALUE == search )
		return NULL; // not found
	FindClose(search);

	if( fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
	{
		SafePtr<IFileSystem> child(new OSFileSystem(this, dirName));
		if( string_t::npos != p )
			return child->GetFileSystem(path.substr(p)); // process the rest of the path
		return child; // last path node was processed
	}

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// end of file

