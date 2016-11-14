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

// Ctor
cBot::cBot()
{
	expectMonsters = 0;
	expectSpawn = false;
	expectDespawn = false;
	hunt = false;
	hunting = false;

	temp3417 = new BYTE[8192];
	temp3417Size = 6;
}

//----------------------------------------------------------------------------------------

// Dtor
cBot::~cBot()
{
}

//----------------------------------------------------------------------------------------

// Initialize text data : monster-list, item-list, skill-list
void cBot::InitializeTextData()
{
	std::ifstream monsterFile("mobs.txt", std::ios_base::in);
	std::ifstream itemFile("items.txt", std::ios_base::in);
	std::ifstream skillFile("skills.txt", std::ios_base::in);
	std::string line;

	while(std::getline(monsterFile, line))
	{		
		monsterList.append(line);
		monsterList.append("\n");
	}	

	while(std::getline(itemFile, line))
	{		
		itemList.append(line);
		itemList.append("\n");
	}	

	while(std::getline(skillFile, line))
	{		
		skillList.append(line);
		skillList.append("\n");
	}
}

//----------------------------------------------------------------------------------------

// Find monster in the monster queue and return index
// [object] is the ID of the monster
int cBot::FindMonster(DWORD object)
{
	std::vector<monster>::iterator Itr = monsters.begin();

	while(Itr != monsters.end())
	{
		if (Itr->id == object)
		{			
			size_t i = Itr - monsters.begin();
			return i;
		}
		else Itr++;
	}

	return -1;
}

//----------------------------------------------------------------------------------------

// Find monster in the monster queue and return iterator
// [object] is the ID of the monster
std::vector<monster>::iterator cBot::FindMobIt(DWORD object)
{
	std::vector<monster>::iterator Itr = monsters.begin();

	while(Itr != monsters.end())
	{
		if (Itr->id == object)
		{			
			return Itr;
		}
		else Itr++;
	}

	return monsters.end();
}

//----------------------------------------------------------------------------------------

// Add monster to the monster queue
// [mob] is the line that is returned by FindObject()
bool cBot::AddMonster(DWORD object, std::string mob, int dist, int x, int y, BYTE mobType)
{	
	// If we haven't already added the monster to our queue
	if(FindMonster(object) == -1)
	{
		std::string name = GetName(mob);					// Get the mob name
		std::string hp = GetHP(mob);						// Get the HP of the mob
		std::string lvl = GetLevel(mob);					// Get the level

		printf("Adding monster %s\n", name.c_str());		// Debug
		
		monster monster;									// Create the struct
		monster.distance = dist;							// Distance
		monster.HP = atoi(hp.c_str());						// HP
		monster.level = atoi(lvl.c_str());					// Level
		monster.name = name;								// Name of the mob
		monster.x = x;										// X
		monster.y = y;										// Y
		monster.id = object;								// Unique ID
		monster.type = mobType;								// Type

		monsters.push_back(monster);						// Add it to the queue

		// Done
		return true;
	}
	else
	{
		// Object already added
		return false;
	}
}

//----------------------------------------------------------------------------------------

