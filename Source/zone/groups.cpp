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
#include "../common/debug.h"
#include "masterentity.h"
#include "NpcAI.h"
#include "../common/packet_functions.h"
#include "../common/packet_dump.h"
#include "worldserver.h"
extern EntityList entity_list;
extern WorldServer worldserver;

#ifdef GUILDWARS
#include "StringIDs.h"
#include "../GuildWars/GuildWars.h"
extern GuildLocationList location_list;
extern GuildWars guildwars;
#include "../common/guilds.h"
extern GuildRanks_Struct guilds[512];
#endif

#ifdef RAIDADDICTS
#include "RaidAddicts.h"
extern RaidAddicts raidaddicts;
#endif

//
// Xorlac: This will need proper synchronization to make it work correctly.
//			Also, should investigate client ack for packet to ensure proper synch.
//

/*

note about how groups work:
A group contains 2 list, a list of pointers to members and a 
list of member names. All members of a group should have their
name in the membername array, wether they are in the zone or not.
Only members in this zone will have non-null pointers in the
members array. 

*/

Group::Group(int32 gid) {
	memset(members,0,sizeof(Mob*) * MAX_GROUP_MEMBERS);
	int i;
	for(i=0;i<MAX_GROUP_MEMBERS;i++)
		memset(membername[i],0,64);
#ifdef ENABLE_GROUP_LINKING
	for (i = 0; i < MAX_GROUP_LINKS; i++) {
		link[i] = 0;
	}
#endif
	id = gid;
	if(id != 0) {
		if(!LearnMembers())
			id = 0;
	}
}

Group::Group(Mob* leader)
{
	memset(members,0,sizeof(Mob*) * MAX_GROUP_MEMBERS);
	members[0] = leader;
	 leader->CastToClient()->isgrouped = true;
	SetLeader(leader);
	int i;
#ifdef ENABLE_GROUP_LINKING
	for (i = 0; i < MAX_GROUP_LINKS; i++) {
		link[i] = 0;
	}
#endif
	for(i=0;i<MAX_GROUP_MEMBERS;i++)
		memset(membername[i],0,64);
	strcpy(membername[0],leader->GetName());
	strcpy(leader->CastToClient()->GetPP().groupMembers[0],leader->GetName());
}

//Cofruben:Split money used in OP_Split.
//Rewritten by Father Nitwit
void Group::SplitMoney(uint32 copper, uint32 silver, uint32 gold, uint32 platinum, Client *splitter) {
	//avoid unneeded work
	if(copper == 0 && silver == 0 && gold == 0 && platinum == 0)
		return;
  
  //I could not get MoneyOnCorpse to work
  APPLAYER* outapp = new APPLAYER(OP_MoneyUpdate,sizeof(MoneyUpdate_Struct));
  MoneyUpdate_Struct* mus= (MoneyUpdate_Struct*)outapp->pBuffer;  
	
  int i;
  int8 membercount = 0;
  for (i = 0; i < MAX_GROUP_MEMBERS; i++) { 
	  if (members[i] != NULL) {

		  membercount++; 
	  } 
  } 

  if (membercount == 0) 
	  return;
  
  uint32 mod;
  //try to handle round off error a little better
  if(membercount > 1) {
	mod = platinum % membercount;
  	if((mod) > 0) {
  		platinum -= mod;
  		gold += 10 * mod;
  	}
	mod = gold % membercount;
  	if((mod) > 0) {
  		gold -= mod;
  		silver += 10 * mod;
  	}
	mod = silver % membercount;
  	if((mod) > 0) {
  		silver -= mod;
  		copper += 10 * mod;
  	}
  }
  
  //calculate the splits
  //We can still round off copper pieces, but I dont care
  uint32 sc;
  uint32 cpsplit = copper / membercount;
  sc = copper   % membercount;
  uint32 spsplit = silver / membercount;
  uint32 gpsplit = gold / membercount;
  uint32 ppsplit = platinum / membercount;

  char buf[128];
  buf[63] = '\0';
  string msg = "You receive";
  bool one = false;
  
  if(ppsplit > 0) {
	 snprintf(buf, 63, " %u platinum", ppsplit);
	 msg += buf;
	 one = true;
  }
  if(gpsplit > 0) {
	 if(one)
	 	msg += ",";
	 snprintf(buf, 63, " %u gold", gpsplit);
	 msg += buf;
	 one = true;
  }
  if(spsplit > 0) {
	 if(one)
	 	msg += ",";
	 snprintf(buf, 63, " %u silver", spsplit);
	 msg += buf;
	 one = true;
  }
  if(cpsplit > 0) {
	 if(one)
	 	msg += ",";
	 //this message is not 100% accurate for the splitter
	 //if they are receiving any roundoff
	 snprintf(buf, 63, " %u copper", cpsplit);
	 msg += buf;
	 one = true;
  }
  msg += " as your split";
  
  for (i = 0; i < MAX_GROUP_MEMBERS; i++) { 
	  if (members[i] != NULL && members[i]->IsClient()) { // If Group Member is Client
	  	Client *c = members[i]->CastToClient();
		  c->AddMoneyToPP(cpsplit, spsplit, gpsplit, ppsplit, false);
		  
		  mus->platinum = c->GetPP().platinum;
		  mus->gold = c->GetPP().gold;
		  mus->silver = c->GetPP().silver;
		  mus->copper = c->GetPP().copper;
		  if(c == splitter)
		  	mus->copper += sc;
		  c->QueuePacket(outapp);
		  
		  c->Message(2, msg.c_str());
	  }
  }
  safe_delete(outapp);
}

