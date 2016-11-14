/******************************************************************************
Copyright (c) 2009 maxproject
http://maxproject-online.net

# Research started on 14-02-2009. 

Head (and only) developer currently is Ahmed Popovic (mafioso1930).
Bot will be free to use in the first month or two after the release.
Features fast grinding, smart skilling, training place (to be implemented) 
and a lot more. Made for maxproject and Stealthex. You will have to register
at maxproject in order to use it. Enjoy.

******************************************************************************/

#include <fstream>
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <algorithm>
#include <iostream>
#include <io.h>
#include <tchar.h>
#include <malloc.h>
#include <cmath>
#include <windowsx.h>
#include <stdlib.h>
#include "cbot.h"

//-----------------------------------------------------------------------------

// Tokenizes a string into a vector
std::vector<std::string> TokenizeString(const std::string& str, const std::string& delim);

//-----------------------------------------------------------------------------

// Window procedure for the GUI
INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Window procedure for the skill dlg
INT_PTR CALLBACK SkillDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Window procedure for the mob dlg
INT_PTR CALLBACK MobDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

DWORD threadId = 0;
HANDLE hThread = 0;
HWND gHwnd = 0;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Everything here is probably self-explainable
void IncreaseINT(DWORD client);						
void IncreaseSTR(DWORD client);
void JoinMatchParty(DWORD party, DWORD client);
void InviteToParty(DWORD object, DWORD client);
void LeaveParty(DWORD client);
void Emote(BYTE emoteType, DWORD client);
void GrabItem(DWORD object, DWORD client);
void CastImbueOrBuff(DWORD skill, DWORD client);
void SkillAtkMonster(DWORD object, DWORD skill);
void NormalAtkMonster(DWORD object, DWORD client);
void SelectMonster(DWORD object, DWORD client);
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// Plugin functions to use from SR33
PluginInjectFunc PluginInjectServerToClient = 0;
PluginInjectFunc PluginInjectClientToServer = 0;

//-----------------------------------------------------------------------------

// Create console
void CreateConsole(const char *winTitle);

// DLL instance
HINSTANCE gInstance = 0;

// Bot class
cBot bot;

// Hunt
void Hunt(DWORD client);

HWND inventory = 0;
HWND skillsList = 0;
HWND mobList = 0;
HWND mobCoords = 0;
HWND mobIDs = 0;
HWND mobDistance = 0;

HWND skillsHwnd = 0;
HWND mobsHwnd = 0;

//-----------------------------------------------------------------------------