// Character data parsing
void cBot::Parse32B3(tPacket * packet)
{
	PacketReader r(packet);

	DWORD charType = r.ReadDword();						// Character type
	BYTE volHeight = r.ReadByte();						// Mix of volume and height
	BYTE lvl = r.ReadByte();							// Level
	r.ReadByte();										// Level
	QWORD charEXP = r.ReadQword();						// Character EXP
	r.ReadDword();										// Exp/sp bar
	QWORD gold = r.ReadQword();							// Gold
	DWORD charSP = r.ReadDword();						// Character SP
	WORD attribs = r.ReadWord();						// Attributes
	BYTE berserk = r.ReadByte();						// Berserk
	r.ReadDword();										// Unknown
	curHP = r.ReadDword();							// Max HP
	curMP = r.ReadDword();							// Max MP
	r.ReadByte();										// Noob icon
	BYTE dailyPK = r.ReadByte();						// Daily PK
	WORD pkLevel = r.ReadWord();						// PK level
	DWORD murderLevel = r.ReadDword();					// Murder level

	// Now print stuff to dialog
	char level[5];
	sprintf(level, "%u", lvl);
	char hp[15];
	sprintf(hp, "%u", curHP);
	char mp[15];
	sprintf(mp, "%u", curMP);
	char goldTo[15];
	sprintf(goldTo, "%u", gold);

	SetDlgItemText(gHwnd, IDC_CHARLEVEL, (LPCSTR)level);
	SetDlgItemText(gHwnd, IDC_CHARHP, (LPCSTR)hp);
	SetDlgItemText(gHwnd, IDC_CHARMP, (LPCSTR)mp);
	SetDlgItemText(gHwnd, IDC_CHARGOLD, (LPCSTR)goldTo);

	//-----------------------------------------------------------------------------
	// Items
	BYTE maxSlots = r.ReadByte();							// Inventory max slots
	BYTE numbOfItems = r.ReadByte();						// Number of items
	
	for(int i = 1; i <= numbOfItems; i++)					// Parse items
	{	
		BYTE slot = r.ReadByte();							// Slot of the item
		DWORD pk2 = r.ReadDword();							// PK2 ID of the item

		std::string item = FindObject(itemList, pk2);	// Find the object from itemlist
		printf("%s\n", item.c_str());						// Debug

		if(item != "")										// If we found the item
		{
			std::string name = GetName(item);				// Name of the object
			std::string type = GetItemType(item);			// Item type

			printf("Item : %s\n", item.c_str());			// Debug
			printf("Type : %s\n", type.c_str());			// Debug

			if(type == "ETC")								// If the type is ETC
			{
				DWORD quantity = r.ReadWord();				// Quantity of the item

				// Add to inventory listbox
				char buffer[255];
				sprintf(buffer, "%s (%u)", name.c_str(), quantity);
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), LB_ADDSTRING, 0, (LPARAM)buffer);
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), WM_VSCROLL, SB_BOTTOM, 0);

			}

			// This has to be fixed !!!! NAME OF THE PET, LEVEL, ETC.
			if(type == "COS")								// If the type is COS
			{
				DWORD quantity = r.ReadWord();				// Get quantity

				// Add to inventory listbox
				char buffer[255];
				sprintf(buffer, "%s (%u)", name.c_str(), quantity);
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), LB_ADDSTRING, 0, (LPARAM)buffer);
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), WM_VSCROLL, SB_BOTTOM, 0);

			}

			if(type == "QNO")
			{
				DWORD quantity = r.ReadWord();

				// Add to inventory listbox
				char buffer[255];
				sprintf(buffer, "%s (%u)", name.c_str(), quantity);
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), LB_ADDSTRING, 0, (LPARAM)buffer);
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), WM_VSCROLL, SB_BOTTOM, 0);

			}
			if(type == "MAL")
			{
				DWORD quantity = r.ReadWord();

				// Add to inventory listbox
				char buffer[255];
				sprintf(buffer, "%s (%u)", name.c_str(), quantity);
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), LB_ADDSTRING, 0, (LPARAM)buffer);
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), WM_VSCROLL, SB_BOTTOM, 0);

			}
			if(type == "CH" || type == "EU")
			{
				r.ReadQword();								// Item modifier
				BYTE plus = r.ReadByte();					// Plus value
				r.ReadDword();								// Durability
				r.ReadByte();								// Blue stats

				// Add to inventory listbox
				char buffer[255];
				if(plus > 0)
					sprintf(buffer, "%s (+%u)", name.c_str(), plus);
				if(plus == 0)
					sprintf(buffer, "%s", name.c_str());
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), LB_ADDSTRING, 0, (LPARAM)buffer);
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), WM_VSCROLL, SB_BOTTOM, 0);
			}
		}			
	}

	r.ReadByte();									// Start of extra item list
	BYTE extraItems = r.ReadByte();					// Number of items in the extra itemlist
	printf("Extra item list : %u\n", extraItems);

	for(int i = 1; i <= extraItems; i++)					// Parse items
	{	
		BYTE slot = r.ReadByte();							// Slot of the item
		DWORD pk2 = r.ReadDword();							// PK2 ID of the item

		std::string item = FindObject(itemList, pk2);	// Find the object from itemlist
		printf("%s\n", item.c_str());						// Debug

		if(item != "")										// If we found the item
		{
			std::string name = GetName(item);				// Name of the object
			std::string type = GetItemType(item);			// Item type

			printf("Item : %s\n", item.c_str());			// Debug
			printf("Type : %s\n", type.c_str());			// Debug

			if(type == "ETC")								// If the type is ETC
			{
				DWORD quantity = r.ReadWord();				// Quantity of the item

				// Add to inventory listbox
				char buffer[255];
				sprintf(buffer, "%s (%u)", name.c_str(), quantity);
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), LB_ADDSTRING, 0, (LPARAM)buffer);
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), WM_VSCROLL, SB_BOTTOM, 0);

			}

			// This has to be fixed !!!! NAME OF THE PET, LEVEL, ETC.
			if(type == "COS")								// If the type is COS
			{
				DWORD quantity = r.ReadWord();				// Get quantity

				// Add to inventory listbox
				char buffer[255];
				sprintf(buffer, "%s (%u)", name.c_str(), quantity);
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), LB_ADDSTRING, 0, (LPARAM)buffer);
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), WM_VSCROLL, SB_BOTTOM, 0);

			}

			if(type == "QNO")
			{
				DWORD quantity = r.ReadWord();

				// Add to inventory listbox
				char buffer[255];
				sprintf(buffer, "%s (%u)", name.c_str(), quantity);
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), LB_ADDSTRING, 0, (LPARAM)buffer);
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), WM_VSCROLL, SB_BOTTOM, 0);

			}
			if(type == "MAL")
			{
				DWORD quantity = r.ReadWord();

				// Add to inventory listbox
				char buffer[255];
				sprintf(buffer, "%s (%u)", name.c_str(), quantity);
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), LB_ADDSTRING, 0, (LPARAM)buffer);
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), WM_VSCROLL, SB_BOTTOM, 0);

			}
			if(type == "CH" || type == "EU")
			{
				r.ReadQword();								// Item modifier
				BYTE plus = r.ReadByte();					// Plus value
				r.ReadDword();								// Durability
				r.ReadByte();								// Blue stats

				// Add to inventory listbox
				char buffer[255];
				if(plus > 0)
					sprintf(buffer, "%s (+%u)", name.c_str(), plus);
				if(plus == 0)
					sprintf(buffer, "%s", name.c_str());
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), LB_ADDSTRING, 0, (LPARAM)buffer);
				SendMessage(GetDlgItem(gHwnd, IDC_INVENTORY), WM_VSCROLL, SB_BOTTOM, 0);
			}
		}			
	}

	//-----------------------------------------------------------------------------

	//-----------------------------------------------------------------------------
	// Masteries
	r.ReadByte();				// Mastery list start
	
	std::string characterType = FindObject(monsterList, charType);
	std::string race = GetItemType(characterType);

	// If the char is chinese, 7 masteries
	if(race == "CH")
	{
		printf("Character is chinese ...\n");
		for(int i = 1; i <= 7; i++)
		{
			r.ReadByte();						// Mastery following
			r.ReadDword();						// Mastery type
			r.ReadByte();						// Mastery level
		}
	}
	// If the char is euro, 6 masteries
	if(race == "EU")
	{
		printf("Character is euro ...\n");
		for(int i = 1; i <= 6; i++)
		{
			r.ReadByte();						// Mastery following
			r.ReadDword();						// Mastery type
			r.ReadByte();						// Mastery level
		}
	}
	//-----------------------------------------------------------------------------

	//-----------------------------------------------------------------------------
	// Skills

	BYTE end = 0;
	r.ReadWord();									// What is this ?
	BYTE cSkill = r.ReadByte();						// ??

	do 
	{
		DWORD skillType = r.ReadDword();			// Skill type
		r.ReadByte();								// ??
		end = r.ReadByte();							// End

		// Read in skills
		if(end != 2)
		{
			// Push it to available skills vector
			skills.push_back(skillType);

			// Get it from skillList
			std::string skill = FindObject(skillList, skillType);
			printf("Skill is : %.8X\n", skillType);
			printf("Skill : %s\n", skill.c_str());
			if(skill != "")
			{
				std::string name = GetName(skill);

				// Add to skill list in GUI
				char buffer[455];
				sprintf(buffer, "%s", name.c_str());
				SendMessage(GetDlgItem(skillsHwnd, IDC_SKILLSLIST), LB_ADDSTRING, 0, (LPARAM)buffer);
				SendMessage(GetDlgItem(skillsHwnd, IDC_SKILLSLIST), WM_VSCROLL, SB_BOTTOM, 0);
			}
		}
		if(end == 2) break;
	} while(end != 2);

	//-----------------------------------------------------------------------------
	// Quests
	WORD finishedQuests = r.ReadWord();
	for(int i = 1; i <= finishedQuests; i++)
		r.ReadDword();							//Quest ID from server_dep\silkroad\textdata\questdata.txt 
	BYTE activeQuests = r.ReadByte();
	for(int i = 1; i <= activeQuests; i++)
	{
		r.ReadWord();
		r.ReadByte();
		r.ReadByte();
		r.ReadByte();							// Number of how much this quest can be repeated
		r.ReadByte();
		r.ReadByte();
		WORD questNameLen = r.ReadWord();
		char questName[100];
		r.ReadString(questNameLen, questName);
		r.ReadByte();
		r.ReadDword();
		r.ReadByte();
		r.ReadDword();
		r.ReadByte();
	}
	//-----------------------------------------------------------------------------

	//-----------------------------------------------------------------------------
	// Char data
	charID = r.ReadDword();					// Character ID

	BYTE XSec = r.ReadByte();					// X sector
	BYTE YSec = r.ReadByte();					// Y sector

	float X = r.ReadFloat();					// X
	float Z = r.ReadFloat();					// Z
	float Y = r.ReadFloat();					// Y

	myX = (int)(XSec - 135) * 192 + X / 10;
	myY = (int)(YSec - 92) * 192 + Y / 10;
	printf("Our X : %u\nOur Y: %u\n", myX, myY);

	r.ReadWord();								// Angle
	BYTE gotDest = r.ReadByte();				// Got destination ?
	if(gotDest == 0x01)
	{
		r.ReadWord();							// Sectors
		r.ReadWord();							// X
		r.ReadWord();							// Z
		r.ReadWord();							// Y
	}
	else
	{
		r.ReadByte();
		r.ReadWord();							// Angle
	}

	r.ReadByte();								// Byte flag
	r.ReadByte();								// Move flag
	r.ReadByte();								// Berserker

	// Player speeds
	for(int i = 1; i <= 3; i++)
		r.ReadFloat();

	// Buffs
	BYTE buffNumb = r.ReadByte();				// Number of buffs
	for(int i = 1; i <= buffNumb; i++)
	{
		r.ReadDword();							// Buff ID
		r.ReadDword();							// Buff duration in ms
	}

	char charName[50];
	WORD charNameLen = r.ReadWord();			// Char name length
	r.ReadString(charNameLen, charName);		// Char name

	SetDlgItemText(gHwnd, IDC_CHARNAME, charName);
}

