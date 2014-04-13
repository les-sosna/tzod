// FileSystem.h

#pragma once

#include <map>
#include <string>
#include <vector>
#include <memory>

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

class MemMap
{
public:
	virtual char* GetData() = 0;
	virtual unsigned long GetSize() const = 0;
	virtual void SetSize(unsigned long size) = 0; // may invalidate pointer returned by GetData()

protected:
	virtual ~MemMap() {}
};

class Stream
{
public:
	virtual ErrorCode Read(void *dst, size_t size) = 0;
	virtual void Write(const void *src, size_t size) = 0;
	virtual void Seek(long long amount, unsigned int origin) = 0;

protected:
	virtual ~Stream() {}
};

class File
{
public:
	virtual std::shared_ptr<MemMap> QueryMap() = 0;
	virtual std::shared_ptr<Stream> QueryStream() = 0;

protected:
	virtual ~File() {} // delete only via Release
};

///////////////////////////////////////////////////////////////////////////////

class FileSystem
{
public:
	virtual std::shared_ptr<FileSystem> GetFileSystem(const std::string &path, bool create = false, bool nothrow = false);
	virtual std::vector<std::string> EnumAllFiles(const std::string &mask);
	std::shared_ptr<File> Open(const std::string &path, FileMode mode = ModeRead);
	void Mount(const std::string &nodeName, std::shared_ptr<FileSystem> fs);

private:
	std::map<std::string, std::shared_ptr<FileSystem>> _children;
	// open a file that strictly belongs to this file system
	virtual std::shared_ptr<File> RawOpen(const std::string &fileName, FileMode mode);
};

} // end of namespace FS
///////////////////////////////////////////////////////////////////////////////
// end of file
