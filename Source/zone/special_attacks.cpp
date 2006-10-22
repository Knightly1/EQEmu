/*  EQEMu:  Everquest Server Emulator
Copyright (C) 2001-2002  EQEMu Development Team (http://eqemulator.net)

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
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "masterentity.h"
#include "StringIDs.h"
#include "../common/MiscFunctions.h"

int Mob::GetKickDamage() const {
	float multiple=(GetLevel()/5);
	multiple++;
	float dmg=(
			    (
				 (GetSkill(KICK) + GetSTR() + GetLevel()) / 90
				) * multiple
			  )
			  + 1.0;	//base 10 damage???
	if(GetClass() == WARRIOR || GetClass() == WARRIORGM
	 ||GetClass() == BERSERKER || GetClass() == BERSERKERGM) {
		dmg*=1.2f;//small increase for warriors
	}
	return(int(dmg));
}

int Mob::GetBashDamage() const {
	float multiple=(GetLevel()/5);
	multiple++;

	//this is complete shite
	float dmg=(
			    (
				 (GetSkill(KICK) + GetSTR() + GetLevel()/2) / 100
				) * multiple
			  )
			  + 1.0;
	return(int(dmg));
}

void Mob::DoSpecialAttackDamage(Mob *who, int8 skill, sint32 max_damage) {
	//this really should go through the same code as normal melee damage to
	//pick up all the special behavior there
	if(target->SpecAttacks[IMMUNE_MELEE] || target->SpecAttacks[IMMUNE_MELEE_NONMAGICAL] || target->SpecAttacks[IMMUNE_MELEE_EXCEPT_BANE]) {
		max_damage = -5;
	}
	
	if(max_damage > 0) {
		target->AvoidDamage(this, max_damage);
	}
	
	target->Damage(this, max_damage, SPELL_UNKNOWN, skill);
}


void Client::OPCombatAbility(const EQApplicationPacket *app) {
	if(!target)
		return;
	//make sure were actually able to use such an attack.
	if(spellend_timer.Enabled() || IsStunned() || IsMezzed() || dead)
		return;
	
	CombatAbility_Struct* ca_atk = (CombatAbility_Struct*) app->pBuffer;
	
	if(target->GetID() != ca_atk->m_target)
		return;	//invalid packet.
	
	if(!IsAttackAllowed(target))
		return;
	
	//These two are not subject to the combat ability timer, as they
	//allready do their checking in conjunction with the attack timer
	//throwing weapons
	if(ca_atk->m_atk == 11) {
		if (ca_atk->m_skill == THROWING) {
			ThrowingAttack(target);
			return;
		}
		//ranged attack (archery)
		if (ca_atk->m_skill == ARCHERY) {
			RangedAttack(target);
			return;
		}
		//could we return here? Im not sure is m_atk 11 is used for real specials
	}
	
	//check range for all these abilities, they are all close combat stuff
	if(!CombatRange(target))
		return;
	
	if(!p_timers.Expired(&database, pTimerCombatAbility, false)) {
		Message(13,"Ability recovery time not yet met.");
		return;
	}
	
	
	if ((ca_atk->m_atk == 100) 
	  && (ca_atk->m_skill == BASH)) {    // SLAM - Bash without a shield equipped
		if (target!=this) {
			sint32 dmg = GetBashDamage();
			
			if(!target->IsClient())
				dmg = (dmg * 76) / 100;	//scale down for PVP
			else {
				CheckIncreaseSkill(BASH);
			}
			DoAnim(animTailRake);

			if(target->CheckHitChance(this, BASH, 0, BASH)) {
				DoSpecialAttackDamage(target, BASH, dmg);
			}
		
			p_timers.Start(pTimerCombatAbility, BashReuseTime-1);
		}
		return;
	}
	
	switch(GetClass())
	{
	case BERSERKER:
	case WARRIOR:
	case RANGER:
	case BEASTLORD:
		if (ca_atk->m_atk != 100 || ca_atk->m_skill != KICK) {
			break;
		}
		if (target!=this) {
			sint32 dmg = GetKickDamage();
			
			if(!target->IsClient())
				dmg = (dmg * 76) / 100;	//scale down for PVP
			else {
				CheckIncreaseSkill(KICK);
			}
			DoAnim(animKick);

			if(target->CheckHitChance(this, KICK, 0, KICK)) {
				DoSpecialAttackDamage(target, KICK, dmg);
			}
			
			p_timers.Start(pTimerCombatAbility, KickReuseTime-1);
		}
		break;
	case MONK: {
		CheckIncreaseSkill(ca_atk->m_skill);
		int reuse = MonkSpecialAttack(target, ca_atk->m_skill);
		if(reuse > 0)
			p_timers.Start(pTimerCombatAbility, reuse-1);
		break;
	}
	case ROGUE: {
		if (ca_atk->m_atk != 100 || ca_atk->m_skill != BACKSTAB) {
			break;
		}
		const ItemInst *weapon = m_inv.GetItem(SLOT_PRIMARY);
		TryBackstab(target, weapon?weapon->GetItem():NULL);
		p_timers.Start(pTimerCombatAbility, BackstabReuseTime-1);
		break;
	}
	default:
		//they have no abilities... wtf? make em wait a bit
		p_timers.Start(pTimerCombatAbility, 9);
		break;
	}
}



//returns the reuse time in sec for the special attack used.
//this code is so messed up... it does not appear to check
// hit chance at all... it seems like every attack always hits....
int Mob::MonkSpecialAttack(Mob* other, int8 type)
{
	bool candamage = true;
	int avoidchance = other->spellbonuses.AvoidMeleeChance + other->itembonuses.AvoidMeleeChance;
	if(avoidchance > 0 && MakeRandomInt(0, 99) < avoidchance) {
		candamage = false;
	}
	
	sint32 ndamage = 0;
	//PlayerProfile_Struct pp;
	float hitsuccess = (float)other->GetLevel() - (float)level;
	float hitmodifier = 0.0;
	float skillmodifier = 0.0;
	if(level > other->GetLevel())
	{
		hitsuccess += 2;
		hitsuccess *= 14;
	}
	if ((int)hitsuccess >= 40)
	{
		hitsuccess *= 3.0;
		hitmodifier = 1.1;
	}
	if ((int)hitsuccess >= 10 && hitsuccess <= 39)
	{
		hitsuccess /= 4.0;
		hitmodifier = 0.25;
	}
	else if ((int)hitsuccess < 10 && (int)hitsuccess > -1)
	{
		hitsuccess = 0.5;
		hitmodifier = 1.5;
	}
	else if ((int)hitsuccess <= -1)
	{
		hitsuccess = 0.1;
		hitmodifier = 1.8;
	}
#if EQDEBUG >= 11
    LogFile->write(EQEMuLog::Debug,"MonkSpecialAttack() 2 - %d", hitsuccess);
#endif
	if ((int)GetSkill(type) >= 100)
	{
		skillmodifier = 1;
	}
	else if ((int)GetSkill(type) >= 200)
	{
		skillmodifier = 2;
	}
	
	hitsuccess -= ((float)GetSkill(type)/10000) + skillmodifier;
#if EQDEBUG >= 11
    LogFile->write(EQEMuLog::Debug,"MonkSpecialAttack() 3 - %d", hitsuccess);
#endif
	hitsuccess += MakeRandomFloat(0,1);
#if EQDEBUG >= 11
    LogFile->write(EQEMuLog::Debug,"MonkSpecialAttack() 4 - %d", hitsuccess);
#endif
	float ackwardtest = 2.4f;
	float random = MakeRandomFloat(0,1);
	if(random <= 0.2)
	{
		ackwardtest = 4.5;
	}
	if(random > 85 && random < 400.0)
	{
		ackwardtest = 3.2;
	}
	if(random > 400 && random < 800.0)
	{
		ackwardtest = 3.7;

	}
	if(random > 900 && random < 1400.0)
	{
		ackwardtest = 1.9;
	}
	if(random > 1400 && random < 14000.0)
	{
		ackwardtest = 2.3;
	}

	if(random > 14000 && random < 24000.0)
	{
		ackwardtest = 1.3;
	}
	if(random > 24000 && random < 34000.0)
	{
		ackwardtest = 1.3;
	}
	if(random > 990000)
	{
		ackwardtest = 1.2;
	}
	if(random < 0.2)
	{
		ackwardtest = 0.8;
	}
	
	int reuse = 0;
	
	ackwardtest += MakeRandomFloat(0,1);
	ackwardtest = abs((long)ackwardtest);
	if (type == FLYING_KICK) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (10 * ackwardtest) * (GetSkill(FLYING_KICK) + GetSTR() + level) / 600);
		if(other->IsClient())
			ndamage = ndamage * 3 / 4;
		if (MakeRandomFloat(0,1) < 0.2) {
			ndamage = (sint32) (ndamage * 1.9);
			
			//I dont know how this can ever get to be negative...
			if(ndamage <= 0) {
				entity_list.MessageClose(this, false, 200, 10, "%s misses at an attempt to thunderous kick %s!",name,other->name);
			}
			else {
				entity_list.MessageClose(this, false, 200, 10, "%s lands a thunderous kick!(%d)", name, ndamage);
			}
		}
		
		if(IsClient() && CastToClient()->CheckDiscipline(discThunderkick, true)) {
			//values are very approximate
			if(ndamage < 81)
				ndamage = 81;
			else
				ndamage = ndamage * 4 / 3;
		}
		
		if(candamage)
			DoSpecialAttackDamage(other, type, ndamage);
		DoAnim(animFlyingKick);
		reuse = FlyingKickReuseTime;
	}
	else if (type == TIGER_CLAW) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (4 * ackwardtest) * (GetSkill(TIGER_CLAW) + GetSTR() + level) / 700);
		if(other->IsClient())
			ndamage = ndamage * 7 / 10;
		if(candamage)
			DoSpecialAttackDamage(other, type, ndamage);
		DoAnim(animTigerClaw);
		reuse = TigerClawReuseTime;
	}
	else if (type == ROUND_KICK) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (6 * ackwardtest) * (GetSkill(ROUND_KICK) + GetSTR() + level) / 600);
		if(other->IsClient())
			ndamage = ndamage * 9 / 10;
		if(candamage)
			DoSpecialAttackDamage(other, type, ndamage);
		DoAnim(animRoundKick);
		reuse = RoundKickReuseTime;
	}
	else if (type == EAGLE_STRIKE) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (8 * ackwardtest) * (GetSkill(EAGLE_STRIKE) + GetSTR() + level) / 800);
		if(other->IsClient())
			ndamage = ndamage * 7 / 10;
		
		
		if(IsClient() && CastToClient()->CheckDiscipline(discAshenhand, true)) {
			//values are very approximate
			ndamage = ndamage * 3;
			if(other->GetLevel() < 49 && MakeRandomFloat(0,1) < 0.005)
				ndamage = 32000;
		}
		
		if(candamage)
			DoSpecialAttackDamage(other, type, ndamage);
		DoAnim(animEagleStrike);
		reuse = EagleStrikeReuseTime;
	}
	else if (type == DRAGON_PUNCH) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (10 * ackwardtest) * (GetSkill(DRAGON_PUNCH) + GetSTR() + level) / 600);
		if(other->IsClient())
			ndamage = ndamage * 7 / 10;
		
		if(IsClient() && CastToClient()->CheckDiscipline(discSilentfist, true)) {
			//values are very approximate
			ndamage = ndamage * 4 / 3;
			if(MakeRandomFloat(0,1) < 0.5)	//chance should be right
				other->Stun(2);		//duration unknown
		}
		
		if(candamage)
			DoSpecialAttackDamage(other, type, ndamage);
		DoAnim(animTailRake);
		reuse = TailRakeReuseTime;
	}
	else if (type == KICK) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (6 * ackwardtest) * (GetSkill(KICK) + GetSTR() + level) / 1000);
		if(other->IsClient())
			ndamage = ndamage * 7 / 10;
		if(candamage)
			DoSpecialAttackDamage(other, type, ndamage);
		DoAnim(animKick);
		reuse = KickReuseTime;
	}
	return(reuse);
}

void Mob::TryBackstab(Mob *other, const Item_Struct* weapon) {
	if(!other)
		return;
	
	//make sure we have a proper weapon if we are a client.
	if(IsClient()) {
		if(	weapon == NULL 	//no weapon
			|| weapon->ItemClass != ItemClassCommon	//not possibly piercing
			|| (weapon->ItemType != ItemTypePierce	//not a piercer
				&& weapon->ItemType != ItemType2HPierce) ) {
			Message_StringID(13, BACKSTAB_WEAPON);
			return;
		}
	}
	
	if (BehindMob(other, GetX(), GetY())) // Player is behind other
	{
		// solar - chance to assassinate
		
		// TODO: it's set to 40% chance, should be a formula involving DEX
		float chance=40;
		if(
			level >= 60 && // player is 60 or higher
			other->GetLevel() <= 45 && // mob 45 or under
			!other->CastToNPC()->IsEngaged() && // not aggro
			other->GetHP()<=32000
			&& other->IsNPC()
			&& MakeRandomFloat(0, 99) < chance // chance
			) {
			
			//char temp[100];
			//snprintf(temp, 100, "%s ASSASSINATES their victim!!", this->GetName());
			//entity_list.MessageClose(this, 0, 200, 10, temp);
			entity_list.MessageClose_StringID(this, false, 200, 10, ASSASSINATES, GetName());
			if(IsClient())
				CastToClient()->CheckIncreaseSkill(BACKSTAB);
			RogueAssassinate(other);
		}
		else {
			RogueBackstab(other, weapon, GetSkill(BACKSTAB));
			if (level > 54) {
				float DoubleAttackProbability = (GetSkill(DOUBLE_ATTACK) + GetLevel()) / 500.0f; // 62.4 max
				// Check for double attack with main hand assuming maxed DA Skill (MS)
				float random = MakeRandomFloat(0, 1);
				
				if(random < DoubleAttackProbability)		// Max 62.4 % chance of DA
					if(other->GetHP() > 0)
						RogueBackstab(other, weapon, GetSkill(BACKSTAB));
			}
			if(IsClient())
				CastToClient()->CheckIncreaseSkill(BACKSTAB);
		}
	}
	else if(GetAA(aaChaoticStab) > 0) {
		//we can stab from any angle, we do min damage though.
		RogueBackstab(other, weapon, GetSkill(BACKSTAB), true);
		if (level > 54) {
			float DoubleAttackProbability = (GetSkill(DOUBLE_ATTACK) + GetLevel()) / 500.0f; // 62.4 max
			if(IsClient())
				CastToClient()->CheckIncreaseSkill(BACKSTAB);
			// Check for double attack with main hand assuming maxed DA Skill (MS)
			float random = MakeRandomFloat(0, 1);
			if(random < DoubleAttackProbability)		// Max 62.4 % chance of DA
				if(other->GetHP() > 0)
					RogueBackstab(other, weapon, GetSkill(BACKSTAB), true);
		}
	}
	else {	// Player is in front of other... do we want to give them extra attacks like this?
		Attack(other, 13);
		if (level > 54) {
			float DoubleAttackProbability = (GetSkill(DOUBLE_ATTACK) + GetLevel()) / 500.0f; // 62.4 max
			
			// Check for double attack with main hand assuming maxed DA Skill (MS)
			float random = MakeRandomFloat(0, 1);
			if(random < DoubleAttackProbability)		// Max 62.4 % chance of DA
				if(other->GetHP() > 0)
					Attack(other, 13);
		}
	}
}

//heko: backstab
void Mob::RogueBackstab(Mob* other, const Item_Struct* weapon, int8 bs_skill, bool min_damage)
{
	int ndamage = 0;
	int max_hit, min_hit;
	float skillmodifier = 0.0;
	int8 primaryweapondamage;
	if (weapon && weapon->ItemClass == ItemClassCommon)
		primaryweapondamage = weapon->Damage; //backstab uses primary weapon
	else
		primaryweapondamage = this->GetLevel() % 10; // fallback incase it's a npc without a weapon
	
    // catch a divide by zero error
    if (!bs_skill)
        return;
	
	skillmodifier = (float)bs_skill/25.0;	//formula's from www.thesafehouse.org
	
	// formula is (weapon damage * 2) + 1 + (level - 25)/3 + (strength+skill)/100
	max_hit = (int)(((float)primaryweapondamage * 2.0) + 1.0 + ((level - 25)/3.0) + ((GetSTR()+GetSkill(BACKSTAB))/100));
	max_hit *= (int)skillmodifier;
	
	// determine minimum hits
	if (level < 51)
	{
		min_hit = 0;
	}
	else
	{
		// Trumpcard:  Replaced switch statement with formula calc.  This will give minhit increases all the way to 65.
		min_hit= (int)( level * ( 1.5 + ( (level - 51) * .05 ) ));
	}
	if (max_hit < min_hit)
		max_hit = min_hit;
	if(min_damage)
		ndamage = min_hit;
	else
		ndamage = min_hit + MakeRandomInt(0, max_hit-min_hit);	// TODO: better formula, consider mob level vs player level, strength/atk
	
//checked elsewhere	
//	if (!BehindMob(other, GetX(), GetY()))
//		ndamage = min_hit;
	//other->Damage(this, ndamage, 0xffff, BACKSTAB);
	DoSpecialAttackDamage(other, BACKSTAB, ndamage);
	DoAnim(animPiercing);	//piercing animation
}

// solar - assassinate
void Mob::RogueAssassinate(Mob* other)
{
	//can you dodge, parry, etc.. an assassinate??
	//if so, use DoSpecialAttackDamage(other, BACKSTAB, 32000); instead
	other->Damage(this, 32000, 0xffff, BACKSTAB);
	DoAnim(animPiercing);	//piercing animation
}

float Client::RangedHitChance(uint8 skill, Mob *other) {
	float chancetohit = 0;
	if(target->IsNPC())
		chancetohit = GetSkill(skill) / 3.75;
	else
		chancetohit = GetSkill(skill) / 4.75; //harder to hit players

	if (m_pp.level-target->GetLevel() < 0) {
		chancetohit -= (float)((other->GetLevel()-GetLevel())*(other->GetLevel()-GetLevel()))/4;
	}
	
	int16 targetagi = other->GetAGI();
	
	targetagi = (targetagi <= 200) ? targetagi:targetagi + ((targetagi-200)/5);
	chancetohit -= (float)targetagi*0.05;
	chancetohit += GetDEX()/2.0f;
	
	//minimum 15% chance to hit?
	if(chancetohit > 0)
		chancetohit += 30;
	else
		chancetohit = 30;
	
	float hit_bonuses = 0;
	if(spellbonuses.HitChanceSkill == skill || spellbonuses.HitChanceSkill == 0xFF)
		hit_bonuses += (spellbonuses.HitChance*2.0f) / 15.0f;
	if(itembonuses.HitChanceSkill == skill || spellbonuses.HitChanceSkill == 0xFF)
		hit_bonuses += (itembonuses.HitChance*2.0f) / 15.0f;
	chancetohit += hit_bonuses;
	
	//cap chance to hit at 95%, if they arnt garunteed
	if(hit_bonuses >= 600)	//garunteed hit disipline
		chancetohit = 600;
	else if(chancetohit > 190)
		chancetohit = 190;
	
	return(chancetohit / 2.0);
}

void Client::RangedAttack(Mob* other) {
	//conditions to use an attack checked before we are called
	
	//make sure the attack and ranged timers are up
	//if the ranged timer is disabled, then they have no ranged weapon and shouldent be attacking anyhow
	if((attack_timer.Enabled() && !attack_timer.Check(false)) || (ranged_timer.Enabled() && !ranged_timer.Check())) {
		mlog(COMBAT__RANGED, "Ranged attack canceled. Timer not up. Attack %d, ranged %d", attack_timer.GetRemainingTime(), ranged_timer.GetRemainingTime());
		Message(0, "Error: Timer not up. Attack %d, ranged %d", attack_timer.GetRemainingTime(), ranged_timer.GetRemainingTime());
		return;
	}
	
	//NOTE: augments are not properly supported on bows.
	
	const ItemInst* RangeWeapon = m_inv[SLOT_RANGE];
	
	//locate ammo
	int ammo_slot = SLOT_AMMO;
	const ItemInst* Ammo = m_inv[SLOT_AMMO];
	
	if (!RangeWeapon || !RangeWeapon->IsType(ItemClassCommon)) {
		mlog(COMBAT__RANGED, "Ranged attack canceled. Missing or invalid ranged weapon (%d) in slot %d", GetItemIDAt(SLOT_RANGE), SLOT_RANGE);
		Message(0, "Error: Rangeweapon: GetItem(%i)==0, you have no bow!", GetItemIDAt(SLOT_RANGE));
		return;
	}
	if (!Ammo || !Ammo->IsType(ItemClassCommon)) {
		mlog(COMBAT__RANGED, "Ranged attack canceled. Missing or invalid ammo item (%d) in slot %d", GetItemIDAt(SLOT_AMMO), SLOT_AMMO);
		Message(0, "Error: Ammo: GetItem(%i)==0, you have no ammo!", GetItemIDAt(SLOT_AMMO));
		return;
	}
	
	const Item_Struct* RangeItem = RangeWeapon->GetItem();
	const Item_Struct* AmmoItem = Ammo->GetItem();
	
	if(RangeItem->ItemType != ItemTypeBow) {
		mlog(COMBAT__RANGED, "Ranged attack canceled. Ranged item is not a bow. type %d.", RangeItem->ItemType);
		Message(0, "Error: Rangeweapon: Item %d is not a bow.", RangeWeapon->GetID());
		return;
	}
	if(AmmoItem->ItemType != ItemTypeArrow) {
		mlog(COMBAT__RANGED, "Ranged attack canceled. Ammo item is not an arrow. type %d.", AmmoItem->ItemType);
		Message(0, "Error: Ammo: type %d != %d, you have the wrong type of ammo!", AmmoItem->ItemType, ItemTypeArrow);
		return;
	}
	
	mlog(COMBAT__RANGED, "Shooting %s with bow %s (%d) and arrow %s (%d)", target->GetName(), RangeItem->Name, RangeItem->ID, AmmoItem->Name, AmmoItem->ID);
	
	//look for ammo in inventory if we only have 1 left...
	if(Ammo->GetCharges() == 1) {
		//first look for quivers
		int r;
		bool found = false;
		for(r = SLOT_PERSONAL_BEGIN; r <= SLOT_PERSONAL_END; r++) {
			const ItemInst *pi = m_inv[r];
			if(pi == NULL || !pi->IsType(ItemClassContainer))
				continue;
			const Item_Struct* bagitem = pi->GetItem();
			if(!bagitem || bagitem->BagType != bagTypeQuiver)
				continue;
			
			//we found a quiver, look for the ammo in it
			int i;
			for (i = 0; i < bagitem->BagSlots; i++) {
				ItemInst* baginst = pi->GetItem(i);
				if(!baginst)
					continue;	//empty
				if(baginst->GetID() == Ammo->GetID()) {
					//we found it... use this stack
					//the item wont change, but the instance does
					Ammo = baginst;
					ammo_slot = m_inv.CalcSlotId(r, i);
					found = true;
					mlog(COMBAT__RANGED, "Using ammo from quiver stack at slot %d. %d in stack.", ammo_slot, Ammo->GetCharges());
					break;
				}
			}
			if(found)
				break;
		}
		
		if(!found) {
			//if we dont find a quiver, look through our inventory again
			//not caring if the thing is a quiver.
			sint32 aslot = m_inv.HasItem(AmmoItem->ID, 1, invWherePersonal);
			if(aslot != SLOT_INVALID) {
				ammo_slot = aslot;
				Ammo = m_inv[aslot];
				mlog(COMBAT__RANGED, "Using ammo from inventory stack at slot %d. %d in stack.", ammo_slot, Ammo->GetCharges());
			}
		}
	}
	
	float range = RangeItem->Range + AmmoItem->Range + 5; //Fudge it a little, client will let you hit something at 0 0 0 when you are at 205 0 0
	mlog(COMBAT__RANGED, "Calculated bow range to be %.1f", range);
	range *= range;
	if(DistNoRootNoZ(*target) > range) {
		mlog(COMBAT__RANGED, "Ranged attack out of range... client should catch this. (%f > %f).\n", DistNoRootNoZ(*target), range);
		//target is out of range, client does a message
		return;
	}
	
	DoAnim(animShootBow);
	
	//send item animation struct
	SendItemAnimation(target, AmmoItem);
	
	float chancetohit = RangedHitChance(ARCHERY, target);
	
	// Hit?
	if (MakeRandomFloat(0, 100) > chancetohit) {
		mlog(COMBAT__RANGED, "Ranged attack missed %s. %.3f%%% chance to hit.", target->GetName(), chancetohit);
		//this->Message(MT_Emote, "You missed your target");
		//this->Message_StringID(M,GENERIC_MISS,"You","your target.");
		target->Damage(this, 0, SPELL_UNKNOWN, ARCHERY);
	} else {
		mlog(COMBAT__RANGED, "Ranged attack hit %s. %.3f%%% chance to hit.", target->GetName(), chancetohit);
		
		bool was_bane1, was_bane2;
		uint16 WDmg = GetWeaponDamage(target, RangeItem, was_bane1);
		uint16 ADmg = GetWeaponDamage(target, AmmoItem, was_bane2);
		if(other->SpecAttacks[IMMUNE_MELEE_EXCEPT_BANE] && !was_bane1 && !was_bane2) {
			//mob is immune to this attack
			mlog(COMBAT__RANGED, "Ranged attack avoided. %s is immune to non-bane damage.", target->GetName());
			target->Damage(this, -5, SPELL_UNKNOWN, ARCHERY);
		} else if(other->SpecAttacks[IMMUNE_MELEE]) {
			//mob is immune to this attack
			mlog(COMBAT__RANGED, "Ranged attack avoided. %s is immune to all melee damage.", target->GetName());
			target->Damage(this, -5, SPELL_UNKNOWN, ARCHERY);
		} else if(other->SpecAttacks[IMMUNE_MELEE_NONMAGICAL] && (!RangeItem->Magic && !AmmoItem->Magic)) {
			//mob is immune to this attack
			mlog(COMBAT__RANGED, "Ranged attack avoided. %s is immune non-magical damage.", target->GetName());
			target->Damage(this, -5, SPELL_UNKNOWN, ARCHERY);
		} else {
			uint16 levelBonus = (GetSTR()+GetLevel()+GetSkill(ARCHERY)) / 100;
			uint16 MaxDmg = (WDmg+ADmg)*levelBonus;
						
			switch(GetAA(aaArcheryMastery)) {
				case 1:
					MaxDmg = MaxDmg * 115/100;
					break;
				case 2:
					MaxDmg = MaxDmg * 125/100;
					break;
				case 3:
					MaxDmg = MaxDmg * 150/100;
					break;
			}
			
			mlog(COMBAT__RANGED, "Bow DMG %d, Arrow DMG %d, level bonus %d. Max Damage %d", WDmg, ADmg, levelBonus, MaxDmg);
			
			
			if(GetClass()==RANGER && target->IsNPC() && !target->IsMoving() && !target->IsRooted() && GetLevel() > 50){
				MaxDmg *= 2;
				mlog(COMBAT__RANGED, "Ranger. Target is stationary, doubling max damage to %d", MaxDmg);
			}
			
			if(target->IsClient()) { //Tone down pvp damage
				MaxDmg *= 3/4;
				mlog(COMBAT__RANGED, "PVP Max Damage reduction to %d", MaxDmg);
			}
			
			sint32 TotalDmg = 0;
			sint32 critDmg = 0;
			
			if (MaxDmg == 0)
				MaxDmg = 1;
			
			TotalDmg = 1 + MakeRandomInt(0, MaxDmg);
			
			// no crits before level 12 cap is maxed
			if(GetClass() == RANGER && GetSkill(ARCHERY) > 65 && chancetohit < 85 &&
			  ((uint16)MakeRandomInt(0, 355) < (GetSkill(ARCHERY)+GetDEX())/4)) {
				
				critDmg = (sint32)(TotalDmg * 2);
				
			  	mlog(COMBAT__RANGED, "Landed a critical hit on %s for %d damage. Archery %d, DEX %d", target->GetName(), critDmg, GetSkill(ARCHERY), GetDEX());
				char val1[20]={0};
				entity_list.MessageClose_StringID(this, false, 200, MT_CritMelee, CRITICAL_HIT, GetName(), ConvertArray(critDmg,val1));
				//this->Message_StringID(MT_CritMelee,CRITICAL_HIT,GetName(),ConvertArray(critDmg,val1));
				//this->Message(MT_CritMelee, "You score a critical hit!(%d)", critDmg);
				target->Damage(this, critDmg, SPELL_UNKNOWN, ARCHERY);
			} else {
			  	mlog(COMBAT__RANGED, "Arrow hit %s for %d damage.", target->GetName(), TotalDmg);
				char hitname[64]={0};
				strncpy(hitname,target->GetName(),strlen(target->GetName())-2);
				//char val1[20]={0};
				//Message_StringID(MT_Emote,HIT_NON_MELEE,"You",hitname,ConvertArray(TotalDmg,val1));
				//this->Message(MT_Emote, "You Hit for a total of %d non-melee damage.", TotalDmg);
				target->Damage(this, TotalDmg, SPELL_UNKNOWN, ARCHERY);
			}
		}
	}
	
	//try proc on hits and misses
	if(target && (target->GetHP() > -10))
		TryWeaponProc(RangeWeapon, target);
	
	//consume ammo (should stay at the end, after we are done with everything)
	if(!GetAA(aaEndlessQuiver)) {
		DeleteItemInInventory(ammo_slot, 1, true);
		mlog(COMBAT__RANGED, "Consumed one arrow from slot %d", ammo_slot);
	} else {
		mlog(COMBAT__RANGED, "Endless Quiver prevented ammo consumption.");
	}
	
	
	// See if the player increases their skill - with cap
	/*float wisebonus =  (GetWIS() > 200) ? 20 + ((GetWIS() - 200) * 0.05) : GetWIS() * 0.1;
	
	if (((55-(GetSkill(ARCHERY)*0.240))+wisebonus > MakeRandomFloat(0, 100)) && (GetSkill(ARCHERY)<(m_pp.level+1)*5) && GetSkill(ARCHERY) < 252)
		this->SetSkill(ARCHERY,GetRawSkill(ARCHERY)+1);*/
	CheckIncreaseSkill(ARCHERY);
}