// Pre-group spawn
void cBot::Parse30CB(tPacket * packet)
{
	PacketReader r(packet);

	BYTE mode = r.ReadByte();					// 01 - spawn; 02 - despawn
	expectMonsters = r.ReadWord();				// Number of objects to spawn / despawn

	if(mode = 0x01)
	{
		expectSpawn = true;
	}
	else
	{
		expectDespawn = true;
	}
}


//----------------------------------------------------------------------------------------

// Main packet
void cBot::Parse3417(tPacket * packet)
{
	if(expectSpawn || expectDespawn)
	{
		// We have to append the packets since they don't come all at once
		// We'll parse them once we receive the groupspawn end
		memcpy(temp3417 + temp3417Size, (void *)packet->data, packet->size);
		temp3417Size += packet->size;
	}
}

//----------------------------------------------------------------------------------------

// End group spawn
void cBot::Parse330A(tPacket * packet)
{
	if(expectSpawn)
	{
		ParseGroupSpawn(true);
	}
	if(expectDespawn)
	{
		ParseGroupSpawn(false);
	}
}

//----------------------------------------------------------------------------------------

// Parse groupspawn
void cBot::ParseGroupSpawn(bool spawn)
{
	tPacket * packet = (tPacket *)temp3417;

	PacketReader r(packet);

	if(spawn)			// Spawning
	{
		for(int i = 1; i <= expectMonsters; i++)
		{
			DWORD pk2id = r.ReadDword();							// PK2 ID

			std::string mob = FindObject(monsterList, pk2id);		// Find it in the monsterfile

			// Everything is fine
			if(mob != "")
			{
				// Tokenizing
				// Get type
				std::string objType = GetType(mob);				

				// For monsters
				if(objType == "MOB")
				{
					char mobID[255];	
					char mobster[555];
					char mobcoords[15];
					char dist[15];

					DWORD objID = r.ReadDword();		// Object ID
					
					BYTE XSec = r.ReadByte();			// X sector
					BYTE YSec = r.ReadByte();			// Y sector
					float X = r.ReadFloat();			// X
					float Z = r.ReadFloat();			// Z
					float Y = r.ReadFloat();			// Y

					int PosX = (int)(XSec - 135) * 192 + X / 10;
					int PosY = (int)(YSec - 92) * 192 + Y / 10;

					int mobDist = 0;
					int distX = 0;
					int distY = 0;

					if(myX > PosX)
						distX = myX - PosX;
					else
						distX = PosX - myX;

					if(myY > PosY)
						distY = myY - PosY;
					else
						distY = PosY - myY;

					mobDist = distX + distY;

					r.ReadWord();						// Angle
					BYTE gotDest = r.ReadByte();		// Got destination ?
					r.ReadByte();						// Walking / running : 00 / 01
					if(gotDest == 0x01)					// There's a destination
					{
						r.ReadWord();					// Sectors
						r.ReadWord();					// X
						r.ReadWord();					// Z
						r.ReadWord();					// Y
					}
					else
					{
						r.ReadByte();					// No destination
						r.ReadWord();					// Direction angle
					}
					BYTE deathFlag = r.ReadByte();		// Death flag
					r.ReadByte();						// Moving flag
					r.ReadByte();						// Berserk flag
					r.ReadFloat();						// Walk speed
					r.ReadFloat();						// Run speed
					r.ReadFloat();						// Berserk speed

					BYTE numbOfBuffs = r.ReadByte();	// Number of buffs
					for(int b = 1; b <= numbOfBuffs; b++)
					{
						r.ReadDword();					// Skill ID
						r.ReadDword();					// Skill duration
					}

					BYTE nameType = r.ReadByte();		// Name type

					if(nameType == 0x01)
					{
						DWORD len = r.ReadWord();
						char name[20];
						r.ReadString(len, name);
					}

					if(nameType == 0x02)
						r.ReadDword();

					if(nameType == 0x03)
					{
						DWORD len = r.ReadWord();
						char name[20];
						r.ReadString(len, name);
						r.ReadDword();
					}

					BYTE mobType = r.ReadByte();				// Mob type
					
					sprintf(mobID, "%.8X", swap(objID));		// ID swapped to match the monsterlist format
					sprintf(mobster, "%s", mob.c_str());		// Mob name
					sprintf(mobcoords, "%u / %u", PosX, PosY);	// Coords
					sprintf(dist, "%u", mobDist);				// Distance

					// Add the name & ID to the lists
					SendMessage(GetDlgItem(mobsHwnd, IDC_MOBLIST), LB_ADDSTRING, 0, (LPARAM)mobster);
					SendMessage(GetDlgItem(mobsHwnd, IDC_MOBLIST), WM_VSCROLL, SB_BOTTOM, 0);
					SendMessage(GetDlgItem(mobsHwnd, IDC_MOBIDS), LB_ADDSTRING, 0, (LPARAM)mobID);
					SendMessage(GetDlgItem(mobsHwnd, IDC_MOBIDS), WM_VSCROLL, SB_BOTTOM, 0);
					SendMessage(GetDlgItem(mobsHwnd, IDC_MOBCOORDS), LB_ADDSTRING, 0, (LPARAM)mobcoords);
					SendMessage(GetDlgItem(mobsHwnd, IDC_MOBCOORDS), WM_VSCROLL, SB_BOTTOM, 0);
					SendMessage(GetDlgItem(mobsHwnd, IDC_MOBDIST), LB_ADDSTRING, 0, (LPARAM)dist);
					SendMessage(GetDlgItem(mobsHwnd, IDC_MOBDIST), WM_VSCROLL, SB_BOTTOM, 0);

					bool r = AddMonster(objID, mob, mobDist, PosX, PosY, mobType);

					if(!r)
					{
						// Update coordinates and distance 
						// Because monster is already there
						int index = FindMonster(objID);

						monsters.at(index).x = PosX;
						monsters.at(index).y = PosY;
						monsters.at(index).distance = mobDist;
					}

					expectSpawn = false;
					expectDespawn = false;

					delete[] temp3417;
				}				
			}	
		}

	}
	else				// Despawning
	{
		PacketReader r(packet);

		for(int i = 1; i <= expectMonsters; i++)						// Despawn all mobs
		{
			DWORD id = r.ReadDword();									// Read the ID of monster to be despawned
			
			std::vector<monster>::iterator it = FindMobIt(id);		// Iterator

			if(it != monsters.end())								// If we found something
			{
				printf("Monster in our queue is being despawned ... \nDeleting it from our queue.\n");

				char lb_id[10];											// Delete it from listbox too
				sprintf(lb_id, "%.8X", swap(id));

				int i = ListBox_FindStringExact(GetDlgItem(mobsHwnd, IDC_MOBIDS), -1, lb_id);
				if(i != LB_ERR)
				{
					ListBox_DeleteString(GetDlgItem(mobsHwnd, IDC_MOBIDS), i);
					ListBox_DeleteString(GetDlgItem(mobsHwnd, IDC_MOBCOORDS), i);
					ListBox_DeleteString(GetDlgItem(mobsHwnd, IDC_MOBLIST), i);
					ListBox_DeleteString(GetDlgItem(mobsHwnd, IDC_MOBDIST), i);
				}

				monsters.erase(it);									// Delete the monster from our queue

				expectSpawn = false;
				expectDespawn = false;

				delete[] temp3417;
			}

		}
	}
}

