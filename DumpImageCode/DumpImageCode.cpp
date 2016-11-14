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

#ifndef _ZLIB_H
	#include "zlib/zlib.h"
#endif

//-----------------------------------------------------------------------------

// Plugin functions to use from SR33
PluginInjectFunc PluginInjectServerToClient = 0;
PluginInjectFunc PluginInjectClientToServer = 0;

// DLL instance
HINSTANCE gInstance = 0;

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
	// Image code packet
	if(packet->opcode == 0x2322)
	{
		PacketReader reader(packet);
		BYTE b1 = reader.ReadByte();
		WORD w1 = reader.ReadWord();
		WORD compressed = reader.ReadWord();
		WORD uncompressed = reader.ReadWord();
		DWORD width = reader.ReadWord();
		DWORD height = reader.ReadWord();
		DWORD outLen = uncompressed;
		LPBYTE ptr = packet->data + 11;

		// Allocate memory for the image data
		char * rawbytes = new char [uncompressed];
		memset(rawbytes, 0, uncompressed);

		int result = uncompress((Bytef*)rawbytes, &outLen, ptr, compressed);
		if(result != Z_OK)
		{
			// Tell the error
			MessageBox(0, "Could not decompress the image code.", "Error", MB_ICONERROR);

			// Free memory
			delete [] rawbytes;
			
			// Accept the packet
			return true;
		}

		// Allocate memory for the image
		DWORD * image = new DWORD[width * height];
		memset(image, 0, sizeof(DWORD) * width * height);

		// Build the image code from pixels
		int imgIndex = 0;
		for(DWORD c = 0; c < height; ++c)
		{
			for(DWORD r = 0; r < width; ++r)
			{
				DWORD dwEax = 0;
				DWORD dwEdx = 0;
				BYTE dlTmp = 0;
				__asm
				{
					mov EAX, width
					mov EBX, c
					mov edx, 0

					mov ecx, r
					AND ECX, 0x80000007
					IMUL EAX, EBX
					MOV DL, 1
					SHL DL, CL
					mov ecx, rawbytes
					add EAX, r
					SAR EAX, 3
					mov dlTmp, DL
					mov dwEax, eax
				}
				dlTmp &= rawbytes[dwEax];
				__asm
				{
					mov dl, dlTmp
					neg dl
					SBB EDX, EDX
					AND EDX, 0x00FFFFFF
					ADD EDX, 0xFF000000
					mov dwEdx, edx
				}
				image[imgIndex++] = dwEdx;
			}
		}

		// Make the dir it goes in
		CreateDirectory("imagecode", 0);

		// Save as a targa file - code ripped from the net somewhere
		char filename[256] = {0};
		_snprintf(filename, 255, "imagecode\\%i.tga", GetTickCount());
		FILE * outFile  = fopen(filename, "wb");
		if(outFile)
		{
			unsigned char TGAheader[12]={0,0,2,0,0,0,0,0,0,0,0,0};
			unsigned char header[6] = {width % 256, width / 256, height % 256, height / 256, 32, 0};

			fwrite(TGAheader, sizeof(unsigned char), 12, outFile);
			fwrite(header, sizeof(unsigned char), 6, outFile);

			for(int c = height - 1; c >= 0; --c)
			{
				for(int r = 0; r < width; ++r)
				{
					DWORD dwEdx = image[c * width + r];
					fwrite(&dwEdx, 1, 4, outFile);
				}
			}
			fclose(outFile);
		}

		// Free memory
		delete [] image;
		delete [] rawbytes;
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
