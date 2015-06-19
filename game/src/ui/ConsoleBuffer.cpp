// ConsoleBuffer.cpp

#include "inc/ui/ConsoleBuffer.h"
#include <cassert>
#include <algorithm>
#include <cstdarg>

#define GET_LINE(x) (&_buffer[(_lineLength + 1) * (x)])

namespace UI
{

///////////////////////////////////////////////////////////////////////////////

ConsoleBuffer::StreamHelper& ConsoleBuffer::StreamHelper::operator << (const char *sz)
{
	m_buf << (sz ? sz : "<null>");
	return *this;
}

ConsoleBuffer::StreamHelper& ConsoleBuffer::StreamHelper::operator << (char *sz)
{
	m_buf << (sz ? sz : "<null>");
	return *this;
}

///////////////////////////////////////////////////////////////////////////////

ConsoleBuffer::ConsoleBuffer(size_t lineLength, size_t maxLines)
  : _log(NULL)
  , _buffer(maxLines * lineLength + maxLines, 0)
  , _times(maxLines)
  , _sev(maxLines)
  , _currentPos(0)
  , _currentLine(0)
  , _currentCount(1)
#ifdef _DEBUG
  ,_locked(0)
#endif
    , _lineLength(lineLength)
    , _lineCount(maxLines)
{
}

ConsoleBuffer::~ConsoleBuffer()
{
	assert(!_locked);
	if( _log )
	{
		_log->Release();
	}
}

void ConsoleBuffer::SetLog(IConsoleLog *pLog)
{
	if( _log )
	{
		_log->Release();
	}
	_log = pLog;
}

size_t ConsoleBuffer::GetLineCount() const
{
	Lock();
	size_t result = _currentCount;
	Unlock();
	return result;
}

const char* ConsoleBuffer::GetLine(size_t index) const
{
	Lock();
	assert(index < _currentCount);
	const char *result = GET_LINE((_lineCount + index + _currentLine - _currentCount) % _lineCount);
	Unlock();
	return result;
}

ConsoleBuffer::ClockType::time_point ConsoleBuffer::GetTimeStamp(size_t index) const
{
	Lock();
	assert(index < _currentCount);
	ClockType::time_point result = _times[(_lineCount + index + _currentLine - _currentCount) % _lineCount];
	Unlock();
	return result;
}

unsigned int ConsoleBuffer::GetSeverity(size_t index) const
{
	Lock();
	assert(index < _currentCount);
	unsigned int result = _sev[(_lineCount + index + _currentLine - _currentCount) % _lineCount];
	Unlock();
	return result;
}

void ConsoleBuffer::WriteLine(int severity, const std::string &s)
{
	Lock();

	const char *src = s.c_str();
	ClockType::time_point time = ClockType::now();

	if( _log )
	{
		_log->WriteLine(severity, s);
	}

	char *dst = GET_LINE(_currentLine);

	while( *src )
	{
		switch( *src )
		{
		case '\n':
			++src;
			dst[_currentPos] = '\0';
			_currentPos = 0;
			_currentLine = (_currentLine + 1) % _lineCount;
			_currentCount = std::min(_lineCount, _currentCount + 1);
			dst = GET_LINE(_currentLine);
			break;
		default:
			_times[_currentLine] = time;
			_sev[_currentLine] = severity;
			if( _currentPos < _lineLength )
			{
				dst[_currentPos++] = *(src++);
			}
			else
			{
				dst[_currentPos] = '\0';
				_currentPos = 0;
				_currentLine = (_currentLine + 1) % _lineCount;
				_currentCount = std::min(_lineCount, _currentCount + 1);
				dst = GET_LINE(_currentLine);
			}
			break;
		}
	}
	dst[_currentPos] = '\0';
	_times[_currentLine] = time;
	_sev[_currentLine] = severity;

	_currentPos = 0;
	_currentLine = (_currentLine + 1) % _lineCount;
	_currentCount = std::min(_lineCount, _currentCount + 1);


	Unlock();
}

void ConsoleBuffer::Printf(int severity, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char buf[4096];
	vsnprintf(buf, 4096, fmt, args);
	va_end(args);

	WriteLine(severity, &buf[0]);
}

void ConsoleBuffer::Lock() const
{
    _cs.lock();
#ifndef NDEBUG
	++_locked;
#endif
}

void ConsoleBuffer::Unlock() const
{
#ifndef NDEBUG
	assert(_locked);
	--_locked;
#endif
	_cs.unlock();
}


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI
// end of file
