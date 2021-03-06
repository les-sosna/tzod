#pragma once
#include <fs/FileSystem.h>

namespace FS {
    namespace detail
    {
        struct StdioFileDeleter
        {
            void operator()(FILE *file);
        };
        using StdioFile = std::unique_ptr<FILE, StdioFileDeleter>;
    }

class FileSystemPosix final
	: public FileSystem
{
    class OSFile final
        : public File
        , public std::enable_shared_from_this<OSFile>
    {
    public:
        explicit OSFile(detail::StdioFile file);
        ~OSFile();

        void Unmap();
        void Unstream();

        // File
        std::shared_ptr<MemMap> QueryMap() override;
        std::shared_ptr<Stream> QueryStream() override;

    private:
        class OSMemMap final
			: public MemMap
        {
        public:
            OSMemMap(std::shared_ptr<OSFile> parent);
            ~OSMemMap();

            // MemMap
            const void* GetData() const override;
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
        detail::StdioFile _file;
        bool _mapped;
        bool _streamed;
    };

    std::string _rootDirectory;

protected:
	std::shared_ptr<File> RawOpen(std::string_view fileName, FileMode mode, bool nothrow) override;

public:
    FileSystemPosix(std::string rootDirectory);
	std::shared_ptr<FileSystem> GetFileSystem(std::string_view path, bool create = false, bool nothrow = false) override;
	std::vector<std::string> EnumAllFiles(std::string_view mask) override;
};

} // namespace FS
