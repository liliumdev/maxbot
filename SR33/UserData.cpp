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
	#include "UserData.h"
#endif

#ifndef _INC_STDIO
	#include <stdio.h>
#endif

#ifndef _ALGORITHM_
	#include <algorithm>
#endif

//-----------------------------------------------------------------------------
/*
	SR33 Demo 2

	To see the original UserData, please look at Version 1's files. This demo
	removes a lot of the demo stuff in place of adding the new plugin logic.
	With plugins you will be able to run SR33 once and load or unload dlls
	on demand from the chat console to test your code.

	This means that you will not have to leave the game unless you crash or
	disconnect when developing new tools and utilities for Silkroad. Since you
	will be able to use other people's plugins, please be extremely careful.
	A plugin could be written with malicious intent, so never use a plugin
	from someone you do not know.
*/
//-----------------------------------------------------------------------------

// List of clients to use with plugins
std::vector<Client *> clientList;
CRITICAL_SECTION clientCS;

// List of plugins
std::vector<tPlugin *> pluginList;
CRITICAL_SECTION pluginCS;

//-----------------------------------------------------------------------------

// Called once at SilkroadProxy::Initialize
void InitializeUserData()
{
	InitializeCriticalSection(&clientCS);
	InitializeCriticalSection(&pluginCS);

	//
	// NOTE: You can auto load plugins here!
	//

	if(LoadPlugin("DumpImageCode") == false)
		MessageBox(0, "The \"DumpImageCode\" plugin was not loaded.", "Plugin Loading Error", MB_ICONERROR);
	//if(LoadPlugin("Demo") == false)
		//MessageBox(0, "The \"Demo\" plugin was not loaded.", "Plugin Loading Error", MB_ICONERROR);
	//if(LoadPlugin("Analyzer") == false)
		//MessageBox(0, "The \"Analyzer\" plugin was not loaded.", "Plugin Loading Error", MB_ICONERROR);
	//if(LoadPlugin("ServerStats") == false)
		//MessageBox(0, "The \"ServerStats\" plugin was not loaded.", "Plugin Loading Error", MB_ICONERROR);
	//if(LoadPlugin("Injector") == false)
		//MessageBox(0, "The \"Injector\" plugin was not loaded.", "Plugin Loading Error", MB_ICONERROR);
}

//-----------------------------------------------------------------------------

// Called once at SilkroadProxy::Deinitialize
void DeinitializeUserData()
{
	// Unload all plugins
	for(size_t x = 0; x < pluginList.size(); ++x)
	{
		// Get the plugin
		tPlugin * plugin = pluginList[x];

		// With the plugin about to be removed now, disconnect all existing clients from it
		size_t size = clientList.size();
		for(size_t y = 0; y < size; ++y)
		{
			plugin->OnDisconnect(PtrToUlong(clientList[y]));
		}

		// The plugin is now unloaded
		plugin->OnUnload();

		// Unload the dll
		FreeLibrary(plugin->hDll);
		
		// Free memory
		delete plugin;
	}

	// Clear plugin list
	pluginList.clear();

	// Free cs data
	DeleteCriticalSection(&clientCS);
	DeleteCriticalSection(&pluginCS);
}

//-----------------------------------------------------------------------------

// Default ctor
UserData::UserData()
{
	// Clear memory of data
	realCount = 0;
	fakeCount = 0;
}

//-----------------------------------------------------------------------------

// Default dtor
UserData::~UserData()
{
}

//-----------------------------------------------------------------------------

// Called when the client connects to the server
void UserData::OnConnect(Client * client)
{
	// Multithread access protection
	EnterCriticalSection(&clientCS);
		clientList.push_back(client);
	LeaveCriticalSection(&clientCS);

	// Pass the event to existing plugins
	size_t size = pluginList.size();
	if(size)
	{
		EnterCriticalSection(&pluginCS);
			for(size_t x = 0; x < size; ++x)
			{
				pluginList[x]->OnConnect(PtrToUlong(client));
			}
		LeaveCriticalSection(&pluginCS);
	}
}

//-----------------------------------------------------------------------------

// Called when the client is closed
void UserData::OnDisconnect(Client * client)
{
	// Pass the event to plugins
	size_t size = pluginList.size();
	if(size)
	{
		EnterCriticalSection(&pluginCS);
			for(size_t x = 0; x < size; ++x)
			{
				pluginList[x]->OnDisconnect(PtrToUlong(client));
			}
		LeaveCriticalSection(&pluginCS);
	}

	// Multithread access protection
	EnterCriticalSection(&clientCS);
		clientList.erase(std::find(clientList.begin(), clientList.end(), client));
	LeaveCriticalSection(&clientCS);
}