//----------------------------------------------------------------------------------------

// Object action
void cBot::Parse3122(tPacket * packet)
{
	PacketReader r(packet);

	DWORD object = r.ReadDword();

	std::vector<monster>::iterator it = FindMobIt(object);		// Iterator

	if(it != monsters.end())									// If we do have it
	{
		BYTE type = r.ReadByte();									// Read the type

		if(type == 0x00)
		{
			BYTE subtype = r.ReadByte();							// Read the subtype
			printf("Type is 0.\n");

			if(subtype == 0x02 || subtype == 0x03)
			{
				printf("Monster dying ... \nDeleting it from our queue.\n");
				char toFind[10];
				sprintf(toFind, "%.8X", swap(object));

				monsters.erase(it);								// Delete the monster from our queue

				if(object == currentMonster)
				{
					hunting = false;
				}

				int index = ListBox_FindStringExact(GetDlgItem(mobsHwnd, IDC_MOBIDS), -1, toFind);

				ListBox_DeleteString(GetDlgItem(mobsHwnd, IDC_MOBIDS), index);
				ListBox_DeleteString(GetDlgItem(mobsHwnd, IDC_MOBLIST), index);
				ListBox_DeleteString(GetDlgItem(mobsHwnd, IDC_MOBDIST), index);
				ListBox_DeleteString(GetDlgItem(mobsHwnd, IDC_MOBCOORDS), index);
			}
		}
	}
}
//----------------------------------------------------------------------------------------

