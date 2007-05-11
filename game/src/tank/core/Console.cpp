// Console.cpp

#include "stdafx.h"
#include "Console.h"

///////////////////////////////////////////////////////////////////////////////

ConsoleBuffer::ConsoleBuffer(size_t lineLength, size_t maxLines)
{
	_lineLength = lineLength;
	_maxLines   = maxLines;
	_line       = (char *) malloc(_lineLength+1);
}

ConsoleBuffer::~ConsoleBuffer()
{
	for( size_t i = 0; i < _lines.size(); ++i )
	{
		free( _lines[i] );
	}
	free(_line);
}

void ConsoleBuffer::Fill(const ConsoleBuffer *src)
{
}

size_t ConsoleBuffer::GetLineCount() const
{
	return _lines.size();
}

const char* ConsoleBuffer::GetLine(size_t index) const
{
	return _lines[index];
}

void ConsoleBuffer::print(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int size = _vscprintf(fmt, args) + 1; // check how much space is needed

	char *buf = (char *) malloc(size);
	vsprintf(buf, fmt, args);
    va_end(args);

	char *src = buf;
	while( *src )
	{
		// fill current line until src buffer ends or '\n'
		size_t pos = 0;
		for(;;)
		{
			if( '\0' == (_line[pos] = *src) )
			{
				break;
			}
			++src;
			if( '\n' == _line[pos] )
			{
				_line[pos] = '\0';
				break;
			}
			++pos;
			if( pos == _lineLength )
			{
				_line[pos] = '\0';
				break;
			}
		}

		// send current line to console buffer
		_lines.push_back(_line);

		// retrieve new line buffer
		if( _lines.size() < _maxLines )
		{
			_line = (char *) malloc(_lineLength+1); // alloc new line
		}
		else
		{
			_line = _lines.front(); // get line back from buffer
			_lines.pop_front();
		}
	}

	free(buf);
}

///////////////////////////////////////////////////////////////////////////////
// end of file