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
	#include "Network.h"
#endif

#ifndef _INC_STDIO
	#include <stdio.h>
#endif

//-----------------------------------------------------------------------------

// Link in the winsock library
#pragma comment(lib, "ws2_32.lib")

//-----------------------------------------------------------------------------

// Accept thread structure
struct tAcceptThreadStruct
{
	tNetworkServer * server;
};

// Network thread structure
struct tNetworkThreadStruct
{
	tNetworkServer * server;
};

//-----------------------------------------------------------------------------

// The window class for the network event window
WNDCLASS NetworkWindowClass = {0};

// Error function for the library
OnErrorFunction ErrorFunction = 0;

//-----------------------------------------------------------------------------

// The thread function that accepts incoming connections
DWORD WINAPI AcceptThread(LPVOID lpParam);

// The thread function that handles network traffic for the login server
DWORD WINAPI NetworkThread(LPVOID lpParam);

// This is the network message handling callback function
LRESULT CALLBACK NetworkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Initialize the winsock library
BOOL Network_InitializeWinsock(BYTE major, BYTE minor);

// Deinitialize the winsock library
VOID Network_DeinitializeWinsock();

//-----------------------------------------------------------------------------

// Call this macro with the error string to log where an error took place at
#define OnError(error) DefaultOnErrorFunction(__FILE__, __FUNCTION__, __LINE__, error)

//-----------------------------------------------------------------------------

// The error logging function
void DefaultOnErrorFunction(CONST CHAR * file, CONST CHAR * function, CONST INT line, CONST CHAR * error)
{
	// Error variable
	CHAR lastError[4096] = {0};	

	// Build the error message
	_snprintf(lastError, 4095, "File: %s\nFunction: %s\nLine: %i\nError: %s\n", file, function, line, error);

	// Pass the error message along
	if(ErrorFunction) ErrorFunction(lastError);
}

//-----------------------------------------------------------------------------

// Initialize the core of the network
BOOL Network_InitializeCore(OnErrorFunction function)
{
	// Error variables
	CHAR lastError[4096] = {0};
	DWORD dwError = 0;

	// The error logging function
	ErrorFunction = function;

	// Store the window class information and register it
	NetworkWindowClass.lpfnWndProc = (WNDPROC)NetworkWndProc;
	NetworkWindowClass.hInstance = GetModuleHandle(0);
	NetworkWindowClass.lpszClassName = "NetworkClass";

	// Try and register the class
	if(!RegisterClass(&NetworkWindowClass))
	{
		// Build the error message
		_snprintf(lastError, 4095, "Unhandled error: [%i] generated by CreateWindowEx.", dwError);

		// Call the error function
		OnError(lastError);

		// Signal error
		return FALSE;
	}

	// Return the result of setting up winsock
	return Network_InitializeWinsock(2, 2);
}

//-----------------------------------------------------------------------------

// Initialize the core of the network
void Network_DeinitializeCore()
{
	// Unregister class name
	UnregisterClass("NetworkClass", GetModuleHandle(0));

	// Free winsock library
	Network_DeinitializeWinsock();
}

//-----------------------------------------------------------------------------

// Initialize the winsock library
BOOL Network_InitializeWinsock(BYTE major, BYTE minor)
{
	// Error variables
	CHAR lastError[4096] = {0};
	DWORD dwError = 0;

	// Winsock data
	WSADATA wsaData = {0};

	// Try to start the winsock library
	dwError = WSAStartup(MAKEWORD(major, minor), &wsaData);
	if(dwError != ERROR_SUCCESS)
	{
		// Handle the error
		switch(dwError)
		{
			case WSASYSNOTREADY: _snprintf(lastError, 4095, "The underlying network subsystem is not ready for network communication."); break;
			case WSAVERNOTSUPPORTED: _snprintf(lastError, 4095, "The version of Windows Sockets support requested is not provided by this particular Windows Sockets implementation."); break;
			case WSAEINPROGRESS: _snprintf(lastError, 4095, "A blocking Windows Sockets 1.1 operation is in progress."); break;
			case WSAEPROCLIM: _snprintf(lastError, 4095, "A limit on the number of tasks supported by the Windows Sockets implementation has been reached."); break;
			case WSAEFAULT: _snprintf(lastError, 4095, "The lpWSAData parameter is not a valid pointer."); break;
			default: _snprintf(lastError, 4095, "Unhandled error: [%i] generated by WSAStartup.", dwError);
		}

		// Call the errpr fimctopm
		OnError(lastError);

		// Signal error
		return FALSE;
	}

	// Verify the version of winsock we wish to use is supported
	if(LOBYTE(wsaData.wVersion) != major || HIBYTE(wsaData.wVersion) != minor)
	{
		// Clean up the winsock library
		WSACleanup();

		// Build the error message
		_snprintf(lastError, 4095, "Winsock version %i.%i is not supported on this computer. Only version %i.%i.", major, minor, LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion));

		// Call the error function
		OnError(lastError);

		// Signal error
		return FALSE;
	}

	// Success
	return TRUE;
}

