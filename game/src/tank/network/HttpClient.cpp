// HttpClient.cpp

#include "stdafx.h"
#include "HttpClient.h"

#include "core/Application.h"
#include "core/debug.h"

///////////////////////////////////////////////////////////////////////////////

static void UrlParamEncode(std::string &result, const HttpClient::Param &param)
{
	result.clear();
	for( HttpClient::Param::const_iterator it = param.begin(); it != param.end(); ++it )
	{
		if( !result.empty() )
		{
			result += "&";
		}
		// TODO: escape arguments
		result += it->first + "=" + it->second;
	}
}

static int ParseHttpReply(std::string &resultBody, HttpClient::Param &resultHeaders, const std::string &reply)
{
	std::istringstream in(reply);

	const int stateBegin         = 0;  // [ :] -> error; '\n' -> goto stateBody; else goto stateKey
	const int stateKey           = 1;  // [ ] -> goto stateDelim0; [:] -> goto stateDelim1; '\n' -> error; else accumulate
	const int stateDelim0        = 2;  // [ ] -> goto stateDelim0; [:] -> goto stateDelim1; else error
	const int stateDelim1        = 3;  // [ ] -> goto stateDelim1; else goto stateValue
	const int stateValue         = 4;  // '\n' -> goto stateBegin; else accumulate
	const int stateBody          = 5;  // accumulate
	const int stateEnd           = 6;

	int status = 0;
	std::string httpver, reason;
	std::string key, value;

	in >> httpver >> status;
	std::getline(in, reason); // read the rest of the line
	if( in.fail() )
	{
		return 0;
	}

	int state = stateBegin;
	std::noskipws(in);
	char c;
	while( in >> c )
	{
		if( '\r' == c ) 
			continue;

		switch( state )
		{
		case stateBegin:
			switch( c )
			{
				case '\n':
					state = stateBody;
					break;
				case ' ':
				case ':':
					return 0; // error
				default:
					in.unget(); // this it the beginning of key
					state = stateKey;
			}
			break;
		case stateKey:
			switch( c )
			{
				case ' ':  state = stateDelim0; break;
				case ':':  state = stateDelim1; break;
				case '\n': return 0;      // error
				default:   key += c;  // accumulate
			}
			break;
		case stateDelim0:
			switch( c )
			{
				case ' ': break; // keep stateDelim0
				case ':': state = stateDelim1; break;
				default:  return 0; // error
			}
			break;
		case stateDelim1:
			switch( c )
			{
				case ' ': break; // keep stateDelim1
				default:
					in.unget(); // this it the beginning of value
					state = stateValue;
			}
			break;
		case stateValue:
			switch( c )
			{
				case '\n':
					state = stateBegin;
					resultHeaders[key] = value; // insert new key-value pair
					key.clear();
					value.clear();
					break;
				default:
					value += c; // accumulate
			}
			break;
		case stateBody:
			resultBody += c; // accumulate
			break;
		} // switch( state )
	} // while( !in.eof() )

	return status;
}

///////////////////////////////////////////////////////////////////////////////

HttpClient::HttpClient()
  : _connected(false)
{

}

HttpClient::~HttpClient()
{
	if( INVALID_SOCKET != _socket )
	{
		_socket.Close();
	}
}

