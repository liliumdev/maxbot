/******************************************************************************
Copyright (c) 2008 Drew Benton

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
******************************************************************************/

#ifndef SR33_H_
	#include "SR33.h"
#endif

#ifndef _ALGORITHM_
	#include <algorithm>
#endif

#ifndef CLIENT_H_
	#include "Client.h"
#endif

//-----------------------------------------------------------------------------

// Returns a hostent object with the information about a host
hostent * GetHost(const char * host);

//-----------------------------------------------------------------------------

// Tracking of proxy objects
std::map<tNetworkServer *, SilkroadProxy *> SilkroadProxy::loginServerMap;
std::map<tNetworkServer *, SilkroadProxy *> SilkroadProxy::worldServerMap;

// Critical sections for accessing the maps
CRITICAL_SECTION SilkroadProxy::loginProxyCS;
CRITICAL_SECTION SilkroadProxy::worldProxyCS;

//-----------------------------------------------------------------------------

// This function needs to be called to setup the critical sections for the class
void SilkroadProxy::Initialize()
{
	InitializeCriticalSection(&loginProxyCS);
	InitializeCriticalSection(&worldProxyCS);
	InitializeUserData();
}

// This function needs to be called to destroy the critical sections of the class
void SilkroadProxy::Deinitialize()
{
	DeinitializeUserData();
	DeleteCriticalSection(&loginProxyCS);
	DeleteCriticalSection(&worldProxyCS);
}

// Called when a client connects to the login server
void Login_OnConnectFunc(tNetworkServer * server, SOCKET s)
{
	EnterCriticalSection(&SilkroadProxy::loginProxyCS);
		SilkroadProxy::loginServerMap[ server ]->Login_OnConnect(s);
	LeaveCriticalSection(&SilkroadProxy::loginProxyCS);
}

//-----------------------------------------------------------------------------

// When a socket is read to have data read from it
void Login_OnReadFunc(tNetworkServer * server, SOCKET s)
{
	EnterCriticalSection(&SilkroadProxy::loginProxyCS);
		SilkroadProxy::loginServerMap[ server ]->Login_OnRead(s);
	LeaveCriticalSection(&SilkroadProxy::loginProxyCS);
}

//-----------------------------------------------------------------------------

// When a client is ready to have data sent to it
void Login_OnWriteFunc(tNetworkServer * server, SOCKET s)
{
	EnterCriticalSection(&SilkroadProxy::loginProxyCS);
		SilkroadProxy::loginServerMap[ server ]->Login_OnWrite(s);
	LeaveCriticalSection(&SilkroadProxy::loginProxyCS);
}

//-----------------------------------------------------------------------------

// When a client closes the connection
void Login_OnCloseFunc(tNetworkServer * server, SOCKET s)
{
	EnterCriticalSection(&SilkroadProxy::loginProxyCS);
		SilkroadProxy::loginServerMap[ server ]->Login_OnClose(s);
	LeaveCriticalSection(&SilkroadProxy::loginProxyCS);
}

//-----------------------------------------------------------------------------

// Called when a client connects to the world server
void World_OnConnectFunc(tNetworkServer * server, SOCKET s)
{
	EnterCriticalSection(&SilkroadProxy::worldProxyCS);
		SilkroadProxy::worldServerMap[ server ]->World_OnConnect(s);
	LeaveCriticalSection(&SilkroadProxy::worldProxyCS);
}

//-----------------------------------------------------------------------------

// When a socket is read to have data read from it
void World_OnReadFunc(tNetworkServer * server, SOCKET s)
{
	EnterCriticalSection(&SilkroadProxy::worldProxyCS);
		SilkroadProxy::worldServerMap[ server ]->World_OnRead(s);
	LeaveCriticalSection(&SilkroadProxy::worldProxyCS);
}

//-----------------------------------------------------------------------------

// When a client is ready to have data sent to it
void World_OnWriteFunc(tNetworkServer * server, SOCKET s)
{
	EnterCriticalSection(&SilkroadProxy::worldProxyCS);
		SilkroadProxy::worldServerMap[ server ]->World_OnWrite(s);
	LeaveCriticalSection(&SilkroadProxy::worldProxyCS);
}

//-----------------------------------------------------------------------------

// When a client closes the connection
void World_OnCloseFunc(tNetworkServer * server, SOCKET s)
{
	EnterCriticalSection(&SilkroadProxy::worldProxyCS);
		SilkroadProxy::worldServerMap[ server ]->World_OnClose(s);
	LeaveCriticalSection(&SilkroadProxy::worldProxyCS);
}

//-----------------------------------------------------------------------------

// Default ctor
SilkroadProxy::SilkroadProxy()
{
	// Create the critical sections
	InitializeCriticalSection(&pendingQueueCS);
	InitializeCriticalSection(&loginListCS);
	InitializeCriticalSection(&worldListCS);

	// Clear ports
	loginPort = 0;
	worldPort = 0;

	// Clear server memory
	memset(&loginServer, 0, sizeof(tNetworkServer));
	memset(&worldServer, 0, sizeof(tNetworkServer));

	// Track this object
	loginServerMap.insert( std::make_pair(&loginServer, this) );
	worldServerMap.insert( std::make_pair(&worldServer, this) );
}

