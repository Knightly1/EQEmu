/*  EQEMu:  Everquest Server Emulator
Copyright (C) 2001-2004  EQEMu Development Team (http://eqemu.org)

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
#include <iomanip>
using namespace std;
#include <stdlib.h>
#include <math.h>
#include "npc.h"
#include "masterentity.h"
#include "NpcAI.h"
#include "map.h"
#include "../common/moremath.h"
#include "parser.h"
#include "StringIDs.h"
#include "../common/bodytypes.h"
#ifdef GUILDWARS
#include "../GuildWars/GuildWars.h"
extern GuildWars guildwars;
#endif

#ifndef NEW_LoadSPDat
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif

extern EntityList entity_list;
extern Database database;
extern Zone *zone;
extern Parser * parse;

const int Z_AGGRO=10;

const int MobAISpellRange=100; // max range of buffs
const int SpellType_Nuke=1;
const int SpellType_Heal=2;
const int SpellType_Root=4;
const int SpellType_Buff=8;
const int SpellType_Escape=16;
const int SpellType_Pet=32;
const int SpellType_Lifetap=64;
const int SpellType_Snare=128;
const int SpellType_DOT=256;

#define SpellType_Any		0xFFFF
#ifdef _EQDEBUG
	#define MobAI_DEBUG_Spells	-1
#else
	#define MobAI_DEBUG_Spells	-1
#endif

bool Mob::AICastSpell(Mob* tar, int8 iChance, int16 iSpellTypes) {
// Faction isnt checked here, it's assumed you wouldnt pass a spell type you wouldnt want casted on the mob
	if (!tar)
		return false;
	if (iChance < 100) {
		int8 tmp = rand()%100;
		if (tmp >= iChance)
			return false;
	}
		
	float dist2;

	if (iSpellTypes & SpellType_Escape) {
	    dist2 = 0; //DistNoRoot(*this);	//WTF was up with this...
    }
	else 
	    dist2 = DistNoRoot(*tar);
	
	float manaR = GetManaRatio();
//	for (int i=0; i<MAX_AISPELLS; i++) {
	for (int i=MAX_AISPELLS-1; i >= 0; i--) {
		if (AIspells[i].spellid <= 0 || AIspells[i].spellid >= SPDAT_RECORDS) {
			// this is both to quit early to save cpu and to avoid casting bad spells
			// Bad info from database can trigger this incorrectly, but that should be fixed in DB, not here
			//return false;
			continue;
		}
		if (iSpellTypes & AIspells[i].type) {
			// manacost has special values, -1 is no mana cost, -2 is instant cast (no mana)
			sint32 mana_cost = AIspells[i].manacost;
			if (mana_cost == -1)
				mana_cost = spells[AIspells[i].spellid].mana;
			else if (mana_cost == -2)
				mana_cost = 0;
			if (
				((
					(spells[AIspells[i].spellid].targettype==ST_AECaster || spells[AIspells[i].spellid].targettype==ST_AEBard)
					&& dist2 <= spells[AIspells[i].spellid].aoerange*spells[AIspells[i].spellid].aoerange
				 ) ||
				 dist2 <= spells[AIspells[i].spellid].range*spells[AIspells[i].spellid].range
				)
				&& (mana_cost <= GetMana() || GetMana() == GetMaxMana())
				&& (AIspells[i].time_cancast+(rand()%5)) <= Timer::GetCurrentTime() //break up the spelling casting over a period of time.
				) {

#if MobAI_DEBUG_Spells >= 21
				cout << "Mob::AICastSpell: Casting: spellid=" << AIspells[i].spellid
                    << ", tar=" << tar->GetName() 
                    << ", dist2[" << dist2 << "]<=" << spells[AIspells[i].spellid].range *spells[AIspells[i].spellid].range 
                    << ", mana_cost[" << mana_cost << "]<=" << GetMana() 
                    << ", cancast[" << AIspells[i].time_cancast << "]<=" << Timer::GetCurrentTime()
                    << ", type=" << AIspells[i].type << endl;
#endif

				switch (AIspells[i].type) {
					case SpellType_Heal: {
						if (
							(spells[AIspells[i].spellid].targettype == ST_Target || tar == this)
							&& tar->DontHealMeBefore() < Timer::GetCurrentTime()
							) {
							int8 hpr = (int8)tar->GetHPRatio();
							if (
								hpr <= 35 
								|| (!IsEngaged() && hpr <= 50)
								|| (tar->IsClient() && hpr <= 99)
								) {
								AIDoSpellCast(i, tar, mana_cost, &tar->DontHealMeBefore());
								return true;
							}
						}
						break;
					}
					case SpellType_Root: {
						if (
							!tar->IsRooted() && dist2 >= 900 && (rand()%100) < 50
							&& tar->DontRootMeBefore() < Timer::GetCurrentTime()
							&& tar->CanBuffStack(AIspells[i].spellid, GetLevel(), true) >= 0
							) {
							AIDoSpellCast(i, tar, mana_cost, &tar->DontRootMeBefore());
							return true;
						}
						break;
					}
					case SpellType_Buff: {
						if (
							(spells[AIspells[i].spellid].targettype == ST_Target || tar == this)
							&& tar->DontBuffMeBefore() < Timer::GetCurrentTime()
							&& tar->CanBuffStack(AIspells[i].spellid, GetLevel(), true) >= 0
							) {
							AIDoSpellCast(i, tar, mana_cost, &tar->DontBuffMeBefore());
							return true;
						}
						break;
					}
					case SpellType_Escape: {
	                #ifdef IPC          
                        if (GetHPRatio() <= 5 || (IsNPC() && CastToNPC()->IsInteractive() && tar != this) )
					#else
                        if (GetHPRatio() <= 5 )	
                    #endif
                    	{
                            AIDoSpellCast(i, tar, mana_cost);
							return true;
						}
						break;
					}
					case SpellType_Nuke: {
						if (
							manaR >= 40 && (rand()%100) < 50
							&& tar->CanBuffStack(AIspells[i].spellid, GetLevel(), true) >= 0
							) {
							AIDoSpellCast(i, tar, mana_cost);
							return true;
						}
						break;
					}
					case SpellType_Pet: {
						if (!(GetPetID()||GetOwner()) && (rand()%100) < 25) {
							AIDoSpellCast(i, tar, mana_cost);
							return true;
						}
						break;
					}
					case SpellType_Lifetap: {
						if (GetHPRatio() <= 75
							&& (rand()%100) < 50
							&& tar->CanBuffStack(AIspells[i].spellid, GetLevel(), true) >= 0
							) {
							AIDoSpellCast(i, tar, mana_cost);
							return true;
						}
						break;
					}
					case SpellType_Snare: {
						if (
							!tar->IsRooted() && (rand()%100) < 50
							&& tar->DontSnareMeBefore() < Timer::GetCurrentTime()
							&& tar->CanBuffStack(AIspells[i].spellid, GetLevel(), true) >= 0
							) {
							AIDoSpellCast(i, tar, mana_cost, &tar->DontSnareMeBefore());
							return true;
						}
						break;
					}
					case SpellType_DOT: {
						if (
							tar->GetHPRatio() > 50 && (rand()%100) < 20
							&& tar->DontDotMeBefore() < Timer::GetCurrentTime()
							&& tar->CanBuffStack(AIspells[i].spellid, GetLevel(), true) >= 0
							) {
							AIDoSpellCast(i, tar, mana_cost, &tar->DontDotMeBefore());
							return true;
						}
						break;
					}
					default: {
						cout<<"Error: Unknown spell type in AICastSpell. caster:"<<this->GetName()<<" type:"<<AIspells[i].type<<" slot:"<<i<<endl;
						break;
					}
				}
			}
#if MobAI_DEBUG_Spells >= 21
			else {
				cout << "Mob::AICastSpell: NotCasting: spellid=" << AIspells[i].spellid << ", tar=" << tar->GetName() << ", dist2[" << dist2 << "]<=" << spells[AIspells[i].spellid].range*spells[AIspells[i].spellid].range << ", mana_cost[" << mana_cost << "]<=" << GetMana() << ", cancast[" << AIspells[i].time_cancast << "]<=" << Timer::GetCurrentTime() << endl;
			}
#endif
		}
	}
	return false;
}

void Mob::AIDoSpellCast(int8 i, Mob* tar, sint32 mana_cost, int32* oDontDoAgainBefore) {
#if MobAI_DEBUG_Spells >= 1
	cout << "Mob::AIDoSpellCast: spellid=" << AIspells[i].spellid << ", tar=" << tar->GetName() << ", mana=" << mana_cost << ", Name: " << spells[AIspells[i].spellid].name << endl;
#endif
	casting_spell_AIindex = i;
	
	//stop moving if were casting a spell and were not a bard...
	if(!IsBardSong(AIspells[i].spellid)) {
		SetRunAnimSpeed(0);
		SendPosition();
		SetMoving(false);
	}
	
	CastSpell(AIspells[i].spellid, tar->GetID(), 1, AIspells[i].manacost == -2 ? 0 : -1, mana_cost, oDontDoAgainBefore);
}

bool EntityList::AICheckCloseSpells(Mob* caster, int8 iChance, float iRange, int16 iSpellTypes) {
	if (iChance < 100) {
		int8 tmp = rand()%100;
		if (tmp >= iChance)
			return false;
	}
	iRange *= iRange;
    LinkedListIterator<Mob*> iterator(mob_list);
	iterator.Reset();
    while(iterator.MoreElements()) {
		Mob* mob = iterator.GetData();
		if (mob != caster  
			#ifndef GUILDWARS
			&& !( mob->IsClient() )
			#endif
			&&
			((mob->IsNPC() && mob->GetFactionCon(caster) <= FACTION_AMIABLE)
#ifdef GUILDWARS
			|| (mob->IsClient() && ((mob->GetLevel()-caster->GetLevel()) >= -10) && mob->GetFactionCon(caster) <= FACTION_KINDLY)
#endif
			))
			{
			if (mob->DistNoRoot(*caster) <= iRange) {
				//they are in range, and we like them, now make sure
				//that we can see them...
				if(caster->CheckLosFN(mob)) {
					// we have a winner!
					if (caster->AICastSpell(mob, 100, iSpellTypes))
						return true;
				}
			}
		}
		iterator.Advance();
	}
	return false;
}

void Mob::AI_Init() {
	pAIControlled = false;
	AIthink_timer = 0;
	AIwalking_timer = 0;
	AImovement_timer = 0;
	AIautocastspell_timer = 0;
	AIscanarea_timer = 0;
	pLastFightingDelayMoving = 0;
	minLastFightingDelayMoving = 10000;
	maxLastFightingDelayMoving = 20000;
	memset(AIspells, 0, sizeof(AIspells));
	casting_spell_AIindex = MAX_AISPELLS;
	npc_spells_id = 0;

	pDontHealMeBefore = 0;
	pDontBuffMeBefore = 0;
	pDontDotMeBefore = 0;
	pDontRootMeBefore = 0;
	pDontSnareMeBefore = 0;
	pDontCastBefore_casting_spell = 0;

	roambox_max_x = 0;
	roambox_max_y = 0;
	roambox_min_x = 0;
	roambox_min_y = 0;
	roambox_distance = 0;
	roambox_movingto_x = 0;
	roambox_movingto_y = 0;
	roambox_delay = 2500;
}

void NPC::AI_Init() {
	Mob::AI_Init();
}

void Client::AI_Init() {
	Mob::AI_Init();
	minLastFightingDelayMoving = CLIENT_LD_TIMEOUT;
	maxLastFightingDelayMoving = CLIENT_LD_TIMEOUT;
}

void Mob::AI_Start(int32 iMoveDelay) {
	if (iMoveDelay)
		pLastFightingDelayMoving = Timer::GetCurrentTime() + iMoveDelay;
	else
		pLastFightingDelayMoving = 0;
	if (pAIControlled)
		return;
	pAIControlled = true;
	AIthink_timer = new Timer(50);
	AIthink_timer->Trigger();
	AIwalking_timer = new Timer(0);
	AImovement_timer = new Timer(100);
	AIautocastspell_timer = new Timer(750);
	AIautocastspell_timer->Start(RandomTimer(0, 15000), false);
	AIscanarea_timer = new Timer(500);
	for (int i=0; i<MAX_AISPELLS; i++) {
		AIspells[i].spellid = 0xFFFF;
		AIspells[i].type = 0;
	}

	if (GetArrgoRange() == 0)
		pArrgoRange = 70;
	if (GetAssistRange() == 0)
		pAssistRange = 70;
	hate_list.Wipe();

	delta_heading = 0;
	delta_x = 0;
	delta_y = 0;
	delta_z = 0;
	pRunAnimSpeed = 0;
	pLastChange = Timer::GetCurrentTime();
}

void Client::AI_Start(int32 iMoveDelay) {
	Mob::AI_Start(iMoveDelay);
	if (!pAIControlled)
		return;
	// copy memed spells to the spells struct here
	this->Message_StringID(13,PLAYER_CHARMED);
/*	APPLAYER *app = new APPLAYER(OP_Charm, sizeof(Charm_Struct));
	Charm_Struct *ps = (Charm_Struct*)app->pBuffer;
	ps->owner_id = GetOwnerOrSelf()->GetID();
	ps->pet_id = this->GetID();
	ps->command = 1;
	FastQueuePacket(&app);*/
	Group* group = GetGroup();
	if (this->isgrouped && group != NULL)
    {
		group->DelMember(CastToMob(),true);
    }

	if (AIspells[0].spellid == 0)
		AIautocastspell_timer->Disable();
	SaveSpawnSpot();
	pClientSideTarget = target ? target->GetID() : 0;
	SendAppearancePacket(AT_Anim, ANIM_FREEZE);	// this freezes the client
	SendAppearancePacket(AT_Linkdead, 1); // Sending LD packet so *LD* appears by the player name when charmed/feared -Kasai
	attack_timer.Enable();
	attack_dw_timer.Enable();
	SetAttackTimer();
}

