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

void Client::OPCombatAbility(const APPLAYER *app) {
	if(!target)
		return;
	if(!IsAttackAllowed(target))
		return;

	CombatAbility_Struct* ca_atk = (CombatAbility_Struct*) app->pBuffer;
	if ((ca_atk->m_atk == 100) 
	  && (ca_atk->m_skill == BASH)) {    // SLAM - Bash without a shield equipped
		DoAnim(animTailRake);
		float chance = (level+GetSkill(BASH)+GetSTR())/5;
		sint32 dmg = 0;
		if(chance<20)
			chance=20;
		else if(chance>90)
			chance=90;

		//this formula is just a hack, its not perfect by any means
		if((rand()%100)<chance)//success figure damage
			dmg = ((((level/10)*(rand()%7))+GetSkill(BASH)*5+GetSTR())/100)*(rand()%10);
		target->Damage(this, dmg, 0xffff, BASH);
		CheckIncreaseSkill(BASH);
		
		/* using CheckIncreaseSkill now
		if (GetClass()==WARRIOR&&(GetRace()==BARBARIAN||GetRace()==TROLL||GetRace()==OGRE)) { // large race warriors only *
			float wisebonus =  (m_pp.WIS > 200) ? 20 + ((m_pp.WIS - 200) * 0.05) : m_pp.WIS * 0.1;
			if (((55-(GetSkill(BASH)*0.240))+wisebonus > MakeRandomFloat(0, 100))&& (GetSkill(BASH)<(m_pp.level+1)*5))
					this->SetSkill(BASH,GetRawSkill(BASH)+1);
		}*/
		return;
	}
	
	//throwing weapons
	if ((ca_atk->m_atk == 11)&&(ca_atk->m_skill == THROWING)) {
		ThrowingAttack(target);
		return;
	}
	
	//ranged attack (archery)
	if ((ca_atk->m_atk == 11)&&(ca_atk->m_skill == ARCHERY)) {
		RangedAttack(target);
		return;
	}
	
	float multiple=(GetLevel()/5);
	multiple++;
	switch(GetClass())
	{
	case WARRIOR:
		if (target!=this) {
			float dmg=((((GetSkill(KICK) + GetSTR() + GetLevel())/90)*multiple)+10) * ( MakeRandomFloat(0, 1) );
			if(target->IsClient())
				dmg*=.76;
			else{
				CheckIncreaseSkill(KICK);
				dmg*=1.2f;//small increase for warriors
			}
			target->Damage(this, (int32)dmg, 0xffff, 0x1e);
			DoAnim(animKick);
		}
		break;
	case RANGER:
	case BEASTLORD:
		if (target!=this) {
			float dmg=((((GetSkill(KICK) + GetSTR() + GetLevel())/250)*multiple)+5) * ( MakeRandomFloat(0, 1) );
			if(target->IsClient())
				dmg*=.67f;
			else
				CheckIncreaseSkill(KICK);
			target->Damage(this, (int32)dmg, 0xffff, 0x1e);
			DoAnim(animKick);
		}
		break;
	case PALADIN:
	case SHADOWKNIGHT:
		break;
	case MONK:
		CheckIncreaseSkill(ca_atk->m_skill);
		MonkSpecialAttack(target, ca_atk->m_skill);
		break;
	case ROGUE:
		if (ca_atk->m_atk != 100 || ca_atk->m_skill != BACKSTAB) {
			break;
		}
		const ItemInst *weapon = m_inv.GetItem(SLOT_PRIMARY);
		TryBackstab(target, weapon?weapon->GetItem():NULL);
		break;
	}
}



//returns the reuse time for the special attack used.
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
	hitsuccess += (float)rand()/RAND_MAX;
#if EQDEBUG >= 11
    LogFile->write(EQEMuLog::Debug,"MonkSpecialAttack() 4 - %d", hitsuccess);
