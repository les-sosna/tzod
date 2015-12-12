#pragma once
#include "inc/fs/FileSystem.h"

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

    class OSFile
        : public File
        , public std::enable_shared_from_this<OSFile>
    {
    public:
        OSFile(const std::string &fileName, FileMode mode);
        virtual ~OSFile();

        // File
        virtual std::shared_ptr<MemMap> QueryMap();
        virtual std::shared_ptr<Stream> QueryStream();
        virtual void Unmap();
        virtual void Unstream();

    private:
        class OSMemMap : public MemMap
        {
        public:
            OSMemMap(std::shared_ptr<OSFile> parent);
            virtual ~OSMemMap();

            virtual char* GetData();
            virtual unsigned long GetSize() const;
            virtual void SetSize(unsigned long size); // may invalidate pointer returned by GetData()

        private:
            std::shared_ptr<OSFile> _file;
            std::vector<char> _data;
            void SetupMapping();
        };

        class OSStream : public Stream
        {
        public:
            OSStream(std::shared_ptr<OSFile> parent);
            virtual ~OSStream();

            virtual size_t Read(void *dst, size_t size, size_t count);
            virtual void Write(const void *src, size_t size);
            virtual void Seek(long long amount, unsigned int origin);
            virtual long long Tell() const;

        private:
            std::shared_ptr<OSFile> _file;
        };

    private:
        AutoHandle _file;
        FileMode _mode;
        bool _mapped;
        bool _streamed;
    };

    std::string  _rootDirectory;

protected:
    virtual std::shared_ptr<File> RawOpen(const std::string &fileName, FileMode mode);

public:
    OSFileSystem(const std::string &rootDirectory);
    virtual std::shared_ptr<FileSystem> GetFileSystem(const std::string &path, bool create = false, bool nothrow = false);
	virtual std::vector<std::string> EnumAllFiles(const std::string &mask);

    static std::shared_ptr<OSFileSystem> Create(const std::string &rootDirectory);
};

} // namespace FS
