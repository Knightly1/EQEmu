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


/*

	solar:	General outline of spell casting process
	
	1.
		a)	Client clicks a spell bar gem, ability, or item.  client_process.cpp
		gets the op and calls CastSpell() with all the relevant info including
		cast time.

		b)  NPC does CastSpell() from AI

	2.
		a)	CastSpell() determines there is a cast time and sets some state keeping
		flags to be used to check the progress of casting and finish it later.

		b)	CastSpell() sees there's no cast time, and calls CastedSpellFinished()
		Go to step 4.

	3.
		SpellProcess() notices that the spell casting timer which was set by
		CastSpell() is expired, and calls CastedSpellFinished()

	4.
		CastedSpellFinished() checks some timed spell specific things, like
		wether to interrupt or not, due to movement or melee.  If successful
		SpellFinished() is called.

	5.
		SpellFinished() checks some things like LoS, reagents, target and
		figures out what's going to get hit by this spell based on its type.

	6.
		a)	Single target spell, SpellOnTarget() is called.

		b)	AE spell, Entity::AESpell() is called.

		c)	Group spell, Group::CastGroupSpell()/SpellOnTarget() is called as
		needed.

	7.
		SpellOnTarget() may or may not call SpellEffect() to cause effects to
		the target

	8.
		If this was timed, CastedSpellFinished() will restore the client's
		spell bar gems.


	Most user code should call CastSpell(), with a 0 casting time if needed,
	and not SpellFinished().

*/



#include "../common/debug.h"
#include "spdat.h"
#include "masterentity.h"
#include "../common/packet_dump.h"
#include "../common/moremath.h"
#include "../common/Item.h"
#include "worldserver.h"
#include "../common/skills.h"
#include "../common/bodytypes.h"
#include "../common/classes.h"
#include <math.h>
#include <assert.h>
#ifndef WIN32
//	#include <pthread.h>
#include <stdlib.h>
#include "../common/unix.h"
#endif
#ifdef _GOTFRAGS
	#include "../common/packet_dump_file.h"
#endif

#ifdef GUILDWARS
#include "../GuildWars/GuildWars.h"
extern GuildWars guildwars;
extern GuildLocationList location_list;
#endif
#include "StringIDs.h"

extern Database database;
extern Zone* zone;
extern volatile bool ZoneLoaded;
#ifndef NEW_LoadSPDat
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif
extern bool spells_loaded;
extern WorldServer worldserver;
uchar blah[]={0x0D,0x00,0x00,0x00,0x01,0x00,0x00,0x00};
uchar blah2[]={0x12,0x00,0x00,0x00,0x16,0x01,0x00,0x00};


// solar: this is run constantly for every mob
void Mob::SpellProcess()
{
	// check the rapid recast prevention timer
	if(delaytimer == true && spellend_timer.Check())
	{
		spellend_timer.Disable();
		delaytimer = false;
		return;
	}

	// a timed spell is finished casting
	if (casting_spell_id != 0 && spellend_timer.Check())
	{
		spellend_timer.Disable();
		delaytimer = false;
		CastedSpellFinished(casting_spell_id, casting_spell_targetid, casting_spell_slot, casting_spell_mana, casting_spell_inventory_slot);
	}

}

void NPC::SpellProcess()
{
	// check the rapid recast prevention timer
	if(delaytimer == true && spellend_timer.Check())
	{
		spellend_timer.Disable();
		delaytimer = false;
		return;
	}

	// a timed spell is finished casting
	if (casting_spell_id != 0 && spellend_timer.Check())
	{
		spellend_timer.Disable();
		delaytimer = false;
		CastedSpellFinished(casting_spell_id, casting_spell_targetid, casting_spell_slot, casting_spell_mana, casting_spell_inventory_slot);
	}
	
	//Dook- swarm pets 
	if(GetBodyType() == bodyTypeSwarmPet) {
		if(swarm_timer.Check()) {
			Depop(); 
			swarm_timer.Disable();
		} 
   } 

}

///////////////////////////////////////////////////////////////////////////////
// functions related to begin/finish casting, fizzling etc

//
// solar: only CastSpell and DoCastSpell should be setting casting_spell_id.
// basically casting_spell_id is only set when casting a triggered spell from
// the spell bar gems, an ability, or an item.  note that it's actually set
// even if it's a 0 cast time, but then the spell is finished right after and
// it's unset.  this is ok, since the 0 cast time spell is still a triggered
// one.
// the rule is you can cast one triggered (usually timed) spell at a time
// but things like SpellFinished() can run concurrent with a triggered cast
// to allow procs to work
void Mob::CastSpell(int16 spell_id, int16 target_id, int16 slot,
	sint32 cast_time, sint32 mana_cost, int32* oSpellWillFinish,int item_slot)
{
	if
	(
		!IsValidSpell(spell_id) ||
		casting_spell_id ||
		delaytimer ||
		spellend_timer.Enabled() ||
		IsStunned() ||
		IsMezzed()
	)
	{
		if(IsClient())
			CastToClient()->SendSpellBarEnable(spell_id);
		return;
	}
	
	//cannot cast under deivne aura
	if(DivineAura()) {
		InterruptSpell(173, 0x121, false);
		return;
	}

#ifdef GUILDWARS
	Mob *target_mob = entity_list.GetMob(target_id);
	if
	(
		guildwars.GetPVPAbility() == 3 &&
		((IsBeneficialSpell(spell_id) && !guildwars.SpecialCastPrivs(this, target_mob ? target_mob : this)) || (IsDetrimentalSpell(spell_id) && !guildwars.SpecialAttackPrivs(this, target_mob ? target_mob : this)))
	)
	{
		if(IsClient())
			CastToClient()->SendSpellBarEnable(spell_id);
		return;
	}
#endif


	casting_spell_id = spell_id;
	casting_spell_slot = slot;
	casting_spell_inventory_slot = item_slot;

	// check for fizzle
	// note that CheckFizzle itself doesn't let NPCs fizzle,
	// but this code allows for it.
	if(slot < MAX_PP_MEMSPELL && !CheckFizzle(spell_id))
	{
		int fizzle_msg = IsBardSong(spell_id) ? MISS_NOTE : SPELL_FIZZLE;
		InterruptSpell(fizzle_msg, 0x121, spell_id);

		// fizzle 1/4 the mana away
		SetMana(GetMana() - (mana_cost / 4));
		return;
	}
	
	
    if (bardsong != 0) {
        StopSong();
		casting_spell_id = 0;
        return;
    }

	DoCastSpell(spell_id, target_id, slot, cast_time, mana_cost, oSpellWillFinish, item_slot);
}

//
// solar: the order of things here is intentional and important.  make sure you
// understand the whole spell casting process and the flags that are passed
// around if you're gonna modify this
//
// this is the 2nd phase of CastSpell, broken up like this to make it easier
// to repeat a spell for bard songs
//
void Mob::DoCastSpell(int16 spell_id, int16 target_id, int16 slot,
                    sint32 cast_time, sint32 mana_cost, int32* oSpellWillFinish, int item_slot)
{
	Mob* pMob = NULL;
	float mobDist;
	sint32 orgcasttime;
	float modrange;
	APPLAYER *outapp = NULL;

	if(!IsValidSpell(spell_id))
		return;

	casting_spell_id = spell_id;
	casting_spell_slot = slot;
	casting_spell_inventory_slot = item_slot;

	SaveSpellLoc();

	// if this spell doesn't require a target, or if it's an optional target
	// and a target wasn't provided, then it's us; unless TGB is on and this
	// is a TGB compatible spell.
	if(
		(
			IsGroupSpell(spell_id) &&
			!(
				IsClient() &&
				CastToClient()->TGB() &&
				IsTGBCompatibleSpell(spell_id)
			)
		) ||
		spells[spell_id].targettype == ST_Self ||
		spells[spell_id].targettype == ST_AECaster ||
		(spells[spell_id].targettype == ST_TargetOptional && target_id == 0)
	)
	{
		target_id = GetID();
	}

	if(cast_time == -1)
	{
		// save the non-reduced cast time to use in the packet
		cast_time = orgcasttime = spells[spell_id].cast_time;
		// if there's a cast time, check if they have a modifier for it
		if(cast_time)
		{
			cast_time = GetActSpellCasttime(spell_id, cast_time);
			
		}
	}
	else
		orgcasttime = cast_time;

	// we checked for spells not requiring targets above
	if(target_id == 0)
	{
#if EQDEBUG >= 8
				LogFile->write(EQEMuLog::Debug, "%s: Spell Error: no target spell=%d\n", GetName(), spell_id);
#endif
printf("Error no target.\n");
		if(IsClient()) {
			//clients produce messages... npcs should not for this case
			Message(13, "Error: Spell requires a target.");
			InterruptSpell();
		} else {
			InterruptSpell(0, 0, 0);	//the 0 args should cause no messages
		}
		return;
	}

	// ok now we know the target
	casting_spell_targetid = target_id;

	if (mana_cost == -1)
	{
		mana_cost = spells[spell_id].mana;
		mana_cost = GetActSpellCost(spell_id, mana_cost);
	}

	// neotokyo: 19-Nov-02
	// mana is checked for clients on the frontend. we need to recheck it for NPCs though
	// fix: items dont need mana :-/
	// Quagmire: If you're at full mana, let it cast even if you dont have enough mana
	// solar: TODO reduce mana cost focus items
	// we calculated this above, now enforce it
	if(mana_cost > 0 && slot != 10)
	{
		int my_curmana = GetMana();
		int my_maxmana = GetMaxMana();
		if(my_curmana < mana_cost)	// not enough mana
		{
			//this is a special case for NPCs with no mana...
			if(my_maxmana > 0 &&	my_curmana == my_maxmana)
			{
				mana_cost = 0;
			} else {
#if EQDEBUG >= 8
				LogFile->write(EQEMuLog::Debug, "%s: Spell Error not enough mana spell=%d mymana=%d cost=%d\n", GetName(), spell_id, my_curmana, mana_cost);
#endif
				if(IsClient()) {
					//clients produce messages... npcs should not for this case
					Message(13, "Error: Insufficent mana.");
					InterruptSpell();
				} else {
					InterruptSpell(0, 0, 0);	//the 0 args should cause no messages
				}
				return;
			}
		}
	}

	// we know our mana cost now
	casting_spell_mana = mana_cost;
	

	// cast time is 0, just finish it right now and be done with it
	if(cast_time == 0)
	{
		CastedSpellFinished(spell_id, target_id, slot, mana_cost, item_slot);
		return;
	}

	// ok we know it has a cast time so we can start the timer now
	spellend_timer.Start(cast_time);
	
	// we check this variable later, for the begins to glow message
	modrange = spells[spell_id].range;

	// find the target in entity_list
	pMob = entity_list.GetMob(target_id);
	
	if(!pMob)
	{
		Message(13, "Error aquiring target.");
		InterruptSpell();
		return;
	}

	// check distance to target against spell range, but first let's
	// eliminate the case where it's our self targeted, or it's a group
	// spell
	// note: client checks this, but this makes sure
	if(pMob != this && !IsGroupSpell(spell_id))
	{
		mobDist = DistNoRoot(*pMob);
		if(mobDist > spells[spell_id].range)
		{
			modrange = GetActSpellRange(spell_id, spells[spell_id].range);

			if(modrange*modrange < mobDist) // still not enough
			{
				if (IsClient() && !(spells[spell_id].targettype == ST_AECaster))
				{
					Message(MT_Spells, "Your target is out of range(2)!");
				}
				InterruptSpell();
				return;
			}
		}
	}
	
	if (IsAIControlled())
	{
		SetRunAnimSpeed(0);
		if(this != pMob)
			this->FaceTarget(pMob, true);
	}
	
	// if we got here we didn't fizzle, and are starting our cast

	pDontCastBefore_casting_spell = oSpellWillFinish;
	if (oSpellWillFinish)
		*oSpellWillFinish = Timer::GetCurrentTime() + cast_time + 100;


	// now tell the people in the area
	outapp = new APPLAYER(OP_BeginCast,sizeof(BeginCast_Struct));
	BeginCast_Struct* begincast = (BeginCast_Struct*)outapp->pBuffer;
	begincast->caster_id = GetID();
	begincast->spell_id = spell_id;
	begincast->cast_time = orgcasttime; // client calculates reduced time by itself
	outapp->priority = 3;
	entity_list.QueueCloseClients(this, outapp, false, 200, 0, true, IsClient() ? FILTER_PCSPELLS : FILTER_NPCSPELLS);
	safe_delete(outapp);
	outapp = NULL;

/*	if (IsClient()) // begins to glow messages
	{
		char itemname[65];
		int16 focusspell;
		sint32 buffdur, newdur, dmg=0, newdmg;

		// mana cost - slot 10 is an item
		if(slot != 10 && CastToClient()->GetReduceManaCostItem(focusspell, itemname))
		{
			Message_StringID(MT_Spells, BEGINS_TO_GLOW, itemname);
		}

		// spell range
		if (modrange != spells[spell_id].range)
		{
			CastToClient()->GetExtendedRangeItem(focusspell, itemname);
			Message_StringID(MT_Spells, BEGINS_TO_GLOW, itemname);
		}
		
		// buff duration
		buffdur = CalcBuffDuration_formula(
			GetCasterLevel(spell_id), 
			spells[spell_id].buffdurationformula, 
			spells[spell_id].buffduration
		);
//		buffdur = CalcBuffDuration(this, pMob, spell_id);
// we use the straight formula calculation here, since we're just trying
// to see if there's a reduction.  this whole message printing thing is
// probably not ideal, since this isn't where it's actually applying
// the reductions...
		
		newdur = GetActSpellDuration(spell_id, buffdur);
		if (newdur != buffdur)
		{
			CastToClient()->GetIncreaseSpellDurationItem(focusspell, itemname);
			Message_StringID(MT_Spells, BEGINS_TO_GLOW, itemname);
		}
		
		// Improved Healing and Damage
		for (int i = 0; i < EFFECT_COUNT; i++)
		{
			if (spells[spell_id].effectid[i] == SE_CurrentHP)
			{
				dmg = CalcSpellEffectValue(spell_id, i, GetCasterLevel(spell_id));
				break;
			}   
		}

		if( dmg < 0)
		{
			newdmg = GetActSpellValue(spell_id, dmg);
			if (newdmg != dmg)
			{
				CastToClient()->GetImprovedDamageItem(focusspell, itemname);
				Message_StringID(MT_Spells, BEGINS_TO_GLOW, itemname);
			}

		}
		else if(dmg > 0)
		{
			newdmg = GetActSpellValue(spell_id, dmg);
			if (newdmg != dmg)
			{
				CastToClient()->GetImprovedHealingItem(focusspell, itemname);
				Message_StringID(MT_Spells, BEGINS_TO_GLOW, itemname);
			}

		}
		
		// CastTime
		if(orgcasttime != cast_time)
		{
			CastToClient()->GetReduceCastTimeItem(focusspell, itemname);
			Message_StringID(MT_Spells, BEGINS_TO_GLOW, itemname);
		}

	}
	end obsolete glow messages
	*/
}

