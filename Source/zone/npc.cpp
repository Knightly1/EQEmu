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
#include <iostream>
using namespace std;
#include <math.h>
#include "../common/moremath.h"
#include <stdio.h>
#include "../common/packet_dump_file.h"
#include "zone.h"
#ifdef WIN32
#define snprintf	_snprintf
#define strncasecmp	_strnicmp
#define strcasecmp  _stricmp
#else
#include <stdlib.h>
#include <pthread.h>
#endif

#include "npc.h"
#include "map.h"
#include "entity.h"
#include "masterentity.h"
#include "spdat.h"
#include "../common/bodytypes.h"
#include "spawngroup.h"

#ifdef GUILDWARS
#include "../GuildWars/GuildWars.h"
extern GuildLocationList location_list;
extern GuildWars guildwars;
#endif

//#define SPELLQUEUE //Use only if you want to be spammed by spell testing

extern Database database;
extern Zone* zone;
extern volatile bool ZoneLoaded;
extern EntityList entity_list;
#ifndef NEW_LoadSPDat
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif

NPC::NPC(const NPCType* d, Spawn2* in_respawn, float x, float y, float z, float heading, bool IsCorpse)
: Mob(d->name,
	  d->lastname,
	  d->max_hp,
	  d->max_hp,
	  d->gender,
	  d->race,
	  d->class_,
      d->bodytype,
	  d->deity,
	  d->level,
	  d->npc_id, // rembrant, Dec. 20, 2001
	  d->skills, // socket 12-29-01
	  d->size,
	  d->walkspeed,
	  d->runspeed,
	  heading,
	  x,
	  y,
	  z,
	  d->light,
	  d->equipment,
	  d->texture,
	  d->helmtexture,
	  d->AC,
	  d->ATK,
	  d->STR,
	  d->STA,
	  d->DEX,
	  d->AGI,
	  d->INT,
	  d->WIS,
	  d->CHA,
	  d->haircolor,
	  d->beardcolor,
	  d->eyecolor1,
	  d->eyecolor2,
	  d->hairstyle,
// vesuvias - appearence fix
	  d->luclinface,
	  d->beard,
	  d->aa_title,
	  d->fixedZ,
	  d->d_meele_texture1,
	  d->d_meele_texture2,
	  d->see_invis,			// pass see_invis/see_ivu flags to mob constructor
	  d->see_invis_undead,
// SCORPIOUS2K - qglobal
	  d->qglobal ),
	#ifdef IPC 
        interactive_timer(1000),
	#endif
	forget_timer(500),
	attacked_timer(12000),
	swarm_timer(100),
	classattack_timer(1000),
	taunt_timer(TauntReuseTime * 1000),
	assist_timer(AIassistcheck_delay),
	sendhpupdate_timer(1000)
{
	Mob* mob = entity_list.GetMob(name);
	if(mob != 0)
		entity_list.RemoveEntity(mob->GetID());

	int moblevel=GetLevel();
	
	
	NPCTypedata = new NPCType;
	memcpy(NPCTypedata, d, sizeof(NPCType));
	respawn2 = in_respawn;
	swarm_timer.Disable();

	proximity = NULL;
	itemlist = new ItemList();
	copper = 0;
	silver = 0;
	gold = 0;
	platinum = 0;
	banishcapability=d->banish;
	max_dmg=d->max_dmg;
	min_dmg=d->min_dmg;
	passengers =false;
	grid = 0;
	wp_m = 0;
	spawn_group = 0;
	//Wandering
   /* for (int l=0;l<50;l++)
	{
		wp_x[l] = 0;
		wp_y[l] = 0;
		wp_z[l] = 0;
		wp_s[l] = 0;
		if (l < 6)
			wp_a[l] = 0;
	}*/

	
	pAggroRange = d->aggroradius;
	pAssistRange = GetAggroRange();
	mana_regen=d->mana_regen;

    // neotokyo: fix for lazy db-updaters
    if (GetCasterClass() != 'N' && mana_regen == 0)
        mana_regen = (GetLevel() / 10) + 4;
	hp_regen=d->hp_regen;
	
	//Trumpcard:  Gives low end monsters no regen if set to 0 in database. Should make low end monsters killable
	//Might want to lower this to /5 rather than 10.
	if(hp_regen == 0)
		hp_regen = (int)( moblevel / 10 );
	
    CalcMaxMana();
    SetMana(GetMaxMana());

	d_meele_texture1=d->equipment[7];
	d_meele_texture2=d->equipment[8];
#ifdef IPC
	if(IsInteractive())
	{
	//	tiredmax = RandomTimer(45,135);
		pvp = true;
	//	
	//	for(int i = 0 ; i < 9; i++) // neotokyo: out of bounds again :-/
	//		equipment[i] = RandomTimer(1000,12000);
	}
#endif	
	MerchantType=d->merchanttype; // Yodason: merchant stuff
	org_x = x;
	org_y = y;
	org_z = z;
	org_heading = heading;	
	p_depop = false;
	loottable_id = d->loottable_id;	

//	if (d->npc_faction_id)
//		DebugBreak();
	primary_faction = 0;
	SetNPCFactionID(d->npc_faction_id);
	
	pet_spell_id = 0;
	
	ignore_target = 0;
	delaytimer = false;
    feign_memory = "0";
	forgetchance = 0;
	attack_event = 0;

#ifdef GUILDWARS
	if(respawn2 != 0)
	{
	if(database.GetNPC(respawn2->GetID()))
	{
	if(guildlocationid = database.GetNPCLocationID(respawn2->GetID()))
	{
#ifdef GWDEBUG
		printf("LocationID found: %i (NPC Information: (%i;%i;%i)\n",guildlocationid,respawn2->GetID(),respawn2->SpawnGroupID(),respawn2->CurrentNPCID());
#endif
	INT = 70;
	GuildLocation* loc = location_list.FindLocationByID(GetGuildLocationID());
#ifdef GWDEBUG
		printf("FindLocationByID()\n",guildlocationid);
#endif
	if(loc != 0)
	{
	memcpy(name,guildwars.GetRandomName(),64);
	if(in_respawn && in_respawn->CurrentNPCID() < 180190 && in_respawn->CurrentNPCID() > 180192)
	race = loc->GetRaceIDForZone();

	loc->AddNPC(this);
#ifdef GWDEBUG
		printf("Location Inserted\n",guildlocationid);
#endif
	}
	}
	}
	}
#endif

	EntityList::RemoveNumbers(name);
#ifdef IPC
	if(!IsInteractive())
		entity_list.MakeNameUnique(name);
#else
    entity_list.MakeNameUnique(name);
#endif

    MR = d->MR;
    CR = d->CR;
    DR = d->DR;
    FR = d->FR;
    PR = d->PR;

    if (!MR)
        MR = (int)( moblevel * 1.1f);
    if (!CR)
        CR = (int)( moblevel * 1.1f);
    if (!DR)
        DR = (int)( moblevel * 1.1f);
    if (!FR)
        FR = (int)( moblevel * 1.1f);
    if (!PR)
        PR = (int)( moblevel * 1.1f);

	npc_aggro = d->npc_aggro;

	AI_Start();
	
	//give NPCs skill values...
	int r;
	int skil;
	for(r = 0; r <= HIGHEST_SKILL; r++) {
		skil = 4 + moblevel*3 + MakeRandomInt(1, moblevel+moblevel);
		if(skil > 250)
			skil = 250;	//cap them just like players... to keep it fairer
		skills[r] = skil;
	}
}
	  
NPC::~NPC()
{
	if(proximity != NULL) {
		entity_list.RemoveProximity(GetID());
		safe_delete(proximity);
	}
	safe_delete(itemlist);
	safe_delete(NPCTypedata);
 #ifdef IPC	  
	if(IsInteractive())
	{
	    Group* group = entity_list.GetGroupByMob(this);
		if(group != 0)
		{
		    group->DelMember(this);
	    }
    }
#endif
}

