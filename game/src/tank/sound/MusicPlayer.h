// MusicPlayer.h

class IFile;


class MusicPlayer
{
	SafePtr<IFile> _file;
	bool _looping;
	bool _playbackDone;
	bool _firstHalfPlaying;
	OggVorbis_File _vorbisFile;
	LPDIRECTSOUNDBUFFER8 _buffer;
	DWORD _bufHalfSize;

	void Cleanup();
	bool Fill(bool firstHalf);

	static size_t read_func  (void *ptr, size_t size, size_t nmemb, void *datasource);
//	static int    close_func (void *datasource);
	static int    seek_func  (void *datasource, ogg_int64_t offset, int whence);
	static long   tell_func  (void *datasource);

	void OnChangeVolume();

public:
	MusicPlayer();
	~MusicPlayer();

	bool Load(SafePtr<IFile> file);

	void Stop();
	void Play(bool looping = false);

	void HandleBufferFilling();
};


// end of file