int Mob::GetSpecializeSkill(int16 spell_id) {
	switch(spells[spell_id].skill) {
		case ABJURE:
			return(SPECIALIZE_ABJURE);
			break;
		case ALTERATION:
			return(SPECIALIZE_ALTERATION);
			break;
		case CONJURATION:
			return(SPECIALIZE_CONJURATION);
			break;
		case DIVINATION:
			return(SPECIALIZE_DIVINATION);
			break;
		case EVOCATION:
			return(SPECIALIZE_EVOCATION);
			break;
		default:
			//wtf...
			break;
	}
	return(0xFFFF);
}

/*
solar: returns true if spell is successful, false if it fizzled.
only works for clients, npcs shouldn't be fizzling..
neotokyo: new algorithm thats closer to live eq (i hope)
kathgar TODO: Add aa skills, item mods, reduced the chance to fizzle and gm's don't fizzle
*/
bool Mob::CheckFizzle(int16 spell_id)
{
	if(!this->IsClient())
		return 1;

	Client *c = this->CastToClient();

	// GMs don't fizzle
	if (c->GetGM()) return(true);

	assert(IsValidSpell(spell_id));
	
	
	int no_fizzle_level = 0;
	if (GetAA(aaMasteryofthePast)) {
		switch (GetAA(aaMasteryofthePast)) {
			case 1:
				no_fizzle_level = 55;
				break;
			case 2:
				no_fizzle_level = 60;
				break;
			case 3:
				no_fizzle_level = 65;
				break;
		}
	} else {
		switch (GetAA(aaSpellCastingExpertise)) {
			case 1:
				no_fizzle_level = 20;
				break;
			case 2:
				no_fizzle_level = 35;
				break;
			case 3:
				no_fizzle_level = 52;
				break;
		}
	}
	if (spells[spell_id].classes[GetClass()-1] <= no_fizzle_level)
		return true;
	
	//is there any sort of focus that affects fizzling?
	
	
	// neotokyo: this is my try to get something going
	int par_skill;
	int act_skill;
	
	par_skill = spells[spell_id].classes[c->GetClass()-1] * 5 - 10;//IIRC even if you are lagging behind the skill levels you don't fizzle much
	/*par_skill = spells[spell_id].classes[c->GetClass()-1] * 5 + 5;*/
	if (par_skill > 235)
		par_skill = 235;

	par_skill += spells[spell_id].classes[c->GetClass()-1]; // maximum of 270 for level 65 spell

	act_skill = c->GetSkill(spells[spell_id].skill);
	act_skill += c->GetLevel(); // maximum of whatever the client can cheat
	
	//FatherNitwit: spell specialization
	int spec_skill = GetSpecializeSkill(spell_id);
	float specialize = spec_skill < HIGHEST_SKILL ? GetSkill(spec_skill) : 0;
		//VERY rough success formula, needs research
	if(specialize > 0) {
		specialize += GetAA(aaSpellCastingMastery) * 5.0;
		if(((specialize/6.0f) + 15.0f) < MakeRandomFloat(0, 100)) {
			specialize *= SPECIALIZE_FIZZLE / 200;
		} else {
			specialize = 0.0f;
		}
	}
	
	// == 0 --> on par
	// > 0  --> skill is lower, higher chance of fizzle
	// < 0  --> skill is better, lower chance of fizzle
	// the max that diff can be is +- 235
	float diff = par_skill + spells[spell_id].basediff - act_skill;

	// if you have high int/wis you fizzle less, you fizzle more if you are stupid
	if (c->GetCasterClass() == 'W')
		diff -= (GetWIS() - 125) / 20.0;
	if (c->GetCasterClass() == 'I')
		diff -= (GetINT() - 125) / 20.0;

	// base fizzlechance is lets say 5%, we can make it lower for AA skills or whatever
	float basefizzle = 10;
	float fizzlechance = basefizzle - specialize + diff / 5.0;

	// always at least 5% chance to fail or succeed
	fizzlechance = fizzlechance < 5 ? 5 : (fizzlechance > 95 ? 95 : fizzlechance);
	float fizzle_roll = MakeRandomFloat(0, 100);

#if EQDEBUG >= 5
	LogFile->write(EQEMuLog::Debug, "Check Fizzle %s  fizzlechance: %0.2f%%   diff: %0.2f  roll: %0.2f", GetName(), fizzlechance, diff, fizzle_roll);
#endif

	if(fizzle_roll > fizzlechance)
		return(true);
	return(false);
}

void Mob::ZeroCastingVars()
{
	// zero out the state keeping vars
	isattacked = false;
	attacked_count = 0;
	spellend_timer.Disable();
	casting_spell_id = 0;
	casting_spell_targetid = 0;
	casting_spell_slot = 0;
	casting_spell_mana = 0;
	casting_spell_inventory_slot = 0;
	delaytimer = false;
}

void Mob::InterruptSpell(int16 spellid)
{
	if (spellid == SPELL_UNKNOWN)
		spellid = casting_spell_id;

	InterruptSpell(0, 0x121, spellid);
}

// solar: color not used right now
void Mob::InterruptSpell(int16 message, int16 color, int16 spellid)
{
	APPLAYER *outapp;
	int16 message_other;

	if (spellid == 0xFFFF)
		spellid = casting_spell_id;

	if(casting_spell_id)
		AI_Event_SpellCastFinished(false, casting_spell_slot);

	ZeroCastingVars();	// resets all the state keeping stuff

	if(!spellid)
		return;

	if (bardsong || IsBardSong(casting_spell_id))
		StopSong();
			
	if(!message)
		message = IsBardSong(spellid) ? SONG_ENDS_ABRUPTLY : INTERRUPT_SPELL;


// whatever this was good for ... dont do it anymore
//		if(this->IsNPC() && spells[casting_spell_id].resisttype)
//		{
//			this->CastToNPC()->AddQueuedSpell(casting_spell_id);
//		}
//

	if(spellid)
	{
		// clients need some packets
		if (IsClient())
		{
			// the interrupt message
			outapp = new APPLAYER(OP_InterruptCast, sizeof(InterruptCast_Struct));
			InterruptCast_Struct* ic = (InterruptCast_Struct*) outapp->pBuffer;
			ic->messageid = message;
			ic->spawnid = GetID();
			outapp->priority = 5;
			CastToClient()->QueuePacket(outapp);
			safe_delete(outapp);

			SendSpellBarEnable(spellid);
		}

		// notify people in the area

		// first figure out what message others should get
		switch(message)
		{
			case SONG_ENDS:
				message_other = SONG_ENDS_OTHER;
				break;
			case SONG_ENDS_ABRUPTLY:
				message_other = SONG_ENDS_ABRUPTLY_OTHER;
				break;
			case MISS_NOTE:
				message_other = MISS_NOTE_OTHER;
				break;
			case SPELL_FIZZLE:
				message_other = SPELL_FIZZLE_OTHER;
				break;
			default:
				message_other = INTERRUPT_SPELL_OTHER;
		}

		// this is the actual message, it works the same as a formatted message
		outapp = new APPLAYER(OP_InterruptCast, sizeof(InterruptCast_Struct) + strlen(GetCleanName()) + 1);
		InterruptCast_Struct* ic = (InterruptCast_Struct*) outapp->pBuffer;
		ic->messageid = message_other;
		ic->spawnid = GetID();
		strcpy(ic->message, GetCleanName());
		entity_list.QueueCloseClients(this, outapp, true, 200, 0, true, IsClient() ? FILTER_PCSPELLS : FILTER_NPCSPELLS);
		safe_delete(outapp);

		// solar: TODO need another packet or something here to make the caster
		// stop animating.  other people still see him casting until the normal
		// duration (what was sent in the BeginCast) is up.

	}
}

// solar: this is called after the timer is up and the spell is finished
// casting.  everything goes through here, including items with zero cast time
// only to be used from SpellProcess
// NOTE: do not put range checking, etc into this function.  this should
// just check timed spell specific things before passing off to SpellFinished
// which figures out proper targets etc
void Mob::CastedSpellFinished(int16 spell_id, int32 target_id, int16 slot, int16 mana_used, int inventory_slot)
{
	//watch timer for long ass reuse_time spells
	if(IsClient() && slot != 10 && spells[spell_id].recast_time > 30000) {	// 10 is item
		if(!CastToClient()->GetPTimers().Expired(pTimerSpellStart + spell_id)) {
			//should we issue a  message or send them a spell gem packet?
			Message(13, "Spell reuse timer not expired yet.");
			InterruptSpell();
			return;
		}
	}
	
	bool regain_conc = false;
	float channelchance, distance_moved, d_x, d_y, distancemod;
	 
	if(!IsValidSpell(spell_id))
	{
		InterruptSpell();
		return;
	}

	// prevent rapid recast - this can happen if somehow the spell gems
	// become desynced and the player casts again.
	if(IsClient())
	{
		if(delaytimer)
		{
			Message(13, "You are unable to focus.");
			InterruptSpell();
			return;
		}
	}

	// make sure they aren't somehow casting 2 timed spells at once
	if (casting_spell_id != spell_id)
	{
		Message_StringID(13,ALREADY_CASTING);
		InterruptSpell();
		return;
	}

	//WR: not sure what this does, but it looks interesting (: commented until its figured out
	/*if (GetPet() && GetPet()->GetPetType() == 4)
	{
		if (slot < 8 && IsAttackAllowed(entity_list.GetMob(target_id)) && spells[spell_id].targettype == ST_Target && IsDD(spell_id) && !BeneficialSpell(spell_id) && rand()%100 < 25)
		{
			GetPet()->CastSpell(spell_id,target_id,slot);
		}
	}*/


	// here we do different things if this is a bard casting a bard song from
	// a spell bar slot
	if(GetClass() == BARD) // bard's can move when casting any spell...
	{
		if (IsBardSong(spell_id) && spells[spell_id].buffduration != 0xFFFF 
			&& spells[spell_id].recast_time == 0)
		{
			bardsong = spell_id;
			bardsong_slot = slot;
			if (!entity_list.GetMob(target_id) || (spells[spell_id].targettype != ST_Target && spells[spell_id].targettype != ST_AETarget))
				bardsong_target = this;
			else
				bardsong_target = entity_list.GetMob(target_id);
			bardsong_timer.Start(6000);
		}
	}
	else // not bard, check movement
	{
		// if has been attacked, or moved while casting
		// and this is not a bard song
		// check for regain concentration
		if
		(
			!IsBardSong(spell_id) &&
			(
				this->isattacked ||
				GetX() != GetSpellX() ||
				GetY() != GetSpellY()
			)
		)
		{
			// modify the chance based on how many times they were hit
			// but cap it so it's not that large a factor
			if(attacked_count > 15) attacked_count = 15;

			if(IsClient())
			{
				// max 93% chance at 252 skill
				channelchance = 30 + GetSkill(CHANNELING) / 400.0f * 100;
				channelchance -= attacked_count * 2;
			}
			else
			{
				// NPCs are just hard to interrupt, otherwise they get pwned
				channelchance = 85;
				channelchance -= attacked_count;
			}
		
			// solar: as you get farther from your casting location,
			// it gets squarely harder to regain concentration
			if(GetX() != GetSpellX() || GetY() != GetSpellY())
			{
				d_x = fabs(fabs(GetX()) - fabs(GetSpellX()));
				d_y = fabs(fabs(GetY()) - fabs(GetSpellY()));
				if(d_x < 5 && d_y < 5)
				{
					//avoid the square root...
					distance_moved = d_x * d_x + d_y * d_y;
					// if you moved 1 unit, that's 25% off your chance to regain.
					// if you moved 2, you lose 100% off your chance
					distancemod = distance_moved * 25;
					channelchance -= distancemod;
				}
				else
				{
					channelchance = 0;
				}
			}

#ifdef SOLAR
			printf("spell x: %f  spell y: %f  cur x: %f  cur y: %f\n", GetSpellX(), GetSpellY(), GetX(), GetY());
			printf("channelchance %f channeling skill %d\n", channelchance, GetSkill(CHANNELING));
#endif

			if(MakeRandomFloat(0, 100) > channelchance)
			{
				InterruptSpell();
				return;
			}
			// if we got here, we regained concentration
			regain_conc = true;
			Message_StringID(MT_Spells,REGAIN_AND_CONTINUE);
			entity_list.MessageClose_StringID(this, true, 200, MT_Spells, OTHER_REGAIN_CAST, this->GetCleanName());
		}
	}

	// this is common to both bard and non bard

	// we're done casting, now try to apply the spell
	if( SpellFinished(spell_id, target_id, slot, mana_used) == false )
	{
Message(13, "Spell Finished returned false, interrupting.");
		InterruptSpell();
		return;
	}

	//
	// solar: at this point the spell has successfully been cast
	//

	// if this was cast from an inventory slot, check out the item that's there
	if(IsClient() && slot == 10)	// 10 is an item
	{
		const ItemInst* inst = CastToClient()->GetInv()[inventory_slot];
		if (inst && inst->IsType(ItemTypeCommon))
		{
			//const Item_Struct* item = inst->GetItem();
			sint16 charges = inst->GetItem()->Common.MaxCharges;
			if(charges > -1)	// charged item, expend a charge
				CastToClient()->DeleteItemInInventory(inventory_slot, 1, true);
		}
		else
		{
			Message(0, "Error: item not found for inventory slot #%i", inventory_slot);
			InterruptSpell();
		}
	}

	if(UseBardSpellLogic())
	{
		if(IsClient())
		{
			switch(spells[spell_id].skill)
			{
				case SINGING:
				{
					CastToClient()->CheckIncreaseSkill(SINGING);
					break;
				}
				case PERCUSSION_INSTRUMENTS:
				{
					if(this->itembonuses.percussionMod > 0)
						CastToClient()->CheckIncreaseSkill(PERCUSSION_INSTRUMENTS);
					else
						CastToClient()->CheckIncreaseSkill(SINGING);
					break;
				}
				case STRINGED_INSTRUMENTS:
				{
					if(this->itembonuses.stringedMod > 0)
						CastToClient()->CheckIncreaseSkill(STRINGED_INSTRUMENTS);
					else
						CastToClient()->CheckIncreaseSkill(SINGING);
					break;
				}
				case WIND_INSTRUMENTS:
				{
					if(this->itembonuses.windMod > 0)
						CastToClient()->CheckIncreaseSkill(WIND_INSTRUMENTS);
					else
						CastToClient()->CheckIncreaseSkill(SINGING);
					break;
				}
				case BRASS_INSTRUMENTS:
				{
					if(this->itembonuses.brassMod > 0)
						CastToClient()->CheckIncreaseSkill(BRASS_INSTRUMENTS);
					else
						CastToClient()->CheckIncreaseSkill(SINGING);
					break;
				}

			}
		}
		// go again in 6 seconds
		DoCastSpell(casting_spell_id, casting_spell_targetid, casting_spell_slot, 6000, casting_spell_mana);
	}
	else
	{
		if(IsClient())
		{
			Client *c = CastToClient();
			SendSpellBarEnable(spell_id);

			// this causes the delayed refresh of the spell bar gems
			c->MemorizeSpell(slot, spell_id, memSpellSpellbar);

			// this tells the client that casting may happen again
			SetMana(GetMana());

			// skills
			if(slot < MAX_PP_MEMSPELL)
			{
				c->CheckIncreaseSkill(spells[spell_id].skill);
				
				// increased chance of gaining channel skill if you regained concentration
				c->CheckIncreaseSkill(CHANNELING, regain_conc ? 5 : 0);
				
				int spec_skill = GetSpecializeSkill(spell_id);
				int specialize = spec_skill < HIGHEST_SKILL ? GetSkill(spec_skill) : 0;
				if(specialize > 0)
					c->CheckIncreaseSkill(spec_skill);
			}
			
			
		}

		// there should be no casting going on now
		ZeroCastingVars();

		// set the rapid recast timer for next time around
		delaytimer = true;
		spellend_timer.Start(400,true);
	}


}

