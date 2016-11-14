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

#ifndef HANDSHAKE_API_H_
	#include "../SR33/HandshakeAPi.h"
#endif

#ifndef _VECTOR_
	#include <vector>
#endif

#ifndef _MAP_
	#include <map>
#endif

#include "resource.h"

#pragma comment(lib, "ws2_32.lib")

//-----------------------------------------------------------------------------

// Server stats object - Optional
struct tServerStats
{
	WORD sid;
	char name[32];
	WORD cur;
	WORD max;
	BYTE state;
};

struct tServerStatsHandles
{
	HWND name;
	HWND cur;
	HWND max;
	HWND state;
};

//-----------------------------------------------------------------------------

// Tokenizes a string into a vector
std::vector<std::string> TokenizeString(const std::string& str, const std::string& delim);

// Update server information
void UpdateServer(int id, char * name, WORD cur, WORD max, BYTE state);

// Window procedure for the GUI
INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//-----------------------------------------------------------------------------

// Plugin functions to use from SR33
PluginInjectFunc PluginInjectServerToClient = 0;
PluginInjectFunc PluginInjectClientToServer = 0;

// CS for server stats
std::map<int, tServerStats> stats;
std::map<int, tServerStatsHandles> statHandles;

DWORD threadId = 0;
HANDLE hThread = 0;
HWND gHwnd = 0;

// DLL instance
HINSTANCE gInstance = 0;

// Socket to use for getting server stats
SOCKET loginSocket;

// Handshake api object
cHandShakeApi handshake;

// This client
DWORD myClient = 0;

// Try to keep packets from flooding
BOOL needServerStats = TRUE;

// If we should try and get the right client for serverstats
BOOL getClient = FALSE;

//-----------------------------------------------------------------------------

