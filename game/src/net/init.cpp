// init.cpp

#include "stdafx.h"
#include "init.h"

#include "core/Application.h"
#include "core/debug.h"


NetworkInitHelper::NetworkInitHelper()
{
	WSAData wsad;
	if( WSAStartup(0x0002, &wsad) )
	{
		throw std::runtime_error("Windows sockets init failed");
	}
	TRACE("Windows sockets initialized");
}

NetworkInitHelper::~NetworkInitHelper()
{
	WSACleanup();
}

// end of file