//-----------------------------------------------------------------------------

// Deinitialize the winsock library
VOID Network_DeinitializeWinsock()
{
	// Clean up the winsock library
	WSACleanup();
}

//-----------------------------------------------------------------------------

// Creates a server on the specific port and returns the socket
BOOL Network_CreateServer(tNetworkServer & server)
{
	// The socket for the server
	SOCKET s = INVALID_SOCKET;

	// Error variables
	CHAR lastError[4096] = {0};
	DWORD dwError = 0;

	// Address info
	addrinfo * results = NULL;
	addrinfo hints = {0};
	char portname[8] = {0};

	// Convert and store the desired port
	_snprintf(portname, 7, "%i", server.port);

	// Setup the hints
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Try to get the address info
	dwError = getaddrinfo(NULL, portname, &hints, &results);
	if(dwError != 0)
	{
		// Handle the error
		switch(dwError)
		{
			case WSATRY_AGAIN: _snprintf(lastError, 4095, "A temporary failure in name resolution occurred."); break;
			case WSAEINVAL: _snprintf(lastError, 4095, "An invalid value was provided for the ai_flags member of the hints parameter."); break;
			case WSANO_RECOVERY: _snprintf(lastError, 4095, "A nonrecoverable failure in name resolution occurred."); break;
			case WSAEAFNOSUPPORT: _snprintf(lastError, 4095, "The ai_family member of the hints parameter is not supported."); break;
			case WSA_NOT_ENOUGH_MEMORY: _snprintf(lastError, 4095, "A memory allocation failure occurred."); break;
			case WSAHOST_NOT_FOUND: _snprintf(lastError, 4095, "The name does not resolve for the supplied parameters or the nodename and servname parameters were not provided."); break;
			case WSATYPE_NOT_FOUND: _snprintf(lastError, 4095, "The servname parameter is not supported for the specified ai_socktype member of the hints parameter."); break;
			case WSAESOCKTNOSUPPORT: _snprintf(lastError, 4095, "The ai_socktype member of the hints parameter is not supported."); break;
			default: _snprintf(lastError, 4095, "Unhandled error: [%i] generated by getaddrinfo.", dwError);
		}

		// Call the error function
		OnError(lastError);

		// Failure
		return FALSE;
	}

	// Create the main socket for the server
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if(s == INVALID_SOCKET)
	{
		// Handle the error
		dwError = WSAGetLastError();
		switch(dwError)
		{
			case WSANOTINITIALISED: _snprintf(lastError, 4095, "A successful WSAStartup call must occur before using this function."); break;
			case WSAENETDOWN: _snprintf(lastError, 4095, "The network subsystem has failed."); break;
			case WSAEAFNOSUPPORT: _snprintf(lastError, 4095, "The specified address family is not supported."); break;
			case WSAEINPROGRESS: _snprintf(lastError, 4095, "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function."); break;
			case WSAEMFILE: _snprintf(lastError, 4095, "No more socket descriptors are available."); break;
			case WSAENOBUFS: _snprintf(lastError, 4095, "No buffer space is available. The socket cannot be created."); break;
			case WSAEPROTONOSUPPORT: _snprintf(lastError, 4095, "The specified protocol is not supported."); break;
			case WSAEPROTOTYPE: _snprintf(lastError, 4095, "The specified protocol is the wrong type for this socket."); break;
			case WSAESOCKTNOSUPPORT: _snprintf(lastError, 4095, "The specified socket type is not supported in this address family."); break;
			case WSAEINVAL: _snprintf(lastError, 4095, "This value is true for any of the following conditions.:\n\t* The parameter g specified is not valid.\n\t* The WSAPROTOCOL_INFO structure that lpProtocolInfo points to is incomplete, the contents are invalid or the WSAPROTOCOL_INFO structure has already been used in an earlier duplicate socket operation.\n\t* The values specified for members of the socket triple <af, type, and protocol> are individually supported, but the given combination is not."); break;
			case WSAEFAULT: _snprintf(lastError, 4095, "The lpProtocolInfo parameter is not in a valid part of the process address space."); break;
			default: _snprintf(lastError, 4095, "Unhandled error: [%i] generated by WSASocket.", dwError);
		}

		// Cleanup on error
		freeaddrinfo(results);

		// Call the error function
		OnError(lastError);

		// Failure
		return FALSE;
	}

	// Bind the socket
	dwError = bind(s, (SOCKADDR*)results->ai_addr, (int)results->ai_addrlen);

	// Free the address info since we do not need it anymore
	freeaddrinfo(results);

	// Make sure it was bound
	if(dwError == SOCKET_ERROR)
	{
		// Handle the error
		dwError = WSAGetLastError();
		switch(dwError)
		{
			case WSANOTINITIALISED: _snprintf(lastError, 4095, "A successful WSAStartup call must occur before using this function."); break;
			case WSAENETDOWN: _snprintf(lastError, 4095, "The network subsystem has failed."); break;
			case WSAEACCES: _snprintf(lastError, 4095, "Attempt to connect datagram socket to broadcast address failed because setsockopt option SO_BROADCAST is not enabled."); break;
			case WSAEADDRINUSE: _snprintf(lastError, 4095, "A process on the computer is already bound to the same fully-qualified address and the socket has not been marked to allow address reuse with SO_REUSEADDR. For example, the IP address and port are bound in the af_inet case). (See the SO_REUSEADDR socket option under setsockopt.)\n"); break;
			case WSAEADDRNOTAVAIL: _snprintf(lastError, 4095, "The specified address is not a valid address for this computer."); break;
			case WSAEFAULT: _snprintf(lastError, 4095, "The name or namelen parameter is not a valid part of the user address space, the namelen parameter is too small, the name parameter contains an incorrect address format for the associated address family, or the first two bytes of the memory block specified by name does not match the address family associated with the socket descriptor s."); break;
			case WSAEINPROGRESS: _snprintf(lastError, 4095, "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function."); break;
			case WSAEINVAL: _snprintf(lastError, 4095, "The socket is already bound to an address."); break;
			case WSAENOBUFS: _snprintf(lastError, 4095, "Not enough buffers available, too many connections."); break;
			case WSAENOTSOCK: _snprintf(lastError, 4095, "The descriptor is not a socket."); break;
			default: _snprintf(lastError, 4095, "Unhandled error: [%i] generated by bind.", dwError);
		}

		// Cleanup on error
		closesocket(s);

		// Call the error function
		OnError(lastError);

		// Failure
		return FALSE;
	}

	// Listen for incoming connections
	dwError = listen(s, SOMAXCONN);
	if(dwError == SOCKET_ERROR)
	{
		// Handle the error
		dwError = WSAGetLastError();
		switch(dwError)
		{
			case WSANOTINITIALISED: _snprintf(lastError, 4095, "A successful WSAStartup call must occur before using this function."); break;
			case WSAENETDOWN: _snprintf(lastError, 4095, "The network subsystem has failed."); break;
			case WSAEADDRINUSE: _snprintf(lastError, 4095, "The socket's local address is already in use and the socket was not marked to allow address reuse with SO_REUSEADDR. This error usually occurs during execution of the bind function, but could be delayed until this function if the bind was to a partially wildcard address (involving ADDR_ANY) and if a specific address needs to be committed at the time of this function."); break;
			case WSAEINPROGRESS: _snprintf(lastError, 4095, "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function."); break;
			case WSAEINVAL: _snprintf(lastError, 4095, "The socket has not been bound with bind."); break;
			case WSAEISCONN: _snprintf(lastError, 4095, "The socket is already connected."); break;
			case WSAEMFILE: _snprintf(lastError, 4095, "No more socket descriptors are available."); break;
			case WSAENOBUFS: _snprintf(lastError, 4095, "No buffer space is available."); break;
			case WSAENOTSOCK: _snprintf(lastError, 4095, "The descriptor is not a socket."); break;
			case WSAEOPNOTSUPP: _snprintf(lastError, 4095, "The referenced socket is not of a type that supports the listen operation."); break;
			default: _snprintf(lastError, 4095, "Unhandled error: [%i] generated by listen.", dwError);
		}

		// Cleanup on error
		closesocket(s);

		// Call the error function
		OnError(lastError);

		// Failure
		return FALSE;
	}

	// Save the socket now
	server.s = s;

	// Always execute, but limit scope of 'param'
	if(true)
	{
		// Allocate memory for a new parameter, deleted at the end of the AcceptThread function
		tAcceptThreadStruct * params = new tAcceptThreadStruct;

		// Clear out memory
		memset(params, 0, sizeof(tAcceptThreadStruct));

		// Store the server object address into the thread parameter object
		params->server = &server;

		// Create the thread - assume it is created
		server.hAcceptThread = CreateThread(0, 0, AcceptThread, params, CREATE_SUSPENDED, &server.hAcceptThreadId);

		// Create a temp event to sync threads 
		server.hTmpEvent = CreateEvent(0, TRUE, FALSE, "TempEvent");

		// Resume the thread after all data has been filled out
		ResumeThread(server.hAcceptThread);

		// Wait for the singal
		WaitForSingleObject(server.hTmpEvent, INFINITE);

		// Done with the handle now
		CloseHandle(server.hTmpEvent);

		// Clear it out
		server.hTmpEvent = 0;

		// Check to see if there was an error
		if(server.bTmpVar == FALSE)
		{
			// Cleanup on error
			closesocket(server.s);

			// Clear the socket
			server.s = INVALID_SOCKET;

			// Failure
			return FALSE;
		}
	}

	// Always execute, but limit scope of 'param'
	if(true)
	{
		// Exit code of the thread
		// Allocate memory for a new parameter, deleted at the end of the NetworkThread function
		tNetworkThreadStruct * params = new tNetworkThreadStruct;

		// Clear out memory
		memset(params, 0, sizeof(tNetworkThreadStruct));

		// Store the server object address into the thread parameter object
		params->server = &server;

		// Create the thread - assume it is created
		server.hNetworkThread = CreateThread(0, 0, NetworkThread, params, CREATE_SUSPENDED, &server.hNetworkThreadId);

		// Create a temp event to sync threads 
		server.hTmpEvent = CreateEvent(0, TRUE, FALSE, "TempEvent");

		// Resume the thread after all data has been filled out
		ResumeThread(server.hNetworkThread);

		// Wait for the singal
		WaitForSingleObject(server.hTmpEvent, INFINITE);

		// Done with the handle now
		CloseHandle(server.hTmpEvent);

		// Clear it out
		server.hTmpEvent = 0;

		// Check to see if there was an error
		if(server.bTmpVar == FALSE)
		{
			// Cleanup on error
			closesocket(server.s);

			// Clear the socket
			server.s = INVALID_SOCKET;

			// Failure
			return FALSE;
		}
	}

	// Success, return the socket
	return TRUE;
}