// Stats Thread
DWORD WINAPI ServerStatsThread(LPVOID lParam)
{
	// Winsock data
	WSADATA wsaData = {0};

	// server address information
	sockaddr_in server = {0};

	// host information
	hostent * host = 0;

	// Create the GUI
	gHwnd = CreateDialog(gInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
	if(gHwnd == NULL)
	{
		MessageBox(NULL, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	// Try to start the winsock library
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	// create a SOCKET for connecting to server
	loginSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	// try to connect to the silkroad server
	for(int x = 0; x < 3; ++x)
	{
		// first iteration
		if(x == 0)
		{
			// try the first login server first
			host = gethostbyname("gwgt1.joymax.com");
			if(host != NULL)
			{
				// Setup the connection properties
				server.sin_addr.s_addr = *((unsigned long*)host->h_addr);
				server.sin_family = AF_INET;
				server.sin_port = htons(0x33);

				getClient = TRUE;

				// try to connect
				if(connect(loginSocket, (struct sockaddr*)&server, sizeof(server)) == 0)
					break;

				getClient = FALSE;
			}
		}
		// second iteration
		else if(x == 1)
		{
			// if that fails, try the second login server
			host = gethostbyname("gwgt2.joymax.com");
			if(host != NULL)
			{
				// setup the connection properties
				server.sin_addr.s_addr = *((unsigned long*)host->h_addr);
				server.sin_family = AF_INET;
				server.sin_port = htons(0x33);

				getClient = TRUE;

				// try to connect
				if(connect(loginSocket, (struct sockaddr*)&server, sizeof(server)) == 0)
					break;

				getClient = FALSE;
			}
		}
		// last iteration
		else
		{
			// close the socket
			closesocket(loginSocket);

			// Show the error
			MessageBox(0, "Could not connect to the login servers.", "Error", MB_ICONERROR);

			// signal error
			return 0;
		}
	}

	// Register the socket to the hwnd
	WSAAsyncSelect(loginSocket, gHwnd, WM_USER + 1, FD_CLOSE | FD_READ | FD_WRITE);

	// Process while we have a message queue
	MSG Msg = {0};
	while(GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	// Standard return
	return 0;
}

//-----------------------------------------------------------------------------

// Called when the plugin is loaded
void OnLoad()
{
	handshake.SetRandomNumber(0x33);
	hThread = CreateThread(0, 0, ServerStatsThread, 0, 0, &threadId);
}

//-----------------------------------------------------------------------------

// Called when the plugin is unloaded
void OnUnload()
{
	closesocket(loginSocket);
	EndDialog(gHwnd, 0);
	TerminateThread(hThread, 0);
	WSACleanup();
}

//-----------------------------------------------------------------------------

// Called when the client is connected to the server
void OnConnect(DWORD client)
{
	// Assume the client just now connecting is ourself, no other nice way to handle this
	// for now with the plugin setup.
	if(getClient && myClient == 0)
		myClient = client;
}

//-----------------------------------------------------------------------------

// Client to Server packet
bool OnClientToServer(DWORD client, tPacket * packet, bool encrypted, bool injected)
{
	// Do not process packets that are not ours
	if(client != myClient)
		return true;

	// Accept the packet
	return true;
}

//-----------------------------------------------------------------------------

// Server to Client packet
bool OnServerToClient(DWORD client, tPacket * packet, bool encrypted, bool injected)
{
	// Do not process packets that are not ours
	if(client != myClient)
		return true;

	// Handshake packets
	if(packet->opcode == 0x5000)
	{
		handshake.OnPacketRecv((LPBYTE)packet, packet->size + 6, loginSocket);
	}

	// Whoami packet
	else if(packet->opcode == 0x2001)
	{
		PacketReader reader(packet);

		WORD nameLen = reader.ReadWord();
		char name[256] = {0};
		reader.ReadString(nameLen, name);

		// If the server is the gateway server
		if(strcmp(name, "GatewayServer") == 0)
		{
			// Set the ping packet
			SetTimer(gHwnd, 1, 5000, 0);

			// Build the whoami packet
			PacketBuilder builder;
			builder.SetOpcode(0x6100);
			builder.AppendByte(0x12);
			builder.AppendWord(9);
			builder.AppendString("SR_Client");
			builder.AppendDword(144); // Version
			PluginInjectClientToServer(client, builder.GetPacket(), true);
		}
		else
		{
			MessageBox(0, "The server is not a GatewayServer", "Fatal Error", MB_ICONERROR);
		}
	}

	// Version info stuff
	else if(packet->opcode == 0x600D)
	{
		// Wait for the last byte in this series
		if(packet->size == 2)
		{
			// Set the server stats packet
			SetTimer(gHwnd, 2, 1000, 0);
		}
	}

	// Server stats
	else if(packet->opcode == 0xA101)
	{
		// Packet reader
		PacketReader reader(packet);

		// Length and name of server
		WORD len = 0;
		char name[256] = {0};

		reader.ReadByte();	// 0x01
		reader.ReadByte();	// 0x15
		len = reader.ReadWord();	// Name length
		reader.ReadString(len, name);
		reader.ReadByte();	// 0x00
		
		// A new server?
		BYTE b = reader.ReadByte();
		while(b)
		{
			// Server id
			WORD serverId = reader.ReadWord();

			// Server name
			len = reader.ReadWord();
			reader.ReadString(len, name);

			// Chop off the (NEW) tag
			for(size_t x = 0; x < strlen(name); ++x)
			{
				if(name[x] == '(')
				{
					name[x] = 0;
					break;
				}
			}

			// Read stats
			WORD cur = reader.ReadWord();
			WORD max = reader.ReadWord();

			// Check or Open
			BYTE state = reader.ReadByte();

			// Update the server
			UpdateServer(serverId, name, cur, max, state);

			// A new server?
			b = reader.ReadByte();
		}

		// We need stats after it's been processed
		needServerStats = TRUE;
	}

	// Accept the packet
	return true;
}

//-----------------------------------------------------------------------------

// Called when the client is closed
void OnDisconnect(DWORD client)
{
	if(myClient == client)
		myClient = 0;
}

//-----------------------------------------------------------------------------

// Called when a command is typed in
void OnCommand(DWORD client, std::string cmd)
{
	std::vector<std::string> args = TokenizeString(cmd, " ");
	size_t argCount = args.size() - 1;
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

// Update server information
void UpdateServer(int id, char * name, WORD cur, WORD max, BYTE state)
{
	char buffer[32];
	_snprintf(stats[id].name, 31, "%s", name);
	stats[id].cur = cur;
	stats[id].max = max;
	stats[id].state = state;

	SetWindowText(statHandles[id].name, name);
	_snprintf(buffer, 31, "%i", cur);
	SetWindowText(statHandles[id].cur, buffer);
	_snprintf(buffer, 31, "%i", max);
	SetWindowText(statHandles[id].max, buffer);
	if(state)
		SetWindowText(statHandles[id].state, "Open");
	else
		SetWindowText(statHandles[id].state, "Check");
}

//-----------------------------------------------------------------------------

// handle network events
void OnNetworkEvent(SOCKET s, DWORD event, DWORD error)
{
	// Socket is ready for reading
	if(event == FD_READ)
	{
		// recv buffer count
		static size_t recvsize = 0;

		// recv buffer
		static char recvbuf[8192] = {0};

		// receive data from the client, data is handled bt SR33 so we do not need to process it ourselves
		recvsize = recv(s, recvbuf, 8192, 0);
	}
	else if(event == FD_WRITE)
	{
	}
	else if(event == FD_CLOSE)
	{
	}
}

//-----------------------------------------------------------------------------

// Window procedure for the GUI
INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Handle the message
	switch(uMsg) 
    {
		// Dialog initialize
		case WM_INITDIALOG:
        {
			// Allow 1 dialog
			if(gHwnd)
			{
				SetForegroundWindow(gHwnd);
				EndDialog(hWnd, 0);
				return FALSE;
			}

			// Start the control handle off at the first edit box
			DWORD edit = IDC_EDIT1;

			// Save handles to the server stats control slots

			// Xian
			statHandles[65].name = GetDlgItem(hWnd, edit++);
			statHandles[65].cur = GetDlgItem(hWnd, edit++);
			statHandles[65].max = GetDlgItem(hWnd, edit++);
			statHandles[65].state = GetDlgItem(hWnd, edit++);

			// Aege
			statHandles[74].name = GetDlgItem(hWnd, edit++);
			statHandles[74].cur = GetDlgItem(hWnd, edit++);
			statHandles[74].max = GetDlgItem(hWnd, edit++);
			statHandles[74].state = GetDlgItem(hWnd, edit++);

			// Troy
			statHandles[76].name = GetDlgItem(hWnd, edit++);
			statHandles[76].cur = GetDlgItem(hWnd, edit++);
			statHandles[76].max = GetDlgItem(hWnd, edit++);
			statHandles[76].state = GetDlgItem(hWnd, edit++);

			// Athens
			statHandles[94].name = GetDlgItem(hWnd, edit++);
			statHandles[94].cur = GetDlgItem(hWnd, edit++);
			statHandles[94].max = GetDlgItem(hWnd, edit++);
			statHandles[94].state = GetDlgItem(hWnd, edit++);

			// Oasis
			statHandles[96].name = GetDlgItem(hWnd, edit++);
			statHandles[96].cur = GetDlgItem(hWnd, edit++);
			statHandles[96].max = GetDlgItem(hWnd, edit++);
			statHandles[96].state = GetDlgItem(hWnd, edit++);

			// Venice
			statHandles[102].name = GetDlgItem(hWnd, edit++);
			statHandles[102].cur = GetDlgItem(hWnd, edit++);
			statHandles[102].max = GetDlgItem(hWnd, edit++);
			statHandles[102].state = GetDlgItem(hWnd, edit++);

			// Greece
			statHandles[107].name = GetDlgItem(hWnd, edit++);
			statHandles[107].cur = GetDlgItem(hWnd, edit++);
			statHandles[107].max = GetDlgItem(hWnd, edit++);
			statHandles[107].state = GetDlgItem(hWnd, edit++);

			// Alps
			statHandles[113].name = GetDlgItem(hWnd, edit++);
			statHandles[113].cur = GetDlgItem(hWnd, edit++);
			statHandles[113].max = GetDlgItem(hWnd, edit++);
			statHandles[113].state = GetDlgItem(hWnd, edit++);

			// Olympus
			statHandles[114].name = GetDlgItem(hWnd, edit++);
			statHandles[114].cur = GetDlgItem(hWnd, edit++);
			statHandles[114].max = GetDlgItem(hWnd, edit++);
			statHandles[114].state = GetDlgItem(hWnd, edit++);

			// Tibet
			statHandles[132].name = GetDlgItem(hWnd, edit++);
			statHandles[132].cur = GetDlgItem(hWnd, edit++);
			statHandles[132].max = GetDlgItem(hWnd, edit++);
			statHandles[132].state = GetDlgItem(hWnd, edit++);

			// Babel
			statHandles[134].name = GetDlgItem(hWnd, edit++);
			statHandles[134].cur = GetDlgItem(hWnd, edit++);
			statHandles[134].max = GetDlgItem(hWnd, edit++);
			statHandles[134].state = GetDlgItem(hWnd, edit++);

			// RedSea
			statHandles[150].name = GetDlgItem(hWnd, edit++);
			statHandles[150].cur = GetDlgItem(hWnd, edit++);
			statHandles[150].max = GetDlgItem(hWnd, edit++);
			statHandles[150].state = GetDlgItem(hWnd, edit++);

			// Rome
			statHandles[151].name = GetDlgItem(hWnd, edit++);
			statHandles[151].cur = GetDlgItem(hWnd, edit++);
			statHandles[151].max = GetDlgItem(hWnd, edit++);
			statHandles[151].state = GetDlgItem(hWnd, edit++);

			// Sparta
			statHandles[152].name = GetDlgItem(hWnd, edit++);
			statHandles[152].cur = GetDlgItem(hWnd, edit++);
			statHandles[152].max = GetDlgItem(hWnd, edit++);
			statHandles[152].state = GetDlgItem(hWnd, edit++);

			// Eldorado
			statHandles[156].name = GetDlgItem(hWnd, edit++);
			statHandles[156].cur = GetDlgItem(hWnd, edit++);
			statHandles[156].max = GetDlgItem(hWnd, edit++);
			statHandles[156].state = GetDlgItem(hWnd, edit++);

			// Pacific
			statHandles[159].name = GetDlgItem(hWnd, edit++);
			statHandles[159].cur = GetDlgItem(hWnd, edit++);
			statHandles[159].max = GetDlgItem(hWnd, edit++);
			statHandles[159].state = GetDlgItem(hWnd, edit++);

			// Alexander
			statHandles[162].name = GetDlgItem(hWnd, edit++);
			statHandles[162].cur = GetDlgItem(hWnd, edit++);
			statHandles[162].max = GetDlgItem(hWnd, edit++);
			statHandles[162].state = GetDlgItem(hWnd, edit++);

			// Persia
			statHandles[165].name = GetDlgItem(hWnd, edit++);
			statHandles[165].cur = GetDlgItem(hWnd, edit++);
			statHandles[165].max = GetDlgItem(hWnd, edit++);
			statHandles[165].state = GetDlgItem(hWnd, edit++);

			// Zeus
			statHandles[166].name = GetDlgItem(hWnd, edit++);
			statHandles[166].cur = GetDlgItem(hWnd, edit++);
			statHandles[166].max = GetDlgItem(hWnd, edit++);
			statHandles[166].state = GetDlgItem(hWnd, edit++);

			// Poseidon
			statHandles[174].name = GetDlgItem(hWnd, edit++);
			statHandles[174].cur = GetDlgItem(hWnd, edit++);
			statHandles[174].max = GetDlgItem(hWnd, edit++);
			statHandles[174].state = GetDlgItem(hWnd, edit++);

			// Hercules
			statHandles[178].name = GetDlgItem(hWnd, edit++);
			statHandles[178].cur = GetDlgItem(hWnd, edit++);
			statHandles[178].max = GetDlgItem(hWnd, edit++);
			statHandles[178].state = GetDlgItem(hWnd, edit++);

			// Odin
			statHandles[179].name = GetDlgItem(hWnd, edit++);
			statHandles[179].cur = GetDlgItem(hWnd, edit++);
			statHandles[179].max = GetDlgItem(hWnd, edit++);
			statHandles[179].state = GetDlgItem(hWnd, edit++);

			// Mercury
			statHandles[180].name = GetDlgItem(hWnd, edit++);
			statHandles[180].cur = GetDlgItem(hWnd, edit++);
			statHandles[180].max = GetDlgItem(hWnd, edit++);
			statHandles[180].state = GetDlgItem(hWnd, edit++);

			// Mars
			statHandles[181].name = GetDlgItem(hWnd, edit++);
			statHandles[181].cur = GetDlgItem(hWnd, edit++);
			statHandles[181].max = GetDlgItem(hWnd, edit++);
			statHandles[181].state = GetDlgItem(hWnd, edit++);

			// Saturn
			statHandles[182].name = GetDlgItem(hWnd, edit++);
			statHandles[182].cur = GetDlgItem(hWnd, edit++);
			statHandles[182].max = GetDlgItem(hWnd, edit++);
			statHandles[182].state = GetDlgItem(hWnd, edit++);

			// Venus
			statHandles[183].name = GetDlgItem(hWnd, edit++);
			statHandles[183].cur = GetDlgItem(hWnd, edit++);
			statHandles[183].max = GetDlgItem(hWnd, edit++);
			statHandles[183].state = GetDlgItem(hWnd, edit++);

			// Uranus
			statHandles[187].name = GetDlgItem(hWnd, edit++);
			statHandles[187].cur = GetDlgItem(hWnd, edit++);
			statHandles[187].max = GetDlgItem(hWnd, edit++);
			statHandles[187].state = GetDlgItem(hWnd, edit++);

			// Pluto
			statHandles[188].name = GetDlgItem(hWnd, edit++);
			statHandles[188].cur = GetDlgItem(hWnd, edit++);
			statHandles[188].max = GetDlgItem(hWnd, edit++);
			statHandles[188].state = GetDlgItem(hWnd, edit++);

			// Neptune
			statHandles[190].name = GetDlgItem(hWnd, edit++);
			statHandles[190].cur = GetDlgItem(hWnd, edit++);
			statHandles[190].max = GetDlgItem(hWnd, edit++);
			statHandles[190].state = GetDlgItem(hWnd, edit++);

			// Hera
			statHandles[191].name = GetDlgItem(hWnd, edit++);
			statHandles[191].cur = GetDlgItem(hWnd, edit++);
			statHandles[191].max = GetDlgItem(hWnd, edit++);
			statHandles[191].state = GetDlgItem(hWnd, edit++);

			// Gaia
			statHandles[194].name = GetDlgItem(hWnd, edit++);
			statHandles[194].cur = GetDlgItem(hWnd, edit++);
			statHandles[194].max = GetDlgItem(hWnd, edit++);
			statHandles[194].state = GetDlgItem(hWnd, edit++);

		} break;

		case WM_TIMER:
		{
			switch(wParam)
			{
				// Ping packet for server stats
				case 1:
				{
					PacketBuilder builder;
					builder.SetOpcode(0x2002);
					PluginInjectClientToServer(myClient, builder.GetPacket(), false);
				} break;

				// Server stats request
				case 2:
				{
					// Check to see if we need to request stats
					if(needServerStats == TRUE)
					{
						PacketBuilder builder;
						builder.SetOpcode(0x6101);
						PluginInjectClientToServer(myClient, builder.GetPacket(), true);

						// Do not need to request it again
						needServerStats = FALSE;
					}
				} break;
			}
		} break;

		// network message from WSAAsyncSelect
		case WM_USER + 1:
		{
			// pass the event along
			OnNetworkEvent(wParam, WSAGETSELECTEVENT(lParam), WSAGETSELECTERROR(lParam));
		}
		break;

		default:
		{
			// Message not handled
			return FALSE;
		}
	}

	// Message handled
	return TRUE;
}

//-----------------------------------------------------------------------------
