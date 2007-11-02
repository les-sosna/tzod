// Console.cpp

#include "stdafx.h"
#include "Console.h"

///////////////////////////////////////////////////////////////////////////////

ConsoleBuffer::ConsoleBuffer(size_t lineLength, size_t maxLines, const char *logfile)
{
	_buffer = new char [maxLines * (lineLength+1)];
	_lines  = new char*[maxLines];
	for( size_t i = 0; i < maxLines; ++i )
		_lines[i] = _buffer + (lineLength + 1) * i;
	*_buffer = 0;

	_currentPos   = 0;
	_currentLine  = 0;
	_currentCount = 1;
	_lineLength   = lineLength;
	_lineCount    = maxLines;

	_logFile = logfile ? fopen(logfile, "w") : NULL;
}

ConsoleBuffer::~ConsoleBuffer()
{
	delete[] _lines;
	delete[] _buffer;

	if( _logFile )
		fclose(_logFile);
}

void ConsoleBuffer::Fill(const ConsoleBuffer *src)
{
}

size_t ConsoleBuffer::GetLineCount() const
{
	return _currentCount;
}

const char* ConsoleBuffer::GetLine(size_t index) const
{
	_ASSERT(index < _currentCount);
	return _lines[(_lineCount + index + _currentLine + 1 - _currentCount) % _lineCount];
}

void ConsoleBuffer::printf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	int size = _vscprintf(fmt, args) + 1; // check how much space is needed

	char *buf = new char[size];
	vsprintf(buf, fmt, args);
	va_end(args);

	puts(buf);

	delete[] buf;
}

void ConsoleBuffer::puts(const char *s)
{
	const char *src = s;
	char *dst = _lines[_currentLine];

	while( *src )
	{
		switch( *src )
		{
		case '\0':
			break;
		default:
			if( _currentPos < _lineLength )
			{
				dst[_currentPos++] = *(src++);
			}
			else
			{
				dst[_currentPos] = '\0';
				_currentPos = 0;
				_currentLine = (_currentLine + 1) % _lineCount;
				_currentCount = __min(_lineCount, _currentCount + 1);
				dst = _lines[_currentLine];
			}
			break;
		case '\n':
			++src;
			dst[_currentPos] = '\0';
			_currentPos = 0;
			_currentLine = (_currentLine + 1) % _lineCount;
			_currentCount = __min(_lineCount, _currentCount + 1);
			dst = _lines[_currentLine];
		}
	}
	dst[_currentPos] = '\0';

	if( _logFile )
	{
		fputs(s, _logFile);
		fflush(_logFile);
	}
}


///////////////////////////////////////////////////////////////////////////////
// end of file
