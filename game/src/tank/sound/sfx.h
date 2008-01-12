// sfx.h


struct LoadSoundException
{
	string_t  filename;
	HRESULT   hr;
};

//  throws LoadSoundException
void LoadOggVorbis(bool init, enumSoundTemplate sound, const char *filename);


// end of file