void HttpClient::Get(const std::string &url, const Param &param)
{
	assert(INVALID_SOCKET == _socket);

	g_app->InitNetwork();


	// extract protocol if specified
	std::string::size_type delim = url.find("://");
	std::string protocol = std::string::npos != delim ? url.substr(0, delim) : "";
	std::string fullpath = std::string::npos != delim ? url.substr(delim + 3) : url;

	// extract host name and request
	delim = fullpath.find('/');
	_host = fullpath.substr(0, delim);
	_query = std::string::npos != delim ? fullpath.substr(delim) : "/";


	std::string body;
	UrlParamEncode(body, param);


	//
	// get ip address
	//

	sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port   = htons(80); // TODO: get from address

	// try to convert string to ip address
	addr.sin_addr.s_addr = inet_addr(_host.c_str());

	if( addr.sin_addr.s_addr == INADDR_NONE )
	{
		// try to resolve string as host name
		hostent* he = gethostbyname(_host.c_str());
		if( NULL == he )
		{
			TRACE("http client: ERROR - Unable to resolve IP-address (%u)", WSAGetLastError());
			assert(eventResult);
			INVOKE(eventResult) (WSAGetLastError(), "Unable to resolve IP-address", NULL);
			return;
		}
		addr.sin_addr.s_addr = *((u_long*)he->h_addr_list[0]);
	}


	//
	// connect
	//

	_incoming.clear();
	_connected = false;
	_socket.Attach(socket(AF_INET, SOCK_STREAM, 0));
	if( INVALID_SOCKET == _socket )
	{
		int err = WSAGetLastError();
		TRACE("http client: ERROR - Unable to create socket (%u)", WSAGetLastError());
		assert(eventResult);
		INVOKE(eventResult) (WSAGetLastError(), "Unable to create socket", NULL);
		return;
	}

	if( _socket.SetEvents(FD_READ|FD_WRITE|FD_CONNECT|FD_CLOSE) )
	{
		_socket.Close();
		TRACE("http client: ERROR - Unable to select event (%u)", WSAGetLastError());
		assert(eventResult);
		INVOKE(eventResult) (WSAGetLastError(), "Unable to select event", NULL);
	}

	_socket.SetCallback(CreateDelegate(&HttpClient::OnSocketEvent, this));


	TRACE("http: connecting to %s", inet_ntoa(addr.sin_addr));


	if( !connect(_socket, (sockaddr *) &addr, sizeof(sockaddr_in)) )
	{
		_socket.Close();
		TRACE("cl: ERROR - connect call failed!");
		assert(eventResult);
		INVOKE(eventResult) (WSAGetLastError(), "Could not connect", NULL);
		return;
	}
	if( WSAEWOULDBLOCK != WSAGetLastError() )
	{
		_socket.Close();
		TRACE("cl: error %d; WSAEWOULDBLOCK expected!", WSAGetLastError());
		assert(eventResult);
		INVOKE(eventResult) (WSAGetLastError(), "Could not connect", NULL);
	}


	//
	// prepare and send http request
	//

	std::stringstream stream;
	stream << "POST " << _query << " HTTP/1.0" << std::endl;
	stream << "Host: " << _host << std::endl;
	stream << "Content-Type: application/x-www-form-urlencoded" << std::endl;
	stream << "Content-Length: " << body.length() << std::endl;
	stream << std::endl;
	stream << body;

	Send(stream.str());
}

