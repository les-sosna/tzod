// FileSystem.h

#pragma once

///////////////////////////////////////////////////////////////////////////////

namespace FS {

enum FileMode
{
	ModeRead = 0x01,
	ModeWrite = 0x02,
};

class File : public RefCounted
{
//    FileSystem::Ptr _hostFileSystem; // this prevents the file system to be
									  // destroyed while the file remains open
protected:
	File(/*FileSystem::Ptr host*/); // create via FileSystem::Open only
	virtual ~File(); // delete only via Release

public:
	virtual char* GetData() = 0;
	virtual unsigned long GetSize() const = 0;
	virtual void SetSize(unsigned long size) = 0; // may invalidate pointer returned by GetData()
};

///////////////////////////////////////////////////////////////////////////////

class FileSystem : public RefCounted
{
	typedef std::map<string_t, SafePtr<FileSystem> > StrToFileSystemMap;

	StrToFileSystemMap _children;
	string_t           _nodeName;
	FileSystem*        _parent;  // 'unsafe' pointer allows to avoid cyclic references
	                             // it is set to NULL when the parent is destroyed

protected:
	FileSystem(const string_t &nodeName);
	virtual ~FileSystem(void); // delete only via Release

	// open a file that strictly belongs to this file system
	virtual SafePtr<File> RawOpen(const string_t &fileName, FileMode mode);

public:
	static const TCHAR DELIMITER = TEXT('/');

	const string_t GetFullPath(void) const;
	const string_t& GetNodeName(void) const { return _nodeName; }

	FileSystem* GetParent(void) const { return _parent; }

	virtual bool MountTo(FileSystem *parent);
	virtual void Unmount(); // object can become destroyed after that

	virtual SafePtr<FileSystem> GetFileSystem(const string_t &path, bool create = false, bool nothrow = false);

	virtual bool IsValid() const;
	virtual void EnumAllFiles(std::set<string_t> &files, const string_t &mask);
	SafePtr<File> Open(const string_t &path, FileMode mode = ModeRead);

	static SafePtr<FileSystem> Create(const string_t &nodeName = TEXT(""));
};

///////////////////////////////////////////////////////////////////////////////

class OSFileSystem : public FileSystem
{
	struct AutoHandle
	{
		HANDLE h;
		AutoHandle() : h(NULL) {}
		~AutoHandle()
		{
			if( NULL != h && INVALID_HANDLE_VALUE != h )
			{
				CloseHandle(h);
			}
		}
	private:
		AutoHandle(const AutoHandle&);
		AutoHandle& operator = (const AutoHandle&);
	};

	class OSFile : public File
	{
	public:
		OSFile(const string_t &fileName, FileMode mode);
		virtual ~OSFile();

		virtual char* GetData();
		virtual unsigned long GetSize() const;
		virtual void SetSize(unsigned long size); // may invalidate pointer returned by GetData()

	private:
		AutoHandle _file;
		AutoHandle _map;
		void *_data;
		DWORD _size;
		FileMode _mode;

		void SetupMapping();
	};

	string_t  _rootDirectory;

private:
	// private constructors for internal use by GetFileSystem() and Create()
	OSFileSystem(OSFileSystem *parent, const string_t &nodeName);
	OSFileSystem(const string_t &rootDirectory, const string_t &nodeName = TEXT(""));

protected:
	virtual ~OSFileSystem(); // protected destructor. delete via Release() only
	virtual SafePtr<File> RawOpen(const string_t &fileName, FileMode mode);

public:
	virtual SafePtr<FileSystem> GetFileSystem(const string_t &path, bool create = false, bool nothrow = false);
	virtual bool IsValid() const;
	virtual void EnumAllFiles(std::set<string_t> &files, const string_t &mask);

	static SafePtr<OSFileSystem> Create(const string_t &rootDirectory, const string_t &nodeName = TEXT(""));
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace FS
///////////////////////////////////////////////////////////////////////////////
// end of file
