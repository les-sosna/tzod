#pragma once
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace UI
{

struct IConsoleLog
{
	virtual void WriteLine(int severity, const std::string &str) = 0;
	virtual void Release() = 0;
};

class ConsoleBuffer
{
	class StreamHelper
	{
	public:
		StreamHelper(ConsoleBuffer &con, int severity)
            : m_con(&con)
            , m_severity(severity)
		{
		}

		StreamHelper(StreamHelper &&other)
			: m_con(other.m_con)
			, m_severity(other.m_severity)
			, m_buf(std::move(other.m_buf))
		{
			other.m_con = nullptr;
		}

		~StreamHelper()
		{
			if( m_con && !m_buf.str().empty() )
				m_con->WriteLine(m_severity, m_buf.str().c_str());
		}

		StreamHelper& operator << (const char *sz);
		StreamHelper& operator << (char *sz);

		template <class T>
		StreamHelper& operator << (const T &arg)
		{
			m_buf << arg;
			return *this;
		}

	private:
		ConsoleBuffer *m_con;
		int m_severity;
		std::ostringstream m_buf;
	};

public:
	ConsoleBuffer(size_t lineLength, size_t _maxLines);
	~ConsoleBuffer();

    typedef std::chrono::high_resolution_clock ClockType;

	void SetLog(IConsoleLog *pLog);

	size_t GetLineCount() const;
	const char* GetLine(size_t index) const;
    ClockType::time_point GetTimeStamp(size_t index) const;
	unsigned int GetSeverity(size_t index) const;

	void Lock() const;
	void Unlock() const;

	void WriteLine(int severity, const std::string &str);
	void Printf(int severity, const char *fmt, ...);

	StreamHelper Format(int severity)
	{
		return StreamHelper(*this, severity);
	}

private:
	IConsoleLog *_log;

	std::vector<char>  _buffer;
	std::vector<ClockType::time_point>  _times;          // time stumps of lines
	std::vector<unsigned int> _sev;    // severity of lines

	size_t  _currentPos;   // caret position in the current line
	size_t  _currentLine;  // current line number
	size_t  _currentCount; // current number of lines in the buffer

	size_t _lineLength; // including terminating '\0'
	size_t _lineCount;  // maximum number of lines in the buffer

	mutable std::recursive_mutex _cs;
#ifndef NDEBUG
	mutable int  _locked;
#endif
};

} // namespace UI