bool Group::AddMember(Mob* newmember)
{
	int i=0;
	//see if they are allready in the group
	 for (i = 0; i < MAX_GROUP_MEMBERS; i++) {
		if(members[i] != NULL && !strcasecmp(members[i]->GetName(),newmember->GetName()))
			return false;
	}
	//put them in the group
	for (i = 0; i < MAX_GROUP_MEMBERS; i++) {
		if (members[i] == NULL) {
			members[i] = newmember;
			break;
		}
	}
	if ((i == MAX_GROUP_MEMBERS) || (!newmember->IsClient()))
		return false;
	strcpy(membername[i],newmember->GetName());
	int x=1;
	
	//build the template join packet	
	APPLAYER* outapp = new APPLAYER(OP_GroupUpdate,sizeof(GroupJoin_Struct));
	GroupJoin_Struct* gj = (GroupJoin_Struct*) outapp->pBuffer;	
	strcpy(gj->membername, newmember->GetName());
	gj->action = 0;
	
	
	for (i = 0;i < MAX_GROUP_MEMBERS; i++) {
		if (members[i] != NULL && members[i] != newmember) {
			//fill in group join & send it
			strcpy(gj->yourname,members[i]->GetName());
			members[i]->CastToClient()->QueuePacket(outapp);
			
			//put new member into existing person's list
			strcpy(members[i]->CastToClient()->GetPP().groupMembers[this->GroupCount()-1],newmember->GetName());
			
			//put this existing person into the new member's list
			if(IsLeader(members[i])){
				strcpy(newmember->CastToClient()->GetPP().groupMembers[0],members[i]->GetName());
			} else{
				strcpy(newmember->CastToClient()->GetPP().groupMembers[x],members[i]->GetName());
				x++;
			}
		}
	}
	
	//put new member in his own list.
	strcpy(newmember->CastToClient()->GetPP().groupMembers[x],newmember->GetName());
	newmember->isgrouped = true;
	
	if(newmember->IsClient()) {
		newmember->CastToClient()->Save();
		database.SetGroupID(newmember->GetName(), GetID());
	}
	
	safe_delete(outapp);
	return true;
}

void Group::QueuePacket(const APPLAYER *app, bool ack_req)
{
	for(int i = 0; i < MAX_GROUP_MEMBERS; i++)
		if(members[i] && members[i]->IsClient())
			members[i]->CastToClient()->QueuePacket(app, ack_req);
}

// solar: sends the rest of the group's hps to member.  this is useful when
// someone first joins a group, but otherwise there shouldn't be a need to
// call it
void Group::SendHPPackets(Mob *member)
{
	APPLAYER hpapp;
	int i;

	if(!member || !member->IsClient())
		return;

	for (i = 0; i < MAX_GROUP_MEMBERS; i++)
	{
		if(members[i] && members[i] != member)
		{
			members[i]->CreateHPPacket(&hpapp);
			member->CastToClient()->QueuePacket(&hpapp, false);
		}
	}
}

