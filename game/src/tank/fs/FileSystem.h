// FileSystem.h

#pragma once

#include <core/SafePtr.h>
#include <map>
#include <string>

namespace FS {

enum FileMode
{
	ModeRead = 0x01,
	ModeWrite = 0x02,
};
    
enum ErrorCode
{
    EC_OK, 
    EC_EOF,
    EC_ERROR,
};

class File;

class MemMap : public RefCounted
{
public:
	virtual char* GetData() = 0;
	virtual unsigned long GetSize() const = 0;
	virtual void SetSize(unsigned long size) = 0; // may invalidate pointer returned by GetData()

protected:
	virtual ~MemMap() {}
};

class Stream : public RefCounted
{
public:
	virtual ErrorCode Read(void *dst, size_t size) = 0;
	virtual void Write(const void *src, size_t size) = 0;
	virtual void Seek(long long amount, unsigned int origin) = 0;

protected:
	virtual ~Stream() {}
};

class File : public RefCounted
{
public:
	virtual SafePtr<MemMap> QueryMap() = 0;
	virtual SafePtr<Stream> QueryStream() = 0;

protected:
	virtual ~File() {} // delete only via Release
};

///////////////////////////////////////////////////////////////////////////////

class FileSystem : public RefCounted
{
	typedef std::map<std::string, SafePtr<FileSystem> > StrToFileSystemMap;

	StrToFileSystemMap _children;
	std::string       _nodeName;
	FileSystem*        _parent;  // 'unsafe' pointer allows to avoid cyclic references
	                             // it is set to NULL when the parent is destroyed

protected:
	FileSystem(const std::string &nodeName);
	virtual ~FileSystem(void); // delete only via Release

	// open a file that strictly belongs to this file system
	virtual SafePtr<File> RawOpen(const std::string &fileName, FileMode mode);

public:
	const std::string GetFullPath(void) const;
	const std::string& GetNodeName(void) const { return _nodeName; }

	FileSystem* GetParent(void) const { return _parent; }

	virtual bool MountTo(FileSystem *parent);
	virtual void Unmount(); // object can become destroyed after that

	virtual SafePtr<FileSystem> GetFileSystem(const std::string &path, bool create = false, bool nothrow = false);

	virtual bool IsValid() const;
	virtual void EnumAllFiles(std::set<std::string> &files, const std::string &mask);
	SafePtr<File> Open(const std::string &path, FileMode mode = ModeRead);

	static SafePtr<FileSystem> Create(const std::string &nodeName = std::string());
};

} // end of namespace FS
///////////////////////////////////////////////////////////////////////////////
// end of file
