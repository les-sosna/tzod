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

struct MemMap
{
	virtual char* GetData() = 0;
	virtual unsigned long GetSize() const = 0;
	virtual void SetSize(unsigned long size) = 0; // may invalidate pointer returned by GetData()
};

struct Stream
{
	virtual size_t Read(void *dst, size_t size, size_t count) = 0;
	virtual void Write(const void *src, size_t size) = 0;
	virtual void Seek(long long amount, unsigned int origin) = 0;
	virtual long long Tell() const = 0;
};

struct File
{
	virtual std::shared_ptr<MemMap> QueryMap() = 0;
	virtual std::shared_ptr<Stream> QueryStream() = 0;
};

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

std::shared_ptr<FS::FileSystem> CreateOSFileSystem(const std::string &rootDirectory);

} // namespace FS
