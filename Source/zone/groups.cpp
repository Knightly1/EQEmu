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
//         Also, should investigate client ack for packet to ensure proper synch.
//

Group::Group(Mob* leader)
{
	memset(members,0,sizeof(Mob*) * MAX_GROUP_MEMBERS);
	members[0] = leader;
    leader->CastToClient()->isgrouped = true;
	SetLeader(leader);
	int i;
	for(i=0;i<MAX_GROUP_MEMBERS;i++)
		memset(membername[i],0,64);
	strcpy(membername[0],leader->GetName());
	strcpy(leader->CastToClient()->GetPP().groupMembers[0],leader->GetName());
}
Group::Group(SendGroup_Struct* sgs)
{
	memset(members,0,sizeof(Mob*) * MAX_GROUP_MEMBERS);
	int i;
	for(i=0;i<MAX_GROUP_MEMBERS;i++)
		memset(membername[i],0,64);
	for(i=1;i<sgs->grouptotal;i++){	
		strcpy(membername[i],sgs->members[i]);
	}
	strcpy(membername[0],sgs->leader);
}

//Cofruben:Split money used in OP_Split.
//Rewritten by Father Nitwit
void Group::SplitMoney(uint32 copper, uint32 silver, uint32 gold, uint32 platinum) {
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
  uint32 cpsplit = copper / membercount;
  uint32 spsplit = silver / membercount;
  uint32 gpsplit = gold / membercount;
  uint32 ppsplit = platinum / membercount;
  
  //make sure they at least get something, since they started with something...
  if(cpsplit == 0 && spsplit == 0 && gpsplit == 0 && ppsplit == 0)
  	cpsplit = 1;
  
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
        c->QueuePacket(outapp);
        
        c->Message(2, msg.c_str());
     }
  }
  safe_delete(outapp);
}

