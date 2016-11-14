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

#ifndef PLUGIN_H_
#define PLUGIN_H_

//-----------------------------------------------------------------------------

#ifndef PACKET_H_
	#include "../SR33/Packet.h"
#endif

#ifndef _STRING_
	#include <string>
#endif

#ifndef _WINDOWS_
	#include <windows.h>
#endif

//-----------------------------------------------------------------------------

// Typedef for the functions from SR33 for injecting packets
typedef void (*PluginInjectFunc)(DWORD client, tPacket * packet, bool encrypted);

// Called when the plugin is ready to be used
extern "C" __declspec(dllexport) void OnSetup(PluginInjectFunc InjectClientToServer, PluginInjectFunc InjectServerToClient);

// Called when the client is connected to the server
extern "C" __declspec(dllexport) void OnConnect(DWORD client);

// Client to Server packet
extern "C" __declspec(dllexport) bool OnClientToServer(DWORD client, tPacket * packet, bool encrypted, bool injected);

// Server to Client packet
extern "C" __declspec(dllexport) bool OnServerToClient(DWORD client, tPacket * packet, bool encrypted, bool injected);

// Called when the client is closed
extern "C" __declspec(dllexport) void OnDisconnect(DWORD client);

// Called when a command is typed in
extern "C" __declspec(dllexport) void OnCommand(DWORD client, std::string cmd);

// Called when a plugin is loaded
extern "C" __declspec(dllexport) void OnLoad();

// Called when a plugin is unloaded
extern "C" __declspec(dllexport) void OnUnload();

//-----------------------------------------------------------------------------

#endif