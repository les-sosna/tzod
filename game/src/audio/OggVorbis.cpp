#include "OggVorbis.h"
#include "inc/audio/detail/FormatDesc.h"
#include <fs/FileSystem.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include <stdexcept>

namespace
{
	struct FileState
	{
		std::shared_ptr<FS::Stream> s;
		long position = 0;
	};

	size_t read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
	{
		auto *state = (FileState *)datasource;
		return state->s->Read(ptr, size, nmemb);
	}

	int seek_func(void *datasource, ogg_int64_t offset, int whence)
	{
		auto *state = (FileState *)datasource;
		state->s->Seek(offset, whence);
		return 0;
	}

	long tell_func(void *datasource)
	{
		auto *state = (FileState *)datasource;
		return static_cast<long>(state->s->Tell());
	}
} // unnamed namespace

void LoadOggVorbis(std::shared_ptr<FS::Stream> stream, FormatDesc &outFormatDesc, std::vector<char> &outData)
{
    FileState state;
    state.s = stream;

	ov_callbacks cb;
	cb.read_func  = read_func;
	cb.seek_func  = seek_func;
	cb.close_func = nullptr;
	cb.tell_func  = tell_func;

	OggVorbis_File vf;
	if( int result = ov_open_callbacks(&state, &vf, nullptr, 0, cb) )
	{
		switch( result )
		{
		case OV_EREAD: throw std::runtime_error("A read from media returned an error");
		case OV_ENOTVORBIS: throw std::runtime_error("Bitstream does not contain any Vorbis data");
		case OV_EVERSION: throw std::runtime_error("Vorbis version mismatch");
		case OV_EBADHEADER: throw std::runtime_error("Invalid Vorbis bitstream header");
		case OV_EFAULT: throw std::runtime_error("Internal logic fault; indicates a bug or heap/stack corruption");
		}
		throw std::runtime_error("unknown error opening ov stream");
	}

	try
	{
		vorbis_info *pinfo = ov_info(&vf, -1);
		if( nullptr == pinfo )
		{
			throw std::runtime_error("could not get info from ov stream");
		}


		outFormatDesc.channels = pinfo->channels;
		outFormatDesc.frequency = pinfo->rate;

		ogg_int64_t nSamples = ov_pcm_total(&vf, -1);
		size_t size = static_cast<size_t>(nSamples * pinfo->channels * 2);
		outData.resize(size);

		int bitstream = 0;
		size_t total = 0;

		while( total < size )
		{
			long ret = ov_read(&vf, &outData[total], size - total, 0, 2, 1, &bitstream);
			if( 0 == ret )
			{
				break; // eof
			}
			if( ret < 0 )
			{
				// error in stream
				switch( ret )
				{
				case OV_HOLE:
					throw std::runtime_error("garbage between pages, loss of sync followed by recapture, or a corrupt page");
				case OV_EBADLINK:
					throw std::runtime_error("invalid stream section or the requested link is corrupt");
				case OV_EINVAL:
					throw std::runtime_error("initial file headers couldn't be read or are corrupt");
				}
				throw std::runtime_error("unknown error in ov stream");
			}
			else
			{
				total += ret;
			}
		}
	}
	catch( const std::exception& )
	{
		ov_clear(&vf);
		throw;
	}
	ov_clear(&vf);
}