void Client::ThrowingAttack(Mob* other) { //old was 51
	//conditions to use an attack checked before we are called
	
	//make sure the attack and ranged timers are up
	//if the ranged timer is disabled, then they have no ranged weapon and shouldent be attacking anyhow
	if((attack_timer.Enabled() && !attack_timer.Check(false)) || (ranged_timer.Enabled() && !ranged_timer.Check())) {
		mlog(COMBAT__RANGED, "Throwing attack canceled. Timer not up. Attack %d, ranged %d", attack_timer.GetRemainingTime(), ranged_timer.GetRemainingTime());
		Message(0, "Error: Timer not up. Attack %d, ranged %d", attack_timer.GetRemainingTime(), ranged_timer.GetRemainingTime());
		return;
	}
		
	int ammo_slot = SLOT_RANGE;
	const ItemInst* RangeWeapon = m_inv[SLOT_RANGE];
	//enabling this fucks with attack timers:
	//if(!RangeWeapon)
	//	RangeWeapon = m_inv[SLOT_AMMO];
	
	if (!RangeWeapon || !RangeWeapon->IsType(ItemClassCommon)) {
		mlog(COMBAT__RANGED, "Ranged attack canceled. Missing or invalid ranged weapon (%d) in slot %d", GetItemIDAt(SLOT_RANGE), SLOT_RANGE);
		Message(0, "Error: Rangeweapon: GetItem(%i)==0, you have nothing to throw!", GetItemIDAt(SLOT_RANGE));
		return;
	}
	
	const Item_Struct* item = RangeWeapon->GetItem();
	if(item->ItemType != ItemTypeThrowing && item->ItemType != ItemTypeThrowingv2) {
		mlog(COMBAT__RANGED, "Ranged attack canceled. Ranged item %d is not a throwing weapon. type %d.", item->ItemType);
		Message(0, "Error: Rangeweapon: GetItem(%i)==0, you have nothing useful to throw!", GetItemIDAt(SLOT_RANGE));
		return;
	}
	
	mlog(COMBAT__RANGED, "Throwing %s (%d) at %s", item->Name, item->ID, target->GetName());
	
	if(RangeWeapon->GetCharges() == 1) {
		//first check ammo
		const ItemInst* AmmoItem = m_inv[SLOT_AMMO];
		if(AmmoItem != NULL && AmmoItem->GetID() == RangeWeapon->GetID()) {
			//more in the ammo slot, use it
			RangeWeapon = AmmoItem;
			ammo_slot = SLOT_AMMO;
			mlog(COMBAT__RANGED, "Using ammo from ammo slot, stack at slot %d. %d in stack.", ammo_slot, RangeWeapon->GetCharges());
		} else {
			//look through our inventory for more
			sint32 aslot = m_inv.HasItem(item->ID, 1, invWherePersonal);
			if(aslot != SLOT_INVALID) {
				//the item wont change, but the instance does, not that it matters
				ammo_slot = aslot;
				RangeWeapon = m_inv[aslot];
				mlog(COMBAT__RANGED, "Using ammo from inventory slot, stack at slot %d. %d in stack.", ammo_slot, RangeWeapon->GetCharges());
			}
		}
	}
	
	int range = item->Range +50/*Fudge it a little, client will let you hit something at 0 0 0 when you are at 205 0 0*/;
	mlog(COMBAT__RANGED, "Calculated bow range to be %.1f", range);
	range *= range;
	if(DistNoRootNoZ(*target) > range) {
		mlog(COMBAT__RANGED, "Throwing attack out of range... client should catch this. (%f > %f).\n", DistNoRootNoZ(*target), range);
		//target is out of range, client does a message
		return;
	}
	
	// Throw stuff
	DoAnim(animShootBow);
//	DoAnim(anim1HWeapon);		//same number as 1HS/1HB, this is prolly wrong..
	
	//send item animation
	SendItemAnimation(target, item);
	 
//	uint8 WDmg = item->Damage;
	sint32 TotalDmg = 0;
	
	float chancetohit = RangedHitChance(THROWING, target);
	
	// Hit?
	if (MakeRandomFloat(0, 100) > chancetohit) {
		mlog(COMBAT__RANGED, "Ranged attack missed %s. %.3f%%% chance to hit.", target->GetName(), chancetohit);
		//this->Message(MT_Emote, "You missed your target");
		//this->Message_StringID(M,GENERIC_MISS,"You","your target.");
		target->Damage(this, 0, SPELL_UNKNOWN, THROWING);
	} else {
		mlog(COMBAT__RANGED, "Throwing attack hit %s. %.3f%%% chance to hit.", target->GetName(), chancetohit);
		
		bool was_bane1;
		uint16 WDmg = GetWeaponDamage(target, item, was_bane1);
		if(other->SpecAttacks[IMMUNE_MELEE_EXCEPT_BANE] && !was_bane1) {
			//mob is immune to this attack
			mlog(COMBAT__RANGED, "Throwing attack avoided. %s is immune to non-bane damage.", target->GetName());
			target->Damage(this, -5, SPELL_UNKNOWN, THROWING);
		} else if(other->SpecAttacks[IMMUNE_MELEE]) {
			//mob is immune to this attack
			mlog(COMBAT__RANGED, "Throwing attack avoided. %s is immune to all melee damage.", target->GetName());
			target->Damage(this, -5, SPELL_UNKNOWN, THROWING);
		} else if(other->SpecAttacks[IMMUNE_MELEE_NONMAGICAL] && !item->Magic) {
			//mob is immune to this attack
			mlog(COMBAT__RANGED, "Throwing attack avoided. %s is immune non-magical damage.", target->GetName());
			target->Damage(this, -5, SPELL_UNKNOWN, THROWING);
		} else {
			
			//this is a terrible damage formula...
			
			
			uint8 levelBonus = (GetSTR()+GetLevel()+GetSkill(THROWING)) / 100;
			uint8 MaxDmg = (WDmg)*levelBonus;
			if (MaxDmg == 0)
				MaxDmg = 1;
			TotalDmg = 1 + MakeRandomInt(0, MaxDmg);
			mlog(COMBAT__RANGED, "Item DMG %d, level bonus %d. Max Damage %d. Hit for damage %d", WDmg, levelBonus, MaxDmg, TotalDmg);
			//this->Message(MT_Emote, "You Hit for a total of %d damage.", TotalDmg);
			target->Damage(this, TotalDmg, SPELL_UNKNOWN, THROWING);
		}
	}
	
	if(target && (target->GetHP() > -10))
		TryWeaponProc(RangeWeapon, target);
	
	//consume ammo
	DeleteItemInInventory(ammo_slot, 1, true);
	
	// See if the player increases their skill - with cap
	/*float wisebonus =  (GetWIS() > 200) ? 20 + ((GetWIS() - 200) * 0.05) : GetWIS() * 0.1;
	
	if (((55-(GetSkill(THROWING)/4))+wisebonus > MakeRandomInt(0, 100)) && GetSkill(THROWING) < (uint16)((GetLevel()*5)+5))
		SetSkill(THROWING,GetRawSkill(THROWING)+1);*/
	CheckIncreaseSkill(THROWING);
}

