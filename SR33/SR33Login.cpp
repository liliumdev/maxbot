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

// Adds a client to be maintained
Client * SilkroadProxy::AddLoginClient(SOCKET local, SOCKET remote)
{
	// Multithread access
	EnterCriticalSection(&loginListCS);

		// Allocate memory for a new client
		Client * client = new Client;

		// Store the sockets
		client->Create(this, local, remote);

		// Save the pointer
		loginClientList.push_back(client);

	// Multithread access
	LeaveCriticalSection(&loginListCS);

	// Return the pointer we just created
	return client;
}

//-----------------------------------------------------------------------------

// When a socket is connecting
void SilkroadProxy::Login_OnConnect(SOCKET s)
{
	// Server address information
	sockaddr_in svr = {0};

	// Host information
	hostent * host = 0;

	// Create a SOCKET for connecting to server
	SOCKET localSocket = s;
	SOCKET remoteSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	// Add the client to the managed list
	Client * client = AddLoginClient(localSocket, remoteSocket);

	// Try to connect to the silkroad server
	int retry = 3;
	for(int x = 0; x < 3 || retry; ++x)
	{
		// First try
		if(x == 0)
		{
			// Try the first login server first
			host = GetHost("gwgt1.joymax.com");
		}
		// Second try
		else if(x == 1)
		{
			// If that fails, try the second login server
			host = GetHost("gwgt2.joymax.com");
		}
		else
		{
			// Count down retry count
			retry--;

			// If we have retires left
			if(retry > 0)
			{
				// Reset loop counter
				x = -1;

				// Delay a little before next retry
				Sleep(1500);

				// Next try
				continue;
			}

			// Close the sockets
			closesocket(remoteSocket);
			closesocket(localSocket);

			// Show the error
			OnError("Could not connect to the login servers.");

			// Done with the loop
			break;
		}

		// If the host was obtained
		if(host != NULL)
		{
			// Setup the connection properties
			svr.sin_addr.s_addr = *((unsigned long*)host->h_addr);
			svr.sin_family = AF_INET;
			svr.sin_port = htons(15779);

			// Try to connect
			if(connect(remoteSocket, (struct sockaddr*)&svr, sizeof(svr)) == 0)
			{
				// Setup the client	
				client->Setup(loginServer.hNetworkWnd);

				// Done with the loop
				break;
			}
		}
	}
}

//-----------------------------------------------------------------------------

// When a socket is read to have data read from it
void SilkroadProxy::Login_OnRead(SOCKET s)
{
	// Client to call the read function
	Client* client = 0;

	// Multithread access
	EnterCriticalSection(&loginListCS);

		// Loop through the list of clients
		std::vector<Client*>::iterator itr = loginClientList.begin();
		while(itr != loginClientList.end())
		{
			// Check to see if this socket belongs to it
			if((*itr)->BelongsToClient(s))
			{
				// Store the client
				client = (*itr);

				// Done searching
				break;
			}

			// Next client
			itr++;
		}

	// Multithread access
	LeaveCriticalSection(&loginListCS);

	// Call the read function
	if(client) client->OnRead(s);
}

//-----------------------------------------------------------------------------

// When a client is ready to have data sent to it
void SilkroadProxy::Login_OnWrite(SOCKET s)
{
	// Client to call the read function
	Client* client = 0;

	// Multithread access
	EnterCriticalSection(&loginListCS);

		// Loop through the list of clients
		std::vector<Client*>::iterator itr = loginClientList.begin();
		while(itr != loginClientList.end())
		{
			// Check to see if this socket belongs to it
			if((*itr)->BelongsToClient(s))
			{
				// Store the client
				client = (*itr);

				// Done searching
				break;
			}

			// Next client
			itr++;
		}

	// Multithread access
	LeaveCriticalSection(&loginListCS);

	// Call the write function
	if(client) client->OnWrite(s);
}

//-----------------------------------------------------------------------------

// When a client closes the connection
void SilkroadProxy::Login_OnClose(SOCKET s)
{
	// Multithread access
	EnterCriticalSection(&loginListCS);

		// Loop through the list of clients
		std::vector<Client*>::iterator itr = loginClientList.begin();
		while(itr != loginClientList.end())
		{
			// Check to see if this socket belongs to it
			if((*itr)->BelongsToClient(s))
			{
				// Call the close function
				(*itr)->OnDisconnect(s);

				// Destroy the client
				(*itr)->Disconnect();

				// Delete the client's memory
				delete (*itr);

				// Remove the client from the list
				loginClientList.erase(itr);

				// Done searching
				break;
			}

			// Next client
			itr++;
		}

	// Multithread access
	LeaveCriticalSection(&loginListCS);
}

//-----------------------------------------------------------------------------

// Helper functions to remove clients
void SilkroadProxy::RemoveAllLoginClients()
{
	// Multithread access
	EnterCriticalSection(&loginListCS);

		// Loop through the list of clients
		std::vector<Client*>::iterator itr = loginClientList.begin();
		while(itr != loginClientList.end())
		{
			// Destroy the client
			(*itr)->Disconnect();

			// Free memory
			delete (*itr);

			// Next client
			itr++;
		}

		// Clear the client list
		loginClientList.clear();

	// Multithread access
	LeaveCriticalSection(&loginListCS);
}

//-----------------------------------------------------------------------------