// only used from CastedSpellFinished, and procs
// solar: we can't interrupt in this, or anything called from this!
// if you need to abort the casting, return false
bool Mob::SpellFinished(int16 spell_id, int32 target_id, int16 slot, int16 mana_used)
{
	APPLAYER *outapp = NULL;
	int recourse_spell=0;
	float range;
	Mob *spell_target = NULL, *ae_center = NULL;
	
	if(!IsValidSpell(spell_id))
		return false;

	if
	(
		this->IsClient() && 
		(zone->GetZoneID() == 183 || zone->GetZoneID() == 184) && 	// load
		CastToClient()->Admin() < 80
	)
	{
		if
		(
			IsEffectInSpell(spell_id, SE_Gate) ||
			IsEffectInSpell(spell_id, SE_Translocate) ||
			IsEffectInSpell(spell_id, SE_Teleport)
		)
		{
			Message(0, "The Gods brought you here, only they can send you away.");
			return false;
		}
	}


/*
	solar: The basic types of spells:
	
	Single target - some might be undead only, self only, etc, but these
	all affect the target of the caster.

	AE around caster - these affect entities close to the caster, and have
	no target.
	
	AE around target - these have a target, and affect the target as well as
	entities close to the target.
	
	AE on location - this is a tricky one that is cast on a mob target but
	has a special AE duration that keeps it recasting every 2.5 sec on the
	same location.  These work the same as AE around target spells, except
	the target is a special beacon that's created when the spell is cast

	Group - the caster is always affected, but there's more
		targetgroupbuffs on - these affect the target and the target's group.
		targetgroupbuffs off - no target, affects the caster's group.

	Group Teleport - the caster plus his group are affected.  these cannot
	be targeted.

*/

	// during this switch, this variable gets set to one of these things
	// and that causes the spell to be executed differently
	enum CastAction_type
	{
		SingleTarget,	// causes effect to spell_target
		AETarget,			// causes effect in aerange of target + target
		AECaster,			// causes effect in aerange of 'this'
		GroupSpell,		// causes effect to caster + target's group
		Unknown
	}
	CastAction;
	
	//I think the string ID SPELL_NEED_TAR is wrong, it dosent seem to show up.
	
//
// solar: Switch #1 - determine spell target
//
	switch (spells[spell_id].targettype)
	{
// single target spells
		case ST_Self:
		{
			spell_target = this;
			CastAction = SingleTarget;
			break;
		}

		case ST_TargetOptional:
		{
			spell_target = entity_list.GetMob(target_id);
			if(!spell_target)
				spell_target = this;
			CastAction = SingleTarget;
			break;
		}

		// target required for these
		case ST_Undead: {
			spell_target = entity_list.GetMob(target_id);
			if(!spell_target || spell_target->GetBodyType() != bodyTypeUndead)
			{
				//invalid target
				Message_StringID(13,SPELL_NEED_TAR);
				return false;
			}
			CastAction = SingleTarget;
			break;
		}
		
		case ST_Summoned: {
			spell_target = entity_list.GetMob(target_id);
			if(!spell_target || spell_target->GetBodyType() != bodyTypeSummoned)
			{
				//invalid target
				Message_StringID(13,SPELL_NEED_TAR);
				return false;
			}
			CastAction = SingleTarget;
			break;
		}
		
		case ST_Animal: {
			spell_target = entity_list.GetMob(target_id);
			if(!spell_target || spell_target->GetBodyType() != bodyTypeAnimal)
			{
				//invalid target
				Message_StringID(13,SPELL_NEED_TAR);
				return false;
			}
			CastAction = SingleTarget;
			break;
		}
		
		case ST_Plant:
		case ST_Dragon:
		case ST_Giant:
		case ST_Tap:
		case ST_Target: {
			spell_target = entity_list.GetMob(target_id);
			if(!spell_target)
			{
				Message_StringID(13,SPELL_NEED_TAR);
				return false;	// can't cast these unless we have a target
			}
			CastAction = SingleTarget;
			break;
		}

		case ST_Corpse:
		{
			spell_target = entity_list.GetMob(target_id);
			if(!spell_target || !spell_target->IsPlayerCorpse())
			{
				int message = ONLY_ON_CORPSES;
				if(!spell_target) message = SPELL_NEED_TAR;
				else if(!spell_target->IsCorpse()) message = ONLY_ON_CORPSES;
				else if(!spell_target->IsPlayerCorpse()) message = CORPSE_NOT_VALID;
				Message_StringID(13,SPELL_NEED_TAR);
				return false;
			}
			CastAction = SingleTarget;
			break;
		}
		case ST_Pet:
		{
			spell_target = GetPet();
			if(!spell_target)
			{
				Message_StringID(13,NO_PET);
				return false;	// can't cast these unless we have a target
			}
			CastAction = SingleTarget;
			break;
		}

// AE spells
		case ST_AECaster:
		{
			spell_target = NULL;
			ae_center = this;
			CastAction = AECaster;
			break;
		}

		case ST_UndeadAE:	//should only affect undead...
		case ST_AETarget:
		{
			spell_target = entity_list.GetMob(target_id);
			if(!spell_target)
			{
				Message_StringID(13,SPELL_NEED_TAR);
				return false;
			}
			ae_center = spell_target;
			CastAction = AETarget;
			break;
		}

// Group spells
		// solar: TODO mass group buff AA crap.  AESpell could handle that.
		case ST_AEBard:
		case ST_GroupTeleport:
		case ST_Group:
		{
			// if targetgroupbuff is on, group spells require a target
			// note: TGB only works for regular buffs with a duration, and not
			// group illusions
			if(IsClient() && CastToClient()->TGB() && IsTGBCompatibleSpell(spell_id))
			{
				spell_target = entity_list.GetMob(target_id);
				if(!spell_target)
				{
					Message_StringID(13,SPELL_NEED_TAR);
					return false;
				}
			}
			else	// TGB off or caster isn't a client
			{
				spell_target = this;
			}
			CastAction = GroupSpell;
			break;
		}

		default:
		{
			Message(0, "I dont know that Target Type: %d   Spell: (%d) %s", spells[spell_id].targettype, spell_id, spells[spell_id].name);
			CastAction = Unknown;
			break;
		}
	}

	// solar: if a spell has the AEDuration flag, it becomes an AE on target
	// spell that's recast every 2500 msec for AEDuration msec.  There are
	// spells of all kinds of target types that do this, strangely enough
	// TODO: finish this
	if(IsAEDurationSpell(spell_id))
	{
		// solar: the spells are AE target, but we aim them on a beacon
		Mob *beacon_loc =  spell_target ? spell_target : this;
		Beacon *beacon = new Beacon(beacon_loc, spells[spell_id].AEDuration);
		entity_list.AddBeacon(beacon);
		spell_target = NULL;
		ae_center = beacon;
		CastAction = AECaster;
	}

	// solar: check line of sight to target if it's a detrimental spell
	// NOTE: remove the map files if you're having problems with this,
	// don't remove this check
	if(spell_target && IsDetrimentalSpell(spell_id) && !CheckLos(spell_target))
	{
		Message_StringID(13,CANT_SEE_TARGET);
		return false;
	}

	// Check for consumables and Reagent focus items
	// first check for component reduction...
	if(IsClient() && CastToClient()->GetFocusEffect(focusReagentCost,spell_id) < MakeRandomInt(0, 100)) {
    	Client *c = this->CastToClient();
    	int component, component_count, inv_slot_id;
	    for(int t_count = 0; t_count < 4; t_count++) {
			if(IsBardSong(spell_id)) // bard spells don't use up reagents right?
				break;
			component = spells[spell_id].components[t_count];
			component_count = spells[spell_id].component_counts[t_count];

			if (component == -1)
				continue;
			if(c->GetInv().HasItem(component, component_count, invWhereWorn|invWherePersonal) == -1) // item not found
			{
				c->Message_StringID(13, MISSING_SPELL_COMP);

				const Item_Struct *item = database.GetItem(component);
				if(item)
					c->Message_StringID(13, MISSING_SPELL_COMP_ITEM, item->Name);

				if(c->GetGM())
					c->Message(0, "Your GM status allows you to finish casting even though you're missing required components.");
				else
					return false;
			}
			else
			{
				// Components found, Deleteing
				// now we go looking for and deleting the items one by one
				for(int s = 0; s < component_count; s++)
				{
					inv_slot_id = c->GetInv().HasItem(component, 1);
					if(inv_slot_id != -1)
					{
						c->DeleteItemInInventory(inv_slot_id, 1, true);
					}
					else
					{	// some kind of error in the code if this happens
						c->Message(13, "ERROR: reagent item disappeared while processing?");
					}
				}
			}
		}
	}
	
	//
	// solar: Switch #2 - execute the spell
	//
	switch(CastAction)
	{
		default:
		case Unknown:
		case SingleTarget:
		{
			SpellOnTarget(spell_id, spell_target);
			break;
		}

		case AECaster:
		case AETarget:
		{
			range = spells[spell_id].aoerange;
			range = GetActSpellRange(spell_id, range);

			// we can't cast an AE spell without something to center it on
			assert(ae_center != NULL);

			if(ae_center->IsBeacon()) {
				// special ae duration spell
				ae_center->CastToBeacon()->AELocationSpell(this, range, spell_id);
			} else {
				// regular PB AE or targeted AE spell - spell_target is null if PB
				if(spell_target)	// this must be an AETarget spell
				{
					// affect the target too
					SpellOnTarget(spell_id, spell_target);
				}
				bool affect_caster = !IsNPC();	//NPC AE spells do not affect the NPC caster
				entity_list.AESpell(this, ae_center, range, spell_id, affect_caster);
			}
			break;
		}

		case GroupSpell:
		{
			// at this point spell_target is a member of the other group, or the
			// caster if they're not using TGB
			// NOTE: this will always hit the caster, plus the target's group so
			// it can affect up to 7 people if the targeted group is not our own
			if(spell_target->IsGrouped())
			{
				Group *target_group = entity_list.GetGroupByMob(spell_target);
				if(target_group)
				{
					target_group->CastGroupSpell(this, spell_id);
				}
			}
			else
			{
				// if target is grouped, CastGroupSpell will cast it on the caster
				// too, but if not then we have to do that here.
				SpellOnTarget(spell_id, this);
#ifdef GROUP_BUFF_PETS
				//pet too
				if (GetPet())
					SpellOnTarget(spell_id, GetPet());
#endif
			}
			break;
		}
	}

	// animation
	outapp = new APPLAYER(OP_Animation, sizeof(Animation_Struct));
	Animation_Struct* a = (Animation_Struct*)outapp->pBuffer;
	a->spawn_id = GetID();
	a->animation_speed = 10;
	a->animation = spells[spell_id].CastingAnim;
	outapp->priority = 2;
	entity_list.QueueCloseClients(this, outapp, false, 200, 0, true, IsClient() ? FILTER_PCSPELLS : FILTER_NPCSPELLS);
	safe_delete(outapp);
	
	// if this was a spell slot or an ability use up the mana for it
	// CastSpell already reduced the cost for it if we're a client with focus
	if(slot != 10 && mana_used > 0)	// 10 is item
	{
		SetMana(GetMana() - mana_used);
		
		//set our reuse timer on long ass reuse_time spells...
		if(IsClient() && spells[spell_id].recast_time > 30000) {
			int recast = spells[spell_id].recast_time/1000;
			if (spell_id == SPELL_LAY_ON_HANDS)	//lay on hands
			{
				recast -= GetAA(aaFervrentBlessing) * 420;
			}
			else if (spell_id == SPELL_HARM_TOUCH || spell_id == SPELL_HARM_TOUCH2)	//harm touch
			{
				recast -= GetAA(aaTouchoftheWicked) * 420;
			}
			CastToClient()->GetPTimers().Start(pTimerSpellStart + spell_id, recast);
		}
	}
	
	//WR: I dont know what these do... uncomment them if you do...
	/*if (spell_id == 2155)
		SpellFinished(2156,GetID());
	if (spell_id == 1994)
		SpellFinished(1995,GetID());
	*/

	// neotokyo: 09-Nov-02
	// Recourse means there is a spell linked to that spell in that the recourse spell will
	// be automatically casted on the casters group or the caster only depending on Targettype
	// solar: this is for things like dark empathy, shadow vortex
	recourse_spell = spells[spell_id].RecourseLink;
	if(recourse_spell != 0)
	{
		if(IsGrouped() && spells[recourse_spell].targettype == ST_Group)
		{
			if(IsGrouped()) {
				Group *g = entity_list.GetGroupByMob(this);;
				g->CastGroupSpell(this, recourse_spell);
			} else {
				SpellOnTarget(recourse_spell, this);
#ifdef GROUP_BUFF_PETS
				//pet too
				if (GetPet())
					SpellOnTarget(recourse_spell, GetPet());
#endif
			}
		}
		else
		{
			SpellOnTarget(recourse_spell, this);
		}
	}
		
	AI_Event_SpellCastFinished(true, slot);

	return true;	
}

