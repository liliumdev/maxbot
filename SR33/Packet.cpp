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
	#include "Packet.h"
#endif

//-----------------------------------------------------------------------------

// Ctor
PacketBuilder::PacketBuilder()
{
	Clear();
}

// Dtor
PacketBuilder::~PacketBuilder()
{
}

//-----------------------------------------------------------------------------

// Clear the packet data
void PacketBuilder::Clear()
{
	// Clear out the data
	memset(data, 0, 8192);
	size = 6;
}

//-----------------------------------------------------------------------------

// Get the packet buffer
tPacket * PacketBuilder::GetPacket()
{
	return (tPacket *)data;
}

//-----------------------------------------------------------------------------

// Return the packet size
WORD PacketBuilder::GetSize()
{
	return size;
}

//-----------------------------------------------------------------------------

// Set the packet's opcode
void PacketBuilder::SetOpcode(WORD opcode)
{
	LPWORD stream = (LPWORD)data;
	stream++;
	(*stream) = opcode;
}

//-----------------------------------------------------------------------------

// Add data function
void PacketBuilder::AppendByte(BYTE byte)
{
	LPBYTE stream = (LPBYTE)data;
	stream += size;
	(*stream) = byte;
	++size;
	WORD tmpSize = size - 6;
	memcpy(data, &tmpSize, 2);
}

//-----------------------------------------------------------------------------

// Add data function
void PacketBuilder::AppendWord(WORD word)
{
	LPBYTE stream = (LPBYTE)data;
	stream += size;
	(*(LPWORD)stream) = word;
	size += 2;
	WORD tmpSize = size - 6;
	memcpy(data, &tmpSize, 2);
}

//-----------------------------------------------------------------------------

// Add data function
void PacketBuilder::AppendDword(DWORD dword)
{
	LPBYTE stream = (LPBYTE)data;
	stream += size;
	(*(LPDWORD)stream) = dword;
	size += 4;
	WORD tmpSize = size - 6;
	memcpy(data, &tmpSize, 2);
}

//-----------------------------------------------------------------------------

// Append a string
void PacketBuilder::AppendString(const char * str)
{
	LPBYTE stream = (LPBYTE)data;
	stream += size;
	memcpy(stream, str, strlen(str));
	size += (WORD)strlen(str);
	WORD tmpSize = size - 6;
	memcpy(data, &tmpSize, 2);
}

//-----------------------------------------------------------------------------

// Append a string
void PacketBuilder::AppendWideString(const wchar_t * str)
{
	LPBYTE stream = (LPBYTE)data;
	stream += size;
	memcpy(stream, str, 2 * wcslen(str));
	size += (WORD)(2 * wcslen(str));
	WORD tmpSize = size - 6;
	memcpy(data, &tmpSize, 2);
}

//-----------------------------------------------------------------------------

// Append a string
void PacketBuilder::AppendArray(int length, void * buffer)
{
	LPBYTE stream = (LPBYTE)data;
	stream += size;
	memcpy((char*)stream, buffer, length);
	size += length;
	WORD tmpSize = size - 6;
	memcpy(data, &tmpSize, 2);
}

//-----------------------------------------------------------------------------

// Append a double
void PacketBuilder::AppendDouble(double value)
{
	LPBYTE stream = (LPBYTE)data;
	stream += size;
	(*(DOUBLE*)stream) = value;
	size += sizeof(double);
	WORD tmpSize = size - 6;
	memcpy(data, &tmpSize, 2);
}

//-----------------------------------------------------------------------------

// Append a float
void PacketBuilder::AppendFloat(float value)
{
	LPBYTE stream = (LPBYTE)data;
	stream += size;
	(*(float*)stream) = value;
	size += sizeof(float);
	WORD tmpSize = size - 6;
	memcpy(data, &tmpSize, 2);
}

//-----------------------------------------------------------------------------

// Copy this packet into the buffer
void PacketBuilder::CopyInto(void * packet)
{
	memcpy(packet, data, size);
}

//-----------------------------------------------------------------------------

// Ctor
PacketReader::PacketReader()
{
	packet = 0;
	index = 0;
}

//-----------------------------------------------------------------------------

PacketReader::~PacketReader()
{
}

//-----------------------------------------------------------------------------

