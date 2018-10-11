#pragma once
#include <fs/FileSystem.h>

namespace FS {

class FileSystemJni final
	: public FileSystem
{
    class OSFile final
        : public File
        , public std::enable_shared_from_this<OSFile>
    {
    public:
        OSFile(const std::string &fileName, FileMode mode);
        ~OSFile();

        // File
        std::shared_ptr<MemMap> QueryMap() override;
        std::shared_ptr<Stream> QueryStream() override;

        void Unmap();
        void Unstream();

    private:
        class OSMemMap final
			: public MemMap
        {
        public:
            OSMemMap(std::shared_ptr<OSFile> parent);
            ~OSMemMap();

            // MemMap
            char* GetData() override;
            unsigned long GetSize() const override;
            void SetSize(unsigned long size) override; // may invalidate pointer returned by GetData()

        private:
            std::shared_ptr<OSFile> _file;
            std::vector<char> _data;
            void SetupMapping();
        };

        class OSStream final
			: public Stream
        {
        public:
            OSStream(std::shared_ptr<OSFile> parent);
            ~OSStream();

            size_t Read(void *dst, size_t size, size_t count) override;
            void Write(const void *src, size_t size) override;
            void Seek(long long amount, unsigned int origin) override;
            long long Tell() const override;

        private:
            std::shared_ptr<OSFile> _file;
        };

    private:
        FileMode _mode;
        bool _mapped;
        bool _streamed;
    };

    std::string _rootDirectory;

protected:
	std::shared_ptr<File> RawOpen(std::string_view fileName, FileMode mode) override;

public:
    FileSystemJni(std::string rootDirectory);
	std::shared_ptr<FileSystem> GetFileSystem(std::string_view path, bool create = false, bool nothrow = false) override;
	std::vector<std::string> EnumAllFiles(std::string_view mask) override;
};

} // namespace FS