void NPC::AI_Start(int32 iMoveDelay) {
	Mob::AI_Start(iMoveDelay);
	if (!pAIControlled)
		return;
	if (NPCTypedata) {
		AI_AddNPCSpells(NPCTypedata->npc_spells_id);
		NPCSpecialAttacks(NPCTypedata->npc_attacks,0);
	}
	if (AIspells[0].spellid == 0)
		AIautocastspell_timer->Disable();
	SendTo(GetX(), GetY(), GetZ());
	SetChanged();
	SaveSpawnSpot();
	SaveGuardSpot();
}

void Mob::AI_Stop() {
	if (!IsAIControlled())
		return;
	pAIControlled = false;
	Waypoints.ClearListAndData();
	safe_delete(AIthink_timer);
	safe_delete(AIwalking_timer);
	safe_delete(AImovement_timer);
	safe_delete(AIautocastspell_timer);
	safe_delete(AIscanarea_timer);
	hate_list.Wipe();
}

void Client::AI_Stop() {
	Mob::AI_Stop();
	this->Message_StringID(13,PLAYER_REGAIN);
	APPLAYER *app = new APPLAYER(OP_Charm, sizeof(Charm_Struct));
	Charm_Struct *ps = (Charm_Struct*)app->pBuffer;
	ps->owner_id = 0;
	ps->pet_id = this->GetID();
	ps->command = 0;
	FastQueuePacket(&app);
	target = entity_list.GetMob(pClientSideTarget);
	SendAppearancePacket(AT_Anim, GetAppearanceValue(appearance));
	SendAppearancePacket(AT_Linkdead, 0); // Removing LD packet so *LD* no longer appears by the player name when charmed/feared -Kasai
	if (!auto_attack) {
		attack_timer.Disable();
		attack_dw_timer.Disable();
	}
	if (IsLD())
	{
		Save();
		Disconnect();
	}
}