void NPC::SetTarget(Mob* mob) {
	if (mob) {
		SetAttackTimer();
	}
	else {
		attack_timer.Disable();
		attack_dw_timer.Disable();
	}
	target = mob;
}

ServerLootItem_Struct* NPC::GetItem(int slot_id) {
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	iterator.Reset();
	while(iterator.MoreElements()) {
		ServerLootItem_Struct* item = iterator.GetData();
		if (item->equipSlot == slot_id) {
			return item;
		}
		iterator.Advance();
	}
	cout << "no item found for slot: " << slot_id << endl;
	return 0;
}
	  
void NPC::RemoveItem(uint16 item_id, int16 quantity, int16 slot) {
  LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
  iterator.Reset();
  while(iterator.MoreElements()) {
    //if (iterator.GetData()->ItemNumber == item_id && iterator.GetData()->lootslot == slot)
    if (iterator.GetData()->item_id == item_id && slot <= 0  && quantity <= 0) {
          //cout<<"NPC::RemoveItem"<<" equipSlot:"<<iterator.GetData()->equipSlot<<endl;
      iterator.RemoveCurrent();
      return;
    }
    else if (iterator.GetData()->item_id == item_id && iterator.GetData()->equipSlot == slot  && quantity >= 1) {
          //cout<<"NPC::RemoveItem"<<" equipSlot:"<<iterator.GetData()->equipSlot<<" quantity:"<< quantity<<endl;
          iterator.GetData()->charges -= quantity;
          if (iterator.GetData()->charges <= 0)
            iterator.RemoveCurrent();
          return;
    }
    iterator.Advance();
  }
  return;
}
	  
void NPC::ClearItemList() {
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);

	iterator.Reset();
	while(iterator.MoreElements()) {
		iterator.RemoveCurrent();
	}
}
	  
void NPC::QueryLoot(Client* to) {
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);

	iterator.Reset();
	int x = 0;
	to->Message(0, "Coin: %ip %ig %is %ic", platinum, gold, silver, copper);
	while(iterator.MoreElements()) {
		const Item_Struct* item = database.GetItem(iterator.GetData()->item_id);
		if (item)
		    to->Message(0, "  %d: %s", item->ItemNumber, item->Name);
		else
		    LogFile->write(EQEMuLog::Error, "Database error, invalid item");
		x++;
		iterator.Advance();
	}
	to->Message(0, "%i items on %s.", x, this->GetName());
}

void NPC::AddCash(int16 in_copper, int16 in_silver, int16 in_gold, int16 in_platinum) {
	copper = in_copper;
	silver = in_silver;
	gold = in_gold;
	platinum = in_platinum;
}
	  
void NPC::AddCash() {
	copper = (rand() % 100)+1;
	silver = (rand() % 50)+1;
	gold = (rand() % 10)+1;
	platinum = (rand() % 5)+1;
}
	  
void NPC::RemoveCash() {
	copper = 0;
	silver = 0;
	gold = 0;
	platinum = 0;
}

