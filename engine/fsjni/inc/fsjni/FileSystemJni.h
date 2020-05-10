#pragma once
#include <fs/FileSystem.h>

struct AAsset;
struct AAssetDir;
struct AAssetManager;

namespace FS {

struct AAssetDirDeleter
{
    void operator() (AAssetDir *assetDir);
};
using AAssetDirPtr = std::unique_ptr<AAssetDir, AAssetDirDeleter>;

struct AAssetDeleter
{
    void operator() (AAsset *asset);
};
using AAssetPtr = std::unique_ptr<AAsset, AAssetDeleter>;

class FileSystemJni final
	: public FileSystem
{
    class OSFile final
        : public File
        , public std::enable_shared_from_this<OSFile>
    {
    public:
        OSFile(AAssetPtr asset);

        // File
        std::shared_ptr<MemMap> QueryMap() override;
        std::shared_ptr<Stream> QueryStream() override;

    private:
        void Unmap();
        void Unstream();

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
        AAssetPtr _asset;
        bool _mapped = false;
        bool _streamed = false;
    };

    AAssetManager *_assetManager;
    std::string _rootDirectory;
    AAssetDirPtr _assetDir;

protected:
	std::shared_ptr<File> RawOpen(std::string_view fileName, FileMode mode, bool nothrow) override;

public:
    explicit FileSystemJni(AAssetManager *assetManager, std::string rootDirectory = {});
	std::shared_ptr<FileSystem> GetFileSystem(std::string_view path, bool create = false, bool nothrow = false) override;
	std::vector<std::string> EnumAllFiles(std::string_view mask) override;
};

} // namespace FS