void Mob::AI_Process() {

	sint16 gridno; 


	if (!IsAIControlled())
		return;

	if (!(AIthink_timer->Check() || attack_timer.Check(false)))
		return;

	if (IsCasting())
		return;

	if (IsEngaged()) 
	{
		if (IsRooted())
			SetTarget(hate_list.GetClosest(this));
		else
			SetTarget(hate_list.GetTop());

		if (!target)
			return;

	        if (GetHPRatio() < 15)
        	    StartEnrage();
		
		bool is_combat_range = CombatRange(target);
		
		//swarm pet procs. Adapted from Dook's work
		if(GetBodyType() == BT_SwarmPet 
			&& is_combat_range
			&& IsAttackAllowed(target) ) {
			if(GetClass() == RANGER) {
				//Dook- Swarm Pets -Swarm of Decay
				if(DistNoRoot(*target) <= 10000) {
					FaceTarget(target); 
					DoAnim(animSwarmAttack, 9); 
					int dmg=MakeRandomInt(50,200);
					target->Damage(this, dmg, 0xffff, 0x07, true); 
				}
			} else if(GetRace() == 89) {
	//WTF is up with classes 90 and 91???
				//Dook- Servant of Ro castin Bolt of Lava 
				CastSpell(3005,target->GetID(),9,3000,0,0,0); 
			} else if(GetRace()==127 && GetClass()==90) {
				if(MakeRandomInt(0,10) >= 9) {
					CastSpell(1638,target->GetID(),9,0,0,0,0); 
				}
			} else if(GetRace() == 120 && GetClass() == 91) {
				//Dook- ToDo Add GoD shaman swarm puppy proc here 
			}
		}
		
        if (is_combat_range) 
        {
			if (AImovement_timer->Check()) 
			{
				SetRunAnimSpeed(0);
			}
			if(IsMoving())
			{
				SetMoving(false);
				moved=false;
				/*while(DistNoZ(*target)<10){ //dont want them too close
					x_pos -= tar_vx*.2;
					y_pos -= tar_vy*.2;
					z_pos -= tar_vz*.2;
				}*/
				SendPosition();
				tar_ndx =0;
			}
			if (GetAppearance() == 0 && GetRunAnimSpeed() == 0 && !IsStunned())
			
			//should implement some checks for the target being dead mid-attack.
			if (attack_timer.Check()) 
			{
				Attack(target, 13);
				if (target) 
				{
					if (CanThisClassDoubleAttack()) 
					{
						sint32 RandRoll = rand()%100;
						if (target && RandRoll < (GetLevel() + 20))  
						{
							if (Attack(target, 13)) 
							{
								// lets see if we can do a triple attack with the main hand
								if (SpecAttacks[SPECATK_TRIPLE]) 
								{
									if (!GetOwner() && RandRoll < (GetLevel()))
									{
										if (Attack(target, 13)) 
										{	// now lets check the quad attack
											if (SpecAttacks[SPECATK_QUAD]) 
											{
												if (!GetOwner() && RandRoll < (GetLevel() - 20))  
												{
													Attack(target, 13);
												}
											} // if (SpecAttacks[SPECATK_QUAD])
										}
									}
								} // if (SpecAttacks[SPECATK_TRIPLE])
							}
						}
					} // if (CanThisClassDoubleAttack())
				}

				if (SpecAttacks[SPECATK_FLURRY]) {
				    // perhaps get the values from the db?
				    if (MakeRandomInt(0, 99) < 20)
						Flurry();
				}

				if (SpecAttacks[SPECATK_RAMPAGE]) {
				    // perhaps get the values from the db?
				    if (MakeRandomInt(0, 99) < 20)
						Rampage();
				}
			}
		
			if (target && attack_dw_timer.Check() && CanThisClassDualWield()) 
			{
				int myclass = GetClass();
				//can only dual weild without a weapon if your a monk
				if((equipment[8] && GetLevel() > 39) || myclass == MONK || myclass == MONKGM) {
					float DualWieldProbability = (GetSkill(DUAL_WIELD) + GetLevel()) / 400.0f;
					DualWieldProbability -= MakeRandomFloat(0, 1);
					if(DualWieldProbability < 0){
						Attack(target, 14);
						if (CanThisClassDoubleAttack()) 
						{
							sint32 RandRoll = rand()%100;
							if (RandRoll < (GetLevel() + 20))  
							{
								if (target && Attack(target, 14));
							}
						} // if (CanThisClassDoubleAttack())
					}
				}
			}
			//should be checking for dead I think...
			if(!target) return;
				
			if(IsNPC())
				CastToNPC()->DoClassAttacks(target);
			
			if (AIautocastspell_timer->Check()) 
			{
		#if MobAI_DEBUG_Spells >= 25
				cout << "Engaged autocast check triggered: " << this->GetName() << endl;
		#endif
				if (!AICastSpell(this, 100, SpellType_Heal | SpellType_Escape)) // try casting a heal or gate
					if (!entity_list.AICheckCloseSpells(this, 25, MobAISpellRange, SpellType_Heal)) // try casting a heal on nearby
						AICastSpell(target, 20, SpellType_Nuke | SpellType_Lifetap | SpellType_DOT);
			}
		}	//end is within combat range
		else {
			// See if we can summon the mob to us
			if (!HateSummon()) 
			{
// TODO: Check here for another person on hate list with close hate value
				if (AIautocastspell_timer->Check()) 
				{
#if MobAI_DEBUG_Spells >= 25
					cout << "Engaged (pursing) autocast check triggered: " << this->GetName() << endl;
#endif
					AICastSpell(target, 90, SpellType_Root | SpellType_Nuke | SpellType_Lifetap | SpellType_Snare);
				}
				else if (AImovement_timer->Check()) 
				{
					if(!IsRooted())
						CalculateNewPosition(target->GetX(), target->GetY(), target->GetZ(), GetRunspeed());
					else if(IsMoving())
					{
						SetHeading(CalculateHeadingToTarget(target->GetX(), target->GetY()));
						SetRunAnimSpeed(0);
						SendPosition();
						SetMoving(false);
						moved=false;
					}
				}
			}
		}
	}
	else { // not engaged
		//if (pStandingPetOrder == SPO_Follow && GetOwnerID() && !IsStunned())
			//SetHeading(CalculateHeadingToTarget(target->GetX(), target->GetY())*8);
			//FaceTarget(GetOwner(), true);
// trigger EVENT_SIGNAL if required
		if (signaled==true)
		{
//			printf("Signal received\n");
			parse->Event(EVENT_SIGNAL, this->GetNPCTypeID(), "", this->CastToMob(), 0);
			signaled=false;
		}
		if (AIautocastspell_timer->Check()) 
		{
#if MobAI_DEBUG_Spells >= 25
			cout << "Non-Engaged autocast check triggered: " << this->GetName() << endl;
#endif
			AIautocastspell_timer->Start(2500, false);
			if (!AICastSpell(this, 100, SpellType_Heal | SpellType_Buff | SpellType_Pet))
				entity_list.AICheckCloseSpells(this, 33, MobAISpellRange, SpellType_Heal | SpellType_Buff);
		}
		else if (AIscanarea_timer->Check()) 
		{
			Mob* tmptar = entity_list.AICheckCloseArrgo(this, GetArrgoRange(), GetAssistRange());
			if (tmptar) 
			{
				AddToHateList(tmptar);
			}
		}
		else if (AImovement_timer->Check() && !IsRooted()) 
		{
			SetRunAnimSpeed(0);
			if (GetOwnerID()) 
			{
				// we're a pet, do as we're told
				switch (pStandingPetOrder) 
				{
					case SPO_Follow: 
					{
						
						Mob* owner = GetOwnerOrSelf();
						
						//if(owner->IsClient())
						//	printf("Pet start pos: (%f, %f, %f)\n", GetX(), GetY(), GetZ());
						
						float dist = DistNoRoot(*owner);
						if (dist >= 100) 
						{
							float speed = GetWalkspeed();
							if (dist >= 25)
								speed = GetRunspeed();
							CalculateNewPosition2(owner->GetX(), owner->GetY(), owner->GetZ(), speed);
						}
						else
						{
							SetHeading(owner->GetHeading());
							if(moved)
							{
								SendPosition();
								moved=false;
								SetMoving(false);
							}
						}
					
						/*
						//fix up Z
						float zdiff = GetZ() - owner->GetZ();
						if(zdiff < 0)
							zdiff = 0 - zdiff;
						if(zdiff > 2.0f) {
							SendTo(GetX(), GetY(), owner->GetZ());
							SendPosition();
						}
						
						if(owner->IsClient())
							printf("Pet pos: (%f, %f, %f)\n", GetX(), GetY(), GetZ());
						*/
						
						break;
					}
					case SPO_Sit: 
					{
						SetAppearance(1, false);
						break;
					}
					case SPO_Guard: 
					{
						if (!CalculateNewPosition2(GetGuardX(), GetGuardY(), GetGuardZ(), GetWalkspeed())) 
						{
							SetHeading(GetGuardHeading());
						}
						break;
					}
				}
			}
			else if (GetFollowID()) 
			{
				Mob* follow = entity_list.GetMob(GetFollowID());
				if (!follow) SetFollowID(0);
				else 
				{
					float dist2 = DistNoRoot(*follow);
					if (dist2 >= 100) 
					{
						float speed = GetWalkspeed();
						if (dist2 >= 225)
							speed = GetRunspeed();
						CalculateNewPosition2(follow->GetX(), follow->GetY(), follow->GetZ(), speed);
					}
					else
					{
						if(moved)
						{
							SendPosition();
							moved=false;
							SetMoving(false);
						}
					}
					
					/*
					//fix up Z proble mssince CalculateNewPosition2 ignores pure-Z-movement now...
					float zdiff = GetZ() - follow->GetZ();
					if(zdiff < 0)
						zdiff = 0 - zdiff;
					if(zdiff > 2.0f) {
						SendTo(GetX(), GetY(), follow->GetZ());
						SendPosition();
					}
					
					if(follow->IsClient())
						printf("Follow pos: (%f, %f, %f)\n", GetX(), GetY(), GetZ());
					*/
					
				}
			}

			else 
			{
                 //this kinda assumes that we are an NPC without checking it..
                  	
				// dont move till a bit after you last fought
				if (pLastFightingDelayMoving < Timer::GetCurrentTime()) 
				{
					if (this->IsClient()) 
					{
						// LD timer expired, drop out of world
						if (this->CastToClient()->IsLD())
							this->CastToClient()->Disconnect();
						return;
					}
					if (roambox_distance) 
					{
						if (
							roambox_movingto_x > roambox_max_x
							|| roambox_movingto_x < roambox_min_x
							|| roambox_movingto_y > roambox_max_y
							|| roambox_movingto_y < roambox_min_y
							) 
						{
							float movedist = roambox_distance*roambox_distance;
							float movex = movedist * ((float)rand()/RAND_MAX);
							float movey = movedist - movex;
							movex = sqrt(movex);
							movey = sqrt(movey);
//cout << "1: MoveDist: " << roambox_distance << " MoveX: " << movex << " MoveY: " << movey << " MaxX: " << roambox_max_x << " MinX: " << roambox_min_x << " MaxY: " << roambox_max_y << " MinY: " << roambox_min_y << endl;
							movex *= rand()%2 ? 1 : -1;
							movey *= rand()%2 ? 1 : -1;
							roambox_movingto_x = GetX() + movex;
							roambox_movingto_y = GetY() + movey;
//printf("Roambox: Moving to: %1.2f, %1.2f  Move: %1.2f, %1.2f\n", roambox_movingto_x, roambox_movingto_y, movex, movey);
//cout << "2: RoamBox: Moving to: " << roambox_movingto_x << ", " << roambox_movingto_y << "  Move: " << movex << ", " << movey << endl;
							if (roambox_movingto_x > roambox_max_x || roambox_movingto_x < roambox_min_x)
								roambox_movingto_x -= movex * 2;
							if (roambox_movingto_y > roambox_max_y || roambox_movingto_y < roambox_min_y)
								roambox_movingto_y -= movey * 2;
//cout << "3: RoamBox: Moving to: " << roambox_movingto_x << ", " << roambox_movingto_y << "  Move: " << movex << ", " << movey << endl;
							if (roambox_movingto_x > roambox_max_x || roambox_movingto_x < roambox_min_x)
								roambox_movingto_x = roambox_max_x;
							if (roambox_movingto_y > roambox_max_y || roambox_movingto_y < roambox_min_y)
								roambox_movingto_y = roambox_max_y;
//cout << "4: RoamBox: Moving to: " << roambox_movingto_x << ", " << roambox_movingto_y << "  Move: " << movex << ", " << movey << endl;
						}
						else if (!CalculateNewPosition2(roambox_movingto_x, roambox_movingto_y, GetZ(), GetWalkspeed(), true)) 
						{
							roambox_movingto_x = roambox_max_x + 1; // force update
							pLastFightingDelayMoving = Timer::GetCurrentTime() + RandomTimer(roambox_delay, roambox_delay + 5000);
						}
					}
					else if (roamer) 
					{	
						if (AIwalking_timer->Check())
						{
							movetimercompleted=true;
							AIwalking_timer->Disable();
						}

						gridno = CastToNPC()->GetGrid(); 

// handle quest command roamers with no grids too
						if (gridno > 0 || cur_wp==-2)  { 
							if (movetimercompleted==true) {  // time to pause at wp is over
// MYRA - Added code to depop at end of grid for wander type 4
								if (wandertype == 4 && cur_wp == max_wp) { 
						           CastToNPC()->Depop(); 
								} else { 
									movetimercompleted=false; 
									char temp[100]; 
									entity_list.OpenDoorsNear(CastToNPC());
									parse->Event(EVENT_WAYPOINT,this->GetNPCTypeID(), itoa(cur_wp,temp,10), this->CastToMob(), 0); 
									CalculateNewWaypoint(); 
									SetAppearance(0, false); 
		                        } 
		                    }	// endif (movetimercompleted==true)     
							else if (!(AIwalking_timer->Enabled()))
							{	// currently moving
								if (cur_wp_x == GetX() && cur_wp_y == GetY()) 
								{	// are we there yet? then stop
									SetWaypointPause(); 
									SetAppearance(0, false); 
									SendPosition();
								} 
								else
								{	// not at waypoint yet, so keep moving
									CalculateNewPosition2(cur_wp_x, cur_wp_y, cur_wp_z, GetWalkspeed(), true); 
								}
							} 
						}		// endif (gridno > 0) 
// handle new quest grid command processing
						else if (gridno < 0) 
						{	// this mob is under quest control
							if (movetimercompleted==true)    
							{ // time to pause has ended
								this->CastToNPC()->SetGrid( 0 - this->CastToNPC()->GetGrid()); // revert to AI control
								SetAppearance(0, false); 
							}
						}

                  } 
                  else if (!(GetGuardX() == 0 && GetGuardY() == 0 && GetGuardZ() == 0)) 
                  { 
                     if (!CalculateNewPosition2(GetGuardX(), GetGuardY(), GetGuardZ(), GetWalkspeed())) 
                     { 
						if(moved)
						{
							moved=false;
							SetMoving(false);
							SendPosition();
							if (!GetTarget() || 
							  (GetTarget() && CalculateDistance(GetTarget()->GetX(),GetTarget()->GetY(),GetTarget()->GetZ()) >= 5) )
							{
								SetHeading(GetGuardHeading()); 
							}
							else 
							{ 
								FaceTarget(GetTarget(), true); 
							} 
						}
					 } 
				  } 
            } 
         } 
      } // else if (AImovement_timer->Check()) 
   }
}