void Mob::SendItemAnimation(Mob *to, const Item_Struct *item) {
	EQApplicationPacket *outapp = new EQApplicationPacket(OP_SomeItemPacketMaybe, sizeof(Arrow_Struct));
	Arrow_Struct *as = (Arrow_Struct *) outapp->pBuffer;
	as->type = 1;
	as->src_x = GetX();
	as->src_y = GetY();
	as->src_z = GetZ();
	as->source_id = GetID();
	as->target_id = to->GetID();
	as->item_id = item->ID;
	strncpy(as->model_name, item->IDFile, 16);
	

	/*
		The angular field affects how the object flies towards the target.
		A low angular (10) makes it circle the target widely, where a high 
		angular (20000) makes it go straight at them.

		The tilt field causes the object to be tilted flying through the air
		and also seems to have an effect on how it behaves when circling the
		target based on the angular field.

		Arc causes the object to form an arc in motion. A value too high will
	*/
	as->velocity = 4.0;
	as->launch_angle = 140;
	as->tilt = 0;
	as->arc = 1;
	
	
	//fill in some unknowns, we dont know their meaning yet
	//neither of these seem to change the behavior any
	as->unknown088 = 125;
	as->unknown092 = 16;
	
	entity_list.QueueCloseClients(this, outapp);
	safe_delete(outapp);
}