bool NPC::Process()
{
	_ZP(NPC_Process);
    if (attacked_timer.Check() && attack_event == 1)
	{
		attack_event = 0;
	}
#ifdef IPC
    if(IsInteractive())
	{
	    if(IsEngaged() && CurrentPosition() != 0)
		    TakenAction(0,0);

        if(interactive_timer.Check())
        {
		    tired++;
			if(tired >= tiredmax && CurrentPosition() != 1 && !IsEngaged())
			{
			    TakenAction(1,0);
            }
			if(tired >= tiredmax)
			    tired = 0;
		}
    }
#endif
    adverrorinfo = 1;
	if (IsStunned() && stunned_timer.Check())
    {
        this->stunned = false;
        this->stunned_timer.Disable();
    }

    if (p_depop)
    {
        Mob* owner = entity_list.GetMob(this->ownerid);
        if (owner != 0)
        {
        	if(GetBodyType() != BT_SwarmPet)
	            owner->SetPetID(0);
			this->ownerid = 0;
            this->petid = 0;
        }
        return false;
    }

    adverrorinfo = 2;
    
    SpellProcess();
    
    if (tic_timer.Check()) {
        TicProcess();
	    #ifdef IPC
        if(IsInteractive() )
		    SendPosUpdate();
        #endif
        int32 bonus = 0;
		#ifdef IPC
        if(IsInteractive() && CurrentPosition() == 1)
		    bonus+=3;
        #else
        if(CurrentPosition() == 1)
           bonus+=3;
        #endif
        
        if(GetHP() < GetMaxHP()) {
			if(GetOwnerID()!=0 && !IsEngaged()) //pet
				SetHP(GetHP()+hp_regen+bonus+(GetLevel()/5));
			else
				SetHP(GetHP()+hp_regen+bonus);
		}
		if(GetMana() < GetMaxMana()) {
			SetMana(GetMana()+mana_regen+bonus);
		}
    }
		  
    if (IsStunned()||IsMezzed())
	    return true;

	//Feign Death Memory
	if (forget_timer.Check() && strstr(GetFeignMemory(),"0") == NULL) {
		Client* remember_client = entity_list.GetClientByName(GetFeignMemory());
		if (remember_client != 0)
		{
			if (!remember_client->CastToClient()->GetFeigned())
			{
				AddToHateList(remember_client,1);
				SetFeignMemory("0");
				forgetchance = 0;
			}
			else if (rand()%100 <= forgetchance)
			{
				SetFeignMemory("0");
				forgetchance = 0;
			}
			else
			{
				forgetchance += 1;
			}
		}
		else
		{
			SetFeignMemory("0");
		}
	}

	if (sendhpupdate_timer.Check() && IsTargeted()) {
		if(!IsFullHP || cur_hp<max_hp){
			SendHPUpdate();
		}
	}
	
	//Handle assists...
	Mob *hated = NULL;
	if(assist_timer.Check() && (hated = hate_list.GetTop()) != NULL) {
		entity_list.AIYellForHelp(this, hated);
	}
	
	adverrorinfo = 3;
	AI_Process();
	adverrorinfo = 0;
    return true;

/*

    adverrorinfo = 3;

    // neotokyo: moved the call for selfbuffing after mez and stun
    if( !IsEngaged() )
	    this->CheckSelfBuffs();

    if (scanarea_timer.Check() &&(!zone->AggroLimitReached()))
    {
		if(entity_list.AddHateToCloseMobs(this))
            zone->AddAggroMob();
        entity_list.CheckSupportCloseMobs(this);
    }

    // neotokyo: check frenzy
    this->hate_list.CheckFrenzyHate();

    adverrorinfo = 4;
	if(IsEngaged())
	    SetTarget(hate_list.GetTop());
    else
    {
        target = 0;
    }
		  
    if (gohome_timer.Check())
    {	
	    gohome_timer.Disable();
        if (!IsEngaged())
	        ismovinghome = true;
    }

    adverrorinfo = 5;	
    if (IsEngaged())
    {
        if(banishcapability != 101 && (GetHateTop()->GetLevel() >= banishcapability) && (banishcapability != 0) && GetHateTop()->IsClient())
        {
            GetHateTop()->Message(4,"I shall not fight you, but I shall banish you!");
            GetHateTop()->GoToBind();
            this->RemoveFromHateList(GetHateTop());
            }
        }
        adverrorinfo = 15;
        if(rooted && IsEngaged())
        {
            int disttest = (int)(this->GetSize() * 2);
            if(disttest <= 9)
            {
                disttest = 10;
            }
            if(this->GetRace() == 49)
            {
                disttest = 85;
            }

			  if (DistNoRootNoZ(GetHateTop()) > disttest * 40)
			  {
				  Mob* closehate = hate_list.GetClosest(this->CastToMob());
				  if(closehate != 0 && closehate != target)
					  SetTarget(closehate);
				  if(closehate != 0 && closehate == target)
				  {
					  SetTarget(closehate);
					  evader = true;
				  }
			  }
		  }
		  
		  if(evader == true && !rooted && !IsEngaged())
			  evader = false;
		  
		  adverrorinfo = 6;
		  if((spells_timer.Check() || (IsEngaged() && spells_timer.GetRemainingTime() > 14000)) && !(gohome_timer.Enabled()||ismovinghome))
		  {
			  if(!IsEngaged()) {
				  spells_timer.Start(RandomTimer(100000,240000), true);
				  CheckFriendlySpellStatus();
			  }
			  if(IsEngaged()) {
				  if(target->GetHPRatio() <= 25 || evader == true)
					  spells_timer.Start(RandomTimer(5000,10000), true);
				  else
					  spells_timer.Start(RandomTimer(7000,14000), true);
				  
				  if(GetHPRatio() < 60 && RandomTimer(0,7) == 0)
					  CheckFriendlySpellStatus();
				  else
					  CheckEnemySpellStatus();
			  }
		  }
		  
		  adverrorinfo = 7;	
		  if (target == 0 && this->ownerid != 0) {
			  Mob* obc = entity_list.GetMob(this->ownerid);
			  if(obc == 0)
				  return false;
			  
			  SetTarget(obc);
		  }
		  adverrorinfo = 14;
		  if (casting_spell_id == 0) {
#define NPC_MOVEMENT_PER_TIC		10
			  //		bool pvp_protection=0;
			  if (target != 0)
			  {
				  if (GetOwnerID() != target->GetID() && !IsAttackAllowed(target))
				  {
					  if(IsInteractive())
					  {
						  RemoveFromHateList(target);
						  return true;
					  }
					  
					  char temp[200];
					  snprintf(temp, 200, "%s says, 'That is not a legal target master.'", this->GetName());
					  entity_list.MessageClose(this, 1, 200, 10, temp);
					  RemoveFromHateList(target);
					  return true;
				  }
				  
				  if(this->IsEngaged() && GetOwner() != 0 && GetOwner()->IsNPC() && GetOwnerID() != target->GetID())
				  {
					  if(GetOwner()->CastToNPC()->hate_list.GetEntHate(GetHateTop()) == 0)
						  GetOwner()->CastToNPC()->AddToHateList(GetHateTop(),1);
				  }
				  
				  
				  if (movement_timer.Check() && !rooted) 
				  {
					  adverrorinfo = 8;
					  //			movement_timer.Start();
					  // NPC can move per "think", this number should be an EQ distance squared				
					  float total_move_dist = (float) DistNoRootNoZ(target);
					  appearance = 0;
					  if (total_move_dist > 75) 
					  {
						  appearance = 5; 
						  total_move_dist -= 50;
						  if (total_move_dist > NPC_MOVEMENT_PER_TIC)
							  total_move_dist = NPC_MOVEMENT_PER_TIC;
						  float x2 = (float) pow(target->GetX() - x_pos, 2);
						  float y2 = (float) pow(target->GetY() - y_pos, 2);
						  // divide by zero "should" be impossible here because of the DistNoRootNoZ check
						  float x_move = (float) (total_move_dist * ((double)x2/(x2+y2))); // should be already abs()'d from the square
						  float y_move = (float) (total_move_dist - x_move);
						  x_pos += (float) sqrt((double)x_move) * sign(target->GetX() - x_pos);
						  y_pos += (float) sqrt((double)y_move) * sign(target->GetY() - y_pos);
						  // since we don't use maps, we don't know the correct z nut this should do for now
						  
                          // lets try this and see how it works for pets
                          // neotokyo: 14. Dec. 2002
                          z_pos = (z_pos + target->GetZ()) / 2;
                          
                          //if (this->IsEngaged())
							//  z_pos = this->GetHateTop()->GetZ();
						  pLastChange = Timer::GetCurrentTime();
					  }
					  FaceTarget();
					  
				  }
				  if (target->GetID() != this->ownerid && attack_timer.Check() && this->GetHPRatio() > 0) 
				  {
					  adverrorinfo = 9;
					  if(GetHPRatio() >= 51) {
						  if(SpecialNPCAttacks[1] == 2)
						  {
							  SpecialNPCAttacks[1] = 1;
							  SpecialNPCCounts[1] = 0;
						  }
						  if(SpecialNPCAttacks[2] == 2)
						  {
							  SpecialNPCAttacks[2] = 1;
							  SpecialNPCCounts[2] = 0;
						  }
						  if(SpecialNPCAttacks[3] == 2)
						  {
							  SpecialNPCAttacks[3] = 1;
							  SpecialNPCCounts[3] = 0;
							  SpecialNPCCountstwo[3] = 0;
						  }
					  }
					  
					  if(GetHPRatio() <= 49  && GetHPRatio() > 0 && target->GetID() != this->GetID()) {
						  if(SpecialNPCAttacks[1] == 1)
						  {
							  SpecialNPCCounts[1] += 1;
							  if(SpecialNPCCounts[1] >= 2)
							  {
								  HateSummon();
								  SpecialNPCCounts[1] = 0;
							  }
						  }
					  }
					  
					  if(GetHPRatio() <= 35 && GetHPRatio() > 0 && target->GetID() != this->GetID()) {
						  if(SpecialNPCAttacks[3] == 1)
						  {
							  if(SpecialNPCCountstwo[3] == 0)
							  {
								  entity_list.MessageClose(this,true,800,13,"%s Rampages!",this->GetName());
							  }
							  SpecialNPCCounts[3] = 1;
							  if(SpecialNPCCounts[3] == 1)
							  {
								  Attack(hate_list.GetRandom());
								  SpecialNPCCountstwo[3] += 1;
							  }
						  }
						  if(SpecialNPCCountstwo[3] >= 15)
						  {
							  entity_list.MessageClose(this,true,800,13,"%s loses the rampage.",this->GetName());
							  SpecialNPCAttacks[3] = 2;
							  SpecialNPCCountstwo[3] = 0;
						  }
					  }
					  
					  if(GetHPRatio() <= 17 && GetHPRatio() > 0 && target->GetID() != this->GetID()) {
						  if(SpecialNPCAttacks[2] == 1)
						  {
							  if(SpecialNPCCounts[2] == 0)
							  {
								  SpecialNPCAttacks[2] = 1;
								  entity_list.MessageClose(this,true,800,13,"%s is filled with enragement.",this->GetName());
							  }
							  SpecialNPCCounts[2] += 1;
							  if(SpecialNPCCounts[2] >= 24)
							  {
								  entity_list.MessageClose(this,true,800,13,"%s enragement subsides.",this->GetName());
								  SpecialNPCAttacks[2] = 2;
							  }
						  }
					  }
					  
					  adverrorinfo = 10;
					  if(target != 0 && target->GetID() != this->GetOwnerID())
					  {
						  Attack(target);
						  if(evader == true)
							  evader = false;
						  
						  if(IsInteractive())
						  {
							  tired = 0;
						  }
						  ishome = false;
					  }
					  pLastChange = Timer::GetCurrentTime();
				  }
		}
		else if (ismovinghome)//Go back to bindpoint.. wonder why SendTo don't work for this? - Merkur
		{
			adverrorinfo = 11;
			if (movement_timer.Check()) 
			{
				if (reallygohome) {
					//	cout << "Really Go Home Code exec" << endl;
					float total_move_dist = (float) (org_x-x_pos)*(org_x-x_pos)+(org_y-y_pos)*(org_y-y_pos);
					appearance = 0;
					if (total_move_dist > 75)  {
						appearance = 5;
						total_move_dist -= 50;
						if (total_move_dist > NPC_MOVEMENT_PER_TIC/2) // go a bit slower
							total_move_dist = NPC_MOVEMENT_PER_TIC/2;
						float x2 = (float) pow(org_x - x_pos, 2);
						float y2 = (float) pow(org_y - y_pos, 2);
						float x_move = (float) (total_move_dist * ((double)x2/(x2+y2))); // should be already abs()'d from the square
						float y_move = (float) (total_move_dist - x_move);
						x_pos += (float) sqrt((double)x_move) * sign(org_x - x_pos);
						y_pos += (float) sqrt((double)y_move) * sign(org_y - y_pos);
						z_pos = org_z;
						float angle;
						if (org_x-x_pos > 0)
							angle = - 90 + atan((double)(org_y-y_pos) / (double)(org_x-x_pos)) * 180 / M_PI;
						else {
							if (org_x-x_pos < 0)	
								angle = + 90 + atan((double)(org_y-y_pos) / (double)(org_x-x_pos)) * 180 / M_PI;
							else { // Added?
								if (org_y-y_pos > 0)
									angle = 0;
								else
									angle = 180;
							}
						}
						if (angle < 0)
							angle += 360;
						if (angle > 360	)
							angle -= 360;
						
						heading	= 256*(360-angle)/360.0f;
						pLastChange = Timer::GetCurrentTime();
					} // if total_move_distance
					else {
						appearance = 0;			
						ismovinghome = false;
						ishome = true;
						pLastChange = Timer::GetCurrentTime();
					}
				} // if !reallygohome
				else {
					//				cout << "Not Really Going Home Code exec" << endl;
					float delta_x = (rand()%100) - 50;
					float delta_y = (rand()%100) - 50;
					SendTo(org_x + delta_x, org_y + delta_y);
				}
			} // if movemement_timer
			
		} // if !ismovinghome
		else { 
						float delta_x = (rand()%100) - 50;
		float delta_y = (rand()%100) - 50;
		float total_move_dist = (float) (delta_x-x_pos)*(delta_x-x_pos)+(delta_y-y_pos)*(delta_y-y_pos);
		appearance = 0;
		if (total_move_dist > 10)  {
		appearance = 5;
		total_move_dist -= 50;
		if (total_move_dist > NPC_MOVEMENT_PER_TIC/2) // go a bit slower
		total_move_dist = NPC_MOVEMENT_PER_TIC/2;
		float x2 = (float) pow(delta_x - x_pos, 2);
		float y2 = (float) pow(delta_y - y_pos, 2);
		float x_move = (float) (total_move_dist * ((double)x2/(x2+y2))); // should be already abs()'d from the square
		float y_move = (float) (total_move_dist - x_move);
		x_pos += (float) sqrt((double)x_move) * sign(delta_x - x_pos);
		y_pos += (float) sqrt((double)y_move) * sign(delta_y - y_pos);
		z_pos = z_pos;
		float angle;
		if (delta_x-x_pos > 0)
		angle = - 90 + atan((double)(delta_y-y_pos) / (double)(delta_x-x_pos)) * 180 / M_PI;
		else {
		if (delta_x-x_pos < 0)	
		angle = + 90 + atan((double)(delta_y-y_pos) / (double)(delta_x-x_pos)) * 180 / M_PI;
		else { // Added?
		if (delta_y-y_pos > 0)
								angle = 0;
								else
								angle = 180;
								}
								}
								if (angle < 0)
								angle += 360;
								if (angle > 360	)
								angle -= 360;
								
								  heading	= 256*(360-angle)/360.0f;
								  pLastChange = Timer::GetCurrentTime();
								  } // if total_move_distance
								  //			if (walking_timer && walking_timer.Check()) {
								  //				float delta_x = (rand()%100) - 50;
								  //				float delta_y = (rand()%100) - 50;
								  //				SendTo(org_x + delta_x, org_y + delta_y);
			//			}
		}
	}*/
	adverrorinfo = 0;
    return true;
}

