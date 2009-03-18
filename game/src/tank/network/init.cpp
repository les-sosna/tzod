// init.cpp

#include "stdafx.h"
#include "init.h"

#include "core/Application.h"
#include "core/Console.h"
#include "core/debug.h"


NetworkInitHelper::NetworkInitHelper()
{
	WSAData wsad;
	if( WSAStartup(0x0002, &wsad) )
	{
		TRACE("ERROR: Windows sockets init failed\n");
		throw std::runtime_error("Windows sockets init failed");
	}
	TRACE("Windows sockets initialized\n");
}

NetworkInitHelper::~NetworkInitHelper()
{
	WSACleanup();
}

// end of file
