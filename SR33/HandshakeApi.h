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

#ifndef HANDSHAKE_API_H_
#define HANDSHAKE_API_H_

//-----------------------------------------------------------------------------

#ifndef _WINDOWS_
	#include <windows.h>
#endif

//-----------------------------------------------------------------------------

//1 Byte allignment for easy stream casting
#pragma pack(push, 1)
	struct tPacket_5000			// First/second packets received from the server
	{
		WORD size;				// Size of this packet
		WORD opcode;			// Opcode of this packet (0x5000)
		BYTE securityCount;		// Security count byte (0 from server to client packets)
		BYTE securityCRC;		// Security crc byte (0 from server to client packets)
		BYTE flag;				// Internal flag
	};
	struct tPacket_5000_E		// First packet received from the server
	{
		WORD size;				// Size of this packet (0x25)
		WORD opcode;			// Opcode of this packet (0x5000)
		BYTE securityCount;		// Security count byte (0 from server to client packets)
		BYTE securityCRC;		// Security crc byte (0 from server to client packets)
		BYTE flag;				// Internal flag (0xE)
		BYTE blowfish[8];		// Initial blowfish key
		DWORD seedCount;		// security count seed 
		DWORD seedCRC;			// security crc seed 
		DWORD seedSecurity[5];	// Additional seeds used
	};
	struct tPacket_5000_10		// Second packet received from the server
	{
		WORD size;				// Size of this packet (0xF)
		WORD opcode;			// Opcode of this packet (0x5000)
		BYTE securityCount;		// Security count byte (0 from server to client packets)
		BYTE securityCRC;		// Security crc byte (0 from server to client packets)
		BYTE flag;				// Internal flag (0x10)
		DWORD challenge[2];		// Challenge value to make sure everything is legit
	};
#pragma pack(pop)

//-----------------------------------------------------------------------------

/*
	Blowfish
	By: Jim Conger (original Bruce Schneier)
	Url: http://www.schneier.com/blowfish-download.html
*/
class cBlowFish
{
private:
	DWORD 		* PArray;
	DWORD		(* SBoxes)[256];
	void 		Blowfish_encipher(DWORD *xl, DWORD *xr);
	void 		Blowfish_decipher(DWORD *xl, DWORD *xr);

public:
	cBlowFish();
	~cBlowFish();
	void 		Initialize(BYTE key[], int keybytes);
	DWORD		GetOutputLength(DWORD lInputLong);
	DWORD		Encode(BYTE * pInput, BYTE * pOutput, DWORD lSize);
	void		Decode(BYTE * pInput, BYTE * pOutput, DWORD lSize);
};

//-----------------------------------------------------------------------------

class cHandShakeApi				// This is the handshake API class
{
public:
	cBlowFish blowfish;			// Blowfish object

private:
	unsigned char byte1seeds[3];// Count byte seeds
	tPacket_5000_E firstPacket;	// 1st packet
	DWORD dwArgs[18];			// Args used for the functions
	DWORD dwRand;				// The client generated random value

private:
	// This function was written by jMerlin as part of the article "How to generate the security bytes for SRO"
	unsigned long GenerateValue( unsigned long * Ptr );

	// Sets up the count bytes
	// This function was written by jMerlin as part of the article "How to generate the security bytes for SRO"
	void SetupCountByte( unsigned long seed );

public:
	// Default ctor
	cHandShakeApi();

	// Function called to generate a count byte
	// This function was written by jMerlin as part of the article "How to generate the security bytes for SRO"
	unsigned char GenerateCountByte( void );

	// Function called to generate the crc byte
	// This function was written by jMerlin as part of the article "How to generate the security bytes for SRO"
	unsigned char GenerateCheckByte( char* packet, int length, unsigned long seed );

	// Invoke this function with the packet stream of the 0x5000 packets
	void OnPacketRecv(LPBYTE stream, DWORD maxcount);

	// Invoke this function with the packet stream of the 0x5000 packets (clientless version)
	void OnPacketRecv(LPBYTE stream, DWORD maxcount, SOCKET s);

	// Decrypts a packet
	void DecryptPacket(LPBYTE stream, DWORD size);

	// Encrypts a packet and stores the new data and length in the parametere
	void EncryptPacket(LPBYTE rawpacket, DWORD size);

	// Set the client generated random value
	void SetRandomNumber(DWORD value);

	// Returns the CRC seed for use in the GenerateCheckByte funcion
	DWORD GetCRCSeed();
};

//-----------------------------------------------------------------------------

#endif