// Ctor - build from a packet
PacketReader::PacketReader(tPacket * p)
{
	packet = p;
	index = 0;
}

//-----------------------------------------------------------------------------

// Set a packet for the object
void PacketReader::SetPacket(tPacket * p)
{
	packet = p;
	index = 0;
}

//-----------------------------------------------------------------------------

// Returns the packet opcode
WORD PacketReader::GetOpcode()
{
	return packet->opcode;
}

//-----------------------------------------------------------------------------

// Returns how much data can be read from the packet
int PacketReader::HasData()
{
	if(index < packet->size)
		return packet->size - index;
	else
		return 0;
}

//-----------------------------------------------------------------------------

// Returns the packet's size
WORD PacketReader::GetSize()
{
	return packet->size;
}

//-----------------------------------------------------------------------------

// Read in specific data types
BYTE PacketReader::ReadByte()
{
	LPBYTE stream = packet->data;
	stream += index;
	index += sizeof(BYTE);
	if(index > packet->size) 
		MessageBox(0, "PacketReader::ReadByte past end of buffer.", "Error", MB_ICONERROR);
	return (*stream);
}

//-----------------------------------------------------------------------------

// Read in specific data types
WORD PacketReader::ReadWord()
{
	LPBYTE stream = packet->data;
	stream += index;
	index += sizeof(WORD);
	if(index > packet->size) 
		MessageBox(0, "PacketReader::ReadWord past end of buffer.", "Error", MB_ICONERROR);
	return *((LPWORD)(stream));
}

//-----------------------------------------------------------------------------

// Read in specific data types
DWORD PacketReader::ReadDword()
{
	LPBYTE stream = packet->data;
	stream += index;
	index += sizeof(DWORD);
	if(index > packet->size) 
		MessageBox(0, "PacketReader::ReadDword past end of buffer.", "Error", MB_ICONERROR);
	return *((LPDWORD)(stream));
}

// Read in specific data types
QWORD PacketReader::ReadQword()
{
	LPBYTE stream = packet->data;
	stream += index;
	index += sizeof(QWORD);
	if(index > packet->size) 
		MessageBox(0, "PacketReader::ReadQword past end of buffer.", "Error", MB_ICONERROR);
	return *((LPQWORD)(stream));
}

//-----------------------------------------------------------------------------

// Read in specific data types
void PacketReader::ReadString(int size, char * outBuffer)
{
	LPBYTE stream = packet->data;
	stream += index;
	index += size;
	memcpy(outBuffer, stream, size);
	outBuffer[size] = 0;
	if(index > packet->size) 
		MessageBox(0, "PacketReader::ReadString past end of buffer.", "Error", MB_ICONERROR);
}

//-----------------------------------------------------------------------------

// Read in specific data types
void PacketReader::ReadWideString(int size, wchar_t * outBuffer)
{
	LPBYTE stream = packet->data;
	stream += index;
	index += (size * sizeof(wchar_t));
	memcpy(outBuffer, stream, size * 2);
	outBuffer[size * 2] = 0;
	if(index > packet->size) 
		MessageBox(0, "PacketReader::ReadWideString past end of buffer.", "Error", MB_ICONERROR);
}

//-----------------------------------------------------------------------------

// Read in specific data types
float PacketReader::ReadFloat()
{
	LPBYTE stream = packet->data;
	stream += index;
	index += sizeof(float);
	if(index > packet->size) 
		MessageBox(0, "PacketReader::ReadFloat past end of buffer.", "Error", MB_ICONERROR);
	return *((float*)(stream));
}

//-----------------------------------------------------------------------------

// Read in specific data types
double PacketReader::ReadDouble()
{
	LPBYTE stream = packet->data;
	stream += index;
	index += sizeof(double);
	if(index > packet->size) 
		MessageBox(0, "PacketReader::ReadDouble past end of buffer.", "Error", MB_ICONERROR);
	return *((double*)(stream));
}

//-----------------------------------------------------------------------------

// Read in specific data types
void PacketReader::ReadArray(int size, void * outBuffer)
{
	LPBYTE stream = packet->data;
	stream += index;
	index += size;
	memcpy(outBuffer, stream, size);
	if(index > packet->size) 
		MessageBox(0, "PacketReader::ReadArray past end of buffer.", "Error", MB_ICONERROR);
}

//-----------------------------------------------------------------------------
