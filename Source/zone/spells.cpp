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
	if(delaytimer == true && spellend_timer->Check())
	{
		spellend_timer->Disable();
		delaytimer = false;
		return;
	}

	// a timed spell is finished casting
	if (casting_spell_id != 0 && spellend_timer->Check())
	{
		spellend_timer->Disable();
		delaytimer = false;
		CastedSpellFinished(casting_spell_id, casting_spell_targetid, casting_spell_slot, casting_spell_mana, casting_spell_inventory_slot);
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
		spellend_timer->Enabled() ||
		IsStunned() ||
		IsMezzed()
	)
	{
		if(IsClient())
			CastToClient()->SendSpellBarEnable(spell_id);
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
		Message(13, "Error: Spell requires a target.");
		InterruptSpell();
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
	if(mana_cost && slot != 10)
	{
		int my_curmana = GetMana();
		int my_maxmana = GetMaxMana();
		if(my_curmana < mana_cost)	// not enough mana
		{
			if(!(my_maxmana > 0 &&	my_curmana == my_maxmana))
			{
				Message(13, "Error: Insufficent mana.");
				InterruptSpell();
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
	spellend_timer->Start(cast_time);
	
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
		mobDist = Dist(*pMob);
		if(mobDist > spells[spell_id].range)
		{
			modrange = GetActSpellRange(spell_id, spells[spell_id].range);

			if(modrange < mobDist) // still not enough
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
	
	if (this->IsAIControlled())
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

	if (IsClient()) // begins to glow messages
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
			GetCasterLevel(), 
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
				dmg = CalcSpellEffectValue(spell_id, i, GetCasterLevel());
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
	if (c->GetGM()) return 1;

	assert(IsValidSpell(spell_id));

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
	float specialize = spec_skill < HIGHEST_SKILL ? 0 : GetSkill(spec_skill);
		//VERY rough success formula, needs research
	if(specialize > 0) {
		if(((specialize/6.0f) + 15.0f) < MakeRandomFloat(0, 100)) {
			specialize *= SPECIALIZE_FIZZLE / 200;
		} else {
			specialize = 0.0f;
		}
		if(spec_skill < HIGHEST_SKILL)
			c->CheckIncreaseSkill(spec_skill);
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
		return 1;
	return 0;
}

void Mob::ZeroCastingVars()
{
	// zero out the state keeping vars
	isattacked = false;
	attacked_count = 0;
	spellend_timer->Disable();
	casting_spell_id = 0;
	casting_spell_targetid = 0;
	casting_spell_slot = 0;
	casting_spell_mana = 0;
	casting_spell_inventory_slot = 0;
	delaytimer = false;
}

void Mob::InterruptSpell(int16 spellid)
{
	if (spellid == 0xFFFF)
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



	// here we do different things if this is a bard casting a bard song from
	// a spell bar slot
	if(UseBardSpellLogic()) // bard singing
	{
		// don't need to check anything for bard singing so this is just empty
	}
	else // not bard singing
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
					distance_moved = sqrt(d_x * d_x + d_y * d_y);
					// if you moved 1 unit, that's 25% off your chance to regain.
					// if you moved 2, you lose 100% off your chance
					distancemod = distance_moved * 5;
					distancemod *= distancemod;
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
					if(this->itembonuses->percussionMod > 0)
						CastToClient()->CheckIncreaseSkill(PERCUSSION_INSTRUMENTS);
					else
						CastToClient()->CheckIncreaseSkill(SINGING);
					break;
				}
				case STRINGED_INSTRUMENTS:
				{
					if(this->itembonuses->stringedMod > 0)
						CastToClient()->CheckIncreaseSkill(STRINGED_INSTRUMENTS);
					else
						CastToClient()->CheckIncreaseSkill(SINGING);
					break;
				}
				case WIND_INSTRUMENTS:
				{
					if(this->itembonuses->windMod > 0)
						CastToClient()->CheckIncreaseSkill(WIND_INSTRUMENTS);
					else
						CastToClient()->CheckIncreaseSkill(SINGING);
					break;
				}
				case BRASS_INSTRUMENTS:
				{
					if(this->itembonuses->brassMod > 0)
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
			SendSpellBarEnable(spell_id);

			// this causes the delayed refresh of the spell bar gems
			APPLAYER *outapp = new APPLAYER(OP_MemorizeSpell, sizeof(MemorizeSpell_Struct));
			MemorizeSpell_Struct* memspell = (MemorizeSpell_Struct*)outapp->pBuffer;
			memspell->slot = slot;
			memspell->spell_id = spell_id;
			memspell->scribing = 3;
			outapp->priority = 6;
			CastToClient()->QueuePacket(outapp);

			// this tells the client that casting may happen again
			SetMana(GetMana());

			// skills
			if(slot < MAX_PP_MEMSPELL)
			{
				CastToClient()->CheckIncreaseSkill(spells[spell_id].skill);
				// increased chance of gaining channel skill if you regained concentration
				CastToClient()->CheckIncreaseSkill(CHANNELING, regain_conc ? 5 : 0);
			}
		}

		// there should be no casting going on now
		ZeroCastingVars();

		// set the rapid recast timer for next time around
		delaytimer = true;
		spellend_timer->Start(400,true);
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
		case ST_Undead:
		case ST_Animal:
		case ST_Plant:
		case ST_Dragon:
		case ST_Giant:
		case ST_Summoned:
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

	// Check for consumables and focus items
	if(IsClient())
	{
    Client *c = this->CastToClient();
    int component, component_count, inv_slot_id;
    for(int t_count = 0; t_count < 4; t_count++)
    {
			component = spells[spell_id].components[t_count];
			component_count = spells[spell_id].component_counts[t_count];

			if (component != -1)
			{
				if(c->GetInv().HasItem(component, component_count) == -1) // item not found
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
					// Components found Deleteing
					if(!IsBardSong(spell_id))	// bard spells don't use up reagents right?
					{
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
/*
			else if (spells[spell_id].NoexpendReagent[t_count] != -1)
			{
			}
*/
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

			if(ae_center->IsBeacon())
			{
				// special ae duration spell
				ae_center->CastToBeacon()->AELocationSpell(this, range, spell_id);
			}
			else
			{
				// regular PB AE or targeted AE spell - spell_target is null if PB
				if(spell_target)	// this must be an AETarget spell
				{
					// affect the target too
					SpellOnTarget(spell_id, spell_target);
				}
				entity_list.AESpell(this, ae_center, range, spell_id);
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
	if(slot != 10)	// 10 is item
	{
		SetMana(GetMana() - mana_used);
	}


	// neotokyo: 09-Nov-02
	// Recourse means there is a spell linked to that spell in that the recourse spell will
	// be automatically casted on the casters group or the caster only depending on Targettype
	// solar: this is for things like dark empathy, shadow vortex
	recourse_spell = spells[spell_id].RecourseLink;
	if(recourse_spell != 0)
	{
		if(IsGrouped() && spells[recourse_spell].targettype == ST_Group)
		{
			Group *g = entity_list.GetGroupByMob(this);
			g->CastGroupSpell(this, recourse_spell);
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

	return CalcBuffDuration_formula(caster->GetCasterLevel(), formula, duration);
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
int CheckStackConflict(int16 spellid1, int caster_level1, int16 spellid2, int caster_level2)
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

			if(abs(sp2_value) >= abs(sp1_value))
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
	
	caster_level = caster ? caster->GetCasterLevel() : GetCasterLevel();
    
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

		if(curbuf.spellid != 0xFFFF)
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

			if(curbuf.spellid != 0xFFFF)
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
	assert(buffs[emptyslot].spellid == 0xFFFF);	// sanity check
			
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
		if (curbuf.spellid == 0xffff)
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

	action->level = GetCasterLevel();	// caster level, for animation only
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
	if(spelltar->GetInvul())
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
			if(!(IsPartialCapableSpell(spell_id) && spell_effectiveness > 0))
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

	// solar: TODO fix this - hate needs to be added for the effects
	 // this sucks for charm spells todo: invent something better
	if (spelltar->IsAIControlled() && IsDetrimentalSpell(spell_id))
		spelltar->AddToHateList(this, 0, CalcSpellEffectValue(spell_id, 0, GetLevel()));

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

// the spell can still fail here, if the buff can't stack
// in this case false will be returned, true otherwise
bool Mob::SpellEffect(Mob* caster, int16 spell_id, double partial)
{
	int caster_level, buffslot, effect, effect_value, i;
	SPDat_Spell_Struct spell;
#ifdef SPELL_EFFECT_SPAM
#define _EDLEN	200
	char effect_desc[_EDLEN];
#endif

	if(!IsValidSpell(spell_id))
		return false;

	buffslot = AddBuff(caster, spell_id);
	if(buffslot == -1)	// stacking failure
		return false;

	spell = spells[spell_id];
	caster_level = caster ? caster->GetCasterLevel() : GetCasterLevel();
	
#ifdef SPELL_EFFECT_SPAM
		Message(0, "You are affected by spell '%s' (id %d)", spell.name, spell_id);
		if(buffslot >= 0)
		{
			Message(0, "Buff slot:  %d  Duration:  %d tics", buffslot, buffs[buffslot].ticsremaining);
		}
#endif

	// iterate through the effects in the spell
	for (i = 0; i < EFFECT_COUNT; i++)
	{
		if(IsBlankSpellEffect(spell_id, i))
			continue;

		effect = spell.effectid[i];
		effect_value = CalcSpellEffectValue(spell_id, i, caster_level);
		
		if(!(effect == SE_AttackSpeed || effect == SE_SingingSkill) && caster->IsClient() && caster->GetClass() == BARD)
		{
			switch(spell.skill)
			{
			case SINGING:
				{
					effect_value = (int)ceil(effect_value * caster->CastToClient()->itembonuses->singingMod);
					break;
				}
			case PERCUSSION_INSTRUMENTS:
				{
					effect_value = (int)ceil(effect_value * caster->CastToClient()->itembonuses->percussionMod);
					break;
				}

			case STRINGED_INSTRUMENTS:
				{
					effect_value = (int)ceil(effect_value * caster->CastToClient()->itembonuses->stringedMod);
					break;
				}
			case WIND_INSTRUMENTS:
				{
					effect_value = (int)ceil(effect_value * caster->CastToClient()->itembonuses->windMod);
					break;
				}
			case BRASS_INSTRUMENTS:
				{
					effect_value = (int)ceil(effect_value * caster->CastToClient()->itembonuses->brassMod);
					break;
				}
			}
		}

#ifdef SPELL_EFFECT_SPAM
		effect_desc[0] = 0;
#endif

		switch(effect)
		{
			case SE_CurrentHP:	// nukes, heals; also regen/dot if a buff
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Current Hitpoints: %+i", effect_value);
#endif
				// SE_CurrentHP is calculated at first tick if its a dot/buff
				if (buffslot >= 0)
					break;

				// for offensive spells check if we have a spell rune on
				sint32 dmg = effect_value;
				if(dmg < 0)
				{
					// take partial damage into account
					dmg = (int) (dmg * partial / 100);
					if (caster && caster->IsClient())
					{
						dmg = caster->GetActSpellValue(spell_id, dmg);

						//spell crits
						int chance = 0;
						float ratio =1.0;

						//normal spell crit
						if(caster_level > 12 && caster->GetClass() == WIZARD)
						{
							chance+= 3;
							ratio +=.15;
						}

						//spell casting fury
						//uint8 *aa_item = &(((uint8 *)&caster->CastToClient()->aa)[23]);
						uint8 aa_item = caster->CastToClient()->GetAA(23);
						if(aa_item == 1) {chance+=2; ratio += .333;}
						if(aa_item == 2) {chance+=5; ratio += .666;}
						if(aa_item == 3) {chance+=7; ratio += 1.0;}
				
						if(rand()%100 <= chance) dmg = (int)(dmg * ratio);	//message.. but I don't care atm			
					}

					sint32 origdmg = dmg;
					dmg = ReduceMagicalDamage(dmg, GetMagicRune());
					if (origdmg != dmg && caster)
					{
						caster->Message(15,
							"The Spellshield absorbed %d of %d points of damage",
							abs(origdmg - dmg), abs(origdmg));
					}

					if (dmg == 0)	// rune absorbed it all
						break;
				}
				else if(dmg > 0)
				{
					if (caster && caster->IsClient())
					{
						dmg = caster->GetActSpellValue(spell_id, dmg);
					}
				}

				this->ChangeHP(caster, dmg, spell_id, buffslot);
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Current Hitpoints: %+i  actual: %+i", effect_value, dmg);
#endif
				break;
			}

			case SE_CurrentHPOnce:	// used in buffs usually, see Courage
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Current Hitpoints Once: %+i", effect_value);
#endif
				ChangeHP(caster, effect_value, spell_id, buffslot);
				break;
			}

			case SE_PercentalHeal:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Percental Heal: %+i (%d%% max)", spell.max[i], effect_value);
#endif
				// solar: TODO implement this
				const char *msg = "Percental Heal is not implemented.";
				if(caster) caster->Message(13, msg);
				break;
			}

			case SE_CurrentMana:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Current Mana: %+i", effect_value);
#endif
				if (buffslot >= 0)
					break;

				SetMana(GetMana() + effect_value);
				break;
			}

			case SE_Translocate:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Translocate: %s %d %d %d heading %d", 
					spell.teleport_zone, spell.base[1], spell.base[0], 
					spell.base[2], spell.base[3]
				);
#endif
				if(IsClient())
				{
				Group* group = entity_list.GetGroupByClient(this->CastToClient());
				if(caster != this && (!group || !group->IsGroupMember(caster->CastToMob())))
					break;
					// solar: if it's blank or "0" it means bind point
					// TODO: MovePC needs to take heading too, which is in base[3]
					if(spell.teleport_zone && strlen(spell.teleport_zone) > 1)
					{
						CastToClient()->MovePC
						(
							spell.teleport_zone,
							spell.base[1],
							spell.base[0],
							spell.base[2]
						);
					}
					else
					{
						Gate();
					}
				}
				break;
			}

			case SE_Teleport:	// gates, rings, circles, etc
			case SE_Teleport2:
			case SE_Succor:
			{
				float x, y, z, heading;
				char *target_zone;

				x = spell.base[1];
				y = spell.base[0];
				z = spell.base[2];
				heading = spell.base[3];
								
				if(!strcmp(spell.teleport_zone, "same"))
				{
					target_zone = 0;
				}
				else
				{
					target_zone = spell.teleport_zone;
				}

#ifdef SPELL_EFFECT_SPAM
				const char *efstr = "Teleport";
				if(effect == SE_Teleport)
					efstr = "Teleport v1";
				else if(effect == SE_Teleport2)
					efstr = "Teleport v2";
				else if(effect == SE_Succor)
					efstr = "Succor";

				snprintf(effect_desc, _EDLEN, 
					"%s: %0.2f, %0.2f, %0.2f heading %0.2f in %s",
					efstr, x, y, z, heading, target_zone ? target_zone : "same zone"
				);
#endif
				if(IsClient())
				{
					// TODO: MovePC needs to take heading too, which is in base[3]
					CastToClient()->MovePC(target_zone, x, y, z);
				}
				break;
			}

			case SE_HealOverTime:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Heal over Time: %+i", effect_value);
#endif
				// solar: this is calculated with bonuses
				break;
			}

			case SE_MovementSpeed:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Movement Speed: %+i", effect_value);
#endif
				// solar: this is calculated with bonuses
				break;
			}

			case SE_AttackSpeed:
			case SE_AttackSpeed2:
			case SE_AttackSpeed3:
			{
#ifdef SPELL_EFFECT_SPAM
				if(effect == SE_AttackSpeed)
					snprintf(effect_desc, _EDLEN, "Attack Speed v1: %d%%", effect_value);
				else if(effect == SE_AttackSpeed2)
					snprintf(effect_desc, _EDLEN, "Attack Speed v2: %d%%", effect_value);
				else if(effect == SE_AttackSpeed3)
					snprintf(effect_desc, _EDLEN, "Attack Speed v3: %d%%", effect_value);
#endif
				// solar: this is calculated with bonuses
				break;
			}

			case SE_Invisibility:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Invisibility");
#endif
				// solar: TODO already invis message and spell kill from SpellOnTarget
				SetInvisible(true);
				break;
			}

			case SE_InvisVsUndead:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Invisibility to Undead");
#endif
				invisible_undead = true;		// Mongrel: We're now invis to undead
				break;
			}

			case SE_SeeInvis:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "See Invisible");
#endif
				// solar: handled by client
				break;
			}

			case SE_WaterBreathing:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Water Breathing");
#endif
				break;
			}

			case SE_AddFaction:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Faction Mod: %+i", effect_value);
#endif
				// solar: TODO implement this
				const char *msg = "Faction Mod is not implemented.";
				if(caster) caster->Message(13, msg);
				break;
			}

			case SE_Stun:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Stun: %d msec", effect_value);
#endif
				Stun(effect_value);
				break;
			}

			case SE_Charm:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Charm: %+i", effect_value);
