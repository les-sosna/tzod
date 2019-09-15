#include "inc/fs/FileSystem.h"
#include "inc/fs/StreamWrapper.h"

using namespace FS;

StreamBuf::int_type StreamBuf::overflow(int_type ch)
{
	m_stream.Write(&ch, 1);
	return 1;
}

std::streamsize StreamBuf::xsputn(const char_type* s, std::streamsize n)
{
	m_stream.Write(s, static_cast<size_t>(n));
	return n;
}
