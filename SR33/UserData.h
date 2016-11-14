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

#ifndef USERDATA_H_
#define USERDATA_H_

//-----------------------------------------------------------------------------

#ifndef PACKET_H_
	#include "Packet.h"
#endif

#ifndef CLIENT_H_
	#include "Client.h"
#endif

#ifndef _VECTOR_
	#include <vector>
#endif

#ifndef _STRING_
	#include <string>
#endif

//-----------------------------------------------------------------------------

// Demo user data class to show how to add in your own stuff :D
class UserData
{
private:
	// Chat command stuff
	BYTE realCount;
	BYTE fakeCount;

public:
	// Ctor
	UserData();

	// Default dtor
	~UserData();

	// Called when the client is connected to the server
	void OnConnect(Client * client);

	// Client to Server packet
	bool OnClientToServer(Client * client, tPacket * packet, bool encrypted, bool injected);

	// Server to Client packet
	bool OnServerToClient(Client * client, tPacket * packet, bool encrypted, bool injected);

	// Called when the client is closed
	void OnDisconnect(Client * client);

	// Called when a command is typed in
	void OnCommand(Client * client, std::string cmd);
};

//-----------------------------------------------------------------------------

// Typedef for the functions from SR33 for injecting packets
typedef void (*PluginInjectFunc)(DWORD client, tPacket * packet, bool encrypted);

// Called when the plugin is ready to be used
typedef void (*OnSetupFunc)(PluginInjectFunc InjectClientToServer, PluginInjectFunc InjectServerToClient);

// Called when the client is connected to the server
typedef void (*OnConnectFunc)(DWORD client);

// Client to Server packet
typedef bool (*OnClientToServerFunc)(DWORD client, tPacket * packet, bool encrypted, bool injected);

// Server to Client packet
typedef bool (*OnServerToClientFunc)(DWORD client, tPacket * packet, bool encrypted, bool injected);

// Called when the client is closed
typedef void (*OnDisconnectFunc)(DWORD client);

// Called when a command is typed in
typedef void (*OnCommandFunc)(DWORD client, std::string cmd);

// Called when the plugin is loaded or unloaded 
typedef void (*OnLoadFunc)();

//-----------------------------------------------------------------------------

// Plugin structure
struct tPlugin
{
	// The dll module
	HMODULE hDll;

	// Plugin name
	char name[64];

	// Called when the plugin is ready to be used
	OnSetupFunc OnSetup;

	// Called when the client is connected to the server
	OnConnectFunc OnConnect;

	// Client to Server packet
	OnClientToServerFunc OnClientToServer;

	// Server to Client packet
	OnServerToClientFunc OnServerToClient;

	// Called when the client is closed
	OnDisconnectFunc OnDisconnect;

	// Called when a command is typed in
	OnCommandFunc OnCommand;

	// Called when a plugin is loaded
	OnLoadFunc OnLoad;

	// Called when a plugin is unloaded
	OnLoadFunc OnUnload;
};

//-----------------------------------------------------------------------------

// Inject a packet into the server - plugin export function
extern "C" __declspec(dllexport) void PluginInjectClientToServer(DWORD client, tPacket * packet, bool encrypted);

// Inject a packet into the client from the server - plugin export function
extern "C" __declspec(dllexport) void PluginInjectServerToClient(DWORD client, tPacket * packet, bool encrypted);

//-----------------------------------------------------------------------------

// Fake a notice message to the client
void InjectFakeNotice(Client * client, const char * message);

// Tokenizes a string into a vector
std::vector<std::string> TokenizeString(const std::string& str, const std::string& delim);

//-----------------------------------------------------------------------------

// Loads a plugin
bool LoadPlugin(std::string filename);

// Unloads a plugin
bool UnloadPlugin(std::string filename);

// Returns a list of plugins loaded
std::vector<std::string> GetLoadedPluginNames();

//-----------------------------------------------------------------------------

#endif