#endif
				if (!caster)	// can't be someone's pet unless we know who that someone is
					break;

				//Shawn319: This does not work. we need to re-write it. Players should never be able to charm other players
				if (IsClient() && caster->IsClient())
				{
					caster->Message(0, "Unable to cast charm on a fellow player.");
					break;
				}

				WhipeHateList();
				caster->SetPet(this);
				SetOwnerID(caster->GetID());
				SetPetOrder(SPO_Follow);
                
				// tell caster it has a pet
				if(caster->IsClient())
				{
					APPLAYER *app = new APPLAYER(OP_Charm, sizeof(Charm_Struct));
					Charm_Struct *ps = (Charm_Struct*)app->pBuffer;
					ps->owner_id = caster->GetID();
					ps->pet_id = this->GetID();
					ps->command = 1;
					caster->CastToClient()->FastQueuePacket(&app);
				}

				if (this->IsClient())
				{
					AI_Start();
				}

// solar: random duration stuff - this is going into CalcBuffDuration eventually
				bool bBreak = false;

				// define spells with fixed duration
				// this is handled by the server, and not by the spell database
				switch(spell_id)
				{
					case 3371://call of the banshee
					case 1707://dictate
						bBreak = true;
				}
				if (!bBreak)
				{
					int cha = caster->GetCHA();
					float r1 = (float)rand()/(float)RAND_MAX;
					float r2 = (float)cha  + (caster->GetLevel()/3) / 255.0f;
					float finalPercentDuration = r1 +r2; //When resists work use partial to aid in determining length
					if (finalPercentDuration > 1.0f) finalPercentDuration = 1.0f;
					buffs[buffslot].ticsremaining = (int)ceil(finalPercentDuration *buffs[buffslot].ticsremaining);
				}

				break;
			}

			case SE_Fear:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Fear: %+i", effect_value);
#endif
// solar: random duration, removing this later
				buffs[buffslot].ticsremaining = MakeRandomInt(1, buffs[buffslot].ticsremaining);
								
				if (IsClient() && CastToClient()->disc_inuse == 31)	// fearless
				{
					entity_list.MessageClose_StringID(this, false, 200, MT_Disciplines, RESISTS_URGE, GetCleanName());
					//entity_list.MessageClose(this, false, 100, 0, "%s resists the urge to flee!", GetName());
				}
				else
				{
					//kathgar: Its basicly fear, they don't move
					Stun(buffs[buffslot].ticsremaining * 6000);
				}
                
				break;
			}

			case SE_BindAffinity:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Bind Affinity");
#endif
				if (this->IsClient())
				{
					CastToClient()->SetBindPoint();
					Save();
				}
				break;
			}

			case SE_Gate:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Gate");
#endif
				Gate();
				break;
			}

			case SE_CancelMagic:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Cancel Magic: %d", effect_value);
#endif
				// solar: TODO proper dispel counters, including poison/disease/curse
				int slot;
				for(slot = 0; slot < BUFF_COUNT; slot++)
				{
					if
					(
						buffs[slot].spellid != 0xFFFF &&
						buffs[slot].durationformula != DF_Permanent &&
			    	buffs[slot].casterlevel <= (caster_level + effect_value)
			    )
			    {
						BuffFadeBySlot(slot);
						slot = BUFF_COUNT;
					}
				}
				break;
			}

			case SE_DispelDetrimental:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Dispel Detrimental: %d", effect_value);
#endif
				// solar: TODO implement this
				const char *msg = "Dispel Detrimental is not implemented.";
				if(caster) caster->Message(13, msg);
				break;
			}

			case SE_Mez:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Mesmerize");
#endif
				Mesmerize();
				break;
			}

			case SE_SummonItem:
			{
#ifdef SPELL_EFFECT_SPAM
				const Item_Struct *item = database.GetItem(spell.base[i]);
				const char *itemname = item ? item->Name : "*Unknown Item*";
				snprintf(effect_desc, _EDLEN, "Summon Item: %s (id %d)", itemname, spell.base[i]);
#endif
				if(IsClient())
				{
					int charges;
					if (spell.formula[i] < 100)
					{
						charges = spell.formula[i];
					}
					else	// variable charges
					{
						charges = CalcSpellEffectValue_formula(spell.formula[i], 0, 20, caster_level);
					}
					charges = charges < 1 ? 1 : (charges > 20 ? 20 : charges);
					CastToClient()->SummonItem(spell.base[i], charges);
				}

				break;
			}

			case SE_SummonBSTPet:
			case SE_NecPet:
			case SE_SummonPet:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Summon Pet: %s", spell.teleport_zone);
#endif
				if(GetPet())
				{
					Message_StringID(MT_Shout, ONLY_ONE_PET);
				}
				else
				{
					MakePet(spell.teleport_zone);
				}
				break;
			}

			case SE_Familiar:	// solar: why isn't this just a pet?
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Summon Familiar: %s", spell.teleport_zone);
#endif
				if (GetFamiliarID())
				{
					Message_StringID(MT_Shout, ONLY_ONE_PET);
				}
				else
				{
					MakePet(spell.teleport_zone);
				}
				break;
			}

			case SE_DivineAura:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Invulnerability");
#endif
				SetInvul(true);
				break;
			}

			case SE_ShadowStep:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Shadow Step: %d", effect_value);
#endif
				if(IsNPC())	// see Song of Highsun - sends mob home
				{
					Gate();
				}
				// solar: shadow step is handled by client already, nothing required
				break;
			}

			case SE_TrueNorth:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "True North");
#endif
				// solar: handled by client
				break;
			}

			case SE_Blind:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Blind: %+i", effect_value);
#endif
				// solar: handled by client
				// TODO: blind flag?
				break;
			}

			case SE_SenseDead:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Sense Dead");
#endif
				// solar: handled by client
				break;
			}

			case SE_SenseSummoned:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Sense Summoned");
#endif
				// solar: handled by client
				break;
			}

			case SE_SenseAnimals:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Sense Animals");
#endif
				break;
				// solar: handled by client
			}

			case SE_Rune:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Melee Absorb Rune: %+i", effect_value);
#endif
				SetRune(effect_value);
				break;
			}

			case SE_AbsorbMagicAtt:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Spell Absorb Rune: %+i", effect_value);
#endif
				SetMagicRune(effect_value);
				break;
			}

			case SE_Levitate:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Levitate");
#endif
				// solar: no need to send this, the client already knows to levitate
				//SendAppearancePacket(AT_Levitate, 2);
				break;
			}

			case SE_Illusion:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Illusion: race %d", effect_value);
#endif
				// solar: TODO fix up SendIllusionPacket

				int16 tex = 0;
				if (spell_id == 599||spell_id == 2798||spell_id == 2799||spell_id == 2800)
					tex = 2;// water
				else if (spell_id == 598||spell_id == 2795||spell_id == 2796||spell_id == 2797)
					tex = 1;// fire
				else if (spell_id == 584||spell_id == 2792||spell_id == 2793||spell_id == 2794)
					tex = 0; //earth
				else if (spell_id == 597||spell_id == 2789|| spell_id == 2790|| spell_id == 2791)
					tex = 3;// air

				SendIllusionPacket
				(
					spell.base[i],
					Mob::GetDefaultGender(spell.base[i], GetGender()),
					tex
				);
				break;
			}

			case SE_IllusionCopy:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Illusion Copy");
#endif
				// solar: TODO implement this
				const char *msg = "Illusion Copy is not implemented.";
				if(caster) caster->Message(13, msg);
			}

			case SE_DamageShield:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Damage Shield: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_ReverseDS:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Reverse Damage Shield: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_SpellDamageShield:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Reverse Damage Shield: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_Identify:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Identify");
#endif
				// solar: handled by client
				break;
			}

			case SE_WipeHateList:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Memory Blur: %d", effect_value);
#endif
				// solar: TODO figure out how to handle the chance calculation, for
				// now just make it a toss up
				if(MakeRandomInt(0, 100) > 50)
				{
					if(IsAIControlled())
					{
						WhipeHateList();
					}
					Message(13, "Your mind fogs. Who are my friends? Who are my enimies?... it was all so clear a moment ago...");
				}
				break;
			}

			case SE_SpinTarget:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Spin: %d", effect_value);
#endif
				// solar: the spinning is handled by the client
				if(buffslot >= 0)
					Stun(buffs[buffslot].ticsremaining * 6000);
				break;
			}

			case SE_EyeOfZomm:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Eye of Zomm");
