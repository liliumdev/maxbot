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

#include <windows.h>
#include <stdio.h>
#include "detours.h"
#include <string>
#include <algorithm>

//-----------------------------------------------------------------------------
/*
	This is the main Detour file that you will need to modify on patches. The
	way it is setup is the dll will change the bytes of the sro_client at the
	address in which a random value is generated and used for the security 
	handshake process. SR33 must know this value so I change it to 0x33. To
	make it so you can use SR33 without a loader to do that, this detour 
	DLL will hook CreateProcess and inject itself into the client for you.

	However, please read the notes below on the CBT_HOOK and the problems
	that arise from using it.
*/
//-----------------------------------------------------------------------------

#pragma comment(lib, "ws2_32.lib")

#pragma data_seg(".SHARED")
	static HMODULE s_hDll = 0;
#pragma data_seg()

#pragma comment(linker, "/section:.SHARED,rws")

//-----------------------------------------------------------------------------

// Patches bytes using Win32 methods
BOOL PatchBytesWin32(HANDLE hProcess, DWORD destAddress, LPVOID patch, DWORD numBytes);

//-----------------------------------------------------------------------------

// The detour dll needs this function
HMODULE WINAPI Detoured()
{
    return s_hDll;
}

//-----------------------------------------------------------------------------

static BOOL (WINAPI * trueCreateProcessA)(LPCTSTR lpApplicationName, LPTSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCTSTR lpCurrentDirectory, LPSTARTUPINFO lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation) = CreateProcessA;

BOOL WINAPI hookedCreateProcessA(LPCTSTR lpApplicationName, LPTSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCTSTR lpCurrentDirectory, LPSTARTUPINFO lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
	size_t len = 0;
	char temp[2048] = {0};
	BOOL isSroProcess = false;

	// Check the params
	_snprintf(temp, 2047, "%s", lpApplicationName);
	len = strlen(temp);
	if(len > 0)
	{
		for(size_t x = 0; x < len; ++x)
			temp[x] = tolower(temp[x]);
		if(strstr(temp, "sro_client.exe"))
			isSroProcess = true;
	}

	// Check the params
	_snprintf(temp, 2047, "%s", lpCommandLine);
	len = strlen(temp);
	if(len > 0)
	{
		for(size_t x = 0; x < len; ++x)
			temp[x] = tolower(temp[x]);
		if(strstr(temp, "sro_client.exe"))
			isSroProcess = true;
	}

	// If we have a Silkroad process
	if(isSroProcess)
	{
		// Try to create the process in a suspended state
		BOOL result = trueCreateProcessA(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags | CREATE_SUSPENDED, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);

		// if the process was not created, do not continue
		if(result == FALSE)
			return result;

		// This is where we modify the client code!
		// This address will need to be updated on patches, unless you
		// implement something better ;)
		//
		// To find the address: 
		//		ctrl + f in olly
		//		"AND ECX, 0x7FFFFFFF" and hit ok
		//		hit "ctrl + L" to get the second isntance
		//

		// Old Code:
		// 004909E2                                          |.  81E1 FFFFFF7F              AND ECX, 7FFFFFFF

		// New Code:
		// 004909E2                                              B9 33000000                MOV ECX, 33
		// 004909E7                                              90                         NOP

		// 1.175
		DWORD dwAddress = 0x491CC2;

		// This won't change unless you want to use a new magic value for the security algo
		BYTE patch[6] = {0xB9, 0x33, 0x00, 0x00, 0x00, 0x90};

		// Make the patch
		PatchBytesWin32(lpProcessInformation->hProcess, dwAddress, patch, 6);

		// If the process was already supposed to be created suspended, leave it there
		if(dwCreationFlags & CREATE_SUSPENDED)
			return result;

		// Otherwise resume the process
		ResumeThread(lpProcessInformation->hProcess);
		ResumeThread(lpProcessInformation->hThread);

		// Return the result
		return result;
	}
	else
	{
		// Another process, just create it
		return trueCreateProcessA(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	}
}

//-----------------------------------------------------------------------------

static int (WINAPI * trueconnect)(SOCKET, const struct sockaddr*, int) = connect;

// Hook the connect function - you will not need to change this unless the login server IPs change
int WINAPI hookedconnect(SOCKET s, const struct sockaddr* name, int namelen)
{
	// Store the real port
	WORD port = ntohs((*(WORD*)name->sa_data));

	// Breakup the IP into the parts
	BYTE a = name->sa_data[2];
	BYTE b = name->sa_data[3];
	BYTE c = name->sa_data[4];
	BYTE d = name->sa_data[5];

	// Vairables used to get the filename
	size_t length = 0;
	size_t index = 0;
	char filename[1024] = {0};
	char exename[1024] = {0};

	// Get the file name and parse out the exe name
	GetModuleFileName(0, filename, 1023);
	length = strlen(filename);
	for(size_t x = 0; x < length; ++x)
	{
		filename[x] = tolower(filename[x]);
		if(filename[x] == '\\')
			index = x;
	}
	strcpy(exename, filename + index + 1);

	// Save the IP
	char ip[256] = {0};
	_snprintf(ip, 255, "%i.%i.%i.%i", a, b, c, d);

	// Check to see if it's our program
	bool isClient = (strstr(exename, "sro_client.exe") != 0);
	bool isClientless = (strstr(exename, "sr33.exe") != 0);
	if(isClient || isClientless)
	{
		struct sockaddr myname = {0};
		memcpy(&myname, name, sizeof(sockaddr));

		// Extra sanity check
		if( (isClient && port == 15779) || (isClientless && port == 0x33) )
		{
			// localhost
			myname.sa_data[2] = (char)127;
			myname.sa_data[3] = (char)0;
			myname.sa_data[4] = (char)0;
			myname.sa_data[5] = (char)1;

			// Login servers
			if(a == 121 && b == 128 && c == 133 && (d == 26 || d == 27))
			{
				// Custom port
				(*(WORD*)myname.sa_data) = htons(15778);

				// Modify the call
				return trueconnect(s, &myname, namelen);
			}
			// Game server
			else
			{
				// Custom port
				(*(WORD*)myname.sa_data) = htons(15780);

				// Modify the call
				return trueconnect(s, &myname, namelen);
			}
		}
	}
	return trueconnect(s, name, namelen);
}

//-----------------------------------------------------------------------------

// DLL entry point - detour code from the net modded some
BOOL WINAPI DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID reserved)
{
    LONG error;
    if (dwReason == DLL_PROCESS_ATTACH)
    {
		s_hDll = hinst;
        DisableThreadLibraryCalls(hinst);

		error = DetourRestoreAfterWith();
		//if(error == 0) //MessageBox(0, "DetourRestoreAfterWith", "Error", 0);

		error = DetourTransactionBegin();
		if(error != NO_ERROR)
			MessageBox(0, "DetourTransactionBegin", "Error", 0);

		error = DetourUpdateThread(GetCurrentThread());
		if(error != NO_ERROR)
			MessageBox(0, "DetourUpdateThread", "Error", 0);

		error = DetourAttach(&(PVOID&)trueconnect, hookedconnect);
		if(error != NO_ERROR)
			MessageBox(0, "DetourAttach", "Error", 0);

		error = DetourAttach(&(PVOID&)trueCreateProcessA, hookedCreateProcessA);
		if(error != NO_ERROR)
			MessageBox(0, "DetourAttach", "Error", 0);

		error = DetourTransactionCommit();
		if(error != NO_ERROR)
			MessageBox(0, "DetourTransactionCommit", "Error", 0);
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&(PVOID&)trueconnect, hookedconnect);
        error = DetourTransactionCommit();
    }
    return TRUE;
}

