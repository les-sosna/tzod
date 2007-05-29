// FileSystem.h

#pragma once

///////////////////////////////////////////////////////////////////////////////

class IFile : public RefCounted
{
//    IFileSystem::Ptr _hostFileSystem; // this prevents the file system to be
                                      // destroyed while the file remains open
protected:
    IFile(/*IFileSystem::Ptr host*/); // create via IFileSystem::Open only
    virtual ~IFile(); // delete only via Release

public:
    virtual bool Read(void *data, unsigned int size)        = 0;
    virtual bool Write(const void *data, unsigned int size) = 0;
};

///////////////////////////////////////////////////////////////////////////////

class IFileSystem : public RefCounted
{
    typedef std::map<string_t, SafePtr<IFileSystem> > StrToFileSystemMap;

    StrToFileSystemMap _children;
    string_t           _nodeName;
    IFileSystem*       _parent;  // 'unsafe' pointer allows to avoid cyclic references
                                 // it assigned to NULL when the parent is been destoyed

protected:
    IFileSystem(const string_t &nodeName);
    virtual ~IFileSystem(void); // delete only via Release

    // open a file that strictly belongs to this file system
    virtual SafePtr<IFile> RawOpen(const string_t &fileName);

public:
    static const TCHAR DELIMITER = TEXT('/');

    const string_t  GetFullPath(void) const;
    const string_t& GetNodeName(void) const { return _nodeName; }

    const SafePtr<IFileSystem> GetParent(void) const { return _parent; }

    virtual bool MountTo(SafePtr<IFileSystem> parent);
    virtual void Unmount(); // object can become destroyed after that

    virtual SafePtr<IFileSystem> GetFileSystem(const string_t &path);

    virtual bool IsValid() const;
    virtual SafePtr<IFile> Open(const string_t &path);
	virtual bool EnumAllFiles(std::set<string_t> &files, const string_t &mask);

    static SafePtr<IFileSystem> Create(const string_t &nodeName = TEXT(""));
};

///////////////////////////////////////////////////////////////////////////////

class OSFileSystem : public IFileSystem
{
    class OSFile : public IFile
    {
        HANDLE _handle;
    public:
        OSFile(const TCHAR *fileName);
        virtual ~OSFile();

        virtual bool Read(void *data, unsigned int size);
        virtual bool Write(const void *data, unsigned int size);

        bool IsOpen() const;
    };

    string_t  _rootDirectory;

private:
    // private constructors for internal use by GetFileSystem() and Create()
    OSFileSystem(OSFileSystem *parent, const string_t &nodeName);
    OSFileSystem(const string_t &rootDirectory, const string_t &nodeName = TEXT(""));

protected:
    virtual ~OSFileSystem(); // protected destructor. delete via Release() only
    virtual SafePtr<IFile> RawOpen(const string_t &fileName);

public:
    virtual SafePtr<IFileSystem> GetFileSystem(const string_t &path);
    virtual bool IsValid() const;
	virtual bool EnumAllFiles(std::set<string_t> &files, const string_t &mask);

    static SafePtr<OSFileSystem> Create(const string_t &rootDirectory,
                                        const string_t &nodeName = TEXT(""));
};

///////////////////////////////////////////////////////////////////////////////
// end of file
