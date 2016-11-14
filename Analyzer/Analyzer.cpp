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

#ifndef _INC_WINDOWSX
	#include <windowsx.h>
#endif

#ifndef _RICHEDIT_
	#include <Richedit.h>
#endif

#ifndef _ALGORITHM_
	#include <algorithm>
#endif

#ifndef _VECTOR_
	#include <vector>
#endif

#ifndef _FSTREAM_
	#include <fstream>
#endif

#include "resource.h"

//-----------------------------------------------------------------------------

// Window procedure for the GUI
INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//add a string to the rich edit control
void AppendText(bool doShow, char *cText);

// Format a packet for displaying
void DisplayPacket(bool direction, DWORD client, tPacket * packet, bool encrypted, bool injected);

// Returns an integer from a hex string
int HexStringToInteger(const char * str);

// Tokenizes a string into a vector
std::vector<std::string> TokenizeString(const std::string& str, const std::string& delim);

//-----------------------------------------------------------------------------

// Plugin functions to use from SR33
PluginInjectFunc PluginInjectServerToClient = 0;
PluginInjectFunc PluginInjectClientToServer = 0;

//-----------------------------------------------------------------------------

DWORD threadId = 0;
HANDLE hThread = 0;
HWND gHwnd = 0;

// DLL instance
HINSTANCE gInstance = 0;

HMODULE hRichLib = 0;
HWND hRichEdit = 0;
HFONT hf = 0;

//-----------------------------------------------------------------------------

// Log file
FILE * logFile = 0;

// Filter options
bool bShowC2S = true;
bool bShowS2C = true;
std::vector<DWORD> showOnlyOpcodes;
std::vector<DWORD> filterOpcodes;

// Save and load settings
void SaveFilter();
void LoadFilter();

// Ignore opcodes
bool AddIgnoreOpcode(DWORD opcode);
void RemoveIgnoreOpcode(DWORD opcode);
void ClearIgnoreOpcodes();

// Show only opcodes
bool AddAllowOpcode(DWORD opcode);
void RemoveAllowOpcode(DWORD opcode);
void ClearAllowOpcodes();

// Destination
void ShowCtoS(bool show);
void ShowStoC(bool show);
void ToggleCtoS();
void ToggleStoC();

// Get destination
BOOL GetStoC();
BOOL GetCtoS();

// Get lists of opcodes
std::vector<DWORD> GetAllowedOpcodes();
std::vector<DWORD> GetIgnoredOpcodes();

//-----------------------------------------------------------------------------