//-----------------------------------------------------------------------------

// User functions - Packet handler
bool UserData::OnClientToServer(Client * client, tPacket * packet, bool encrypted, bool injected)
{
	// Should the packet be kept?
	bool bKeep = true;

	// Packet reading object
	PacketReader reader(packet);

	// Client is sending a chat packet
	if(packet->opcode == 0x7367)
	{
		// Read chat type
		BYTE type = reader.ReadByte();

		// Read the original chat stamp, not used
		BYTE stamp = reader.ReadByte();

		// Fix the counter of the chat messages with the value that is actually sent
		packet->data[1] = realCount;

		// If the packet is injected, we do not need to set the fake count
		if(injected == false)
		{
			// Need to track what the client is expecting to see
			fakeCount++;
		}

		// If chat is said in all chat - note this code is specific to ALL chat
		// you will have to rework it for the other chat types.
		if(type == 1)
		{
			// Chat messages
			wchar_t chat[1024] = {0};
			char buffer[1024] = {0};

			// Length of the text
			WORD len = reader.ReadWord();
			reader.ReadWideString(len, chat);

			// Convert to a ascii string
			wcstombs(buffer, chat, 1023);

			// A custom command :) I do not let it show in the chat window
			if(buffer[0] == '.')
			{
				// Store as a string, skip by the . at the front of the string
				std::string cmd = buffer + 1;

				// Makes life easier
				OnCommand(client, cmd);

				// Do not send to the server!
				return false;
			}
			// Otherwise a regular message so update the chat counter so we do not get out of sync
			else
			{
				realCount++;
			}
		}
		// Not handled, so update the counter
		else
		{
			realCount++;
		}
	}

	//
	// NOTE: Some packets you may not want being sent to plugins. If this is the case, then 
	// you will need to not call the next code block if the opcode matches against the one
	// you wish is not forwarded.
	//

	// Pass the event to plugins
	size_t size = pluginList.size();
	if(size)
	{
		EnterCriticalSection(&pluginCS);
			for(size_t x = 0; x < size; ++x)
			{
				bKeep = bKeep && pluginList[x]->OnClientToServer(PtrToUlong(client), packet, encrypted, injected);
			}
		LeaveCriticalSection(&pluginCS);
	}

	// Return if we should keep the packet
	return bKeep;
}

//-----------------------------------------------------------------------------

// User functions - Packet handler
bool UserData::OnServerToClient(Client * client, tPacket * packet, bool encrypted, bool injected)
{
	// Should the packet be kept?
	bool bKeep = true;

	// Packet reading object
	PacketReader reader(packet);

	// Chat sync message
	if(packet->opcode == 0xB367)
	{
		// We need to fake the response to be what the client expects to keep things 
		// nice and smooth. FakeCount will be one past the index we need, so subtract 1
		packet->data[2] = (fakeCount - 1);
	}

	//
	// NOTE: Some packets you may not want being sent to plugins. If this is the case, then 
	// you will need to not call the next code block if the opcode matches against the one
	// you wish is not forwarded.
	//

	// Pass the event to plugins
	size_t size = pluginList.size();
	if(size)
	{
		EnterCriticalSection(&pluginCS);
			for(size_t x = 0; x < size; ++x)
			{
				bKeep = bKeep && pluginList[x]->OnServerToClient(PtrToUlong(client), packet, encrypted, injected);
			}
		LeaveCriticalSection(&pluginCS);
	}

	// Return if we should keep the packet
	return bKeep;
}

//-----------------------------------------------------------------------------