//updates a group member's client pointer when they zone in
//if the group was in the zone allready
bool Group::UpdatePlayer(Mob* update){
	VerifyGroup();
	
	int i=0;
	if(update->IsClient()) {
		//update their player profile
		PlayerProfile_Struct &pp = update->CastToClient()->GetPP();
		for (i = 0; i < MAX_GROUP_MEMBERS; i++) {
			if(membername[0] == '\0')
				memset(pp.groupMembers[i], 0, 64);
			else
				strncpy(pp.groupMembers[i], membername[i], 64);
		}
	}
	
	for (i = 0; i < MAX_GROUP_MEMBERS; i++)
	{
		if (!strcasecmp(membername[i],update->GetName()))
		{
			members[i] = update;
			members[i]->isgrouped = true;
			return true;
		}
	}
	return false;
}


void Group::MemberZoned(Mob* removemob) {
	int i;

	if (removemob == NULL)
		return;

	 for (i = 0; i < MAX_GROUP_MEMBERS; i++) {
		  if (members[i] == removemob) {
				members[i] = NULL;
				//should NOT clear the name, it is used for world communication.
				break;
		  }
	 }
}

bool Group::DelMember(Mob* oldmember,bool ignoresender){
	int i;

	if (oldmember == NULL)
	 {
		return false;
	 }

	 for (i = 0; i < MAX_GROUP_MEMBERS; i++)
	 {
		  if (members[i] == oldmember)
		  {
		  	//handle leader quitting group gracefully
			if (oldmember == GetLeader() && GroupCount() > 2)
			{
				APPLAYER* outapp = new APPLAYER(OP_GroupUpdate,sizeof(GroupJoin_Struct));

				GroupJoin_Struct* gu = (GroupJoin_Struct*) outapp->pBuffer;
				gu->action = 8;
				for (int nl = 0; nl < MAX_GROUP_MEMBERS; nl++) {
					if (members[nl] && members[nl] != oldmember) {
						strcpy(gu->membername, members[nl]->GetName());
						strcpy(gu->yourname, oldmember->GetName());
						SetLeader(members[nl]);
						for (int ld = 0; ld < MAX_GROUP_MEMBERS; ld++) {
							if (members[ld] && members[ld] != oldmember) {
								members[ld]->CastToClient()->QueuePacket(outapp);
							}
						}
						break;
					}
				}
				
				safe_delete(outapp);
			}
			members[i] = NULL;
			membername[i][0] = '\0';
			break;
		  }
	 }
	 memset(membername[i],0,64);
	APPLAYER* outapp = new APPLAYER(OP_GroupUpdate,sizeof(GroupJoin_Struct));

	GroupJoin_Struct* gu = (GroupJoin_Struct*) outapp->pBuffer;
	gu->action = groupActLeave;
	strcpy(gu->membername, oldmember->GetName());
	strcpy(gu->yourname, oldmember->GetName());

	for (i = 0; i < MAX_GROUP_MEMBERS; i++)
	{
		  if (members[i] == NULL) {
					 //if (DEBUG>=5) LogFile->write(EQEMuLog::Debug, "Group::DelMember() null member at slot %i", i);
					 continue;
		  }
		  if (members[i] != oldmember && members[i]->IsClient()) {
			strcpy(gu->yourname, members[i]->GetName());
				members[i]->CastToClient()->QueuePacket(outapp);
		}
		#ifdef IPC
		if(members[i] == oldmember && members[i]->IsNPC() && members[i]->CastToNPC()->IsGrouped() && members[i]->CastToNPC()->IsInteractive()) {
			 members[i]->CastToNPC()->TakenAction(23,0);
		}
		#endif	
	}

	if (!ignoresender && oldmember->IsClient()) {
		strcpy(gu->yourname,oldmember->GetName());
		strcpy(gu->membername,oldmember->GetName());
		gu->action = groupActLeave;

		 oldmember->CastToClient()->QueuePacket(outapp);
	 }

	database.SetGroupID(oldmember->GetName(), 0);
	
	oldmember->isgrouped = false;
	disbandcheck = true;

	 safe_delete(outapp);
	return true;	
}