// Note: Mob that caused this may not get added to the hate list until after this function call completes
void Mob::AI_Event_Engaged(Mob* attacker, bool iYellForHelp) {
	if (!IsAIControlled())
		return;
	if (iYellForHelp)
		entity_list.AIYellForHelp(this, attacker);
}

// Note: Hate list may not be actually clear until after this function call completes
void Mob::AI_Event_NoLongerEngaged() {
	if (!IsAIControlled())
		return;
	this->AIwalking_timer->Start(RandomTimer(3000,20000));
	pLastFightingDelayMoving = Timer::GetCurrentTime();
	if (minLastFightingDelayMoving == maxLastFightingDelayMoving)
		pLastFightingDelayMoving += minLastFightingDelayMoving;
	else
		pLastFightingDelayMoving += (rand() % (maxLastFightingDelayMoving-minLastFightingDelayMoving)) + minLastFightingDelayMoving;
}

void Mob::AI_Event_SpellCastFinished(bool iCastSucceeded, int8 slot) {
	if (!IsAIControlled())
		return;
	if (slot == 1) {
		if (pDontCastBefore_casting_spell) {
			*pDontCastBefore_casting_spell = 0;
			pDontCastBefore_casting_spell = 0;
		}
		int32 recovery_time = 0;
		if (iCastSucceeded) {
			if (casting_spell_AIindex < MAX_AISPELLS) {
					recovery_time += spells[AIspells[casting_spell_AIindex].spellid].recovery_time;
					if (AIspells[casting_spell_AIindex].recast_delay >= 0){
						if (AIspells[casting_spell_AIindex].recast_delay <1000)
							AIspells[casting_spell_AIindex].time_cancast = Timer::GetCurrentTime() + (AIspells[casting_spell_AIindex].recast_delay*1000);
}
					else
						AIspells[casting_spell_AIindex].time_cancast = Timer::GetCurrentTime() + spells[AIspells[casting_spell_AIindex].spellid].recast_time;
			}
			if (!IsEngaged())
				recovery_time += 2500;
			if (recovery_time < AIautocastspell_timer->GetSetAtTrigger())
				recovery_time = AIautocastspell_timer->GetSetAtTrigger();
			AIautocastspell_timer->Start(recovery_time, false);
		}
		else
			AIautocastspell_timer->Start(800, false);
		casting_spell_AIindex = MAX_AISPELLS;
	}
}

