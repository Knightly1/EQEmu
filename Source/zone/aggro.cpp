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
#include <stdlib.h>
#include <math.h>
#include "masterentity.h"
#include "faction.h"
#include "map.h"
#include "spdat.h"
#include "../common/skills.h"
#include "StringIDs.h"

//#define LOSDEBUG 6

Mob* EntityList::AICheckCloseArrgo(Mob* sender, float iArrgoRange, float iAssistRange) {
	if (!sender || !sender->IsNPC())
		return 0;
	LinkedListIterator<Mob*> iterator(mob_list);
	iterator.Reset();
	float dist2;
	float iArrgoRange2 = iArrgoRange*iArrgoRange;
	float iAssistRange2 = iAssistRange*iAssistRange;
	//float distZ;
	while(iterator.MoreElements()) {
		Mob* mob = iterator.GetData();

			// Check If it's invisible and if we can see invis
			// Check if it's a client, and that the client is connected and not linkdead,
			//   and that the client isn't Playing an NPC, with thier gm flag on
			// Check if it's not a Interactive NPC
			// Trumpcard: The 1st 3 checks are low cost calcs to filter out unnessecary distance checks. Leave them at the beginning, they are the most likely occurence.
			// Image: I moved this up by itself above faction and distance checks because if one of these return true, theres no reason to go through the other information
			float t1, t2, t3;
			t1 = mob->GetX() - sender->GetX();
			t2 = mob->GetY() - sender->GetY();
			t3 = mob->GetZ() - sender->GetZ();
			if(t1 < 0)
				t1 = 0 - t1;
			if(t2 < 0)
				t2 = 0 - t2;
			if(t3 < 0)
				t3 = 0 - t3;
			if(   ( t1 > iArrgoRange)
			   || ( t2 > iArrgoRange)
			   || ( t3 > iArrgoRange)
			   ||(mob->IsInvisible(sender))
			   || (mob->IsClient() &&
			       (!mob->CastToClient()->Connected()
			  	    || mob->CastToClient()->IsLD()
			        || mob->CastToClient()->IsBecomeNPC()
			        || mob->CastToClient()->GetGM())   
			   || mob == sender  
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
			
			dist2  = mob->DistNoRoot(*sender);
			//distZ = dist - mob->DistNoZ(*sender);
	
			// TC - removing z checks.  Not implemented correctly. distZ will never be less than -10.
			//if( ( dist > (iAssistRange*2) && dist > iArrgoRange )
            //   || ( (distZ <= 0 && distZ < (Z_AGGRO-Z_AGGRO-Z_AGGRO)) 
			//   || (distZ >= 0 && distZ > (Z_AGGRO)) )
            //   )

			if(  ( dist2 > (iAssistRange2) ) &&  (dist2 > iArrgoRange2)  )
			{	// Skip it, out of range
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
        // Are we stupid, or is friends target not green
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
				dist2 <= iAssistRange2
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
			&& (dist2 <= iArrgoRange2)
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
		  cout<<"Is In zone?:"<<mob->InZone()<<endl;
		  cout<<"Dist^2:"<<dist2<<endl;
		  cout<<"Range^2:"<<iArrgoRange2<<endl;
		  cout<<"Faction:"<<fv<<endl;
		  cout<<"Int:"<<sender->GetINT()<<endl;
		  cout<<"Con:"<<sender->GetLevelCon(mob->GetLevel())<<endl;
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

/*
solar: returns false if attack should not be allowed
I try to list every type of conflict that's possible here, so it's easy
to see how the decision is made.  Yea, it could be condensed and made
faster, but I'm doing it this way to make it readable and easy to modify
*/

///////////////////////////////////////////////////////////////////////////////
//////////// real one is below this function, this is all ifdef'd GW //////////
///////////////////////////////////////////////////////////////////////////////
#ifdef GUILDWARS
bool Mob::IsAttackAllowed(Mob *target)
{
	if(!target)
        return 0;
	if(target->IsNPC() && target->CastToNPC()->GetOwner() != 0 && this == target->CastToNPC()->GetOwner())
		return false; // People killing charmed pets for XP
	if(target->IsClient() && target->CastToClient()->GetOwner() != 0 && this == target->CastToClient()->GetOwner())
		return false; // People killing charmed pets for XP

	/* self */
#ifdef GUILDWARS
	if((target->IsClient() && target->CastToClient()->GetGM()) || this == target) {
		return false; // GMs cant get attacked

	}
#else
	if(this == target) {
		return true; // Quag: sure, you can attack yourself, why not?
	}
#endif
	if (this->IsCorpse() || target->IsCorpse())
		return false;

//Begin of GuildWars rules
#ifndef GUILDWARS
goto normalrules;
#endif

#ifdef GUILDWARS

if(guildwars.GetPVPAbility() == 1)
return false;
else if(guildwars.GetPVPAbility() == 2 && ((_CLIENT(this) && _CLIENT(target)) || (_CLIENT(this) && _CLIENTPET(target)) || (_CLIENTPET(this) && _CLIENT(target)) || (_CLIENTPET(this) && _CLIENTPET(target))))
return false;
else if(guildwars.GetPVPAbility() == 3)
return guildwars.SpecialAttackPrivs(this,target);
else if(target->IsClient() && target->CastToClient()->GuildDBID() == 0 && IsClient() && target->CastToClient()->GuildDBID() == 0)
goto normalrules;
#endif

#ifdef GUILDWARS
	if(_CLIENT(this))
	{
		if(_CLIENT(target))
		{
		if(CastToClient()->GuildDBID() != target->CastToClient()->GuildDBID())
			return true;
		}
		else if(_CLIENTPET(target))
		{
		if(CastToClient()->GuildDBID() != target->GetOwner()->CastToClient()->GuildDBID())
			return true;
		}
	}
	else if(_CLIENTPET(this)) {
		if(_CLIENT(target))
		{
		if(GetOwner()->CastToClient()->GuildDBID() != target->CastToClient()->GuildDBID())
			return true;
		}
		else if(_CLIENTPET(target)) {
		if(GetOwner()->CastToClient()->GuildDBID() != target->GetOwner()->CastToClient()->GuildDBID())
			return true;
		}
	}
#endif
//End of Guildwars rules

normalrules:
	if(_CLIENT(this))
	{
		if(_CLIENT(target))
		{
			if(this->CastToClient()->GetPVP() != target->CastToClient()->GetPVP())
				return false;
			else if(this->CastToClient()->GetPVP() && target->CastToClient()->GetPVP())
				return true;
			else if(this->CastToClient()->GetDuelTarget() == target->GetID() && target->CastToClient()->GetDuelTarget() == GetID() && target->CastToClient()->IsDueling() && this->CastToClient()->IsDueling())
				return true;
		}
		else if(_NPC(target))
		{
			#ifdef IPC
			if(target->CastToNPC()->IsInteractive() && (!target->CastToNPC()->IsPVP() || !this->CastToClient()->GetPVP()))
				return false;
			#endif
				return true;
		}
		else if(_BECOMENPC(target))
		{
			if(this->CastToClient()->GetLevel() > target->CastToClient()->GetBecomeNPCLevel())
				return false;
			else
				return true;
		}
		else if(_CLIENTPET(target))
		{
			if(this->CastToClient()->GetDuelTarget() == target->CastToMob()->GetOwner()->GetID() && target->CastToMob()->GetOwner()->CastToClient()->GetDuelTarget() == GetID() && target->CastToMob()->GetOwner()->CastToClient()->IsDueling() && this->CastToClient()->IsDueling())
				return true;
			else if(this->CastToClient()->GetPVP() != target->CastToMob()->GetOwner()->CastToClient()->GetPVP())
				return false;
			else if(this->CastToClient()->GetPVP() && target->CastToMob()->GetOwner()->CastToClient()->GetPVP())
				return true;
		}
		else if(_NPCPET(target))
		{
			return true;
		}
		else if(_BECOMENPCPET(target))
		{
			if(this->CastToClient()->GetLevel() > target->CastToMob()->GetOwner()->CastToClient()->GetBecomeNPCLevel())
				return false;
			else
				return true;
		}
		else
		{
			goto nocase;
		}
	}
	else if(_NPC(this))
	{
		if(_CLIENT(target))
		{
			#ifdef IPC
			if(CastToNPC()->IsInteractive() && (!CastToNPC()->IsPVP() || !target->CastToClient()->GetPVP()))
				return false;
			#endif
			return true;
		}
		else if(_NPC(target))
		{
			if(target->CastToNPC()->GetPrimaryFaction() != 0 && target->CastToNPC()->GetPrimaryFaction() == CastToNPC()->GetPrimaryFaction())
				return false;
			if(CastToNPC()->GetPrimaryFaction() != 0 && target->CastToNPC()->GetPrimaryFaction() != 0 && CastToNPC()->GetPrimaryFaction() == target->CastToNPC()->GetPrimaryFaction() || CastToNPC()->IsFactionListAlly(target->CastToNPC()->GetPrimaryFaction()))
				return false;


			return 1;
		}
		else if(_BECOMENPC(target))
		{

			return 1;
		}
		else if(_CLIENTPET(target))
		{
			return 1;
		}
		else if(_NPCPET(target))
		{
			return 1;
		}
		else if(_BECOMENPCPET(target))
		{
			return 1;
		}
		else
		{
			goto nocase;
		}
	}
	else if(_BECOMENPC(this))
	{
		if(_CLIENT(target))
		{
			if(target->CastToClient()->GetLevel() > this->CastToClient()->GetBecomeNPCLevel())
				return 0;
			else
				return 1;
		}
		else if(_NPC(target))
		{
			return 1;
		}
		else if(_BECOMENPC(target))
		{
			return 1;
		}
		else if(_CLIENTPET(target)) {
			if(target->CastToMob()->GetOwner()->CastToClient()->GetLevel() > this->CastToClient()->GetBecomeNPCLevel())
				return 0;
			else
				return 1;
		}
		else if(_NPCPET(target)) {
			return 1;
		}
		else if(_BECOMENPCPET(target)) {
			return 1;
		}
		else {
			goto nocase;
		}
	}
	else if(_CLIENTPET(this)) {
		if(_CLIENT(target)) {
			if(this->CastToMob()->GetOwner()->CastToClient()->IsDueling() && this->CastToMob()->GetOwner()->CastToClient()->GetDuelTarget() == target->GetID() && target->CastToClient()->IsDueling() && target->CastToClient()->GetDuelTarget() == this->CastToMob()->GetOwner()->GetID())
				return 1;
			else if(this->CastToMob()->GetOwner()->CastToClient()->GetPVP() != target->CastToClient()->GetPVP())
				return 0;
			else if(this->CastToMob()->GetOwner()->CastToClient()->GetPVP() && target->CastToClient()->GetPVP())
				return 1;
		}
		else if(_NPC(target)) {
			return 1;

		}
		else if(_BECOMENPC(target)) {
			if(this->CastToMob()->GetOwner()->CastToClient()->GetLevel() > target->CastToClient()->GetBecomeNPCLevel())
				return 0;
			else
				return 1;
		}
		else if(_CLIENTPET(target)) {
			if(this->CastToMob()->GetOwner()->CastToClient()->IsDueling() && this->CastToMob()->GetOwner()->CastToClient()->GetDuelTarget() == target->CastToMob()->GetOwner()->GetID() && target->CastToMob()->GetOwner()->CastToClient()->IsDueling() && target->CastToMob()->GetOwner()->CastToClient()->GetDuelTarget() == this->CastToMob()->GetOwner()->GetID())
				return 1;
			else if(this->CastToMob()->GetOwner()->CastToClient()->GetPVP() != target->CastToMob()->GetOwner()->CastToClient()->GetPVP())
				return 0;
			else if(this->CastToMob()->GetOwner()->CastToClient()->GetPVP() && target->CastToMob()->GetOwner()->CastToClient()->GetPVP())
				return 1;
		}
		else if(_NPCPET(target))
		{
			return 1;
		}
		else if(_BECOMENPCPET(target))
		{
			if(this->CastToMob()->GetOwner()->CastToClient()->GetLevel() > target->CastToMob()->GetOwner()->CastToClient()->GetBecomeNPCLevel())
				return 0;
			else
				return 1;
		}
		else
		{
			goto nocase;
		}
	}
	else if(_NPCPET(this))
	{
		if(_CLIENT(target))
		{
			return 1;
		}
		else if(_NPC(target))
		{
			return 1;
		}
		else if(_BECOMENPC(target))
		{
			return 1;
		}
		else if(_CLIENTPET(target))
		{
			return 1;
		}
		else if(_NPCPET(target))
		{
			return 1;

		}
		else if(_BECOMENPCPET(target))
		{
			return 1;
		}



		else
		{
			goto nocase;
		}
	}
	else if(_BECOMENPCPET(this))
	{
		if(_CLIENT(target))
		{
			if(target->CastToClient()->GetLevel() > this->CastToMob()->GetOwner()->CastToClient()->GetBecomeNPCLevel())
				return 0;
			else
				return 1;
		}
		else if(_NPC(target))
		{
			return 1;
		}
		else if(_BECOMENPC(target))
		{
			return 1;
		}
		else if(_CLIENTPET(target))
		{
			if(target->CastToMob()->GetOwner()->CastToClient()->GetLevel() > this->CastToMob()->GetOwner()->CastToClient()->GetBecomeNPCLevel())
				return 0;
			else
				return 1;
		}
		else if(_NPCPET(target))
		{
			return 1;
		}
		else if(_BECOMENPCPET(target))
		{
			return 1;
		}
		else
		{
			goto nocase;
		}
	}

nocase:
	LogFile->write(EQEMuLog::Debug, "Mob::IsAttackAllowed: don't have a rule for this - %s vs %s\n", this->GetName(), target->GetName());
	return 0;
}

#else // not guildwars

bool Mob::IsAttackAllowed(Mob *target)
{
	Mob *mob1, *mob2, *tempmob;
	Client *c1, *c2, *becomenpc;
	NPC *npc1, *npc2;
	int reverse;

	// some special cases
	if(!target)
		return false;

	if(this == target)	// you can attack yourself
		return true;

	// can't damage own pet (applies to everthing)
	if(target->GetOwner() && target->GetOwner() == this)
		return false;
	else if(GetOwner() && GetOwner() == target)
		return false;

#ifdef SHAWN319
	if
	(
	(IsClient() && CastToClient()->GetGM()) ||
	(target->IsClient() && target->CastToClient()->GetGM())
	)
	return false;
#endif
	// solar: the format here is a matrix of mob type vs mob type.
	// redundant ones are omitted and the reverse is tried if it falls through.

	
	// first figure out if we're pets.  we always look at the master's flags.
	// no need to compare pets to anything
	mob1 = this->GetOwner() ? this->GetOwner() : this;
	mob2 = target->GetOwner() ? target->GetOwner() : target;

	reverse = 0;
	do
	{
		if(_CLIENT(mob1))
		{
			if(_CLIENT(mob2))					// client vs client
			{
				c1 = mob1->CastToClient();
				c2 = mob2->CastToClient();
	
				if	// if both are pvp they can fight
				(
					c1->GetPVP() &&
					c2->GetPVP()
				)
					return true;
				else if	// if they're dueling they can go at it
				(
					c1->IsDueling() &&
					c2->IsDueling() &&
					c1->GetDuelTarget() == c2->GetID() &&
					c2->GetDuelTarget() == c1->GetID()
				)
					return true;
				else
					return false;
			}
			else if(_NPC(mob2))				// client vs npc
			{	
				return true;
			}
			else if(_BECOMENPC(mob2))	// client vs becomenpc
			{
				c1 = mob1->CastToClient();
				becomenpc = mob2->CastToClient();
	
				if(c1->GetLevel() > becomenpc->GetBecomeNPCLevel())
					return false;
				else
					return true;
			}	
			else if(_CLIENTCORPSE(mob2))	// client vs client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// client vs npc corpse
			{
				return false;
			}
		}
		else if(_NPC(mob1))
		{
			if(_NPC(mob2))						// npc vs npc
			{
				npc1 = mob1->CastToNPC();
				npc2 = mob2->CastToNPC();
				if
				(
					npc1->GetPrimaryFaction() != 0 &&
					npc2->GetPrimaryFaction() != 0 &&
					(
						npc1->GetPrimaryFaction() == npc2->GetPrimaryFaction() ||
						npc1->IsFactionListAlly(npc2->GetPrimaryFaction())
					)
				)
					return false;
				else
					return true;
			}
			else if(_BECOMENPC(mob2))	// npc vs becomenpc
			{
				return true;
			}
			else if(_CLIENTCORPSE(mob2))	// npc vs client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// npc vs npc corpse
			{
				return false;
			}
		}
		else if(_BECOMENPC(mob1))
		{
			if(_BECOMENPC(mob2))			// becomenpc vs becomenpc
			{
				return true;
			}
			else if(_CLIENTCORPSE(mob2))	// becomenpc vs client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// becomenpc vs npc corpse
			{
				return false;
			}
		}
		else if(_CLIENTCORPSE(mob1))
		{
			if(_CLIENTCORPSE(mob2))		// client corpse vs client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// client corpse vs npc corpse
			{
				return false;
			}
		}
		else if(_NPCCORPSE(mob1))
		{
			if(_NPCCORPSE(mob2))			// npc corpse vs npc corpse
			{
				return false;
			}
		}

		// we fell through, now we swap the 2 mobs and run through again once more
		tempmob = mob1;
		mob1 = mob2;
		mob2 = tempmob;
	}
	while( reverse++ == 0 );

	LogFile->write(EQEMuLog::Debug, "Mob::IsAttackAllowed: don't have a rule for this - %s vs %s\n", this->GetName(), target->GetName());
	return false;
}

#endif // not guildwars

// solar: this is to check if non detrimental things are allowed to be done
// to the target.  clients cannot affect npcs and vice versa, and clients
// cannot affect other clients that are not of the same pvp flag as them.
// also goes for their pets
bool Mob::IsBeneficialAllowed(Mob *target)
{
	Mob *mob1, *mob2, *tempmob;
	Client *c1, *c2;
	int reverse;

	if(!target)
		return false;

	// solar: see IsAttackAllowed for notes
	
	// first figure out if we're pets.  we always look at the master's flags.
	// no need to compare pets to anything
	mob1 = this->GetOwner() ? this->GetOwner() : this;
	mob2 = target->GetOwner() ? target->GetOwner() : target;

	// if it's self target or our own pet it's ok
	if(mob1 == mob2)
		return true;

	reverse = 0;
	do
	{
		if(_CLIENT(mob1))
		{
			if(_CLIENT(mob2))					// client to client
			{
				c1 = mob1->CastToClient();
				c2 = mob2->CastToClient();

				if(c1->GetPVP() == c2->GetPVP())
					return true;
				else if	// if they're dueling they can heal each other too
				(
					c1->IsDueling() &&
					c2->IsDueling() &&
					c1->GetDuelTarget() == c2->GetID() &&
					c2->GetDuelTarget() == c1->GetID()
				)
					return true;
				else
					return false;
			}
			else if(_NPC(mob2))				// client to npc
			{
			}
			else if(_BECOMENPC(mob2))	// client to becomenpc
			{
				return false;
			}
			else if(_CLIENTCORPSE(mob2))	// client to client corpse
			{
				return true;
			}
			else if(_NPCCORPSE(mob2))	// client to npc corpse
			{
				return false;
			}
		}
		else if(_NPC(mob1))
		{
#ifdef GUILDWARS
			if(_CLIENT(mob2))
			{
				return true;
			}
			else if(_NPC(mob2))						// npc to npc
#else
			if(_NPC(mob2))						// npc to npc
#endif
			{
				return true;
			}
			else if(_BECOMENPC(mob2))	// npc to becomenpc
			{
				return true;
			}
			else if(_CLIENTCORPSE(mob2))	// npc to client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// npc to npc corpse
			{
				return false;
			}
		}
		else if(_BECOMENPC(mob1))
		{
			if(_BECOMENPC(mob2))			// becomenpc to becomenpc
			{
				return true;
			}
			else if(_CLIENTCORPSE(mob2))	// becomenpc to client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// becomenpc to npc corpse
			{
				return false;
			}
		}
		else if(_CLIENTCORPSE(mob1))
		{
			if(_CLIENTCORPSE(mob2))		// client corpse to client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// client corpse to npc corpse
			{
				return false;
			}
		}
		else if(_NPCCORPSE(mob1))
		{
			if(_NPCCORPSE(mob2))			// npc corpse to npc corpse
			{
				return false;
			}
		}

		// we fell through, now we swap the 2 mobs and run through again once more
		tempmob = mob1;
		mob1 = mob2;
		mob2 = tempmob;
	}
	while( reverse++ == 0 );

	LogFile->write(EQEMuLog::Debug, "Mob::IsBeneficialAllowed: don't have a rule for this - %s to %s\n", this->GetName(), target->GetName());
	return false;
}

bool Mob::CombatRange(Mob* other)
{
    // neotokyo: some mobs have set size == -1; this caused a signed/unsigned overflow
	sint32 size_mod = (sint32)GetSize();
	if(GetRace() == 49 || GetRace() == 158 || GetRace() == 196) //For races with a fixed size
 		size_mod = 60;
	else if (size_mod < 6)
		size_mod = 8;
	if (other->GetSize() > size_mod)
		size_mod = (sint32)other->GetSize();
	if (DistNoZ(*other) <= size_mod*2)
		return true;
	return false;
}

//Old LOS function, prolly not used anymore
//Not removed because I havent looked it over to see if anything
//useful is in here before we delete it.
bool Mob::CheckLos(Mob* other) {
	if (zone->map == 0)
	{
		return true;
	}
	float tmp_x = GetX();
	float tmp_y = GetY();
	float tmp_z = GetZ();
	float trg_x = other->GetX();
	float trg_y = other->GetY();
	float trg_z = other->GetZ();
	float perwalk_x = 0.5;
	float perwalk_y = 0.5;
	float perwalk_z = 0.5;
	float dist_x = tmp_x - trg_x;
	if (dist_x < 0)
		dist_x *= -1;
	float dist_y = tmp_y - trg_y;
	if (dist_y < 0)
		dist_y *= -1;
	float dist_z = tmp_z - trg_z;
	if (dist_z < 0)
		dist_z *= -1;
	if (dist_x  < dist_y && dist_z < dist_y)
	{
		perwalk_x /= (dist_y/dist_x);
		perwalk_z /= (dist_y/dist_z);
	}
	else if (dist_y  < dist_x && dist_z < dist_x)
	{
		perwalk_y /= (dist_x/dist_y);
		perwalk_z /= (dist_x/dist_z);
	}
	else if (dist_x  < dist_z && dist_y < dist_z)
	{
		perwalk_x /= (dist_z/dist_x);
		perwalk_y /= (dist_z/dist_y);
	}
	float steps = (dist_x/perwalk_x + dist_y/perwalk_y + dist_z/perwalk_z)*10; //Just a safety check to prevent endless loops.
	while (steps > 0) {
		steps--;
		if (tmp_x < trg_x)
		{
			if (tmp_x + perwalk_x < trg_x)
				tmp_x += perwalk_x;
			else
				tmp_x = trg_x;
		}
		if (tmp_y < trg_y)
		{
			if (tmp_y + perwalk_y < trg_y)
				tmp_y += perwalk_y;
			else
				tmp_y = trg_y;
		}
		if (tmp_z < trg_z)
		{
			if (tmp_z + perwalk_z < trg_z)
				tmp_z += perwalk_z;
			else
				tmp_z = trg_z;
		}
		if (tmp_x > trg_x)
		{
			if (tmp_x - perwalk_x > trg_x)
				tmp_x -= perwalk_x;
			else
				tmp_x = trg_x;
		}
		if (tmp_y > trg_y)
		{
			if (tmp_y - perwalk_y > trg_y)
				tmp_y -= perwalk_y;
			else
				tmp_y = trg_y;
		}
		if (tmp_z > trg_z)
		{
			if (tmp_z - perwalk_z > trg_z)
				tmp_z -= perwalk_z;
			else
				tmp_z = trg_z;
		}
		if (tmp_y == trg_y && tmp_x == trg_x && tmp_z == trg_z)
		{
			return true;
		}

//I believe this is contributing to breaking mob spawns when a map is loaded
//		NodeRef pnode = zone->map->SeekNode( zone->map->GetRoot(), tmp_x, tmp_y );
		NodeRef pnode = NODE_NONE;
		if (pnode != NODE_NONE)
		{
			int *iface = zone->map->SeekFace( pnode, tmp_x, tmp_y );
			if (*iface == -1) {
				return false;
			}
			float temp_z = 0;
			float best_z = 999999;
			while(*iface != -1)
			{
				temp_z = zone->map->GetFaceHeight( *iface, x_pos, y_pos );
//UMM.. OMG... sqrt(pow(x, 2)) == x.... retards
				float best_dist = sqrt((double)(pow(best_z-tmp_z, 2)));
				float tmp_dist = sqrt((double)(pow(tmp_z-tmp_z, 2)));
				if (tmp_dist < best_dist)
				{
					best_z = temp_z;
				}
				iface++;
			}
/*	solar: our aggro code isn't using this right now, just spells, so i'm
    taking out the +-10 check for now to make it work right on hills
			if (best_z - 10 > trg_z || best_z + 10 < trg_z)
			{
				return false;
			}
*/
		}
	}
	return true;
}


//Father Nitwit's LOS code
bool Mob::CheckLosFN(Mob* other) {
	if(other == NULL || zone->map == NULL) {
		//not sure what the best return is on error
		//should make this a database variable, but im lazy today
#ifdef LOS_DEFAULT_CAN_SEE
		return(true);
#else
		return(false);
#endif
	}
	
	VERTEX myloc;
	VERTEX oloc;
	
#define LOS_DEFAULT_HEIGHT 6.0f
	
	myloc.x = GetX();
	myloc.y = GetY();
	myloc.z = GetZ() + (GetSize()==0.0?LOS_DEFAULT_HEIGHT:GetSize())/2 * HEAD_POSITION;
	
	oloc.x = other->GetX();
	oloc.y = other->GetY();
	oloc.z = other->GetZ() + (other->GetSize()==0.0?LOS_DEFAULT_HEIGHT:other->GetSize())/2 * SEE_POSITION;

#if LOSDEBUG>=5
	LogFile->write(EQEMuLog::Debug, "LOS from (%.2f, %.2f, %.2f) to (%.2f, %.2f, %.2f) sizes: (%.2f, %.2f)", myloc.x, myloc.y, myloc.z, oloc.x, oloc.y, oloc.z, GetSize(), other->GetSize());
#endif
	
	FACE *onhit;
	NodeRef mynode;
	NodeRef onode;
	
	VERTEX hit;
	//see if anything in our node is in the way
	mynode = zone->map->SeekNode( zone->map->GetRoot(), myloc.x, myloc.y);
	if(mynode != NODE_NONE) {
		if(zone->map->LineIntersectsNode(mynode, myloc, oloc, &hit, &onhit)) {
#if LOSDEBUG>=5
			LogFile->write(EQEMuLog::Debug, "Check LOS for %s target %s, cannot see.", GetName(), other->GetName() );
			LogFile->write(EQEMuLog::Debug, "\tPoly: (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f)\n",
				onhit->a.x, onhit->a.y, onhit->a.z,
				onhit->b.x, onhit->b.y, onhit->b.z, 
				onhit->c.x, onhit->c.y, onhit->c.z);
#endif
			return(false);
		}
	}
#if LOSDEBUG>=5
	 else {
		LogFile->write(EQEMuLog::Debug, "WTF, I have no node, what am I standing on??? (%.2f, %.2f).", myloc.x, myloc.y);
	}
#endif
	
	//see if they are in a different node.
	//if so, see if anything in their node is blocking me.
	if(! zone->map->LocWithinNode(mynode, oloc.x, oloc.y)) {
		onode = zone->map->SeekNode( zone->map->GetRoot(), oloc.x, oloc.y);
		if(onode != NODE_NONE && onode != mynode) {
			if(zone->map->LineIntersectsNode(onode, myloc, oloc, &hit, &onhit)) {
#if LOSDEBUG>=5
			LogFile->write(EQEMuLog::Debug, "Check LOS for %s target %s, cannot see (2).", GetName(), other->GetName());
			LogFile->write(EQEMuLog::Debug, "\tPoly: (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f)\n",
				onhit->a.x, onhit->a.y, onhit->a.z,
				onhit->b.x, onhit->b.y, onhit->b.z, 
				onhit->c.x, onhit->c.y, onhit->c.z);
#endif
				return(false);
			}
		}
#if LOSDEBUG>=5
		 else if(onode == NODE_NONE) {
			LogFile->write(EQEMuLog::Debug, "WTF, They have no node, what are they standing on??? (%.2f, %.2f).", myloc.x, myloc.y);
		}
#endif
	}
	
	/*
	if(zone->map->LineIntersectsZone(myloc, oloc, CHECK_LOS_STEP, &onhit)) {
#if LOSDEBUG>=5
		LogFile->write(EQEMuLog::Debug, "Check LOS for %s target %s, cannot see.", GetName(), other->GetName() );
		LogFile->write(EQEMuLog::Debug, "\tPoly: (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f)\n",
			onhit->a.x, onhit->a.y, onhit->a.z,
			onhit->b.x, onhit->b.y, onhit->b.z, 
			onhit->c.x, onhit->c.y, onhit->c.z);
#endif
		return(false);
	}*/
	
#if LOSDEBUG>=5
			LogFile->write(EQEMuLog::Debug, "Check LOS for %s target %s, CAN SEE.", GetName(), other->GetName());
#endif
	
	return(true);
}

//offensive spell aggro
int16 Mob::CheckAggroAmount(int16 spellid) {
	int16 spell_id = spellid;
	int16 AggroAmount = 1;
	int16 slevel = GetLevel();

	for (int o = 0; o < EFFECT_COUNT; o++) {
		switch(spells[spell_id].effectid[o]) {
			case SE_MovementSpeed: {
				int val = CalcSpellEffectValue_formula(spells[spell_id].formula[o], spells[spell_id].base[o], spells[spell_id].max[o], this->GetLevel(), spell_id);
				if (val < 0)
				{
					AggroAmount+=slevel*4;
					//AggroAmount += val*-4;
					break;
				}
				break;
			}
			case SE_AttackSpeed: {
				int val = CalcSpellEffectValue_formula(spells[spell_id].formula[o], spells[spell_id].base[o], spells[spell_id].max[o], this->GetLevel(), spell_id);
				if (val < 100)
				{
					//AggroAmount += (100-val)*15;
					AggroAmount+=slevel*6;
				}
				break;
			}
			case SE_Stun: {
				if (spells[spell_id].base[o] > 1)
					AggroAmount+=slevel*6;
				else
					AggroAmount+=slevel*2;
				break;
			}
			case SE_Blind: {
				AggroAmount+=slevel*3;
				break;
			}
			case SE_Mez: {
				int mez_amount = slevel*10;
				switch (GetAA(aaJewelCraftMastery))
				{
					case 1:
						mez_amount = mez_amount * 90 / 100;
						break;
					case 2:
						mez_amount = mez_amount * 75 / 100;
						break;
					case 3:
						mez_amount = mez_amount * 60 / 100;
						break;
				}
				AggroAmount+=mez_amount;
				break;
			}
			case SE_Charm: {
				AggroAmount+=slevel*15;
				break;
			}
			case SE_Root: {
				AggroAmount+=slevel*2;
				break;
			}
			case SE_Fear: {
				AggroAmount+=slevel*4;
				break;
			}
			case SE_ArmorClass:
			case SE_ResistMagic:
			case SE_ResistAll:
			case SE_ResistFire:
			case SE_ResistCold:
			case SE_ResistPoison:
			case SE_ResistDisease:
			case SE_STR:
			case SE_STA:
			case SE_DEX:
			case SE_AGI:
			case SE_INT:
			case SE_WIS:
			case SE_CHA:
			case SE_ATK:
		//	case SE_DiseaseCounters:
		//	case SE_PoisonCounters:
			{
				int val = CalcSpellEffectValue_formula(spells[spell_id].formula[o], spells[spell_id].base[o], spells[spell_id].max[o], this->GetLevel(), spell_id);
				if (val < 0)
				{
					AggroAmount += val*-2;
				}
				break;
			}
		}
	}
	if (IsBardSong(spell_id))
		AggroAmount /= 3;
	if (GetOwner())
		AggroAmount /= 10;
	return AggroAmount;
}

//healing and buffing aggro
//I dont think this accounts for direct healing spells.
int16 Mob::CheckHealAggroAmount(int16 spellid) {
	int16 spell_id = spellid;
	int16 AggroAmount = 1;
//	int16 slevel = GetLevel();

	for (int o = 0; o < EFFECT_COUNT; o++) {
		switch(spells[spell_id].effectid[o]) {
			case SE_MovementSpeed: {
				int val = CalcSpellEffectValue_formula(spells[spell_id].formula[o], spells[spell_id].base[o], spells[spell_id].max[o], this->GetLevel(), spell_id);
				if (val > 0)
				{
					AggroAmount += val;
					break;
				}
				break;
			}
			case SE_AttackSpeed: {
				int val = CalcSpellEffectValue_formula(spells[spell_id].formula[o], spells[spell_id].base[o], spells[spell_id].max[o], this->GetLevel(), spell_id);
				if (val > 100)
				{
					AggroAmount += val-100;
				}
				break;
			}
			case SE_Rune:
			case SE_HealOverTime: {
				int val = CalcSpellEffectValue_formula(spells[spell_id].formula[o], spells[spell_id].base[o], spells[spell_id].max[o], this->GetLevel(), spell_id);
				AggroAmount += val/4;
				break;
			}
			case SE_ArmorClass:
			case SE_ResistMagic:
			case SE_ResistAll:
			case SE_ResistFire:
			case SE_ResistCold:
			case SE_ResistPoison:
			case SE_ResistDisease:
			case SE_STR:
			case SE_STA:
			case SE_DEX:
			case SE_AGI:
			case SE_INT:
			case SE_WIS:
			case SE_CHA:
			case SE_ATK:
			{
				int val = CalcSpellEffectValue_formula(spells[spell_id].formula[o], spells[spell_id].base[o], spells[spell_id].max[o], this->GetLevel(), spell_id);
				if (val < 0)
				{
					AggroAmount += val/2;
				}
				break;
			}
		}
	}
	if (IsBardSong(spell_id))
		AggroAmount /= 3;
	if (GetOwner())
		AggroAmount /= 10;
	return AggroAmount;
}