// does the caster + group
void Group::CastGroupSpell(Mob* caster, uint16 spell_id)
{
	int z;
	float range, distance;

	if(!caster)
		return;

	castspell = true;
	
	range = spells[spell_id].aoerange;
	
	float mod = 0;
	if (caster->IsClient() && IsBardSong(spell_id)) {
		switch (caster->GetAA(aaExtendedNotes) + caster->GetAA(aaExtendedNotes2))
		{
			case 1:
				mod += range * 0.10;
				break;
			case 2:
				mod += range * 0.15;
				break;
			case 3:
			case 4:
			case 5:
			case 6:
				mod += range * 0.25;
				break;
		}
		switch (caster->GetAA(aaSionachiesCrescendo)+caster->GetAA(aaSionachiesCrescendo2))
		{
			case 1:
				mod += range * 0.05;
				break;
			case 2:
				mod += range * 0.10;
				break;
			case 3:
			case 4:
			case 5:
			case 6:
				mod += range * 0.15;
				break;
		}
		range += mod;
	}
	
	if(caster->IsClient())
	{
		range = caster->CastToClient()->GetActSpellRange(spell_id, range);
	}
	
	float range2 = range*range;

//	caster->SpellOnTarget(spell_id, caster);

	for(z=0; z < MAX_GROUP_MEMBERS; z++)
	{
		if(members[z] == caster) {
			caster->SpellOnTarget(spell_id, caster);
#ifdef GROUP_BUFF_PETS
			if(caster->GetPet() != NULL)
				caster->SpellOnTarget(spell_id, caster->GetPet());
#endif
		}
		else if(members[z] != NULL)
		{
			distance = caster->DistNoRoot(*members[z]);
			if(distance <= range2) {
				caster->SpellOnTarget(spell_id, members[z]);
#ifdef GROUP_BUFF_PETS
				if(members[z]->GetPet() != NULL)
					caster->SpellOnTarget(spell_id, members[z]->GetPet());
#endif
			}
#if EQDEBUG >= 5
			else
				caster->Message(0, "Group spell: %s is out of range %f at distance %f", members[z]->GetName(), range, distance);
#endif
		}
	}

	castspell = false;
	disbandcheck = true;
	
#ifdef ENABLE_GROUP_LINKING
	//dont give links with short spells...
	//if(spells[spellid].buffduration < 150)
	//	return;
	
	//cast on links
	Group* lnkgrp = NULL;
	int i;
	for (i = 0; i < MAX_GROUP_LINKS; i++) {
		if (link[i] == 0)
			continue;
		lnkgrp = entity_list.GetGroupByID(link[i]);
		if(lnkgrp == NULL)
			continue;
		for(z=0; z < MAX_GROUP_MEMBERS; z++)
		{
			if(lnkgrp->members[z] != NULL)
			{
				distance = caster->DistNoRoot(*lnkgrp->members[z]);
				if(distance <= range2) {
					caster->SpellOnTarget(spell_id, lnkgrp->members[z]);
					
#ifdef GROUP_BUFF_PETS
					if(lnkgrp->members[z]->GetPet() != NULL)
						caster->SpellOnTarget(spell_id, lnkgrp->members[z]->GetPet());
#endif
				}
#if EQDEBUG >= 5
				else
					caster->Message(0, "Group spell: %s is out of range %f at distance %f", lnkgrp->members[z]->GetName(), range, distance);
#endif
			}
		}
	}
#endif
}

bool Group::IsGroupMember(Mob* client)
{
	for (int i = 0; i < MAX_GROUP_MEMBERS; i++)
	 {
		if (members[i] == client)
		  {
			return true;
		  }
	}

	return false;
}

