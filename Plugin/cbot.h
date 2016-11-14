#ifndef PLUGIN_H_
	#include "Plugin.h"
#endif

#ifndef _VECTOR_
	#include <vector>
#endif

#include "../SR33/Packet.h"
#include "resource.h"

// Swap the DWORD to the real "format"
DWORD swap(DWORD x);

std::vector<std::string> TokenizeString(const std::string& str, const std::string& delim);

struct monster 
{
	DWORD id;
	int x;
	int y;
	int distance;
	int level;	
	int HP;
	int type;
	std::string name;	
};

class cBot 
{	
private:	
	// Monsterlist file
	std::string monsterList;	

	// Itemlist file
	std::string itemList;

	// Skillist file
	std::string skillList;

	// Monster we're currently attacking
	DWORD currentMonster;

	// Groupspawn stuff
	int expectMonsters;											    // How much monsters to expect	
	BYTE* temp3417;													// Current 3417 packet
	int temp3417Size;												// Temp size of 3417 packet we have
	bool expectSpawn;											    // Expect spawn ?
	bool expectDespawn;												// Expect despawn ?

public:
	cBot();
	~cBot();

	// GUI main dialog handle
	HWND gHwnd;

	// Skills dialog handle
	HWND skillsHwnd;

	// Mobs dialog handle
	HWND mobsHwnd;

	// Initialize text data (monsterlist, itemlist, skill-list)
	void InitializeTextData();

	// Available skill list
	std::vector<DWORD> skills;

	// Chosen skills
	std::vector<DWORD> chosenSkills;

	// Monster queue
	std::vector<monster> monsters;

	// Find monster in the monster queue and return the index (int)
	int FindMonster(DWORD object);	

	// Find monster in the monster queue and return the iterator
	std::vector<monster>::iterator FindMobIt(DWORD object);	

	// Add monster to the monster queue
	bool AddMonster(DWORD object, std::string mob, int dist, int x, int y, BYTE mobType);

	// ATM stuff
	DWORD charID;													// Current character ID
	int curHP;														// Current HP of character
	int curMP;														// Current MP of character
	int myX;														// Current X coordinate of character
	int myY;														// Current Y coordinate of character

	// Hunting stuff
	bool hunt;														// Should we start hunting
	bool hunting;													// Are we hunting
	void Hunt(DWORD client);										// Start hunt

	/* PACKET PARSING */

	// Character data
	void Parse32B3(tPacket * packet);

	// Pre-group spawn
	void Parse30CB(tPacket * packet);

	// Group spawn
	void Parse3417(tPacket * packet);

	// End group spawn
	void Parse330A(tPacket * packet);

	// Parse group spawn
	void ParseGroupSpawn(bool spawn);

	// Object action
	void Parse3122(tPacket * packet);

	// Character ID
	void Parse32A6(tPacket * packet);

	// Object movement
	void ParseB738(tPacket * packet);

	// Object stuck
	void ParseB245(tPacket * packet);

};

std::string FindObject(std::string monsters, DWORD obj)
{
	char mobID[10];
	sprintf(mobID, "%.8X", swap(obj));
	
	std::string monsterFound;								// To return

	int startLoc = monsters.find(mobID);					// Declare the start locations

	if(startLoc != std::string::npos)						// If we found the ID
	{
		int endLoc = monsters.find("\n", startLoc);			// Get the \n after the ID

		monsterFound = monsters.substr(startLoc, endLoc - startLoc);	// Find the whole line	
	}

	return monsterFound;
}

// Get type
std::string GetType(std::string object)
{
	// Tokenizing
	std::vector<std::string> args = TokenizeString(object, ",");

	std::vector<std::string> mobArgs = TokenizeString(args[1], "_");

	return mobArgs[0];
}

// Get item type / or monster type
std::string GetItemType(std::string object)
{
	// Tokenizing
	std::vector<std::string> args = TokenizeString(object, ",");

	std::vector<std::string> mobArgs = TokenizeString(args[1], "_");

	return mobArgs[1];
}

// Get name
std::string GetName(std::string object)
{
	// Tokenizing
	std::vector<std::string> args = TokenizeString(object, ",");
	
	return args[2];
}

// Get HP
std::string GetHP(std::string object)
{
	// Tokenizing
	std::vector<std::string> args = TokenizeString(object, ",");
	
	return args[4];
}

// Get level
std::string GetLevel(std::string object)
{
	// Tokenizing
	std::vector<std::string> args = TokenizeString(object, ",");
	
	return args[3];
}

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

// Cheers and credits to clearscreen ;)
DWORD swap(DWORD x) 
{
	return 
		(DWORD)(
			((x&0xff000000) >> 24)| 
			((x&0x00ff0000) >> 8) | 
			((x&0x0000ff00) << 8) | 
			((x&0x000000ff) << 24)
		);
}