#endif
				const char *msg = "Eye of Zomm is not implemented.";
				if(caster) caster->Message(13, msg);
				break;
			}

			case SE_ReclaimPet:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Reclaim Pet");
#endif
				if
				(
					IsNPC() &&
					GetOwnerID() &&		// I'm a pet
					caster &&					// there's a caster
					caster->GetID() == GetOwnerID()	&& // and it's my master
					GetPetType() != 0xFF
				)
				{
					SetOwnerID(0);	// this will kill the pet
				}
				break;
			}

			case SE_BindSight:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Bind Sight");
#endif
				// solar: handled by client
				break;
			}

			case SE_FeignDeath:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Feign Death");
#endif
				if(IsClient())
					CastToClient()->SetFeigned(true);
				break;
			}

			case SE_VoiceGraft:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Voice Graft");
#endif
				const char *msg = "Voice Graft is not implemented.";
				if(caster) caster->Message(13, msg);
				break;
			}

			case SE_Sentinel:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Sentinel");
#endif
				if(caster)
				{
					if(caster == this)
					{
						Message_StringID(MT_Spells,
							SENTINEL_TRIG_YOU);
					}
					else
					{
						caster->Message_StringID(MT_Spells,
							SENTINEL_TRIG_OTHER, GetCleanName());
					}
				}
				break;
			}

			case SE_LocateCorpse:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Locate Corpse");
#endif
				// solar: handled by client
				break;
			}

			case SE_Revive:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Revive");	// heh the corpse won't see this
#endif
				if (IsCorpse() && CastToCorpse()->IsPlayerCorpse())
					CastToCorpse()->CastRezz(spell_id, caster);
				break;
			}

			case SE_ModelSize:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Model Size: %d%%", effect_value);
#endif
				ChangeSize(GetSize() * (effect_value / 100.0));
				break;
			}

			case SE_TestSpells:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Test Spell: %+i", effect_value);
#endif
				break;
			}

			case SE_Root:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Root: %+i", effect_value);
#endif
				BuffFadeByEffect(SE_MovementSpeed);
				rooted = true;
				break;
			}

			case SE_SummonHorse:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Summon Mount: %s", spell.teleport_zone);
#endif
				if(IsClient())	// NPCs can't ride
				{
					CastToClient()->MakeHorseSpawnPacket(spell_id);
				}
				break;
			}

			case SE_SummonCorpse:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Summon Corpse: %d", effect_value);
#endif
				if(IsClient())	// can only summon corpses of clients
				{
					Corpse *corpse = entity_list.GetCorpseByOwner(CastToClient());
					if(corpse)
					{
						if(caster)
						{
							caster->Message_StringID(4, SUMMONING_CORPSE_OTHER, GetCleanName());
						}
						corpse->Summon(CastToClient(), true);
					}
					else	// corpse not found
					{
						if(caster)
						{
							caster->Message_StringID(4, CORPSE_CANT_SENSE);
						}
					}
				}
				break;
			}

			case SE_WeaponProc:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Weapon Proc: %s (id %d)", spells[effect_value].name, effect_value);
#endif
				AddProcToWeapon(spell.base[i]);
				break;
			}

			case SE_Calm:	// cinder jolt, enraging blow
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Hate Mod: %+i%%", effect_value);
#endif
				break;
			}

			case SE_ChangeAggro:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Aggro Mod: %+i%%", effect_value);
#endif
				break;
			}

			case SE_ChangeFrenzyRad:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Frenzy Radius Mod: %d/%d", spell.base[i], spell.max[i]);
#endif
				break;
			}

			case SE_Harmony:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Reaction Radius Mod: %d/%d", spell.base[i], spell.max[i]);
#endif
				break;
			}

			case SE_Lull:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Lull");
#endif
				// TODO: check vs. CHA when harmony effect failed, if caster is to be added to hatelist
				break;
			}

			case SE_TotalHP:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Total Hitpoints: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_ManaPool:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Total Mana: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_InfraVision:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Infravision");
#endif
				// solar: handled by client
				break;
			}

			case SE_UltraVision:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Ultravision");
#endif
				// solar: handled by client
				break;
			}

			case SE_Stamina:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Stamina: %+i", effect_value);
#endif
				// solar: handled with bonuses - this is stamina regen, like CurrentHP
				break;
			}

			case SE_ArmorClass:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Armor Class: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_ATK:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "ATK: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_STR:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "STR: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_DEX:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "DEX: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_AGI:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "AGI: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_STA:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "STA: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_INT:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "INT: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_WIS:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "WIS: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_CHA:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "CHA: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_AllStats:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "All Stats: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_ResistFire:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Resist Fire: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_ResistCold:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Resist Cold: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_ResistPoison:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Resist Poison: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_ResistDisease:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Resist Disease: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_ResistMagic:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Resist Magic: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_ResistAll:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Resist All: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_CastingLevel:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Effective Casting Level Mod: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_PoisonCounter:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Poison Counter: %+i", effect_value);
#endif
				// solar: don't have to do anything with these, but they need to
				// be used for dispelling these later.
				break;
			}

			case SE_DiseaseCounter:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Disease Counter: %+i", effect_value);
#endif
				break;
			}

			case SE_CurseCounter:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Curse Counter: %+i", effect_value);
#endif
				break;
			}

			case SE_NegateIfCombat:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Negate if Combat");
#endif
				// solar: TODO implement this
				break;
			}

			case SE_Destroy:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Destroy");
#endif
				if(IsNPC() && GetLevel() < 52)
					CastToNPC()->Depop();
				break;
			}

			case SE_Lycanthropy:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Lycanthropy: %+i", effect_value);
#endif
				// solar: TODO figure out what this is
				const char *msg = "Lycanthropy is not implemented.";
				if(caster) caster->Message(13, msg);
				break;
			}
			
			case SE_TossUp:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Toss Up: %d", effect_value);
#endif
				// solar: TODO implement this
				const char *msg = "Toss Up is not implemented.";
				if(caster) caster->Message(13, msg);
				break;
			}

			case SE_MagnifyVision:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Magnify Vision: %d%%", effect_value);
#endif
				// solar: handled by client
				break;
			}

			case SE_StopRain:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Stop Rain");
#endif
				zone->zone_weather = 0;
				zone->weatherSend();
				break;
			}

			case SE_Sacrifice:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Sacrifice");
#endif
				// solar: TODO implement this
				const char *msg = "Sacrifice is not implemented.";
				if(caster) caster->Message(13, msg);
				break;
			}

			case SE_Silence:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Silence");
#endif
				// solar: TODO implement this
				const char *msg = "Silence is not implemented.";
				if(caster) caster->Message(13, msg);
				break;
			}

			case SE_Fearless:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Fearless");
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_CallPet:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Call Pet");
#endif
				// solar: this is cast on self, not on the pet
				if(GetPet() && GetPet()->IsNPC())
				{
					GetPet()->CastToNPC()->GMMove(GetX(), GetY(), GetZ(), GetHeading());
				}
				break;
			}

			case SE_AntiGate:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Anti-Gate: %d", effect_value);
#endif
				// solar: TODO implement this
				const char *msg = "Anti-Gate is not implemented.";
				if(caster) caster->Message(13, msg);
				break;
			}

			case SE_Hunger:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Hunger");
#endif
				// solar: TODO implement this
				const char *msg = "Hunger is not implemented.";
				if(caster) caster->Message(13, msg);
				break;
			}

			case SE_MagicWeapon:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Magic Weapon");
#endif
				// solar: TODO implement this
				const char *msg = "Magic Weapon is not implemented.";
				if(caster) caster->Message(13, msg);
				break;
			}

			case SE_SingingSkill:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Singing Skill: %+i", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_HealRate:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Heal Effectiveness: %d%%", effect_value);
#endif
				// solar: TODO implement this
				const char *msg = "Heal Effectiveness is not implemented.";
				if(caster) caster->Message(13, msg);
				break;
			}

			case SE_Screech:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Screech: %d", effect_value);
#endif
				// solar: TODO figure out what this is
				const char *msg = "Screech is not implemented.";
				if(caster) caster->Message(13, msg);
				break;
			}

			case SE_Reflect:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Reflect: %+i%%", effect_value);
#endif
				// solar: handled with bonuses
				break;
			}

			case SE_StackingCommand_Block:
			case SE_StackingCommand_Overwrite:
			{
				// solar: these are special effects used by the buff stuff
				break;
			}

			default:
			{
#ifdef SPELL_EFFECT_SPAM
				snprintf(effect_desc, _EDLEN, "Unknown Effect ID %d", effect);
#else
				Message(0, "Unknown spell effect %d in spell %s (id %d)", effect, spell.name, spell_id);
#endif
			}
		}
#ifdef SPELL_EFFECT_SPAM
		Message(0, ". . . Effect #%i: %s", i + 1, strlen(effect_desc) ? effect_desc : "Unknown");
#endif
	}

	CalcBonuses();

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

int CalcSpellEffectValue(int16 spell_id, int effect_id, int caster_level)
{
	int formula, base, max;

	if
	(
		!IsValidSpell(spell_id) ||
		effect_id < 0 ||
		effect_id >= EFFECT_COUNT
	)
		return 0;

	formula = spells[spell_id].formula[effect_id];
	base = spells[spell_id].base[effect_id];
	max = spells[spell_id].max[effect_id];

	if(IsBlankSpellEffect(spell_id, effect_id))
		return 0;
	
	return CalcSpellEffectValue_formula(formula, base, max, caster_level);
}

// solar: generic formula calculations
int CalcSpellEffectValue_formula(int formula, int base, int max, int caster_level)
{
/*
neotokyo: i need those formulas checked!!!!

0 = base
1 - 99 = base + level * formulaID
100 = base
101 = base + level / 2
102 = base + level
103 = base + level * 2
104 = base + level * 3
105 = base + level * 4
106 ? base + level * 5
107 ? min + level / 2
108 = min + level / 3
109 = min + level / 4
110 = min + level / 5
119 ? min + level / 8
121 ? min + level / 4
122 = splurt
123 ?
203 = stacking issues ? max
205 = stacking issues ? 105


  0x77 = min + level / 8
*/
	
	int result = 0, updownsign = 1, ubase = abs(base);
	
	// solar: this updown thing might look messed up but if you look at the
	// spells it actually looks like some have a positive base and max where
	// the max is actually less than the base, hence they grow downward
	if (max < base && max != 0)
	{
		// values are calculated down
		updownsign = -1;
	}
	else
	{
		// values are calculated up
		updownsign = 1;
	}
	
	switch(formula)
	{
		case   0:
		case 100:	// solar: confirmed 2/6/04
			result = ubase; break;
		case 101:	// solar: confirmed 2/6/04
			result = ubase + updownsign * (caster_level / 2); break;
		case 102:	// solar: confirmed 2/6/04
			result = ubase + updownsign * caster_level; break;
		case 103:	// solar: confirmed 2/6/04
			result = ubase + updownsign * (caster_level * 2); break;
		case 104:	// solar: confirmed 2/6/04
			result = ubase + updownsign * (caster_level * 3); break;
		case 105:	// solar: confirmed 2/6/04
			result = ubase + updownsign * (caster_level * 4); break;
		case 108:
			result = ubase + updownsign * (caster_level / 3); break;
		case 109:	// solar: confirmed 2/6/04
			result = ubase + updownsign * (caster_level / 4); break;
		case 110:	// solar: confirmed 2/6/04
			result = ubase + (caster_level / 5); break;
		case 111:	// solar: this doesn't look right
            result = ubase + 5 * (caster_level - 16); break;
		case 112:
            result = ubase + 8 * (caster_level - 24); break;
		case 113:
            result = ubase + 12 * (caster_level - 34); break;
		case 114:
            result = ubase + 15 * (caster_level - 44); break;
		//case 115:	// solar: this is only in symbol of transal
		//case 116:	// solar: this is only in symbol of ryltan
		//case 117:	// solar: this is only in symbol of pinzarn
		//case 118:	// solar: used in naltron and a few others
		case 119:	// solar: confirmed 2/6/04
			result = ubase + (caster_level / 8); break;
		case 121:	// solar: corrected 2/6/04
			result = ubase + (caster_level / 3); break;
		case 122:	// todo: we need the remaining tics here
			break;
		case 123:	// solar: added 2/6/04
			result = MakeRandomInt(ubase, abs(max));
		default:
			if (formula < 100)
				result = ubase + (caster_level * formula);
			else
				LogFile->write(EQEMuLog::Debug, "Unknown spell effect value forumula %d", formula);
	}
	
	// now check result against the allowed maximum
	if (max != 0)
	{
		if (updownsign == 1)
		{
			if (result > max)
				result = max;
		}
		else
		{
			if (result < max)
				result = max;
		}
	}

	// if base is less than zero, then the result need to be negative too
	if (base < 0 && result > 0)
		result *= -1;
	return result;
}