void Group::GroupMessage(Mob* sender,const char* message) {
	int i;
	for (i = 0; i < MAX_GROUP_MEMBERS; i++) {
		if(!members[i]) {
			//they are not in zone, send using world.
			if(strlen(membername[i])>1){
				worldserver.SendChannelMessage(sender->CastToClient(), membername[i], 2, 0, 0, message);
			}
			continue;
		}

		if (members[i]->IsClient() && members[i]->CastToClient()->GetFilter(FILTER_GROUP)!=0)
			members[i]->CastToClient()->ChannelMessageSend(sender->GetName(),members[i]->GetName(),2,0,message);
		#ifdef IPC
		if (members[i]->CastToNPC()->IsInteractive() && members[i] != sender)
			members[i]->CastToNPC()->InteractiveChat(2,1,message,(sender->GetTarget() != NULL) ? sender->GetTarget()->GetName():sender->GetName(),sender);
				//InteractiveChat(int8 chan_num, int8 language, const char * message, const char* targetname,Mob* sender);
  		 #endif
	}
	
#ifdef ENABLE_GROUP_LINKING
	int j;
	for (j = 0; j < MAX_GROUP_LINKS; j++) {
		if (link[j] == 0)
			continue;
		Group* lnkgrp = entity_list.GetGroupByID(link[j]);
		if(lnkgrp == NULL) {
			link[j] = 0;	//they are gone, why keep checking.
			continue;
		}
		for (i = 0; i < MAX_GROUP_MEMBERS; i++)  {
			if(!lnkgrp->members[i]) {
				//they are not in zone, send using world.
				if(strlen(lnkgrp->membername[i])>1){
					worldserver.SendChannelMessage(sender->CastToClient(), lnkgrp->membername[i], 2, 0, 0, message);
				}
				continue;
			}
			
			if (lnkgrp->members[i]->IsClient() && lnkgrp->members[i]->CastToClient()->GetFilter(FILTER_GROUP)!=0)
				lnkgrp->members[i]->CastToClient()->ChannelMessageSend(sender->GetName(),lnkgrp->members[i]->GetName(),2,0,message);
			#ifdef IPC
			if (lnkgrp->members[i]->CastToNPC()->IsInteractive() && lnkgrp->members[i] != sender)
				lnkgrp->members[i]->CastToNPC()->InteractiveChat(2,1,message,(sender->GetTarget() != NULL) ? sender->GetTarget()->GetName():sender->GetName(),sender);
					//InteractiveChat(int8 chan_num, int8 language, const char * message, const char* targetname,Mob* sender);
	  		 #endif
		}
	}
#endif
}

int32 Group::GetTotalGroupDamage(Mob* other) {
	 int32 total = 0;
	
	for (int i = 0; i < MAX_GROUP_MEMBERS; i++) {
		if(!members[i])
			continue;
		if (other->CheckAggro(members[i]))
			total += other->GetHateAmount(members[i],true);
	}
	return total;
}

void Group::DisbandGroup() {
	APPLAYER* outapp = new APPLAYER(OP_GroupUpdate,sizeof(GroupUpdate_Struct));

	GroupUpdate_Struct* gu = (GroupUpdate_Struct*) outapp->pBuffer;
	gu->action = groupActDisband;
	
	 for (int i = 0; i < MAX_GROUP_MEMBERS; i++) {
		if (members[i] == NULL) {
			if(membername[i][0] == '\0')
				continue;	//no member at all
			
			//member is not in this zone, have world boot them.
			ServerPacket* pack = new ServerPacket(ServerOP_GroupLeave, sizeof(ServerGroupLeave_Struct));
			ServerGroupLeave_Struct* sgl = (ServerGroupLeave_Struct*)pack->pBuffer;
			
			strncpy(sgl->member_name, membername[i], 64);
			
			pack->Deflate();
			worldserver.SendPacket(pack);
			safe_delete(pack);
			continue;
		}
		if (members[i]->IsClient()) {
			strcpy(gu->yourname, members[i]->GetName());
			database.SetGroupID(members[i]->GetName(), 0);
			members[i]->CastToClient()->QueuePacket(outapp);
		}
		members[i]->isgrouped = false;
		members[i] = NULL;
		membername[i][0] = '\0';
	}
	
	entity_list.RemoveGroup(id);
	if(id != 0)
		 database.ClearGroup(id);

	safe_delete(outapp);
}

bool Group::Process() {
	if(disbandcheck && !GroupCount())
		return false;
	else if(disbandcheck && GroupCount())
		disbandcheck = false;
	return true;
}

void Group::SendUpdate(int32 type, Mob* member){
	if(!member->IsClient())
		return;
	APPLAYER* outapp = new APPLAYER(OP_GroupUpdate,sizeof(GroupUpdate2_Struct));
	GroupUpdate2_Struct* gu = (GroupUpdate2_Struct*)outapp->pBuffer;	
	gu->action = type;
	strcpy(gu->yourname,member->GetName());
	
	int x=0;
	int i=0;
	for (i = 0;i < MAX_GROUP_MEMBERS; i++) {
		if (members[i] != NULL && members[i] != member) {
			if(IsLeader(members[i])){
				strcpy(gu->leadersname,members[i]->GetName());
				strcpy(gu->membername[x],members[i]->GetName());
				x++;
			}
			else{
				strcpy(gu->membername[x],members[i]->GetName());
				x++;
			}
		}
	}
	member->CastToClient()->QueuePacket(outapp);
	safe_delete(outapp);
}

