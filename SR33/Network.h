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

#ifndef NETWORK_H_
#define NETWORK_H_

//-----------------------------------------------------------------------------

#ifndef _WINSOCK2API_
	#include <winsock2.h>
#endif

#ifndef _WS2TCPIP_H_
	#include <ws2tcpip.h>
#endif

#ifndef _WINDOWS_
	#include <windows.h>
#endif

//-----------------------------------------------------------------------------

// Server object
struct tNetworkServer
{
	// Listening socket for the server
	SOCKET s;

	// Port of the server
	WORD port;

	// Handle to the thread
	HANDLE hAcceptThread;

	// Thread ID
	DWORD hAcceptThreadId;

	// Handle to the thread
	HANDLE hNetworkThread;

	// Thread ID
	DWORD hNetworkThreadId;

	// Network window for messages
	HWND hNetworkWnd;

	// Exit flag for the server
	BOOL bExit;

	// Temp flag for syncing threads
	BOOL bTmpVar;

	// Temp event to syncing threads
	HANDLE hTmpEvent;

	// Function called when a client has connected to the server
	void (*OnConnect)(tNetworkServer *, SOCKET);

	// Function called when a socket is ready for reading
	void (*OnRead)(tNetworkServer *, SOCKET);

	// Function called when a socket is ready for writing
	void (*OnWrite)(tNetworkServer *, SOCKET);

	// Function called when a socket was closed
	void (*OnClose)(tNetworkServer *, SOCKET);
};

//-----------------------------------------------------------------------------

// The error logging function
typedef void (*OnErrorFunction)(CONST CHAR * error);

// Initialize the core of the proxy
BOOL Network_InitializeCore(OnErrorFunction function);

// Initialize the core of the proxy
VOID Network_DeinitializeCore();

// Creates a server on the specific port and returns the socket
BOOL Network_CreateServer(tNetworkServer & server);

// Destroys the server
VOID Network_DestroyServer(tNetworkServer & server);

//-----------------------------------------------------------------------------

#endif