/*
int CalcSpellValue(int formula, int base, int max, int caster_level, int16 spell_id)
{
}
*/


void Mob::CalcBonuses()
{
	if(IsClient())
	{
		memset(itembonuses, 0, sizeof(StatBonuses));
		CastToClient()->CalcItemBonuses(itembonuses);
		CastToClient()->CalcEdibleBonuses(itembonuses);
	}

	CalcSpellBonuses(spellbonuses);

	CalcMaxHP();
	CalcMaxMana();

	rooted = FindType(SE_Root);
}

int Client::CalcRecommendedLevelBonus(int8 level, uint8 reclevel, int basestat)
{
	if( (reclevel > 0) && (level < reclevel) )
	{
		double statmod = (level / reclevel) * basestat;
	
		if( statmod < 0 )
		{
			statmod *= -1;
			statmod += 0.5;
			return (((int)statmod) * -1);
		}
		else
		{
			statmod += 0.5;
			return (int)statmod;
		}
	}

	return 0;
}

void Client::CalcItemBonuses(StatBonuses* newbon) {
	for (int i=0; i<21; i++) {
		if(m_inv[i] == 0) {continue;}
		const ItemInst* inst = m_inv[i];
		if (inst && inst->IsType(ItemTypeCommon)) {
			const Item_Struct *item = inst->GetItem();
			const ItemCommon_Struct& common = inst->GetItem()->Common;
			if( GetLevel() >= common.RecommendedLevel )
			{
				newbon->AC += common.AC;
				newbon->HP += common.HP;
				newbon->Mana += common.Mana;
				newbon->STR += common.STR;
				newbon->STA += common.STA;
				newbon->DEX += common.DEX;
				newbon->AGI += common.AGI;
				newbon->INT += common.INT;
				newbon->WIS += common.WIS;
				newbon->CHA += common.CHA;
				
				newbon->MR += common.SvMagic;
				newbon->FR += common.SvFire;
				newbon->CR += common.SvCold;
				newbon->PR += common.SvPoison;
				newbon->DR += common.SvDisease;
			}
			else
			{
				int lvl = GetLevel();
				int reclvl = common.RecommendedLevel;

				newbon->AC += CalcRecommendedLevelBonus( lvl, reclvl, common.AC );
				newbon->HP += CalcRecommendedLevelBonus( lvl, reclvl, common.HP );
				newbon->Mana += CalcRecommendedLevelBonus( lvl, reclvl, common.Mana );
				newbon->STR += CalcRecommendedLevelBonus( lvl, reclvl, common.STR );
				newbon->STA += CalcRecommendedLevelBonus( lvl, reclvl, common.STA );
				newbon->DEX += CalcRecommendedLevelBonus( lvl, reclvl, common.DEX );
				newbon->AGI += CalcRecommendedLevelBonus( lvl, reclvl, common.AGI );
				newbon->INT += CalcRecommendedLevelBonus( lvl, reclvl, common.INT );
				newbon->WIS += CalcRecommendedLevelBonus( lvl, reclvl, common.WIS );
				newbon->CHA += CalcRecommendedLevelBonus( lvl, reclvl, common.CHA );

				newbon->MR += CalcRecommendedLevelBonus( lvl, reclvl, common.SvMagic );
				newbon->FR += CalcRecommendedLevelBonus( lvl, reclvl, common.SvFire );
				newbon->CR += CalcRecommendedLevelBonus( lvl, reclvl, common.SvCold );
				newbon->PR += CalcRecommendedLevelBonus( lvl, reclvl, common.SvPoison );
				newbon->DR += CalcRecommendedLevelBonus( lvl, reclvl, common.SvDisease );
			}
			
			//FatherNitwit: New style haste, shields, and regens
			if(common.EffectType == 2) {
				if(newbon->haste < (sint8)item->hastepercent)
					newbon->haste = item->hastepercent;
			}
			if(item->hpregen > 0) {
				newbon->HPRegen += item->hpregen;
			}
			if(item->manaregen > 0) {
				newbon->ManaRegen += item->manaregen;
			}
			if(item->damageshield > 0) {
				newbon->DamageShield += item->damageshield;
			}
			if(common.SpellShield > 0) {
				newbon->SpellDamageShield += common.SpellShield;
			}
			if(common.Shielding > 0) {
				newbon->Shielding += common.Shielding;
			}
			if(common.StunResist > 0) {
				newbon->StunResist += common.StunResist;
			}
			if(common.StrikeThrough > 0) {
				newbon->StrikeThrough += common.StrikeThrough;
			}
			if(common.Avoidance > 0) {
				newbon->Avoidance += common.Avoidance;
			}
			if(common.Accuracy > 0) {
				newbon->Accuracy += common.Accuracy;
			}
			if(common.CombatEffects > 0) {
				newbon->CombatEffects += common.CombatEffects;
			}
			
			if ((common.SpellId == 998) && (common.EffectType == 2)) { // item haste
				if (newbon->haste < common.ProcLevel)
					newbon->haste = common.ProcLevel;
			}
			else if ((common.SpellId != 0xFFFF) && (common.EffectType == 2)) { // latent effects
				ApplySpellsBonuses(common.SpellId, common.ProcLevel, newbon);
			}
			switch(common.BardSkillType)
			{
			case 51: /* All (e.g. Singing Short Sword) */
				{
					if(common.BardSkillAmt > newbon->singingMod)
						newbon->singingMod = common.BardSkillAmt;
					if(common.BardSkillAmt > newbon->brassMod)
						newbon->brassMod = common.BardSkillAmt;
					if(common.BardSkillAmt > newbon->stringedMod)
						newbon->stringedMod = common.BardSkillAmt;
					if(common.BardSkillAmt > newbon->percussionMod)
						newbon->percussionMod = common.BardSkillAmt;
					if(common.BardSkillAmt > newbon->windMod)
						newbon->windMod = common.BardSkillAmt;
					break;
				}
			case 50: /* Singing */
				{
					if(common.BardSkillAmt > newbon->singingMod)
						newbon->singingMod = common.BardSkillAmt;
					break;
				}
			case 23: /* Wind */
				{
					if(common.BardSkillAmt > newbon->windMod)
						newbon->windMod = common.BardSkillAmt;
					break;
				}
			case 24: /* stringed */
				{
					if(common.BardSkillAmt > newbon->stringedMod)
						newbon->stringedMod = common.BardSkillAmt;
					break;
				}
			case 25: /* brass */
				{
					if(common.BardSkillAmt > newbon->brassMod)
						newbon->brassMod = common.BardSkillAmt;
					break;
				}
			case 26: /* Percussion */
				{
					if(common.BardSkillAmt > newbon->percussionMod)
						newbon->percussionMod = common.BardSkillAmt;
					break;
				}
			}
			/*
			if (common->SkillModPercent!=0){
				if (newbon->skillmod[common->SkillModId] < common->SkillModPercent)
					newbon->skillmod[common->SkillModId] = (uint8)common->SkillModPercent;
			}
			*/
		}
	}
	if (newbon->singingMod!=0)
		newbon->singingMod/=10;
	if (newbon->windMod!=0)
		newbon->windMod/=10;
	if (newbon->stringedMod!=0)
		newbon->stringedMod/=10;
	if (newbon->brassMod!=0)
		newbon->brassMod/=10;
	if (newbon->percussionMod!=0)
		newbon->percussionMod/=10;
	SetAttackTimer();
}

void Client::CalcEdibleBonuses(StatBonuses* newbon) {
#if EQDEBUG >= 11
    cout<<"Client::CalcEdibleBonuses(StatBonuses* newbon)"<<endl;
#endif
  // Search player slots for skill=14(food) and skill=15(drink)
  // pp.inventory[22] - pp.inventory[29]
  // pp.containerinv[0] - containerinv[79]

	// @merth: Do this after inventory struct is solid
	/*	solar: NOTE this whole thing is commented
  // Find food
  uint32 food_inr = 0;
  int16 food_slot = 0;
  uint32 drink_inr = 0;
  int16 drink_slot = 0;

  const Item_Struct* search_item = 0;
  for (int16 cur_i = 22; cur_i <= 29 ; cur_i++) {
    search_item = GetItemIDAt(cur_i);
    if (!search_item) {
      continue;
    }
    if (search_item->Common.Skill == 14 && !food_slot) {
      food_slot = cur_i;
      food_inr = GetItemIDAt(cur_i);
    } else if (search_item->Common.Skill == 15 && !drink_slot) {
      drink_slot = cur_i;
      drink_inr = GetItemIDAt(cur_i);
    }
    search_item = NULL;
  }
  //for (int16 cur_b = 0+250; cur_b <= 79+250; cur_b++) {
  //  if ( !food_slot || !drink_slot ) {
  //    search_item = GetItemIDAt(cur_b);
  //    if (!search_item) {
  //      // Database error
  //      // FIXME    Log error function goes here
  //      continue;
  //    }
  //    if (search_item->Common.Skill == 14 && !food_slot) {
  //      food_slot = cur_b;
  //      food_inr = GetItemIDAt(cur_b);
  //    }
  //    if (search_item->Common.Skill == 15 && !drink_slot) {
  //      drink_slot = cur_b;
  //      drink_inr = GetItemIDAt(cur_b);

  //    }

  //    search_item = NULL;
  //  }
  //}
  // End Find food

		if (food_slot) {
			search_item = database.GetItem( food_inr );
			if (search_item != NULL) {
#if EQDEBUG >= 11
    cout<<"Food_inr: "<< search_item->ItemNumber<<endl;
#endif
			newbon->AC += search_item->Common.AC;
			newbon->HP += search_item->Common.HP;
			newbon->Mana += search_item->Common.Mana;
			newbon->STR += search_item->Common.STR;
			newbon->STA += search_item->Common.STA;
			newbon->DEX += search_item->Common.DEX;
			newbon->AGI += search_item->Common.AGI;
			newbon->INT += search_item->Common.INT;
			newbon->WIS += search_item->Common.WIS;
			newbon->CHA += search_item->Common.CHA;
			newbon->MR += search_item->Common.SvMagic;
			newbon->FR += search_item->Common.SvFire;
			newbon->CR += search_item->Common.SvCold;
			newbon->PR += search_item->Common.SvPoison;
			newbon->DR += search_item->Common.SvDisease;
			search_item = NULL;
		}
	}
	
	if (drink_slot) {
		search_item = database.GetItem( drink_inr );
		if (search_item != NULL) {
			#if EQDEBUG >= 11
				cout<<"Drink_inr: "<< search_item->ItemNumber<<endl;
			#endif
			newbon->AC += search_item->Common.AC;
			newbon->HP += search_item->Common.HP;
			newbon->Mana += search_item->Common.Mana;
			newbon->STR += search_item->Common.STR;
			
			newbon->STA += search_item->Common.STA;
			newbon->DEX += search_item->Common.DEX;
			newbon->AGI += search_item->Common.AGI;
			newbon->INT += search_item->Common.INT;
			newbon->WIS += search_item->Common.WIS;
			newbon->CHA += search_item->Common.CHA;
			newbon->MR += search_item->Common.SvMagic;
			newbon->FR += search_item->Common.SvFire;
			newbon->CR += search_item->Common.SvCold;
			newbon->PR += search_item->Common.SvPoison;
			newbon->DR += search_item->Common.SvDisease;
			search_item = NULL;
		}
	}
	
	return;
	*/
}

void Mob::CalcSpellBonuses(StatBonuses* newbon)
{
	int i;

	memset(newbon, 0, sizeof(StatBonuses));
	newbon->ArrgoRange = -1;
	newbon->AssistRange = -1;

	for(i = 0; i < BUFF_COUNT; i++)
	{
		ApplySpellsBonuses(buffs[i].spellid, buffs[i].casterlevel, newbon);
	}
}