int32 NPC::CountLoot() {
	if (itemlist == 0)
		return 0;
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	int32 count = 0;
	
	iterator.Reset();
	while(iterator.MoreElements())	
	{
		count++;
		iterator.Advance();
	}
	return count;
}

void NPC::DumpLoot(int32 npcdump_index, ZSDump_NPC_Loot* npclootdump, int32* NPCLootindex) {
	if (itemlist == 0)
		return;
	LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
	//int32 count = 0;
	
	iterator.Reset();
	while(iterator.MoreElements())	
	{
		npclootdump[*NPCLootindex].npc_dump_index = npcdump_index;
		npclootdump[*NPCLootindex].itemid = iterator.GetData()->item_id;
		npclootdump[*NPCLootindex].charges = iterator.GetData()->charges;
		npclootdump[*NPCLootindex].equipSlot = iterator.GetData()->equipSlot;
		(*NPCLootindex)++;
		iterator.RemoveCurrent();
	}
}

void NPC::Depop(bool StartSpawnTimer) {
	p_depop = true;
	if (StartSpawnTimer) {
		if (respawn2 != 0) {
			respawn2->Reset();
		}
	}
}

bool NPC::DatabaseCastAccepted(int spell_id) {
	for (int i=0; i < 12; i++) {
		switch(spells[spell_id].effectid[i]) {
		case SE_Stamina: {
			if(IsEngaged() && GetHPRatio() < 100)
				return true;
			else
				return false;
			break;
						 }
		case SE_CurrentHPOnce:
		case SE_CurrentHP: {
			if(this->GetHPRatio() < 100 && spells[spell_id].buffduration == 0)
				return true;
			else
				return false;
			break;
						   }
			
		case SE_HealOverTime: {
			if(this->GetHPRatio() < 100)
				return true;
			else
				return false;
			break;
							  }
		case SE_DamageShield: {
			return true;
							  }
		case SE_NecPet:
		case SE_SummonPet: {
			if(GetPetID() != 0){
#ifdef SPELLQUEUE
				printf("%s: Attempted to make a second pet, denied.\n",GetName());
#endif
				return false;
			}
			break;
						   }
		case ST_Corpse: {
			return false; //Pfft, npcs don't need to summon corpses/locate corpses!
			break;
						}
		default:
			if(spells[spell_id].goodEffect == 1 && !(spells[spell_id].buffduration == 0 && this->GetHPRatio() == 100) && !IsEngaged())
				return true;
			return false;
		}
	}
	return false;
}

