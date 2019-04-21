#pragma once
#include <vector>

namespace FS
{
	struct Stream;
}
void LoadWavPcm(FS::Stream &stream, unsigned int &outFrequency, std::vector<char> &outData);