void NPC::DoClassAttacks(Mob *target) {
	if(target == NULL)
		return;	//gotta have a target for all these
	
	bool taunt_time = taunt_timer.Check();
	bool ca_time = classattack_timer.Check(false);
	
	//only check attack allowed if we are funna do something
	if((taunt_time || ca_time) && !IsAttackAllowed(target))
		return;
	
	//general stuff, for all classes....
	//only gets used when their primary ability get used too
	//this might be bad for pally's with long reuse time
	if (taunting && HasOwner() && target->IsNPC() && target->GetBodyType() != BT_Undead && taunt_time) {
		Taunt(target->CastToNPC(), false);
	}
	
	if(!ca_time)
		return;
	
	int level = GetLevel();
	int reuse = TauntReuseTime * 1000;	//make this very long since if they dont use it once, they prolly never will
	bool did_attack;
	//class specific stuff...
	switch(GetClass()) {
		case ROGUE: case ROGUEGM:
			if(level >= 10) {
				const Item_Struct* weapon = NULL;
//currently disabled because the values in primary and secondary are ID files, not item ids...
//				if(equipment[MATERIAL_PRIMARY] != 0)
//					weapon = database.GetItem(equipment[MATERIAL_PRIMARY]);
				TryBackstab(target, weapon);
				reuse = BackstabReuseTime * 1000;
				did_attack = true;
			}
			break;
		case MONK: case MONKGM: {
			int8 satype = KICK;
			if(level > 29) {
				satype = FLYING_KICK;
			} else if(level > 24) {
				satype = DRAGON_PUNCH;
			} else if(level > 19) {
				satype = EAGLE_STRIKE;
			} else if(level > 9) {
				satype = TIGER_CLAW;
			} else if(level > 4) {
				satype = ROUND_KICK;
			}
			reuse = MonkSpecialAttack(target, satype);
			reuse *= 1000;
			did_attack = true;
			break;
		}
		case BERSERKER: case BERSERKERGM:
		case WARRIOR: case WARRIORGM:
		case RANGER: case RANGERGM:
		case BEASTLORD: case BEASTLORDGM: {
			//kick
			
			DoAnim(animKick);
			
			sint32 dmg = GetKickDamage();
			if(target->CheckHitChance(this, KICK, 0, KICK)) {
				DoSpecialAttackDamage(target, KICK, dmg);
			}
			
			reuse = KickReuseTime * 1000;
			did_attack = true;
			break;
		}
		case SHADOWKNIGHT: case SHADOWKNIGHTGM:
			CastSpell(SPELL_NPC_HARM_TOUCH, target->GetID());
			reuse = HarmTouchReuseTime * 1000;
			did_attack = true;
			break;
		case PALADIN: case PALADINGM:
			if(GetHPRatio() < 20) {
				CastSpell(SPELL_LAY_ON_HANDS, GetID());
				reuse = LayOnHandsReuseTime * 1000;
			} else {
				reuse = 1000 * 5;	//check again in 5 seconds
			}
			break;
	}

	if(did_attack) {
		if(!combat_event) {
			mlog(COMBAT__HITS, "Triggering EVENT_COMBAT due to attack on %s", target->GetName());
			parse->Event(EVENT_COMBAT, this->GetNPCTypeID(), "1", this, target);
			combat_event = true;
		}
		combat_event_timer.Start(CombatEventTimer_expire);
	}
	
	classattack_timer.Start(reuse);
}

