// Console.h

#pragma once


class ConsoleBuffer
{
	char*  _buffer;
	char** _lines;      // line positions in the buffer

	size_t  _currentPos;   // carret position in the current line
	size_t  _currentLine;  // current line number
	size_t  _currentCount; // current number of lines in the buffer

	size_t _lineLength; // including terminating '\0'
	size_t _lineCount;  // maximum number of lines in the buffer

	FILE  *_logFile;

public:
	ConsoleBuffer(size_t lineLength, size_t _maxLines);
	~ConsoleBuffer();

	void Fill(const ConsoleBuffer *src);

	size_t GetLineCount() const;
	const char* GetLine(size_t index) const;

	void printf(const char *fmt, ...);
	void puts(const char *s);
};


// end of file
