#pragma once
#include <fs/FileSystem.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace FS {

struct AutoHandle
{
	HANDLE h;
	AutoHandle() : h(nullptr) {}
	~AutoHandle()
	{
		if (nullptr != h && INVALID_HANDLE_VALUE != h)
		{
			CloseHandle(h);
		}
	}
private:
	AutoHandle(const AutoHandle&) = delete;
	AutoHandle& operator = (const AutoHandle&) = delete;
};

class FileWin32 final
	: public File
	, public std::enable_shared_from_this<FileWin32>
{
public:
	FileWin32(std::wstring fileName, FileMode mode);
	~FileWin32();
	void Unmap();
	void Unstream();

	// File
	std::shared_ptr<MemMap> QueryMap() override;
	std::shared_ptr<Stream> QueryStream() override;

private:
	AutoHandle _file;
	FileMode _mode;
	bool _mapped;
	bool _streamed;
};

class MemMapWin32 final
	: public MemMap
	, public Stream
{
public:
	MemMapWin32(std::shared_ptr<FileWin32> parent, HANDLE hFile);
	~MemMapWin32();

	// MemMap
	const void* GetData() const override;
	unsigned long GetSize() const override;
	void SetSize(unsigned long size) override; // may invalidate pointer returned by GetData()

	// Stream
	size_t Read(void *dst, size_t size, size_t count) override;
	void Write(const void *src, size_t size) override;
	void Seek(long long amount, unsigned int origin) override;
	long long Tell() const override { return _pos; }

private:
	std::shared_ptr<FileWin32> _file;
	HANDLE _hFile;
	AutoHandle _map;
	const void *_data;
	size_t _size;
	size_t _pos = 0;
	void SetupMapping();
};

class StreamWin32 final
	: public Stream
{
public:
	StreamWin32(std::shared_ptr<FileWin32> parent, HANDLE hFile);
	~StreamWin32();

	// Stream
	size_t Read(void *dst, size_t size, size_t count) override;
	void Write(const void *src, size_t size) override;
	void Seek(long long amount, unsigned int origin) override;
	long long Tell() const override;

private:
	std::shared_ptr<FileWin32> _file;
	HANDLE _hFile;
};

class FileSystemWin32 final
	: public FileSystem
{
public:
	explicit FileSystemWin32(std::wstring rootDirectory);
	explicit FileSystemWin32(std::string_view rootDirectory);

	// FileSystem
	std::shared_ptr<FileSystem> GetFileSystem(std::string_view path, bool create = false, bool nothrow = false) override;
	std::vector<std::string> EnumAllFiles(std::string_view mask) override;

private:
	std::wstring _rootDirectory;
	std::shared_ptr<File> RawOpen(std::string_view fileName, FileMode mode) override;
};

} // namespace FS
