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
	
	bool	Process();
	
	//packet stuff
	void	Message(Client* sender,const char* message);
	void	QueuePacket(const EQApplicationPacket *app, bool ack_req = true);
	EQApplicationPacket	*BuildFullUpdate(Client *for_who);
	
	//leadership stuff
	void	SetLeader(Client* newleader){ leader=newleader; };
	bool	IsLeader(Mob* leadertest) { return leadertest==leader; };
	
	//group management
	void	AddGroup(Group *g);
	
	//member stuff
	bool	ContainsMember(const char *name);
	bool	ContainsMember(Client *who);
	int8	GroupCount();
	int32	GetHighestLevel();
	int32	GetLowestLevel();
	//Loads up the structure from the database
	bool	LearnMembers();
	
protected:
	Client *leader;
	Group *groups[MAX_RAID_GROUPS];
};


#endif