#endif
	float ackwardtest = 2.4f;
	float random = (float)rand()/RAND_MAX;
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
	
	ackwardtest += (float)rand()/RAND_MAX;
	ackwardtest = abs((long)ackwardtest);
	if (type == FLYING_KICK) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (10 * ackwardtest) * (GetSkill(FLYING_KICK) + GetSTR() + level) / 600);
		if(other->IsClient())
			ndamage = ndamage * 3 / 4;
		if ((float)rand()/RAND_MAX < 0.2) {
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
			other->Damage(this, ndamage, 0xffff, 0x1A);
		DoAnim(animFlyingKick);
		reuse = FlyingKickReuseTime * 1000;
	}
	else if (type == TIGER_CLAW) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (4 * ackwardtest) * (GetSkill(TIGER_CLAW) + GetSTR() + level) / 700);
		if(other->IsClient())
			ndamage = ndamage * 7 / 10;
		if(candamage)
			other->Damage(this, ndamage, 0xffff, 0x34);
		DoAnim(animTigerClaw);
		reuse = TigerClawReuseTime * 1000;
	}
	else if (type == ROUND_KICK) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (6 * ackwardtest) * (GetSkill(ROUND_KICK) + GetSTR() + level) / 600);
		if(other->IsClient())
			ndamage = ndamage * 9 / 10;
		if(candamage)
			other->Damage(this, ndamage, 0xffff, 0x26);
		DoAnim(animRoundKick);
		reuse = RoundKickReuseTime * 1000;
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
			other->Damage(this, ndamage, 0xffff, 0x17);
		DoAnim(animEagleStrike);
		reuse = EagleStrikeReuseTime * 1000;
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
			other->Damage(this, ndamage, 0xffff, 0x15);
		DoAnim(animTailRake);
		reuse = TailRakeReuseTime * 1000;
	}
	else if (type == KICK) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (6 * ackwardtest) * (GetSkill(KICK) + GetSTR() + level) / 1000);
		if(other->IsClient())
			ndamage = ndamage * 7 / 10;
		if(candamage)
			other->Damage(this, ndamage, 0xffff, 0x1e);
		DoAnim(animKick);
		reuse = KickReuseTime * 1000;
	}
	return(reuse);
}