///////////////////////////////////////////////////////////////////////////////
// buff related functions

// solar: returns how many _ticks_ the buff will last.
// a tick is 6 seconds
// this is the place to figure out random duration buffs like fear and charm.
// both the caster and target mobs are passed in, so different behavior can
// even be created depending on the types of mobs involved
//
// right now this is just an outline, working on this..
int CalcBuffDuration(Mob *caster, Mob *target, int16 spell_id)
{
	int formula, duration;

	if(!IsValidSpell(spell_id) || (!caster && !target))
		return 0;
	
	if(!caster && !target)
		return 0;
	
	// if we have at least one, we can make do, we'll just pretend they're the same
	if(!caster)
		caster = target;
	if(!target)
		target = caster;

	formula = spells[spell_id].buffdurationformula;
	duration = spells[spell_id].buffduration;

	return CalcBuffDuration_formula(caster->GetCasterLevel(spell_id), formula, duration);
}

// the generic formula calculations
int CalcBuffDuration_formula(int level, int formula, int duration)
{
	int i;	// temp variable

	switch(formula)
	{
		case 0:	// not a buff
			return 0;

		case 1:	// solar: 2/7/04
			i = (int)ceil(level / 2.0f);
			return i < duration ? (i < 1 ? 1 : i) : duration;

		case 2:	// solar: 2/7/04
			i = (int)ceil(duration / 5.0f * 3);
			return i < duration ? (i < 1 ? 1 : i) : duration;

		case 3:	// solar: 2/7/04
			i = level * 30;
			return i < duration ? (i < 1 ? 1 : i) : duration;

		case 4:	// only used by 'LowerElement'
			return duration;

		case 5:	// solar: 2/7/04
			i = duration;
			return i < 3 ? (i < 1 ? 1 : i) : 3;

		case 6:	// solar: 2/7/04
			i = (int)ceil(level / 2.0f);
			return i < duration ? (i < 1 ? 1 : i) : duration;

		case 7:	// solar: 2/7/04
			i = level;
			return i < duration ? (i < 1 ? 1 : i) : duration;

		case 8:	// solar: 2/7/04
			i = level + 10;
			return i < duration ? (i < 1 ? 1 : i) : duration;

		case 9:	// solar: 2/7/04
			i = level * 2 + 10;
			return i < duration ? (i < 1 ? 1 : i) : duration;

		case 10:	// solar: 2/7/04
			i = level * 3 + 10;
			return i < duration ? (i < 1 ? 1 : i) : duration;

		case 11:	// solar: confirmed 2/7/04
			return duration;

		case 12:	// solar: 2/7/04
			return duration;

		case 50:	// solar: lucy says this is unlimited?
			return 72000;	// 5 days

		case 3600:
			return duration ? duration : 3600;

		default:
			LogFile->write(EQEMuLog::Debug, "CalcBuffDuration_formula: unknown formula %d", formula);
			return 0;
	}
}

// solar: helper function for AddBuff to determine stacking
// spellid1 is the spell already worn, spellid2 is the one trying to be cast
// returns:
// 0 if not the same type, no action needs to be taken
// 1 if spellid1 should be removed (overwrite)
// -1 if they can't stack and spellid2 should be stopped
int Mob::CheckStackConflict(int16 spellid1, int caster_level1, int16 spellid2, int caster_level2)
{
	SPDat_Spell_Struct sp1 = spells[spellid1];
	SPDat_Spell_Struct sp2 = spells[spellid2];
	
	int i, effect1, effect2, sp1_value, sp2_value;
	int blocked_effect, blocked_below_value, blocked_slot;
	int overwrite_effect, overwrite_below_value, overwrite_slot;

	// an easy case
	if(spellid1 == spellid2)
	{
		if(caster_level2 >= caster_level1)
			return 1;	// overwrite
		return -1; // can't stack
	}
	
	// solar: check for special stacking block command in spell1 against spell2
	for(i = 0; i < EFFECT_COUNT; i++)
	{
		effect1 = sp1.effectid[i];
		if(effect1 == SE_StackingCommand_Block)
		{
			blocked_effect = sp1.base[i];
			blocked_slot = sp1.formula[i] - 200;
			blocked_below_value = sp1.max[i];

			if(sp2.effectid[blocked_slot] == blocked_effect)
			{
				sp2_value = CalcSpellEffectValue(spellid2, blocked_slot, caster_level2);

				if(sp2_value < blocked_below_value)
					return -1;	// blocked
				else
					return 1;		// if we can't block it, let it overwrite this
			}			
		}
	}

	// check for special stacking overwrite in spell2 against effects in spell1
	for(i = 0; i < EFFECT_COUNT; i++)
	{
		effect2 = sp2.effectid[i];
		if(effect2 == SE_StackingCommand_Overwrite)
		{
			overwrite_effect = sp2.base[i];
			overwrite_slot = sp2.formula[i] - 200;
			overwrite_below_value = sp2.max[i];
			if(sp1.effectid[overwrite_slot] == overwrite_effect)
			{
				sp1_value = CalcSpellEffectValue(spellid1, overwrite_slot, caster_level1);

				if(sp1_value < overwrite_below_value)
					return 1;			// overwrite spell if its value is less
				else
					return -1;		// if spell2 can't overwrite spell1, block spell2
			}
		}
	}

	// now compare matching effects
	// abitrartion takes place if 2 spells have th same effect at the same
	// effect slot, otherwise they're stackable, even if it's the same effect
	for(i = 0; i < EFFECT_COUNT; i++)
	{
		if(IsBlankSpellEffect(spellid1, i) || IsBlankSpellEffect(spellid2, i))
		{
			continue;
		}

		effect1 = sp1.effectid[i];
		effect2 = sp2.effectid[i];
		
		// same effect in both spells?
		if(effect1 == effect2)
		{
			// a detrimental spell will overwrite a good spell already worn
			// and vice versa
			if(IsDetrimentalSpell(spellid2) != IsDetrimentalSpell(spellid1))
				return 1;

			sp1_value = CalcSpellEffectValue(spellid1, i, caster_level1);
			sp2_value = CalcSpellEffectValue(spellid2, i, caster_level2);
			
			// some spells are hard to compare just on value.  attack speed spells
			// have a value that's a percentage for instance
			if
			(
				effect1 == SE_AttackSpeed ||
				effect1 == SE_AttackSpeed2 ||
				effect1 == SE_AttackSpeed3
			)
			{
				sp1_value -= 100;
				sp2_value -= 100;
			}
			
			if(sp1_value < 0)
				sp1_value = 0 - sp1_value;
			if(sp2_value < 0)
				sp2_value = 0 - sp2_value;
			
			if(sp2_value >= sp1_value)
				return 1;	// overwrite
			return -1;	// can't stack
		}
	}

	return 0;
}

// returns the slot the buff was added to, -1 if it wasn't added due to
// stacking problems, and -2 if this is not a buff
// if caster is null, the buff will be added with the caster level being
// the level of the mob
int Mob::AddBuff(Mob *caster, int16 spell_id, int duration)
{
	int buffslot, ret, caster_level, emptyslot = -1;
	bool will_overwrite = false;
	Buffs_Struct curbuf;
	
	caster_level = caster ? caster->GetCasterLevel(spell_id) : GetCasterLevel(spell_id);
    
	if(!duration)
	{
		duration = CalcBuffDuration(caster, this, spell_id);
		duration = GetActSpellDuration(spell_id, duration);
	}

	if(!duration)
		return -2;	// no duration? this isn't a buff

	// solar: first we loop through everything checking that the spell
	// can stack with everything.  this is to avoid stripping the spells
	// it would overwrite, and then hitting a buff we can't stack with.
	// we also check if overwriting will occur.  this is so after this loop
	// we can determine if there will be room for this buff
	for(buffslot = 0; buffslot < BUFF_COUNT; buffslot++)
	{
		curbuf = buffs[buffslot];

		if(curbuf.spellid != SPELL_UNKNOWN)
		{
			// there's a buff in this slot
			ret = CheckStackConflict(curbuf.spellid, curbuf.casterlevel, spell_id, caster_level);
			if(ret == -1)	// stop the spell
				return -1;
			if(ret == 1)	// set a flag to indicate that there will be overwriting
				will_overwrite = true;
		}
		else
		{
			if(emptyslot == -1)
				emptyslot = buffslot;
		}
	}
	
	// we didn't find an empty slot to put it in, and it's not overwriting
	// anything so there must not be any room left.
 	if(emptyslot == -1 && !will_overwrite)
 	//	return -1;
 	{  
 		if(IsDetrimentalSpell(spell_id)) //Sucks to be you, bye bye one of your buffs
 		{
 			for(buffslot = 0; buffslot < BUFF_COUNT; buffslot++)
 			{
 				curbuf = buffs[buffslot];
 				if(IsBeneficialSpell(curbuf.spellid))
 				{
 					BuffFadeBySlot(buffslot,false);
 					emptyslot = buffslot;
 					buffslot = BUFF_COUNT+1;//break out of the loop
 				}
 			}
 		}
 		else
 			return -1;
 	}

	// solar: at this point we know that this buff will stick, but we have
	// to remove some other buffs already worn if will_overwrite is true
	// so we loop through again
	if(will_overwrite)
	{
		for(buffslot = 0; buffslot < BUFF_COUNT; buffslot++)
		{
			curbuf = buffs[buffslot];

			if(curbuf.spellid != SPELL_UNKNOWN)
			{
				ret = CheckStackConflict(curbuf.spellid, curbuf.casterlevel, spell_id, caster_level);
				if(ret == 1)
				{
					// strip spell
					BuffFadeBySlot(buffslot, false);

					// if we hadn't found a free slot before, or if this is earlier
					// we use it
					if(emptyslot == -1 || buffslot < emptyslot)
						emptyslot = buffslot;
				}
			}
		}
	}

	// now add buff at emptyslot
	assert(buffs[emptyslot].spellid == SPELL_UNKNOWN);	// sanity check
			
	buffs[emptyslot].spellid = spell_id;
	buffs[emptyslot].casterlevel = caster_level;
	buffs[emptyslot].casterid = caster ? caster->GetID() : 0;
	buffs[emptyslot].durationformula = spells[spell_id].buffdurationformula;
	buffs[emptyslot].ticsremaining = duration;
	buffs[emptyslot].client = caster ? caster->IsClient() : 0;

	// recalculate bonuses since we stripped/added buffs
	CalcBonuses();

	return emptyslot;
}

// solar: used by some MobAI stuff
// NOT USED BY SPELL CODE
// note that this should not be used for determining which slot to place a 
// buff into
// returns -1 on stack failure, -2 if all slots full, the slot number if the buff should overwrite another buff, or a free buff slot
int Mob::CanBuffStack(int16 spellid, int8 caster_level, bool iFailIfOverwrite)
{
	int i, ret, firstfree = -2;
	Buffs_Struct curbuf;


	for (i=0; i < BUFF_COUNT; i++)
	{
		curbuf = buffs[i];

		// no buff in this slot
		if (curbuf.spellid == SPELL_UNKNOWN)
		{
			// if we haven't found a free slot, this is the first one so save it
			if(firstfree == -2)
				firstfree = i;
			continue;
		}

		// there's a buff in this slot
		ret = CheckStackConflict(curbuf.spellid, curbuf.casterlevel, spellid, caster_level);
		if(ret == 1) return iFailIfOverwrite ? -1 : i;		// overwrite current slot
		if(ret == -1) return -1;	// stop the spell, can't stack it
	}

	return firstfree;
}

///////////////////////////////////////////////////////////////////////////////
// spell effect related functions