void Mob::ApplySpellsBonuses(int16 spell_id, int8 casterlevel, StatBonuses* newbon)
{
	int i, effect_value;

	if(!IsValidSpell(spell_id))
		return;

	for (i = 0; i < EFFECT_COUNT; i++)
	{
		if(IsBlankSpellEffect(spell_id, i))
			continue;

	  effect_value = CalcSpellEffectValue(spell_id, i, casterlevel);

		switch (spells[spell_id].effectid[i])
		{
			case SE_CurrentHP:	// @BP Aura of battle, ect
			case SE_HealOverTime:
			{
				if(IsBeneficialSpell(spell_id) && effect_value < 0)
					break;

				newbon->HPRegen += effect_value;
				break;
			}

			case SE_CurrentMana:
			{
				newbon->ManaRegen += effect_value;
				break;
			}

			case SE_Harmony:
			{
				// neotokyo: Harmony effect as buff - kinda tricky
				// harmony could stack with a lull spell, which has better aggro range
				// take the one with less range in any case
				if
				(
					newbon->ArrgoRange == -1 ||
					effect_value < newbon->ArrgoRange
				)
				{
					newbon->ArrgoRange = effect_value;
				}
				break;
			}

			case SE_ChangeFrenzyRad:
			{
				if
				(
					newbon->AssistRange == -1 ||
					effect_value < newbon->AssistRange
				)
				{
					newbon->AssistRange = effect_value;
				}
				break;
			}

			case SE_AttackSpeed:
			case SE_AttackSpeed2:
			case SE_AttackSpeed3:
			{
				newbon->haste += effect_value - 100;
				newbon->haste = newbon->haste > 120 ? 120 : (newbon->haste < -120 ? -120 : newbon->haste);

				break;
			}

			case SE_TotalHP:
			{
				newbon->HP += effect_value;
				break;
			}

			case SE_ManaPool:
			{
				newbon->Mana += effect_value;
				break;
			}

			case SE_ArmorClass:
			{
				newbon->AC += effect_value;
				break;
			}

			case SE_ATK:
			{
				newbon->ATK += effect_value;
				break;
			}

			case SE_STR:
			{
				newbon->STR += effect_value;
				break;
			}

			case SE_DEX:
			{
				newbon->DEX += effect_value;
				break;
			}

			case SE_AGI:
			{
				newbon->AGI += effect_value;
				break;
			}

			case SE_STA:
			{
				newbon->STA += effect_value;
				break;
			}

			case SE_INT:
			{
				newbon->INT += effect_value;
				break;
			}

			case SE_WIS:
			{
				newbon->WIS += effect_value;
				break;
			}

			case SE_CHA:
			{
				newbon->CHA += effect_value;
				break;
			}

			case SE_AllStats:
			{
				newbon->STR += effect_value;
				newbon->DEX += effect_value;
				newbon->AGI += effect_value;
				newbon->STA += effect_value;
				newbon->INT += effect_value;
				newbon->WIS += effect_value;
				newbon->CHA += effect_value;
				break;
			}

			case SE_ResistFire:
			{
				newbon->FR += effect_value;
				break;
			}

			case SE_ResistCold:
			{
				newbon->CR += effect_value;
				break;
			}

			case SE_ResistPoison:
			{
				newbon->PR += effect_value;
				break;
			}

			case SE_ResistDisease:
			{
				newbon->DR += effect_value;
				break;
			}

			case SE_ResistMagic:
			{
				newbon->MR += effect_value;
				break;
			}

			case SE_ResistAll:
			{
				newbon->MR += effect_value;
				newbon->DR += effect_value;
				newbon->PR += effect_value;
				newbon->CR += effect_value;
				newbon->FR += effect_value;
				break;
			}

			case SE_CastingLevel:	// Brilliance of Ro
			{
				newbon->effective_casting_level += effect_value;
				break;
			}

			case SE_MovementSpeed:
			{
				newbon->movementspeed += effect_value;
				break;
			}

			case SE_DamageShield:
			{
				newbon->DamageShield += effect_value;
				break;
			}
			
			case SE_SpellDamageShield:
			{
				newbon->SpellDamageShield += effect_value;
				break;
			}

			case SE_ReverseDS:
			{
				newbon->ReverseDamageShield += effect_value;
				break;
			}

			case SE_Reflect:
			{
				newbon->reflect_chance += effect_value;
				break;
			}

			case SE_SingingSkill:
			{
				//newbon->skillmod[SINGING] += effect_value;
				newbon->singingMod += effect_value/10;
				break;
			}
		}
	}
}

void Mob::DoBuffTic(int16 spell_id, int32 ticsremaining, int8 caster_level, Mob* caster)
{
	int effect, effect_value;
	SPDat_Spell_Struct spell;

	if(!IsValidSpell(spell_id))
		return;


	spell = spells[spell_id];

	if (spell_id != 0xFFFF) {
 		for (int i=0; i < EFFECT_COUNT; i++)
 		{
 			if(IsBlankSpellEffect(spell_id, i))
 				continue;

			effect = spell.effectid[i];
			effect_value = CalcSpellEffectValue(spell_id, i, caster_level);
			
			switch(effect)
			{
				case SE_CurrentHP:
				case SE_HealOverTime:
				{
					ChangeHP(caster ? caster : this, effect_value, spell_id, i, true);
					break;
				}

				case SE_CurrentMana:
				{
					SetMana(GetMana() + effect_value);
					break;
				}

           /* case SE_Charm: { //Do it once in Effect instead of every tic
                bool bBreak = false;

                // define spells with fixed duration
                // this is handled by the server, and not by the spell database
                switch(spell_id) {
                case 3371://call of the banshee
                case 1707://dictate
                    bBreak = true;
                }
				
                if (!bBreak && caster) {
                    int cha = caster->GetCHA();
                    float r1 = (float)rand()/(float)RAND_MAX;
                    float r2 = (float)cha  + (caster->GetLevel()/3) / 255.0f;

                    if (r1 > r2) {
                        BuffFadeByEffect(SE_Charm);
                    }
                }
                break;
            }*/

				// solar: TODO get this outta here
				case SE_Root: {
					float r1 = (float)rand()/RAND_MAX;
					float r2 = (float)(GetMR() - caster_level)/512.0f;//Need to move to Effect and use partial when resists are updated
					// cout<<"Root:"<<(float)r1<<":"<<r2<<endl;
					if ( r1 < r2 )
						BuffFadeByEffect(SE_Root);
					break;
				}
				default: {
					// do we need to do anyting here?
				}
			}
		}
	}
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

	buffs[slot].spellid = 0xFFFF;

	if (iRecalcBonuses)
		CalcBonuses();
}

