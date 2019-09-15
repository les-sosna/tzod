#pragma once
#include <memory>
#include <vector>

namespace FS
{
	struct Stream;
}
struct FormatDesc;
void LoadOggVorbis(std::shared_ptr<FS::Stream> stream, FormatDesc &outFormatDesc, std::vector<char> &outData);