void Mob::StartEnrage()
{
    // dont continue if already enraged
    if (bEnraged)
        return;
    if (SpecAttackTimers[SPECATK_ENRAGE] && !SpecAttackTimers[SPECATK_ENRAGE]->Check())
        return;
    // see if NPC has possibility to enrage
    if (!SpecAttacks[SPECATK_ENRAGE])
        return;
    // check if timer exists (should be true at all times)
    if (SpecAttackTimers[SPECATK_ENRAGE])
    {
		safe_delete(SpecAttackTimers[SPECATK_ENRAGE]);
        SpecAttackTimers[SPECATK_ENRAGE] = NULL;
    }

    if (!SpecAttackTimers[SPECATK_ENRAGE])
    {
        SpecAttackTimers[SPECATK_ENRAGE] = new Timer(10000);
    }
    // start the timer. need to call IsEnraged frequently since we dont have callback timers :-/
    SpecAttackTimers[SPECATK_ENRAGE]->Start();
    bEnraged = true;
    entity_list.MessageClose(this, true, 600, 13, "%s has become ENRAGED.", GetName());
}

bool Mob::IsEnraged()
{
    // check the timer and set to false if time is up
    if (bEnraged && SpecAttackTimers[SPECATK_ENRAGE] && SpecAttackTimers[SPECATK_ENRAGE]->Check())
    {
        entity_list.MessageClose(this, true, 600, 13, "%s is no longer enraged.", GetName());
        safe_delete(SpecAttackTimers[SPECATK_ENRAGE]);
        SpecAttackTimers[SPECATK_ENRAGE] = new Timer(360000);
        SpecAttackTimers[SPECATK_ENRAGE]->Start();
        bEnraged = false;
    }
    return bEnraged;
}