//-----------------------------------------------------------------------------

// For the Windows hook
// 
// A CBT hook will be called on any mouse or keyboard or similar input events to a 
// window. In order for your program to be hooked with this DLL, you must have some
// sort of user interaction to trigger the hook. The original Softmod loader does not
// have such, so it will not be hooked. However, if you add a messagebox call before the
// CreateThread function of the loader, it will properly hook since the CBT hook is invoked.
// Please keep this in mind if you wish to use SR33 with other loaders.
//
extern "C" __declspec(dllexport) LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    return CallNextHookEx(0, nCode, wParam, lParam);
}

//-----------------------------------------------------------------------------

// Patches bytes using Win32 methods
BOOL PatchBytesWin32(HANDLE hProcess, DWORD destAddress, LPVOID patch, DWORD numBytes)
{
	DWORD oldProtect = 0;	// Old protection on page we are writing to
	DWORD bytesRet = 0;		// # of bytes written
	BOOL status = TRUE;		// Status of the function

	// Change page protection so we can write executable code
	if(!VirtualProtectEx(hProcess, UlongToPtr(destAddress), numBytes, PAGE_EXECUTE_READWRITE, &oldProtect))
		return FALSE;

	// Write out the data
	if(!WriteProcessMemory(hProcess, UlongToPtr(destAddress), patch, numBytes, &bytesRet))
		status = FALSE;

	// Compare written bytes to the size of the patch
	if(bytesRet != numBytes)
		status = FALSE;

	// Restore the old page protection
	if(!VirtualProtectEx(hProcess, UlongToPtr(destAddress), numBytes, oldProtect, &oldProtect))
		status = FALSE;

	// Make sure changes are made!
	if(!FlushInstructionCache(hProcess, UlongToPtr(destAddress), numBytes))
		status = FALSE;

	// Return the final status, note once we set page protection, we don't want to prematurly return
	return status;
}

//-----------------------------------------------------------------------------
