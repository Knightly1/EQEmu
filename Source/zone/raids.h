/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2005  EQEMu Development Team (http://eqemulator.net)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef RAIDS_H
#define RAIDS_H

#include "../common/types.h"
#include "../common/linked_list.h"
#include "groups.h"
#include <vector>
#include <string>
#include <queue>

class Client;
class EQApplicationPacket;

using namespace std;

enum {	//raid packet types:
	raidAdd = 0,
	raidRemove2 = 1,	//parameter=0
	raidRemove1 = 3,	//parameter=0xFFFFFFFF
	raidMembers = 6,	//len 395+, details + members list
	raidCreate = 8,		//len 72
	
	raidNoRaid = 10,		//parameter=0
	raidChangeGroupLeader = 13,	//136 raid leader, new group leader, group_id?
	raidBecomeGroupLeader = 14,	//472 
	raidChangeGroup = 16,	//??   len 136 old leader, new leader, 0 (preceeded with a remove2)
	raidLock = 17,		//len 136 leader?, leader, 0
	raidUnlock = 18,		//len 136 leader?, leader, 0
	raidSetLeader = 20,	//len 388, contains 'details' struct without members
};

#define MAX_RAID_GROUPS 6

class Raid : public GroupIDConsumer {
public:
	Raid(Client *leader);
	Raid(uint32 raidID);
	~Raid();

	bool	Process();	
	bool	IsRaid() { return true; }
	
	//packet stuff
	void	Message(Client* sender,const char* message);
	void	Message_StringID(Mob* sender, int32 type, int32 string_id, const char* message,const char* message2=0,const char* message3=0,const char* message4=0,const char* message5=0,const char* message6=0,const char* message7=0,const char* message8=0,const char* message9=0, int32 distance = 0);
	
	void	QueuePacket(const EQApplicationPacket *app, bool ack_req = true);
	EQApplicationPacket	*BuildFullUpdate(Client *for_who);
	
	//leadership stuff
	void	SetLeader(Client* newleader){ leader=newleader; };
	Mob*	GetLeader() { return leader; }
	bool	IsLeader(Mob* leadertest) { return leadertest==leader; };
	void	DisbandRaid();
	
	//group management
	void	AddGroup(Group *g);
	void	RemoveGroup(Group *g);
	void	TeleportRaid(Mob* sender, int32 zoneID, float x, float y, float z, float heading);
	
	//member stuff
	int32	GetTotalRaidDamage(Mob* other);
	void	SplitMoney(uint32 copper, uint32 silver, uint32 gold, uint32 platinum, Client *splitter = NULL);	
	bool	ContainsMember(const char *name);
	bool	ContainsMember(Client *who);
	int8	GroupCount();
	int32	GetHighestLevel();
	int32	GetLowestLevel();
	//Loads up the structure from the database
	bool	LearnMembers();
	bool	VerifyRaid();
	
	void UpdateRaid(); //updates everyone in raid on a change

	Mob *members[MAX_RAID_GROUPS*6];
	char membername[MAX_RAID_GROUPS*6][64];
	
protected:
	Client *leader;
	Group *groups[MAX_RAID_GROUPS];
};


#endif

