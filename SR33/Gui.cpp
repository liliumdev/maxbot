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

#ifndef NETWORK_H_
	#include "Network.h"
#endif

#ifndef SR33_H_
	#include "SR33.h"
#endif

#ifndef _WINDOWS_
	#include <windows.h>
#endif

#ifndef _INC_WINDOWSX
	#include <windowsx.h>
#endif

#ifndef USERDATA_H_
	#include "UserData.h"
#endif

#include "resource.h"

//-----------------------------------------------------------------------------

// Window procedure for the GUI
INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

//-----------------------------------------------------------------------------

// Error handling function
void OnError(const char * error)
{
	MessageBox(0, error, "Error", MB_ICONERROR);
}

//-----------------------------------------------------------------------------

// Main entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// Proxy object
	SilkroadProxy proxy1;

	// Setup the SilkroadProxy class
	SilkroadProxy::Initialize();

	// Startup core network library
	if(Network_InitializeCore(OnError) == false)
	{
		// Cleanup the SilkroadProxy class
		SilkroadProxy::Deinitialize();

		// Standard return
		return 0;
	}

	// Create the proxy
	if(proxy1.Create(15778, 15780) == false)
	{
		// Cleanup the SilkroadProxy class
		SilkroadProxy::Deinitialize();

		// Standard return
		return 0;
	}

	// Create the gui
	CreateDialog(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// Load maxBot
	LoadPlugin("Plugin.dll");

	// Set a hook for the server
	InstallWindowsHook(true);

	// Process while we have a message queue
	MSG Msg = {0};
	while(GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	// Free the hook for the server
	InstallWindowsHook(false);

	// Destroy the proxy
	proxy1.Destroy();

	// Cleanup the SilkroadProxy class
	SilkroadProxy::Deinitialize();

	// Free core network library
	Network_DeinitializeCore();

	// Standard return
	return 0;
}

//-----------------------------------------------------------------------------

// Window procedure for the GUI -> your C++ gui skills go here :)
INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Handle the message
	switch(uMsg) 
    {
		// Dialog initialize
		case WM_INITDIALOG:
        {
			// Change as you change which plugins are auto loaded if you wish

			// Client hooking
			Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO1), BST_CHECKED);
			//Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO2), BST_CHECKED);

			// Serverstats
			//Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO3), BST_CHECKED);
			Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO4), BST_CHECKED);

			// Analyzer
			//Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO5), BST_CHECKED);
			Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO6), BST_CHECKED);

			// Injector
			//Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO7), BST_CHECKED);
			Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO8), BST_CHECKED);

			SendMessage(GetDlgItem(hWnd, IDC_LIST1), LB_RESETCONTENT, 0, 0);
			std::vector<std::string> names = GetLoadedPluginNames();
			for(size_t x = 0; x < names.size(); ++x)
			{
				SendMessage(GetDlgItem(hWnd, IDC_LIST1), LB_ADDSTRING, 0, (LPARAM)names[x].c_str());
			}

			// How long before the list box is updated
			SetTimer(hWnd, 1, 3500, 0);
		}
		break;

		case WM_TIMER:
		{
			SendMessage(GetDlgItem(hWnd, IDC_LIST1), LB_RESETCONTENT, 0, 0);
			std::vector<std::string> names = GetLoadedPluginNames();
			for(size_t x = 0; x < names.size(); ++x)
			{
				SendMessage(GetDlgItem(hWnd, IDC_LIST1), LB_ADDSTRING, 0, (LPARAM)names[x].c_str());
			}
		} break;

		// Commands
		case WM_COMMAND:
        {
			int button = LOWORD(wParam);
            switch (button)
            {
				// Exit
				case ID_FILE_EXIT:
				case IDCANCEL:
				{
					if(MessageBox(0, "Do you really wish to exit the program?", "Exit confirmation", MB_ICONQUESTION | MB_YESNO) == IDYES)
					{
						PostQuitMessage(0);
					}
				} break;

				case IDC_RADIO1:
				{
					InstallWindowsHook(TRUE);
					Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO1), BST_CHECKED);
					Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO2), BST_UNCHECKED);
				} break;

				case IDC_RADIO2:
				{
					InstallWindowsHook(FALSE);
					Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO2), BST_CHECKED);
					Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO1), BST_UNCHECKED);
				} break;

				case IDC_RADIO3:
				{
					LoadPlugin("serverstats");
					Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO3), BST_CHECKED);
					Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO4), BST_UNCHECKED);
				} break;

				case IDC_RADIO4:
				{
					UnloadPlugin("serverstats");
					Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO4), BST_CHECKED);
					Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO3), BST_UNCHECKED);
				} break;

				case IDC_RADIO5:
				{
					LoadPlugin("analyzer");
					Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO5), BST_CHECKED);
					Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO6), BST_UNCHECKED);
				} break;

				case IDC_RADIO6:
				{
					UnloadPlugin("analyzer");
					Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO6), BST_CHECKED);
					Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO5), BST_UNCHECKED);
				} break;

				case IDC_BUTTON1:
				{
					char name[256] = {0};
					GetWindowText(GetDlgItem(hWnd, IDC_EDIT1), name, 255);
					if(strlen(name) == 0)
						break;
					if(LoadPlugin(name) == 0)
						MessageBox(0, "The plugin was not loaded.", "Error", MB_ICONERROR);
				} break;

				case IDC_BUTTON2:
				{
					char name[256] = {0};
					GetWindowText(GetDlgItem(hWnd, IDC_EDIT1), name, 255);
					if(strlen(name) == 0)
						break;
					if(UnloadPlugin(name) == 0)
						MessageBox(0, "The plugin was not unloaded.", "Error", MB_ICONERROR);
				} break;

				case IDC_RADIO7:
				{
					LoadPlugin("injector");
					Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO7), BST_CHECKED);
					Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO8), BST_UNCHECKED);
				} break;

				case IDC_RADIO8:
				{
					UnloadPlugin("injector");
					Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO8), BST_CHECKED);
					Button_SetCheck(GetDlgItem(hWnd, IDC_RADIO7), BST_UNCHECKED);
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
