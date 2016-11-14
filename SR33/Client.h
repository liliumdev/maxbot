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

#ifndef CLIENT_H_
#define CLIENT_H_

//-----------------------------------------------------------------------------

#include "HandshakeApi.h"
#include "Packet.h"

//-----------------------------------------------------------------------------

// Forward declarations
class SilkroadProxy;
class UserData;

//-----------------------------------------------------------------------------

// A pending client connection
struct PendingClient
{
	// Login id
	DWORD id;

	// Server id
	DWORD sid;

	// World ip and port to connect to
	char worldIp[32];
	WORD worldPort;
};

//-----------------------------------------------------------------------------

// Client object
class Client
{
	// Friend class to access private data
	friend class SilkroadProxy;

private:
	// Silkroad proxy object this client belongs to
	SilkroadProxy * proxy;

	// Critical section access for the handshake api
	CRITICAL_SECTION cs;

	// Sockets to maintain
	SOCKET LocalSocket;
	SOCKET RemoteSocket;

	// Port and ip of the client
	WORD port;
	char ip[32];

	// Handshake api object for the user
	cHandShakeApi HandShakeApi;

	// Random number for the client
	DWORD dwRandNumber;

	// Buffers for the sockets - login
	char loginLocalRecvBuffer[16384];
	char loginRemoteRecvBuffer[16384];
	int loginLocalRecvIndex;
	int loginRemoteRecvIndex;

	// Login Id assigned
	DWORD loginId;

	// Id of server to connect to
	DWORD serverId;

	// User data
	UserData * data;

private:
	// Ctor
	Client();

	// Dtor
	~Client();

	// Adds a client
	void Create(SilkroadProxy * parent, SOCKET local, SOCKET remote);

	// Setup a client
	bool Setup(HWND hwnd);

	// Returns true of this socket belongs to the client
	bool BelongsToClient(SOCKET s);

	// Called when a socket is ready to have recv called
	void OnRead(SOCKET s);

	// Called when a socket is ready to have send called
	void OnWrite(SOCKET s);

	// Called when a socket is closed
	void OnDisconnect(SOCKET s);

	// User functions - Packet handler
	bool OnServerToClient(tPacket * packet, bool encrypted, bool injected);

	// User functions - Packet handler
	bool OnClientToServer(tPacket * packet, bool encrypted, bool injected);

public:
	// Disconnects the client
	void Disconnect();

	// Return the port
	WORD GetPort() const;

	// Return the ip
	const char * const GetIp() const;

	// The world server the client is connected to - 0 if it is a login client
	DWORD GetServerId();

	// Returns the server name the client is connected to -
	// "" if it is a login client, or "<Unknown>" if the server is not handled
	const char * const GetServerName();

	// Inject a packet into the client from the world server
	void InjectServerToClient(tPacket * packet, bool encrypted);

	// Inject a packet into the world server
	void InjectClientToServer(tPacket * packet, bool encrypted);
};

//-----------------------------------------------------------------------------

#endif
