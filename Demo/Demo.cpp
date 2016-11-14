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
	#include "../Plugin/Plugin.h"
#endif

#ifndef _VECTOR_
	#include <vector>
#endif

//-----------------------------------------------------------------------------

// Tokenizes a string into a vector
std::vector<std::string> TokenizeString(const std::string& str, const std::string& delim);

//-----------------------------------------------------------------------------

// Plugin functions to use from SR33
PluginInjectFunc PluginInjectServerToClient = 0;
PluginInjectFunc PluginInjectClientToServer = 0;

// DLL instance
HINSTANCE gInstance = 0;

//-----------------------------------------------------------------------------

// (DEMO)
//
// Fake a global message to the client (DEMO)
void InjectFakeGlobal(DWORD client, const char * playername, const char * message);

// Fake a global message to the client (DEMO)
void InjectFakeNotice(DWORD client, const char * message);

// Increase str stat point (DEMO)
void RaiseStr(DWORD  client);

// Increase int stat point (DEMO)
void RaiseInt(DWORD  client);

// Drop an amount of gold (DEMO)
void DropGold(DWORD  client, DWORD amount);

//-----------------------------------------------------------------------------

// Called when the plugin is loaded
void OnLoad()
{
}

//-----------------------------------------------------------------------------

// Called when the plugin is unloaded
void OnUnload()
{
}

//-----------------------------------------------------------------------------

// Called when the client is connected to the server
void OnConnect(DWORD client)
{
}

//-----------------------------------------------------------------------------

// Client to Server packet
bool OnClientToServer(DWORD client, tPacket * packet, bool encrypted, bool injected)
{
	// Accept the packet
	return true;
}

//-----------------------------------------------------------------------------

// Server to Client packet
bool OnServerToClient(DWORD client, tPacket * packet, bool encrypted, bool injected)
{
	// Accept the packet
	return true;
}

//-----------------------------------------------------------------------------

// Called when the client is closed
void OnDisconnect(DWORD client)
{
}

//-----------------------------------------------------------------------------

// Called when a command is typed in
void OnCommand(DWORD client, std::string cmd)
{
	std::vector<std::string> args = TokenizeString(cmd, " ");
	size_t argCount = args.size() - 1;

	//
	// Your commands go here
	//

	// (DEMO)
	//
	// Command processing (DEMO)
	if(cmd == "sit")
	{
		PacketBuilder builder;											// Build and send a sit packet
		builder.SetOpcode(0x7017);										// Opcode
		builder.AppendByte(0x04);										// Party chat
		PluginInjectClientToServer(client, builder.GetPacket(), false);		// Inject the packet

		// Status message via notice
		InjectFakeNotice(client, "Your character will now sit/stand.");
	}

	// Global faking  client side (DEMO)
	else if(cmd == "global")
	{
		InjectFakeGlobal(client, "[SR33]", "This is a fake global.");
	}

	// Drop a piece of gold (DEMO)
	else if(cmd == "dropgold")
	{
		// Code that you might want to execute more than once you can 
		// throw into a function like this, so when you need to use it,
		// you can just call it and not have to replicate code.
		DropGold(client, 1);
	}

	// Raising stats manually (DEMO)
	else if(cmd == "str")
	{
		RaiseStr(client);
	}

	// Raising stats manually (DEMO)
	else if(cmd == "int")
	{
		RaiseInt(client);
	}
}

//-----------------------------------------------------------------------------

// Tokenizes a string into a vector
std::vector<std::string> TokenizeString(const std::string& str, const std::string& delim)
{
	// http://www.gamedev.net/community/forums/topic.asp?topic_id=381544#TokenizeString
	using namespace std;
	vector<string> tokens;
	size_t p0 = 0, p1 = string::npos;
	while(p0 != string::npos)
	{
		p1 = str.find_first_of(delim, p0);
		if(p1 != p0)
		{
			string token = str.substr(p0, p1 - p0);
			tokens.push_back(token);
		}
		p0 = str.find_first_not_of(delim, p1);
	}
	return tokens;
}

//-----------------------------------------------------------------------------

// DLL entry point
// (You should not need to modify this function)
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    if(dwReason == DLL_PROCESS_ATTACH){ DisableThreadLibraryCalls(hinst); gInstance = hinst; }
	return TRUE;
}

//-----------------------------------------------------------------------------

// Called when the plugin is ready to be used
// (You should not need to modify this function)
void OnSetup(PluginInjectFunc InjectClientToServer, PluginInjectFunc InjectServerToClient)
{
	PluginInjectServerToClient = InjectServerToClient;
	PluginInjectClientToServer = InjectClientToServer;
}

//-----------------------------------------------------------------------------
// (DEMO)

// Fake a global message to the client
void InjectFakeGlobal(DWORD client, const char * playername, const char * message)
{
	// Covert the ascii message into unicode since that is what the server expects
	wchar_t buffer[1024] = {0};
	mbstowcs(buffer, message, 1023);

	PacketBuilder builder;
	builder.SetOpcode(0x3667);										// Chat opcode
	builder.AppendByte(0x06);										// Global chat
	builder.AppendWord((WORD)strlen(playername));					// Save length of name
	builder.AppendString(playername);								// Save the name
	builder.AppendWord((WORD)strlen(message));						// Save length of message
	builder.AppendWideString(buffer);								// Save the converted message
	PluginInjectServerToClient(client, builder.GetPacket(), false);		// Inject the packet
}

//-----------------------------------------------------------------------------

// Fake a global message to the client
void InjectFakeNotice(DWORD client, const char * message)
{
	// Covert the ascii message into unicode since that is what the server expects
	wchar_t buffer[1024] = {0};
	mbstowcs(buffer, message, 1023);

	PacketBuilder builder;
	builder.SetOpcode(0x3667);										// Chat opcode
	builder.AppendByte(0x07);										// Notice
	builder.AppendWord((WORD)strlen(message));						// Save length of message
	builder.AppendWideString(buffer);								// Save the converted message
	PluginInjectServerToClient(client, builder.GetPacket(), false);		// Inject the packet
}

//-----------------------------------------------------------------------------

/*
	Decrease 1 stat point - str (Response Packet)
	[Server->Client][0][B27A][01]
	01
*/
// Increase str stat point
void RaiseStr(DWORD client)
{
	PacketBuilder builder;
	builder.SetOpcode(0x727A);										// Chat opcode
	PluginInjectClientToServer(client, builder.GetPacket(), false);		// Inject the packet
}

//-----------------------------------------------------------------------------

/*
	Decrease 1 stat point - int  (Response Packet)
	[Server->Client][0][B552][01]
	01
*/
// Increase int stat point
void RaiseInt(DWORD client)
{
	PacketBuilder builder;
	builder.SetOpcode(0x7552);										// Chat opcode
	PluginInjectClientToServer(client, builder.GetPacket(), false);		// Inject the packet
}

//-----------------------------------------------------------------------------

// Drop an amount of gold
void DropGold(DWORD client, DWORD amount)
{
	PacketBuilder builder;
	builder.SetOpcode(0x706D);										// Chat opcode
	builder.AppendByte(0x0A);										// Gold slot
	builder.AppendDword(amount);									// Add the amount to drop
	PluginInjectClientToServer(client, builder.GetPacket(), false);		// Inject the packet
}

//-----------------------------------------------------------------------------
