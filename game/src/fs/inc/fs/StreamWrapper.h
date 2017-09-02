#pragma once
#include <streambuf>

namespace FS
{
	struct Stream;

	class StreamBuf : public std::streambuf
	{
	public:
		StreamBuf(Stream &stream)
			: m_stream(stream)
		{}

	protected:
		int_type overflow(int_type ch) override;
		std::streamsize xsputn(const char_type* s, std::streamsize n) override;

	private:
		Stream &m_stream;
	};

	class OutStreamWrapper
		: private StreamBuf
		, public std::ostream
	{
	public:
		OutStreamWrapper(Stream &stream)
			: StreamBuf(stream)
			, std::ostream(this)
		{}
	};
}