//-----------------------------------------------------------------------------

// Destroys the server
VOID Network_DestroyServer(tNetworkServer & server)
{
	// Mark the server as needing to exit
	server.bExit = TRUE;

	// Close the socket
	closesocket(server.s);

	// Wait for the thread to complete
	WaitForSingleObject(server.hAcceptThread, INFINITE);

	// Send the quit message to the network event window
	SendMessage(server.hNetworkWnd, WM_QUIT, 0, 0);

	// Wait for the thread to complete
	WaitForSingleObject(server.hNetworkThread, INFINITE);

	// Zero out the memory
	memset(&server, 0, sizeof(tNetworkServer));
}

//-----------------------------------------------------------------------------

// The thread function that accepts incoming connections
DWORD WINAPI AcceptThread(LPVOID lpParam)
{
	// Error variables
	CHAR lastError[4096] = {0};
	DWORD dwError = 0;

	// Store the thread structure
	tAcceptThreadStruct * params = (tAcceptThreadStruct *)lpParam;

	// No error
	params->server->bTmpVar = false;

	// Make sure we have a valid socket
	if(params->server->s == INVALID_SOCKET)
	{
		// Signal the other thread
		SetEvent(params->server->hTmpEvent);

		// Call the error function
		OnError("Invalid socket specified in the server object.");

		// Done with the thread
		return 0;
	}
	// Make sure the user function exists
	if(params->server->OnConnect == NULL)
	{
		// Signal the other thread
		SetEvent(params->server->hTmpEvent);

		// Call the error function
		OnError("No OnConnect function specified in the server object.");

		// Done with the thread
		return 0;
	}
	// Make sure the user function exists
	if(params->server->OnRead == NULL)
	{
		// Signal the other thread
		SetEvent(params->server->hTmpEvent);

		// Call the error function
		OnError("No OnRead function specified in the server object.");

		// Done with the thread
		return 0;
	}
	// Make sure the user function exists
	if(params->server->OnWrite == NULL)
	{
		// Signal the other thread
		SetEvent(params->server->hTmpEvent);

		// Call the error function
		OnError("No OnWrite function specified in the server object.");

		// Done with the thread
		return 0;
	}
	// Make sure the user function exists
	if(params->server->OnClose == NULL)
	{
		// Signal the other thread
		SetEvent(params->server->hTmpEvent);

		// Call the error function
		OnError("No OnClose function specified in the server object.");

		// Done with the thread
		return 0;
	}

	// No error
	params->server->bTmpVar = true;

	// Signal the other thread
	SetEvent(params->server->hTmpEvent);

	// Loop forever waiting for connections
	while(1)
	{
		// Accept a connection
		SOCKET clientSocket = accept(params->server->s, 0, 0);

		// If the connection is invalid, find out why
		if(clientSocket == INVALID_SOCKET || clientSocket == SOCKET_ERROR)
		{
			// The server is supposed to exit now
			if(params->server->bExit)
				break;

			// Handle the error
			dwError = WSAGetLastError();
			switch(dwError)
			{
				case WSANOTINITIALISED: _snprintf(lastError, 4095, "A successful WSAStartup call must occur before using this function."); break;
				case WSAECONNRESET: _snprintf(lastError, 4095, "An incoming connection was indicated, but was subsequently terminated by the remote peer prior to accepting the call."); break;
				case WSAEFAULT: _snprintf(lastError, 4095, "The addrlen parameter is too small or addr is not a valid part of the user address space."); break;
				case WSAEINTR: _snprintf(lastError, 4095, "A blocking Windows Sockets 1.1 call was canceled through WSACancelBlockingCall."); break;
				case WSAEINVAL: _snprintf(lastError, 4095, "The listen function was not invoked prior to accept."); break;
				case WSAEINPROGRESS: _snprintf(lastError, 4095, "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function."); break;
				case WSAEMFILE: _snprintf(lastError, 4095, "The queue is nonempty upon entry to accept and there are no descriptors available."); break;
				case WSAENETDOWN: _snprintf(lastError, 4095, "The network subsystem has failed."); break;
				case WSAENOBUFS: _snprintf(lastError, 4095, "No buffer space is available."); break;
				case WSAENOTSOCK: _snprintf(lastError, 4095, "The descriptor is not a socket."); break;
				case WSAEOPNOTSUPP: _snprintf(lastError, 4095, "The referenced socket is not a type that supports connection-oriented service."); break;
				case WSAEWOULDBLOCK: _snprintf(lastError, 4095, "The socket is marked as nonblocking and no connections are present to be accepted."); break;
				default: _snprintf(lastError, 4095, "Unhandled error: [%i] generated by accept.", dwError);
			}

			// Call the error function
			OnError(lastError);

			// Break out of the loop
			break;
		}

		// A client has connected, now send it to the user's function
		params->server->OnConnect(params->server, clientSocket);
	}

	// Delete the memory allocated previously
	delete params;

	// Standard return
	return 0;
}