void Mob::Taunt(NPC* who, bool always_succeed) {
	if (who == NULL)
		return;
	
	if (!always_succeed && IsClient())
		CastToClient()->CheckIncreaseSkill(TAUNT);
	
	int level = GetLevel();
	
	Mob *hate_top = who->GetHateTop();
	
	// Check to see if we're already at the top of the target's hate list
	// a mob will not be taunted if its target's health is below 20%
	if ((hate_top != this) 
	&& (who->GetLevel() < level) 
	&& (hate_top == NULL || hate_top->GetHPRatio() >= 20) ) {
		sint32 newhate, tauntvalue;

		float tauntchance;
		if(always_succeed) {
			tauntchance = 101;
		} else {
			
			// no idea how taunt success is actually calculated
			// TODO: chance for level 50+ mobs should be lower
			int level_difference = level - target->GetLevel();
			if (level_difference <= 5) {
				tauntchance = 25.0;	// minimum
				tauntchance += tauntchance * (float)GetSkill(TAUNT) / 200.0;	// skill modifier
				if (tauntchance > 65.0)
					tauntchance = 65.0;
			}
			else if (level_difference <= 10) {
				tauntchance = 30.0;	// minimum
				tauntchance += tauntchance * (float)GetSkill(TAUNT) / 200.0;	// skill modifier
				if (tauntchance > 85.0)
					tauntchance = 85.0;
			}
			else if (level_difference <= 15) {
				tauntchance = 40.0;	// minimum
				tauntchance += tauntchance * (float)GetSkill(TAUNT) / 200.0;	// skill modifier
				if (tauntchance > 90.0)
					tauntchance = 90.0;
			}
			else {
				tauntchance = 50.0;	// minimum
				tauntchance += tauntchance * (float)GetSkill(TAUNT) / 200.0;	// skill modifier
				if (tauntchance > 95.0)
					tauntchance = 95.0;
			}
		}
		if (tauntchance > MakeRandomFloat(0, 100)) {
			// this is the max additional hate added per succesfull taunt
			tauntvalue = (int)MakeRandomFloat(1, level * 10.0);
			//tauntvalue = (sint32) ((float)level * 10.0 * (float)rand()/(float)RAND_MAX + 1);
			// new hate: find diff of player's hate and whoever's at top of list, add that plus tauntvalue to players hate
			newhate = who->GetNPCHate(hate_top) - who->GetNPCHate(this) + tauntvalue;
			// add the hate
			who->CastToNPC()->AddToHateList(this, newhate);
		}
	}
	
	//generate at least one hate reguardless of the outcome.
	who->CastToNPC()->AddToHateList(this, 1);
}