void HttpClient::OnSocketEvent()
{
	assert(INVALID_SOCKET != _socket);

	WSANETWORKEVENTS ne = {0};
	if( _socket.EnumNetworkEvents(&ne) )
	{
		_socket.Close();
		TRACE("http: EnumNetworkEvents error 0x%08x", WSAGetLastError());
		assert(eventResult);
		INVOKE(eventResult) (WSAGetLastError(), "EnumNetworkEvents error", NULL);
		return;
	}

	if( ne.lNetworkEvents & FD_CONNECT )
	{
		if( ne.iErrorCode[FD_CONNECT_BIT] )
		{
			_socket.Close();
			TRACE("http: could not connect 0x%08x", ne.iErrorCode[FD_CONNECT_BIT]);
			assert(eventResult);
			INVOKE(eventResult) (ne.iErrorCode[FD_CONNECT_BIT], "could not connect", NULL);
		}
		_connected = true;
		TRACE("http: connected");
	}

	if( ne.lNetworkEvents & FD_READ )
	{
		if( ne.iErrorCode[FD_READ_BIT] )
		{
			_socket.Close();
			TRACE("http: read error 0x%08x", ne.iErrorCode[FD_READ_BIT]);
			assert(eventResult);
			INVOKE(eventResult) (ne.iErrorCode[FD_READ_BIT], "recv error", NULL);
			return;
		}

		char buf[1024];
		int result = recv(_socket, buf, sizeof(buf), 0);

		if( result < 0 )
		{
			_socket.Close();
			TRACE("http: unexpected recv error 0x%08x", WSAGetLastError());
			assert(eventResult);
			INVOKE(eventResult) (WSAGetLastError(), "unexpected recv error", NULL);
			return;
		}
		else if( 0 == result )
		{
			// connection was gracefully closed
			_socket.Close();
			TRACE("http: connection closed by server");
			TRACE("http reply:\n%s", _incoming.c_str());
			assert(eventResult);
			INVOKE(eventResult) (0, _incoming, NULL);
			return;
		}
		else
		{
			_incoming.insert(_incoming.end(), buf, buf + result);
		}
	}

	if( ne.lNetworkEvents & FD_WRITE )
	{
		if( ne.iErrorCode[FD_WRITE_BIT] )
		{
			_socket.Close();
			TRACE("http: write error 0x%08x", ne.iErrorCode[FD_WRITE_BIT]);
			assert(eventResult);
			INVOKE(eventResult) (ne.iErrorCode[FD_WRITE_BIT], "write error", NULL);
			return;
		}

		if( _outgoing.empty() )
		{
			TRACE("http: nothing to send");
			return;
		}

		size_t sent = 0;
		do
		{
			int result = send(_socket, &_outgoing.front() + sent, _outgoing.size() - sent, 0);
			if( SOCKET_ERROR == result )
			{
				if( WSAEWOULDBLOCK != WSAGetLastError() )
				{
					_socket.Close();
					TRACE("http: send error %u", WSAGetLastError());
					assert(eventResult);
					INVOKE(eventResult) (WSAGetLastError(), "send error", NULL);
					return;
				}
				break;
			}
			else
			{
				assert(result > 0);
				sent += result;
			}
		} while( sent < _outgoing.size() );

		// remove sent bytes
		assert(sent <= _outgoing.size());
		_outgoing.erase(_outgoing.begin(), _outgoing.begin() + sent);
	}

	if( ne.lNetworkEvents & FD_CLOSE )
	{
		_socket.Close();
		assert(eventResult);
		if( ne.iErrorCode[FD_CLOSE_BIT] )
		{
			TRACE("http: connection error 0x%08x", ne.iErrorCode[FD_CLOSE_BIT]);
			INVOKE(eventResult) (ne.iErrorCode[FD_CLOSE_BIT], "connection error", NULL);
		}
		else
		{
			std::string body;
			Param param;
			int status = ParseHttpReply(body, param, _incoming);
			INVOKE(eventResult) (status, body, &param);
		}
	}
}

void HttpClient::Send(const std::string &msg)
{
	assert(INVALID_SOCKET != _socket);
	assert(_outgoing.empty());
	
	// try sending immediately if connected
	if( _connected )
	{
		size_t sent = 0;
		do
		{
			int result = send(_socket, msg.c_str() + sent, msg.length() - sent, 0);
			if( SOCKET_ERROR == result )
			{
				// schedule delayed sending rest of data
				if( WSAEWOULDBLOCK != WSAGetLastError() )
				{
					_socket.Close();
					TRACE("http: send error %u", WSAGetLastError());
					assert(eventResult);
					INVOKE(eventResult) (WSAGetLastError(), "send error", NULL);
					return;
				}
				_outgoing.insert(_outgoing.end(), msg.c_str() + sent, msg.c_str() + msg.length());
				break;
			}
			else
			{
				assert(result > 0);
				sent += result;
			}
		} while( sent < msg.length() );
	}
	else
	{
		_outgoing.insert(_outgoing.end(), msg.c_str(), msg.c_str() + msg.length());
	}
}

// end of file