//
// solar:
// this is actually applying a spell cast from 'this' on 'spelltar'
// it performs pvp checking and applies resists, etc then it
// passes it to SpellEffect which causes effects to the target
//
// this is called by these functions:
// Mob::SpellFinished
// Entity::AESpell (called by Mob::SpellFinished)
// Group::CastGroupSpell (called by Mob::SpellFinished)
//
// also note you can't interrupt the spell here. at this point it's going
// and if you don't want effects just return false.  interrupting here will
// break stuff
//
bool Mob::SpellOnTarget(int16 spell_id, Mob* spelltar)
{
	APPLAYER *action_packet, *message_packet;
	double spell_effectiveness;

	if(!IsValidSpell(spell_id))
		return false;

	// well we can't cast a spell on target without a target
	if(!spelltar)
	{
		Message(13, "SOT: You must have a target for this spell.");
		return false;
	}
	
	int16 caster_level = GetCasterLevel(spell_id);

	// Actual cast action - this causes the caster animation and the particles
	// around the target
	// solar: we do this first, that way we get the particles even if the spell
	// doesn't land due to pvp protection
	// note: this packet is sent again if the spell is successful, with a flag
	// set
	action_packet = new APPLAYER(OP_Action, sizeof(Action_Struct));
	Action_Struct* action = (Action_Struct*) action_packet->pBuffer;

	// select source
	if(IsClient() && CastToClient()->GMHideMe())
	{
		action->source = spelltar->GetID();
	}
	else
	{
		action->source = GetID();
		// solar: this is a hack that makes detrimental buffs work client to client
		// TODO figure out how to do this right
		if
		(
			IsDetrimentalSpell(spell_id) &&
			IsClient() &&
			spelltar->IsClient()
		)
		{
			action->source = spelltar->GetID();
		}
	}

	// select target
	if	// Bind Sight line of spells 
	(
		spell_id == 500 || 	// bind sight
		spell_id == 407 		// cast sight
	)
	{ 
		action->target = GetID(); 
	} 
	else
	{ 
		action->target = spelltar->GetID(); 
	} 

	action->level = caster_level;	// caster level, for animation only
	action->type = 231;	// 231 means a spell
	action->spell = spell_id;
	action->sequence = (int32) (GetHeading() * 2);	// just some random number
	action->unknown06 = 0x0A;	// seems to always be 0x0A (10)
	action->buff_unknown = 0;

	if(spelltar->IsClient())	// send to target
		spelltar->CastToClient()->QueuePacket(action_packet);
	if(IsClient())	// send to caster
		CastToClient()->QueuePacket(action_packet);
	// send to people in the area, ignoring caster and target
	entity_list.QueueCloseClients(spelltar, action_packet, true, 200, this, true, IsClient() ? FILTER_PCSPELLS : FILTER_NPCSPELLS);

// end of action packet


	// solar: now check if the spell is allowed to land

#ifdef GUILDWARS
	if(spelltar->GetInvul())
	{
		// selfcast of translocate for GW
		if( !(spelltar == this && IsEffectInSpell(spell_id, SE_Translocate)) )
		{
			printf("spell can't take hold due to invulnerability; %s -> %s\n", GetName(), spelltar->GetName());
			return false;
		}
	}
#else
	// invuln mobs can't be affected by any spells, good or bad
	if(spelltar->GetInvul() || spelltar->DivineAura())
		return false;
#endif

	if(!(IsClient() && CastToClient()->GetGM()))	// GMs can cast on anything
	{
		// Beneficial spells check
		if(IsBeneficialSpell(spell_id))
		{
			if
			(
				spelltar != this &&
				(
					!IsBeneficialAllowed(spelltar) ||
					(
						!IsNPC() &&
						IsGroupOnlySpell(spell_id) &&
						!(
							entity_list.GetGroupByMob(this) &&
							entity_list.GetGroupByMob(this)->IsGroupMember(spelltar)
						)
					)
				)
			)
			{
#ifdef SOLAR
				printf("beneficial spell can't take hold %s -> %s\n", GetName(), spelltar->GetName());
#endif
				Message_StringID(MT_Shout, SPELL_NO_HOLD);
				return false;
			}
		}
		else if	// Detrimental spells - PVP check
		(
			!IsAttackAllowed(spelltar)
#ifdef GUILDWARS	// can't cast bad spells on yourself on GW
			|| spelltar == this
#endif
		)
		{
#ifdef SOLAR
			printf("detrimental spell can't take hold %s -> %s\n", GetName(), spelltar->GetName());
#endif
			spelltar->Message_StringID(MT_Shout, YOU_ARE_PROTECTED, GetCleanName());
			return false;
		}
	}

	// solar: ok at this point the spell is permitted to affect the target,
	// but we need to check special cases and resists


	// check immunities
	if(spelltar->IsImmuneToSpell(spell_id, this))
	{
#ifdef SOLAR
		printf("spell can't take hold due to immunity %s -> %s\n", GetName(), spelltar->GetName());
#endif
		return false;
	}

	// solar: resist check - every spell can be resisted, beneficial or not
	// add: ok this isn't true, eqlive's spell data is fucked up, buffs are
	// not all unresistable, so changing this to only check certain spells
	if(IsResistableSpell(spell_id))
	{
		spell_effectiveness = spelltar->ResistSpell(spell_id, this);
		if(spell_effectiveness < 100)
		{
			if(!IsPartialCapableSpell(spell_id) || spell_effectiveness == 0)
			{
				Message_StringID(MT_Shout, TARGET_RESISTED, spells[spell_id].name);
				spelltar->Message_StringID(MT_Shout, YOU_RESIST, spells[spell_id].name);

				if(spelltar->IsAIControlled())
					spelltar->AddToHateList(this, 1);

				return false;
			}
		}
	}
	else
	{
		spell_effectiveness = 100;
	}

	if(spell_id == 982)	// Cazic Touch, hehe =P
	{
		char target_name[64];
		strcpy(target_name, spelltar->GetCleanName());
		strupr(target_name);
		Shout("%s!", target_name);
	}

	if (spelltar->IsAIControlled() && spells[spell_id].goodEffect == 0
		//IsDetrimentalSpell(spell_id)
		) {
		int16 aggro_amount = CheckAggroAmount(spell_id);//*spelltar->CastToNPC()->AggroModifier();
		if (IsClient()) {
			switch (GetAA(aaSpellCastingSubtlety))
			{
			case 1:
				aggro_amount = aggro_amount * 95 / 100;
				break;
			case 2:
				aggro_amount = aggro_amount * 90 / 100;
				break;
			case 3:
				aggro_amount = aggro_amount * 80 / 100;
				break;
			}
		}
		if (spell_effectiveness < 100)
			aggro_amount /= 2;
		spelltar->AddToHateList(this, aggro_amount);
	}
	else if (IsBeneficialSpell(spell_id))
		entity_list.AddHealAggro(spelltar, this, CheckHealAggroAmount(spell_id));

	// cause the effects to the target
	if(!spelltar->SpellEffect(this, spell_id, spell_effectiveness))
	{
		// solar: if SpellEffect returned false there's a problem applying the
		// spell.  It's most likely a buff that can't stack.
#ifdef SOLAR
		printf("spell effect cannot take hold %s -> %s\n", GetName(), spelltar->GetName());
#endif
		Message_StringID(MT_Shout, SPELL_NO_HOLD);
		return false;
	}

	// solar: send the action packet again now that the spell is successful
	// NOTE: this is what causes the buff icon to appear on the client, if
	// this is a buff - but it sortof relies on the first packet.
	// the complete sequence is 2 actions and 1 damage message
	action->buff_unknown = 0x04;	// this is a success flag
	if(spelltar->IsClient())	// send to target
		spelltar->CastToClient()->QueuePacket(action_packet);
	if(IsClient())	// send to caster
		CastToClient()->QueuePacket(action_packet);
	// send to people in the area, ignoring caster and target
	entity_list.QueueCloseClients(spelltar, action_packet, true, 200, this, true, IsClient() ? FILTER_PCSPELLS : FILTER_NPCSPELLS);

	// solar: TEMPORARY - this is the message for the spell.
	// double message on effects that use ChangeHP - working on this
	message_packet = new APPLAYER(OP_Damage, sizeof(CombatDamage_Struct));
	CombatDamage_Struct *cd = (CombatDamage_Struct *)message_packet->pBuffer;
	cd->target = action->target;
	cd->source = action->source;
	cd->type = action->type;
	cd->spellid = action->spell;
	cd->sequence = action->sequence;
	cd->damage = 0;
	entity_list.QueueCloseClients(spelltar, message_packet, false, 200, 0, true, IsClient() ? FILTER_PCSPELLS : FILTER_NPCSPELLS);

	safe_delete(action_packet);
	safe_delete(message_packet);

	return true;
}

void Corpse::CastRezz(int16 spellid, Mob* Caster){
/*
	if (!rezzexp) {
		Caster->Message(4, "You cannot resurrect this corpse");
		return;
	}
*/

	APPLAYER* outapp = new APPLAYER(OP_RezzRequest, sizeof(Resurrect_Struct));
	Resurrect_Struct* rezz = (Resurrect_Struct*) outapp->pBuffer;
	memcpy(rezz->your_name,this->orgname,30);
	memcpy(rezz->corpse_name,this->name,30);
	memcpy(rezz->rezzer_name,Caster->GetName(),30);
	memcpy(rezz->zone,zone->GetShortName(),15);
	rezz->spellid = spellid;
	rezz->x = this->x_pos;
	rezz->y = this->y_pos;
	rezz->z = (float)this->z_pos;
	worldserver.RezzPlayer(outapp, rezzexp, OP_RezzRequest);
	//DumpPacket(outapp);
	safe_delete(outapp);
}

bool Mob::FindBuff(int16 spellid)
{
	int i;

	for(i = 0; i < BUFF_COUNT; i++)
		if(buffs[i].spellid == spellid)
			return true;

	return false;
}

// solar: removes all buffs
void Mob::BuffFadeAll()
{
	for (int j = 0; j < BUFF_COUNT; j++)
		BuffFadeBySlot(j, false);

	CalcBonuses();
}

// solar: removes the buff matching spell_id
void Mob::BuffFadeBySpellID(int16 spell_id)
{
	for (int j = 0; j < BUFF_COUNT; j++)
	{
		if (buffs[j].spellid == spell_id)
			BuffFadeBySlot(j, false);
	}

	CalcBonuses();
}

// solar: removes buffs containing effectid, skipping skipslot
void Mob::BuffFadeByEffect(int effectid, int skipslot)
{
	int i;

	for(i = 0; i < BUFF_COUNT; i++)
	{
		if(IsEffectInSpell(buffs[i].spellid, effectid) && i != skipslot)
			BuffFadeBySlot(i, false);
	}

	CalcBonuses();
}

// solar: removes the buff in the buff slot 'slot'
void Mob::BuffFadeBySlot(int slot, bool iRecalcBonuses)
{
	if(slot < 0 || slot > BUFF_COUNT)
		return;

	if(!IsValidSpell(buffs[slot].spellid))
		return;

	if (this->IsClient())
		this->CastToClient()->MakeBuffFadePacket(buffs[slot].spellid, slot);

	for (int i=0; i < EFFECT_COUNT; i++)
	{
		if(IsBlankSpellEffect(buffs[slot].spellid, i))
			continue;

		switch (spells[buffs[slot].spellid].effectid[i])
		{
			case SE_SummonHorse:
			{
				if(IsClient())
				{
					Mob* horse = entity_list.GetMob(this->CastToClient()->GetHorseId());
					if (horse) horse->Depop();
					CastToClient()->SetHasMount(false);
				}
				break;
			}

			case SE_Illusion:
			{
				SendIllusionPacket(0, GetBaseGender());
				break;
			}

			case SE_Levitate:
			{
				SendAppearancePacket(AT_Levitate, 0);
				break;
			}

			case SE_Invisibility:
			{
				SetInvisible(false);
				break;
			}

			case SE_InvisVsUndead:
			{
				invisible_undead = false;	// Mongrel: No longer IVU
				break;
			}

			case SE_DivineAura:
			{
				SetInvul(false);
				break;
			}

			case SE_Rune:
			{
				SetRune(0);
				break;
			}

			case SE_AbsorbMagicAtt:
			{
				SetMagicRune(0);
				break;
			}

			case SE_Familiar:
			{
				Mob * myfamiliar = GetFamiliar();
				if (!myfamiliar)
					break; // familiar already gone
				myfamiliar->CastToNPC()->Depop();
				SetFamiliarID(0);
				break;
			}

			case SE_Mez:
			{
				SendAppearancePacket(AT_Anim, ANIM_STAND);	// unfreeze
				this->mezzed = false;
				break;
			}

			case SE_Charm:
			{
				Mob* tempmob = GetOwner();
				SetOwnerID(0);
				if(tempmob)
				{
					tempmob->SetPet(0);
				}
				if (IsAIControlled())
				{
					// clear the hate list of the mobs
					entity_list.ReplaceWithTarget(this, tempmob);
					WhipeHateList();
					if(tempmob)
						AddToHateList(tempmob, 1, 0);
					SendAppearancePacket(AT_Anim, ANIM_STAND);
				}
				if(tempmob && tempmob->IsClient())
				{
					APPLAYER *app = new APPLAYER(OP_Charm, sizeof(Charm_Struct));
					Charm_Struct *ps = (Charm_Struct*)app->pBuffer;
					ps->owner_id = tempmob->GetID();
					ps->pet_id = this->GetID();
					ps->command = 0;
					tempmob->CastToClient()->FastQueuePacket(&app);
				}
				if(IsClient())
				{
					if (this->CastToClient()->IsLD())
						AI_Start(CLIENT_LD_TIMEOUT);
					else
						AI_Stop();
				}
				break;
			}

			case SE_Root:
			{
				rooted = false;
				break;
			}
		}
	}



	// notify caster of buff that it's worn off
	Mob *p = entity_list.GetMob(buffs[slot].casterid);
	if (p && p->IsClient() && p != this && !IsBardSong(buffs[slot].spellid))
	{
		p->Message_StringID(MT_Broadcasts, SPELL_WORN_OFF_OF,
			spells[buffs[slot].spellid].name, GetCleanName());
	}

	buffs[slot].spellid = SPELL_UNKNOWN;

	if (iRecalcBonuses)
		CalcBonuses();
}

// solar: checks if 'this' can be affected by spell_id from caster
// returns true if the spell should fail, false otherwise
bool Mob::IsImmuneToSpell(int16 spell_id, Mob *caster)
{
	int effect_index;

	assert(caster != NULL);
	
	//TODO: this function loops through the effect list for 
	//this spell like 10 times, this could easily be consolidated
	//into one loop through with a switch statement.

	if(!IsValidSpell(spell_id))
	{
		return true;
	}

	if(IsMezSpell(spell_id))
	{
		if
		(
			SpecAttacks[UNMEZABLE] ||
			(IsNPC() && CastToNPC()->HasBanishCapability() == 101)
		)
		{
			caster->Message_StringID(MT_Shout, CANNOT_MEZ);
			return true;
		}

		// check max level for spell
		effect_index = GetSpellEffectIndex(spell_id, SE_Mez);
		assert(effect_index >= 0);
		if(GetLevel() > spells[spell_id].max[effect_index])
		{
			caster->Message_StringID(MT_Shout, CANNOT_MEZ_WITH_SPELL);
			return true;
		}
	}

	// solar: stun spells are special.  we only issue a warning here, and it's
	// also checked in SpellEffect() where the effect is skipped.  This check
	// is only for the message, the real stun checking is done there.
	if(SpecAttacks[UNSTUNABLE] && (IsStunSpell(spell_id) || IsEffectInSpell(spell_id, SE_SpinTarget)))
	{
		caster->Message_StringID(MT_Shout, IMMUNE_STUN);
		return true;
	}

	// slow and haste spells
	if(SpecAttacks[UNSLOWABLE] && IsEffectInSpell(spell_id, SE_AttackSpeed))
	{
		caster->Message_StringID(MT_Shout, IMMUNE_ATKSPEED);
		return true;
	}

	// client vs client fear
	if(IsEffectInSpell(spell_id, SE_Fear))
	{
		if(IsClient() && caster->IsClient())
		{
			caster->Message_StringID(MT_Shout, IMMUNE_FEAR);
			return true;
		}
	}

	if(IsCharmSpell(spell_id))
	{
		if(SpecAttacks[UNCHARMABLE])
		{
			caster->Message_StringID(MT_Shout, CANNOT_CHARM);
			return true;
		}

		if(this == caster)
		{
			caster->Message(MT_Shout, "You cannot charm yourself.");
			return true;
		}

		// solar: clients cannot be charmed right now.  Maybe this will change
		// at some point.
		if(IsClient())
		{
			caster->Message_StringID(MT_Shout, CANNOT_AFFECT_PC);
			return true;
		}

		// check level limit of charm spell
		effect_index = GetSpellEffectIndex(spell_id, SE_Charm);
		assert(effect_index >= 0);
		if(GetLevel() > spells[spell_id].max[effect_index])
		{
			caster->Message_StringID(MT_Shout, CANNOT_CHARM_YET);
			return true;
		}
	}

	if
	(
		IsEffectInSpell(spell_id, SE_Root) ||
		IsEffectInSpell(spell_id, SE_MovementSpeed)
	)
	{
		if(SpecAttacks[UNSNAREABLE]) {
			caster->Message_StringID(MT_Shout, IMMUNE_MOVEMENT);
			return true;
		}
		
		int8 buffslot = GetBuffSlotFromType(SE_MovementSpeed);
		if((FindType(SE_Root) && IsEffectInSpell(spell_id, SE_MovementSpeed)) 
			|| (buffslot!=255 && buffs[buffslot].spellid > 0 && buffs[buffslot].spellid < (int32)SPDAT_RECORDS 
			&& IsDetrimentalSpell(buffs[buffslot].spellid) && IsBeneficialSpell(spell_id)))
		{
			caster->Message_StringID(MT_Shout,CANNOT_AFFECT_PC);
			return true;
		}
	}

	if(IsLifetapSpell(spell_id))
	{
		if(this == caster)
		{
			caster->Message_StringID(MT_Shout, CANT_DRAIN_SELF);
			return true;
		}
	}

	if(IsSacrificeSpell(spell_id))
	{
		if(this == caster)
		{
			caster->Message_StringID(MT_Shout, CANNOT_SAC_SELF);
			return true;
		}
	}

	return false;
}

