// Console.h

#pragma once


class ConsoleBuffer
{
	char *_line;
	std::deque<char*> _lines;

	size_t _lineLength; // including terminating '\0'
	size_t _maxLines;

public:
	ConsoleBuffer(size_t lineLength, size_t _maxLines);
	~ConsoleBuffer();

	void Fill(const ConsoleBuffer *src);

	size_t GetLineCount() const;
	const char* GetLine(size_t index) const;

	void print(const char *fmt, ...);
};


// end of file
