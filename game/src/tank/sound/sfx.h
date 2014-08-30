// sfx.h

namespace FS
{
	class FileSystem;
}

#ifndef NOSOUND
bool InitSound(FS::FileSystem *fs, bool init);
void FreeSound();
#endif

// end of file