//-----------------------------------------------------------------------------

// Default dtor
SilkroadProxy::~SilkroadProxy()
{
	// Free the critical sections
	DeleteCriticalSection(&pendingQueueCS);
	DeleteCriticalSection(&loginListCS);
	DeleteCriticalSection(&worldListCS);

	// Do not trakc this object anymore
	loginServerMap.erase( &loginServer );
	worldServerMap.erase( &worldServer );
}

//-----------------------------------------------------------------------------

// Create the proxy
bool SilkroadProxy::Create(WORD loginport, WORD worldport)
{
	// Store the ports
	loginPort = loginport;
	worldPort = worldport;

	// Set the login server members
	loginServer.OnConnect = Login_OnConnectFunc;
	loginServer.OnRead = Login_OnReadFunc;
	loginServer.OnWrite = Login_OnWriteFunc;
	loginServer.OnClose = Login_OnCloseFunc;
	loginServer.port = loginPort;

	// Set the world server members
	worldServer.OnConnect = World_OnConnectFunc;
	worldServer.OnRead = World_OnReadFunc;
	worldServer.OnWrite = World_OnWriteFunc;
	worldServer.OnClose = World_OnCloseFunc;
	worldServer.port = worldport;

	// Create the login server
	if(!Network_CreateServer(loginServer))
	{
		return false;
	}

	// Create the world server
	if(!Network_CreateServer(worldServer))
	{
		Network_DestroyServer(loginServer);
		return false;
	}

	// Success
	return true;
}

//-----------------------------------------------------------------------------

// Destroy the proxy
void SilkroadProxy::Destroy()
{
	// Helper functions to remove clients
	RemoveAllLoginClients();
	RemoveAllWorldClients();

	// Destroy the servers
	Network_DestroyServer(loginServer);
	Network_DestroyServer(worldServer);
}

//-----------------------------------------------------------------------------

// Returns a client waiting to be accepted
PendingClient * SilkroadProxy::GetNextPendingClient()
{
	// Client
	PendingClient * client = 0;

	// Multithreaded access
	EnterCriticalSection(&pendingQueueCS);

	// Make sure we have a client to get
	if(pendingClientList.size())
	{
		// Save the entry
		client = pendingClientList[0];

		// Remove the entry
		pendingClientList.erase(pendingClientList.begin());
	}

	// Multithreaded access
	LeaveCriticalSection(&pendingQueueCS);

	// Return the client
	return client;
}

//-----------------------------------------------------------------------------

// Adds a client to be maintained
void SilkroadProxy::AddPendingClient(DWORD id, DWORD sid, const char * ip, WORD port)
{
	// Allocate memory for a new client
	PendingClient * client = new PendingClient;

	// Clear memory
	memset(client, 0, sizeof(PendingClient));

	// Save the data
	client->id = id;
	client->sid = sid;
	_snprintf(client->worldIp, 31, "%s", ip);
	client->worldPort = port;

	// Multithreaded access
	EnterCriticalSection(&pendingQueueCS);

		// Save the pointer
		pendingClientList.push_back(client);

	// Multithreaded access
	LeaveCriticalSection(&pendingQueueCS);
}

//-----------------------------------------------------------------------------

// This function will use a detour dll to hook the connect api functions for us
// NOTE: It will only hook processes that handle messages via GetMessage or PeekMessage
// To get Softmod working with this, I just added a MessageBox call in the loader to
// make the hook propogate.
void InstallWindowsHook(bool mode)
{
	static HHOOK hHook = 0;
	static HMODULE hDll = 0;
	char curdir[256] = {0};
	GetCurrentDirectory(255, curdir);
	strcat(curdir, "\\");
	if(mode != 0)
	{
		HOOKPROC proc = 0;
		strcat(curdir, "Detour.dll");
		hDll = LoadLibrary(curdir);
		if(hDll == NULL)
		{
			MessageBox(0, "Could not load the Detour DLL.", curdir, MB_ICONERROR);
			return;
		}
		proc = (HOOKPROC)GetProcAddress(hDll, "_CBTProc@12");
		if(proc == NULL)
		{
			FreeLibrary(hDll);
			MessageBox(0, "Could not load the _CBTProc@12 function from the DLL.", "Fatal Error", MB_ICONERROR);
			return;
		}
	    hHook = SetWindowsHookEx(WH_CBT, proc, hDll, 0);
		if(hHook == 0)
		{
			FreeLibrary(hDll);
			MessageBox(0, "Could not set the WindowsHook.", "Fatal Error", MB_ICONERROR);
			return;
		}
	}
	else
	{
		UnhookWindowsHookEx(hHook);
		if(hDll) FreeLibrary(hDll);
		hHook = 0;
		hDll = 0;
	}
}

//-----------------------------------------------------------------------------

// Returns a hostent object with the information about a host
hostent * GetHost(const char * host)
{
	hostent * hp = 0;
	if(inet_addr(host) == INADDR_NONE)
	{
		 hp = gethostbyname(host);
	}
	else
	{
		unsigned long addr = 0;
		addr = inet_addr(host);
		hp = gethostbyaddr((char*)&addr, sizeof(addr), AF_INET);
	}
	return hp;
}

//-----------------------------------------------------------------------------