int8 Group::GroupCount() {
	int count = 0;
	for (int i = 0; i < MAX_GROUP_MEMBERS; i++)
	 {
		if (strlen(membername[i])>0)
		  {
			count++;
		  }
	}

	return count;
}

int32 Group::GetHighestLevel()
{
int32 level = 0;
	for (int i = 0; i < MAX_GROUP_MEMBERS; i++)
	 {
		if (members[i])
		  {
			if(members[i]->GetLevel() > level)
				level = members[i]->GetLevel();
		  }
	}
	return level;
}
int32 Group::GetLowestLevel()
{
int32 level = 255;
	for (int i = 0; i < MAX_GROUP_MEMBERS; i++)
	 {
		if (members[i])
		  {
			if(members[i]->GetLevel() < level)
				level = members[i]->GetLevel();
		  }
	}
	return level;
}

#ifdef ENABLE_GROUP_LINKING
void Group::ClearLink(int32 clear_id, bool all)
{
	for (int m = 0; m < 8; m++)
	{
		if (all)
		{
			link[m] = 0;
		}
		else if (link[m] == clear_id)
		{
			link[m] = 0;
			return;
		}
	}
}

bool Group::IsLinked(int32 link_id)
{
	for (int m = 0; m < 8; m++)
	{
		if (link[m] == link_id)
		{
			return true;
		}
	}
	return false;
}

void Group::EstablishLink(int32 link_id)
{
	for (int m = 0; m < 8; m++)
	{
		if (link[m] == 0)
		{
			link[m] = link_id;
			return;
		}
	}
}
#endif

#ifdef GUILDWARS
void Group::CauseEXPLoss() {
	for(int i=0;i<MAX_GROUP_MEMBERS;i++)
	{
		if(!members[i])
			continue;
		else if(members[i]->IsClient())
			members[i]->CastToClient()->SetEXP((int32)(members[i]->CastToClient()->GetEXP() - members[i]->GetLevel()*((float)members[i]->GetLevel()/18)*1000 > 0)? (int32)(members[i]->CastToClient()->GetEXP() - members[i]->GetLevel()*((float)members[i]->GetLevel()/18)*1000) : 1,members[i]->CastToClient()->GetAAXP());
	}
}

void Group::GivePoints(Client* killed) {
for(int i=0;i<MAX_GROUP_MEMBERS;i++)
{
if(!members[i])
continue;
else if(members[i]->IsClient())
{
sint32 points = guildwars.PlayerPointsEarned(members[i]->CastToClient(),killed);

				if(points > 0 && members[i]->CastToClient()->GuildDBID() != 0)
				{
				members[i]->CastToClient()->UpdateLDoNPoints(points,0);

				members[i]->CastToClient()->Message(0,"You received %i points killing %s",points,GetName());
				}
				else if(points == -1)
				members[i]->CastToClient()->Message(0,"You have killed %s within the last 10 minutes, you receive no points.",GetName());
				else if(points == 0)
				members[i]->CastToClient()->Message(0,"You received no points killing %s.",GetName());
}
}
}
#endif

