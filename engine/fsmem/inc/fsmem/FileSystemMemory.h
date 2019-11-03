#pragma once
#include <fs/FileSystem.h>
#include <vector>

namespace FS
{
	class MemoryStream : public Stream
	{
	public:
		size_t Read(void *dst, size_t size, size_t count) override;
		void Write(const void *src, size_t size)  override;
		void Seek(long long amount, unsigned int origin)  override;
		long long Tell() const  override;

	private:
		std::vector<char> _data;
		size_t _streamPos = 0;
	};
}
