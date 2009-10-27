// Console.h

#pragma once

#include "Base.h"


namespace UI
{

struct IConsoleLog
{
	virtual void WriteLine(int severity, const string_t &str) = 0;
	virtual void Release() = 0;
};

class ConsoleBuffer
{
	IConsoleLog *_log;

	std::vector<TCHAR>  _buffer;
	std::vector<DWORD>  _times;        // time stumps of lines
	std::vector<unsigned int> _sev;    // severity of lines

	size_t  _currentPos;   // caret position in the current line
	size_t  _currentLine;  // current line number
	size_t  _currentCount; // current number of lines in the buffer

	size_t _lineLength; // including terminating '\0'
	size_t _lineCount;  // maximum number of lines in the buffer

	mutable CRITICAL_SECTION _cs;
#ifdef _DEBUG
	mutable int  _locked;
#endif


	class StreamHelper
	{
	public:
		StreamHelper(ConsoleBuffer *con, int severity)
			: m_con(con)
			, m_severity(severity)
		{
		}

		StreamHelper(const StreamHelper &other)
			: m_con(other.m_con)
			, m_severity(other.m_severity)
		{
		}

		~StreamHelper()
		{
			if( m_con && !m_buf.str().empty() )
				m_con->WriteLine(m_severity, m_buf.str().c_str());
		}

		StreamHelper& operator << (const char *sz);
		StreamHelper& operator << (char *sz);
		StreamHelper& operator << (const wchar_t *sz);
		StreamHelper& operator << (wchar_t *sz);

		template <class T>
		StreamHelper& operator << (const T &arg)
		{
			m_buf << arg;
			return *this;
		}

	private:
		int m_severity;
		ConsoleBuffer *m_con;
		ostrstream_t m_buf;
	};

public:
	ConsoleBuffer(size_t lineLength, size_t _maxLines);
	~ConsoleBuffer();

	void SetLog(IConsoleLog *pLog);

	size_t GetLineCount() const;
	const TCHAR* GetLine(size_t index) const;
	DWORD GetTimeStamp(size_t index) const;
	unsigned int GetSeverity(size_t index) const;

	void Lock() const;
	void Unlock() const;

	void WriteLine(int severity, const string_t &str);
	void Printf(int severity, const char *fmt, ...);

	StreamHelper Format(int severity)
	{
		return StreamHelper(this, severity);
	}
};

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI
// end of file
