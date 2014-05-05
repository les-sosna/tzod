// MusicPlayer.h

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include <al.h>
#include <array>
#include <memory>

namespace FS
{
	class MemMap;
}


class MusicPlayer
{
	struct State
	{
        std::shared_ptr<FS::MemMap> file; // TODO: use stream
		unsigned long ptr = 0;
	};
	State _state;
	OggVorbis_File _vorbisFile;
    std::array<ALuint, 3> _buffers;
    ALuint _source;
    bool _playing;

	void Cleanup();
	void FillAndQueue(ALuint bufName);

	static size_t read_func  (void *ptr, size_t size, size_t nmemb, void *datasource);
	static int    seek_func  (void *datasource, ogg_int64_t offset, int whence);
	static long   tell_func  (void *datasource);

	void OnChangeVolume();

public:
	MusicPlayer();
	virtual ~MusicPlayer();

	bool Load(std::shared_ptr<FS::MemMap> file); // TODO: use stream

	void Stop();
	void Play();

	void HandleBufferFilling();
};


// end of file