// Stats Thread
DWORD WINAPI MaxThread(LPVOID lParam)
{
	bot.InitializeTextData();

	// Create the GUI
	gHwnd = CreateDialog(gInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);
	if(gHwnd == NULL)
	{
		MessageBox(NULL, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	skillsHwnd = CreateDialog(gInstance, MAKEINTRESOURCE(IDD_SKILLDLG), NULL, SkillDlgProc);
	if(gHwnd == NULL)
	{
		MessageBox(NULL, "Window #2 creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	mobsHwnd = CreateDialog(gInstance, MAKEINTRESOURCE(IDD_MOBDLG), NULL, MobDlgProc);
	if(gHwnd == NULL)
	{
		MessageBox(NULL, "Window #3 creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	ShowWindow(skillsHwnd, SW_HIDE);
	ShowWindow(mobsHwnd, SW_HIDE);

	inventory = GetDlgItem(gHwnd, IDC_INVENTORY);
	skillsList = GetDlgItem(skillsHwnd, IDC_SKILLSLIST);
	mobList = GetDlgItem(mobsHwnd, IDC_MOBLIST);
	mobIDs = GetDlgItem(mobsHwnd, IDC_MOBIDS);
	mobCoords = GetDlgItem(mobsHwnd, IDC_MOBCOORDS);
	mobDistance = GetDlgItem(mobsHwnd, IDC_MOBDIST);

	bot.gHwnd = gHwnd;
	bot.skillsHwnd = skillsHwnd;
	bot.mobsHwnd = mobsHwnd;

	// Process while we have a message queue
	MSG Msg = {0};
	while(GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	//-----------------------------------------------------------------------------

	// Standard return
	return 0;
}

//-----------------------------------------------------------------------------

// Called when the plugin is loaded
void OnLoad()
{
	CreateConsole("maxdebug");														// Create debug window
	hThread = CreateThread(0, 0, MaxThread, 0, 0, &threadId);						// Start the main thread
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
	// Hunt
	if(bot.hunt == true && bot.hunting == false)
		bot.Hunt(client);

	switch(packet->opcode)
	{
		// Character data packet
		case 0x32B3:
			bot.Parse32B3(packet);
			break;

		// Pre-group spawn
		case 0x30CB:
			bot.Parse30CB(packet);
			break;

		// Group spawn
		case 0x3417:
			bot.Parse3417(packet);
			break;

		// End group spawn
		case 0x330A:
			bot.Parse330A(packet);
			break;

		// Object action
		case 0x3122:
			bot.Parse3122(packet);
			break;

		// Character ID
		case 0x32A6:
			bot.Parse32A6(packet);
			break;

		// Object movement
		case 0xB738:
			bot.ParseB738(packet);
			break;

		case 0xB245:
			bot.ParseB245(packet);
			break;
	}

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
}

//-----------------------------------------------------------------------------

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
			// Do stuff on dialog initialization here
		} break;

		// Commands
		case WM_COMMAND:
		{
			int lID = LOWORD(wParam);
			int hID = HIWORD(wParam);
			switch(lID)
			{
				case IDC_CONFSKILLS:
				{
					ShowWindow(skillsHwnd, SW_SHOW);
				} break;
				case IDC_MOBSAROUND:
				{
					ShowWindow(mobsHwnd, SW_SHOW);
				} break;
				case IDC_START:
				{
					bot.hunt = true;
					EnableWindow(GetDlgItem(gHwnd, IDC_START), false);
					EnableWindow(GetDlgItem(gHwnd, IDC_STOP), true);					
				} break;
				case IDC_STOP:
				{
					bot.hunt = false;
					bot.hunting = false;
					EnableWindow(GetDlgItem(gHwnd, IDC_STOP), false);
					EnableWindow(GetDlgItem(gHwnd, IDC_START), true);
				} break;
			}
		} break;

		case WM_TIMER:
		{
			// Handle timers here
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

// Window procedure for the skill dialog
INT_PTR CALLBACK SkillDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Handle the message
	switch(uMsg) 
    {
		// Dialog initialize
		case WM_INITDIALOG:
        {
			// Do stuff on dialog initialization here
		} break;

		// Commands
		case WM_COMMAND:
		{
			int lID = LOWORD(wParam);
			int hID = HIWORD(wParam);
			switch(lID)
			{
				case IDC_SKILLSSAVE:
				{
					ShowWindow(skillsHwnd, SW_HIDE);
				} break;

				case IDCANCEL:
				{
					ShowWindow(skillsHwnd, SW_HIDE);
				}break;
			}
		} break;

		case WM_TIMER:
		{
			// Handle timers here
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

// Window procedure for the skill dialog
INT_PTR CALLBACK MobDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Handle the message
	switch(uMsg) 
    {
		// Dialog initialize
		case WM_INITDIALOG:
        {
			// Do stuff on dialog initialization here
		} break;

		// Commands
		case WM_COMMAND:
		{
			int lID = LOWORD(wParam);
			int hID = HIWORD(wParam);
			switch(lID)
			{
				case IDCANCEL:
				{
					ShowWindow(mobsHwnd, SW_HIDE);
				} break;
			}
		} break;

		case WM_TIMER:
		{
			// Handle timers here
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

// Create console
void CreateConsole(const char *winTitle)
{
	// http://www.gamedev.net/community/forums/viewreply.asp?ID=1958358
	int hConHandle = 0;
	HANDLE lStdHandle = 0;
	FILE *fp = 0 ;

	AllocConsole();
	if(winTitle)
		SetConsoleTitle(winTitle);

	// redirect unbuffered STDOUT to the console
	lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle(PtrToUlong(lStdHandle), _O_TEXT);
	fp = _fdopen(hConHandle, "w");
	*stdout = *fp;
	setvbuf(stdout, NULL, _IONBF, 0);

	// redirect unbuffered STDIN to the console
	lStdHandle = GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle(PtrToUlong(lStdHandle), _O_TEXT);
	fp = _fdopen(hConHandle, "r");
	*stdin = *fp;
	setvbuf(stdin, NULL, _IONBF, 0);

	// redirect unbuffered STDERR to the console
	lStdHandle = GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle(PtrToUlong(lStdHandle), _O_TEXT);
	fp = _fdopen(hConHandle, "w");
	*stderr = *fp;
	setvbuf(stderr, NULL, _IONBF, 0);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Attacking
//-----------------------------------------------------------------------------
// Select monster
void SelectMonster(DWORD object, DWORD client)
{
	PacketBuilder b;

	b.SetOpcode(0x745A);			// 0x745A - select target
	b.AppendDword(object);			// Object to select

	PluginInjectClientToServer(client, b.GetPacket(), false);
}

// Normal attack monster
void NormalAtkMonster(DWORD object, DWORD client)
{
	PacketBuilder b;
	
	b.SetOpcode(0x72CD);			// 0x72CD - action packet
	b.AppendByte(0x01);				// Normal attack		
	b.AppendByte(0x01);				// Normal attack
	b.AppendByte(0x01);				// Normal attack
	b.AppendDword(object);			// Object to attack

	PluginInjectClientToServer(client, b.GetPacket(), false);
}

// Cast imbue
void SkillAtkMonster(DWORD object, DWORD skill, DWORD client)
{
	PacketBuilder b;
	
	b.SetOpcode(0x72CD);			// 0x72CD - action packet
	b.AppendByte(0x01);				// Skill action			
	b.AppendByte(0x04);				// Skill action
	b.AppendDword(skill);			// Imbue to cast
	b.AppendByte(0x01);				// Has target
	b.AppendDword(object);			// Object to attack

	PluginInjectClientToServer(client, b.GetPacket(), false);
}

// Cast imbue
void CastImbueOrBuff(DWORD skill, DWORD client)
{
	PacketBuilder b;
	
	b.SetOpcode(0x72CD);			// 0x72CD - action packet
	b.AppendByte(0x01);				// Skill action								
	b.AppendByte(0x04);				// Skill action	
	b.AppendDword(skill);			// Imbue to cast
	b.AppendByte(0x00);				// No target

	PluginInjectClientToServer(client, b.GetPacket(), false);
}

// Pickup item
void GrabItem(DWORD object, DWORD client) 
{
	PacketBuilder b;

	b.SetOpcode(0x72CD);			// 0x72CD - action packet
	b.AppendByte(0x01);				// Grab action
	b.AppendByte(0x02);				// Grab action
	b.AppendByte(0x01);				// Grab action
	b.AppendDword(object);			// Object to grab

	PluginInjectClientToServer(client, b.GetPacket(), false);
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Emotes
//-----------------------------------------------------------------------------
// Emote
void Emote(BYTE emoteType, DWORD client)
{
	/* Emote types are :
		0			Hi
		1			Greeting
		2			Rush
		3			Joy
		4			No
		5			Yes
		6			Smile
	*************************/

	PacketBuilder b;
	
	b.SetOpcode(0x324B);
	b.AppendByte(emoteType);

	PluginInjectClientToServer(client, b.GetPacket(), false);
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Party
//-----------------------------------------------------------------------------
// Leave party
void LeaveParty(DWORD client)
{
	PacketBuilder b;

	b.SetOpcode(0x704F);			// Leaves current party

	PluginInjectClientToServer(client, b.GetPacket(), false);
}

// Invite player to party
void InviteToParty(DWORD object, DWORD client)
{
	PacketBuilder b;

	b.SetOpcode(0x751A);			// Invite
	b.AppendDword(object);

	PluginInjectClientToServer(client, b.GetPacket(), false);
}

// Join matchmaking party
void JoinMatchParty(DWORD party, DWORD client)
{
	PacketBuilder b;

	b.SetOpcode(0x75BF);			// Join
	b.AppendDword(party);

	PluginInjectClientToServer(client, b.GetPacket(), false);
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Stats
//-----------------------------------------------------------------------------
// Increase STR
void IncreaseSTR(DWORD client)
{
	PacketBuilder b;

	b.SetOpcode(0x727A);			// Increase STR

	PluginInjectClientToServer(client, b.GetPacket(), false);
}

// Increase INT
void IncreaseINT(DWORD client)
{
	PacketBuilder b;

	b.SetOpcode(0x7552);			// Increase INT

	PluginInjectClientToServer(client, b.GetPacket(), false);
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
