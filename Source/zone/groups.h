/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2002  EQEMu Development Team (http://eqemu.org)

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
#ifndef GROUPS_H
#define GROUPS_H

#include "../common/types.h"
#include "../common/linked_list.h"
#include "../common/eq_opcodes.h"
#include "../common/eq_packet_structs.h"
#include "entity.h"
#include "mob.h"
#include "../common/servertalk.h"

#define MAX_GROUP_MEMBERS 6

class Group: public Entity
{
public:
	Group::~Group() {}
	Group::Group(Mob* leader);
	Group::Group(SendGroup_Struct* sgs);
	bool	AddMember(Mob* newmember);
	void	SendUpdate(int32 type,Mob* member);
	void	SendWorldGroup(int32 zone_id,Mob* zoningmember);
	bool	DelMember(Mob* oldmember,bool ignoresender = false);
	void	DisbandGroup();
	bool	IsGroupMember(Mob* client);
	bool	Process();
	bool	IsGroup()			{ return true; }
	void	CastGroupSpell(Mob* caster,uint16 spellid);
	void	SplitExp(uint32 exp, Mob* other);
	void	GroupMessage(Mob* sender,const char* message);
	int32	GetTotalGroupDamage(Mob* other);
	//Cofruben: Split money used in OP_Split
	void SplitMoney(uint32 copper, uint32 silver, uint32 gold, uint32 platinum);
	Mob* members[MAX_GROUP_MEMBERS];
	char	membername[MAX_GROUP_MEMBERS][64];
	void	SetLeader(Mob* newleader){ leader=newleader; };
	Mob*	GetLeader(){ return leader; };
	char*	GetLeaderName(){ return membername[0]; };
	void	SendHPPackets(Mob* newmember);
	bool	UpdatePlayer(Mob* update);
	void	Remove(Mob* removemob);
	bool	IsLeader(Mob* leadertest) { return leadertest==leader; };
	int8	GroupCount();
	int32	GetHighestLevel();
	void	QueuePacket(const APPLAYER *app, bool ack_req = true);
	void	TeleportGroup(Mob* sender, int32 zoneID, float x, float y, float z);

	bool	disbandcheck;
	bool	castspell;

#ifdef GUILDWARS
	void	CauseEXPLoss();
	void	CauseFactionLoss(Mob* killed);
	void	GivePoints(Client* killed);
#endif

#ifdef RAIDADDICTS
	void	RASplitPoints(sint32 npcid);
#endif

private:
	Mob*	leader;
};

#endif
