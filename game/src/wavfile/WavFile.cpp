#include "inc/wavfile/WavFile.h"
#include <fs/FileSystem.h>

#pragma pack (push)
#pragma pack (1)
struct RiffHeader
{
	uint32_t ChunkID;
	uint32_t ChunkSize;
	uint32_t Format;
};

struct SubHeader
{
	uint32_t SubchunkID;
	uint32_t SubchunkSize;
};

struct SubchunkFmt
{
	uint16_t AudioFormat;
	uint16_t NumChannels;
	uint32_t SampleRate;
	uint32_t ByteRate;
	uint16_t BlockAlign;
	uint16_t BitsPerSample;
};
#pragma pack (pop)

void LoadWavPcm(FS::Stream& stream, unsigned int& outFrequency, std::vector<char>& outData)
{
	RiffHeader riffHeader;
	if (1 != stream.Read(&riffHeader, sizeof(RiffHeader), 1)
		|| riffHeader.ChunkID != 0x46464952 // RIFF
		|| riffHeader.Format != 0x45564157) // WAVE
	{
		throw std::runtime_error("not a valid WAV audio file");
	}

	SubchunkFmt fmt;
	bool fmtFound = false;
	bool dataFound = false;
	while (!fmtFound || !dataFound)
	{
		SubHeader subHeader;
		if (0 == stream.Read(&subHeader, sizeof(SubHeader), 1))
			break; // end of file
		
		switch (subHeader.SubchunkID)
		{
		case 0x20746d66: // fmt
			if (subHeader.SubchunkSize < sizeof(SubchunkFmt))
				throw std::runtime_error("invalid WAV file");
			if (1 != stream.Read(&fmt, sizeof(SubchunkFmt), 1))
				throw std::runtime_error("unexpected end of file");
			stream.Seek(subHeader.SubchunkSize - sizeof(SubchunkFmt), SEEK_CUR);
			fmtFound = true;
			break;

		case 0x61746164: // data
			outData.resize(subHeader.SubchunkSize);
			if(1 != stream.Read(outData.data(), subHeader.SubchunkSize, 1))
				throw std::runtime_error("unexpected end of file");
			dataFound = true;
			break;

		default:
			stream.Seek(subHeader.SubchunkSize, SEEK_CUR);
		}
	}

	if (!fmtFound)
		throw std::runtime_error("RIFF fmt not found");
	if (!dataFound)
		throw std::runtime_error("RIFF data not found");
	if (fmt.AudioFormat != 1)
		throw std::runtime_error("only PCM audio format is supported");
	if (fmt.NumChannels != 1)
		throw std::runtime_error("only 1 audio channel is supported");
	if (fmt.BitsPerSample != 16)
		throw std::runtime_error("only 16 bit audio is supported");
	outFrequency = fmt.SampleRate;
}