NPC* NPC::SpawnNPC(const char* spawncommand, float in_x, float in_y, float in_z, float in_heading, Client* client) {
	if(spawncommand == 0 || spawncommand[0] == 0) {
		return 0;
	}
	else {
		Seperator sep(spawncommand);
		//Lets see if someone didn't fill out the whole #spawn function properly 
		sep.arg[0][63] = 0;
		if (!sep.IsNumber(1))
			sprintf(sep.arg[1],"1"); 
		if (!sep.IsNumber(2))
			sprintf(sep.arg[2],"1"); 
		if (!sep.IsNumber(3))
			sprintf(sep.arg[3],"0");
		if (atoi(sep.arg[4]) > 2100000000 || atoi(sep.arg[4]) <= 0)
			sprintf(sep.arg[4]," ");
		if (!strcmp(sep.arg[5],"-"))
			sprintf(sep.arg[5]," "); 
		if (!sep.IsNumber(5))
			sprintf(sep.arg[5]," "); 
		if (!sep.IsNumber(6))
			sprintf(sep.arg[6],"1");
		if (!sep.IsNumber(8))
			sprintf(sep.arg[8],"0");
		if (!sep.IsNumber(9))
			sprintf(sep.arg[9], "0");
		if (!sep.IsNumber(7))
			sprintf(sep.arg[7],"0");
		if (!strcmp(sep.arg[4],"-"))
			sprintf(sep.arg[4]," "); 
		if (!sep.IsNumber(10))	// solar: bodytype
			sprintf(sep.arg[10], "0");
		//Calc MaxHP if client neglected to enter it...
		if (!sep.IsNumber(4)) {
			//Stolen from Client::GetMaxHP...
			int8 multiplier = 0;
			int tmplevel = atoi(sep.arg[2]);
			switch(atoi(sep.arg[5]))
			{
			case WARRIOR:
				if (tmplevel < 20)
					multiplier = 22;
				else if (tmplevel < 30)
					multiplier = 23;
				else if (tmplevel < 40)
					multiplier = 25;
				else if (tmplevel < 53)
					multiplier = 27;
				else if (tmplevel < 57)
					multiplier = 28;
				else 
					multiplier = 30;
				break;
				
			case DRUID:
			case CLERIC:
			case SHAMAN:
				multiplier = 15;
				break;
				
			case PALADIN:
			case SHADOWKNIGHT:
				if (tmplevel < 35)
					multiplier = 21;
				else if (tmplevel < 45)
					multiplier = 22;
				else if (tmplevel < 51)
					multiplier = 23;
				else if (tmplevel < 56)
					multiplier = 24;
				else if (tmplevel < 60)
					multiplier = 25;
				else
					multiplier = 26;
				break;
				
			case MONK:
			case BARD:
			case ROGUE:
				//		case BEASTLORD:
				if (tmplevel < 51)
					multiplier = 18;
				else if (tmplevel < 58)
					multiplier = 19;
				else
					multiplier = 20;				
				break;
				
			case RANGER:
				if (tmplevel < 58)
					multiplier = 20;
				else
					multiplier = 21;			
				break;
				
			case MAGICIAN:
			case WIZARD:
			case NECROMANCER:
			case ENCHANTER:
				multiplier = 12;
				break;
				
			default:
				if (tmplevel < 35)
					multiplier = 21;
				else if (tmplevel < 45)
					multiplier = 22;
				else if (tmplevel < 51)
					multiplier = 23;
				else if (tmplevel < 56)
					multiplier = 24;
				else if (tmplevel < 60)
					multiplier = 25;
				else
					multiplier = 26;
				break;
			}
			sprintf(sep.arg[4],"%i",5+multiplier*atoi(sep.arg[2])+multiplier*atoi(sep.arg[2])*75/300);
		}
		
		// Autoselect NPC Gender... (Scruffy)
		if (sep.arg[5][0] == 0) {
			sprintf(sep.arg[5], "%i", (int) Mob::GetDefaultGender(atoi(sep.arg[1])));
		}
		
		if (client) {
			// Well we want everyone to know what they spawned, right? 
			client->Message(0, "New spawn:");
			client->Message(0, "Name: %s",sep.arg[0]);
			client->Message(0, "Race: %s",sep.arg[1]);
			client->Message(0, "Level: %s",sep.arg[2]);
			client->Message(0, "Material: %s",sep.arg[3]);
			client->Message(0, "Current/Max HP: %s",sep.arg[4]);
			client->Message(0, "Gender: %s",sep.arg[5]);
			client->Message(0, "Class: %s",sep.arg[6]);
			
			client->Message(0, "Weapon Item Number: %s/%s",sep.arg[7],sep.arg[8]);
			client->Message(0, "MerchantID: %s",sep.arg[9]);
			client->Message(0, "Bodytype: %s",sep.arg[10]);
		}
		//Time to create the NPC!! 
		NPCType* npc_type = new NPCType;
		memset(npc_type, 0, sizeof(NPCType));
		strcpy(npc_type->name,sep.arg[0]);
		npc_type->cur_hp = atoi(sep.arg[4]); 
		npc_type->max_hp = atoi(sep.arg[4]); 
		npc_type->race = atoi(sep.arg[1]);
		npc_type->gender = atoi(sep.arg[5]); 
		npc_type->class_ = atoi(sep.arg[6]); 
		npc_type->deity= 1;
		npc_type->level = atoi(sep.arg[2]);
		npc_type->npc_id = 0;
		npc_type->loottable_id = 0;
		npc_type->texture = atoi(sep.arg[3]);
		npc_type->light = 0;
		npc_type->fixedZ = 1;
		npc_type->walkspeed = 0.67;
		npc_type->runspeed = 1.25;
		// Weapons are broke!!
		npc_type->equipment[7] = atoi(sep.arg[7]);
		npc_type->equipment[8] = atoi(sep.arg[8]);
		npc_type->merchanttype = atoi(sep.arg[9]);	
		npc_type->bodytype = atoi(sep.arg[10]);
		//for (int i=0; i<9; i++)
		//	npc_type->equipment[i] = atoi(sep.arg[7]);
		
		npc_type->STR = 150;
		npc_type->STA = 150;
		npc_type->DEX = 150;
		npc_type->AGI = 150;
		npc_type->INT = 150;
		npc_type->WIS = 150;
		npc_type->CHA = 150;
		
		NPC* npc = new NPC(npc_type, 0, in_x, in_y, in_z, in_heading/8);
		safe_delete(npc_type);
		
		entity_list.AddNPC(npc);
		return npc;
	}
}



/*Interactive NPC Stuff*/
#ifdef IPC
void NPC::InteractiveChat(int8 chan_num, int8 language, const char * message, const char* targetname,Mob* sender)
{
	char* tmp = new char[strlen(message)+1];
	strcpy(tmp,message);
	strupr(tmp);

	if(sender == 0 || sender == this)
		return;
    LogFile->write(EQEMuLog::Debug,
        "InteractiveChat(chan_num:%i, language:%i, message:\"%s\", targetname:\"%s\",sender:%p:%s)",
                chan_num, language, message, targetname, sender, sender->GetName());
	
	
	switch(chan_num)
	{
	case 2:
		{
			Group* group = entity_list.GetGroupByMob(this);
			if(group == 0)
				return;
			char* _GetName = new char[strlen(GetName())+1];
			strcpy(_GetName, GetName());
			if ( !strstr(tmp, strupr(_GetName)) )
				return;
			
			if(strstr(tmp,"HELP ME ATTACK"))
			{
				SetTarget(sender->GetTarget());
				if(target != 0)
				{
					AddToHateList(target,1);
					group->GroupMessage(this,"Im helping ya!");
				}
			}
			else if( strstr(tmp,"HEAL ME") )
			{
			   if (IsCasting())
			         break;
               if(AICastSpell(sender, 100, 2))
					group->GroupMessage(this,"Healing, ya.");
			   else
					group->GroupMessage(this,"lom, medding or something.");
			}
			else if( strstr(tmp,"PVP") )
			{
				this->CastToClient()->SetPVP(true);
			}
			else if( strstr(tmp, "FOLLOW ME") ) {
				SetFollowID(sender->GetID());
				group->GroupMessage(this, "Right behind ya.");
			}
			else if( strstr(tmp, "WAIT HERE") ){
				SaveGuardSpot(true);
				SetFollowID(0);
				group->GroupMessage(this, "Waiting.");
			}
			else if( strstr(tmp, "EVAC") ){
			   cout<<"IPC evac requested by "<<sender->GetName()<<endl;
			   if (IsCasting())
			         break;
			    if(AICastSpell(sender, 100, 16))
			           group->GroupMessage(this,"Evacing!");
                else {
                       group->GroupMessage(this,"Sissy!");
                       entity_list.ChannelMessage(this,1,0,"To the death then!");                       
                }
                       
			}
			else if( strstr(tmp, "BUFF ") ){
			   if (IsCasting())
			         break;
			   if ( strstr(tmp, "ME") && AICastSpell(sender, 100, 8) ) {
			         group->GroupMessage(this, "Buffs inc, stay in range.");
               }
               else if ( strstr(tmp, "TARGET") && sender->GetTarget() && AICastSpell(sender->GetTarget(), 100, 8) ) {
			         group->GroupMessage(this, "Buffs inc, stay in range.");
               }
			}
			else {
				group->GroupMessage(this, "Try: help me attack, heal me, pvp, follow me, wait here, evac, buff me, buff target");
			}
			break;
		}
	case 8:
		{
			if(strstr(tmp,"HAIL") && sender->GetTarget() == this) {
				entity_list.ChannelMessage(this,1,0,"Hail, %s", sender->GetName());
			}
			else if (strstr(tmp,"GROUP ME") && sender->GetTarget() == this) {
			      Group* group = entity_list.GetGroupByMob(this->CastToMob());
			      if (group && group->members[0] != NULL && group->members[0] != this) {
                           //group->AddMember(sender);
                           entity_list.ChannelMessage(this,1,0,"%s, has lead", group->members[0]->GetName());
                  }
                  else if (group && group->members[0] != NULL && group->members[0] == this) {
                           group->AddMember(sender);
                  }
                  else if (group && group->members[0] == NULL) {
			               entity_list.ChannelMessage(this,1,0,"I'm group bugged, try again");
			               group->DisbandGroup();
                  }
			      else if (!group) {
			               entity_list.ChannelMessage(this,1,0,"I'm not grouped. Maybe you should Invite..");
                  }
                  else {
                           LogFile->write(EQEMuLog::Error,
                           "InteractiveChat(chan_num:%i, language:%i, message:\"%s\", targetname:\"%s\",sender:%p:%s)",
                           chan_num, language, message, targetname, sender, sender->GetName());
                  }
			}
			
			break;
		}
	}
	
	delete tmp;
}