//
// solar: spell resists:
// returns an effectiveness index from 0 to 100.  for most spells, 100 means
// it landed, and anything else means it was resisted; however there are some
// spells that can be partially effective, and this value can be used there.
//
double Mob::ResistSpell(int16 spell_id, Mob *caster)
{
	int caster_level, target_level, resist;
	double roll, roll2, effectiveness_index;
	double no_resist_chance, full_hit_cutoff, partial_hit_cutoff;
	
	int8 resist_type = spells[spell_id].resisttype;
	
	if (spell_id == 0) //elem damage!
	{
		adverrorinfo = 91;
		int castlevel = caster->GetLevel();
		int targlevel = GetLevel();
		if (castlevel > 60)
			castlevel -= (castlevel-60)/2;
		if (targlevel > 60)
			targlevel -= (targlevel-60)/2;
		int variance = (castlevel-targlevel)*5;
		if ((caster->IsClient() && variance > 0) || (IsClient() && variance < 0)) //Levels shouldn't matter as much against players.
			variance /= 5;
		if (variance < -50)
			return true;
		if (variance < -25)
			variance *= 2;
		
		int targMR = GetResist(resist_type);
		
		if (GetLevel() < 50)
		{
			targMR -= (targMR*(caster->GetLevel()-50)/100);
		}
		int resistchance = (targMR + spells[spell_id].ResistDiff - variance);
		resistchance /= 2;

		if (rand()%100 < resistchance)
			return 100;
		return 0;
	}
	
	if(!IsValidSpell(spell_id))
	{
		return 0;
	}
	
	if(SpecAttacks[IMMUNE_MAGIC]) {
		return(0);
	}

	target_level = GetLevel();
	caster_level = caster ? caster->GetCasterLevel(spell_id) : target_level;

	// if NPC target and more than X levels above caster, it's always resisted
	if(IsNPC() && target_level - caster_level > AUTO_RESIST_LEVEL_DIFF)
		return 0;

	switch(resist_type)
	{
		case RESIST_NONE:	// unresistable
			return 100;

		case RESIST_MAGIC:
			resist = GetMR();
			break;

		case RESIST_FIRE:
			resist = GetFR();
			break;
		
		case RESIST_COLD:
			resist = GetCR();
			break;
		
		case RESIST_POISON:
			resist = GetPR();
			break;
		
		case RESIST_DISEASE:
			resist = GetDR();
			break;
		
		// solar: I don't know how to calculate this stuff
		case RESIST_CHROMATIC:
		case RESIST_PRISMATIC:
		case RESIST_PHYSICAL:
		default:
			resist = GetMR();
			break;
	}

	// resistant discipline bonus 
	if(IsClient() && CastToClient()->disc_inuse == discResistant)
	{
		int level = GetLevel();
		if (level <= 32)
			resist += 3;
		else if (level >= 33 && level <= 35)
			resist += 4;
		else if (level >= 36 && level <= 38)
			resist += 5;
		else if (level >= 39 && level <= 41)
			resist += 6;
		else if (level >= 42 && level <= 44)
			resist += 7;
		else if (level >= 45 && level <= 47)
			resist += 8;
		else if (level >= 48 && level <= 49)
			resist += 9;
		else if (level >= 50)
			resist += 10;
	}

	// value in spell to adjust base resist by
	resist += spells[spell_id].ResistDiff;

	//
	// solar: at this point we have:
	//
	//   resist: a value of the target's resist, generally around 0 - 300
	//   target_level: level of target
	//   caster_level: level of caster
	//

	//
	// solar:
	// the general concept is that we will do a random roll and get a number
	// from say 1 to 1000.
	// we divide this range into distinct sections to indicate the action
	// that is to happen, one of no resist, partial resist, full resist.
	// we assign an arbitrary likelyhood to each situation, with no variables.
	// no resist: 90%
	// resist: (the partial and total percentages are of the % that is resists)
	//   partial: 70%
	//   total: 30%
	// now given this, we can adjust the no resist % based on level difference
	// and resists to shift the whole scale
	//	


	// our base chance to land the spell assuming zero resist and same level.
	// this is out of 100
	no_resist_chance = 90;
	
	float lvldiff = caster_level - target_level;
	float level_adj = lvldiff * lvldiff + 1;
	// level_adj is a positive value indicating the magnitude of the adjustment
	no_resist_chance += level_adj * (caster_level > target_level ? 1 : -1);

	// now we add the resistance we have
	no_resist_chance -= resist / 2.0;
	
	//still working on this...
	if (IsFearSpell(spell_id)) {
		sint16 rchance = 0;
		switch (GetAA(aaFearResistance))
		{
			case 1:
				rchance += 5;
				break;
			case 2:
				rchance += 10;
				break;
			case 3:
				rchance += 20;
				break;
		}
		rchance += itembonuses.StunResist + spellbonuses.StunResist;
		
		//I dont think these should get factored into standard spell resist...
		if(MakeRandomInt(0, 99) < rchance) {
			return(0);
		}
	}
	
	//this is prolly wrong, but I dont see a good way to roll
	//it into the rest of this stuff
	sint16 bonus_resists = spellbonuses.ResistSpellChance + itembonuses.ResistSpellChance;
	no_resist_chance -= bonus_resists;

//this calculation is all fucked up....	

	roll = MakeRandomFloat(0, 1000);
	// figure out cutoff points
	full_hit_cutoff = 1000 * no_resist_chance / 100;


	if(roll < full_hit_cutoff)	// spell landed
	{
		effectiveness_index = 100;
	}
	else
	{
		partial_hit_cutoff = (1000 - full_hit_cutoff) * 0.70; // 70% chance for partial
		roll2 = 1000 - roll;

		if(roll2 < partial_hit_cutoff)	// partial
		{
			effectiveness_index = roll2 * 100 / partial_hit_cutoff;
		}
		else	// resisted
		{
			effectiveness_index = 0;
		}
	}

#ifdef SOLAR
	printf("ResistSpell: chance to land: %f%%  roll: %f cutoff: %f  effectiveness: %f\n", no_resist_chance, roll, full_hit_cutoff, effectiveness_index);
#endif

	return effectiveness_index;
}

///////////////////////////////////////////////////////////////////////////////
// 'other' functions

void Mob::Spin() {
	APPLAYER* outapp = new APPLAYER(OP_Action, sizeof(Action_Struct));
	outapp->pBuffer[0] = 0x0B;
	outapp->pBuffer[1] = 0x0A;
	outapp->pBuffer[2] = 0x0B;
	outapp->pBuffer[3] = 0x0A;
	outapp->pBuffer[4] = 0xE7;
	outapp->pBuffer[5] = 0x00;
	outapp->pBuffer[6] = 0x4D;
	outapp->pBuffer[7] = 0x04;
	outapp->pBuffer[8] = 0x00;
	outapp->pBuffer[9] = 0x00;
	outapp->pBuffer[10] = 0x00;
	outapp->pBuffer[11] = 0x00;
	outapp->pBuffer[12] = 0x00;
	outapp->pBuffer[13] = 0x00;
	outapp->pBuffer[14] = 0x00;
	outapp->pBuffer[15] = 0x00;
	outapp->pBuffer[16] = 0x00;
	outapp->pBuffer[17] = 0x00;
	outapp->pBuffer[18] = 0xD4;
	outapp->pBuffer[19] = 0x43;
	outapp->pBuffer[20] = 0x00;
	outapp->pBuffer[21] = 0x00;
	outapp->pBuffer[22] = 0x00;
	outapp->priority = 5;
	CastToClient()->QueuePacket(outapp);
	safe_delete(outapp);
}

void Mob::SendSpellBarDisable()
{
	if (!IsClient())
		return;
	
	CastToClient()->MemorizeSpell(0, SPELLBAR_UNLOCK, memSpellSpellbar);
}

// solar: this puts the spell bar back into a usable state fast
void Mob::SendSpellBarEnable(int16 spell_id)
{
	if(!IsClient())
		return;

	APPLAYER *outapp = new APPLAYER(OP_ManaChange, sizeof(ManaChange_Struct));
	ManaChange_Struct* manachange = (ManaChange_Struct*)outapp->pBuffer;
	manachange->new_mana = GetMana();
	manachange->spell_id = spell_id;
	manachange->stamina = 6000;
	outapp->priority = 6;
	CastToClient()->QueuePacket(outapp);
	safe_delete(outapp);
}

void Mob::Stun(int duration)
{
	if(casting_spell_id)
		InterruptSpell();

	if(duration > 0)
	{
		stunned = true;
		stunned_timer.Start(duration);
	}
}
		
// Hogie - Stuns "this"
void Client::Stun(int duration)
{
	Mob::Stun(duration);

	APPLAYER* outapp = new APPLAYER(OP_Stun, sizeof(Stun_Struct));
	Stun_Struct* stunon = (Stun_Struct*) outapp->pBuffer;
	stunon->duration = duration;
	outapp->priority = 5;
	QueuePacket(outapp);
	safe_delete(outapp);
}

void NPC::Stun(int duration)
{
	if(HasBanishCapability() == 101)
		return;

	Mob::Stun(duration);
	SetRunAnimSpeed(0);
	SendPosition();
}

void Mob::Mesmerize()
{
	mezzed = true;

	if (casting_spell_id)
		InterruptSpell();

/* this stuns the client for max time, with no way to break it -solar
	if (this->IsClient()){
		APPLAYER* outapp = new APPLAYER(OP_Stun, sizeof(Stun_Struct));
		Stun_Struct* stunon = (Stun_Struct*) outapp->pBuffer;
		stunon->duration = 0xFFFF;
		this->CastToClient()->QueuePacket(outapp);
		safe_delete(outapp);
    } else {
		SetRunAnimSpeed(0);
	}
*/
}

void Client::MakeBuffFadePacket(int16 spell_id, int slot_id, bool send_message)
{
	APPLAYER* outapp;
	
	outapp = new APPLAYER(OP_Buff, sizeof(SpellBuffFade_Struct));
	SpellBuffFade_Struct* sbf = (SpellBuffFade_Struct*) outapp->pBuffer;

	sbf->entityid=GetID();
	// solar: i dont know why but this works.. for now
	sbf->slot=2;
//	sbf->slot=m_pp.buffs[slot_id].slotid;
//	sbf->level=m_pp.buffs[slot_id].level;
//	sbf->effect=m_pp.buffs[slot_id].effect;
	sbf->spellid=spell_id;
	sbf->slotid=slot_id;
	sbf->bufffade = 1;
#if EQDEBUG >= 11
	printf("Sending SBF 1 from server:\n");
	DumpPacket(outapp);
#endif
	QueuePacket(outapp);

/*
	sbf->effect=0;
	sbf->level=0;
	sbf->slot=0;
*/
	sbf->spellid=0xffffffff;
#if EQDEBUG >= 11
	printf("Sending SBF 2 from server:\n");
	DumpPacket(outapp);
#endif
	QueuePacket(outapp);
	safe_delete(outapp);
	
	if(send_message)
	{
		const char *fadetext = spells[spell_id].spell_fades;
		sint32 color = MT_Spells;
		outapp = new APPLAYER(OP_BuffFadeMsg, sizeof(color) + strlen(fadetext) + 1);
		*(sint32 *)outapp->pBuffer = color;
		char *bufptr = (char *)outapp->pBuffer + sizeof(color);
		memcpy(bufptr,fadetext,strlen(fadetext));
		QueuePacket(outapp);
		safe_delete(outapp);
	}
}

