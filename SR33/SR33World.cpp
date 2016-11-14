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
Client * SilkroadProxy::AddWorldClient(SOCKET local, SOCKET remote)
{
	// Multithread access
	EnterCriticalSection(&worldListCS);

		// Allocate memory for a new client
		Client * client = new Client;

		// Store the sockets
		client->Create(this, local, remote);

		// Save the pointer
		worldClientList.push_back(client);

	// Multithread access
	LeaveCriticalSection(&worldListCS);

	// Return the pointer we just created
	return client;
}

//-----------------------------------------------------------------------------

// Called when a client connects to the world server
void SilkroadProxy::World_OnConnect(SOCKET s)
{
	// Get a client connecting to the server
	PendingClient * oldClient = GetNextPendingClient();

	// No pending client
	if(oldClient == 0)
	{
		// Show the error
		//OnError("No client avaliable to connect.");

		// Skip that connection silently
		return;
	}

	// Server address information
	sockaddr_in svr = {0};

	// Create a SOCKET for connecting to server
	SOCKET localSocket = s;
	SOCKET remoteSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	// Setup the connection properties
	svr.sin_addr.s_addr = inet_addr(oldClient->worldIp);
	svr.sin_family = AF_INET;
	svr.sin_port = htons(oldClient->worldPort);

	// Add the client to the managed list
	Client * client = AddWorldClient(localSocket, remoteSocket);

	// Store the server id
	client->serverId = oldClient->sid;

	// Free the memory of the pending client
	delete oldClient;

	// Try to connect
	if(connect(remoteSocket, (struct sockaddr*)&svr, sizeof(svr)) == 0)
	{
		// Setup the client	
		client->Setup(worldServer.hNetworkWnd);
	}
	else
	{
		// Close the sockets
		closesocket(remoteSocket);
		closesocket(localSocket);

		// Show the error
		OnError("Could not connect to the world server.");
	}
}

//-----------------------------------------------------------------------------

// When a socket is read to have data read from it
void SilkroadProxy::World_OnRead(SOCKET s)
{
	// Client to call the read function
	Client* client = 0;

	// Multithread access
	EnterCriticalSection(&worldListCS);

		// Loop through the list of clients
		std::vector<Client*>::iterator itr = worldClientList.begin();
		while(itr != worldClientList.end())
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
	LeaveCriticalSection(&worldListCS);

	// Call the read function
	if(client) client->OnRead(s);
}

//-----------------------------------------------------------------------------

// When a client is ready to have data sent to it
void SilkroadProxy::World_OnWrite(SOCKET s)
{
	// Client to call the read function
	Client* client = 0;

	// Multithread access
	EnterCriticalSection(&worldListCS);

		// Loop through the list of clients
		std::vector<Client*>::iterator itr = worldClientList.begin();
		while(itr != worldClientList.end())
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
	LeaveCriticalSection(&worldListCS);

	// Call the write function
	if(client) client->OnWrite(s);
}

//-----------------------------------------------------------------------------

// When a client closes the connection
void SilkroadProxy::World_OnClose(SOCKET s)
{
	// Multithread access
	EnterCriticalSection(&worldListCS);

		// Loop through the list of clients
		std::vector<Client*>::iterator itr = worldClientList.begin();
		while(itr != worldClientList.end())
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
				worldClientList.erase(itr);

				// Done searching
				break;
			}

			// Next client
			itr++;
		}

	// Multithread access
	LeaveCriticalSection(&worldListCS);
}

//-----------------------------------------------------------------------------

// Helper functions to remove clients
void SilkroadProxy::RemoveAllWorldClients()
{
	// Multithread access
	EnterCriticalSection(&worldListCS);

		// Loop through the list of clients
		std::vector<Client*>::iterator itr = worldClientList.begin();
		while(itr != worldClientList.end())
		{
			// Destroy the client
			(*itr)->Disconnect();

			// Free memory
			delete (*itr);

			// Next client
			itr++;
		}

		// Clear the client list
		worldClientList.clear();

	// Multithread access
	LeaveCriticalSection(&worldListCS);
}

//-----------------------------------------------------------------------------