void Mob::TryBackstab(Mob *other, const Item_Struct* weapon) {
	if(!other)
		return;
	
	//make sure we have a proper weapon if we are a client.
	if(IsClient()) {
		if(	weapon == NULL 	//no weapon
			|| weapon->ItemClass != ItemTypeCommon	//not possibly piercing
			|| (weapon->Common.ItemUse != ItemUsePierce	//not a piercer
				&& weapon->Common.ItemUse != ItemUse2HPierce) ) {
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
	if (weapon && weapon->ItemClass == ItemTypeCommon)
		primaryweapondamage = weapon->Common.Damage; //backstab uses primary weapon
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
	other->Damage(this, ndamage, 0xffff, BACKSTAB);
	DoAnim(animPiercing);	//piercing animation
}

// solar - assassinate
void Mob::RogueAssassinate(Mob* other)
{
	other->Damage(this, 32000, 0xffff, BACKSTAB);
	DoAnim(animPiercing);	//piercing animation
}

void Client::RangedAttack(Mob* other) {
	DoAnim(animShootBow);
	
	const ItemInst* RangeWeapon = m_inv[SLOT_RANGE];
	
	//locate ammo
	int ammo_slot = SLOT_AMMO;
	const ItemInst* Ammo = m_inv[SLOT_AMMO];
	
	if (!RangeWeapon || !RangeWeapon->IsType(ItemTypeCommon)) {
		Message(0, "Error: Rangeweapon: GetItem(%i)==0, you have no bow!", GetItemIDAt(SLOT_RANGE));
		return;
	}
	if (!Ammo || !Ammo->IsType(ItemTypeCommon)) {
		Message(0, "Error: Ammo: GetItem(%i)==0, you have no ammo!", GetItemIDAt(SLOT_AMMO));
		return;
	}
	
	const Item_Struct* RangeItem = RangeWeapon->GetItem();
	const Item_Struct* AmmoItem = Ammo->GetItem();
	
	if(RangeItem->Common.ItemUse != ItemUseBow) {
		Message(0, "Error: Rangeweapon: GetItem(%i)==0, you have no bow!", GetItemIDAt(SLOT_RANGE));
		return;
	}
	if(AmmoItem->Common.ItemUse != ItemUseArrow) {
		Message(0, "Error: Ammo: GetItem(%i)==0, you have no ammo!", GetItemIDAt(ammo_slot));
		return;
	}
	
	//look for ammo in inventory if we only have 1 left...
	if(Ammo->GetCharges() == 1) {
		sint32 aslot = m_inv.HasItem(AmmoItem->ItemNumber, 1, invWherePersonal);
		if(aslot != SLOT_INVALID) {
			ammo_slot = aslot;
		}
	}
	
	int range = RangeItem->Common.Range + AmmoItem->Common.Range +5/*Fudge it a little, client will let you hit something at 0 0 0 when you are at 205 0 0*/;
	range *= range;
	if(DistNoRootNoZ(*target) > range)
	{
		//target is out of range, client does a message
		return;
	}
	
	//restart their attack timers since this is an attack.
	if (attack_timer.Enabled())
		attack_timer.Start();
	if (attack_dw_timer.Enabled())
		attack_dw_timer.Start();

	float chancetohit = 0;
	if(!target)
		return;
	if(target->IsNPC())
		chancetohit = GetSkill(ARCHERY) / 3.75;
	else
		chancetohit = GetSkill(ARCHERY) / 4.75; //harder to hit players

	if (m_pp.level-target->GetLevel() < 0) {
		chancetohit -= (float)((target->GetLevel()-m_pp.level)*(target->GetLevel()-m_pp.level))/4;
	}
	
	int16 targetagi = target->GetAGI();
	int16 playerDex = (int16)(this->itembonuses.DEX + this->spellbonuses.DEX)/2;
	
	targetagi = (targetagi <= 200) ? targetagi:targetagi + ((targetagi-200)/5);
	chancetohit -= (float)targetagi*0.05;
	chancetohit += playerDex;
	chancetohit = (chancetohit > 0) ? chancetohit+30:30;
	chancetohit = chancetohit > 95 ? 95 : chancetohit; // cap to 95%
	
	bool trueshot = CheckDiscipline(discTrueshot);
	if(trueshot)
		chancetohit += 30;		//+15%
	
	// Hit?
	if (MakeRandomFloat(0, 200) > chancetohit) {
		//this->Message(MT_Emote, "You missed your target");
		//this->Message_StringID(M,GENERIC_MISS,"You","your target.");
		target->Damage(this, 0, 0xffff, 0x07);
	}
	else {
		const Item_Struct* RangeItem = RangeWeapon->GetItem();
		const Item_Struct* AmmoItem = Ammo->GetItem();
		uint16 WDmg = RangeItem->Common.Damage;
		uint16 ADmg = AmmoItem->Common.Damage;
		
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
		
		sint32 TotalDmg = 0;
		sint32 critDmg = 0;
		
		if(GetClass()==RANGER) {
			critDmg = (sint32)(MaxDmg * 1.2);
		}
		
		if(trueshot)
			MaxDmg *= 2;	//up to 2X the damage
		
		if (MaxDmg == 0)
			MaxDmg = 1;
		TotalDmg = 1 + MakeRandomInt(0, MaxDmg);
		if(target->IsClient()) { //Tone down pvp damage
			if(critDmg>0)
				critDmg-=critDmg/4;
			TotalDmg-=TotalDmg/4;
		}
		// no crits before level 12 cap is maxed
		if((GetClass()==RANGER)&&(GetSkill(ARCHERY)>65)&&((uint16)MakeRandomInt(0, 355) < (GetSkill(ARCHERY)+playerDex)/2)&&(chancetohit > 85)) {
			if(target->IsNPC() && !target->IsMoving() && !target->IsRooted() && this->GetLevel()>50){
				if(this->GetGM())
					Message(0,"(GM ONLY) Doubling attack damage, npc isnt moving!");
				critDmg*=2;
			}
			char val1[20]={0};
			entity_list.MessageClose_StringID(this, false, 200, MT_CritMelee, CRITICAL_HIT, GetName(), ConvertArray(critDmg,val1));
			//this->Message_StringID(MT_CritMelee,CRITICAL_HIT,GetName(),ConvertArray(critDmg,val1));
			//this->Message(MT_CritMelee, "You score a critical hit!(%d)", critDmg);
			target->Damage(this, critDmg, 0xffff, 0x07);
		}
		else {
			if(GetClass()==RANGER && !target->IsMoving() && !target->IsRooted() && this->GetLevel()>50){
				if(this->GetGM())
					Message(0,"(GM ONLY) Doubling attack damage, npc isnt moving!");
				TotalDmg*=2;
			}
			char hitname[64]={0};
			strncpy(hitname,target->GetName(),strlen(target->GetName())-2);
			//char val1[20]={0};
			//Message_StringID(MT_Emote,HIT_NON_MELEE,"You",hitname,ConvertArray(TotalDmg,val1));
			//this->Message(MT_Emote, "You Hit for a total of %d non-melee damage.", TotalDmg);
			target->Damage(this, TotalDmg, 0xffff, 0x07);
		}
		
		//TODO: check bane and elemental damages
		
		if(target && (target->GetHP() > -10))
			TryWeaponProc(RangeItem, target);
	}

	// See if the player increases their skill - with cap
	/*float wisebonus =  (GetWIS() > 200) ? 20 + ((GetWIS() - 200) * 0.05) : GetWIS() * 0.1;
	
	if (((55-(GetSkill(ARCHERY)*0.240))+wisebonus > MakeRandomFloat(0, 100)) && (GetSkill(ARCHERY)<(m_pp.level+1)*5) && GetSkill(ARCHERY) < 252)
		this->SetSkill(ARCHERY,GetRawSkill(ARCHERY)+1);*/
	CheckIncreaseSkill(ARCHERY);
	
	
	//apparently we are supposed to trust the client to send
	//us a delete request for the item instead...
//	if(!GetAA(aaEndlessQuiver))
//		DeleteItemInInventory(ammo_slot, 1, false);	//do we need the update, or is the client smart?
	return;
}

void Client::ThrowingAttack(Mob* other) { //old was 51
	const ItemInst* RangeWeapon = m_inv[SLOT_RANGE];
	if(!RangeWeapon)
		RangeWeapon = m_inv[SLOT_AMMO];
	
	if (!RangeWeapon || !RangeWeapon->IsType(ItemTypeCommon)) {
		Message(0, "Error: Rangeweapon: GetItem(%i)==0, you have nothing to throw!", GetItemIDAt(SLOT_RANGE));
		return;
	}
	
	const Item_Struct* item = RangeWeapon->GetItem();
	if(item->Common.ItemUse != ItemUseThrowing && item->Common.ItemUse != ItemUseThrowingv2) {
		Message(0, "Error: Rangeweapon: GetItem(%i)==0, you have nothing useful to throw!", GetItemIDAt(SLOT_RANGE));
		return;
	}
	 
	uint8 WDmg = item->Common.Damage;
	// Throw stuff
	DoAnim(anim1HWeapon);		//same number as 1HS/1HB, this is prolly wrong..
	sint32 TotalDmg = 0;
	
	// borrowed this from attack.cpp
	// chance to hit
	
	float chancetohit;
	if(target->IsNPC())
		chancetohit = GetSkill(THROWING) / 3.75;
	else
		chancetohit = GetSkill(THROWING) / 4.75; //harder to hit players
	
	if (GetLevel()-target->GetLevel() < 0) {
		chancetohit -= (float)((target->GetLevel()-GetLevel())*(target->GetLevel()-GetLevel()))/4;
	}
	
	int16 targetagi = target->GetAGI();
	int16 playerDex = (int16)GetDEX()/2;
	
	targetagi = (targetagi <= 200) ? targetagi:targetagi + ((targetagi-200)/5);
	chancetohit -= (float)targetagi*0.05;
	chancetohit += playerDex;
	chancetohit = (chancetohit > 0) ? chancetohit+30:30;
	chancetohit = chancetohit > 95 ? 95 : chancetohit; // cap to 95%
	
	uint8 levelBonus = (GetSTR()+GetLevel()+GetSkill(THROWING)) / 100;
	uint8 MaxDmg = (WDmg)*levelBonus;
	if (MaxDmg == 0)
		MaxDmg = 1;
	TotalDmg = 1 + MakeRandomInt(0, MaxDmg);
	
	// Hit?
	if (MakeRandomFloat(0, 100) > chancetohit) {
			target->Damage(this, 0, 0xffff, THROWING);
	}
	else {
		//this->Message(MT_Emote, "You Hit for a total of %d damage.", TotalDmg);
		target->Damage(this, TotalDmg, 0xffff, THROWING);
	}
	
	if(target && (target->GetHP() > -10))
		TryWeaponProc(item, target);
	
	// See if the player increases their skill - with cap
	/*float wisebonus =  (GetWIS() > 200) ? 20 + ((GetWIS() - 200) * 0.05) : GetWIS() * 0.1;
	
	if (((55-(GetSkill(THROWING)/4))+wisebonus > MakeRandomInt(0, 100)) && GetSkill(THROWING) < (uint16)((GetLevel()*5)+5))
		SetSkill(THROWING,GetRawSkill(THROWING)+1);*/
	CheckIncreaseSkill(THROWING);
	
	//apparently we are supposed to trust the client to send
	//us a delete request for the item instead...
//	DeleteItemInInventory(SLOT_RANGE, 1, false);	//do we need the update, or is the client smart?
	return;
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
				const Item_Struct* weapon = NULL;
				if(equipment[MATERIAL_PRIMARY] != 0)
					weapon = database.GetItem(equipment[MATERIAL_PRIMARY]);
				TryBackstab(target, weapon);
				reuse = BackstabReuseTime * 1000;
			}
			break;
		case MONK: {
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
			break;
		}
		case WARRIOR:
			//kick
			reuse = MonkSpecialAttack(target, KICK);
			break;
		case RANGER:
		case BEASTLORD:
			if(level > 5) {
				//kick
				reuse = MonkSpecialAttack(target, KICK);
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
			} else {
				reuse = 1000 * 5;	//check again in 5 seconds
			}
			break;
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
}