//-----------------------------------------------------------------------------

// The thread function that handles network traffic for the login server
DWORD WINAPI NetworkThread(LPVOID lpParam)
{
	// Error variables
	CHAR lastError[4096] = {0};
	DWORD dwError = 0;

	// Store the thread structure
	tNetworkThreadStruct * params = (tNetworkThreadStruct *)lpParam;

	// No error
	params->server->bTmpVar = false;

	// Make sure we have a valid socket
	if(params->server->s == INVALID_SOCKET)
	{
		// Signal the other thread
		SetEvent(params->server->hTmpEvent);

		// Call the error function
		OnError("Invalid socket specified in the server object.");

		// Done with the thread
		return 0;
	}
	// Make sure the user function exists
	if(params->server->OnConnect == NULL)
	{
		// Signal the other thread
		SetEvent(params->server->hTmpEvent);

		// Call the error function
		OnError("No OnConnect function specified in the server object.");

		// Done with the thread
		return 0;
	}
	// Make sure the user function exists
	if(params->server->OnRead == NULL)
	{
		// Signal the other thread
		SetEvent(params->server->hTmpEvent);

		// Call the error function
		OnError("No OnRead function specified in the server object.");

		// Done with the thread
		return 0;
	}
	// Make sure the user function exists
	if(params->server->OnWrite == NULL)
	{
		// Signal the other thread
		SetEvent(params->server->hTmpEvent);

		// Call the error function
		OnError("No OnWrite function specified in the server object.");

		// Done with the thread
		return 0;
	}
	// Make sure the user function exists
	if(params->server->OnClose == NULL)
	{
		// Signal the other thread
		SetEvent(params->server->hTmpEvent);

		// Call the error function
		OnError("No OnClose function specified in the server object.");

		// Done with the thread
		return 0;
	}

	// No error
	params->server->bTmpVar = true;

	// Signal the other thread
	SetEvent(params->server->hTmpEvent);

	// Create the network message hwnd
	params->server->hNetworkWnd = CreateWindowEx(0, "NetworkClass", "", 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(0), NULL);

	// Make sure it was created
	if(params->server->hNetworkWnd == NULL)
	{
		// Build the error message
		_snprintf(lastError, 4095, "Unhandled error: [%i] generated by CreateWindowEx.", dwError);

		// Call the error function
		OnError(lastError);

		// Done with the thread
		return 0;
	}

	// Set the pointer for the window
	SetWindowLong(params->server->hNetworkWnd, GWL_USERDATA, PtrToUlong(params->server));

	// Process while we have a message queue
	MSG Msg = {0};
	while(GetMessage(&Msg, 0, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	// Done with the window
	DestroyWindow(params->server->hNetworkWnd);

	// Clear the handle
	params->server->hNetworkWnd = NULL;

	// Delete the memory allocated previously
	delete params;

	// Standard return
	return 0;
}

//-----------------------------------------------------------------------------

// This is the network message handling callback function
LRESULT CALLBACK NetworkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Get the server associated with the HWND
	tNetworkServer * server = (tNetworkServer *)UlongToPtr(GetWindowLong(hWnd, GWL_USERDATA));

	// Error string
	static char lastError[4096] = {0};

	// Network message variables
	static DWORD event = 0;
	static DWORD dwError = 0;
	static SOCKET currentSocket = 0;

	// Handle the message
	switch(uMsg)
	{
		// Quit message
		case WM_QUIT:
		{
			// If we have a valid server object
			if(server)
			{
				// Post the exit message
				PostThreadMessage(server->hNetworkThreadId, WM_QUIT, 0, 0);
			}
		} break;

		// Custom network event
		case WM_USER + 1:
		{
			// Store the network event
			event = WSAGETSELECTEVENT(lParam);

			// Store the network error
			dwError = WSAGETSELECTERROR(lParam);

			// Store the current socket
			currentSocket = wParam;

			// No server to handle the network event
			if(server == 0)
			{
				return 0;
			}

			// Network is down 
			if(dwError == WSAENETDOWN)
			{
				// Build the error
				_snprintf(lastError, 4095, "The network subsystem has failed.");

				// Call the error function
				OnError(lastError);

				// Do not forward this message
				return 0;
			}

			// Read event
			if(event == FD_READ)
			{
				server->OnRead(server, currentSocket);
			}
			// Write event
			else if(event == FD_WRITE)
			{
				server->OnWrite(server, currentSocket);
			}
			// Close event
			else if(event == FD_CLOSE)
			{
				server->OnClose(server, currentSocket);
			}

			// Do not forward this message
			return 0;
		} break;
	}

	// Default handler
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

//-----------------------------------------------------------------------------
