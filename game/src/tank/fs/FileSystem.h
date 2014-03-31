// FileSystem.h

#pragma once

///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////
#ifdef _WIN32
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
		OSFile(const std::string &fileName, FileMode mode);
		virtual ~OSFile();

		// File
		virtual SafePtr<MemMap> QueryMap();
		virtual SafePtr<Stream> QueryStream();
		virtual void Unmap();
		virtual void Unstream();

	private:
		class OSMemMap : public MemMap
		{
		public:
			OSMemMap(const SafePtr<File> &parent, HANDLE hFile);
			virtual ~OSMemMap();

			virtual char* GetData();
			virtual unsigned long GetSize() const;
			virtual void SetSize(unsigned long size); // may invalidate pointer returned by GetData()

		private:
			HANDLE _hFile;
			AutoHandle _map;
			void *_data;
			size_t _size;
			void SetupMapping();
		};

		class OSStream : public Stream
		{
		public:
			OSStream(const SafePtr<File> &parent, HANDLE hFile);

			virtual ErrorCode Read(void *dst, size_t size);
			virtual void Write(const void *src, size_t size);
			virtual void Seek(long long amount, unsigned int origin);

		private:
			HANDLE _hFile;
		};

	private:
		AutoHandle _file;
		FileMode _mode;
		bool _mapped;
		bool _streamed;
	};

	std::string  _rootDirectory;

private:
	// private constructors for internal use by GetFileSystem() and Create()
	OSFileSystem(OSFileSystem *parent, const std::string &nodeName);
	OSFileSystem(const std::string &rootDirectory, const std::string &nodeName = std::string());

protected:
	virtual ~OSFileSystem(); // protected destructor. delete via Release() only
	virtual SafePtr<File> RawOpen(const std::string &fileName, FileMode mode);

public:
	virtual SafePtr<FileSystem> GetFileSystem(const std::string &path, bool create = false, bool nothrow = false);
	virtual bool IsValid() const;
	virtual void EnumAllFiles(std::set<std::string> &files, const std::string &mask);

	static SafePtr<OSFileSystem> Create(const std::string &rootDirectory, const std::string &nodeName = std::string());
};
#else
class OSFileSystem : public FileSystem
{
    struct AutoHandle
    {
        FILE *f = nullptr;
        AutoHandle() {}
        ~AutoHandle()
        {
            if( f )
                fclose(f);
        }
    private:
        AutoHandle(const AutoHandle&);
        AutoHandle& operator = (const AutoHandle&);
    };
    
    class OSFile : public File
    {
    public:
        OSFile(const std::string &fileName, FileMode mode);
        virtual ~OSFile();
        
        // File
        virtual SafePtr<MemMap> QueryMap();
        virtual SafePtr<Stream> QueryStream();
        virtual void Unmap();
        virtual void Unstream();
        
    private:
        class OSMemMap : public MemMap
        {
        public:
            OSMemMap(const SafePtr<OSFile> &parent);
            virtual ~OSMemMap();
            
            virtual char* GetData();
            virtual unsigned long GetSize() const;
            virtual void SetSize(unsigned long size); // may invalidate pointer returned by GetData()
            
        private:
            SafePtr<OSFile> _file;
            std::vector<char> _data;
            void SetupMapping();
        };
        
        class OSStream : public Stream
        {
        public:
            OSStream(const SafePtr<OSFile> &parent);
            virtual ~OSStream();
            
            virtual ErrorCode Read(void *dst, size_t size);
            virtual void Write(const void *src, size_t size);
            virtual void Seek(long long amount, unsigned int origin);

        private:
            SafePtr<OSFile> _file;
        };
        
    private:
        AutoHandle _file;
        FileMode _mode;
        bool _mapped;
        bool _streamed;
    };
    
    std::string  _rootDirectory;
    
private:
    // private constructors for internal use by GetFileSystem() and Create()
    OSFileSystem(OSFileSystem *parent, const std::string &nodeName);
    OSFileSystem(const std::string &rootDirectory, const std::string &nodeName = std::string());
    
protected:
    virtual ~OSFileSystem(); // protected destructor. delete via Release() only
    virtual SafePtr<File> RawOpen(const std::string &fileName, FileMode mode);
    
public:
    virtual SafePtr<FileSystem> GetFileSystem(const std::string &path, bool create = false, bool nothrow = false);
    virtual bool IsValid() const;
    virtual void EnumAllFiles(std::set<std::string> &files, const std::string &mask);
    
    static SafePtr<OSFileSystem> Create(const std::string &rootDirectory, const std::string &nodeName = std::string());
};
#endif
///////////////////////////////////////////////////////////////////////////////
} // end of namespace FS
///////////////////////////////////////////////////////////////////////////////
// end of file
