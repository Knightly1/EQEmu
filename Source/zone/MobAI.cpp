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

const int THREATENLY_ARRGO_CHANCE=32; // 32/128 (25%) chance that a mob will arrgo on con Threatenly
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
		
	float dist;

	if (iSpellTypes & SpellType_Escape) {
	    dist = Dist(*this);
    }
	else 
	    dist = Dist(*tar);
	
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
					&& dist <= spells[AIspells[i].spellid].aoerange
				 ) ||
				 dist <= spells[AIspells[i].spellid].range
				)
				&& (mana_cost <= GetMana() || GetMana() == GetMaxMana())
				&& (AIspells[i].time_cancast+(rand()%5)) <= Timer::GetCurrentTime() //break up the spelling casting over a period of time.
				) {

#if MobAI_DEBUG_Spells >= 21
				cout << "Mob::AICastSpell: Casting: spellid=" << AIspells[i].spellid
                    << ", tar=" << tar->GetName() 
                    << ", dist[" << dist << "]<=" << spells[AIspells[i].spellid].range 
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
							!tar->IsRooted() && dist >= 30 && (rand()%100) < 50
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
                        if (GetHPRatio() <= 5 || (IsNPC() && CastToNPC()->IsInteractive() && tar != this) ) {
					#else
                        if (GetHPRatio() <= 5 ) {	
                    #endif
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
				cout << "Mob::AICastSpell: NotCasting: spellid=" << AIspells[i].spellid << ", tar=" << tar->GetName() << ", dist[" << dist << "]<=" << spells[AIspells[i].spellid].range << ", mana_cost[" << mana_cost << "]<=" << GetMana() << ", cancast[" << AIspells[i].time_cancast << "]<=" << Timer::GetCurrentTime() << endl;
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
	CastSpell(AIspells[i].spellid, tar->GetID(), 1, AIspells[i].manacost == -2 ? 0 : -1, mana_cost, oDontDoAgainBefore);
}

Mob* EntityList::AICheckCloseArrgo(Mob* sender, float iArrgoRange, float iAssistRange) {
	if (!sender || !sender->IsNPC())
		return 0;
	LinkedListIterator<Mob*> iterator(mob_list);
	iterator.Reset();
	float dist;
	//float distZ;
	while(iterator.MoreElements()) {
		Mob* mob = iterator.GetData();

			// Check If it's invisible and if we can see invis
			// Check if it's a client, and that the client is connected and not linkdead,
			//   and that the client isn't Playing an NPC, with thier gm flag on
			// Check if it's not a Interactive NPC
			// Trumpcard: The 1st 3 checks are low cost calcs to filter out unnessecary distance checks. Leave them at the beginning, they are the most likely occurence.
			// Image: I moved this up by itself above faction and distance checks because if one of these return true, theres no reason to go through the other information
			if(   ( abs (mob->GetX() - sender->GetX() ) > iArrgoRange)
			   || ( abs (mob->GetY() - sender->GetY() ) > iArrgoRange)
			   || ( abs (mob->GetZ() - sender->GetZ() ) > iArrgoRange)
			   ||(mob->IsInvisible(sender))
			   || (mob->IsClient() && (!mob->CastToClient()->Connected()
			   || mob->CastToClient()->IsLD()
			   || mob->CastToClient()->IsBecomeNPC()
			   || mob->CastToClient()->GetGM())   
			   ))
			{
				iterator.Advance();
				continue;
			}

			if(sender->GetOwner() != 0 && mob != sender->GetOwner())
			{
				iterator.Advance();
				continue;
			}
			
			dist  = mob->Dist(*sender);
			//distZ = dist - mob->DistNoZ(*sender);
	
			// TC - removing z checks.  Not implemented correctly. distZ will never be less than -10.
			//if( ( dist > (iAssistRange*2) && dist > iArrgoRange )
            //   || ( (distZ <= 0 && distZ < (Z_AGGRO-Z_AGGRO-Z_AGGRO)) 
			//   || (distZ >= 0 && distZ > (Z_AGGRO)) )
            //   )

			if(  ( dist > (iAssistRange*2) ) &&  (dist > iArrgoRange)  )
			{	// Skip it
                     //if (   EQDEBUG >= 5
                     //    && mob->IsClient()
                     //    && mob->CastToClient()->Connected()
                     //    && !mob->CastToClient()->IsLD()
                     //    && dist <= iArrgoRange
                     //   )
                     //     LogFile->write(EQEMuLog::Debug, "Check aggro for %s skipping client %s.", sender->GetName(), mob->GetName());
                     //if (   EQDEBUG >= 5
                     //    && mob->IsNPC()
                     //    && mob->CastToNPC()->IsInteractive()
                     //   )
                     //     LogFile->write(EQEMuLog::Debug, "Check aggro for %s skipping IPC %s.", sender->GetName(), mob->GetName());
			iterator.Advance();
			continue;
		}
		//Image: Get their current target and faction value now that its required
		Mob* mobTarget = mob->GetTarget();
		FACTION_VALUE fv = mob->GetFactionCon(sender);
        // Assist check
        // Check faction amiable or better (friend)
        // Is friend engaged
        // Does friend have a target
        // Is friend in range
        // Are we stupid, or is friends target green
        // Is friends target in range

// solar: i broke these ifs out all ridiculous for debugging, compress em
// if you want but make sure not to take out any parens without understanding
// the order of evaluation completely
	if
		(
			mobTarget
			&&
			(
				( fv <= FACTION_AMIABLE )
			)
			&&
			(
				( mob->IsNPC() && mob->IsEngaged() )
			)
			&&
				dist <= iAssistRange
			&&
			( 
				( mob->GetINT() <= 100 )
				|| ( mobTarget->GetLevelCon(sender->GetLevel()) != CON_GREEN )
			)
		)
		{
			// Had an if statement to check if it wasn't a GM but theres no reason, we check that above
			// Also had an interactive npc check but I believe these are no longer used, if required can be put above
			// Assist friend
			
			//FatherNiwtit: make sure we can see them. last since it is very expensive
			if(sender->CheckLosFN(mobTarget)) {
			
#if EQDEBUG>=5
				LogFile->write(EQEMuLog::Debug, "Check aggro for %s assisting %s, target %s.", sender->GetName(), mob->GetName(), mobTarget->GetName());
#endif
				return mobTarget;
			}
		}
		// Make sure they're still in the zone
		// Are they in range?
		// Are they kos?
		// Are we stupid or are they green
		// and they don't have thier gm flag on
		else if
		(
			mob->InZone()
			&& (dist <= iArrgoRange)
			&&
			(
				(
					fv == FACTION_SCOWLS
					||
					(mob->GetPrimaryFaction() != sender->GetPrimaryFaction() && mob->GetPrimaryFaction() == -4 && sender->GetOwner() == 0)
					||
					(
						fv == FACTION_THREATENLY
						&& (rand()%100) < THREATENLY_ARRGO_CHANCE
					)
				)
			) //Image: Do not tamper with this random code, it is optimized!
			&&
			(
				( sender->GetINT() <= 75 )
				||( mob->GetLevelCon(sender->GetLevel()) != CON_GREEN )
			)
		)
		{
			//FatherNiwtit: make sure we can see them. last since it is very expensive
			if(sender->CheckLosFN(mob)) {
			
			// Aggro
#if EQDEBUG>=5
				LogFile->write(EQEMuLog::Debug, "Check aggro for %s target %s.", sender->GetName(), mob->GetName());
#endif			
				return mob;
			}
	  }
#if EQDEBUG >= 6
		  cout<<"In zone:"<<mob->InZone()<<endl;
		  cout<<"Dist:"<<dist<<endl;
		  cout<<"Range:"<<iArrgoRange<<endl;
		  cout<<"Faction:"<<fv<<endl;
		  cout<<"Int:"<<sender->GetINT()<<endl;
		  cout<<"Con:"<<mob->GetLevelCon(sender->GetLevel())<<endl;
#endif		

		iterator.Advance();
	}
	//LogFile->write(EQEMuLog::Debug, "Check aggro for %s no target.", sender->GetName());
	return 0;
}

void EntityList::AIYellForHelp(Mob* sender, Mob* attacker) {
	if(!sender || !attacker)
		return;
	if (sender->GetPrimaryFaction() == 0 )
		return; // well, if we dont have a faction set, we're gonna be indiff to everybody
	LinkedListIterator<Mob*> iterator(mob_list);
	iterator.Reset();
	while(iterator.MoreElements()) {
		Mob* mob = iterator.GetData();
		float mobDistance= mob->Dist(*sender);

		if (
			mob != sender
			&& mob != attacker
			&& !mob->IsCorpse()
			&& mob->IsAIControlled()
			&& mobDistance <= mob->GetAssistRange()
			)
		{
			if(mob->CastToNPC()->GetPrimaryFaction()==sender->CastToNPC()->GetPrimaryFaction() && attacker->GetLevelCon(mob->GetLevel()) != CON_GREEN){//attacking someone on same faction
#if (EQDEBUG>=5) 
				LogFile->write(EQEMuLog::Debug, "AIYellForHelp(\"%s\",\"%s\") %s attacking %s Dist %f Z %f", 
					sender->GetName(), attacker->GetName(), mob->GetName(), attacker->GetName(), mobDistance, fabs(sender->GetZ()+mob->GetZ()));
#endif
				//Father Nitwit:  make sure we can see them.
				if(mob->CheckLosFN(attacker)) {
					mob->AddToHateList(attacker, 1, 0, false);
				}
			}
		}
		iterator.Advance();
	}
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
				// we have a winner!
				if (caster->AICastSpell(mob, 100, iSpellTypes))
					return true;
			}
		}
		iterator.Advance();
	}
	return false;
}