void Client::MakeHorseSpawnPacket(int16 spell_id) {
	if(!hasmount) {
		// Spell: 2862 Tan Rope
		// Spell: 2863 Tan Leather
		// Spell: 2864 Tan Silken
		// Spell: 2865 Brown Chain
		// Spell: 2866 Tan Ornate Chain
		// Spell: 2867 White Rope
		// Spell: 2868 White Leather
		// Spell: 2869 White Silken
		// Spell: 2870 White Chain
		// Spell: 2871 White Ornate Chain
		// Spell: 2872 Black Rope
		// Spell: 2919 Tan Rope
		// Spell: 2918 Guide
		// Spell: 2917 Black Chain,		
		
		// No Horse, lets get them one.
	NPCType* npc_type = new NPCType;
  		memset(npc_type, 0, sizeof(NPCType));
  		char f_name[64];
 		char mount_color=0;
  		strcpy(f_name,this->GetCleanName());
  		strcat(f_name,"`s_Mount");
  		strcpy(npc_type->name,f_name);
  		npc_type->cur_hp = 1; 
  		npc_type->max_hp = 1; 
 		npc_type->race = 216;
 		npc_type->gender = (spell_id >= 3813 && spell_id <= 3832) ? 1 : 0; // Drogmor's are female horses. Yuck.
  		npc_type->class_ = 1; 
  		npc_type->deity= 1;
  		npc_type->level = 1;
 		npc_type->npc_id = 0;
 		npc_type->loottable_id = 0;
 
 		switch(spell_id) {
 			case 2862:
 				mount_color=0;  // Brown horse
 				npc_type->walkspeed=MOUNT_SLOW1_WALK;
 				npc_type->runspeed=MOUNT_SLOW1_RUN;
 				break;
 			case 2863:
 				mount_color=0;  // Brown horse
 				npc_type->walkspeed=MOUNT_SLOW2_WALK;
 				npc_type->runspeed=MOUNT_SLOW2_RUN;
 				break;
 			case 2864:
 				mount_color=0;  // Brown horse
 				npc_type->walkspeed=MOUNT_RUN1_WALK;
 				npc_type->runspeed=MOUNT_RUN1_RUN;
 				break;
 			case 2865:
 				mount_color=0;  // Brown horse
 				npc_type->walkspeed=MOUNT_RUN2_WALK;
 				npc_type->runspeed=MOUNT_RUN2_RUN;
 				break;
 			case 2866:
 				mount_color=0;  // Brown horse
 				npc_type->walkspeed=MOUNT_FAST_WALK;
 				npc_type->runspeed=MOUNT_FAST_RUN;
 				break;
 			case 2867:
 				mount_color=1;  // White horse
 				npc_type->walkspeed=MOUNT_SLOW1_WALK;
 				npc_type->runspeed=MOUNT_SLOW1_RUN;
 				break;
 			case 2868:
 				mount_color=1;  // White horse
 				npc_type->walkspeed=MOUNT_SLOW2_WALK;
 				npc_type->runspeed=MOUNT_SLOW2_RUN;
 				break;
 			case 2869:
 				mount_color=1;  // White horse
 				npc_type->walkspeed=MOUNT_RUN1_WALK;
 				npc_type->runspeed=MOUNT_RUN1_RUN;
 				break;
 			case 2870:
 				mount_color=1;  // White horse
 				npc_type->walkspeed=MOUNT_RUN2_WALK;
 				npc_type->runspeed=MOUNT_RUN2_RUN;
 				break;
 			case 2871:
 				mount_color=1;  // White horse
 				npc_type->walkspeed=MOUNT_FAST_WALK;
 				npc_type->runspeed=MOUNT_FAST_RUN;
 				break;
 			case 2872:
 				mount_color=2;  // Black horse
 				npc_type->walkspeed=MOUNT_SLOW1_WALK;
 				npc_type->runspeed=MOUNT_SLOW1_RUN;
 				break;
 			case 2873:
 				mount_color=2;  // Black horse
 				npc_type->walkspeed=MOUNT_SLOW2_WALK;
 				npc_type->runspeed=MOUNT_SLOW2_RUN;
 				break;
 			case 2916:
 				mount_color=2;  // Black horse
 				npc_type->walkspeed=MOUNT_RUN1_WALK;
 				npc_type->runspeed=MOUNT_RUN1_RUN;
 				break;
 			case 2917:
 				mount_color=2;  // Black horse
 				npc_type->walkspeed=MOUNT_RUN2_WALK;
 				npc_type->runspeed=MOUNT_RUN2_RUN;
 				break;
 			case 2918:
 				mount_color=2;  // Black horse
 				npc_type->walkspeed=MOUNT_FAST_WALK;
 				npc_type->runspeed=MOUNT_FAST_RUN;
 				break;
 			case 2919:
 				mount_color=3;  // Tan horse
 				npc_type->walkspeed=MOUNT_SLOW1_WALK;
 				npc_type->runspeed=MOUNT_SLOW1_RUN;
 				break;
 			case 2920:
 				mount_color=3;  // Tan horse
 				npc_type->walkspeed=MOUNT_SLOW2_WALK;
 				npc_type->runspeed=MOUNT_SLOW2_RUN;
 				break;
 			case 2921:
 				mount_color=3;  // Tan horse
 				npc_type->walkspeed=MOUNT_RUN1_WALK;
 				npc_type->runspeed=MOUNT_RUN1_RUN;
 				break;
 			case 2922:
 				mount_color=3;  // Tan horse
 				npc_type->walkspeed=MOUNT_RUN2_WALK;
 				npc_type->runspeed=MOUNT_RUN2_RUN;
 				break;
 			case 2923:
 				mount_color=3;  // Tan horse
 				npc_type->walkspeed=MOUNT_FAST_WALK;
 				npc_type->runspeed=MOUNT_FAST_RUN;
 				break;
 			case 3813:
 				mount_color=0;  // White drogmor
 				npc_type->walkspeed=MOUNT_SLOW1_WALK;
 				npc_type->runspeed=MOUNT_SLOW1_RUN;
 				break;
 			case 3814:
 				mount_color=0;  // White drogmor
 				npc_type->walkspeed=MOUNT_SLOW2_WALK;
 				npc_type->runspeed=MOUNT_SLOW2_RUN;
 				break;
 			case 3815:
 				mount_color=0;  // White drogmor
 				npc_type->walkspeed=MOUNT_RUN1_WALK;
 				npc_type->runspeed=MOUNT_RUN1_RUN;
 				break;
 			case 3816:
 				mount_color=0;  // White drogmor
 				npc_type->walkspeed=MOUNT_RUN2_WALK;
 				npc_type->runspeed=MOUNT_RUN2_RUN;
 				break;
 			case 3817:
 				mount_color=0;  // White drogmor
 				npc_type->walkspeed=MOUNT_FAST_WALK;
 				npc_type->runspeed=MOUNT_FAST_RUN;
 				break;
 			case 3818:
 				mount_color=1;  // Black drogmor
 				npc_type->walkspeed=MOUNT_SLOW1_WALK;
 				npc_type->runspeed=MOUNT_SLOW1_RUN;
 				break;
 			case 3819:
 				mount_color=1;  // Black drogmor
 				npc_type->walkspeed=MOUNT_SLOW2_WALK;
 				npc_type->runspeed=MOUNT_SLOW2_RUN;
 				break;
 			case 3820:
 				mount_color=1;  // Black drogmor
 				npc_type->walkspeed=MOUNT_RUN1_WALK;
 				npc_type->runspeed=MOUNT_RUN1_RUN;
 				break;
 			case 3821:
 				mount_color=1;  // Black drogmor
 				npc_type->walkspeed=MOUNT_RUN2_WALK;
 				npc_type->runspeed=MOUNT_RUN2_RUN;
 				break;
 			case 3822:
 				mount_color=1;  // Black drogmor
 				npc_type->walkspeed=MOUNT_FAST_WALK;
 				npc_type->runspeed=MOUNT_FAST_RUN;
 				break;
 			case 3823:
 				mount_color=2;  // Green drogmor
 				npc_type->walkspeed=MOUNT_SLOW1_WALK;
 				npc_type->runspeed=MOUNT_SLOW1_RUN;
 				break;
 			case 3824:
 				mount_color=2;  // Green drogmor
 				npc_type->walkspeed=MOUNT_SLOW2_WALK;
 				npc_type->runspeed=MOUNT_SLOW2_RUN;
 				break;
 			case 3825:
 				mount_color=2;  // Green drogmor
 				npc_type->walkspeed=MOUNT_RUN1_WALK;
 				npc_type->runspeed=MOUNT_RUN1_RUN;
 				break;
 			case 3826:
 				mount_color=2;  // Green drogmor
 				npc_type->walkspeed=MOUNT_RUN2_WALK;
 				npc_type->runspeed=MOUNT_RUN2_RUN;
 				break;
 			case 3827:
 				mount_color=2;  // Green drogmor
 				npc_type->walkspeed=MOUNT_FAST_WALK;
 				npc_type->runspeed=MOUNT_FAST_RUN;
 				break;
 			case 3828:
 				mount_color=3;  // Red drogmor
 				npc_type->walkspeed=MOUNT_SLOW1_WALK;
 				npc_type->runspeed=MOUNT_SLOW1_RUN;
 				break;
 			case 3829:
 				mount_color=3;  // Red drogmor
 				npc_type->walkspeed=MOUNT_SLOW2_WALK;
 				npc_type->runspeed=MOUNT_SLOW2_RUN;
 				break;
 			case 3830:
 				mount_color=3;  // Red drogmor
 				npc_type->walkspeed=MOUNT_RUN1_WALK;
 				npc_type->runspeed=MOUNT_RUN1_RUN;
 				break;
 			case 3831:
 				mount_color=3;  // Red drogmor
 				npc_type->walkspeed=MOUNT_RUN2_WALK;
 				npc_type->runspeed=MOUNT_RUN2_RUN;
 				break;
 			case 3832:
 				mount_color=3;  // Red drogmor
 				npc_type->walkspeed=MOUNT_FAST_WALK;
 				npc_type->runspeed=MOUNT_FAST_RUN;
 				break;
 			default:
 				Message(13,"I dont know what mount spell this is! (%i)", spell_id);
 				mount_color= 0;  // Brown horse
 				npc_type->walkspeed=MOUNT_SLOW1_WALK;
 				npc_type->runspeed=MOUNT_SLOW1_RUN;
 				break;
 		}
 
  		npc_type->light = 0;
  		npc_type->fixedZ = 1;
  		npc_type->STR = 75;
		npc_type->STA = 75;
		npc_type->DEX = 75;
		npc_type->AGI = 75;
		npc_type->INT = 75;
		npc_type->WIS = 75;
		npc_type->CHA = 75;

		
		NPC* horse = new NPC(npc_type, 0, GetX(), GetY(), GetZ(), GetHeading());
		

		entity_list.AddNPC(horse, false);
		APPLAYER* outapp = new APPLAYER;
		horse->CreateHorseSpawnPacket(outapp,this->GetName(), this->GetID());
		// Doodman: Kludged in here instead of adding a field to PCType. FIXME!
		NewSpawn_Struct* ns=(NewSpawn_Struct*)outapp->pBuffer;
		ns->spawn.mount_color=mount_color;
		ns->spawn.pet_owner_id=0;
		ns->spawn.walkspeed=npc_type->walkspeed;
		ns->spawn.runspeed=npc_type->runspeed;
		entity_list.QueueClients(horse, outapp);
		safe_delete(outapp);
		safe_delete(npc_type);
		// Okay, lets say he has a horse now.

		hasmount = true;
		int16 tmpID = horse->GetID();
		SetHorseId(tmpID);
    } else {
		if (hasmount)
			Message(13,"You already have a Horse.  Get off (or zone) Fatbutt!");

	}
}

// solar: add/update a spell in the client's spell bar
void Client::MemSpell(int16 spell_id, int slot, bool update_client)
{
	if(slot >= MAX_PP_MEMSPELL || slot < 0)
		return;

	if(update_client)
	{
		if(m_pp.mem_spells[slot] != 0xFFFFFFFF)
			UnmemSpell(slot, update_client);
	}

	m_pp.mem_spells[slot] = spell_id;

	if(update_client)
	{
		MemorizeSpell(slot, spell_id, memSpellMemorize);
	}
}

// solar: remove a spell from the client's spell bar
void Client::UnmemSpell(int slot, bool update_client)
{
	if(slot > MAX_PP_MEMSPELL || slot < 0)
		return;

	m_pp.mem_spells[slot] = 0xFFFFFFFF;

	if(update_client)
	{
		MemorizeSpell(slot, m_pp.mem_spells[slot], memSpellForget);
	}
}

void Client::UnmemSpellAll(bool update_client)
{
	int i;
	
	for(i = 0; i < MAX_PP_MEMSPELL; i++)
		if(m_pp.mem_spells[i] != 0xFFFFFFFF)
			UnmemSpell(i, update_client);
}

// solar: add a spell to client's spellbook
void Client::ScribeSpell(int16 spell_id, int slot, bool update_client)
{
	if(slot >= MAX_PP_SPELLBOOK || slot < 0)
		return;

	if(update_client)
	{
		if(m_pp.spell_book[slot] != 0xFFFFFFFF)
			UnscribeSpell(slot, update_client);
	}

	m_pp.spell_book[slot] = spell_id;

	if(update_client)
	{
		MemorizeSpell(slot, spell_id, memSpellScribing);
	}
}

// solar: remove a spell from client's spellbook
void Client::UnscribeSpell(int slot, bool update_client)
{
	if(slot >= MAX_PP_SPELLBOOK || slot < 0)
		return;

	m_pp.spell_book[slot] = 0xFFFFFFFF;

	if(update_client)
	{
		APPLAYER* outapp = new APPLAYER(OP_DeleteSpell, sizeof(DeleteSpell_Struct));
		DeleteSpell_Struct* del = (DeleteSpell_Struct*)outapp->pBuffer;
		del->spell_slot = slot;
		del->success = 1;
		QueuePacket(outapp);
		safe_delete(outapp);
	}
}

void Client::UnscribeSpellAll(bool update_client)
{
	int i;

	for(i = 0; i < MAX_PP_SPELLBOOK; i++)
	{
		if(m_pp.spell_book[i] != 0xFFFFFFFF)
			UnscribeSpell(i, update_client);
	}
}

void Client::SetBindPoint(int to_zone, float new_x, float new_y, float new_z) {
	if (to_zone == -1) {
		m_pp.bind_zone_id = zone->GetZoneID();
		m_pp.bind_x[0] = x_pos;
		m_pp.bind_y[0] = y_pos;
		m_pp.bind_z[0] = z_pos;
	}
	else {
		m_pp.bind_zone_id = to_zone;
		m_pp.bind_x[0] = new_x;
		m_pp.bind_y[0] = new_y;
		m_pp.bind_z[0] = new_z;
	}
}

void Client::GoToBind() {
	if (m_pp.bind_zone_id == zone->GetZoneID()) { //if same zone no reason to zone
		GMMove(m_pp.bind_x[0],
               m_pp.bind_y[0],
               m_pp.bind_z[0]);
    } else {
		MovePC(m_pp.bind_zone_id,
               m_pp.bind_x[0],
               m_pp.bind_y[0],
               m_pp.bind_z[0], 1); //lets zone
    }
}

void Mob::CheckBuffs() {
	if (this->casting_spell_id == 0) {

		this->CheckPet();
		int8 newtype[15] = { SE_ArmorClass, SE_STR, SE_DEX, SE_AGI, SE_WIS,
                             SE_INT, SE_CHA, SE_AttackSpeed, SE_MovementSpeed,
                             SE_DamageShield, SE_ResistFire, SE_ResistCold,
                             SE_ResistMagic, SE_ResistPoison, SE_ResistDisease };
		for (int h=0; h<15; h++) {
			if (!this->FindType(newtype[h])) {
				int16 buffid = FindSpell(this->class_, this->level,
                                         newtype[h], SPELLTYPE_SELF, 0,
                                         GetMana());
				if (buffid != 0) {
					this->CastSpell(buffid, this->GetID());
				}
			}
		}
	}
}