// Called when a command is typed in
void UserData::OnCommand(Client * client, std::string cmd)
{
	std::vector<std::string> args = TokenizeString(cmd, " ");
	size_t argCount = args.size() - 1;
	char msg[256] = {0};
	size_t size = 0;

	// Load a plugin
	if(args[0] == "load")
	{
		// Check args
		if(argCount != 1)
		{
			InjectFakeNotice(client, "Usage: .load PluginName");
			return;
		}

		// Try to load the plugin
		if(LoadPlugin(args[1]) == false)
			InjectFakeNotice(client, "The plugin was not loaded successfully.");
		else
			InjectFakeNotice(client, "The plugin was loaded successfully.");

		// Do not let plugins have this command
		return;
	}

	// Unload a plugin
	else if(args[0] == "unload")
	{
		// Check args
		if(argCount != 1)
		{
			InjectFakeNotice(client, "Usage: .unload PluginName");
			return;
		}

		// Unload the plugin
		if(UnloadPlugin(args[1]))
			InjectFakeNotice(client, "The plugin was unloaded successfully.");
		else
			InjectFakeNotice(client, "The plugin was not unloaded successfully.");

		// Do not let plugins have this command
		return;
	}

	//
	// NOTE: If you do not want a command to be forwarded to a plugin, return from
	// this function before the next coe block is exeucted
	//

	// Pass the event to plugins
	size = pluginList.size();
	if(size)
	{
		EnterCriticalSection(&pluginCS);
			for(size_t x = 0; x < size; ++x)
			{
				pluginList[x]->OnCommand(PtrToUlong(client), cmd);
			}
		LeaveCriticalSection(&pluginCS);
	}
}

//-----------------------------------------------------------------------------

