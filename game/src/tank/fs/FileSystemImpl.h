// FileSystem.h

#pragma once

#include "FileSystem.h"


///////////////////////////////////////////////////////////////////////////////
#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace FS {

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
		OSFile(std::wstring &&fileName, FileMode mode);
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
			OSMemMap(const SafePtr<OSFile> &parent, HANDLE hFile);
			virtual ~OSMemMap();

			virtual char* GetData();
			virtual unsigned long GetSize() const;
			virtual void SetSize(unsigned long size); // may invalidate pointer returned by GetData()

		private:
			SafePtr<OSFile> _file;
			HANDLE _hFile;
			AutoHandle _map;
			void *_data;
			size_t _size;
			void SetupMapping();
		};

		class OSStream : public Stream
		{
		public:
			OSStream(const SafePtr<OSFile> &parent, HANDLE hFile);
			virtual ~OSStream();

			virtual ErrorCode Read(void *dst, size_t size);
			virtual void Write(const void *src, size_t size);
			virtual void Seek(long long amount, unsigned int origin);

		private:
			SafePtr<OSFile> _file;
			HANDLE _hFile;
		};

	private:
		AutoHandle _file;
		FileMode _mode;
		bool _mapped;
		bool _streamed;
	};

	std::wstring  _rootDirectory;

private:
	// private constructors for internal use by GetFileSystem() and Create()
	OSFileSystem(OSFileSystem *parent, const std::string &nodeName);
	OSFileSystem(std::wstring &&rootDirectory, const std::string &nodeName = std::string());

protected:
	virtual ~OSFileSystem(); // protected destructor. delete via Release() only
	virtual SafePtr<File> RawOpen(const std::string &fileName, FileMode mode);

public:
	virtual SafePtr<FileSystem> GetFileSystem(const std::string &path, bool create = false, bool nothrow = false);
	virtual bool IsValid() const;
	virtual std::vector<std::string> EnumAllFiles(const std::string &mask);

	static SafePtr<OSFileSystem> Create(const std::string &rootDirectory, const std::string &nodeName = std::string());
};
} // end of namespace FS
#else
namespace FS {

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
	virtual std::vector<std::string> EnumAllFiles(const std::string &mask);
    
    static SafePtr<OSFileSystem> Create(const std::string &rootDirectory, const std::string &nodeName = std::string());
};
} // end of namespace FS
#endif
///////////////////////////////////////////////////////////////////////////////
// end of file