void Mob::InstillDoubt(Mob *who) {
	//make sure we can use this skill
	int skill = GetSkill(INTIMIDATION);
	if(skill < 1 || skill > 252)
		return;
	
	//make sure our target is an NPC
	if(!who || !who->IsNPC())
		return;
	
	//range check
	if(!CombatRange(who))
		return;
	
	if(IsClient()) {
		CastToClient()->CheckIncreaseSkill(INTIMIDATION);
	}

	//I think this formula needs work
	int value = 0;
	
	//user's bonus
	value += GetSkill(INTIMIDATION) + GetCHA()/4;
	
	//target's counters
	value -= target->GetLevel()*4 + who->GetWIS()/4;
	
	if (MakeRandomInt(0,99) < value) {
		//temporary hack...
		//cast fear on them... should prolly be a different spell
		//and should be un-resistable.
		SpellOnTarget(229, who);
		//is there a success message?
	} else {
		Message_StringID(4,NOT_SCARING);
		//Idea from WR:
		/* if (target->IsNPC() && MakeRandomInt(0,99) < 10 ) {
			entity_list.MessageClose(target, false, 50, MT_Rampage, "%s lashes out in anger!",target->GetName());
			//should we actually do this? and the range is completely made up, unconfirmed
			entity_list.AEAttack(target, 50);
		}*/
	}
}