// returns what Other thinks of this
FACTION_VALUE Client::GetFactionCon(Mob* iOther) {
	if (GetOwnerID()) {
		return GetOwnerOrSelf()->GetFactionCon(iOther);
	}
	iOther = iOther->GetOwnerOrSelf();
	if (iOther->GetPrimaryFaction() < 0)
		return GetSpecialFactionCon(iOther);
	if (iOther->GetPrimaryFaction() == 0)
		return FACTION_INDIFFERENT;

	return GetFactionLevel(this->CharacterID(), 0, this->GetRace(), this->GetClass(), this->GetDeity(), iOther->GetPrimaryFaction(), iOther);
}

FACTION_VALUE NPC::GetFactionCon(Mob* iOther) {

	iOther = iOther->GetOwnerOrSelf();
	int primaryFaction= iOther->GetPrimaryFaction();

	if (primaryFaction < 0)
		return GetSpecialFactionCon(iOther);
	if (primaryFaction == 0)
		return FACTION_INDIFFERENT;
	if (GetOwnerID())
		return GetOwnerOrSelf()->GetFactionCon(iOther);

	sint8 tmp = CheckNPCFactionAlly(primaryFaction);
	if (tmp == 1)
		return FACTION_ALLY;
	else if (tmp == -1)
		return FACTION_SCOWLS;
	return FACTION_INDIFFERENT;
}