// GUI Thread
DWORD WINAPI GuiThread(LPVOID lParam)
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

			// Load the library once
			if(hRichLib == 0)
			{
				hRichLib = LoadLibrary("RICHED32.DLL");
				if(!hRichLib)
				{
					MessageBox(hWnd, "Loading RICHED32.DLL failed", "Error", MB_OK | MB_ICONEXCLAMATION);
					return FALSE;
				}
			}

			// Create the rich edit control
			DWORD Style = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL | ES_AUTOVSCROLL;
			hRichEdit = CreateWindowEx(NULL, "RichEdit", NULL, Style, 1, 1, 560, 570, hWnd, NULL, gInstance, NULL);
			if(!hRichEdit)
			{
				MessageBox(hWnd, "RichEdit creation failed", "Error", MB_OK | MB_ICONEXCLAMATION);
				return FALSE;
			}

			// Update the font 
			hf = CreateFont(16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "courier new");
			SendMessage(hRichEdit, WM_SETFONT, (WPARAM)hf, MAKELPARAM(TRUE, 0));

			// Load the filter
			LoadFilter();

			// Clear out the list boxes
			SendMessage(GetDlgItem(hWnd, IDC_LIST1), LB_RESETCONTENT, 0, 0);
			SendMessage(GetDlgItem(hWnd, IDC_LIST2), LB_RESETCONTENT, 0, 0);

			// update check boxes with the value from the ini
			Button_SetCheck(GetDlgItem(hWnd, IDC_CHECK2), GetCtoS());
			Button_SetCheck(GetDlgItem(hWnd, IDC_CHECK1), GetStoC());

			// Load ignore opcodes
			std::vector<DWORD> filterOpcodes = GetIgnoredOpcodes();
			for(size_t x = 0; x < filterOpcodes.size(); ++x)
			{
				char tmp[256] = {0};
				_snprintf(tmp, 255, "%X", filterOpcodes[x]);
				SendMessage(GetDlgItem(hWnd, IDC_LIST1), LB_ADDSTRING, 0, (LPARAM)tmp);
			}

			// Load allow opcodes
			std::vector<DWORD> showOnlyOpcodes = GetAllowedOpcodes();
			for(size_t x = 0; x < showOnlyOpcodes.size(); ++x)
			{
				char tmp[256] = {0};
				_snprintf(tmp, 255, "%X", showOnlyOpcodes[x]);
				SendMessage(GetDlgItem(hWnd, IDC_LIST2), LB_ADDSTRING, 0, (LPARAM)tmp);
			}

			// 4 characters max
			SendMessage(GetDlgItem(hWnd, IDC_EDIT1), EM_LIMITTEXT, 4, 0);
			SendMessage(GetDlgItem(hWnd, IDC_EDIT2), EM_LIMITTEXT, 4, 0);
		} break;

		// Commands
		case WM_COMMAND:
        {
			int button = LOWORD(wParam);
            switch (button)
            {
//
// Do not show opcodes
//
				// Add opcode
				case IDC_BUTTON1:
				{
					char tmp[8] = {0};
					GetWindowText(GetDlgItem(hWnd, IDC_EDIT1), tmp, 5);
					int length = GetWindowTextLength(GetDlgItem(hWnd, IDC_EDIT1));

					// Support 3 and 4 byte opcodes, do not think there are 2 or 1 byte opcodes :P
					if(length == 4) if(!isalnum(tmp[0]) || !isalnum(tmp[1]) || !isalnum(tmp[2]) || !isalnum(tmp[3])) break;
					if(length == 3) if(!isalnum(tmp[0]) || !isalnum(tmp[1]) || !isalnum(tmp[2])) break;
					if(length < 3) break;

					if(AddIgnoreOpcode(HexStringToInteger(tmp)))
					{
						SendMessage(GetDlgItem(hWnd, IDC_LIST1), LB_ADDSTRING, 0, (LPARAM)tmp);
					}
					SetWindowText(GetDlgItem(hWnd, IDC_EDIT1), "");
				} break;

				// Remove opcode
				case IDC_BUTTON2:
				{
					LRESULT sel = SendMessage(GetDlgItem(hWnd, IDC_LIST1), LB_GETCURSEL, 0, 0);
					if(sel != -1)
					{
						char tmp[1024] = {0};
						SendMessage(GetDlgItem(hWnd, IDC_LIST1), LB_GETTEXT, sel, (LPARAM)tmp);
						DWORD opcode = HexStringToInteger(tmp);
						RemoveIgnoreOpcode(opcode);
						SendMessage(GetDlgItem(hWnd, IDC_LIST1), LB_DELETESTRING, sel, 0);
					}
				} break;

				// Clear all
				case IDC_BUTTON3:
				{
					if(MessageBox(0, "Are you sure you wish to clear the \"Do Not Show Opcodes\" list?", "Confirmation", MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES)
					{
						SendMessage(GetDlgItem(hWnd, IDC_LIST1), LB_RESETCONTENT, 0, 0);
						ClearIgnoreOpcodes();
					}
				} break;

//
// show only opcodes
// 
				// Add opcode
				case IDC_BUTTON4:
				{
					char tmp[8] = {0};
					GetWindowText(GetDlgItem(hWnd, IDC_EDIT2), tmp, 5);
					int length = GetWindowTextLength(GetDlgItem(hWnd, IDC_EDIT2));

					// Support 3 and 4 byte opcodes, do not think there are 2 or 1 byte opcodes :P
					if(length == 4) if(!isalnum(tmp[0]) || !isalnum(tmp[1]) || !isalnum(tmp[2]) || !isalnum(tmp[3])) break;
					if(length == 3) if(!isalnum(tmp[0]) || !isalnum(tmp[1]) || !isalnum(tmp[2])) break;
					if(length < 3) break;

					if(AddAllowOpcode(HexStringToInteger(tmp)))
					{
						SendMessage(GetDlgItem(hWnd, IDC_LIST2), LB_ADDSTRING, 0, (LPARAM)tmp);
					}
					SetWindowText(GetDlgItem(hWnd, IDC_EDIT2), "");
				} break;

				// Remove opcode
				case IDC_BUTTON5:
				{
					LRESULT sel = SendMessage(GetDlgItem(hWnd, IDC_LIST2), LB_GETCURSEL, 0, 0);
					if(sel != -1)
					{
						char tmp[1024] = {0};
						SendMessage(GetDlgItem(hWnd, IDC_LIST2), LB_GETTEXT, sel, (LPARAM)tmp);
						DWORD opcode = HexStringToInteger(tmp);
						RemoveAllowOpcode(opcode);
						SendMessage(GetDlgItem(hWnd, IDC_LIST2), LB_DELETESTRING, sel, 0);
					}
				} break;

				// Clear all
				case IDC_BUTTON6:
				{
					if(MessageBox(0, "Are you sure you wish to clear the \"Show Only Opcodes\" list?", "Confirmation", MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES)
					{
						SendMessage(GetDlgItem(hWnd, IDC_LIST2), LB_RESETCONTENT, 0, 0);
						ClearAllowOpcodes();
					}
				} break;
//
//  Extra option
//
				// Clear the richedit
				case IDC_BUTTON7:
				{
					SendMessage(hRichEdit, WM_SETTEXT, 0, 0);
				} break;
//
//  General filtering
//
				// Toggle destinations
				case IDC_CHECK1:
				{
					ToggleStoC();
				} break;

				case IDC_CHECK2:
				{
					ToggleCtoS();
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

void OnLoad()
{
	hThread = CreateThread(0, 0, GuiThread, 0, 0, &threadId);
	char filename[MAX_PATH + 1] = {0};
	CreateDirectory("logs", 0);
	_snprintf(filename, MAX_PATH, "logs\\%i.txt", GetTickCount());
	logFile = fopen(filename, "w");
}

//-----------------------------------------------------------------------------

void OnUnload()
{
	fclose(logFile);
	logFile = 0;
	DeleteObject(hf);
	TerminateThread(hThread, 0);
	hRichLib = 0;
	FreeLibrary(hRichLib);
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
	// Display the packet
	DisplayPacket(0, client, packet, encrypted, injected);

	// Accept the packet
	return true;
}

//-----------------------------------------------------------------------------

// Server to Client packet
bool OnServerToClient(DWORD client, tPacket * packet, bool encrypted, bool injected)
{
	// Display the packet
	DisplayPacket(1, client, packet, encrypted, injected);

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

	if(args[0] == ".analyzer")
	{
		if(hThread == 0)
			hThread = CreateThread(0, 0, GuiThread, 0, 0, &threadId);
	}
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

//add a string to the rich edit control
void AppendText(bool doShow, char *cText)
{
	if(logFile)	fprintf(logFile, "%s\n", cText);
	if(doShow)
	{
		if(!hRichEdit)
			return;
		CHARRANGE cr = {-1, -1};
		SendMessage(hRichEdit,EM_EXSETSEL,0,(LPARAM)&cr);
		SendMessage(hRichEdit,EM_REPLACESEL, 0, (LPARAM)cText);
		SendMessage(hRichEdit,WM_VSCROLL, /*SB_LINEDOWN*/ SB_BOTTOM, (LPARAM)NULL);
	}
}

//-----------------------------------------------------------------------------

// Format a packet for displaying
void DisplayPacket(bool direction, DWORD client, tPacket * packet, bool encrypted, bool injected)
{
	static char msg[131072] = {0};
	bool doShow = false;

	// CS
	if(direction == false)
	{
		bool b1 = bShowC2S;
		bool b2 = (showOnlyOpcodes.size() == 0) || (std::find(showOnlyOpcodes.begin(), showOnlyOpcodes.end(), packet->opcode) != showOnlyOpcodes.end());
		bool b3 = (std::find(filterOpcodes.begin(), filterOpcodes.end(), packet->opcode) == filterOpcodes.end());
		bool b4 = std::find(showOnlyOpcodes.begin(), showOnlyOpcodes.end(), packet->opcode) != showOnlyOpcodes.end();
		if((b1 && b2 && b3 && !b4) || b4)
		{
			doShow = true;
		}
		_snprintf(msg, 1023, "[%X][Client->Server][%i][%i][%.2X][%.2X]\n", client, encrypted, injected, packet->opcode, packet->size);
		AppendText(doShow, msg);
	}
	// SC
	else
	{
		bool b1 = bShowS2C;
		bool b2 = (showOnlyOpcodes.size() == 0) || (std::find(showOnlyOpcodes.begin(), showOnlyOpcodes.end(), packet->opcode) != showOnlyOpcodes.end());
		bool b3 = (std::find(filterOpcodes.begin(), filterOpcodes.end(), packet->opcode) == filterOpcodes.end());
		bool b4 = std::find(showOnlyOpcodes.begin(), showOnlyOpcodes.end(), packet->opcode) != showOnlyOpcodes.end();
		if((b1 && b2 && b3 && !b4) || b4)
		{
			doShow = true;
		}
		_snprintf(msg, 1023, "[%X][Server->Client][%i][%i][%.2X][%.2X]\n", client, encrypted, injected, packet->opcode, packet->size);
		AppendText(doShow, msg);
	}

	int outputsize = packet->size;
	if(outputsize % 16 != 0) outputsize += (16 - outputsize % 16);
	if(outputsize == 0) outputsize = 16;
	int ctr = 0;
	char ch[17] = {0};
	int x1 = 0;
	for(int x = 0; x < outputsize; ++x)
	{
		BYTE b;
		if(x < packet->size)
		{
			b = packet->data[x];
			sprintf(msg + ctr, "%.2X ", b);
			ch[x1] = (isprint(b) && !isspace(b) ? b : '.');
		}
		else
		{
			sprintf(msg + ctr, "   ");
			ch[x1] = '.';
		}
		x1++;
		ctr += 3;
		if((x+1) % 16 == 0)
		{
			x1 = 0;
			sprintf(msg + ctr, "  %s", ch);
			ctr += 18;
			sprintf(msg + ctr, "\r\n");
			ctr += 2;
		}
	}
	strcat(msg, "\n");
	AppendText(doShow, msg);
}

//-----------------------------------------------------------------------------

void LoadFilter()
{
	filterOpcodes.clear();
	showOnlyOpcodes.clear();
	DWORD tmp = 0;
	DWORD tmp2 = 0;
	std::ifstream inFile("settings.ini");
	if(inFile.is_open() == false)
		return;
	inFile >> bShowC2S;
	inFile >> bShowS2C;
	inFile >> tmp;
	for(size_t x = 0; x < tmp; ++x)
	{
		inFile >> tmp2;
		filterOpcodes.push_back(tmp2);
	}
	inFile >> tmp;
	for(size_t x = 0; x < tmp; ++x)
	{
		inFile >> tmp2;
		showOnlyOpcodes.push_back(tmp2);
	}
	inFile.close();
}

//-----------------------------------------------------------------------------

void SaveFilter()
{
	std::ofstream outFile("settings.ini");
	outFile << bShowC2S << std::endl;
	outFile << bShowS2C << std::endl;
	outFile << (int)filterOpcodes.size() << std::endl;
	for(size_t x = 0; x < filterOpcodes.size(); ++x)
	{
		outFile << filterOpcodes[x] << std::endl;
	}
	outFile << (int)(showOnlyOpcodes.size()) << std::endl;
	for(size_t x = 0; x < showOnlyOpcodes.size(); ++x)
	{
		outFile << showOnlyOpcodes[x] << std::endl;
	}
	outFile.close();
}

//-----------------------------------------------------------------------------

bool AddIgnoreOpcode(DWORD opcode)
{
	if(std::find(filterOpcodes.begin(), filterOpcodes.end(), opcode) == filterOpcodes.end())
	{
		filterOpcodes.push_back(opcode);
		SaveFilter();
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------

void RemoveIgnoreOpcode(DWORD opcode)
{
	std::vector<DWORD>::iterator itr = std::find(filterOpcodes.begin(), filterOpcodes.end(), opcode);
	if(itr != filterOpcodes.end())
	{
		filterOpcodes.erase(itr);
		SaveFilter();
	}
}

//-----------------------------------------------------------------------------

void ClearIgnoreOpcodes()
{
	filterOpcodes.clear();
	SaveFilter();
}

//-----------------------------------------------------------------------------

bool AddAllowOpcode(DWORD opcode)
{
	if(std::find(showOnlyOpcodes.begin(), showOnlyOpcodes.end(), opcode) == showOnlyOpcodes.end())
	{
		showOnlyOpcodes.push_back(opcode);
		SaveFilter();
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------

void RemoveAllowOpcode(DWORD opcode)
{
	std::vector<DWORD>::iterator itr = std::find(showOnlyOpcodes.begin(), showOnlyOpcodes.end(), opcode);
	if(itr != showOnlyOpcodes.end())
	{
		showOnlyOpcodes.erase(itr);
		SaveFilter();
	}
}

//-----------------------------------------------------------------------------

void ClearAllowOpcodes()
{
	showOnlyOpcodes.clear();
	SaveFilter();
}

//-----------------------------------------------------------------------------

// Destination filtering
void ShowCtoS(bool show)
{
	bShowC2S = show;
	SaveFilter();
}

void ShowStoC(bool show)
{
	bShowS2C = show;
	SaveFilter();
}

//-----------------------------------------------------------------------------

void ToggleCtoS()
{
	bShowC2S = !bShowC2S;
	SaveFilter();
}

void ToggleStoC()
{
	bShowS2C = !bShowS2C;
	SaveFilter();
}

//-----------------------------------------------------------------------------

BOOL GetStoC()
{
	return bShowS2C;
}

BOOL GetCtoS()
{
	return bShowC2S;
}

//-----------------------------------------------------------------------------

// Get lists of opcodes
std::vector<DWORD> GetAllowedOpcodes()
{
	return showOnlyOpcodes;
}

std::vector<DWORD> GetIgnoredOpcodes()
{
	return filterOpcodes;
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