// Fake a notice message to the client
void InjectFakeNotice(Client * client, const char * message)
{
	// Covert the ascii message into unicode since that is what the server expects
	wchar_t buffer[1024] = {0};
	mbstowcs(buffer, message, 1023);

	PacketBuilder builder;
	builder.SetOpcode(0x3667);										// Chat opcode
	builder.AppendByte(0x07);										// Notice
	builder.AppendWord((WORD)strlen(message));						// Save length of message
	builder.AppendWideString(buffer);								// Save the converted message
	client->InjectServerToClient(builder.GetPacket(), false);		// Inject the packet
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

// Inject a packet into the client from the server - plugin export function
void PluginInjectServerToClient(DWORD client, tPacket * packet, bool encrypted)
{
	EnterCriticalSection(&clientCS);
		for(size_t x = 0; x < clientList.size(); ++x)
		{
			if(UlongToPtr(client) == clientList[x])
			{
				clientList[x]->InjectServerToClient(packet, encrypted);
				break;
			}
		}
	LeaveCriticalSection(&clientCS);
}

//-----------------------------------------------------------------------------

// Inject a packet into the server - plugin export function
void PluginInjectClientToServer(DWORD client, tPacket * packet, bool encrypted)
{
	EnterCriticalSection(&clientCS);
		for(size_t x = 0; x < clientList.size(); ++x)
		{
			if(UlongToPtr(client) == clientList[x])
			{
				clientList[x]->InjectClientToServer(packet, encrypted);
				break;
			}
		}
	LeaveCriticalSection(&clientCS);
}

//-----------------------------------------------------------------------------

// Loads a plugin
bool LoadPlugin(std::string filename)
{
	// Was the plugin added
	bool bAdd = true;

	// Temp string
	char msg[256] = {0};

	// Build the plugin name
	char tmpname[64];
	_snprintf(tmpname, 63, "%s", filename.c_str());
	size_t size = strlen(tmpname);
	for(size_t x = 0; x < size; ++x)
	{
		tmpname[x] = tolower(tmpname[x]);
		if(tmpname[x] == '.')
		{
			tmpname[x] = 0;
			break;
		}
	}

	// Make sure the plugin doesnt already exist
	EnterCriticalSection(&pluginCS);
		for(size_t x = 0; x < pluginList.size(); ++x)
		{
			if(strcmp(tmpname, pluginList[x]->name) == 0)
			{
				bAdd = false;
				break;
			}
		}
	LeaveCriticalSection(&pluginCS);

	// If the plugin exists already
	if(bAdd == false)
		return false;

	// Build the path to the plugin
	_snprintf(msg, 255, "plugins\\%s", filename.c_str());

	// Allocate memory for a new plugin
	tPlugin * plugin = new tPlugin;
	memset(plugin, 0, sizeof(tPlugin));

	// Load it
	plugin->hDll = LoadLibrary(msg);

	// Make sure the dll was loaded
	if(plugin->hDll == NULL)
	{
		// Free memory
		delete plugin;

		// Done with this message
		return false;
	}

	// Get all of the functions for the client
	plugin->OnSetup = (OnSetupFunc)GetProcAddress(plugin->hDll, "OnSetup");
	plugin->OnLoad = (OnLoadFunc)GetProcAddress(plugin->hDll, "OnLoad");
	plugin->OnUnload = (OnLoadFunc)GetProcAddress(plugin->hDll, "OnUnload");
	plugin->OnConnect = (OnConnectFunc)GetProcAddress(plugin->hDll, "OnConnect");
	plugin->OnClientToServer = (OnClientToServerFunc)GetProcAddress(plugin->hDll, "OnClientToServer");
	plugin->OnDisconnect = (OnDisconnectFunc)GetProcAddress(plugin->hDll, "OnDisconnect");
	plugin->OnServerToClient = (OnServerToClientFunc)GetProcAddress(plugin->hDll, "OnServerToClient");
	plugin->OnCommand = (OnCommandFunc)GetProcAddress(plugin->hDll, "OnCommand");

	// Check for any function loading errors
	if(!plugin->OnConnect || !plugin->OnClientToServer || !plugin->OnDisconnect || !plugin->OnServerToClient || !plugin->OnCommand || !plugin->OnLoad || !plugin->OnUnload)
	{
		// Free the dll
		FreeLibrary(plugin->hDll);

		// Free the memory
		delete plugin;

		// Done with this message
		return false;
	}

	// Build the plugin name
	_snprintf(plugin->name, 63, "%s", filename.c_str());
	size = strlen(plugin->name);
	for(size_t x = 0; x < size; ++x)
	{
		plugin->name[x] = tolower(plugin->name[x]);
		if(plugin->name[x] == '.')
		{
			plugin->name[x] = 0;
			break;
		}
	}

	EnterCriticalSection(&pluginCS);
		// Check existing plugins for the same name
		for(size_t x = 0; x < pluginList.size(); ++x)
		{
			if(strcmp(plugin->name, pluginList[x]->name) == 0)
			{
				bAdd = false;
				break;
			}
		}

		// If we should add this plugin
		if(bAdd) 
		{
			// Save it in the list of plugins to maintain
			pluginList.push_back(plugin);
		}
	LeaveCriticalSection(&pluginCS);

	// Check to see if the plugin is to be added
	if(bAdd == false)
	{
		// Free the dll
		FreeLibrary(plugin->hDll);

		// Free the memory
		delete plugin;

		// Done with this message
		return false;
	}

	// The plugin is now loaded
	plugin->OnLoad();

	// Call the setup function now
	plugin->OnSetup(PluginInjectClientToServer, PluginInjectServerToClient);

	// With the plugin added now, connect all existing clients to it
	size = clientList.size();
	EnterCriticalSection(&clientCS);		
		for(size_t x = 0; x < size; ++x)
		{
			plugin->OnConnect(PtrToUlong(clientList[x]));
		}
	LeaveCriticalSection(&clientCS);	

	// Do not let plugins have this command
	return true;
}

//-----------------------------------------------------------------------------

// Unloads a plugin
bool UnloadPlugin(std::string filename)
{
	// Was a plugin unloaded
	bool bResult = false;

	// Make the string lower case
	std::transform(filename.begin(), filename.end(), filename.begin(), tolower);

	// Multithread access protection
	EnterCriticalSection(&pluginCS);
		for(size_t x = 0; x < pluginList.size(); ++x)
		{
			tPlugin * plugin = pluginList[x];
			std::string pname = plugin->name;
			if(pname == filename)
			{
				// Remove it from being tracked so events are no longer posted to it
				pluginList.erase(pluginList.begin() + x);

				// With the plugin about to be removed now, disconnect all existing clients from it
				size_t size = clientList.size();

				// Enter the crtical section - need to watch out for possible deadlock
				EnterCriticalSection(&clientCS);
					for(size_t y = 0; y < size; ++y)
					{
						plugin->OnDisconnect(PtrToUlong(clientList[y]));
					}
				LeaveCriticalSection(&clientCS);	

				// The plugin is now unloaded
				plugin->OnUnload();

				// Unload the dll
				FreeLibrary(plugin->hDll);
				
				// Free memory
				delete plugin;

				// Plugin was found and removed
				bResult = true;

				// Done with the loop
				break;
			}
		}
	// Multithread access protection
	LeaveCriticalSection(&pluginCS);

	// Return the status
	return bResult;
}

//-----------------------------------------------------------------------------

// Returns a list of plugins loaded
std::vector<std::string> GetLoadedPluginNames()
{
	std::vector<std::string> names;
	// Multithread access protection
	EnterCriticalSection(&pluginCS);
		size_t size = pluginList.size();
		for(size_t x = 0; x < size; ++x)
		{
			names.push_back(pluginList[x]->name);
		}
	// Multithread access protection
	LeaveCriticalSection(&pluginCS);
	return names;
}

//-----------------------------------------------------------------------------