FACTION_VALUE Mob::GetSpecialFactionCon(Mob* iOther) {
	
	if (!iOther)
		return FACTION_INDIFFERENT;

	iOther = iOther->GetOwnerOrSelf();
	Mob* self = this->GetOwnerOrSelf();

	bool selfAIcontrolled = self->IsAIControlled();
	bool iOtherAIControlled = iOther->IsAIControlled();
	int selfPrimaryFaction = self->GetPrimaryFaction();
	int iOtherPrimaryFaction = iOther->GetPrimaryFaction();

	if (selfPrimaryFaction >= 0 && selfAIcontrolled)
		return FACTION_INDIFFERENT;
	if (iOther->GetPrimaryFaction() >= 0)
		return FACTION_INDIFFERENT;
/* special values:
	-2 = indiff to player, ally to AI on special values, indiff to AI
	-3 = dub to player, ally to AI on special values, indiff to AI
	-4 = atk to player, ally to AI on special values, indiff to AI
	-5 = indiff to player, indiff to AI
	-6 = dub to player, indiff to AI
	-7 = atk to player, indiff to AI
	-8 = indiff to players, ally to AI on same value, indiff to AI
	-9 = dub to players, ally to AI on same value, indiff to AI
	-10 = atk to players, ally to AI on same value, indiff to AI
	-11 = indiff to players, ally to AI on same value, atk to AI
	-12 = dub to players, ally to AI on same value, atk to AI
	-13 = atk to players, ally to AI on same value, atk to AI
*/
	switch (iOtherPrimaryFaction) {
		case -2: // -2 = indiff to player, ally to AI on special values, indiff to AI
			if (selfAIcontrolled && iOtherAIControlled)
				return FACTION_ALLY;
			else
				return FACTION_INDIFFERENT;
		case -3: // -3 = dub to player, ally to AI on special values, indiff to AI
			if (selfAIcontrolled && iOtherAIControlled)
				return FACTION_ALLY;
			else
				return FACTION_DUBIOUS;
		case -4: // -4 = atk to player, ally to AI on special values, indiff to AI
			if (selfAIcontrolled && iOtherAIControlled)
				return FACTION_ALLY;
			else
				return FACTION_SCOWLS;
		case -5: // -5 = indiff to player, indiff to AI
			return FACTION_INDIFFERENT;
		case -6: // -6 = dub to player, indiff to AI
			if (selfAIcontrolled && iOtherAIControlled)
				return FACTION_INDIFFERENT;
			else
				return FACTION_DUBIOUS;
		case -7: // -7 = atk to player, indiff to AI
			if (selfAIcontrolled && iOtherAIControlled)
				return FACTION_INDIFFERENT;
			else
				return FACTION_SCOWLS;
		case -8: // -8 = indiff to players, ally to AI on same value, indiff to AI
			if (selfAIcontrolled && iOtherAIControlled) {
				if (selfPrimaryFaction == iOtherPrimaryFaction)
					return FACTION_ALLY;
				else
					return FACTION_INDIFFERENT;
			}
			else
				return FACTION_INDIFFERENT;
		case -9: // -9 = dub to players, ally to AI on same value, indiff to AI
			if (selfAIcontrolled && iOtherAIControlled) {
				if (selfPrimaryFaction == iOtherPrimaryFaction)
					return FACTION_ALLY;
				else
					return FACTION_INDIFFERENT;
			}
			else
				return FACTION_DUBIOUS;
		case -10: // -10 = atk to players, ally to AI on same value, indiff to AI
			if (selfAIcontrolled && iOtherAIControlled) {
				if (selfPrimaryFaction == iOtherPrimaryFaction)
					return FACTION_ALLY;
				else
					return FACTION_INDIFFERENT;
			}
			else
				return FACTION_SCOWLS;
		case -11: // -11 = indiff to players, ally to AI on same value, atk to AI
			if (selfAIcontrolled && iOtherAIControlled) {
				if (selfPrimaryFaction == iOtherPrimaryFaction)
					return FACTION_ALLY;
				else
					return FACTION_SCOWLS;
			}
			else
				return FACTION_INDIFFERENT;
		case -12: // -12 = dub to players, ally to AI on same value, atk to AI
			if (selfAIcontrolled && iOtherAIControlled) {
				if (selfPrimaryFaction == iOtherPrimaryFaction)
					return FACTION_ALLY;
				else
					return FACTION_SCOWLS;


			}
			else
				return FACTION_DUBIOUS;
		case -13: // -13 = atk to players, ally to AI on same value, atk to AI
			if (selfAIcontrolled && iOtherAIControlled) {
				if (selfPrimaryFaction == iOtherPrimaryFaction)
					return FACTION_ALLY;
				else
					return FACTION_SCOWLS;
			}
			else
				return FACTION_SCOWLS;
		default:
			return FACTION_INDIFFERENT;
	}
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
	if (this->isgrouped && entity_list.GetGroupByClient(this) != NULL)
    {
		Group* group = entity_list.GetGroupByClient(this);
		group->DelMember(CastToMob(),true);
    }

	if (AIspells[0].spellid == 0)
		AIautocastspell_timer->Disable();
	SaveSpawnSpot();
	pClientSideTarget = target ? target->GetID() : 0;
	SendAppearancePacket(AT_Anim, ANIM_FREEZE);	// this freezes the client
	SendAppearancePacket(AT_Linkdead, 1); // Sending LD packet so *LD* appears by the player name when charmed/feared -Kasai
	attack_timer->Enable();
	attack_timer_dw->Enable();
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
		attack_timer->Disable();
		attack_timer_dw->Disable();
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

	if (!(AIthink_timer->Check() || attack_timer->Check(false)))
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

        if (CombatRange(target)) 
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
				
			if (attack_timer->Check()) 
			{
				Attack(target, 13);
				if (target) 
				{
					if (CanThisClassDoubleAttack()) 
					{
						sint32 RandRoll = rand()%100;
						if (RandRoll < (GetLevel() + 20))  
						{
							if(target)
							{
								if (Attack(target, 13)) 
								{
								// lets see if we can do a triple attack with the main hand
								if (SpecAttacks[SPECATK_TRIPLE]) 
								{
									if (!GetOwner() && RandRoll < (GetLevel()))  
									{
									if(target)
									{
										if (Attack(target, 13)) 
										{	// now lets check the quad attack
											if (SpecAttacks[SPECATK_QUAD]) 
											{
												if (!GetOwner() && RandRoll < (GetLevel() - 20))  
												{
            										if(target)
													{
														Attack(target, 13);
													}
												}
											} // if (SpecAttacks[SPECATK_QUAD])
										}
									}
								}
							} // if (SpecAttacks[SPECATK_TRIPLE])
						}
					}
				} // if (CanThisClassDoubleAttack())
			}
		}

				if (SpecAttacks[SPECATK_FLURRY])
					Flurry();

				if (SpecAttacks[SPECATK_RAMPAGE])
					Rampage();
			}
			if (target && attack_timer_dw->Check() && CanThisClassDualWield()) 
			{
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
			if(!target) return;
// TODO: Check special attacks (backstab, etc) here
			if (AIautocastspell_timer->Check()) 
			{
#if MobAI_DEBUG_Spells >= 25
				cout << "Engaged autocast check triggered: " << this->GetName() << endl;
#endif
				if (!AICastSpell(this, 100, SpellType_Heal | SpellType_Escape)) // try casting a heal or gate
					if (!entity_list.AICheckCloseSpells(this, 25, MobAISpellRange, SpellType_Heal)) // try casting a heal on nearby
						AICastSpell(target, 20, SpellType_Nuke | SpellType_Lifetap | SpellType_DOT);
			}
		}
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
						float dist = Dist(*owner);
						if (dist >= 10) 
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
					float dist = Dist(*follow);
					if (dist >= 10) 
					{
						float speed = GetWalkspeed();
						if (dist >= 25)
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
				}
			}

			else 
			{
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
						else if (!CalculateNewPosition2(roambox_movingto_x, roambox_movingto_y, GetZ(), GetWalkspeed())) 
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

								gridno = this->CastToNPC()->GetGrid(); 

// handle quest command roamers with no grids too
						if (gridno > 0 || cur_wp==-2) 

                  { 
                     if (movetimercompleted==true)    
                     {  // time to pause at wp is over
// MYRA - Added code to depop at end of grid for wander type 4
								if (wandertype == 4 && cur_wp == max_wp)
				                { 
						           this->CastToNPC()->Depop(); 
								} 
                        else 
                        { 
                              movetimercompleted=false; 
                              char temp[100]; 
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
									CalculateNewPosition2(cur_wp_x, cur_wp_y, cur_wp_z, GetWalkspeed()); 
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

void Mob::AI_SetRoambox(float iDist, float iRoamDist, int32 iDelay) {
	AI_SetRoambox(iDist, GetX()+iRoamDist, GetX()-iRoamDist, GetY()+iRoamDist, GetY()-iRoamDist, iDelay);
}

void Mob::AI_SetRoambox(float iDist, float iMaxX, float iMinX, float iMaxY, float iMinY, int32 iDelay) {
	roambox_distance = iDist;
	roambox_max_x = iMaxX;
	roambox_min_x = iMinX;
	roambox_max_y = iMaxY;
	roambox_min_y = iMinY;
	roambox_movingto_x = roambox_max_x + 1; // this will trigger a recalc
	roambox_delay = iDelay;
}


// support for new wandering quest commands

void Mob::StopWandering()
{	// stops a mob from wandering, takes him off grid and sends him back to spawn point
	roamer=false;
	this->CastToNPC()->SetGrid(0);
	SendPosition();
	return;
}

void Mob::ResumeWandering()
{	// causes wandering to continue - overrides waypoint pause timer and PauseWandering()
	if (this->CastToNPC()->GetGrid() != 0)
	{
		if (this->CastToNPC()->GetGrid() < 0)
		{	// we were paused by a quest
			AIwalking_timer->Disable();
			this->CastToNPC()->SetGrid( 0 - this->CastToNPC()->GetGrid());
			if (cur_wp==-1)
			{	// got here by a MoveTo()
				cur_wp=save_wp;
				UpdateWaypoint(cur_wp);	// have him head to last destination from here
			}
		}
		else if (AIwalking_timer->Enabled())
		{	// we are at a waypoint paused normally
			AIwalking_timer->Disable();	// disable timer to end pause now
		}
		else
		{
			cout << "NPC not paused - can't resume wandering: " << this->GetNPCTypeID() << endl;
			return;
		}
		if (cur_wp_x == GetX() && cur_wp_y == GetY()) 
		{	// are we we at a waypoint? if so, trigger event and start to next
			char temp[100]; 
			parse->Event(EVENT_WAYPOINT,this->GetNPCTypeID(), itoa(cur_wp,temp,10), this->CastToMob(), 0); 
			CalculateNewWaypoint(); 
	        SetAppearance(0, false); 
		}	// if not currently at a waypoint, we continue on to the one we were headed to before the stop
	}
	else
	{
		cout << "NPC not on grid - can't resume wandering: " << this->GetNPCTypeID() << endl;
	}
	return;
}

void Mob::PauseWandering(int pausetime)
{	// causes wandering to stop but is resumable
	// 0 pausetime means pause until resumed
	// otherwise automatically resume when time is up
	if (this->CastToNPC()->GetGrid() != 0)
	{
		SendPosition();
		if (pausetime<1)
		{	// negative grid number stops him dead in his tracks until ResumeWandering()
			this->CastToNPC()->SetGrid( 0 - this->CastToNPC()->GetGrid());
		}
		else
		{	// specified waiting time, he'll resume after that
			AIwalking_timer->Start(pausetime*1000); // set the timer
		}
	}
	else
	{
		cout << "NPC not on grid, can't pause wandering: " << this->GetNPCTypeID() << "...\n";
	}
	return;
}

void Mob::MoveTo(float mtx, float mty, float mtz)
{	// makes mob walk to specified location
	if (this->CastToNPC()->GetGrid() != 0)
	{	// he is on a grid
		if (this->CastToNPC()->GetGrid() < 0)
		{	// currently stopped by a quest command
			this->CastToNPC()->SetGrid( 0 - this->CastToNPC()->GetGrid());	// get him moving again
		}
		AIwalking_timer->Disable();	// disable timer in case he is paused at a wp
		if (cur_wp>=0)
		{	// we've not already done a MoveTo()
			save_wp=cur_wp;	// save the current waypoint
			cur_wp=-1;		// flag this move as quest controlled
		}
	}
	else
	{	// not on a grid
		roamer=true;
		save_wp=0;
		cur_wp=-2;		// flag as quest controlled w/no grid
	}
	cur_wp_x = mtx;
	cur_wp_y = mty;
	cur_wp_z = mtz;
	cur_wp_pause = 0;
}



void Mob::UpdateWaypoint(int wp_index)
{
	MyListItem <wplist> * Ptr = Waypoints.First;
	while (Ptr) {
		if ( Ptr->Data->index == wp_index) {
				cur_wp_x = Ptr->Data->x;
				cur_wp_y = Ptr->Data->y;
				cur_wp_z = Ptr->Data->z;
				cur_wp_pause = Ptr->Data->pause;
				break;
		}
		Ptr = Ptr->Next;
	}
	return;
}

void Mob::CalculateNewWaypoint()
{
//	int8 max_wp = wp_a[0];
//	int8 wandertype = wp_a[1];
//	int8 pausetype = wp_a[2];
//	int8 cur_wp = wp_a[3];

	int16 ranmax = cur_wp;
	int16 ranmax2 = max_wp - cur_wp;
	int old_wp = cur_wp;

	bool reached_end = false;
	bool reached_beginning = false;

// handle quest mob wandering control
	if (cur_wp <0)
	{	// under quest control, so no new wp - just stop
// printf("cur_wp<0\n");
		if (cur_wp==-1)
		{	// mob is on a grid			
			this->CastToNPC()->SetGrid( 0 - this->CastToNPC()->GetGrid());
		}
		else
		{	// mob is not on a grid
			cur_wp=0;
		}
		return;
	}

	//Determine if we're at the last/first waypoint
	if (cur_wp == max_wp)
		reached_end = true;
	if (cur_wp == 0)
		reached_beginning = true;

	//Declare which waypoint to go to
	switch (wandertype)
	{
	case 0: //Circular
		if (reached_end)
			cur_wp = 0;
		else
			cur_wp = cur_wp + 1;
		break;
	case 1: //Random 5
		if (ranmax > 5)
			ranmax = 5;
		if (ranmax2 > 5)
			ranmax2 = 5;
		cur_wp = cur_wp + rand()%(ranmax+1) - rand()%(ranmax2+1);
		break;
	case 2: //Random
			cur_wp = (rand()%max_wp) + (rand()%2);
		break;
	case 3: //Patrol
		if (reached_end)
			patrol = 1;
		else if (reached_beginning)
			patrol = 0;
		if (patrol == 1)
			cur_wp = cur_wp - 1;
		else
			cur_wp = cur_wp + 1;
		break;
// MYRA - Added wander type 4 (single run)
		case 4:  // single run 
			cur_wp = cur_wp + 1; 
		break;
// end Myra
	}
	tar_ndx=52;		// force new packet to be sent extra 2 times

	// Check to see if we need to update the waypoint. - Wes
	if (cur_wp != old_wp)
		UpdateWaypoint(cur_wp);

}

void Mob::SetWaypointPause() 
{ 
   //Declare time to wait on current WP 
    
   if (cur_wp_pause == 0) { 
      AIwalking_timer->Start(100); 
   } 
   else 
   { 
       
      switch (pausetype) 
      { 
      case 0: //Random Half 
         AIwalking_timer->Start((cur_wp_pause - rand()%cur_wp_pause/2)*1000); 
         break; 
      case 1: //Full 
         AIwalking_timer->Start(cur_wp_pause*1000); 
         break; 
      case 2: //Random Full 
         AIwalking_timer->Start((rand()%cur_wp_pause)*1000); 
         break; 
      } 
   } 
} 




float Mob::CalculateDistanceToNextWaypoint() {
    return CalculateDistance(cur_wp_x, cur_wp_y, cur_wp_z);
}

float Mob::CalculateDistance(float x, float y, float z) {
    return (float)sqrt( ((x_pos-x)*(x_pos-x)) + ((y_pos-y)*(y_pos-y)) + ((z_pos-z)*(z_pos-z)) );
}


int8 Mob::CalculateHeadingToNextWaypoint() {
    return CalculateHeadingToTarget(cur_wp_x, cur_wp_y);
}

sint8 Mob::CalculateHeadingToTarget(float in_x, float in_y) {
	float angle;

	if (in_x-x_pos > 0)
		angle = - 90 + atan((double)(in_y-y_pos) / (double)(in_x-x_pos)) * 180 / M_PI;
	else if (in_x-x_pos < 0)
		angle = + 90 + atan((double)(in_y-y_pos) / (double)(in_x-x_pos)) * 180 / M_PI;
	else // Added?
	{
		if (in_y-y_pos > 0)
			angle = 0;
		else
			angle = 180;
	}
	if (angle < 0)
		angle += 360;
	if (angle > 360)
		angle -= 360;
	return (sint8) (256*(360-angle)/360.0f);
}
bool Mob::CalculateNewPosition2(float x, float y, float z, float speed) {
	if(GetID()==0)
		return true;
	if ((x_pos-x == 0) && (y_pos-y == 0) && (z_pos-z == 0))//spawn is at target coords
		return false;

	if(tar_ndx<20 && tarx==x && tary==y && tarz==z){
		x_pos = x_pos + tar_vx*tar_vector;
		y_pos = y_pos + tar_vy*tar_vector;
		z_pos = z_pos + tar_vz*tar_vector;
		tar_ndx++;
		return true;
	}
	else{
		if (tar_ndx>50)
		{
			tar_ndx--;
		}
		else
		{
			tar_ndx=0;
		}
		tarx=x;
		tary=y;
		tarz=z;
	}

	float nx = this->x_pos;
    float ny = this->y_pos;
    float nz = this->z_pos;
//	float nh = this->heading;
	
	tar_vx = x - nx;
	tar_vy = y - ny;
	tar_vz = z - nz;

	pRunAnimSpeed = (sint8)(speed*36);
	speed *= 46;
	// --------------------------------------------------------------------------
	// 2: get unit vector
	// --------------------------------------------------------------------------
	float mag = sqrt (tar_vx*tar_vx + tar_vy*tar_vy + tar_vz*tar_vz);
	tar_vector = speed / mag;

// mob move fix
	int numsteps = (int) ( mag * 20 / speed) + 1;


// mob move fix

	if (numsteps<20)
	{
		if (numsteps>1)
		{
			tar_vector=1.0f				;
			tar_vx = tar_vx/numsteps;
			tar_vy = tar_vy/numsteps;
			tar_vz = tar_vz/numsteps;
			x_pos = x_pos + tar_vx;
			y_pos = y_pos + tar_vy;
			z_pos = z_pos + tar_vz;
			tar_ndx=22-numsteps;
			heading = CalculateHeadingToTarget(x, y);
		}
	    else
		{
			x_pos = x;
			y_pos = y;
			z_pos = z;
		}
	}

	else {
		tar_vector/=20;
		x_pos = x_pos + tar_vx*tar_vector;
		y_pos = y_pos + tar_vy*tar_vector;
		z_pos = z_pos + tar_vz*tar_vector;
		heading = CalculateHeadingToTarget(x, y);
	}
	this->SetMoving(true);
	moved=true;
	delta_x=x_pos-nx;
	delta_y=y_pos-ny;
	delta_z=z_pos-nz;
	delta_heading=0;
	SendPosUpdate();
	SetAppearance(0, false);
    pLastChange = Timer::GetCurrentTime();
    return true;
}
bool Mob::CalculateNewPosition(float x, float y, float z, float speed) {
	if(GetID()==0)
		return true;
	/*if(tar_ndx<20 && tarx==x && tary==y && tarz==z){
		tar_ndx++;
		x_pos = x_pos + tar_vx*tar_vector;
		y_pos = y_pos + tar_vy*tar_vector;
		z_pos = z_pos + tar_vz*tar_vector;
		return true;
	}
	else{
		tar_ndx=0;
		tarx=x;
		tary=y;
		tarz=z;
	}*/
    float nx = x_pos;
    float ny = y_pos;
    float nz = z_pos;
//	float nh = heading;
	
    // if NPC is rooted
    if (speed == 0.0) {
        SetHeading(CalculateHeadingToTarget(x, y));
		if(moved){
			SendPosition();
			SetMoving(false);
			moved=false;
		}
		SetRunAnimSpeed(0);
        return true;
    }
		float old_test_vector=test_vector;
		tar_vx = x - nx;
		tar_vy = y - ny;
		tar_vz = z - nz;

		if (tar_vx == 0 && tar_vy == 0 && tar_vz == 0)
			return false;
		pRunAnimSpeed = (int8)(speed*43);
		speed *= 2.8;
		// --------------------------------------------------------------------------
		// 2: get unit vector
		// --------------------------------------------------------------------------
		test_vector=sqrt (x*x + y*y + z*z);
		tar_vector = speed / sqrt (tar_vx*tar_vx + tar_vy*tar_vy + tar_vz*tar_vz);
		heading = CalculateHeadingToTarget(x, y);

		if (tar_vector >= 1.0) {
			x_pos = x;
			y_pos = y;
			z_pos = z;
		}
		else {
			x_pos = x_pos + tar_vx*tar_vector;
			y_pos = y_pos + tar_vy*tar_vector;
			z_pos = z_pos + tar_vz*tar_vector;
		}
		//OP_MobUpdate
		if((old_test_vector!=test_vector) || tar_ndx>20){ //send update
			tar_ndx=0;
			this->SetMoving(true);
			moved=true;
			delta_x=(x_pos-nx);
			delta_y=(y_pos-ny);
			delta_z=(z_pos-nz);
			delta_heading=0;//(heading-nh)*8;
			SendPosUpdate();
		}
		tar_ndx++;
    // now get new heading
	SetAppearance(0, false); // make sure they're standing
    pLastChange = Timer::GetCurrentTime();
    return true;
}

void Mob::AssignWaypoints(int16 grid)
{ char errbuf[MYSQL_ERRMSG_SIZE];
  char *query = 0;
  MYSQL_RES *result;
  MYSQL_ROW row;

  bool	GridErr = false,
	WPErr = false;		// Will be set true if any errors encountered while querying the waypoints


	Waypoints.ClearListAndData();
#ifdef _EQDEBUG
	cout<<"Assigning waypoints for grid "<<grid<<" to "<<name<<"...\n";
#endif

	// Retrieve the wander and pause types for this grid
	if(database.RunQuery(query,MakeAnyLenString(&query,"SELECT `type`,`type2` FROM `grid` WHERE `id`=%i AND `zoneid`=%i",grid,zone->GetZoneID()),errbuf, &result))
	{   if((row = mysql_fetch_row(result)))
	    {	if(row[0] != 0)		wandertype = atoi(row[0]);
		else			wandertype = 0;
		if(row[1] != 0)		pausetype = atoi(row[1]);
		else			pausetype = 0;
	    }
	    else	// No grid record found in this zone for the given ID
		GridErr = true;
	    mysql_free_result(result);
	}
	else	// DB query error!
	{   GridErr = true;
	    cerr << "MySQL Error while trying to assign grid " << grid << " to mob " << name << "!\nThe error message was: " << errbuf << "\n\n";
	}
	safe_delete_array(query);

	if(!GridErr)
	{   this->CastToNPC()->SetGrid(grid);	// Assign grid number
	    adverrorinfo = 7561;

	    // Retrieve all waypoints for this grid
	    if(database.RunQuery(query,MakeAnyLenString(&query,"SELECT `x`,`y`,`z`,`pause` FROM grid_entries WHERE `gridid`=%i AND `zoneid`=%i ORDER BY `number`",grid,zone->GetZoneID()),errbuf,&result))
	    {	roamer = true;
		max_wp = -1;	// Initialize it; will increment it for each waypoint successfully added to the list
		adverrorinfo = 7564;

		while((row = mysql_fetch_row(result)))
		{   
		    if(row[0]!=0 && row[1]!=0 && row[2]!=0 && row[3]!=0)
		    {	wplist* newwp = new wplist;
			newwp->index = ++max_wp;
			newwp->x = atof(row[0]);
			newwp->y = atof(row[1]);
			newwp->z = atof(row[2]);
			newwp->pause = atoi(row[3]);
			Waypoints.AddItem(newwp);
		    }
		}
		mysql_free_result(result);
	    }
	    else	// DB query error!
	    {	WPErr = true;
		cerr << "MySQL Error while trying to assign waypoints from grid " << grid << " to mob " << name << "!\nThe error message was: " << errbuf << "\n\n";
	    }
	    safe_delete_array(query);
	} // end if (!GridErr)


#ifdef _EQDEBUG
	cout<<" done."<<endl;
#endif
	if(!GridErr && !WPErr)
	{   UpdateWaypoint(0);
	    SetWaypointPause();
	    this->SendTo(cur_wp_x, cur_wp_y, cur_wp_z);
	    if (wandertype == 1 || wandertype == 2)
		CalculateNewWaypoint();
	}
}

void Mob::SendTo(float new_x, float new_y, float new_z) {
	if (zone->map == 0)


		return;
	
//	float angle;
//	float dx = new_x-x_pos;
//	float dy = new_y-y_pos;
	// 0.09 is a perfect magic number for a human pnj's
//	AIwalking_timer->Start((int32) ( sqrt( dx*dx + dy*dy ) * 0.09f ) * 1000 );
	
/*	if (new_x-x_pos > 0)
		angle = - 90 + atan((double)(new_y-y_pos) / (double)(new_x-x_pos)) * 180 / M_PI;
	else {
		if (new_x-x_pos < 0)	
			angle = + 90 + atan((double)(new_y-y_pos) / (double)(new_x-x_pos)) * 180 / M_PI;
		else { // Added?
			if (new_y-y_pos > 0)
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
	SetRunAnimSpeed(5);*/
	//	SendPosUpdate();
	x_pos = new_x;
	y_pos = new_y;
	
//	PNODE pnode  = zone->map->SeekNode( zone->map->GetRoot(), y_pos, x_pos );
//I believe this is contributing to breaking mob spawns when a map is loaded
	NodeRef pnode = NODE_NONE;
	// Quagmire - Not sure if this is the right thing to do, but it stops the crashing
	if (pnode == NODE_NONE) return;
	
	int  *iface  = zone->map->SeekFace( pnode, y_pos, x_pos );
	float tmp_z = 0;
	float best_z = 999999;
	while(*iface != -1) {
		tmp_z = zone->map->GetFaceHeight( *iface, y_pos, x_pos );
		if (abs((int)(tmp_z - new_z)) <= abs((int)(best_z - new_z)))
			best_z = tmp_z;
		iface++;
	}
	z_pos = best_z + 0.1f;
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
    // perhaps get the values from the db?
    if (rand()%100 < 80)
        return false;
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
    // perhaps get the values from the db?
    if (rand()%100 < 80)
        return false;

    entity_list.MessageClose(this, true, 600, 13, "%s goes on a RAMPAGE!", GetName());
    for (int i = 0; i < MAX_RAMPAGE_TARGETS; i++)
    {
        // range is important
        if (strlen(RampageArray[i]) > 0 && entity_list.GetMob(RampageArray[i]) )
        {
            Mob *target = entity_list.GetMob(RampageArray[i]);
            if (CombatRange(target))
                Attack(target);
        }
    }
    return true;
}

int32 Mob::GetLevelCon(int8 iOtherLevel) {
	if(this==0)
		return CON_GREEN;
    sint16 diff = iOtherLevel - GetLevel();
	int32 conlevel=0;
	
    if (diff == 0)
        return CON_WHITE;
    else if (diff >= 1 && diff <= 2)
        return CON_YELLOW;
    else if (diff >= 3)
        return CON_RED;

    if (GetLevel() <= 6)    // i didnt notice light blue mobs before level 6
    {
        if (diff <= -4)
            conlevel = CON_GREEN;
        else
            conlevel = CON_BLUE;
    }
    else if (GetLevel() <= 9)
	{
        if (diff <= -5)
            conlevel = CON_GREEN;
        else if (diff <= -4)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
    else if (GetLevel() <= 13)
	{
        if (diff <= -6)
            conlevel = CON_GREEN;
        else if (diff <= -5)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (GetLevel() <= 18)
	{
        if (diff <= -7)
            conlevel = CON_GREEN;
        else if (diff <= -6)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (GetLevel() <= 24)
	{
        if (diff <= -8)
            conlevel = CON_GREEN;
        else if (diff <= -7)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (GetLevel() <= 35)
	{
        if (diff <= -9)
            conlevel = CON_GREEN;
        else if (diff <= -8)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (GetLevel() <= 44)
	{
        if (diff <= -12)
            conlevel = CON_GREEN;
        else if (diff <= -11)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (GetLevel() <= 50)
	{
        if (diff <= -14)
            conlevel = CON_GREEN;
        else if (diff <= -12)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (GetLevel() <= 60)
	{
        if (diff <= -16)
            conlevel = CON_GREEN;
        else if (diff <= -14)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (GetLevel() >= 61)
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

// returns 1 if they're allies, -1 if they're enimies, 0 if they dont care either way
sint8 NPC::CheckNPCFactionAlly(sint32 other_faction) {
	LinkedListIterator<struct NPCFaction*> fac_iteratorcur(faction_list);
	fac_iteratorcur.Reset();

	while(fac_iteratorcur.MoreElements()) {
		NPCFaction* fac = fac_iteratorcur.GetData();
		if ((sint32)fac->factionID == other_faction) {
			if (fac->value_mod < 0)
				return 1;
			else if (fac->value_mod > 0)
				return -1;
			else
				return 0;
		}

		fac_iteratorcur.Advance();
	}
	return 0;
}