// Character ID
void cBot::Parse32A6(tPacket * packet)
{
	PacketReader r(packet);

	charID = r.ReadDword();
}

void cBot::ParseB738(tPacket * packet)
{
	PacketReader r(packet);
	std::vector<monster>::iterator it;						// Iterator

	// Source + destination
	if(packet->size == 24)
	{
		DWORD object = r.ReadDword();

		it = FindMobIt(object);								// Check do we have the object in our queue
		
		if(it != monsters.end() || object == charID)		// If we found it
		{
			r.ReadByte();
			BYTE XSec = r.ReadByte();
			BYTE YSec = r.ReadByte();
			WORD X = r.ReadWord();
			r.ReadWord();
			WORD Y = r.ReadWord();

			int PosX = (int)(XSec - 135) * 192 + X / 10;	// Calculate X
			int PosY = (int)(YSec - 92) * 192 + Y / 10;		// Calculate Y

			// If the object moving is actually our character
			if(object == charID)
			{
				myX = PosX;									// Set our X								
				myY = PosY;									// Set our Y

				// Recalculate distances for all monsters again
				for(int i = 0; i <= monsters.size(); i++)
				{
					int mobDist = 0;
					int distX = 0;
					int distY = 0;

					if(myX > monsters.at(i).x)
						distX = myX - monsters.at(i).x;
					else
						distX = monsters.at(i).x - myX;

					if(myY > monsters.at(i).y)
						distY = myY - monsters.at(i).y;
					else
						distY = monsters.at(i).y - myY;

					mobDist = distX + distY;
					monsters.at(i).distance = mobDist;
				}
			}

			// If the object moving is not our character
			if(object != charID)
			{
				int mobDist = 0;
				int distX = 0;
				int distY = 0;

				if(myX > PosX)
					distX = myX - PosX;
				else
					distX = PosX - myX;

				if(myY > PosY)
					distY = myY - PosY;
				else
					distY = PosY - myY;

				mobDist = distX + distY;
				monsters.at(FindMonster(object)).distance = mobDist;
			}
		}
		
	}

	// Only source
	if(packet->size == 19)
	{
		DWORD object = r.ReadDword();									// Object

		it = FindMobIt(object);											// Check do we have the object in the queue

		if(it != monsters.end() || object == charID)
		{
			r.ReadByte();												// No dest
			r.ReadByte();												// ?
			r.ReadWord();												// Angle ?
			r.ReadByte();												// Has got source

			BYTE XSec = r.ReadByte();
			BYTE YSec = r.ReadByte();
			WORD X = r.ReadWord();
			r.ReadDword();
			WORD Y = r.ReadWord();

			int PosX = (int)(XSec - 135) * 192 + X / 10;
			int PosY = (int)(YSec - 92) * 192 + Y / 10;

			if(object == charID)
			{
				myX = PosX;
				myY = PosY;

				// Update distance for all monsters
				for(int i = 0; i <= monsters.size(); i++)
				{
					int mobDist = 0;
					int distX = 0;
					int distY = 0;

					if(myX > monsters.at(i).x)
						distX = myX - monsters.at(i).x;
					else
						distX = monsters.at(i).x - myX;

					if(myY > monsters.at(i).y)
						distY = myY - monsters.at(i).y;
					else
						distY = monsters.at(i).y - myY;

					mobDist = distX + distY;
					monsters.at(i).distance = mobDist;
				}
			}

			if(object != charID)
			{
				int mobDist = 0;
				int distX = 0;
				int distY = 0;

				if(myX > PosX)
					distX = myX - PosX;
				else
					distX = PosX - myX;

				if(myY > PosY)
					distY = myY - PosY;
				else
					distY = PosY - myY;

				mobDist = distX + distY;
				monsters.at(FindMonster(object)).distance = mobDist;
			}
		}
	}

	if(packet->size == 14)											// No source
	{
		DWORD object = r.ReadDword();								// Object

		// Check do we have the object in the queue
		it = FindMobIt(object);	

		if(it != monsters.end() || object == charID)
		{
			r.ReadByte();												// Has dest

			BYTE XSec = r.ReadByte();
			BYTE YSec = r.ReadByte();
			WORD X = r.ReadWord();
			r.ReadWord();
			WORD Y = r.ReadWord();

			int PosX = (int)(XSec - 135) * 192 + X / 10;
			int PosY = (int)(YSec - 92) * 192 + Y / 10;

			if(object == charID)
			{
				myX = PosX;
				myY = PosY;

				// Update distance for all monsters
				for(int i = 0; i <= monsters.size(); i++)
				{
					int mobDist = 0;
					int distX = 0;
					int distY = 0;

					if(myX > monsters.at(i).x)
						distX = myX - monsters.at(i).x;
					else
						distX = monsters.at(i).x - myX;

					if(myY > monsters.at(i).y)
						distY = myY - monsters.at(i).y;
					else
						distY = monsters.at(i).y - myY;

					mobDist = distX + distY;
					monsters.at(i).distance = mobDist;
				}
			}

			if(object != charID)
			{
				int mobDist = 0;
				int distX = 0;
				int distY = 0;

				if(myX > PosX)
					distX = myX - PosX;
				else
					distX = PosX - myX;

				if(myY > PosY)
					distY = myY - PosY;
				else
					distY = PosY - myY;

				mobDist = distX + distY;
				monsters.at(FindMonster(object)).distance = mobDist;
			}
		}
	}
}