bool Mob::Flurry()
{
    // attack the most hated target, regardless of range or whatever
    Mob *target = GetHateTop();
	if (target) {
		entity_list.MessageClose(this, true, 600, 13, "%s executes a FLURRY of attacks on %s!", GetName(), target->GetName());
		for (int i = 0; i < MAX_FLURRY_HITS; i++)
			Attack(target);
	}
    return true;
}

bool Mob::AddRampage(Mob *mob)
{
    if (!SpecAttacks[SPECATK_RAMPAGE])
        return false;
    for (int i = 0; i < MAX_RAMPAGE_TARGETS; i++)
    {
        // if name is already on the list dont add it again
        if (strcasecmp(mob->GetName(), RampageArray[i]) == 0)
            return false;
        strcpy(RampageArray[i], mob->GetName());
        LogFile->write(EQEMuLog::Normal, "Adding %s to Rampage List in slot %d", RampageArray[i], i);
        return true;
    }
    return false;
}

bool Mob::Rampage()
{

    entity_list.MessageClose(this, true, 600, 13, "%s goes on a RAMPAGE!", GetName());
    for (int i = 0; i < MAX_RAMPAGE_TARGETS; i++)
    {
        // range is important
        if (strlen(RampageArray[i]) == 0)
        	continue;
        Mob *target = entity_list.GetMob(RampageArray[i]);
        if(target )
        {
            if (CombatRange(target))
                Attack(target);
        }
    }
    return true;
}

