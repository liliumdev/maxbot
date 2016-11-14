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
#define SR33_H_

//-----------------------------------------------------------------------------

#ifndef NETWORK_H_
	#include "Network.h"
#endif

#ifndef _WINDOWS_
	#include <windows.h>
#endif

#ifndef _VECTOR_
	#include <vector>
#endif

#ifndef _MAP_
	#include <map>
#endif

//-----------------------------------------------------------------------------

// Forward declarations
class Client;
struct PendingClient;

// Silkroad proxy object
class SilkroadProxy
{
private:
	// Friends ;)
	friend void Login_OnConnectFunc(tNetworkServer * server, SOCKET s);
	friend void Login_OnReadFunc(tNetworkServer * server, SOCKET s);
	friend void Login_OnWriteFunc(tNetworkServer * server, SOCKET s);
	friend void Login_OnCloseFunc(tNetworkServer * server, SOCKET s);
	friend void World_OnConnectFunc(tNetworkServer * server, SOCKET s);
	friend void World_OnReadFunc(tNetworkServer * server, SOCKET s);
	friend void World_OnWriteFunc(tNetworkServer * server, SOCKET s);
	friend void World_OnCloseFunc(tNetworkServer * server, SOCKET s);

	// Tracking of proxy objects
	static std::map<tNetworkServer *, SilkroadProxy *> loginServerMap;
	static CRITICAL_SECTION loginProxyCS;

	// Tracking of proxy objects
	static std::map<tNetworkServer *, SilkroadProxy *> worldServerMap;
	static CRITICAL_SECTION worldProxyCS;

private:
	// The server object
	tNetworkServer loginServer;
	tNetworkServer worldServer;

	// Ports to run on
	WORD loginPort;
	WORD worldPort;

	// Queue of pending clients
	std::vector<PendingClient *> pendingClientList;
	CRITICAL_SECTION pendingQueueCS;

	// Vector of all login clients
	std::vector<Client*> loginClientList;
	CRITICAL_SECTION loginListCS;

	// Vector of all world clients
	std::vector<Client*> worldClientList;
	CRITICAL_SECTION worldListCS;

private:
	// When a socket is connecting
	void Login_OnConnect(SOCKET s);

	// When a socket is read to have data read from it
	void Login_OnRead(SOCKET s);

	// When a client is ready to have data sent to it
	void Login_OnWrite(SOCKET s);

	// When a client closes the connection
	void Login_OnClose(SOCKET s);

	// When a socket is connecting
	void World_OnConnect(SOCKET s);

	// When a socket is read to have data read from it
	void World_OnRead(SOCKET s);

	// When a client is ready to have data sent to it
	void World_OnWrite(SOCKET s);

	// When a client closes the connection
	void World_OnClose(SOCKET s);

	// Returns a client waiting to be accepted
	PendingClient * GetNextPendingClient();

	// Adds a client to be maintained
	Client * AddLoginClient(SOCKET local, SOCKET remote);

	// Adds a client to be maintained
	Client * AddWorldClient(SOCKET local, SOCKET remote);

public:
	// This function needs to be called to setup the critical sections for the class
	static void Initialize();

	// This function needs to be called to destroy the critical sections of the class
	static void Deinitialize();

public:
	// Adds a client to be maintained - not for public api objects
	void AddPendingClient(DWORD id, DWORD sid, const char * ip, WORD port);

public:
	// Default ctor
	SilkroadProxy();

	// Default dtor
	~SilkroadProxy();

	// Create the proxy
	bool Create(WORD loginport, WORD worldport);

	// Destroy the proxy
	void Destroy();

	// Helper functions to remove clients
	void RemoveAllWorldClients();

	// Helper functions to remove clients
	void RemoveAllLoginClients();
};

//-----------------------------------------------------------------------------

// One time setup and cleanup functions for the user data
extern void InitializeUserData();
extern void DeinitializeUserData();

// Error handling function
extern void OnError(const char * error);

// This function will use a detour dll to hook the connect api functions for us
void InstallWindowsHook(bool mode);

//-----------------------------------------------------------------------------

#endif