bool Group::AddMember(Mob* newmember)
{
	int i=0;
    for (i = 0; i < MAX_GROUP_MEMBERS; i++) {
		if(members[i] != NULL && !strcasecmp(members[i]->GetName(),newmember->GetName()))
			return false;
	}
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
	APPLAYER* outapp = new APPLAYER(OP_GroupUpdate,sizeof(GroupJoin_Struct));
	GroupJoin_Struct* gj = (GroupJoin_Struct*) outapp->pBuffer;	
	strcpy(gj->membername, newmember->GetName());
	gj->action = 0;      
	for (i = 0;i < MAX_GROUP_MEMBERS; i++) {
		if (members[i] != NULL && members[i] != newmember) {
			strcpy(gj->yourname,members[i]->GetName());		
			members[i]->CastToClient()->QueuePacket(outapp);
			strcpy(members[i]->CastToClient()->GetPP().groupMembers[this->GroupCount()-1],newmember->GetName());
			if(IsLeader(members[i])){
				strcpy(newmember->CastToClient()->GetPP().groupMembers[0],members[i]->GetName());
			}
			else{
				strcpy(newmember->CastToClient()->GetPP().groupMembers[x],members[i]->GetName());
				x++;
			}
		}
	}
	strcpy(newmember->CastToClient()->GetPP().groupMembers[x],newmember->GetName());
	newmember->CastToClient()->Save();
    newmember->isgrouped = true;	
    safe_delete(outapp);
	return true;
}
void Group::SendUpdate(int32 type,Mob* member){
	if(!member->IsClient())
		return;
	APPLAYER* outapp = new APPLAYER(OP_GroupUpdate,sizeof(GroupUpdate2_Struct));
	GroupUpdate2_Struct* gu = (GroupUpdate2_Struct*)outapp->pBuffer;	
	gu->action=type;
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
//
// Xorlac: Does this consider side effects of being the last member to disband in a group, leadership changes, etc?
//
void Group::SendWorldGroup(int32 zone_id,Mob* zoningmember){
	ServerPacket* pack = new ServerPacket(ServerOP_SendGroup,sizeof(SendGroup_Struct));
	SendGroup_Struct* sgs=(SendGroup_Struct*)pack->pBuffer;
	sgs->grouptotal=GroupCount();
	sgs->zoneid=zone_id;
	strcpy(sgs->thismember,zoningmember->GetName());
	strcpy(sgs->leader,GetLeaderName());
	int i=0;
	for (i = 0; i < MAX_GROUP_MEMBERS; i++)
    {
        if (strlen(membername[i])>1)
        {
			strcpy(sgs->members[i],membername[i]);
        }
    }
    pack->Deflate();
	worldserver.SendPacket(pack);
	safe_delete(pack);
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

bool Group::UpdatePlayer(Mob* update){
	int i=0;
	for (i = 0; i < MAX_GROUP_MEMBERS; i++)
    {
        if (!strcasecmp(membername[i],update->GetName()))
        {
            members[i] = update;
			members[i]->isgrouped=true;
            return true;
        }
    }
	return false;
}
void Group::Remove(Mob* removemob){
	int i;

	if (removemob == NULL)
    {
		return;
    }

    for (i = 0; i < MAX_GROUP_MEMBERS; i++)
    {
        if (members[i] == removemob)
        {
            members[i] = NULL;
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
            members[i] = NULL;
            break;
        }
    }
    memset(membername[i],0,64);
	APPLAYER* outapp = new APPLAYER(OP_GroupUpdate,sizeof(GroupJoin_Struct));

	GroupJoin_Struct* gu = (GroupJoin_Struct*) outapp->pBuffer;
	gu->action = 1;
	strcpy(gu->membername, oldmember->GetName());

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
		gu->action = 1;

	    oldmember->CastToClient()->QueuePacket(outapp);
    }

	oldmember->isgrouped = false;
	disbandcheck = true;

    safe_delete(outapp);
	return true;	
}

void Group::TeleportGroup(Mob* sender, int32 zoneID, float x, float y, float z)
{
    for (int i = 0; i < MAX_GROUP_MEMBERS; i++)
    {
    #ifdef IPC
		if (members[i] != NULL && (members[i]->IsClient() || (members[i]->IsNPC() && members[i]->CastToNPC()->IsInteractive())) && members[i] != sender) {
    #else
        if (members[i] != NULL && members[i]->IsClient() && members[i] != sender) {
    #endif
        members[i]->CastToClient()->MovePC(zoneID, x, y, z);
		}
	}	
}
void Group::DisbandGroup(){
	APPLAYER* outapp = new APPLAYER(OP_GroupUpdate,sizeof(GroupUpdate_Struct));

	GroupUpdate_Struct* gu = (GroupUpdate_Struct*) outapp->pBuffer;
	gu->action = 6;
	
    for (int i = 0; i < MAX_GROUP_MEMBERS; i++)
    {
        if (members[i] == NULL)
                continue;
		if (members[i]->IsClient()) {
			strcpy(gu->yourname, members[i]->GetName());
				database.SetGroupID(members[i]->GetName(),0);
            members[i]->CastToClient()->QueuePacket(outapp);
		}

		   

        members[i]->isgrouped = false;
	}	

	entity_list.RemoveGroup(this->GetID());

    safe_delete(outapp);
}

// does the caster + group
void Group::CastGroupSpell(Mob* caster, uint16 spellid)
{
	int z;
	float range, distance;

	if(!caster)
		return;

	castspell = true;
	
	range = spells[spellid].aoerange;
	if(caster->IsClient())
	{
		range = caster->CastToClient()->GetActSpellRange(spellid, range);
	}

	caster->SpellOnTarget(spellid, caster);

	for(z=0; z < MAX_GROUP_MEMBERS; z++)
	{
		if(members[z] != NULL && members[z] != caster)
		{
			distance = caster->Dist(*members[z]);
			if(distance <= range)
				caster->SpellOnTarget(spellid, members[z]);
#if EQDEBUG >= 5
			else
				caster->Message(0, "Group spell: %s is out of range %f at distance %f", members[z]->GetName(), range, distance);
#endif
		}
	}

	castspell = false;
	disbandcheck = true;
}

void Group::SplitExp(uint32 exp, Mob* other) 
{ 
   if( other->CastToNPC()->MerchantType == 0 ) // Ensure NPC isn't a merchant
   { 
      int i; 
      uint32 groupexp = exp; 
      int8 membercount = 0; 
      int8 maxlevel = 1; 
      for (i = 0; i < MAX_GROUP_MEMBERS; i++) 
      { 
         if (members[i] != NULL) 
         { 
            if(members[i]->GetLevel() > maxlevel) 
               maxlevel = members[i]->GetLevel(); 
            //groupexp += exp/10; 
            groupexp += (uint32)(exp * zone->GetGroupEXPBonus()); 

            membercount++; 
         } 
      } 

      if (membercount == 0) 
         return; 

      for (i = 0; i < MAX_GROUP_MEMBERS; i++) 
      { 
         if (members[i] != NULL && members[i]->IsClient()) // If Group Member is Client
          { 
            if( members[i]->GetLevelCon( other->GetLevel() ) != CON_GREEN ) // If Mob doesn't con green
            { 
               // add exp + exp cap 
               sint16 diff = members[i]->GetLevel() - maxlevel; 
               if (diff >= -8) /*Instead of person who killed the mob, the person who has the highest level in the group*/ 
               { 
                  members[i]->CastToClient()->AddEXP(((members[i]->GetLevel()+3) * (members[i]->GetLevel()+3) * 75*3.5f < groupexp/membercount ) ? (int32)(members[i]->GetLevel() * members[i]->GetLevel() * 75*3.5f):(int32)(groupexp/membercount) ); 
               } 
            } 
         } 
      } 
   } 
} 

bool Group::Process()
{
if(disbandcheck && !GroupCount())
	return false;
else if(disbandcheck && GroupCount())
	disbandcheck = false;
return true;
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

int8 Group::GroupCount()
{
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

void Group::GroupMessage(Mob* sender,const char* message)
{
	for (int i = 0; i < MAX_GROUP_MEMBERS; i++) {
		if(!members[i]){
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
void Group::RASplitPoints(sint32 npcid)
{
	int i;
	for (i = 0; i < MAX_GROUP_MEMBERS; i++)
	{
		if (members[i] != NULL && members[i]->IsClient())
    		{
				raidaddicts.NPCDeathProcess(npcid, members[i]);
		}
	}	
}
#endif