void NPC::TakenAction(int8 action,Mob* actiontaker)
{
    LogFile->write(EQEMuLog::Debug, "TakenAction(%i, %s)", action, (actiontaker) ? actiontaker->GetName():"none");
	switch(action)
	{
	case 0:
		{
			tired = 0;
			position = 0;
			SendAppearancePacket(AT_Anim, ANIM_STAND);
			break;
		}
	case 1:
		{
			position = 1;
			if(GetHPRatio() <= 80 && actiontaker != this && !AICastSpell(this, 100, 2)) {
				entity_list.ChannelMessage(this,1,0,"I need to get some HP back, im going to sit.");
			
			//if(actiontaker != this && GetHPRatio() > 80)
			//	entity_list.ChannelMessage(this,1,0,"Im tired...");
			SendAppearancePacket(AT_Anim, ANIM_SIT);
			}
			break;
		}
	case 2:
		{
			position = 2;
			SendAppearancePacket(AT_Anim, ANIM_CROUCH);
			break;
		}
	case 3:
		{
			position = 3;
			SendAppearancePacket(AT_Anim, ANIM_STAND);
			break;
		}
	case 4:
		{
			position = 4;
			SendAppearancePacket(AT_Anim, ANIM_LOOT);
			break;
		}
	case 20: // Inspected
		{
			if(CurrentPosition() == 1)
				FaceTarget(actiontaker);
			break;
		}
	case 21: // Healed
		{
			if(IsEngaged())
				return;
			
			if(CurrentPosition() != 1)
			{
				TakenAction(0,0);
			}
#if 0
			if (actiontaker != this) {
			    FaceTarget(actiontaker);
			    entity_list.ChannelMessage(this,1,0,"Thanks for the heal!");
			}
#endif
			if(GetHPRatio() < 100)
				TakenAction(1,this);
			break;
		}
	case 22://groupinvite
		{
			Group* group = entity_list.GetGroupByMob(this->CastToMob());
			if(group != 0 && actiontaker != 0)
			{
				isgrouped = true;
				group->GroupMessage(this,"Hey hey! Thanks for the invite :D");
				ownerid = actiontaker->GetID();
				SetTarget(actiontaker);
			}
			else {
			   entity_list.ChannelMessage(this,1,0,"Sorry I am already grouped");
			}
			break;
		}
	case 23://groupdisband
		{
			Group* group = entity_list.GetGroupByMob(this);
			if(group == 0) {
                 LogFile->write(EQEMuLog::Error, "IPC: group disband called but IPC not grouped");
			}
			else if (!actiontaker || actiontaker == this) {
				target = 0;
				isgrouped = false;
				ownerid = 0;
				SaveGuardSpot(true);
				SetFollowID(0);
				LogFile->write(EQEMuLog::Debug, "IPC: leaving group");
			}
			else {
			   LogFile->write(EQEMuLog::Debug, "IPC: player leaving group");
			   entity_list.ChannelMessage(this,1,0,"Safe travels.");			   
			}
            if (GetFollowID()) {
                SaveGuardSpot(true);
			    SetFollowID(0);
            }
			break;
		}
	}
}
#endif
int32 Database::NPCSpawnDB(int8 command, const char* zone, NPC* spawn, int32 extra) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
    MYSQL_ROW row;
	int32 tmp = 0;
	int32 tmp2 = 0;
    int32 last_insert_id = 0;
	switch (command) {
		case 0: { // add spawn with new npc_type - khuong
			int32 npc_type_id, spawngroupid;
			char tmpstr[64];
			//char tmpstr2[64];
			EntityList::RemoveNumbers(strn0cpy(tmpstr, spawn->GetName(), sizeof(tmpstr)));
			if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO npc_types (name, level, race, class, hp, gender, texture, helmtexture, size, loottable_id, merchant_id, face, walkspeed, runspeed) values(\"%s\",%i,%i,%i,%i,%i,%i,%i,%f,%i,%i,%i,%f,%f)", tmpstr, spawn->GetLevel(), spawn->GetRace(), spawn->GetClass(), spawn->GetMaxHP(), spawn->GetGender(), spawn->GetTexture(), spawn->GetHelmTexture(), spawn->GetSize(), spawn->GetLoottableID(), spawn->MerchantType, 0, spawn->GetWalkspeed(), spawn->GetRunspeed()), errbuf, 0, 0, &npc_type_id)) {
				safe_delete(query);
				return false;
			}
			snprintf(tmpstr, sizeof(tmpstr), "%s-%s", zone, spawn->GetName());
			if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO spawngroup (id, name) values(%i, '%s')", tmp, tmpstr), errbuf, 0, 0, &spawngroupid)) {
				safe_delete(query);
				return false;
			}
			if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO spawn2 (zone, x, y, z, respawntime, heading, spawngroupID) values('%s', %f, %f, %f, %i, %f, %i)", zone, spawn->GetX(), spawn->GetY(), spawn->GetZ(), 1200, spawn->GetHeading(), spawngroupid), errbuf, 0, 0, &tmp)) {
				safe_delete(query);
				return false;
			}
			if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO spawnentry (spawngroupID, npcID, chance) values(%i, %i, %i)", spawngroupid, npc_type_id, 100), errbuf, 0)) {
				safe_delete(query);
				return false;
			}
			safe_delete_array(query);
			return true;
			break;
		}
		case 1:{
			tmp2 = spawn->GetNPCTypeID();
			char tmpstr[64];
			//char tmpstr2[64];
			snprintf(tmpstr, sizeof(tmpstr), "%s%s%i", zone, spawn->GetName(),Timer::GetCurrentTime());
			if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO spawngroup (name) values('%s')", tmpstr), errbuf, 0, 0, &last_insert_id)) {
				printf("ReturnFalse: spawngroup query in NPCSpawnDB() (query: %s)\n",query);
				safe_delete(query);
				return false;
			}
			int32 respawntime = 0;
			int32 spawnid = 0;
			if (extra)
				respawntime = extra;
			else if(spawn->respawn2 && spawn->respawn2->RespawnTimer() != 0)
				respawntime = spawn->respawn2->RespawnTimer();
			else
				respawntime = 1200;
			if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO spawn2 (zone, x, y, z, respawntime, heading, spawngroupID) values('%s', %f, %f, %f, %i, %f, %i)", zone, spawn->GetX(), spawn->GetY(), spawn->GetZ(), respawntime, spawn->GetHeading(), last_insert_id), errbuf, 0, 0, &spawnid)) {
				safe_delete(query);
				printf("ReturnFalse: spawn2 query in NPCSpawnDB()\n");
				return false;
			}
			if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO spawnentry (spawngroupID, npcID, chance) values(%i, %i, %i)", last_insert_id, tmp2, 100), errbuf, 0)) {
				safe_delete(query);
				printf("ReturnFalse: spawnentry query in NPCSpawnDB()\n");
				return false;
			}

