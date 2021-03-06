#pragma once

#include <map>
#include <string>
#include <string_view>
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
	virtual const void* GetData() const = 0;
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
	virtual std::shared_ptr<FileSystem> GetFileSystem(std::string_view path, bool create = false, bool nothrow = false);
	virtual std::vector<std::string> EnumAllFiles(std::string_view mask);
	std::shared_ptr<File> Open(std::string_view path, FileMode mode = ModeRead, bool nothrow = false);
	void Mount(std::string_view nodeName, std::shared_ptr<FileSystem> fs);

private:
	std::map<std::string, std::shared_ptr<FileSystem>, std::less<>> _children;
	// open a file that strictly belongs to this file system
	virtual std::shared_ptr<File> RawOpen(std::string_view fileName, FileMode mode, bool nothrow) = 0;
};

std::string PathCombine(std::string_view first, std::string_view second);

} // namespace FS