void cBot::ParseB245(tPacket * packet)
{
	if(packet->size == 0x02)
	{
		PacketReader r(packet);

		WORD obstacleCheck = r.ReadWord();

		// If we cannot attack due to an obstacle, then we get 4098
		if(obstacleCheck == 4098)
		{
			printf("Cannot attack due to an obstacle.\n");

			// Delete the monster from queue			
			std::vector<monster>::iterator it = FindMobIt(currentMonster);			// Iterator

			if(it != monsters.end())												// If we found something
			{
				char lb_id[10];
				sprintf(lb_id, "%.8X", swap(currentMonster));

				int i = ListBox_FindStringExact(GetDlgItem(mobsHwnd, IDC_MOBIDS), -1, lb_id);
				if(i != LB_ERR)
				{
					ListBox_DeleteString(GetDlgItem(mobsHwnd, IDC_MOBIDS), i);
					ListBox_DeleteString(GetDlgItem(mobsHwnd, IDC_MOBCOORDS), i);
					ListBox_DeleteString(GetDlgItem(mobsHwnd, IDC_MOBLIST), i);
					ListBox_DeleteString(GetDlgItem(mobsHwnd, IDC_MOBDIST), i);
				}

				monsters.erase(it);			// Delete the monster from our queue
			}

			hunting = false;
		}
	}
}

// Hunt
void cBot::Hunt(DWORD client)
{
	int smallest = 10000, index = 0;							// Declarations for finding the nearest monster

	for(int i = 0; i <= monsters.size(); i++)				// Find the nearest monster
	{
		if(monsters.at(i).distance < smallest)
		{
			index = i;
		}
	}

	if(monsters.at(index).id != NULL)
	{
		printf("Attacking object %.8X ...\n", monsters.at(index).id);
		//NormalAtkMonster(monsters.at(index).id, client);

		hunting = true;
		currentMonster = monsters.at(index).id;
	}
}
