// ConsoleBuffer.cpp

#include "stdafx.h"
#include "ConsoleBuffer.h"


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

ConsoleBuffer::StreamHelper& ConsoleBuffer::StreamHelper::operator << (const wchar_t *sz)
{
#ifdef _UNICODE
	m_buf << (sz ? sz : L"<null>");
#else
	if( sz )
	{
		char buf[4096];
		size_t converted;
		wcstombs_s(&converted, buf, sz, _TRUNCATE);
		m_buf << buf;
	}
	else
	{
		m_buf << "<null>";
	}
#endif
	return *this;
}

ConsoleBuffer::StreamHelper& ConsoleBuffer::StreamHelper::operator << (wchar_t *sz)
{
#ifdef _UNICODE
	m_buf << (sz ? sz : L"<null>");
#else
	if( sz )
	{
		char buf[4096];
		size_t converted;
		wcstombs_s(&converted, buf, sz, _TRUNCATE);
		m_buf << buf;
	}
	else
	{
		m_buf << "<null>";
	}
#endif
	return *this;
}


///////////////////////////////////////////////////////////////////////////////

ConsoleBuffer::ConsoleBuffer(size_t lineLength, size_t maxLines)
  : _buffer(maxLines * lineLength + maxLines, 0)
  , _times(maxLines, 0)
  , _sev(maxLines, 0)
  , _currentPos(0)
  , _currentLine(0)
  , _currentCount(1)
  , _lineLength(lineLength)
  , _lineCount(maxLines)
#ifdef _DEBUG
  ,_locked(0)
#endif
  , _log(NULL)
{
	InitializeCriticalSection(&_cs);
}

ConsoleBuffer::~ConsoleBuffer()
{
	assert(!_locked);
	if( _log )
	{
		_log->Release();
	}
	DeleteCriticalSection(&_cs);
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

const TCHAR* ConsoleBuffer::GetLine(size_t index) const
{
	Lock();
	assert(index < _currentCount);
	const TCHAR *result = GET_LINE((_lineCount + index + _currentLine - _currentCount) % _lineCount);
	Unlock();
	return result;
}

DWORD ConsoleBuffer::GetTimeStamp(size_t index) const
{
	Lock();
	assert(index < _currentCount);
	DWORD result = _times[(_lineCount + index + _currentLine - _currentCount) % _lineCount];
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

void ConsoleBuffer::WriteLine(int severity, const string_t &s)
{
	Lock();

	const TCHAR *src = s.c_str();
	DWORD time = GetTickCount();

	if( _log )
	{
		_log->WriteLine(severity, s);
	}

	TCHAR *dst = GET_LINE(_currentLine);

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
	int size = _vscprintf(fmt, args) + 1; // check how much space is needed

	std::vector<char> buf(size);
	vsprintf(&buf[0], fmt, args);
	va_end(args);

	WriteLine(severity, &buf[0]);
}

void ConsoleBuffer::Lock() const
{
	EnterCriticalSection(&_cs);
#ifdef _DEBUG
	++_locked;
#endif
}

void ConsoleBuffer::Unlock() const
{
#ifdef _DEBUG
	assert(_locked);
	--_locked;
#endif
	LeaveCriticalSection(&_cs);
}


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI
// end of file