#ifdef RAIDADDICTS
void Group::RASplitPointsAndEXP(uint32 exp, Mob* other)
{
	/* 3.0 Code
	int i;
	for (i = 0; i < MAX_GROUP_MEMBERS; i++)
	{
		if (members[i] != NULL && members[i]->IsClient())
    		{
				raidaddicts.NPCDeathProcess(npcid, members[i]);
		}
	} */

	int i; 
	uint32 groupexp = exp; 
	int8 membercount = 0; 
	int8 maxlevel = 1; 
	for (i = 0; i < MAX_GROUP_MEMBERS; i++) { 
		if (members[i] != NULL) { 
			if(members[i]->GetLevel() > maxlevel) maxlevel = members[i]->GetLevel();
			membercount++;
		}
	}

	if (membercount == 0) 
		return; 

	for (i = 0; i < MAX_GROUP_MEMBERS; i++) { 
		if (members[i] != NULL && members[i]->IsClient()) { // If Group Member is Client
			if(members[i]->GetLevelCon(other->GetLevel()) != CON_GREEN) {// If Mob doesn't con green
				sint16 diff = members[i]->GetLevel() - maxlevel; 
				if (diff >= -8){
					if (!raidaddicts.NPCDeathProcess(other->GetNPCTypeID(), members[i], ((members[i]->GetLevel()+3) * (members[i]->GetLevel()+3) * 75*3.5f < groupexp/membercount ) ? (int32)(members[i]->GetLevel() * members[i]->GetLevel() * 75*3.5f):(int32)(groupexp/membercount))) {
						members[i]->CastToClient()->AddEXP(((members[i]->GetLevel()+3) * (members[i]->GetLevel()+3) * 75*3.5f < groupexp/membercount ) ? (int32)(members[i]->GetLevel() * members[i]->GetLevel() * 75*3.5f):(int32)(groupexp/membercount) ); 
						members[i]->CastToClient()->Message(15, "You did not recieve any points from this creature.");
					}
				} 
			} 
		}
	} 
}
#endif

void Group::TeleportGroup(Mob* sender, int32 zoneID, float x, float y, float z)
{
	 for (int i = 0; i < MAX_GROUP_MEMBERS; i++)
	 {
	 #ifdef IPC
		if (members[i] != NULL && (members[i]->IsClient() || (members[i]->IsNPC() && members[i]->CastToNPC()->IsInteractive())) && members[i] != sender)
	 #else
		  if (members[i] != NULL && members[i]->IsClient() && members[i] != sender)
	 #endif
	 	{
		  members[i]->CastToClient()->MovePC(zoneID, x, y, z);
		}
	}	
}

bool Group::LearnMembers() {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (database.RunQuery(query,MakeAnyLenString(&query, "SELECT name FROM character_ WHERE groupid=%lu", id),errbuf,&result)){
		safe_delete_array(query);
		if(mysql_num_rows(result) < 1) {	//could prolly be 2
			mysql_free_result(result);
			LogFile->write(EQEMuLog::Error, "Error getting group members for group %lu: %s", id, errbuf);
			return(false);
		}
		int i = 0;
		while((row = mysql_fetch_row(result))) {
			if(!row[0])
				continue;
			members[i] = NULL;
			strncpy(membername[i], row[0], 64);
			i++;
		}
		mysql_free_result(result);
	}
	return(true);
}

void Group::VerifyGroup() {
	/*
		The purpose of this method is to make sure that a group
		is in a valid state, to prevent dangling pointers.
		Only called every once in a while (on member re-join for now).
	*/
	
	for (int i = 0; i < MAX_GROUP_MEMBERS; i++) {
		if (membername[i][0] == '\0') {
#if EQDEBUG >= 7
LogFile->write(EQEMuLog::Debug, "Group %lu: Verify %d: Empty.\n", id, i);
#endif
			members[i] = NULL;
			continue;
		}
		
		//it should be safe to use GetClientByName, but Group is trying
		//to be generic, so we'll go for general Mob
		Mob *them = entity_list.GetMob(membername[i]);
		if(them == NULL && members[i] != NULL) {	//they arnt here anymore....
#if EQDEBUG >= 6
		LogFile->write(EQEMuLog::Debug, "Member of group %lu named '%s' has disappeared!!", id, membername[i]);
#endif
			membername[i][0] = '\0';
			members[i] = NULL;
			continue;
		}
		
		if(them != NULL && members[i] != them) {	//our pointer is out of date... not so good.
#if EQDEBUG >= 5
		LogFile->write(EQEMuLog::Debug, "Member of group %lu named '%s' had an out of date pointer!!", id, membername[i]);
#endif
			members[i] = them;
			continue;
		}
#if EQDEBUG >= 8
		LogFile->write(EQEMuLog::Debug, "Member of group %lu named '%s' is valid.", id, membername[i]);
#endif
	}
}



void Client::LeaveGroup() {
	Group *g = GetGroup();
	
	if(g) {
		if(g->GroupCount() < 3)
			g->DisbandGroup();
		else
			g->DelMember(this);
	} else {
		//force things a little
		database.SetGroupID(GetName(), 0);
	}
	
	isgrouped = false;
}





