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

#ifndef PACKET_H_
#define PACKET_H_

//-----------------------------------------------------------------------------

#ifndef _WINDOWS_
	#include <windows.h>
#endif

typedef __int64 QWORD, *LPQWORD;

//-----------------------------------------------------------------------------

#pragma pack(push, 1)
	struct tPacket
	{
		WORD size;
		WORD opcode;
		BYTE securityCount;
		BYTE securityCRC;
		BYTE data[8186];
	};
#pragma pack(pop)

//-----------------------------------------------------------------------------

// Packet building class
class PacketBuilder
{
private:
	BYTE data[32767];
	WORD size;

public:
	// Ctor
	PacketBuilder();

	// Dtor
	~PacketBuilder();

	// Clear the packet data
	void Clear();

	// Get the packet buffer
	tPacket * GetPacket();

	// Return the packet size
	WORD GetSize();

	// Set the packet's opcode
	void SetOpcode(WORD opcode);

	// Add data function
	void AppendByte(BYTE byte);

	// Add data function
	void AppendWord(WORD word);

	// Add data function
	void AppendDword(DWORD dword);

	// Append a string
	void AppendString(const char * str);

	// Append a wide string
	void AppendWideString(const wchar_t * str);

	// Append a string
	void AppendArray(int length, void * buffer);

	// Append a double
	void AppendDouble(double value);

	// Append a double
	void AppendFloat(float value);

	// Copy this packet into the buffer
	void CopyInto(void * packet);
};

//-----------------------------------------------------------------------------

// Packet reading class
class PacketReader
{
private:
	tPacket * packet;
	int index;

public:
	PacketReader();
	PacketReader(tPacket * p);
	~PacketReader();

	// Returns how much data can be read
	int HasData();

	// Set the packet pointer
	void SetPacket(tPacket * p);

	// Returns the packet's opcode
	WORD GetOpcode();

	// Returns the packet's size
	WORD GetSize();

	// Read in specific data types
	BYTE ReadByte();

	// Read in specific data types
	WORD ReadWord();

	// Read in specific data types
	DWORD ReadDword();

	// Read in specific data types
	QWORD ReadQword();

	// Read in specific data types
	void ReadArray(int size, void * outBuffer);

	// Read in specific data types
	void ReadString(int size, char * outBuffer);

	// Read in specific data types
	void ReadWideString(int size, wchar_t * outBuffer);

	// Read in specific data types
	double ReadDouble();

	// Read in specific data types
	float ReadFloat();
};

//-----------------------------------------------------------------------------

#endif