// solar: checks if 'this' can be affected by spell_id from caster
// returns true if the spell should fail, false otherwise
bool Mob::IsImmuneToSpell(int16 spell_id, Mob *caster)
{
	int effect_index;

	assert(caster != NULL);

	if(!IsValidSpell(spell_id))
	{
		return true;
	}

	if(IsMezSpell(spell_id))
	{
		if
		(
			SpecAttackTimers[UNMEZABLE] ||
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
	if(IsStunSpell(spell_id) && SpecAttackTimers[UNSTUNABLE])
	{
		caster->Message_StringID(MT_Shout, IMMUNE_STUN);
	}

	// slow and haste spells
	if(IsEffectInSpell(spell_id, SE_AttackSpeed))
	{
		if(SpecAttackTimers[UNSLOWABLE])
		{
			caster->Message_StringID(MT_Shout, IMMUNE_ATKSPEED);
			return true;
		}
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
		if(SpecAttackTimers[UNCHARMABLE])
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
		int8 buffslot = GetBuffSlotFromType(SE_MovementSpeed);
		if((FindType(SE_Root) && IsEffectInSpell(spell_id, SE_MovementSpeed)) || (buffslot!=255 && buffs[buffslot].spellid > 0 && buffs[buffslot].spellid < (int32)SPDAT_RECORDS && IsDetrimentalSpell(buffs[buffslot].spellid) && IsBeneficialSpell(spell_id)))
		{
			caster->Message_StringID(MT_Shout,CANNOT_AFFECT_PC);
			return true;
		}
		// solar: this is where we could check for things that can't be snared
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
	
	if(!IsValidSpell(spell_id))
	{
		return 0;
	}

	target_level = GetLevel();
	caster_level = caster ? caster->GetCasterLevel() : target_level;

	// if NPC target and more than 5 levels above caster, it's always resisted
	if(IsNPC() && target_level - caster_level > 5)
		return 0;

	switch(spells[spell_id].resisttype)
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
	if(IsClient() && CastToClient()->disc_inuse == 30)
	{
		if (GetLevel() <= 32)
			resist += 3;
		else if (GetLevel() >= 33 && GetLevel() <= 35)
			resist += 4;
		else if (GetLevel() >= 36 && GetLevel() <= 38)
			resist += 5;
		else if (GetLevel() >= 39 && GetLevel() <= 41)
			resist += 6;
		else if (GetLevel() >= 42 && GetLevel() <= 44)
			resist += 7;
		else if (GetLevel() >= 45 && GetLevel() <= 47)
			resist += 8;
		else if (GetLevel() >= 48 && GetLevel() <= 49)
			resist += 9;
		else if (GetLevel() >= 50)
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

	float level_adj = pow((float)caster_level - (float)target_level, 2)+1;
	// level_adj is a positive value indicating the magnitude of the adjustment
	no_resist_chance += level_adj * (caster_level > target_level ? 1 : -1);

	// now we add the resistance we have
	no_resist_chance -= resist / 2.0;

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
// pet related functions

char *GetRandPetName()
{
	char petnames[77][64] = { "Gabeker","Gann","Garanab","Garn","Gartik",
            "Gebann","Gebekn","Gekn","Geraner","Gobeker","Gonobtik","Jabantik",
            "Jasarab","Jasober","Jeker","Jenaner","Jenarer","Jobantik",
            "Jobekn","Jonartik","Kabann","Kabartik","Karn","Kasarer","Kasekn",
            "Kebekn","Keber","Kebtik","Kenantik","Kenn","Kentik","Kibekab",
            "Kobarer","Kobobtik","Konaner","Konarer","Konekn","Konn","Labann",
            "Lararer","Lasobtik","Lebantik","Lebarab","Libantik","Libtik",
            "Lobn","Lobtik","Lonaner","Lonobtik","Varekab","Vaseker","Vebobab",
            "Venarn","Venekn","Vener","Vibobn","Vobtik","Vonarer","Vonartik",
            "Xabtik","Xarantik","Xarar","Xarer","Xeber","Xebn","Xenartik",
            "Xeratik","Xesekn","Xonartik","Zabantik","Zabn","Zabeker","Zanab",
            "Zaner","Zenann","Zonarer","Zonarn" };
	int r = (rand()  % (77 - 1)) + 1;
	printf("Pet being created: %s\n",petnames[r]); // DO NOT COMMENT THIS OUT!
	if(r > 77) // Just in case
		r=77;
	return petnames[r];
}

int CalcPetLevel(int nlevel, int nclass)
{
	//int plevel = 0;
	if (nclass == MAGICIAN)
		return (nlevel - 10);
	else
		return (nlevel - 12);
}

int CalcPetHp(int levelb, int classb, int STA)
{
	int multiplier = 0;
	int base_hp = 0;
	switch(classb) {
		case WARRIOR:{
			if (levelb < 20)
				multiplier = 22;
			else if (levelb < 30)
				multiplier = 23;
			else if (levelb < 40)
				multiplier = 25;
			else if (levelb < 53)
				multiplier = 27;
			else if (levelb < 57)
				multiplier = 28;
			else
				multiplier = 30;
			break;
		}
		case DRUID:
		case CLERIC:
		case SHAMAN:{
			multiplier = 15;
			break;
		}
		case PALADIN:
		case SHADOWKNIGHT:{
			if (levelb < 35)
				multiplier = 21;
			else if (levelb < 45)
				multiplier = 22;
			else if (levelb < 51)
				multiplier = 23;
			else if (levelb < 56)
				multiplier = 24;
			else if (levelb < 60)
				multiplier = 25;
			else
				multiplier = 26;
			break;
		}
		case MONK:
		case BARD:
		case ROGUE:
		case BEASTLORD:{
			if (levelb < 51)
				multiplier = 18;
			else if (levelb < 58)
				multiplier = 19;
			else
				multiplier = 20;
			break;
		}
		case RANGER:{
			if (levelb < 58)
				multiplier = 20;
			else
				multiplier = 21;
			break;
		}
		case MAGICIAN:
		case WIZARD:
		case NECROMANCER:
		case ENCHANTER:{
			multiplier = 12;
			break;
		}
		default:{
			if (levelb < 35)
				multiplier = 21;
			else if (levelb < 45)
				multiplier = 22;
			else if (levelb < 51)
				multiplier = 23;
			else if (levelb < 56)
				multiplier = 24;
			else if (levelb < 60)
				multiplier = 25;
			else
				multiplier = 26;
			break;
		}
	}

	if (multiplier == 0)
	{
		cerr << "Multiplier == 0 in CalcPetHp,using Generic...." << endl;
		multiplier=12;
	}

	base_hp = 5 + (multiplier*levelb) + ((multiplier*levelb*STA) + 1)/300;
	return base_hp;
}



void Mob::MakePet(const char* pettype) {
/*Baron-Sprite: Pet types were conflicting all over...
 * was rushed it appears.  I have corrected pet types and
 * assigned ranges for pet types. PLEASE follow these ranges 
 * if you ever add/modify pets: The Pettype ranges for each pet 
 * are shown next to their summoning name, take a minute and look. 
 * Since mage pets are not as percise as necromancers, I am going i
 * to reserve them a large range for future additions I will add.
 *
 * However their base values will still be reserved as 0-3 and epic 
 * will stay on 4. Also I may want to note that even though pet 
 * ranges and types are reserved, it doesn't mean they will be used, 
 * however please respect the reserved spots.  Thanks!
 * TODO: Make anything missing on this list.
*/
	//Baron-Sprite:  Mage pets done by: gej302.  Am I missing anyone else??  Sorry only one I saw.
	Make_Pet_Struct petstruct;
		if (strncmp(pettype, "SumEarthR", 9) == 0) { //Baron-Sprite: This Pettype is reserved to 0. ALSO 74-87.
		int8 tmp = atoi(&pettype[9]);
		if (tmp >= 2 && tmp <= 15) {
                   switch (tmp) { 
					 case  2:
						database.MakePet(&petstruct,74,1);
						break;
					 case  3:
						database.MakePet(&petstruct,75,1);
						break;
					 case  4:
						database.MakePet(&petstruct,76,1);
						break;
					 case  5:
						database.MakePet(&petstruct,77,1);
						break;
					 case  6:
						database.MakePet(&petstruct,78,1);
						break;
					 case  7:
						database.MakePet(&petstruct,79,1);
						break;
					 case  8:
						database.MakePet(&petstruct,80,1);
						break;
					 case  9:
						database.MakePet(&petstruct,81,1);
						break;
					 case 10:
						database.MakePet(&petstruct,82,1);
						break;
					 case 11:
						database.MakePet(&petstruct,83,1);
						break;
					 case 12:
						database.MakePet(&petstruct,84,1);
						break;
					 case 13:
						database.MakePet(&petstruct,85,1);
						break;
					 case 14:
						database.MakePet(&petstruct,86,1);
						break;
					 case 15:
						database.MakePet(&petstruct,87,1);
						break;
                  } //switch 
        } else {
			Message(0, "Error: Unknown Earth Pet formula");
		}
    } else if (strncmp(pettype, "SumFireR", 8) == 0) { //Baron-Sprite: This Pettype is reserved to 1. ALSO 88-101.
		int8 tmp = atoi(&pettype[8]);
		if (tmp >= 2 && tmp <= 15) {
			switch (tmp) { 
					 case  2:
						database.MakePet(&petstruct,88,1);
						break;
					 case  3:
						database.MakePet(&petstruct,89,1);
						break;
					 case  4:
						database.MakePet(&petstruct,90,1);
						break;
					 case  5:
						database.MakePet(&petstruct,91,1);
						break;
					 case  6:
						database.MakePet(&petstruct,92,1);
						break;
					 case  7:
						database.MakePet(&petstruct,93,1);
						break;
					 case  8:
						database.MakePet(&petstruct,94,1);
						break;
					 case  9:
						database.MakePet(&petstruct,95,1);
						break;
					 case 10:
						database.MakePet(&petstruct,96,1);
						break;
					 case 11:
						database.MakePet(&petstruct,97,1);
						break;
					 case 12:
						database.MakePet(&petstruct,98,1);
						break;
					 case 13:
						database.MakePet(&petstruct,99,1);
						break;
					 case 14:
						database.MakePet(&petstruct,100,1);
						break;
					 case 15:
						database.MakePet(&petstruct,101,1);
						break;
                  }
        } else {
			Message(0, "Error: Unknown Fire Pet formula");
		}
    } else if (strncmp(pettype, "SumAirR", 7) == 0) { //Baron-Sprite: This Pettype is reserved to 3. ALSO 60-73.
		int8 tmp = atoi(&pettype[7]);
        if (tmp >= 2 && tmp <= 15) {
                   switch (tmp) {
					 case  2:
						database.MakePet(&petstruct,60,1);
						break;
					 case  3:
						database.MakePet(&petstruct,61,1);
						break;
					 case  4:
						database.MakePet(&petstruct,62,1);
						break;
					 case  5:
						database.MakePet(&petstruct,63,1);
						break;
					 case  6:
						database.MakePet(&petstruct,64,1);
						break;
					 case  7:
						database.MakePet(&petstruct,65,1);
						break;
					 case  8:
						database.MakePet(&petstruct,66,1);
						break;
					 case  9:
						database.MakePet(&petstruct,67,1);
						break;
					 case 10:
						database.MakePet(&petstruct,68,1);
						break;
					 case 11:
						database.MakePet(&petstruct,69,1);
						break;
					 case 12:
						database.MakePet(&petstruct,70,1);
						break;
					 case 13:
						database.MakePet(&petstruct,71,1);
						break;
					 case 14:
						database.MakePet(&petstruct,72,1);
						break;
					 case 15:
						database.MakePet(&petstruct,73,1);
						break;
                  }
        } else {
			Message(0, "Error: Unknown Air Pet formula");
		}
    } else if (strncmp(pettype, "SumWaterR", 9) == 0) { //Baron-Sprite: This Pettype is reserved to 2. ALSO 102-115.
		int8 tmp = atoi(&pettype[9]);
		if (tmp >= 2 && tmp <= 15) {
			                   switch (tmp) {
					 case  2:
						database.MakePet(&petstruct,102,1);
						break;
					 case  3:
						database.MakePet(&petstruct,103,1);
						break;
					 case  4:
						database.MakePet(&petstruct,104,1);
						break;
					 case  5:
						database.MakePet(&petstruct,105,1);
						break;
					 case  6:
						database.MakePet(&petstruct,106,1);
						break;
					 case  7:
						database.MakePet(&petstruct,107,1);
						break;
					 case  8:
						database.MakePet(&petstruct,108,1);
						break;
					 case  9:
						database.MakePet(&petstruct,109,1);
						break;
					 case 10:
						database.MakePet(&petstruct,110,1);
						break;
					 case 11:
						database.MakePet(&petstruct,111,1);
						break;
					 case 12:
						database.MakePet(&petstruct,112,1);
						break;
					 case 13:
						database.MakePet(&petstruct,113,1);
						break;
					 case 14:
						database.MakePet(&petstruct,114,1);
						break;
					 case 15:
						database.MakePet(&petstruct,115,1);
						break;
                  }

        } else {
			Message(0, "Error: Unknown Water Pet formula");
		}
    } else if (strncmp(pettype, "Familiar1",9) == 0) { // neotokyo: reserved type 217 for familiars
        MakePet(27,1,46,1,120,3,217); //Baron-Sprite: This Pettype is reserved to 120-124.
		return;
    } else if (strncmp(pettype, "Familiar2",9) == 0) {
        MakePet(47,1,46,1,121,3,217);
		return;
    } else if (strncmp(pettype, "Familiar3",9) == 0) {
        MakePet(50,1,89,4,122,3,217);
		return;
    } else if (strncmp(pettype, "Familiar4",9) == 0) {
        MakePet(58,1,89,4,123,3,217);
		return;
    } else if (strncmp(pettype, "Familiar5",9) == 0) {
        MakePet(60,1,89,1,124,3,217);
		return;
    } else if (strncmp(pettype, "SpiritWolf", 10) == 0) { //Baron-Sprite: This Pettype is reserved to 40-45.  Looks sloppy, sorry.
		int8 tmp = atoi(&pettype[11]);

		switch (tmp) {
		case 42:
			database.MakePet(&petstruct,45,3);
			break;
		case 37:
			database.MakePet(&petstruct,44,3);
			break;
		case 34:
			database.MakePet(&petstruct,43,3);
			break;
		case 30:
			database.MakePet(&petstruct,42,3);
			break;
		case 27:
			database.MakePet(&petstruct,41,3);
			break;
		case 24:
			database.MakePet(&petstruct,40,3);
			break;
	    default:
			cout << "Unknown pettype: " << tmp<< " : Generating default type." << endl;
			MakePet(24, 1, 42, 0, 40, 7, 3);
			break;
		}
    } else if (strncmp(pettype, "BLpet", 5) == 0) { //Baron-Sprite: This Pettype is reserved to 125-137
		int8 ptype = atoi(&pettype[5]);
		int crace = this->GetRace();
		int prace=0;

		int mat=0;
        float size_mod = 1;
		#ifdef EQDEBUG
			cout << "Setting stats for BL Pet for Race: " << crace << endl;
		#endif

		switch ( crace ) {
		case VAHSHIR:
			prace = 63;
			size_mod = 1.7f;
			break;
		case TROLL:
			prace=91;
			break;
		case OGRE:
			prace=43;
			mat=3;
			break;
		case IKSAR:
			prace=42;
			mat=0;
			break;
		case BARBARIAN:
			prace=42;
			mat=2;
            size_mod = 1.5f;
			break;
		default:
			cout << "No pet type modifications defined for race: " << crace << endl;
			break;
		}
		#ifdef EQDEBUG
			cout << "Summoning BeastLord Pet: " << (int)ptype << endl;
		#endif
		switch ( ptype ) {
		case 51:
			database.MakePet(&petstruct,136,5,6*size_mod);
			break;
		case 49:
			database.MakePet(&petstruct,135,5,5.8*size_mod);
			break;
		case 47:
			database.MakePet(&petstruct,134,5,5.6*size_mod);
			break;
		case 45:
			database.MakePet(&petstruct,133,5,5.4*size_mod);
			break;
		case 43:
			database.MakePet(&petstruct,132,5,5.2*size_mod);
			break;
		case 41:
			database.MakePet(&petstruct,131,5,5*size_mod);
			break;
		case 39:
			database.MakePet(&petstruct,130,5,4.5*size_mod);
			break;
		case 31:
			database.MakePet(&petstruct,129,5,4*size_mod);
			break;
		case 26:
			database.MakePet(&petstruct,128,5,3.5*size_mod);
			break;
		case 22:
			database.MakePet(&petstruct,127,5,3*size_mod);
			break;
		case 16:
			database.MakePet(&petstruct,126,5,2.6*size_mod);
			break;
		case 9:
			database.MakePet(&petstruct,125,5,2.3*size_mod);
			break;
		default:
			MakePet(10, 1, prace, mat, 125, 2*size_mod, 5);
	        cout << "ptype not found: Making default BL pet." << endl;
			break;
		}
		petstruct.race = prace;
		petstruct.texture = mat;
    } else if (strncmp(pettype, "BLBasePet", 9) == 0) { //Baron-Sprite: This Pettype does not need a reserve, it is only a warning message.
			Message(13, "Beastlord pets are summon via the Spirit of Sharik, Khaliz, Keshuval, Herikol, Yekan, Kashek, Omakin, Zehkes, Khurenz, Khati Sha, Arag, or Sorsha spell line.  The Summon Warder ability was taken out of live sometime ago and replaced with this method. ");
			return;
    } else if (strncmp(pettype, "Animation", 9) == 0) { //Baron-Sprite: This Pettype is reserved to 46-59.
		int8 ptype = atoi(&pettype[9]);

		switch ( ptype ) {
		case 14:
			database.MakePet(&petstruct,59,2);
			break;
		case 13:
			database.MakePet(&petstruct,58,2);
			break;
		case 12:
			database.MakePet(&petstruct,57,2);
			break;
		case 11:
			database.MakePet(&petstruct,56,2);
			break;
		case 10:
			database.MakePet(&petstruct,55,2);
			break;
		case 9:
			database.MakePet(&petstruct,54,2);
			break;
		case 8:
			database.MakePet(&petstruct,53,2);
			break;
		case 7:
			database.MakePet(&petstruct,52,2);
			break;
		case 6:
			database.MakePet(&petstruct,51,2);
			break;
		case 5:
			database.MakePet(&petstruct,50,2);
			break;
		case 4:
			database.MakePet(&petstruct,49,2);
			break;
		case 3:
			database.MakePet(&petstruct,48,2);
			break;
		case 2:
			database.MakePet(&petstruct,47,2);
			break;
		case 1:
			database.MakePet(&petstruct,46,2);
			break;
		default:
			MakePet(1, 1, 127, 0, 46, 6, 2);
			cout << "ptype not found: Making default animation pet." << endl;
			break;
		}
    } else if (strncmp(pettype, "SumSword", 8) == 0) { //Baron-Sprite: This Pettype is reserved to 18.
        // for testing make an chanter pet
		MakePet(59, 1, 127,0,46,0,2);
    } else if (strncmp(pettype, "skel_pet_", 9) == 0) { //Baron-Sprite: This Pettype is reserved to 22-39.
		char sztmp[50];
		strcpy(sztmp, pettype);
		sztmp[11] = 0;
		int8 tmp = atoi(&sztmp[9]);
		//Baron-Sprite: No need for level algorithim - Since pet levels are now fixed in EQLive.
		//Baron-Sprite: MakePet(level, class, race, texture, pettype, size, type) 0 Can be a placeholder.
		if (tmp >= 65) {
			database.MakePet(&petstruct,39,4);
        } else if (tmp >= 63) {
			database.MakePet(&petstruct,38,4);
        } else if (tmp >= 61) {
			database.MakePet(&petstruct,37,4);
		} else if (tmp >= 47) {
			database.MakePet(&petstruct,36,4);
		} else if (tmp >= 44) {
			database.MakePet(&petstruct,35,4);
        } else if (tmp >= 43) {
			database.MakePet(&petstruct,34,4);
        } else if (tmp >= 41) {
			database.MakePet(&petstruct,33,4);
        } else if (tmp >= 37) {
			database.MakePet(&petstruct,32,4);
        } else if (tmp >= 33) {
			database.MakePet(&petstruct,31,4);
		} else if (tmp >= 29) {
			database.MakePet(&petstruct,30,4);
        } else if (tmp >= 25) {
			database.MakePet(&petstruct,29,4);
        } else if (tmp >= 22) {
			database.MakePet(&petstruct,28,4);
        } else if (tmp >= 19) {
			database.MakePet(&petstruct,27,4);
        } else if (tmp >= 16) {
			database.MakePet(&petstruct,26,4);
        } else if (tmp >= 11) {
			database.MakePet(&petstruct,25,4);
        } else if (tmp >= 9) {
			database.MakePet(&petstruct,24,4);
        } else if (tmp >= 5) {
			database.MakePet(&petstruct,23,4);
        } else if (tmp >= 1) {
			database.MakePet(&petstruct,22,4);
        } else {
			MakePet(1, 1, 60);
        }
	// in_level, in_class, in_race, in_texture, in_pettype, in_size, type
	/*	else if (strncmp(pettype, "MonsterSum", 9) == 0) { //Baron-Sprite: This Pettype is reserved to 5-7.
	}
	*/
    } else if (strncmp(pettype, "Mistwalker", 10) == 0) { //Baron-Sprite: This Pettype is reserved to 8.
		database.MakePet(&petstruct,8,9);
/* solar: this needs to be fixed right, commenting out for now
		zone->AddAggroMob();
	    this->GetPet()->AddToHateList(target, 1);
		Mob* sictar = entity_list.GetMob(this->GetPetID());
		if (target)
			target->AddToHateList(sictar, 1, 0);
*/
    } else if (strncmp(pettype, "TunareBane", 10) == 0) { //Baron-Sprite: This Pettype is reserved to 13.
		database.MakePet(&petstruct,13,12);
    } else if (strncmp(pettype, "DruidPet", 8) == 0) { //Baron-Sprite: This Pettype is reserved to 11.
		database.MakePet(&petstruct,11,11);
    } else if (strncmp(pettype, "SumMageMultiElement", 19) == 0) {
		database.MakePet(&petstruct,4,15);
	} else {
		Message(13, "Unknown pet type: %s", pettype);
	}
	MakePet(petstruct.level,petstruct.class_,petstruct.race,petstruct.texture,petstruct.pettype,petstruct.size,petstruct.type,petstruct.min_dmg,petstruct.max_dmg);
}

void Mob::MakePet(int8 in_level, int8 in_class, int16 in_race,
                  int8 in_texture, int8 in_pettype, float in_size,
                  int8 type, int32 min_dmg, int32 max_dmg) {
	if (this->GetPetID() != 0) {
		return;
	}
	
	NPCType* npc_type = new NPCType;
	memset(npc_type, 0, sizeof(NPCType));
	if (in_level>1)
		npc_type->hp_regen = 2;//(int)(in_level/5); fixed elsewhere if they arent engaged
	else
		npc_type->hp_regen = 1;

	if (in_race == 216) {
		npc_type->gender = 0;
	}
	else {
		npc_type->gender = 2;
	}

	if (this->IsClient())
		strcpy(npc_type->name, GetRandPetName());
	else {
		strcpy(npc_type->name, this->GetCleanName());
		strcat(npc_type->name, "'s_pet");
	}

	npc_type->level = in_level;
	npc_type->race = in_race;
	npc_type->class_ = in_class;
	npc_type->texture = in_texture;
	npc_type->helmtexture = in_texture;
	npc_type->runspeed = 1.25f;
	npc_type->bodytype = BT_Summoned; /* pets are summoned */
	npc_type->min_dmg = min_dmg;
	npc_type->max_dmg = max_dmg;

	npc_type->walkspeed = 0.7f;
	npc_type->size = in_size;
	//npc_type->npc_spells_id = this->GetNPCSpellsID();
	npc_type->npc_spells_id = 0;

	npc_type->max_hp = CalcPetHp(npc_type->level, npc_type->class_);
	npc_type->cur_hp = npc_type->max_hp;
	npc_type->fixedZ = 1;
	int pettype = in_pettype; //Baron-Sprite: Needed for necro pet types.
//int yourlevel = this->GetLevel();
	NPCType pet;

	switch(type) {
	       case 217: {
        	// wizards familiars
        	char f_name[50];
        	strcpy(f_name,this->GetCleanName());
        	strcat(f_name,"'s Familiar");
        	strcpy(npc_type->name, f_name);
        	npc_type->min_dmg = 0;  //Baron-Sprite: Naughty Familiar.  No Attack 4 u.
        	npc_type->max_dmg = 0;
        	npc_type->max_hp = 1000;
        	break;
           }
	       case 1: { //Bentareth: Mage pets, as close live as I can find, need spell procs added
                 //2 types of procs, last 3 in each category does a new type of proc
                 //Air and earth do damage ~50 hp, water does double previous, and fire needs several wizard spells added
                       npc_type->hp_regen = 6; //default case (true until lvl 39 pet)
					   npc_type->gender=2;
					    if(pettype==4){ //Mage epic pet
							database.GetPetStats(&pet,4);
							npc_type->hp_regen=50;
							npc_type->npc_spells_id = 21;
						}
						else if(pettype>=60 && pettype<74){ //Air Pets
							database.GetPetStats(&pet,pettype);
							npc_type->npc_spells_id = 14;
						}
						else if(pettype>=74 && pettype<88){ //Earth Pets
							database.GetPetStats(&pet,pettype);
							npc_type->npc_spells_id = 15;
						}
						else if(pettype>=88 && pettype<99){ //Fire Pets
							database.GetPetStats(&pet,pettype);
							npc_type->npc_spells_id = 18;
						}
						else if(pettype>=99 && pettype<102){ //Fire Pets
							database.GetPetStats(&pet,pettype);
							npc_type->npc_spells_id = 20;
							if(pettype==101)
								sprintf(npc_type->npc_attacks, "E");
							npc_type->hp_regen = 30;
						}
						else if(pettype>=102 && pettype<116){ //Water Pets
							database.GetPetStats(&pet,pettype);
							npc_type->npc_spells_id = 18;
							if(pettype==115){
								npc_type->hp_regen = 100;
								sprintf(npc_type->npc_attacks, "E");
							}
							else if(pettype>=110){
								npc_type->hp_regen = 30;
								sprintf(npc_type->npc_attacks, "E");
							}
							npc_type->npc_spells_id = 16;
						}
						else{
							printf("Unknown pet number of %i\n",pettype);
							break;
						}
						npc_type->max_hp = pet.max_hp;
						npc_type->cur_hp = pet.cur_hp;
						npc_type->min_dmg = pet.min_dmg;
						npc_type->max_dmg = pet.max_dmg;
						break;
	}
	case 2: { //Baron-Sprite: Enchanter Pets.  Some info from casters realm.
			npc_type->gender = 0; //devn00b: nfi what to do about race here.
			npc_type->equipment[7] = 34;// devn00b: or these equip fields might have to add them as an option in the db
			npc_type->equipment[8] = 202;
			if (pettype >=57 && pettype<=59) {
				database.GetPetStats(&pet,pettype);
				npc_type->equipment[7] = 26;
				npc_type->equipment[8] = 26;
			}
			else if ((pettype>=51 && pettype<=56) || (pettype>=46 && pettype<=48))
				database.GetPetStats(&pet,pettype);
			else if (pettype >= 49 && pettype<=50){
				database.GetPetStats(&pet,pettype);
				npc_type->equipment[7] = 3;
			}
			else{
				printf("Unknown pet number of %i\n",pettype);
				break;
			}
			npc_type->max_hp = pet.max_hp;
			npc_type->cur_hp = pet.cur_hp;
			npc_type->min_dmg = pet.min_dmg;
			npc_type->max_dmg = pet.max_dmg;
			break;
		}
	case 3: { //Baron-Sprite: Shaman pets.  Credits for information go mostly to eq.castersrealm.com.
		if(pettype==45){
			database.GetPetStats(&pet,pettype);
            sprintf(npc_type->npc_attacks, "E"); // should enrage, i guess
			npc_type->max_hp = pet.max_hp;
			npc_type->cur_hp = pet.cur_hp;
			npc_type->min_dmg = pet.min_dmg;
			npc_type->max_dmg = pet.max_dmg;
		}
		else if(pettype>=40 && pettype<45){
			database.GetPetStats(&pet,pettype);
			npc_type->max_hp = pet.max_hp;
			npc_type->cur_hp = pet.cur_hp;
			npc_type->min_dmg = pet.min_dmg;
			npc_type->max_dmg = pet.max_dmg;
		}
		else{
				cout << "Fallthrough case for Shaman Pet." << endl;
				npc_type->max_hp = 25;
				npc_type->cur_hp = 25;
				npc_type->min_dmg = 1;
				npc_type->max_dmg = 3;
		}
		break;
		}
	case 4: { //Baron-Sprite: Necromancer pets.  Some of the info is from eqnecro.com
			npc_type->npc_spells_id = 23;
			npc_type->bodytype = BT_SummonedUndead; /* both summoned and undead */
			if(pettype >= 37 && pettype<=39){ //Baron-Sprite: This is defined above in the Makepet statement.  I use it to single out the individual pet spells.
				database.GetPetStats(&pet,pettype);
				sprintf(npc_type->npc_attacks, "E");
				npc_type->max_hp = pet.max_hp;
				npc_type->cur_hp = pet.cur_hp;
				npc_type->min_dmg = pet.min_dmg;
				npc_type->max_dmg = pet.max_dmg;
			}
			else if(pettype >= 22 && pettype<=36)
			{
				database.GetPetStats(&pet,pettype);
				npc_type->max_hp = pet.max_hp;
				npc_type->cur_hp = pet.cur_hp;
				npc_type->min_dmg = pet.min_dmg;
				npc_type->max_dmg = pet.max_dmg;
			}
			else
			{
				npc_type->max_hp = 25;
				npc_type->cur_hp = 25;
				npc_type->min_dmg = 1;
				npc_type->max_dmg = 3;
			}
			break;
		}
	case 5: //Baron-Sprite: Beastlord pets.  Credits for information go mostly to eq.castersrealm.com.  What's new? :)
		{
		char f_name[50];
        strcpy(f_name,this->GetCleanName());
        strcat(f_name,"`s warder");
        strcpy(npc_type->name, f_name);
			if (pettype >= 125 && pettype<=136)
			{
				database.GetPetStats(&pet,pettype);
				npc_type->max_hp = pet.max_hp;
				npc_type->cur_hp = pet.cur_hp;
				npc_type->min_dmg = pet.min_dmg;
				npc_type->max_dmg = pet.max_dmg;
			}
			else
			{
				npc_type->max_hp = 300;
				npc_type->cur_hp = 300;
				npc_type->min_dmg = 5;
				npc_type->max_dmg = 10;
			}
			if (in_race == 42 && in_texture == 0)
				npc_type->gender = 1;
			else
				npc_type->gender = 2;
			break;
		}
	case 8: { //Baron-Sprite:  Mistwalker :D
		char f_name[50];
		strcat(f_name," pet");
        strcpy(npc_type->name, f_name);
		database.GetPetStats(&pet,9);
		npc_type->max_hp = pet.max_hp;
		npc_type->cur_hp = pet.cur_hp;
		npc_type->min_dmg = pet.min_dmg;
		npc_type->max_dmg = pet.max_dmg;
		break;
	}
	case 11:{ // Druid Pet...  Baron-Sprite: Info from www.eqdruids.com said about 100 hp.
			database.GetPetStats(&pet,11);
			npc_type->max_hp = pet.max_hp;
			npc_type->cur_hp = pet.cur_hp;
			npc_type->min_dmg = pet.min_dmg;
			npc_type->max_dmg = pet.max_dmg;
			break;	
	}
	case 12:{
		char f_name[50];
        strcpy(f_name,this->GetCleanName());
        strcat(f_name," pet");
        strcpy(npc_type->name, f_name);
		database.GetPetStats(&pet,13);
		npc_type->max_hp = pet.max_hp;
		npc_type->cur_hp = pet.cur_hp;
		npc_type->min_dmg = pet.min_dmg;
		npc_type->max_dmg = pet.max_dmg;
		break;
	}
	default:
		printf("Unknown type/pettype of: %i,%i.  Using Default Formula...\n",type,pettype);
		npc_type->max_hp = 5*((in_level*in_level)/2);
		npc_type->cur_hp = npc_type->max_hp;
		npc_type->min_dmg = 1;
		npc_type->max_dmg = (int)(in_level*1.2);
		break;
	}
	
	NPC* npc = new NPC(npc_type, 0,
                       this->GetX()+10, this->GetY()+10,
                       this->GetZ(), this->GetHeading());
	safe_delete(npc_type);
	npc->SetPetType(in_pettype);
	npc->SetOwnerID(this->GetID());
	entity_list.AddNPC(npc);
    if (type != 217)
	    this->SetPetID(npc->GetID());
    else
	    this->SetFamiliarID(npc->GetID());	
}



///////////////////////////////////////////////////////////////////////////////
// spell property testing functions

bool IsSacrificeSpell(int16 spell_id)
{
	return IsEffectInSpell(spell_id, SE_Sacrifice);
}

bool IsLifetapSpell(int16 spell_id)
{
	return
	(
		IsValidSpell(spell_id) &&
		(
			spells[spell_id].targettype == ST_Tap ||
			(
				spell_id == 2115	// Ancient: Lifebane
			)
		)
	);
}

bool IsMezSpell(int16 spell_id)
{
	return IsEffectInSpell(spell_id, SE_Mez);
}

bool IsStunSpell(int16 spell_id)
{
	return IsEffectInSpell(spell_id, SE_Stun);
}

bool IsSlowSpell(int16 spell_id)
{
	int i;
	SPDat_Spell_Struct sp = spells[spell_id];

	for(i = 0; i < EFFECT_COUNT; i++)
	{
		if
		(
			sp.effectid[i] == SE_AttackSpeed &&				// attack speed effect
			CalcSpellEffectValue(spell_id, i) < 100		// less than 100%
		)
			return true;
	}

	return false;
}

bool IsHasteSpell(int16 spell_id)
{
	return
	(
		IsEffectInSpell(spell_id, SE_AttackSpeed) &&
		!IsSlowSpell(spell_id)
	);

	return false;
}

bool IsPercentalHealSpell(int16 spell_id)
{
	return IsEffectInSpell(spell_id, SE_PercentalHeal);
}

bool IsGroupOnlySpell(int16 spell_id)
{
	return IsValidSpell(spell_id) && spells[spell_id].goodEffect == 2;
}

bool IsBeneficialSpell(int16 spell_id)
{
	return spells[spell_id].goodEffect != 0 || IsGroupSpell(spell_id);
}

bool IsDetrimentalSpell(int16 spell_id)
{
	return !IsBeneficialSpell(spell_id);
}

bool IsInvulnerabilitySpell(int16 spell_id)
{
	return IsEffectInSpell(spell_id, SE_DivineAura);
}

bool IsCHDurationSpell(int16 spell_id)
{
	return IsEffectInSpell(spell_id, SE_CompleteHeal);
}

bool IsPoisonCounterSpell(int16 spell_id)
{
	return IsEffectInSpell(spell_id, SE_PoisonCounter);
}

bool IsDiseaseCounterSpell(int16 spell_id)
{
	return IsEffectInSpell(spell_id, SE_DiseaseCounter);
}

bool IsSummonItemSpell(int16 spell_id)
{
	return IsEffectInSpell(spell_id, SE_SummonItem);
}

bool IsSummonSkeletonSpell(int16 spell_id)
{
	return IsEffectInSpell(spell_id, SE_NecPet);
}

bool IsSummonPetSpell(int16 spell_id)
{
	return
	(
		IsEffectInSpell(spell_id, SE_SummonPet) ||
		IsEffectInSpell(spell_id, SE_SummonBSTPet)
	);
}

bool IsCharmSpell(int16 spell_id)
{
	return IsEffectInSpell(spell_id, SE_Charm);
}

bool IsBlindSpell(int16 spell_id)
{
	return IsEffectInSpell(spell_id, SE_Blind);
}

bool IsEffectHitpointsSpell(int16 spell_id)
{
	return IsEffectInSpell(spell_id, SE_CurrentHP);
}

bool IsReduceCastTimeSpell(int16 spell_id)
{
	return IsEffectInSpell(spell_id, SE_IncreaseSpellHaste);
}

bool IsIncreaseDurationSpell(int16 spell_id)
{
	return IsEffectInSpell(spell_id, SE_IncreaseSpellDuration);
}

bool IsReduceManaSpell(int16 spell_id)
{
	return IsEffectInSpell(spell_id, SE_ReduceManaCost);
}

bool IsExtRangeSpell(int16 spell_id)
{
	return IsEffectInSpell(spell_id, SE_IncreaseRange);
}

bool IsImprovedHealingSpell(int16 spell_id)
{
	return IsEffectInSpell(spell_id, SE_ImprovedHeal);
}

bool IsImprovedDamageSpell(int16 spell_id)
{
	return IsEffectInSpell(spell_id, SE_ImprovedDamage);
}

bool IsAEDurationSpell(int16 spell_id)
{
	return IsValidSpell(spell_id) && spells[spell_id].AEDuration !=0;
}

bool IsPureNukeSpell(int16 spell_id)
{
	int i, effect_count = 0;

	if(!IsValidSpell(spell_id))
		return false;

	for(i = 0; i < EFFECT_COUNT; i++)
	{
		if(!IsBlankSpellEffect(spell_id, i))
			effect_count++;
	}

	return
	(
		spells[spell_id].effectid[0] == SE_CurrentHP &&
		effect_count == 1
	);
}

bool IsPartialCapableSpell(int16 spell_id)
{
	if(IsPureNukeSpell(spell_id))
		return true;
	
	return false;
}

bool IsResistableSpell(int16 spell_id)
{
	// solar: for now only detrimental spells are resistable.  later on i will
	// add specific exceptions for the beneficial spells that are resistable
	if(IsDetrimentalSpell(spell_id))
	{
		return true;
	}

	return false;
}

// solar: checks if this spell affects your group
bool IsGroupSpell(int16 spell_id)
{
	return
	(
		IsValidSpell(spell_id) &&
		(
			spells[spell_id].targettype == ST_AEBard ||
			spells[spell_id].targettype == ST_Group || 
			spells[spell_id].targettype == ST_GroupTeleport
		)
	);
}

// solar: checks if this spell can be targeted
bool IsTGBCompatibleSpell(int16 spell_id)
{
	return
	(
		IsValidSpell(spell_id) &&
		(
			!IsDetrimentalSpell(spell_id) &&
			spells[spell_id].buffduration != 0 &&
			!IsBardSong(spell_id) &&
			!IsEffectInSpell(spell_id, SE_Illusion)
		)
	);
}

bool IsBardSong(int16 spell_id)
{
	return
	(
		IsValidSpell(spell_id) &&
		spells[spell_id].classes[BARD - 1] < 255
	);
}

bool IsEffectInSpell(int16 spellid, int effect)
{
	int j;

	if(!IsValidSpell(spellid))
		return false;

	for(j = 0; j < EFFECT_COUNT; j++)
		if(spells[spellid].effectid[j] == effect) 
			return true;

	return false;
}

// solar: arguments are spell id and the index of the effect to check.
// this is used in loops that process effects inside a spell to skip
// the blanks
bool IsBlankSpellEffect(int16 spellid, int effect_index)
{
	int effect, base, formula;

	effect = spells[spellid].effectid[effect_index];
	base = spells[spellid].base[effect_index];
	formula = spells[spellid].formula[effect_index];

	return
	(
		effect == SE_Blank ||	// blank marker
		(	// spacer
			effect == SE_CHA && 
			base == 0 &&
			formula == 100
		)
		||
		effect == SE_StackingCommand_Block ||	// these are only used by stacking code
		effect == SE_StackingCommand_Overwrite
	);
}

// solar: checks some things about a spell id, to see if we can proceed
bool IsValidSpell(int16 spellid)
{
	return
	(
		spells_loaded &&
		spellid != 0 &&
		spellid != 1 &&
		spellid != 0xFFFF &&
		spellid < SPDAT_RECORDS &&
		spells[spellid].player_1[0]
	);
}

// solar: this will find the first occurance of effect.  this is handy
// for spells like mez and charm, but if the effect appears more than once
// in a spell this will just give back the first one.
int GetSpellEffectIndex(int16 spell_id, int effect)
{
	int i;

	if(!IsValidSpell(spell_id))
		return -1;

	for(i = 0; i < EFFECT_COUNT; i++)
	{
		if(spells[spell_id].effectid[i] == effect)
			return i;
	}

	return -1;
}

// solar: returns the level required to use the spell if that class/level
// can use it, 0 otherwise
// note: this isn't used by anything right now
int CanUseSpell(int16 spellid, int classa, int level)
{
	int level_to_use;
	
	if(!IsValidSpell(spellid) || classa >= PLAYER_CLASS_COUNT)
		return 0;

	level_to_use = spells[spellid].classes[classa - 1];

	if
	(
		level_to_use &&
		level_to_use != 255 &&
		level >= level_to_use
	)
		return level_to_use;

	return 0;
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

	APPLAYER *outapp = new APPLAYER(OP_MemorizeSpell, sizeof(MemorizeSpell_Struct));
	MemorizeSpell_Struct* p = (MemorizeSpell_Struct*)outapp->pBuffer;
	p->slot = 0;
	p->spell_id = 0x2bc;
	p->scribing = 3;
	outapp->priority = 5;
	this->CastToClient()->QueuePacket(outapp);

	safe_delete(outapp);
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
	this->CastToClient()->QueuePacket(outapp);
	safe_delete(outapp);
}

void Mob::Stun(int duration)
{
	if(casting_spell_id)
		InterruptSpell();

	if(duration > 0)
	{
		stunned = true;
		stunned_timer->Start(duration);
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
		APPLAYER* outapp = new APPLAYER(OP_MemorizeSpell, sizeof(MemorizeSpell_Struct));
		MemorizeSpell_Struct* mem = (MemorizeSpell_Struct*)outapp->pBuffer;
		mem->slot = slot;
		mem->spell_id = spell_id;
		mem->scribing = 1;
		QueuePacket(outapp);
		safe_delete(outapp);
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
		APPLAYER* outapp = new APPLAYER(OP_MemorizeSpell, sizeof(MemorizeSpell_Struct));
		MemorizeSpell_Struct* mem = (MemorizeSpell_Struct*)outapp->pBuffer;
		mem->slot = slot;
		mem->spell_id = m_pp.mem_spells[slot];
		mem->scribing = 2;
		QueuePacket(outapp);
		safe_delete(outapp);
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
		APPLAYER* outapp = new APPLAYER(OP_MemorizeSpell, sizeof(MemorizeSpell_Struct));
		MemorizeSpell_Struct* mem = (MemorizeSpell_Struct*)outapp->pBuffer;
		mem->slot = slot;
		mem->spell_id = spell_id;
		mem->scribing = 0;
		QueuePacket(outapp);
		safe_delete(outapp);
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
		m_pp.bind_x = x_pos;
		m_pp.bind_y = y_pos;
		m_pp.bind_z = z_pos;
	}
	else {
		m_pp.bind_zone_id = to_zone;
		m_pp.bind_x = new_x;
		m_pp.bind_y = new_y;
		m_pp.bind_z = new_z;
	}
}

void Client::GoToBind() {
	if (m_pp.bind_zone_id == zone->GetZoneID()) { //if same zone no reason to zone
		GMMove(m_pp.bind_x,
               m_pp.bind_y,
               m_pp.bind_z);
    } else {
		MovePC(m_pp.bind_zone_id,
               m_pp.bind_x,
               m_pp.bind_y,
               m_pp.bind_z, 1); //lets zone
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
                                                    level);
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
                                                       level);
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
                                                       level);
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
		if (buffs[i].spellid != 0xFFFF) {

			for (int j = 0; j < EFFECT_COUNT; j++) {
                // adjustments necessary for offensive npc casting behavior
                if (bOffensive) {
				    if (spells[buffs[i].spellid].effectid[j] == type) {
                        sint16 value = 
                                CalcSpellEffectValue_formula(buffs[i].durationformula,
                                               spells[buffs[i].spellid].base[j],
                                               spells[buffs[i].spellid].max[j],
                                               buffs[i].casterlevel);
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
		spell_id != 0xffff &&
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
}	