#ifdef GUILDWARS
			extern Zone* zone;
#ifdef GWDEBUG
			printf("Crash\n");
#endif
			SpawnGroup* newSpawnGroup = new SpawnGroup( last_insert_id, tmpstr);
			zone->spawn_group_list->AddSpawnGroup(newSpawnGroup);
#ifdef GWDEBUG
			printf("Crash2\n");
#endif
			SpawnEntry* newSpawnEntry = new SpawnEntry( spawn->GetNPCTypeID(), 100);
			if (zone->spawn_group_list->GetSpawnGroup(last_insert_id))
				zone->spawn_group_list->GetSpawnGroup(last_insert_id)->AddSpawnEntry(newSpawnEntry);
#ifdef GWDEBUG
			printf("Crash3\n");
#endif
			Spawn2* newSpawn = new Spawn2(spawnid, last_insert_id, spawn->GetX(), spawn->GetY(), spawn->GetZ(), spawn->GetHeading(), respawntime, 0);
			zone->spawn2_list.Insert( newSpawn );
#ifdef GWDEBUG
			printf("Made it!\n");
#endif
#endif
			safe_delete_array(query);
			return spawnid;
			break;
		}
		case 2: { // update npc_type from target spawn - khuong
			if (RunQuery(query, MakeAnyLenString(&query, "UPDATE npc_types SET name=\"%s\", level=%i, race=%i, class=%i, hp=%i, gender=%i, texture=%i, helmtexture=%i, size=%i, loottable_id=%i, merchant_id=%i, face=%i, WHERE id=%i", spawn->GetName(), spawn->GetLevel(), spawn->GetRace(), spawn->GetClass(), spawn->GetMaxHP(), spawn->GetGender(), spawn->GetTexture(), spawn->GetHelmTexture(), spawn->GetSize(), spawn->GetLoottableID(), spawn->MerchantType, spawn->GetNPCTypeID()), errbuf, 0)) {
				safe_delete_array(query);
				return true;
			}
			else {
				safe_delete_array(query);
				return false;
			}
			break;
		}
		case 3: { // delete spawn from spawning - khuong
			if (RunQuery(query, MakeAnyLenString(&query, "SELECT id,spawngroupID from spawn2 where zone='%s' AND x='%f' AND y='%f' AND heading='%f'", zone, spawn->GetSpawnX(),spawn->GetSpawnY(),spawn->GetSpawnHeading()), errbuf, &result)) {
			row = mysql_fetch_row(result);
			if (row[0]) tmp = atoi(row[0]);
			if (row[1]) tmp2 = atoi(row[1]);
			query = 0;
			mysql_free_result(result);
			}
			else { return 0; }
			if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM spawn2 WHERE id='%i'", tmp), errbuf,0)) {
				safe_delete(query);
				return false;
			}
			if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM spawngroup WHERE id='%i'", tmp2), errbuf,0)) {
				safe_delete(query);
				return false;
			}
			if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM spawnentry WHERE spawngroupID='%i'", tmp2), errbuf,0)) {
				safe_delete(query);
				return false;
			}
			safe_delete_array(query);
			return true;


			break;
		}
		case 4: { //delete spawn from DB (including npc_type) - khuong
			if (RunQuery(query, MakeAnyLenString(&query, "SELECT id,spawngroupID from spawn2 where zone='%s' AND x='%f' AND y='%f' AND heading='%f'", zone, spawn->GetX(), spawn->GetY(), spawn->GetHeading()), errbuf, &result)) {
			row = mysql_fetch_row(result);
			if (row[0]) tmp = atoi(row[0]);
			if (row[1]) tmp2 = atoi(row[1]);
			query = 0;
			mysql_free_result(result);
			}
			else { return 0; }
			if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM spawn2 WHERE id='%i'", tmp), errbuf,0)) {
				safe_delete(query);
				return false;
			}
			if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM spawngroup WHERE id='%i'", tmp2), errbuf,0)) {
				safe_delete(query);
				return false;
			}
			if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM spawnentry WHERE spawngroupID='%i'", tmp2), errbuf,0)) {
				safe_delete(query);
				return false;
			}
			if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM npc_types WHERE id='%i'", spawn->GetNPCTypeID()), errbuf,0)) {
				safe_delete(query);
				return false;
			}
			safe_delete_array(query);
			return true;
			break;
		}
		safe_delete_array(query);
		return false;
	}
	return false;
}

sint32 NPC::GetEquipmentMaterial(int8 material_slot)
{
	const Item_Struct *item;

	switch(material_slot)
	{
		case MATERIAL_HEAD:
			return helmtexture;
		case MATERIAL_CHEST:
			return texture;
		case MATERIAL_PRIMARY:
			return d_meele_texture1;
		case MATERIAL_SECONDARY:
			return d_meele_texture2;
		default:
			item = database.GetItem(GetEquipment(material_slot));
			if(item != 0)
			{
				return item->Common.Material;
			}
	}
	return 0;
}

int32 NPC::GetMaxDamage(int8 tlevel)
{
	int32 dmg = 0;
	if (tlevel < 40)
		dmg = tlevel*2+2;
	else if (tlevel < 50)
		dmg = level*25/10+2;
	else if (tlevel < 60)
		dmg = (tlevel*3+2)+((tlevel-50)*30);
	else
		dmg = (tlevel*3+2)+((tlevel-50)*35);
	return dmg;
}