void Mob::CheckPet() {
	int16 buffid = 0;
	if (this->GetPetID() == 0 && 
       (this->GetClass() == 11 || this->GetClass() == 13)) {
		if (this->GetClass() == 13) {
			buffid = FindSpell(this->class_, this->level,
                               SE_SummonPet, SPELLTYPE_OTHER, 0,
                               GetMana());
        } else if (this->GetClass() == 11) {
			buffid = FindSpell(this->class_, this->level,
                               SE_NecPet, SPELLTYPE_OTHER, 0,
                               GetMana());
		}
		if (buffid != 0) {
			this->CastSpell(buffid, this->GetID());
		}
	}
}

int16 Mob::FindSpell(int16 classp, int16 level, int type,
                     FindSpellType spelltype, float distance,
                     sint32 mana_avail) {
    int i,j;

    int bestvalue = -1;
    int bestid = 0;

    if (classp < 1)
        return 0;
	classp = GetEQArrayEQClass(classp);
    if (level < 1)
        return 0;

    // purpose: find a suited spell for a class and level and type
    // the if's are here to filter out anything which isnt normal.
    // its possible that we miss some valid spells, but who cares.
    //  - neotokyo 19-Nov-02

	for (i = 0; i < SPDAT_RECORDS; i++) {
				if(!IsValidSpell(i))
					continue;
        // Filter all spells that should never be used
        if (spells[i].effectid[0] == SE_NegateIfCombat)
            continue;
        if (spells[i].targettype == ST_Group)
            continue;
        if (i == 2632)  // neotokyo: fix for obsolete BST pet summon spell
            continue;
        if (i == 1576)  // neotokyo: fix for torpor
            continue;
        if (spells[i].cast_time < 11)
            continue;
        if (spells[i].mana == 0)
            continue;

        // now for closer checks
        if (spelltype == SPELLTYPE_SELF) {
            if ( i == 357)  // fix for dark empathy
                continue;
            // check buffs 12 would be max, but 90% of all effects are in the first 4 slots
            for (j = 0; j < 5; j++) {
                // neotokyo: fix for pets
                if ( spells[i].effectid[j] == SE_Illusion &&
                     type != SE_Illusion)  // only let illusions thru if explicitly requested
                    continue;
                if ( spells[i].effectid[j] == type &&
                     spells[i].goodEffect != 0 &&
                     spells[i].classes[classp] <= level &&
                     spells[i].classes[classp] <= 65 &&
                     (spells[i].recast_time < 10000 ||
                      type == SE_SummonPet ||
                      type == SE_SummonBSTPet) && // neotokyo: fix for druid pets
                     (type == SE_AbsorbMagicAtt || type == SE_Rune ||
                      type == SE_NecPet || type == SE_SummonPet ||
                      spells[i].components[0] == -1 ) &&
                     spells[i].targettype != ST_Undead &&   // neotokyo: for  necro mend series
                     spells[i].targettype != ST_Group &&    // neotokyo: fix for group spells
                     spells[i].targettype != ST_Pet &&      // neotokyo: fix for beastlords casting pet heals on self
                     spells[i].targettype != ST_Summoned && // neotokyo: fix for vs. summoned spells on normal npcs
                     spells[i].targettype != ST_AETarget && // neotokyo: dont let em cast AEtarget spells
                     spells[i].mana <= mana_avail &&
                     spells[i].range >= distance) {
                    sint32 spellvalue;

                    // lets assume pet is always better if higher, so no formula needed
                    if (type == SE_NecPet ||
                        type == SE_SummonPet ||
                        type == SE_SummonBSTPet) {
                        spellvalue = spells[i].classes[classp];
                    } else {
											spellvalue = CalcSpellEffectValue_formula(spells[i].formula[j],
                                                    spells[i].base[j],
                                                    spells[i].max[j],
                                                    level, i);
                    }

                    if (abs(spellvalue) > bestvalue) {
                        bestvalue = abs(spellvalue);
                        bestid = i;
                    }
                }
            }
        } else if (spelltype == SPELLTYPE_OFFENSIVE) {
            // check offensive spells
            for (j = 0; j < 5; j++) {
                if (spells[i].effectid[j] == SE_Illusion &&
                    type != SE_Illusion)  // only let illusions thru if explicitly requested
                    continue;
                if (spells[i].effectid[j] == type &&
                    spells[i].goodEffect == 0 &&
                    spells[i].classes[classp] <= level &&
                    spells[i].classes[classp] <= 65 &&
                    spells[i].recast_time < 10000 &&
                    spells[i].components[0] == -1 &&
                    spells[i].mana <= mana_avail &&
                    spells[i].targettype != ST_Undead &&   // neotokyo: thats for the necro mend series
                    spells[i].targettype != ST_Group &&    // neotokyo: fix for group spells
                    spells[i].targettype != ST_Pet &&      // neotokyo: fix for beastlords casting pet heals on self
                    spells[i].targettype != ST_Summoned && // neotokyo: fix for vs. summoned spells on normal npcs
                    spells[i].targettype != ST_AETarget && // neotokyo: dont let em cast AEtarget spells
                    spells[i].range >= distance) {
                    sint32 spellvalue = CalcSpellEffectValue_formula(spells[i].formula[j],
                                                       spells[i].base[j],
                                                       spells[i].max[j],
                                                       level, i);
                    if ( abs(spellvalue) > bestvalue ) {
                        bestvalue = abs(spellvalue);
                        bestid = i;
                    }
                }
            }
        } else if (spelltype == SPELLTYPE_OTHER) {
            if ( i == 357)  // fix for dark empathy
                continue;
            // healing and such
            for (j = 0; j < 5; j++) {
                if (spells[i].effectid[j] == SE_Illusion &&
                    type != SE_Illusion)  // only let illusions thru if explicitly requested
                    continue;
                if (spells[i].effectid[j] == type &&
                    spells[i].targettype != ST_Self &&
                    spells[i].goodEffect != 0 &&
                    spells[i].classes[classp] <= level &&
                    spells[i].classes[classp] <= 65 &&
                    spells[i].recast_time < 10000 &&
                    spells[i].components[0] == -1 &&
                    spells[i].targettype != ST_Undead &&   // neotokyo: thats for the necro mend series
                    spells[i].targettype != ST_Group &&    // neotokyo: fix for group spells
                    spells[i].targettype != ST_Pet &&      // neotokyo: fix for beastlords casting pet heals on self
                    spells[i].targettype != ST_Summoned && // neotokyo: fix for vs. summoned spells on normal npcs
                    spells[i].targettype != ST_AETarget && // neotokyo: dont let em cast AEtarget spells
                    spells[i].mana <= mana_avail &&
                    spells[i].range >= distance) {
                    sint32 spellvalue = CalcSpellEffectValue_formula(spells[i].formula[j],
                                                       spells[i].base[j],
                                                       spells[i].max[j],
                                                       level, i);
                    if ( abs(spellvalue) > bestvalue ) {
                        bestvalue = abs(spellvalue);
                        bestid = i;
                    }
                }
            }
        }
    } // for i

//    g_LogFile.write("for combination [class %02d][level %02d][SE_type %02d][type %02d] i selected the spell: %s",
//        classp, level, (int16)type, int16(spelltype), spells[bestid].name);
    return bestid;
}

#if 0
int16 Mob::FindSpell(int16 classp, int16 level, int8 type, int8 spelltype) {
	if (this->casting_spell_id != 0)
		return 0;

	if (spelltype == 2) // for future use
		spelltype = 0;

	//int count=0;
	int16 bestsofar = 0;
	int16 bestspellid = 0;
	for (int i = 0; i < SPDAT_RECORDS; i++) {
		if ((IsLifetapSpell(i) && spelltype == 1) || (spells[i].targettype != ST_Group && spells[i].targettype != ST_Undead && spells[i].targettype != ST_Summoned && spells[i].targettype != ST_Pet && strstr(spells[i].name,"Summoning") == NULL)) {
			int Canuse = CanUseSpell(i, classp, level);
			if (Canuse != 0) {
				for (int z=0; z < 12; z++) {
					int spfo = CalcSpellValue(spells[i].formula[z], spells[i].base[z], spells[i].max[z], this->GetLevel());
					if (spells[i].effectid[z] == SE_ArmorClass && type == SE_ArmorClass && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_TotalHP && type == SE_TotalHP && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_STR && type == SE_STR && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;

							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_DEX && type == SE_DEX && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}

					if (spells[i].effectid[z] == SE_AGI && type == SE_AGI && !FindBuff(i)) {

						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {

							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}

					if (spells[i].effectid[z] == SE_WIS && type == SE_WIS && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}

					if (spells[i].effectid[z] == SE_INT && type == SE_INT && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_CHA && type == SE_CHA && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}

					if (spells[i].effectid[z] == SE_MovementSpeed && type == SE_MovementSpeed && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}

					if (spells[i].effectid[z] == SE_AttackSpeed && type == SE_AttackSpeed && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_ResistFire && type == SE_ResistFire && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_ResistCold && type == SE_ResistCold && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_ResistMagic && type == SE_ResistMagic && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_ResistDisease && type == SE_ResistDisease && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;

						}
					}
					if (spells[i].effectid[z] == SE_ResistPoison && type == SE_ResistPoison && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_DamageShield && type == SE_DamageShield && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_CurrentHPOnce && type == SE_CurrentHPOnce && !FindBuff(i)) {
						if (spfo > 0 && (spfo + spells[i].buffduration) > bestsofar) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_SummonPet && type == SE_SummonPet && !FindBuff(i)) {
						if (Canuse > bestsofar) {
							bestsofar = Canuse;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_NecPet && type == SE_NecPet && !FindBuff(i)) {
						if (Canuse > bestsofar) {
							bestsofar = Canuse;
							bestspellid = i;
						}
					}
					if (spells[i].effectid[z] == SE_CurrentHP && type == SE_CurrentHP && !FindBuff(i)) {
						if (spfo < 0 && (spells[i].buffduration + spfo) < bestsofar && spelltype == 1) {
							bestsofar = ((spells[i].buffduration * -1) + spfo);
							bestspellid = i;
						}
						if ((spfo + spells[i].buffduration) > bestsofar && spfo > 0 && spelltype == 0) {
							bestsofar = spfo + spells[i].buffduration;
							bestspellid = i;
						}

					}
				}
			}
		}
	}

	return bestspellid;
}
#endif

// solar: TODO get rid of this
sint8 Mob::GetBuffSlotFromType(int8 type) {
	for (int i = 0; i < BUFF_COUNT; i++) {
		if (buffs[i].spellid != 0xFFFF) {
			for (int j = 0; j < EFFECT_COUNT; j++) {
				if (spells[buffs[i].spellid].effectid[j] == type )
					return i;
			}
		}
	}
    return -1;
}


bool Mob::FindType(int8 type, bool bOffensive, int16 threshold) {
	for (int i = 0; i < BUFF_COUNT; i++) {
		if (buffs[i].spellid != SPELL_UNKNOWN) {

			for (int j = 0; j < EFFECT_COUNT; j++) {
                // adjustments necessary for offensive npc casting behavior
                if (bOffensive) {
				    if (spells[buffs[i].spellid].effectid[j] == type) {
                        sint16 value = 
                                CalcSpellEffectValue_formula(buffs[i].durationformula,
                                               spells[buffs[i].spellid].base[j],
                                               spells[buffs[i].spellid].max[j],
                                               buffs[i].casterlevel, buffs[i].spellid);
                        LogFile->write(EQEMuLog::Normal, 
                                "FindType: type = %d; value = %d; threshold = %d",
                                type, value, threshold);
                        if (value < threshold)
                            return true;
                    }
                } else {
				    if (spells[buffs[i].spellid].effectid[j] == type )
					    return true;
                }
			}
		}
	}
	return false;
}

bool Mob::AddProcToWeapon(int16 spell_id, bool bPerma, int8 iChance) {
	int i;
	if (bPerma) {
 		for (i = 0; i < MAX_PROCS; i++) {
			if (PermaProcs[i].spellID == 0xFFFF) {
				PermaProcs[i].spellID = spell_id;
				PermaProcs[i].chance = iChance;
				PermaProcs[i].pTimer = NULL;


				return true;
			}
		}
	cout << "Too many perma procs for " << GetName() << endl;
    } else {
		for (i = 0; i < MAX_PROCS; i++) {
			if (SpellProcs[i].spellID == 0xFFFF) {
				SpellProcs[i].spellID = spell_id;
				SpellProcs[i].chance = iChance;
				SpellProcs[i].pTimer = NULL;
				return true;
			}
		}
	cout << "Too many procs for " << GetName() << endl;
	}
    return false;
}

bool Mob::RemoveProcFromWeapon(int16 spell_id, bool bAll) {
	for (int i = 0; i < MAX_PROCS; i++) {
		if (bAll || SpellProcs[i].spellID == spell_id) {
			SpellProcs[i].spellID = 0xFFFF;
			SpellProcs[i].chance = 0;
			SpellProcs[i].pTimer = NULL;
		}
	}
    return true;
}

// solar: this is checked in a few places to decide wether special bard
// behavior should be used.
bool Mob::UseBardSpellLogic(int16 spell_id, int slot)
{
	if(spell_id == 0xffff)
		spell_id = casting_spell_id;

	if(slot == -1)
		slot = casting_spell_slot;

	// should we treat this as a bard singing?
	return
	(
		spell_id != 0 &&
		spell_id != SPELL_UNKNOWN &&
		slot != -1 &&
		GetClass() == BARD &&
		IsBardSong(spell_id) &&
		slot <= 8
	);
}

void Mob::Gate()
{
	GoToBind();
}

void Client::Gate()
{
	Mob::Gate();
}

void NPC::Gate()
{
	entity_list.MessageClose_StringID(this, true, 200, MT_Spells, GATES, GetCleanName());
	Mob::Gate();
}int Mob::GetCasterLevel(int16 spell_id) {
	int level = GetLevel();
	level += spellbonuses.effective_casting_level;
	level += itembonuses.effective_casting_level;
	
	if(IsClient()) {
		if(IsBardSong(spell_id)) {
			//bard item modifiers raise the effective caster level
			//which results in an increase in their effects.
			int instrument_add = CastToClient()->GetInstrumentMod(spell_id);
			
			if (instrument_add > 0) {
				level = (level + level * instrument_add / 1000);
			}
		}
	}
	return(level);
}