int32 Mob::GetLevelCon(int8 mylevel, int8 iOtherLevel) {
    sint16 diff = iOtherLevel - mylevel;
	int32 conlevel=0;
	
    if (diff == 0)
        return CON_WHITE;
    else if (diff >= 1 && diff <= 2)
        return CON_YELLOW;
    else if (diff >= 3)
        return CON_RED;

    if (mylevel <= 6)    // i didnt notice light blue mobs before level 6
    {
        if (diff <= -4)
            conlevel = CON_GREEN;
        else
            conlevel = CON_BLUE;
    }
    else if (mylevel <= 9)
	{
        if (diff <= -5)
            conlevel = CON_GREEN;
        else if (diff <= -4)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
    else if (mylevel <= 13)
	{
        if (diff <= -6)
            conlevel = CON_GREEN;
        else if (diff <= -5)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (mylevel <= 18)
	{
        if (diff <= -7)
            conlevel = CON_GREEN;
        else if (diff <= -6)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (mylevel <= 24)
	{
        if (diff <= -8)
            conlevel = CON_GREEN;
        else if (diff <= -7)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (mylevel <= 35)
	{
        if (diff <= -9)
            conlevel = CON_GREEN;
        else if (diff <= -8)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (mylevel <= 44)
	{
        if (diff <= -12)
            conlevel = CON_GREEN;
        else if (diff <= -11)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (mylevel <= 50)
	{
        if (diff <= -14)
            conlevel = CON_GREEN;
        else if (diff <= -12)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (mylevel <= 60)
	{
        if (diff <= -16)
            conlevel = CON_GREEN;
        else if (diff <= -14)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (mylevel >= 61)
    {
        if (diff <= -17)
            conlevel = CON_GREEN;
        else if (diff <= -15)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
    }
	return conlevel;

}

/*
alter table npc_types drop column usedspells;
alter table npc_types add column npc_spells_id int(11) unsigned not null default 0 after merchant_id;
Create Table npc_spells (
	id int(11) unsigned not null auto_increment primary key,
	name tinytext,
	parent_list int(11) unsigned not null default 0,
	attack_proc smallint(5) not null default -1,
	proc_chance tinyint(3) not null default 3
	);
create table npc_spells_entries (
	id int(11) unsigned not null auto_increment primary key,
	npc_spells_id int(11) not null,
	spellid smallint(5) not null default 0,
	type smallint(5) unsigned not null default 0,
	minlevel tinyint(3) unsigned not null default 0,
	maxlevel tinyint(3) unsigned not null default 255,
	manacost smallint(5) not null default '-1',
	recast_delay int(11) not null default '-1',
	priority smallint(5) not null default 0,
	index npc_spells_id (npc_spells_id)
	);
*/ 

bool IsSpellInList(DBnpcspells_Struct* spell_list, sint16 iSpellID);
void AddSpellToNPCList(Mob::AISpells_Struct* AIspells, sint16 iPriority, sint16 iSpellID, uint16 iType, sint16 iManaCost, sint32 iRecastDelay);

bool Mob::AI_AddNPCSpells(int32 iDBSpellsID) {
	// ok, this function should load the list, and the parent list then shove them into the struct and sort
	npc_spells_id = iDBSpellsID;
	memset(AIspells, 0, sizeof(AIspells));
	if (iDBSpellsID == 0) {
		AIautocastspell_timer->Disable();
		return false;
	}
	DBnpcspells_Struct* spell_list = database.GetNPCSpells(iDBSpellsID);
	if (!spell_list) {
		AIautocastspell_timer->Disable();
		return false;
	}
	DBnpcspells_Struct* parentlist = database.GetNPCSpells(spell_list->parent_list);
	uint32 i;
#if MobAI_DEBUG_Spells >= 10
	cout << "Loading NPCSpells onto " << this->GetName() << ": dbspellsid=" << iDBSpellsID;
	if (spell_list) {
		cout << " (found, " << spell_list->numentries << "), parentlist=" << spell_list->parent_list;
		if (spell_list->parent_list) {
			if (parentlist) {
				cout << " (found, " << parentlist->numentries << ")";
			}
			else
				cout << " (not found)";
		}
	}
	else
		cout << " (not found)";
	cout << endl;
#endif
	sint16 attack_proc_spell = -1;
	sint8 proc_chance = 3;
	if (parentlist) {
		attack_proc_spell = parentlist->attack_proc;
		proc_chance = parentlist->proc_chance;
		for (i=0; i<parentlist->numentries; i++) {
			if (GetLevel() >= parentlist->entries[i].minlevel && GetLevel() <= parentlist->entries[i].maxlevel && parentlist->entries[i].spellid > 0) {
				if (!IsSpellInList(spell_list, parentlist->entries[i].spellid))
					AddSpellToNPCList(AIspells, parentlist->entries[i].priority, parentlist->entries[i].spellid, parentlist->entries[i].type, parentlist->entries[i].manacost, parentlist->entries[i].recast_delay);
			}
		}
	}
	for (i=0; i<spell_list->numentries; i++) {
		if (spell_list->attack_proc >= 0) {
			attack_proc_spell = spell_list->attack_proc;
			proc_chance = spell_list->proc_chance;
		}
		if (GetLevel() >= spell_list->entries[i].minlevel && GetLevel() <= spell_list->entries[i].maxlevel && spell_list->entries[i].spellid > 0) {
			AddSpellToNPCList(AIspells, spell_list->entries[i].priority, spell_list->entries[i].spellid, spell_list->entries[i].type, spell_list->entries[i].manacost, spell_list->entries[i].recast_delay);
		}
	}
	if (attack_proc_spell > 0)
		AddProcToWeapon(attack_proc_spell, true, proc_chance);

#if MobAI_DEBUG_Spells >= 11
	i=0;
	for (int j=0; j<MAX_AISPELLS; j++) {
		if (AIspells[j].spellid > 0) {
			cout << "NPCSpells on " << this->GetName() << ": AIspells[" << j << "].spellid=" << setw(5) << AIspells[j].spellid << ": " << spells[AIspells[j].spellid].name << endl;
			i++;
		}
	}
	cout << i << " NPCSpells on " << this->GetName() << endl;
#endif

	if (AIspells[0].spellid == 0)
		AIautocastspell_timer->Disable();
	else
		AIautocastspell_timer->Trigger();
	return true;
}

bool IsSpellInList(DBnpcspells_Struct* spell_list, sint16 iSpellID) {
	for (uint32 i=0; i < spell_list->numentries; i++) {
		if (spell_list->entries[i].spellid == iSpellID)
			return true;
	}
	return false;
}

// adds a spell to the list, taking into account priority and resorting list as needed.
void AddSpellToNPCList(Mob::AISpells_Struct* AIspells, sint16 iPriority, sint16 iSpellID, uint16 iType, sint16 iManaCost, sint32 iRecastDelay) {
	if (iSpellID <= 0 || iSpellID > SPDAT_RECORDS) {

#if MobAI_DEBUG_Spells >= 1
		cout << "AddSpellToNPCList: Spell #" << iSpellID << " not added, out of bounds" << endl;
#endif

		return;
	}

#if MobAI_DEBUG_Spells >= 12
	cout << "Adding spell #" << iSpellID;
#endif

	for (int i=0; i<MAX_AISPELLS; i++) {
		if (AIspells[i].spellid <= 0) {
			AIspells[i].spellid = iSpellID;
			AIspells[i].priority = iPriority;
			AIspells[i].type = iType;
			AIspells[i].manacost = iManaCost;
			AIspells[i].recast_delay = iRecastDelay;

#if MobAI_DEBUG_Spells >= 12
			cout << " to slot " << i;
#endif

			break;
		}
		else if (AIspells[i].priority < iPriority) {
			for (int j=MAX_AISPELLS-1; j>i; j--) {
				AIspells[j].spellid = AIspells[j-1].spellid;
				AIspells[j].priority = AIspells[j-1].priority;
				AIspells[j].type = AIspells[j-1].type;
				AIspells[j].manacost = AIspells[j-1].manacost;
				AIspells[j].recast_delay = AIspells[j-1].recast_delay;
			}
			AIspells[i].spellid = iSpellID;
			AIspells[i].priority = iPriority;
			AIspells[i].type = iType;
			AIspells[i].manacost = iManaCost;
			AIspells[i].recast_delay = iRecastDelay;

#if MobAI_DEBUG_Spells >= 12
			cout << " to slot " << i;
#endif

			break;
		}
	}

#if MobAI_DEBUG_Spells >= 12
	cout << endl;
#endif

}


DBnpcspells_Struct* Database::GetNPCSpells(int32 iDBSpellsID) {
	if (iDBSpellsID == 0)
		return 0;
	if (!npc_spells_cache) {
		npc_spells_maxid = GetMaxNPCSpellsID();
		npc_spells_cache = new DBnpcspells_Struct*[npc_spells_maxid+1];
		npc_spells_loadtried = new bool[npc_spells_maxid+1];
		for (uint32 i=0; i<=npc_spells_maxid; i++) {
			npc_spells_cache[i] = 0;
			npc_spells_loadtried[i] = false;
		}
	}
	if (iDBSpellsID > npc_spells_maxid)
		return 0;
	if (npc_spells_cache[iDBSpellsID]) { // it's in the cache, easy =)
		return npc_spells_cache[iDBSpellsID];
	}
	else if (!npc_spells_loadtried[iDBSpellsID]) { // no reason to ask the DB again if we have failed once already
		npc_spells_loadtried[iDBSpellsID] = true;
		char errbuf[MYSQL_ERRMSG_SIZE];
		char *query = 0;
		MYSQL_RES *result;
		MYSQL_ROW row;
		
		if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, parent_list, attack_proc, proc_chance from npc_spells where id=%d", iDBSpellsID), errbuf, &result)) {
			safe_delete_array(query);
			if (mysql_num_rows(result) == 1) {
				row = mysql_fetch_row(result);
				int32 tmpparent_list = atoi(row[1]);
				sint16 tmpattack_proc = atoi(row[2]);
				int8 tmpproc_chance = atoi(row[3]);
				mysql_free_result(result);
				if (RunQuery(query, MakeAnyLenString(&query, "SELECT spellid, type, minlevel, maxlevel, manacost, recast_delay, priority from npc_spells_entries where npc_spells_id=%d ORDER BY minlevel", iDBSpellsID), errbuf, &result)) {
					safe_delete_array(query);
					int32 tmpSize = sizeof(DBnpcspells_Struct) + (sizeof(DBnpcspells_entries_Struct) * mysql_num_rows(result));
					npc_spells_cache[iDBSpellsID] = (DBnpcspells_Struct*) new uchar[tmpSize];
					memset(npc_spells_cache[iDBSpellsID], 0, tmpSize);
					npc_spells_cache[iDBSpellsID]->parent_list = tmpparent_list;
					npc_spells_cache[iDBSpellsID]->attack_proc = tmpattack_proc;
					npc_spells_cache[iDBSpellsID]->proc_chance = tmpproc_chance;
					npc_spells_cache[iDBSpellsID]->numentries = mysql_num_rows(result);
					int j = 0;
					while ((row = mysql_fetch_row(result))) {
						npc_spells_cache[iDBSpellsID]->entries[j].spellid = atoi(row[0]);
						npc_spells_cache[iDBSpellsID]->entries[j].type = atoi(row[1]);
						npc_spells_cache[iDBSpellsID]->entries[j].minlevel = atoi(row[2]);
						npc_spells_cache[iDBSpellsID]->entries[j].maxlevel = atoi(row[3]);
						npc_spells_cache[iDBSpellsID]->entries[j].manacost = atoi(row[4]);
						npc_spells_cache[iDBSpellsID]->entries[j].recast_delay = atoi(row[5]);
						npc_spells_cache[iDBSpellsID]->entries[j].priority = atoi(row[6]);
						j++;
					}
					mysql_free_result(result);
					return npc_spells_cache[iDBSpellsID];
				}
				else {
					cerr << "Error in AddNPCSpells query1 '" << query << "' " << errbuf << endl;
					safe_delete_array(query);
					return 0;
				}
			}
			else {
				mysql_free_result(result);
			}
		}
		else {
			cerr << "Error in AddNPCSpells query1 '" << query << "' " << errbuf << endl;
			safe_delete_array(query);
			return 0;
		}
		
		return 0;	
	}
	return 0;
}

int32 Database::GetMaxNPCSpellsID() {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT max(id) from npc_spells"), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 ret = 0;
			if (row[0])
				ret = atoi(row[0]);
			mysql_free_result(result);
			return ret;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetMaxNPCSpellsID query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return 0;
	}
	
	return 0;	
}