void NPC::PickPocket(Client* thief) {
	
	//make sure were allowed to targte them:
	int olevel = GetLevel();
	if(olevel > (thief->GetLevel() - THIEF_PICKPOCKET_UNDER)) {
		thief->Message(13, "Your not good enough to steal from them.");
		//should we check aggro
		return;
	}
	
	
	int steal_skill = thief->GetSkill(PICK_POCKETS);
	int stealchance = (rand()%6+1)*olevel;
	ItemInst* inst = 0;
	int x = 0;
	int slot[50];
	int steal_items[50];
	int charges[50];
	int money[4];
	money[0] = GetPlatinum();
	money[1] = GetGold();
	money[2] = GetSilver();
	money[3] = GetCopper();
	if (steal_skill < 125)
		money[0] = 0;
	if (steal_skill < 60)
		money[1] = 0;
	memset(slot,0,50);
	memset(steal_items,0,50);
	memset(charges,0,50);
	//Determine wheter to steal money or an item.
	bool no_coin = ((money[0] + money[1] + money[2] + money[3]) == 0);
	bool steal_item = (rand()%100 < 50 || no_coin);
	if (steal_item)
	{
		LinkedListIterator<ServerLootItem_Struct*> iterator(*itemlist);
		iterator.Reset();
		while(iterator.MoreElements())
		{
			const Item_Struct* item = database.GetItem(iterator.GetData()->item_id);
			if (item)
			{
				inst = ItemInst::Create(item,iterator.GetData()->charges);
				int slot_id = thief->GetInv().FindFreeSlot(false, true, inst->GetItem()->Size);
				if (/*!Equipped(item->ItemNumber) &&*/
					 !item->loreflag && !item->Common.Magic && item->NoDrop != 0 && !inst->IsType(ItemTypeContainer) && slot_id != SLOT_INVALID 
					/*&& steal_skill > item->Common.StealSkill*/ )
				{
					slot[x] = slot_id;
					steal_items[x] = item->ItemNumber;
					if (inst->IsStackable())
						charges[x] = 1;
					else
						charges[x] = iterator.GetData()->charges;
					x++;
					break;
				}
			}
			iterator.Advance();
		}
		if (x > 0)
		{
			int random = rand()%x;
			const Item_Struct* item = database.GetItem(steal_items[random]);
			inst = ItemInst::Create(item,charges[random]);

			if (/*item->Common.StealSkill || */steal_skill >= stealchance)
			{
				thief->Message_StringID(0,12903,item->Name,0);
				thief->PutItemInInventory(slot[random], *inst);
				thief->SendItemPacket(slot[random], inst, ItemPacketTrade);
				RemoveItem(item->ItemNumber);
			}
			else
			{
				if (stealchance - 25 > steal_skill)
				{
					thief->Message_StringID(0,12904,GetName(),0);
					AddToHateList(thief,50);
				}
				else
				{
					thief->Message_StringID(0,12898);
				}
			}
		}
		else if (!no_coin)
		{
			steal_item = false;
		}
		else
		{
			thief->Message_StringID(0,113);
		}
	}
	if (!steal_item) //Steal money
	{
		uint32 amt = (rand()%((steal_skill/25)+1))+1;
		int steal_type = 0;
		if (!money[0])
		{
			steal_type = 1;
			if (!money[1])
			{
				steal_type = 2;
				if (!money[2])
				{
					steal_type = 3;
				}
			}
		}

		if (steal_skill >= stealchance)
		{
			switch (steal_type)
			{
				case 0:
					if (amt > GetPlatinum())
						amt = GetPlatinum();
					SetPlatinum(GetPlatinum()-amt);
					thief->AddMoneyToPP(0,0,0,amt,true);
					break;
				case 1:
					if (amt > GetGold())
						amt = GetGold();
					SetGold(GetGold()-amt);
					thief->AddMoneyToPP(0,0,amt,0,true);
					break;
				case 2:
					if (amt > GetSilver())
						amt = GetSilver();
					SetSilver(GetSilver()-amt);
					thief->AddMoneyToPP(0,amt,0,0,true);
					break;
				case 3:
					if (amt > GetCopper())
						amt = GetCopper();
					SetCopper(GetCopper()-amt);
					thief->AddMoneyToPP(amt,0,0,0,true);
					break;
			}
			char chr_amt[10];
			memset(chr_amt,0,10);
			itoa(amt,chr_amt,10);
			thief->Message_StringID(0,12899+steal_type,chr_amt,0);
		}
		else
		{
			if (stealchance - 25 > steal_skill)
			{
				thief->Message_StringID(0,12904,GetName(),0);
				AddToHateList(thief,50);
			}
			else
			{
				thief->Message_StringID(0,12898);
			}
		}
	}
	safe_delete(inst);
}

void NPC::DoClassAttacks(Mob *target) {
	if(target == NULL)
		return;	//gotta have a target for all these
	
	//general stuff, for all classes....
	//only gets used when their primary ability get used too
	//this might be bad for pally's with long reuse time
	if (GetOwner() != NULL && taunting && target->IsNPC() && target->GetBodyType() != BT_Undead && taunt_timer.Check()) {
		Taunt(target->CastToNPC(), false);
	}
	
	if(!classattack_timer.Check(false))
		return;
	
	int level = GetLevel();
	int reuse = TauntReuseTime * 1000;	//make this very long since if they dont use it once, they prolly never will
	//class specific stuff...
	switch(GetClass()) {
		case ROGUE:
			if(level >= 10) {
				//does not take advantage of any equipped weapons for simplicity.
				if (BehindMob(target, GetX(), GetY())) {
					RogueBackstab(target, NULL, GetLevel()*5+5);
					reuse = BackstabReuseTime * 1000;
				}
			}
			break;
		case MONK: {
			int8 satype = saKick;
			if(level > 29) {
				satype = saFlyingKick;
			} else if(level > 24) {
				satype = saTailRake;
			} else if(level > 19) {
				satype = saEagleStrike;
			} else if(level > 9) {
				satype = saTigerClaw;
			} else if(level > 4) {
				satype = saRoundKick;
			}
			reuse = MonkSpecialAttack(target, satype);
			break;
		}
		case WARRIOR:
			//kick
			reuse = MonkSpecialAttack(target, saKick);
			break;
		case RANGER:
		case BEASTLORD:
			if(level > 5) {
				//kick
				reuse = MonkSpecialAttack(target, saKick);
			}
			break;
		case SHADOWKNIGHT:
			CastSpell(SPELL_NPC_HARM_TOUCH, target->GetID());
			reuse = HarmTouchReuseTime * 1000;
			break;
		case PALADIN:
			if(GetHPRatio() < 20) {
				CastSpell(SPELL_LAY_ON_HANDS, GetID());
				reuse = LayOnHandsReuseTime * 1000;
			}
			break;
	}
	
	classattack_timer.Start(reuse);
	
	
}

void Mob::NPCSpecialAttacks(const char* parse, int permtag) {
    for(int i = 0; i < SPECATK_MAXNUM; i++)
	{
	    SpecAttacks[i] = false;
        SpecAttackTimers[i] = NULL;
    }
	
	const char* orig_parse = parse;
    while (*parse)
    {
        switch(*parse)
        {
        case 'Z':
//			if (this->IsNPC())
 //   			this->CastToNPC()->interactive = true;
            break;
        case 'X':
   // 		if (this->IsNPC())
    //			this->CastToNPC()->citycontroller = true;
            break;
        case 'Y':
    //		if (this->IsNPC())
    //			this->CastToNPC()->guildbank = true;
            break;
	    case 'E':
    	    SpecAttacks[SPECATK_ENRAGE] = true;
    		break;
	    case 'F':
    	    SpecAttacks[SPECATK_FLURRY] = true;
    		break;
	    case 'R':
    	    SpecAttacks[SPECATK_RAMPAGE] = true;
    		break;

	    case 'S':
    	    SpecAttacks[SPECATK_SUMMON] = true;
            SpecAttackTimers[SPECATK_SUMMON] = new Timer(6000);
            SpecAttackTimers[SPECATK_SUMMON]->Start();
    		break;
	    case 'T':
            SpecAttacks[SPECATK_TRIPLE] = true;
            break;
	    case 'Q':
            SpecAttacks[SPECATK_QUAD] = true;
            break;
		case 'U':
			SpecAttacks[UNSLOWABLE] = true;
			break;
		case 'M':
			SpecAttacks[UNMEZABLE] = true;
			break;
		case 'C':
			SpecAttacks[UNCHARMABLE] = true;
			break;
		case 'N':
			SpecAttacks[UNSTUNABLE] = true;
			break;
		case 'I':
			SpecAttacks[UNSNAREABLE] = true;
			break;
		case 'A':
			SpecAttacks[IMMUNE_MEELE] = true;
			break;
		case 'B':
			SpecAttacks[IMMUNE_MAGIC] = true;
			break;
        default:
            break;
        }
        parse++;
    }
	
	if(permtag == 1 && this->GetNPCTypeID() > 0){
		if(database.SetSpecialAttkFlag(this->GetNPCTypeID(), orig_parse)) {
			LogFile->write(EQEMuLog::Normal, "NPCTypeID: %i flagged to '%s' for Special Attacks.\n",this->GetNPCTypeID(),orig_parse);
		}
	}
}

