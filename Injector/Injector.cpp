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

#ifndef _INC_WINDOWSX
	#include <windowsx.h>
#endif

#ifndef _ALGORITHM_
	#include <algorithm>
#endif

#include "resource.h"

//-----------------------------------------------------------------------------

// Tokenizes a string into a vector
std::vector<std::string> TokenizeString(const std::string& str, const std::string& delim);

// Window procedure for the GUI
INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Returns an integer from a hex string
int HexStringToInteger(const char * str);

//-----------------------------------------------------------------------------

// Plugin functions to use from SR33
PluginInjectFunc PluginInjectServerToClient = 0;
PluginInjectFunc PluginInjectClientToServer = 0;

// DLL instance
HINSTANCE gInstance = 0;
DWORD threadId = 0;
HANDLE hThread = 0;
HWND gHwnd = 0;

CRITICAL_SECTION clientCS;
std::vector<DWORD> clientList;

//-----------------------------------------------------------------------------

// Stats Thread
DWORD WINAPI InjectorThread(LPVOID lParam)
{
	// Create the GUI
	gHwnd = CreateDialog(gInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
	if(gHwnd == NULL)
	{
		MessageBox(NULL, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

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
	hThread = CreateThread(0, 0, InjectorThread, 0, 0, &threadId);
	InitializeCriticalSection(&clientCS);
}

//-----------------------------------------------------------------------------

// Called when the plugin is unloaded
void OnUnload()
{
	TerminateThread(hThread, 0);
	DeleteCriticalSection(&clientCS);
}

//-----------------------------------------------------------------------------

// Called when the client is connected to the server
void OnConnect(DWORD client)
{
	EnterCriticalSection(&clientCS);
	clientList.push_back(client);
	LeaveCriticalSection(&clientCS);
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
	EnterCriticalSection(&clientCS);
	clientList.erase(std::find(clientList.begin(), clientList.end(), client));
	LeaveCriticalSection(&clientCS);
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

			// Best page ;)
			SetWindowText(GetDlgItem(hWnd, IDC_EDIT3), "http://www.paulschou.com/tools/xlate/");

			// Set default button checks
			Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO1), BST_CHECKED);

			// 4 characters max
			SendMessage(GetDlgItem(hWnd, IDC_EDIT1), EM_LIMITTEXT, 4, 0);

			// Time for updating clients connected
			SetTimer(hWnd, 1, 1000, 0);
		} break;

		case WM_TIMER:
		{
			switch(wParam)
			{
				// Update clients
				case 1:
				{
					char tmp[1024] = {0};

					// Get the selection
					LRESULT sel = SendMessage(GetDlgItem(hWnd, IDC_LIST1), LB_GETCURSEL, 0, 0);

					// Get the item selected
					if(sel != -1) SendMessage(GetDlgItem(hWnd, IDC_LIST1), LB_GETTEXT, sel, (LPARAM)tmp);

					// Clear listbox
					SendMessage(GetDlgItem(hWnd, IDC_LIST1), LB_RESETCONTENT, 0, 0);

					// Make sure the client list does not change
					EnterCriticalSection(&clientCS);

					// Loop through all clients
					for(size_t x = 0; x < clientList.size(); ++x)
					{
						char entry[1024] = {0};
						_snprintf(entry, 1023, "%i", clientList[x]);
						SendMessage(GetDlgItem(hWnd, IDC_LIST1), LB_ADDSTRING, 0, (LPARAM)entry);
					}

					// Done with multithreaded access
					LeaveCriticalSection(&clientCS);

					// If we had a previous selection
					if(sel != -1)
					{
						// See if we can find the item again
						sel = SendMessage(GetDlgItem(hWnd, IDC_LIST1), LB_FINDSTRING, 0, (LPARAM)tmp);

						// If so, select it again
						if(sel != -1) SendMessage(GetDlgItem(hWnd, IDC_LIST1), LB_SETCURSEL, sel, 0);
					}
				} break;
			}
		} break;

		// Commands
		case WM_COMMAND:
        {
			int button = LOWORD(wParam);
            switch (button)
            {
				// Clear
				case IDC_BUTTON1:
				{
					SetWindowText(GetDlgItem(hWnd, IDC_EDIT1), "");
					SetWindowText(GetDlgItem(hWnd, IDC_EDIT2), "");
					SetWindowText(GetDlgItem(hWnd, IDC_EDIT3), "http://www.paulschou.com/tools/xlate/");
					Button_SetCheck(GetDlgItem(hWnd, IDC_CHECK1), BST_UNCHECKED);
				} break;

				// Inject
				case IDC_BUTTON2:
				{
					char strOpcode[8] = {0};
					char strData[8192] = {0};
					char tmpData[8192] = {0};
					WORD opcode = 0;
					tPacket packet = {0};

					// Get text
					GetWindowText(GetDlgItem(hWnd, IDC_EDIT1), strOpcode, 8);
					GetWindowText(GetDlgItem(hWnd, IDC_EDIT2), strData, 8191);

					// Verify data
					int tmpIndex = 0;
					size_t size = strlen(strData);
					for(size_t x = 0; x < size; ++x)
					{
						int c = strData[x];
						if(isalpha(c) || isdigit(c))
						{
							tmpData[tmpIndex++] = c;
						}
						else if(isspace(c))
						{
							// Ignore it
						}
						else
						{
							MessageBox(0, "The packet contains invalid characters.", "Error", MB_ICONERROR);
							break;
						}
					}

					// Only 4 bytes for opcode
					strOpcode[4] = 0;

					// Store it
					packet.opcode = HexStringToInteger(strOpcode);

					// Get the total size
					size = strlen(tmpData);
					if(!(size == 0 || size % 2 == 0))
					{
						MessageBox(0, "The packet contains an invalid amount of characters.", "Error", MB_ICONERROR);
						break;
					}

					// Loop through and save real data
					tmpIndex = 0;
					for(size_t x = 0; x < size; x += 2)
					{
						char tmp[3] = {tmpData[x], tmpData[x + 1], 0};
						packet.data[tmpIndex++] = HexStringToInteger(tmp);
						packet.size++;
					}

					// Check to see if the packet should be encrypted
					bool encrypted = Button_GetCheck(GetDlgItem(hWnd, IDC_CHECK1));

					// Get the current selection
					LRESULT sel = SendMessage(GetDlgItem(hWnd, IDC_LIST1), LB_GETCURSEL, 0, 0);
					if(sel == -1)
					{
						MessageBox(0, "Please select a client to inject into.", "Error", MB_ICONERROR);
						break;
					}

					// Client to inject into
					DWORD client = 0;

					// Get the item selected
					char tmp[1024] = {0};
					SendMessage(GetDlgItem(hWnd, IDC_LIST1), LB_GETTEXT, sel, (LPARAM)tmp);

					// Find the client
					EnterCriticalSection(&clientCS);
					if(std::find(clientList.begin(), clientList.end(), atoi(tmp)) != clientList.end())
						client = atoi(tmp);
					LeaveCriticalSection(&clientCS);

					// Make sure the pointer is valid
					if(client == 0)
					{
						MessageBox(0, "This client cannot be injected into.", "Error", MB_ICONERROR);
						break;
					}

					// S->C
					if(Button_GetCheck(GetDlgItem(hWnd, IDC_RADIO1)) == BST_CHECKED)
					{
						PluginInjectServerToClient(client, &packet, encrypted);
					}

					// C->S
					else if(Button_GetCheck(GetDlgItem(hWnd, IDC_RADIO2)) == BST_CHECKED)
					{
						PluginInjectClientToServer(client, &packet, encrypted);
					}

					// None selected
					else
					{
						MessageBox(0, "Please select the packet destination.", "Error", MB_ICONERROR);
						break;
					}
				} break;
			}
		} break;

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

// Returns an integer from a hex string
int HexStringToInteger(const char * str)
{
	size_t length = strlen(str);
	char * strUpper = new char[length + 1];
	memcpy(strUpper, str, length);
	strUpper[length] = 0;
	for(size_t x = 0; x < length; ++x)
		strUpper[x] = toupper(strUpper[x]);
	// http://www.codeproject.com/string/hexstrtoint.asp
	struct CHexMap
	{
		CHAR chr;
		int value;
	};
	const int HexMapL = 16;
	CHexMap HexMap[HexMapL] =
	{
		{'0', 0}, {'1', 1},
		{'2', 2}, {'3', 3},
       	{'4', 4}, {'5', 5},
		{'6', 6}, {'7', 7},
		{'8', 8}, {'9', 9},
		{'A', 10}, {'B', 11},
		{'C', 12}, {'D', 13},
		{'E', 14}, {'F', 15}
	};
	CHAR *mstr = strUpper;
	CHAR *s = mstr;
	int result = 0;
	if (*s == '0' && *(s + 1) == 'X') s += 2;
	bool firsttime = true;
	while (*s != '\0')
	{
		bool found = false;
		for (int i = 0; i < HexMapL; i++)
		{
			if (*s == HexMap[i].chr)
			{
				if (!firsttime) 
					result <<= 4;
				result |= HexMap[i].value;
				found = true;
				break;
			}
		}
		if (!found) break;
		s++;
		firsttime = false;
	}
	delete [] strUpper;
	return result;
}

//-----------------------------------------------------------------------------
