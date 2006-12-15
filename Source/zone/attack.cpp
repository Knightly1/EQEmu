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

#if EQDEBUG >= 5
//#define ATTACK_DEBUG 20
#endif

#include "../common/debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
using namespace std;
#include <assert.h>

#include "masterentity.h"
#include "NpcAI.h"
#include "../common/packet_dump.h"
#include "../common/eq_packet_structs.h"
#include "../common/skills.h"
#include "spdat.h"
#include "zone.h"
#include "StringIDs.h"
#include "../common/MiscFunctions.h"
#include "../common/rulesys.h"

#ifdef WIN32
#define snprintf	_snprintf
#define strncasecmp	_strnicmp
#define strcasecmp  _stricmp
#endif


extern EntityList entity_list;
#ifndef NEW_LoadSPDat
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif

#ifdef RAIDADDICTS
#include "RaidAddicts.h"
extern RaidAddicts raidaddicts;
#endif


extern Zone* zone;

bool Mob::AttackAnimation(SkillType &skillinuse, int Hand, const ItemInst* weapon)
{
	// Determine animation
	int type = 0;
	if (weapon && weapon->IsType(ItemClassCommon)) {
		const Item_Struct* item = weapon->GetItem();
#if EQDEBUG >= 11
			LogFile->write(EQEMuLog::Debug, "Weapon skill:%i", item->ItemType);
#endif		
		switch (item->ItemType)
		{
		case ItemType1HS: // 1H Slashing
		{
			skillinuse = _1H_SLASHING;
			type = anim1HWeapon;
			break;
		}
		case ItemType2HS: // 2H Slashing
		{
			skillinuse = _2H_SLASHING;
			type = anim2HSlashing;
			break;
		}
		case ItemTypePierce: // Piercing
		{
			skillinuse = PIERCING;
			type = animPiercing;
			break;
		}
		case ItemType1HB: // 1H Blunt
		{
			skillinuse = _1H_BLUNT;
			type = anim1HWeapon;
			break;
		}
		case ItemType2HB: // 2H Blunt
		{
			skillinuse = _2H_BLUNT;
			type = anim2HWeapon;
			break;
		}
		case ItemType2HPierce: // 2H Piercing
		{
			skillinuse = PIERCING;
			type = anim2HWeapon;
			break;
		}
		case ItemTypeHand2Hand:
		{
			skillinuse = HAND_TO_HAND;
			type = animHand2Hand;
			break;
		}
		default:
		{
			skillinuse = HAND_TO_HAND;
			type = animHand2Hand;
			break;
		}
		}// switch
	}
	else
	{
		skillinuse = HAND_TO_HAND;
		type = animHand2Hand;
	}
	
	// Kaiyodo - If we're attacking with the seconary hand, play the duel wield anim
	if (Hand == 14)	// DW anim
		type = animDualWeild;
	
	DoAnim(type);
    return true;
}

// solar: called when a mob is attacked, does the checks to see if it's a hit
// and does other mitigation checks.  'this' is the mob being attacked.
bool Mob::CheckHitChance(Mob* other, SkillType skillinuse, int Hand)
{
/*
	Father Nitwit:
		Reworked a lot of this code to achieve better balance at higher levels.
		The old code basically meant that any in high level (50+) combat,
		both parties always had 95% chance to hit the other one.
*/
	Mob *attacker=other;
	Mob *defender=this;
	float chancetohit = 0;

#if ATTACK_DEBUG>=11
		LogFile->write(EQEMuLog::Debug, "CheckHitChance(%s) attacked by %s", defender->GetName(), attacker->GetName());
#endif
	
	bool pvpmode = false;
	if(IsClient() && other->IsClient())
		pvpmode = true;
	
	float bonus;
	
	////////////////////////////////////////////////////////
	// To hit calcs go here
	////////////////////////////////////////////////////////

	int8 attacker_level = attacker->GetLevel() ? attacker->GetLevel() : 1;
	int8 defender_level = defender->GetLevel() ? defender->GetLevel() : 1;

	//
	// we start by giving them a base chance to hit
	//
	chancetohit = 50;

	sint32 level_difference = attacker_level - defender_level;
	if(level_difference < -20) level_difference = -20;
	if(level_difference > 20) level_difference = 20;
	// if attacker is higher level, they have a better chance of hitting their
	// target right from the start.  for each level difference it's 2% chance,
	// up to 20. so, if you're level 21 attacking a level 10 mob, that's a 22% 
	// chance bonus. the level difference shouldn't be that big of a bonus, 
	// otherwise when exping off an even con monster, after you level up it would
	// become significantly easier.  this effect should be subtle, and should
	// increase as you have more and more level on the monster, but it needs to
	// be capped too
	mlog(COMBAT__TOHIT, "Base chance to hit %.2f. Adding level diffrence mod %d*2", chancetohit, level_difference);
	chancetohit += (float) level_difference * 2;
		
	//Quagmire: Take into account offense/defense skill
	// solar: with this formula if you have an attacker with 185 offense skill
	// and a defender with 180 defense, the net effect is
	// 24.05 - 16.2 == 7.85% added
	// an example with the defender having a defense of 85, and the attacker
	// having an offense of 60
	// 7.8 - 7.65 = 0.15% added
	// so basically defense is for cancelling out the attacker's offense,
	// but offense is more effective than defense
#if ATTACK_DEBUG>=15
		LogFile->write(EQEMuLog::Debug, "%s::AvoidDamage(%s) pre off/def %f", GetName(), other->GetName(), chancetohit);
#endif
	if(pvpmode) {
		bonus = 0;
		bonus += (float)((float)attacker->GetSkill(OFFENSE) * 0.20f);
		bonus -= (float)((float)defender->GetSkill(DEFENSE) * 0.05f);
		chancetohit += bonus;
		mlog(COMBAT__TOHIT, "Applied PVP Offense (%d) and Defense (%d) bonus of %.2f, yeilding %.2f", attacker->GetSkill(OFFENSE), defender->GetSkill(DEFENSE), bonus, chancetohit);
	} else {
		//this was a bit unbalanced, since the primary determinate is
		//level difference, and thats allready accounted for
		//so we leave its effects in, shrink them a little and cap it
		//at 10%
		bonus = 0;
		bonus += attacker->GetSkill(OFFENSE);
		bonus -= defender->GetSkill(DEFENSE);
		bonus /= 10;
		if(bonus > 10)
			bonus = 10;
		chancetohit += bonus;
		mlog(COMBAT__TOHIT, "Applied Offense (%d) and Defense (%d) bonus of %.2f, yeilding %.2f", attacker->GetSkill(OFFENSE), defender->GetSkill(DEFENSE), bonus, chancetohit);
	}
	
	sint16 defender_agi = defender->GetAGI();
	// skill points over 200 are 1/5 as effective
	// at max stat of 252 this is a 10.52% bonus
	defender_agi = (defender_agi <= 200) ? defender_agi : (defender_agi + ((defender_agi-200)/10));
	chancetohit -= (float)((float)defender_agi * 0.05f);
	
	// this comes out to about 1% for every 7 dex over 50
	// so someone with 105 dex gets 8.25% added here
	sint16 attacker_dex = attacker->GetDEX();
	attacker_dex = (attacker_dex <= 200) ? attacker_dex : (attacker_dex + ((attacker_dex-200)/25));
	attacker_dex -= 50;
	chancetohit += (float) ((float)attacker_dex * 0.15f);

	mlog(COMBAT__TOHIT, "Applied Defending AGI (%d) and Attacking (DEX-50=%d) yeilding %.2f", defender_agi, attacker_dex, chancetohit);
	
	//divided these bonuses by 4... higher level battles were basically always 95%
	//hit chance because of this 50% bonus...
	
	//Got rid of this because it is really a function of level, and we 
	//allready account for level difference. Also, this is a pure bonus, 
	//so at higher levels, it is as much as 50%, basically ensuring a 95%
	//chance to hit, which seems dumb.
/*#ifdef IPC
	if (attacker->IsClient() || attacker->CastToNPC()->IsInteractive())
#else
	if (attacker->IsClient())
#endif
	{
		Client *client_attack = attacker->CastToClient();
		//
		// if this is a client we want to take their weapon skill and give them
		// a bonus to their chance to hit.  at 200 skill you will get 10% added
		// to your chance to hit
		//
		bonus = ((float)client_attack->GetSkill(skillinuse) / 20.0f);
		
	}
	else
	{
//		chancetohit += 20; // assume NPCs have 120 skill with their weapon
		//NPCs should have skills proportional to their level
		//skill = level*5 + 5, chance to hit = skill / 5
		//so chancetohit gets level + 1 bonus...
		chancetohit += (attacker->GetLevel() + 1) / 4;
	}
	
#if ATTACK_DEBUG>=15
		LogFile->write(EQEMuLog::Debug, "%s::AvoidDamage(%s) after weapon skills %f", GetName(), other->GetName(), chancetohit);
#endif*/
	
	//I dont think this is 100% correct, but at least it does something...
	if(attacker->spellbonuses.MeleeSkillCheckSkill == skillinuse || attacker->spellbonuses.MeleeSkillCheckSkill == 255) {
		chancetohit += attacker->spellbonuses.MeleeSkillCheck;
		mlog(COMBAT__TOHIT, "Applied spell melee skill bonus %d, yeilding %.2f", attacker->spellbonuses.MeleeSkillCheck, chancetohit);
	}
	if(attacker->itembonuses.MeleeSkillCheckSkill == skillinuse || attacker->itembonuses.MeleeSkillCheckSkill == 255) {
		chancetohit += attacker->itembonuses.MeleeSkillCheck;
		mlog(COMBAT__TOHIT, "Applied item melee skill bonus %d, yeilding %.2f", attacker->spellbonuses.MeleeSkillCheck, chancetohit);
	}
	
	
	//add in our hit chance bonuses if we are using the right skill
	//does the hit chance cap apply to spell bonuses from disciplines?
	if(attacker->spellbonuses.HitChanceSkill == 255 || attacker->spellbonuses.HitChanceSkill == skillinuse) {
		chancetohit += attacker->spellbonuses.HitChance / 15.0f;
		mlog(COMBAT__TOHIT, "Applied spell melee hit chance %d/15, yeilding %.2f", attacker->spellbonuses.HitChance, chancetohit);
	}
	if(attacker->itembonuses.HitChanceSkill == 255 || attacker->itembonuses.HitChanceSkill == skillinuse) {
		chancetohit += attacker->itembonuses.HitChance / 15.0f;
		mlog(COMBAT__TOHIT, "Applied item melee hit chance %d/15, yeilding %.2f", attacker->itembonuses.HitChance, chancetohit);
	}
	
	//subtract off avoidance by the defender
	bonus = defender->spellbonuses.AvoidMeleeChance + defender->itembonuses.AvoidMeleeChance;
	if(bonus > 0) {
		chancetohit -= (bonus) / 10;
		mlog(COMBAT__TOHIT, "Applied avoidance chance %.2f/10, yeilding %.2f", bonus, chancetohit);
	}
	
	// Chance to hit;   Max 95%, Min 30%
	if(chancetohit > 1000) {
		//if chance to hit is crazy high, that means a discipline is in use, and let it stay there
	} else if(chancetohit > 95) {
		chancetohit = 95;
	} else if(chancetohit < 30) {
		chancetohit = 30;
	}
	
	//I dont know the best way to handle a garunteed hit discipline being used
	//agains a garunteed riposte (for example) discipline... for now, garunteed hit wins
	
		
	#if EQDEBUG>=11
		LogFile->write(EQEMuLog::Debug, "3 FINAL calculated chance to hit is: %5.2f", chancetohit);
	#endif

	//
	// Did we hit?
	//

	float tohit_roll = MakeRandomFloat(0, 100);
	
	mlog(COMBAT__TOHIT, "Final hit chance: %.2f%%. Hit roll %.2f", chancetohit, tohit_roll);
	
	return(tohit_roll <= chancetohit);
}

/* solar: called when a mob is attacked, does the checks to see if it's a hit
 *  and does other mitigation checks.  'this' is the mob being attacked.
 * 
 * special return values:
 *    -1 - block
 *    -2 - parry
 *    -3 - riposte
 *    -4 - dodge
 * 
 */
bool Mob::AvoidDamage(Mob* other, sint32 &damage)
{
	float skill;
	float bonus;
	float roll;
	Mob *attacker=other;
	Mob *defender=this;
	bool sthrough = false;
	if(MakeRandomInt(0, 100) <= (attacker->itembonuses.StrikeThrough + attacker->spellbonuses.StrikeThrough))
		sthrough = true;
/*
	////////////////////////////////////////////////////////
	// Mitigation goes here
	////////////////////////////////////////////////////////
	if (damage > 1) {
	    if(this->IsClient()){
	    	if (damage > 1 && spell_id == 0xFFFF){
                float acMod = GetAC()/100;
                float acDam = (damage/100)*(acMod*1.5);
                damage = (sint32)((float)damage-acDam);
                if(damage <= 0)
					damage = 1;
			}
	    }
	    else {
	    	if (damage > 1 && spell_id == 0xFFFF){
				float acMod = GetAC()/100;
                float acDam = (damage/100)*acMod;
                damage = (sint32)((float)damage-acDam);
                if(damage <= 0)
					damage = 1;
			}
	    }
	}
*/	
	
	//garunteed hit
	bool ghit = false;
	if((attacker->spellbonuses.MeleeSkillCheck + attacker->itembonuses.MeleeSkillCheck) > 500)
		ghit = true;
	
	if(damage > 0) {
	//handle discipline mitigation and shielding
		sint16 mitigation = defender->spellbonuses.MeleeMitigation + defender->itembonuses.MeleeMitigation;
		if(mitigation > 0) {
			damage = int(damage * (100.0f - mitigation) / 100.0f);
			mlog(COMBAT__DAMAGE, "Applied %.3f mitigation, remaining damage %d", mitigation, damage);
		}
		
		//handle damage increase diciplines + items
		//this is completely wrong, and needs to account for both attacker and defenders modifieres
		/*int mod = attacker->spellbonuses.DamageModifier + attacker->itembonuses.DamageModifier;
		if(mod < -99)
			damage = 0;	//all absorbed, should this be legal?
		else
			damage = damage * (100 + mod) / 100;
		*/
	}
	
	
	//////////////////////////////////////////////////////////
	// make enrage same as riposte
	/////////////////////////////////////////////////////////
	if (IsEnraged() && !other->BehindMob(this, other->GetX(), other->GetY()) && !sthrough) {
		damage = -3;
		mlog(COMBAT__DAMAGE, "I am enraged, riposting frontal attack.");
	}
	
	/////////////////////////////////////////////////////////
	// riposte
	/////////////////////////////////////////////////////////
	if (damage > 0 && CanThisClassRiposte() && !other->BehindMob(this, other->GetX(), other->GetY()) && !sthrough)
	{
        skill = GetSkill(RIPOSTE);
		if (IsClient()) {
        	if (!other->IsClient() && GetLevelCon(other->GetLevel()) != CON_GREEN)
				this->CastToClient()->CheckIncreaseSkill(RIPOSTE);
		}
		
		if (!ghit) {	//if they are not using a garunteed hit discipline
			bonus = (defender->spellbonuses.RiposteChance + defender->itembonuses.RiposteChance);
			bonus += 2.0 + skill/35.0;
			roll = MakeRandomFloat(0,100);
			mlog(COMBAT__DAMAGE, "Check riposte. skill %.0f. %.2f percent chance, roll %.2f", skill, bonus, roll);
			if(roll < bonus)
				damage = -3;
		}
	}
	
	///////////////////////////////////////////////////////	
	// block
	///////////////////////////////////////////////////////
	if (damage > 0 && (
			class_==MONK ||
			class_==BEASTLORD ||
			class_==MONKGM ||
			class_==BEASTLORDGM )
            && !other->BehindMob(this, other->GetX(), other->GetY())
			&& !sthrough)
	{
		skill = CastToClient()->GetSkill(BLOCKSKILL);
		if (IsClient()) {
			if (!other->IsClient() && GetLevelCon(other->GetLevel()) != CON_GREEN)
				this->CastToClient()->CheckIncreaseSkill(BLOCKSKILL);
		}
		
		if (!ghit) {	//if they are not using a garunteed hit discipline
			bonus = 2.0 + skill/35.0;
			roll = MakeRandomFloat(0,100);
			mlog(COMBAT__DAMAGE, "Check block. skill %.0f. %.2fpercent chance, roll %.2f", skill, bonus, roll);
			if(roll < bonus)
				damage = -1;
		}
	}
	
	//////////////////////////////////////////////////////		
	// parry
	//////////////////////////////////////////////////////
	if (damage > 0 && CanThisClassParry() && !other->BehindMob(this, other->GetX(), other->GetY()) && !sthrough)
	{
        skill = CastToClient()->GetSkill(PARRY);
		if (IsClient()) {
			if (!other->IsClient() && GetLevelCon(other->GetLevel()) != CON_GREEN)
				this->CastToClient()->CheckIncreaseSkill(PARRY); 
		}
		
		bonus = (defender->spellbonuses.ParryChance + defender->itembonuses.ParryChance) / 100.0f;
		if (!ghit) {	//if they are not using a garunteed hit discipline
			bonus += 2.0 + skill/35.0;
			roll = MakeRandomFloat(0,100);
			mlog(COMBAT__DAMAGE, "Check parry. skill %.0f. %.2fpercent chance, roll %.2f", skill, bonus, roll);
			if(roll < bonus)
				damage = -2;
		}
	}
	
	////////////////////////////////////////////////////////
	// dodge
	////////////////////////////////////////////////////////
	if (damage > 0 && CanThisClassDodge() && !other->BehindMob(this, other->GetX(), other->GetY()) && !sthrough)
	{
	
        skill = CastToClient()->GetSkill(DODGE);
		if (IsClient()) {
			if (!other->IsClient() && GetLevelCon(other->GetLevel()) != CON_GREEN)
				this->CastToClient()->CheckIncreaseSkill(DODGE);
		}
		
		bonus = (defender->spellbonuses.DodgeChance + defender->itembonuses.DodgeChance) / 100.0f;
		if (!ghit) {	//if they are not using a garunteed hit discipline
			bonus += 2.0 + skill/35.0;
			roll = MakeRandomFloat(0,100);
			mlog(COMBAT__DAMAGE, "Check dodge. skill %.0f. %.2fpercent chance, roll %.2f", skill, bonus, roll);
			if(roll < bonus)
				damage = -4;
		}
	}
	
	////////////////////////////////////////////////////////
// Scorpious2k: Include AC in the calculation

// use serverop variables to set values
	int myac = GetAC();
	if (damage > 0 && myac > 0) {
		int acfail=1000;
		char tmp[10];

		if (database.GetVariable("ACfail", tmp, 9)) {
			acfail = (int) (atof(tmp) * 100);
			if (acfail>100) acfail=100;
		}

		if (acfail<=0 || rand()%101>acfail) {
			float acreduction=1;
			int acrandom=300;
			if (database.GetVariable("ACreduction", tmp, 9))
			{
				acreduction=atof(tmp);
				if (acreduction>100) acreduction=100;
			}
	
			if (database.GetVariable("ACrandom", tmp, 9))
			{
				acrandom = (int) ((atof(tmp)+1) * 100);
				if (acrandom>10100) acrandom=10100;
			}
			
			if (acreduction>0) {
				damage -= (int) (GetAC() * acreduction/100.0f);
			}		
			if (acrandom>0) {
				damage -= (myac * MakeRandomInt(0, acrandom) / 10000);
			}
			if (damage<1) damage=1;
			mlog(COMBAT__DAMAGE, "AC Damage Reduction: fail chance %d%%. Failed. Reduction %.3f%%, random %d. Resulting damage %d.", acfail, acreduction, acrandom, damage);
		} else {
			mlog(COMBAT__DAMAGE, "AC Damage Reduction: fail chance %d%%. Did not fail.", acfail);
		}
	}
	
	mlog(COMBAT__DAMAGE, "Final damage after all avoidances: %d", damage);
	
	if (damage < 0)
		return true;
	return false;
}

int Mob::GetWeaponDamage(Mob *other, const Item_Struct *weapon_item, bool &bane) {
	bane = false;
	int weapon_damage;
	
	//TODO: I think augments might be able to alter DMG
	
	if(IsClient() && GetLevel() < weapon_item->RecLevel) {
		weapon_damage = CastToClient()->CalcRecommendedLevelBonus(GetLevel(), weapon_item->RecLevel, weapon_item->Damage);
		mlog(COMBAT__DAMAGE, "Base DMG (below recommended level %d) is %d", weapon_item->RecLevel, weapon_damage);
	} else {
		weapon_damage = weapon_item->Damage;
		mlog(COMBAT__DAMAGE, "Base DMG is %d", weapon_damage);
	}
	
	if (weapon_damage < 1)
		weapon_damage = 1;
	
	// Racial bane damage
	if (weapon_item->BaneDmgRaceAmt && other->GetRace() == weapon_item->BaneDmgRace) {
		mlog(COMBAT__DAMAGE, "Adding bane DMG %d on matching race %d", weapon_item->BaneDmgRaceAmt, weapon_item->BaneDmgRace);
		weapon_damage += weapon_item->BaneDmgRaceAmt;
		bane = true;
	}
	
	// Body bane damage
	if (weapon_item->BaneDmgAmt && uint8(other->GetBodyType()) == weapon_item->BaneDmgBody) {
		mlog(COMBAT__DAMAGE, "Adding bane DMG %d on matching body type %d", weapon_item->BaneDmgAmt, weapon_item->BaneDmgBody);
		weapon_damage += weapon_item->BaneDmgAmt;
		bane = true;
	}
	
	// Elemental damage
	if(weapon_item->ElemDmgAmt) {
		float resist = other->ResistSpell(weapon_item->ElemDmgType, 0, this);
		if(resist > 0) {
			mlog(COMBAT__DAMAGE, "Adding Elemental damage of type %d. Base ammount %d, %.3f%% effective.", weapon_item->ElemDmgType, weapon_item->ElemDmgAmt, resist);
			weapon_damage += (int)( (weapon_item->ElemDmgAmt * resist) / 100.0f);
		} else {
			mlog(COMBAT__DAMAGE, "Elemental damage of type %d. Base ammount %d, resisted.", weapon_item->ElemDmgType, weapon_item->ElemDmgAmt);
		}
	}
	
	return(weapon_damage);
}

//note: throughout this method, setting `damage` to a negative is a way to
//stop the attack calculations
bool Client::Attack(Mob* other, int Hand, bool bRiposte)
{
	_ZP(Client_Attack);
	
	mlog(COMBAT__ATTACKS, "Attacking %s with hand %d %s", other?other->GetName():"(NULL)", Hand, bRiposte?"(this is a riposte)":"");
	
	//SetAttackTimer();
	if (
		   (IsCasting() && GetClass() != BARD)
		|| other == NULL
		|| ((IsClient() && CastToClient()->dead) || (other->IsClient() && other->CastToClient()->dead))
		|| (GetHP() < 0)
		|| (!IsAttackAllowed(other))
		) {
		mlog(COMBAT__ATTACKS, "Attack canceled, invalid circumstances.");
		return false; // Only bards can attack while casting
	}
	if(DivineAura() && !GetGM()) {//cant attack while invulnerable unless your a gm
		mlog(COMBAT__ATTACKS, "Attack canceled, Divine Aura is in effect.");
		Message(10,"You can't attack while invulnerable!");
		return false;
	}

	
	ItemInst* weapon;
	if (Hand == 14)	// Kaiyodo - Pick weapon from the attacking hand
		weapon = GetInv().GetItem(SLOT_SECONDARY);
	else
		weapon = GetInv().GetItem(SLOT_PRIMARY);
	
	const Item_Struct *weapon_item = NULL;
	if(weapon != NULL) {
		if (!weapon->IsWeapon()) {
			mlog(COMBAT__ATTACKS, "Attack canceled, Item %s (%d) is not a weapon.", weapon->GetItem()->Name, weapon->GetID());
			return(false);
		}
		weapon_item = weapon->GetItem();
		mlog(COMBAT__ATTACKS, "Attacking with weapon: %s (%d)", weapon->GetItem()->Name, weapon->GetID());
	} else {
		mlog(COMBAT__ATTACKS, "Attacking without a weapon.");
	}
	
	// calculate attack_skill and skillinuse depending on hand and weapon
	// also send Packet to near clients
	SkillType skillinuse;
	AttackAnimation(skillinuse, Hand, weapon);
	mlog(COMBAT__ATTACKS, "Attacking with %s in slot %d using skill %d", weapon_item?weapon_item->Name:"Fist", Hand, skillinuse);
	
	/// Now figure out damage
	int damage = 0;
	
	
	//watch for immunities
	if(other->SpecAttacks[IMMUNE_MELEE]) {
		damage = -5;
		mlog(COMBAT__ATTACKS, "%s is immune to melee.", other->GetName());
	} else if(other->SpecAttacks[IMMUNE_MELEE_NONMAGICAL]) {
		if(weapon_item) {
			if(!weapon_item->Magic) {
				mlog(COMBAT__ATTACKS, "%s is immune to non-magical weapon attacks.", other->GetName());
				damage = -5;
			}
		} else if((GetClass() == MONK || GetClass() == BEASTLORD) && GetLevel() >= 30) {
			//monk fists are magical at 30.. 
			//beastlords lumped in here cause im too lazy to figure out if they are different
		} else {
			mlog(COMBAT__ATTACKS, "%s is immune to non-magical attacks.", other->GetName());
			damage = -5;
		}
	}
	
	int8 mylevel = GetLevel();
	mylevel = mylevel ? mylevel : 1;
	
	//determine the weapon's damage
	int weapon_damage = 0;
	if ( damage >= 0 ) {
		int8 otherlevel = other->GetLevel();
		otherlevel = otherlevel ? otherlevel : 1;
		
		
		/*
			Hand to hand weapons are treated just like any other weapon
			and monks gain no advantage from using them. I dont know
			if this is right..
		*/
		
		if(!weapon) {		//we have no weapon, use fists
			if(other->SpecAttacks[IMMUNE_MELEE_EXCEPT_BANE]) {
				mlog(COMBAT__ATTACKS, "%s is immune to non-bane attacks.", other->GetName());
				damage = -5;
			} else if(GetClass() == MONK || GetClass() == BEASTLORD) {
				weapon_damage = GetMonkHandToHandDamage();	// Damage changes based on level
				mlog(COMBAT__DAMAGE, "Using Monk H2H base DMG %d", weapon_damage);
			} else {
				weapon_damage = 2; // This isn't quite right, something more like level/10 is more appropriate
				mlog(COMBAT__DAMAGE, "Using Non-Monk H2H base DMG %d", weapon_damage);
			}
		} else { //we have a weapon
			
			bool bane = false;
			weapon_damage = GetWeaponDamage(other, weapon_item, bane);
			
			mlog(COMBAT__DAMAGE, "Using weapon DMG %d", weapon_damage);
			if(!bane && other->SpecAttacks[IMMUNE_MELEE_EXCEPT_BANE]) {
				mlog(COMBAT__ATTACKS, "%s is immune to non-bane attacks.", other->GetName());
				damage = -5;
			}
		}
		
		//berserker damage bonus
		if(berserk && GetClass() == BERSERKER) {
			int bonus = 3 + GetLevel()/10;		//unverified
			weapon_damage = weapon_damage * (100+bonus) / 100;
			mlog(COMBAT__DAMAGE, "Berserker damage bonus increases DMG to %d", weapon_damage);
		}
	}
	
	// Determine players ability to hit based on:
	// mob level difference, skillinuse, randomness
	if(damage >= 0) {
		int min_hit = 0;
		int max_hit = 0;
		CheckIncreaseSkill(skillinuse, -10);
		CheckIncreaseSkill(OFFENSE, -10);
		
		//////////////////////////////////////////////////////////
		/////////	Finishing Blow
		/////////////////////////////////////////////////////////
		//kathgar: Made it so players cannot be finishing blowed.. something wanky was going on
		//			Added level limits and fixed the chances to the correct values?
		uint16 aa_item = GetAA(aaFinishingBlow);
		if(aa_item>0 && !other->IsClient() && other->GetHPRatio() < 10 && (other->GetLevel()<=54))	//Don't finishing blow players.. at least for now)
		{
			float tempchancerand = MakeRandomFloat(0, 100);
			if
			(
				(aa_item==1 && (tempchancerand<=2)&& other->GetLevel() <= 54) ||
				(aa_item==2 && (tempchancerand<=5)&& other->GetLevel() <= 52) ||
				(aa_item==3 && (tempchancerand<=7)&& other->GetLevel() <= 50)
			)
			{
				mlog(COMBAT__ATTACKS, "Landed a finishing blow: AA at %d, other level %d, roll %.1f", aa_item, other->GetLevel(), tempchancerand);
				entity_list.MessageClose_StringID(this, false, 200, MT_CritMelee, FINISHING_BLOW, GetName());
				other->Damage(this, 32000, SPELL_UNKNOWN, skillinuse);
				return(true);
			}
			mlog(COMBAT__ATTACKS, "Failed a finishing blow: AA at %d, other level %d, roll %.1f", aa_item, other->GetLevel(), tempchancerand);
		}
		
		min_hit = 1;
		//This needs to be researched, it seems terribly off. Changed to use offense skill for now instead of weapon since we know that is correct
		max_hit = (weapon_damage * (((GetSTR()*20) + (GetSkill(OFFENSE)*15) + (mylevel*10)) / 1000));	// Apply damage formula
		
		// Only apply the damage bonus to the main hand
		if(Hand == 13) {	// Kaiyodo - If we're not using the DWDA stuff, will always be the primary hand
			int damage_bonus = GetWeaponDamageBonus(weapon_item);	// Can be NULL, will then assume fists
			min_hit += damage_bonus;
			max_hit += damage_bonus;
		}
		
		min_hit = min_hit * (100 + itembonuses.MinDamageModifier + spellbonuses.MinDamageModifier) / 100;
	
		if(max_hit <= min_hit)
			damage = min_hit;
		else
			damage = MakeRandomInt(min_hit, max_hit);
		
		mlog(COMBAT__DAMAGE, "Damage calculated to %d (min %d, max %d, str %d, skill %d, DMG %d, lv %d)", damage, min_hit, max_hit
		, GetSTR(), GetSkill(skillinuse), weapon_damage, mylevel);
	
		/*#if 0 // Weighted MDF type damage
			float hml = (float) ((float)rand()/(float)RAND_MAX);
			if(GetLevel()>=25){
				if (hml <= 0.10f){ // Low
					damage = (int32) (min_hit + (rand()%(weighted-min_hit)));
					if(damage > min_hit || damage > weighted || damage < min_hit) {
						damage = min_hit;
					}
				}
				else if (hml >= 0.11f && hml <= 0.89f){ // Middle
					damage = (int32) (weighted + (rand()%(magic_number-weighted)+1));
					if(damage > magic_number || damage < weighted) {
						damage = magic_number;
					}
				}
				else { // High
					damage = (int32) (magic_number + (rand()%(max_hit-magic_number)+1));
					if(damage < magic_number || damage >max_hit) {
						damage = magic_number;
					}
				}
#if EQDEBUG>=11 
					LogFile->write(EQEMuLog::Debug,"%s::Attack(): min_hit:%i max_hit:%i weapon_damage:%i damage:%i mod:%f MN:%i WN:%i HML:%f",
						GetName(), min_hit, max_hit, weapon_damage, damage, (( ((float)GetSTR()) + (float)GetSkill(skillinuse)+ (float)mylevel) / 100), magic_number, weighted , hml);
#endif
			}
		#endif // Weighted MDF type damage*/
	
		//check to see if we hit..
		if(!other->CheckHitChance(this, skillinuse, Hand)) {
			mlog(COMBAT__ATTACKS, "Attack missed. Damage set to 0.");
			damage = 0;
		} else {	//we hit, try to avoid it
			other->AvoidDamage(this, damage);
			ApplyMeleeDamageBonus(skillinuse, damage);
			TryCriticalHit(other, skillinuse, damage);
			mlog(COMBAT__DAMAGE, "Final damage after all reductions: %d", damage);
		}
		
		if (bRiposte && damage == -3) {	//cannot riposte a riposte
			mlog(COMBAT__ATTACKS, "Attack canceled. Cannot riposte a riposte");
			return false;
    	}
	}
	
	///////////////////////////////////////////////////////////
	//////    Send Attack Damage
	///////////////////////////////////////////////////////////
	other->Damage(this, damage, SPELL_UNKNOWN, skillinuse);
	if(damage > 0 && (spellbonuses.MeleeLifetap || itembonuses.MeleeLifetap)) {
		mlog(COMBAT__DAMAGE, "Melee lifetap healing for %d damage.", damage);
		//heal self for damage done..
		HealDamage(damage);
	}
	
	//break invis when you attack
	if(invisible) {
		mlog(COMBAT__ATTACKS, "Removing invisibility due to melee attack.");
		BuffFadeByEffect(SE_Invisibility);
	}
	if(invisible_undead) {
		mlog(COMBAT__ATTACKS, "Removing invisibility vs. undead due to melee attack.");
		BuffFadeByEffect(SE_InvisVsUndead);
	}
	
	////////////////////////////////////////////////////////////
	////////  PROC CODE
	////////  Kaiyodo - Check for proc on weapon based on DEX
	///////////////////////////////////////////////////////////
	if(other->GetHP() > -10) {
		TryWeaponProc(weapon, other);
	}
	
	//handle riposet, ensuring they are in front is checked in AvoidDamage
	//this used to test IsNPC, preventing riposte attacks in PvP
	if( damage == -3 ) {
		DoRiposte(other);
	}
	
	if (damage > 0)
        return true;
	else
		return false;
}

//used by complete heal and #heal
void Mob::Heal()
{
	SetMaxHP();
	SendHPUpdate();
}

void Client::Damage(Mob* other, sint32 damage, int16 spell_id, SkillType attack_skill, bool avoidable, sint8 buffslot, bool iBuffTic)
{
	if(dead || IsCorpse())
		return;
	
	if(spell_id==0)
		spell_id = SPELL_UNKNOWN;
	
	// cut all PVP spell damage to 2/3 -solar
	// EverHood - Blasting ourselfs is considered PvP to
	if(other && other->IsClient() && damage > 0) {
		int PvPMitigation = 100;
		if(attack_skill == ARCHERY)
			PvPMitigation = 80;
		else
			PvPMitigation = 67;
		damage = (damage * PvPMitigation) / 100;
	}
			
	//do a majority of the work...
	CommonDamage(other, damage, spell_id, attack_skill, avoidable, buffslot, iBuffTic);
	
	if (damage > 0) {
		//if the other is not green, and this is not a spell
		if (other && other->IsNPC() && (spell_id == SPELL_UNKNOWN) && GetLevelCon(other->GetLevel()) != CON_GREEN )
			CheckIncreaseSkill(DEFENSE, -10);
	}
}

void Client::Death(Mob* other, sint32 damage, int16 spell, SkillType attack_skill)
{
	if(dead)
		return;	//cant die more than once...
	int exploss;

	mlog(COMBAT__HITS, "Fatal blow dealt by %s with %d damage, spell %d, skill %d", other->GetName(), damage, spell, attack_skill);
	
	//
	// #1: Send death packet to everyone
	//

	if(!spell) spell = SPELL_UNKNOWN;
	
	SendLogoutPackets();
	
	//make our become corpse packet, and queue to ourself before OP_Death.
	EQApplicationPacket app2(OP_BecomeCorpse, sizeof(BecomeCorpse_Struct));
	BecomeCorpse_Struct* bc = (BecomeCorpse_Struct*)app2.pBuffer;
	bc->spawn_id = GetID();
	bc->x = GetX();
	bc->y = GetY();
	bc->z = GetZ();
	QueuePacket(&app2);
	
	// make death packet
	EQApplicationPacket app(OP_Death, sizeof(Death_Struct));
	Death_Struct* d = (Death_Struct*)app.pBuffer;
	d->spawn_id = GetID();
	d->killer_id = other ? other->GetID() : 0;
	//d->unknown12 = 1;
	d->bindzoneid = m_pp.binds[0].zoneId;
	d->spell_id = spell == SPELL_UNKNOWN ? 0xffffffff : spell;
	d->attack_skill = spell != SPELL_UNKNOWN ? 0xe7 : attack_skill;
	d->damage = damage;
	app.priority = 6;
	entity_list.QueueClients(this, &app);

	//
	// #2: figure out things that affect the player dying and mark them dead
	//

	InterruptSpell();
	SetPet(0);
	SetHorseId(0);
	dead = true;
	dead_timer.Start(5000, true);

	if (other != NULL)
	{
		if (other->IsNPC())
			parse->Event(EVENT_SLAY, other->GetNPCTypeID(), 0, other->CastToNPC(), this);
		
		if(other->IsClient() && (IsDueling() || other->CastToClient()->IsDueling())) {
			SetDueling(false);
			SetDuelTarget(0);
			if (other->IsClient() && other->CastToClient()->IsDueling() && other->CastToClient()->GetDuelTarget() == GetID())
			{
				//if duel opponent killed us...
				other->CastToClient()->SetDueling(false);
				other->CastToClient()->SetDuelTarget(0);
				entity_list.DuelMessage(other,this,false);
			} else {
				//otherwise, we just died, end the duel.
				Mob* who = entity_list.GetMob(GetDuelTarget());
				if(who && who->IsClient()) {
					who->CastToClient()->SetDueling(false);
					who->CastToClient()->SetDuelTarget(0);
				}
			}
		}
	}

	entity_list.RemoveFromTargets(this);
	hate_list.RemoveEnt(this);
	
	if(isgrouped) {
		Group *g = GetGroup();
		if(g)
			g->MemberZoned(this);
	}
	
	//remove ourself from all proximities
	ClearAllProximities();

	//
	// #3: exp loss and corpse generation
	//

	// figure out if they should lose exp
	exploss = (int)(GetLevel() * (GetLevel() / 18.0) * 12000);

	if( (GetLevel() < RuleI(Character, DeathExpLossLevel)) || IsBecomeNPC() )
	{
		exploss = 0;
	}
	else if( other )
	{
		if( other->IsClient() )
		{
			exploss = 0;
		}
		else if( other->GetOwner() && other->GetOwner()->IsClient() )
		{
			exploss = 0;
		}
	}

	if(spell != SPELL_UNKNOWN)
	{
		for(uint16 buffIt = 0; buffIt < BUFF_COUNT; buffIt++)
		{
			if(buffs[buffIt].spellid == spell && buffs[buffIt].client)
			{
				exploss = 0;	// no exp loss for pvp dot
				break;
			}
		}
	}
	
	// now we apply the exp loss, unmem their spells, and make a corpse
	// unless they're a GM (or less than lvl 10
	if(!GetGM())
	{
		if(exploss > 0) {
			sint32 newexp = GetEXP();
			if(exploss > newexp) {
				//lost more than we have... wtf..
				newexp = 1;
			} else {
				newexp -= exploss;
			}
			SetEXP(newexp, GetAAXP());
			//m_epp.perAA = 0;	//reset to no AA exp on death.
		}

		//this generates a lot of 'updates' to the client that the client does not need
		BuffFadeAll();
		UnmemSpellAll(false);
		
		if(RuleB(Character, LeaveCorpses))
		{
			// creating the corpse takes the cash/items off the player too
			Corpse *new_corpse = new Corpse(this, exploss);

			char tmp[20];
			database.GetVariable("ServerType", tmp, 9);
			if(atoi(tmp)==1 && other != NULL && other->IsClient()){
				char tmp2[10] = {0};
				database.GetVariable("PvPreward", tmp, 9);
				int reward = atoi(tmp);
				if(reward==3){
					database.GetVariable("PvPitem", tmp2, 9);
					int pvpitem = atoi(tmp2);
					if(pvpitem>0 && pvpitem<200000)
						new_corpse->SetPKItem(pvpitem);
				}
				else if(reward==2)
					new_corpse->SetPKItem(-1);
				else if(reward==1)
					new_corpse->SetPKItem(1);
				else
					new_corpse->SetPKItem(0);
				if(other->CastToClient()->isgrouped) {
					Group* group = entity_list.GetGroupByClient(other->CastToClient());
					if(group != 0) {
						for(int i=0;i<6;i++) {
							if(group->members[i] != NULL) {
								new_corpse->AllowMobLoot(group->members[i],i);
							}
						}
					}
				}
			}
			
			entity_list.AddCorpse(new_corpse, GetID());
			SetID(0);
			
			//send the become corpse packet to everybody else in the zone.
			entity_list.QueueClients(this, &app2, true);
		}

//		if(!IsLD())//Todo: make it so an LDed client leaves corpse if its enabled
//			MakeCorpse(exploss);
	} else {
		BuffFadeDetrimental();
	}



#if 0	// solar: commenting this out for now TODO reimplement becomenpc stuff
	if (IsBecomeNPC() == true)
	{
		if (other != NULL && other->IsClient()) {
			if (other->CastToClient()->isgrouped && entity_list.GetGroupByMob(other) != 0)
				entity_list.GetGroupByMob(other->CastToClient())->SplitExp((uint32)(level*level*75*3.5f), this);

			else
				other->CastToClient()->AddEXP((uint32)(level*level*75*3.5f)); // Pyro: Comment this if NPC death crashes zone
			//hate_list.DoFactionHits(GetNPCFactionID());
		}

		Corpse* corpse = new Corpse(this->CastToClient(), 0);
		entity_list.AddCorpse(corpse, this->GetID());
		this->SetID(0);
		if(other->GetOwner() != 0 && other->GetOwner()->IsClient())
			other = other->GetOwner();
		if(other != 0 && other->IsClient()) {
			corpse->AllowMobLoot(other, 0);
			if(other->CastToClient()->isgrouped) {
				Group* group = entity_list.GetGroupByClient(other->CastToClient());
				if(group != 0) {
					for(int i=0; i < MAX_GROUP_MEMBERS; i++) { // Doesnt work right, needs work
						if(group->members[i] != NULL) {
							corpse->AllowMobLoot(group->members[i],i);
						}
					}
				}
			}
		}
	}
#endif


	//
	// Finally, send em home
	//

	// we change the mob variables, not pp directly, because Save() will copy
	// from these and overwrite what we set in pp anyway
	//

	m_pp.zone_id = m_pp.binds[0].zoneId;
	database.MoveCharacterToZone(this->CharacterID(), database.GetZoneName(m_pp.zone_id));
	
	//treat this like we sent them a zone request message
	zonesummon_x = m_pp.binds[0].x;
	zonesummon_y = m_pp.binds[0].y;
	zonesummon_z = m_pp.binds[0].z;
	zonesummon_id = m_pp.binds[0].zoneId;
	zone_mode = ZoneToBindPoint;
	
	heading = 0;
	
	Save();
	
	//temp hack...
	GoToBind();
}

bool NPC::Attack(Mob* other, int Hand, bool bRiposte)	 // Kaiyodo - base function has changed prototype, need to update overloaded version
{
	_ZP(NPC_Attack);
	int damage = 0;
	
	if (!other) {
		SetTarget(NULL);
		return false;
	}
	
	if (!target && GetTarget() != other)
		SetTarget(other);
	
	//Check that we can attack before we calc heading and face our target	
	if (!IsAttackAllowed(other)) {
		if (this->GetOwnerID())
			entity_list.MessageClose(this, 1, 200, 10, "%s says, 'That is not a legal target master.'", this->GetCleanName());
		if(other)
			RemoveFromHateList(other);
		mlog(COMBAT__ATTACKS, "I am not allowed to attack %s", other->GetName());
		return false;
	}
	float calcheading=CalculateHeadingToTarget(target->GetX(), target->GetY());
	if((calcheading)!=GetHeading()){
		SetHeading(calcheading);
		FaceTarget(target, true);
	}
	
	if(!combat_event) {
		mlog(COMBAT__HITS, "Triggering EVENT_COMBAT due to attack on %s", other->GetName());
		parse->Event(EVENT_COMBAT, this->GetNPCTypeID(), "1", this, other);
		combat_event = true;
	}
	combat_event_timer.Start(CombatEventTimer_expire);
	
	//figure out what weapon they are using, if any
	const Item_Struct* weapon = NULL;
	if (Hand == 13 && equipment[7] > 0)
	    weapon = database.GetItem(equipment[7]);
	else if (equipment[8])
	    weapon = database.GetItem(equipment[8]);
	
	//we dont factor anything from the weapon into the attack.......
	if(weapon) {
		mlog(COMBAT__ATTACKS, "Attacking with weapon: %s (%d) (too bad im not using it for anything)", weapon->Name, weapon->ID);
	}
	
	//watch for immunities
	if(other->SpecAttacks[IMMUNE_MELEE]) {
		damage = -5;
		mlog(COMBAT__ATTACKS, "%s is immune to melee attacks.", other->GetName());
	} else if(other->SpecAttacks[IMMUNE_MELEE_EXCEPT_BANE]) {
		/*
		if(weapon) {
			//We dont take the bane damage into account here, just see if its there
			if (weapon->BaneDmgRaceAmt && other->GetRace() == weapon->BaneDmgRace) {
				//matched on race..
			} else if (weapon->BaneDmgAmt && other->GetBodyType() == weapon->BaneDmgBody) {
				//matched on body type
			} else
				damage = -5;	//no match, immune
		} else {
			//just attacking with hands, never a bane weapon
			damage = -5;
		}
		*/
		
		//use same rules as magical attacks temporarily
		if(weapon) {
			if(!weapon->Magic && GetLevel() < PET_ATTACK_MAGICAL_LEVEL) {
				mlog(COMBAT__ATTACKS, "%s is immune to non-bane attacks and %s is not magical.", other->GetName(), weapon->Name);
				damage = -5;
			}
		} else {
			//just attacking with hands
			if(GetLevel() < PET_ATTACK_MAGICAL_LEVEL) {
				mlog(COMBAT__ATTACKS, "%s is immune to non-bane attacks, and we have no weapon.", other->GetName());
				damage = -5;
			}
		}
	} else if(other->SpecAttacks[IMMUNE_MELEE_NONMAGICAL]) {
		if(weapon) {
			if(!weapon->Magic && GetLevel() < PET_ATTACK_MAGICAL_LEVEL) {
				mlog(COMBAT__ATTACKS, "%s is immune to non-magical attacks and %s is not magical.", other->GetName(), weapon->Name);
				damage = -5;
			}
		} else {
			//just attacking with hands
			if(GetLevel() < PET_ATTACK_MAGICAL_LEVEL) {
				mlog(COMBAT__ATTACKS, "%s is immune to non-magical attacks, and we have no weapon.", other->GetName());
				damage = -5;
			}
		}
	}
	
	SkillType skillinuse = HAND_TO_HAND;
	//basically "if not immune"
	if(damage >= 0) {
		
	
		if (Hand == 14 && weapon && weapon->ItemType == ItemTypeShield) {
			mlog(COMBAT__ATTACKS, "Attack with shield canceled.");
			return false; // <Rogean> Cant Dual Wield with Shields
		}
		
		sint16 charges = 0;
		ItemInst weapon_inst(&database, weapon, charges);
		AttackAnimation(skillinuse, Hand, &weapon_inst);
		
		int8 otherlevel = other->GetLevel();
		int8 mylevel = this->GetLevel();
		
		otherlevel = otherlevel ? otherlevel : 1;
		mylevel = mylevel ? mylevel : 1;
		
		//instead of calcing damage in floats lets just go straight to ints
		damage = MakeRandomInt(min_dmg, max_dmg);
		
		//check if we're hitting above our max or below it.
		if(min_dmg != 0 && damage < min_dmg) {
			mlog(COMBAT__DAMAGE, "Damage (%d) is below min (%d). Setting to min.", damage, min_dmg);
		    damage = min_dmg;
		}
		if(max_dmg != 0 && damage > max_dmg) {
			mlog(COMBAT__DAMAGE, "Damage (%d) is above max (%d). Setting to max.", damage, max_dmg);
		    damage = max_dmg;
		}
		
		//THIS IS WHERE WE CHECK TO SEE IF WE HIT:
		if(other->IsClient() && other->CastToClient()->IsSitting()) {
			mlog(COMBAT__DAMAGE, "Client %s is sitting. Hitting for max damage (%d).", other->GetName(), max_dmg);
			damage = max_dmg;
		} else {
			if(!other->CheckHitChance(this, skillinuse, Hand)) {
				damage = 0;	//miss
			} else {	//hit, check for damage avoidance
				other->AvoidDamage(this, damage);
				ApplyMeleeDamageBonus(skillinuse, damage);
				TryCriticalHit(other, skillinuse, damage);
			}
		}
		
		mlog(COMBAT__DAMAGE, "Final damage against %s: %d", other->GetName(), damage);
		
		if(other->IsClient() && IsPet() && GetOwner()->IsClient()) {
			//pets do half damage to clients in pvp
			damage=damage/2;
		}
	}
		
	//cant riposte a riposte
	if (bRiposte && damage == -3) {
		mlog(COMBAT__DAMAGE, "Riposte of riposte canceled.");
		return false;
	}
		
	if(GetHP() > 0 && other->GetHP() >= -11) {
		other->Damage(this, damage, SPELL_UNKNOWN, skillinuse, false); // Not avoidable client already had thier chance to Avoid
    }
	
	
	//break invis when you attack
	if(invisible) {
		mlog(COMBAT__ATTACKS, "Removing invisibility due to melee attack.");
		BuffFadeByEffect(SE_Invisibility);
	}
	if(invisible_undead) {
		mlog(COMBAT__ATTACKS, "Removing invisibility vs. undead due to melee attack.");
		BuffFadeByEffect(SE_InvisVsUndead);
	}
	
	//I doubt this works...
	if (!target)
		return true; //We killed them
	
	// Kaiyodo - Check for proc on weapon based on DEX
	if( !bRiposte && other->GetHP() > 0 ) {
		TryWeaponProc((const Item_Struct*) NULL, other);	//no weapon
	}
	
	// now check ripostes
	if (damage == -3) { // riposting
		DoRiposte(other);
	}
	
	if (damage > 0)
        return true;
	else
        return false;
}

void NPC::Damage(Mob* other, sint32 damage, int16 spell_id, SkillType attack_skill, bool avoidable, sint8 buffslot, bool iBuffTic) {
	if(spell_id==0)
		spell_id = SPELL_UNKNOWN;
	
	//handle EVENT_ATTACK. Resets after we have not been attacked for 12 seconds
	if(!attack_event) {
		mlog(COMBAT__HITS, "Triggering EVENT_ATTACK due to attack by %s", other->GetName());
		parse->Event(EVENT_ATTACK, this->GetNPCTypeID(), 0, this, other);
		attack_event = true;
	}
	if(!combat_event) {
		mlog(COMBAT__HITS, "Triggering EVENT_COMBAT due to attack by %s", other->GetName());
		parse->Event(EVENT_COMBAT, this->GetNPCTypeID(), "1", this, other);
		combat_event = true;
	}
	attacked_timer.Start(CombatEventTimer_expire - 1);	//-1 to solidify an assumption in NPC::Process
	combat_event_timer.Start(CombatEventTimer_expire);
    
	if (!IsEngaged())
		zone->AddAggroMob();
	
	//do a majority of the work...
	CommonDamage(other, damage, spell_id, attack_skill, avoidable, buffslot, iBuffTic);
	
	if(damage > 0) {
	    if (other)
	        AddRampage(other);
#ifdef FLEE_HP_RATIO
		//see if we are gunna start fleeing
		CheckFlee();
#endif
	}
}

void NPC::Death(Mob* other, sint32 damage, int16 spell, SkillType attack_skill) {

	mlog(COMBAT__HITS, "Fatal blow dealt by %s with %d damage, spell %d, skill %d", other->GetName(), damage, spell, attack_skill);
	
	if (this->IsEngaged())
	{
		zone->DelAggroMob();
#if EQDEBUG >= 11
		LogFile->write(EQEMuLog::Debug,"NPC::Death() Mobs currently Aggro %i", zone->MobsAggroCount());
#endif
	}
	SetHP(0);
	SetPet(0);
	Mob* killer = GetHateDamageTop(this);
	
	entity_list.RemoveFromTargets(this);

	if(p_depop == true)
		return;

	BuffFadeAll();
	
	EQApplicationPacket* app= new EQApplicationPacket(OP_Death,sizeof(Death_Struct));
	Death_Struct* d = (Death_Struct*)app->pBuffer;
	d->spawn_id = GetID();
	d->killer_id = other ? other->GetID() : 0;
//	d->unknown12 = 1;
	d->bindzoneid = 0;
	d->spell_id = spell == SPELL_UNKNOWN ? 0xffffffff : spell;
	d->attack_skill = SkillDamageTypes[attack_skill];
	d->damage = damage;
	app->priority = 6;
	entity_list.QueueClients(other, app, false);
	
	if(respawn2) {
		respawn2->Reset();
		if(respawn2->spawn2_id!=0 && respawn2->respawn_!=0)
		    database.UpdateSpawn2Timeleft(respawn2->spawn2_id, respawn2->respawn_);
	}

	if (other) {
//		if (other->IsClient())
//			other->CastToClient()->QueuePacket(app);
		hate_list.Add(other, damage);
	}

	safe_delete(app);
	
	
	Mob *give_exp = hate_list.GetDamageTop(this);
	if(give_exp == NULL)
		give_exp = killer;
	if(give_exp && give_exp->GetOwner() != 0)
		give_exp = give_exp->GetOwner();
	
	Client *give_exp_client = NULL;
	if(give_exp && give_exp->IsClient())
		give_exp_client = give_exp->CastToClient();
	
    if (give_exp_client && !IsCorpse() && MerchantType == 0)
	{
		Group *kg = entity_list.GetGroupByClient(give_exp_client);
		if (give_exp_client->IsGrouped() && kg != NULL)
		{
			if(give_exp_client->GetAdventureID()>0){
				AdventureInfo AF = database.GetAdventureInfo(give_exp_client->GetAdventureID());
				if(zone->GetZoneID() == AF.zonedungeonid && AF.type==ADVENTURE_MASSKILL)
					give_exp_client->SendAdventureUpdate();
				else if(zone->GetZoneID() == AF.zonedungeonid && AF.type==ADVENTURE_NAMED && (AF.Objetive==GetNPCTypeID() || AF.ObjetiveValue==GetNPCTypeID()))
					give_exp_client->SendAdventureFinish(1, AF.points,true);
			}
			kg->SplitExp((EXP_FORMULA), this);
		}
		else
        {
        	int conlevel = give_exp->GetLevelCon(GetLevel());
            if (conlevel != CON_GREEN)
            {
			    give_exp_client->AddEXP((EXP_FORMULA), conlevel); // Pyro: Comment this if NPC death crashes zone
            }
		}
	}
	
	//do faction hits even if we are a merchant, so long as a player killed us
	if(give_exp_client)
		hate_list.DoFactionHits(GetNPCFactionID());
	
	if (!HasOwner() && class_ != MERCHANT && class_ != ADVENTUREMERCHANT 
		&& MerchantType == 0 && killer && (killer->IsClient() || (killer->HasOwner() && killer->GetOwner()->IsClient())) ) {
		Corpse* corpse = new Corpse(this, &itemlist, GetNPCTypeID(), &NPCTypedata);
		entity_list.AddCorpse(corpse, this->GetID());
		this->SetID(0);
		if(killer->GetOwner() != 0 && killer->GetOwner()->IsClient())
			killer = killer->GetOwner();
		if(killer != 0 && killer->IsClient()) {
			corpse->AllowMobLoot(killer, 0);
			if(killer->IsGrouped()) {
				Group* group = entity_list.GetGroupByClient(killer->CastToClient());
				if(group != 0) {
					for(int i=0;i<6;i++) { // Doesnt work right, needs work
						if(group->members[i] != NULL) {
							corpse->AllowMobLoot(group->members[i],i);
						}
					}
				}
			}
		}
	}
	
	// Parse quests even if we're killed by an NPC
	if(other) {
		Mob *oos = other->GetOwnerOrSelf();
		parse->Event(EVENT_DEATH, this->GetNPCTypeID(),0, this, oos);
		if(oos->IsNPC())
			parse->Event(EVENT_NPC_SLAY, this->GetNPCTypeID(), 0, oos->CastToNPC(), this);
	}
	
	this->WhipeHateList();
	p_depop = true;
	if(other)
		other->SetTarget(NULL);
}


void Mob::AddToHateList(Mob* other, sint32 hate, sint32 damage, bool iYellForHelp, bool bFrenzy, bool iBuffTic) {
    assert(other != NULL);
    if (other == this)
        return;
    if(damage < 0){
        hate = 1;
    }
	bool wasengaged = IsEngaged();
	Mob* owner = other->GetOwner();
	Mob* mypet = this->GetPet();
	Mob* myowner = this->GetOwner();
	
	if(other){
		int hatemod = 100 + other->spellbonuses.hatemod + other->itembonuses.hatemod;
		if(hatemod < 1)
			hatemod = 1;
		hate = ((hate * (hatemod))/100);
	}

	if(IsFamiliar()) //familiars can't really attack anything
		return;	

	if (other == myowner)
		return;
	
	if (owner) { // Other is a pet, add him and it
		// EverHood 6/12/06
		// Can't add a feigned owner to hate list
		if(owner->IsClient() && owner->CastToClient()->GetFeigned()) {
			//they avoid hate due to feign death...
		} else {
			hate_list.Add(owner, 1, damage, false, !iBuffTic);
		}
	}
	
	hate_list.Add(other, hate, damage, bFrenzy, !iBuffTic);
	
	if (mypet) { // I have a pet, add other to it
		mypet->hate_list.Add(other, 1, 0, bFrenzy);
	} else if (myowner) { // I am a pet, add other to owner if it's NPC/LD
		if (myowner->IsAIControlled())
			myowner->hate_list.Add(other, 1, 0, bFrenzy);
	}
	if (!wasengaged) { 
		if(IsNPC() && other->IsClient() && other->CastToClient())
			parse->Event(EVENT_AGGRO, this->GetNPCTypeID(), 0, CastToNPC(), other); 
		AI_Event_Engaged(other, iYellForHelp); 
		adverrorinfo = 8293;
	}
}

// solar: this is called from Damage() when 'this' is attacked by 'other.  
// 'this' is the one being attacked
// 'other' is the attacker
// a damage shield causes damage (or healing) to whoever attacks the wearer
// a reverse ds causes damage to the wearer whenever it attack someone
// given this, a reverse ds must be checked each time the wearer is attacking
// and not when they're attacked
void Mob::DamageShield(Mob* attacker) {
	//a damage shield on a spell is a negative value but on an item it's a positive value so add the spell value and subtract the item value to get the end ds value
	int DS = spellbonuses.DamageShield - itembonuses.DamageShield;
	if(DS == 0)
		return;
		
	if(this == attacker) //I was crashing when I hit myself with melee with a DS on, not sure why but we shouldn't be reflecting damage onto ourselves anyway really.
		return;
	
	mlog(COMBAT__HITS, "Applying Damage Shield of value %d to %s", DS, attacker->GetName());
	
	int16 spellid = SPELL_UNKNOWN;
	if(spellbonuses.DamageShieldSpellID != 0 && spellbonuses.DamageShieldSpellID != SPELL_UNKNOWN)
		spellid = spellbonuses.DamageShieldSpellID;
	//invert DS... spells yeild negative values for a true damage shield
	if(DS < 0) {
		attacker->Damage(this, -DS, spellid, ABJURE/*hackish*/, false);
		// todo: send EnvDamage packet to the attacker
		//entity_list.MessageClose(attacker, 0, 200, 10, "%s takes %d damage from %s's damage shield (%s)", attacker->GetName(), -spells[buffs[i].spellid].base[1], this->GetName(), spells[buffs[i].spellid].name);
	} else {
		//we are healing the attacker...
		attacker->HealDamage(DS);
		//TODO: send a packet???
	}
	
/*	int DS = 0;
//	int DSRev = 0;
	int effect_value, dmg, i, z;

	for (i=0; i < BUFF_COUNT; i++)
	{
		if (buffs[i].spellid != SPELL_UNKNOWN)
		{
			for (z=0; z < EFFECT_COUNT; z++)
			{
				if(IsBlankSpellEffect(buffs[i].spellid, z))
					continue;
				
				effect_value = CalcSpellEffectValue(buffs[i].spellid, z, buffs[i].casterlevel, this);
				
				switch(spells[buffs[i].spellid].effectid[z])
				{
					case SE_DamageShield:
					{
						dmg = effect_value;
						DS += dmg;
						spellid = buffs[i].spellid;
						break;
					}
*/
/*
					case SE_ReverseDS:
					{
						dmg = effect_value;
						DSRev += dmg;
						spellid = buffs[i].spellid;
						break;
					}
*/
/*				}
			}
		}
	}
	// there's somewhat of an issue here that the last (slot wise) damage shield
	// will be the one whose spellid is set here
	if (DS) {
	}*/
/*
	if (DSRev)
	{
		this->ChangeHP(attacker, DSRev, spellid);
	}
*/
}

int Mob::GetWeaponDamageBonus(const Item_Struct* Weapon)
{
	// Kaiyodo - Calculate the damage bonus for a weapon on the main hand
	if (GetLevel() < 28)
		return(0);
	
	// Check we're on of the classes that gets a damage bonus
	if (!IsWarriorClass())
		return 0;
	
	int BasicBonus = ((GetLevel() - 25) / 3) + 1;
	
	if(!Weapon)
		return(BasicBonus);
	
	// If we have no weapon, or only a single handed weapon, just return the default
	// damage bonus of (Level - 25) / 3
	if (Weapon->ItemClass == ItemClassCommon)
		return BasicBonus;
	
	if ((Weapon->ItemType == ItemType1HS) || (Weapon->ItemType == ItemTypePierce) || (Weapon->ItemType == ItemType1HB))
		return BasicBonus;
	
	// Things get more complicated with 2 handers, the bonus is based on the delay of
	// the weapon as well as a number stored inside the weapon.
	int WeaponBonus = 0;	// How do you find this out?
	
	// Data for this, again, from www.monkly-business.com
	if (Weapon->Delay <= 27)
		return (WeaponBonus + BasicBonus + 1);
	if (Weapon->Delay <= 39)
		return (WeaponBonus + BasicBonus + ((GetLevel()-27) / 4));
	if (Weapon->Delay <= 42)
		return (WeaponBonus + BasicBonus + ((GetLevel()-27) / 4) + 1);
	// Weapon must be > 42 delay
	return (WeaponBonus + BasicBonus + ((GetLevel()-27) / 4) + ((Weapon->Delay-34) / 3));
}

int Mob::GetMonkHandToHandDamage(void)
{
	// Kaiyodo - Determine a monk's fist damage. Table data from www.monkly-business.com
	// saved as static array - this should speed this function up considerably
	static int damage[66] = {
	//   0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19
        99, 4, 4, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7,
         8, 8, 8, 8, 8, 9, 9, 9, 9, 9,10,10,10,10,10,11,11,11,11,11,
        12,12,12,12,12,13,13,13,13,13,14,14,14,14,14,14,14,14,14,14,
        14,14,15,15,15,15 };
	
	// Have a look to see if we have epic fists on
	if (IsClient() && CastToClient()->GetItemIDAt(12) == 10652)
		return(9);
	else
	{
		int Level = GetLevel();
        if (Level > 65)
		    return(19);
        else
            return damage[Level];
	}
}

int Mob::GetMonkHandToHandDelay(void)
{
	// Kaiyodo - Determine a monk's fist delay. Table data from www.monkly-business.com
	// saved as static array - this should speed this function up considerably
	static int delayshuman[66] = {
	//  0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19
        99,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,
        36,36,36,36,36,35,35,35,35,35,34,34,34,34,34,33,33,33,33,33,
        32,32,32,32,32,31,31,31,31,31,30,30,30,29,29,29,28,28,28,27,
        26,24,22,20,20,20  };
	static int delaysiksar[66] = {
	//  0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19
        99,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,36,
        36,36,36,36,36,36,36,36,36,36,35,35,35,35,35,34,34,34,34,34,
        33,33,33,33,33,32,32,32,32,32,31,31,31,30,30,30,29,29,29,28,
        27,24,22,20,20,20 };
	
	// Have a look to see if we have epic fists on
	if (IsClient() && CastToClient()->GetItemIDAt(12) == 10652)
		return(16);
	else
	{
		int Level = GetLevel();
		if (GetRace() == HUMAN)
		{
            if (Level > 65)
			    return(24);
            else
                return delayshuman[Level];
		}
		else	//heko: iksar table
		{
            if (Level > 65)
			    return(25);
            else
                return delaysiksar[Level];
		}
	}
}


sint32 Mob::ReduceMagicalDamage(sint32 damage) {
	if(damage < 0)
		return(damage);
	
	int16 in_rune = GetMagicRune();
	if(in_rune == 0)
		return(damage);
	
	if (in_rune >= damage)
	{
		in_rune -= damage;
		damage = 0;
	}
	else
	{
		damage += in_rune;
		in_rune = 0;
        int slot = GetBuffSlotFromType(SE_AbsorbMagicAtt);
        if (slot >= 0)
            BuffFadeBySlot(slot);
	}
	SetMagicRune(in_rune);
	return damage;
}

sint32 Mob::ReduceDamage(sint32 damage){
	if(damage < 0)
		return(damage);
	
	int16 in_rune = GetRune();
	if(in_rune == 0)
		return(damage);
	
	if (in_rune >= damage)
	{
		in_rune -= damage;
		damage = -6;
	}
	else
	{
		damage -= in_rune;
		in_rune = 0;
        int slot = GetBuffSlotFromType(SE_Rune);
		LogFile->write(EQEMuLog::Normal, "Fading rune from slot %d",slot);
        if (slot >= 0)
            BuffFadeBySlot(slot);
	}
	SetRune(in_rune);
	return damage;
}

bool Mob::HasProcs() const
{
    for (int i = 0; i < MAX_PROCS; i++)
        if (PermaProcs[i].spellID != SPELL_UNKNOWN || SpellProcs[i].spellID != SPELL_UNKNOWN)
            return true;
    return false;
}

bool Client::CheckDoubleAttack(bool AAadd, bool Triple) {
	int skill = 0;
	if (Triple)
	{
		if(!HasSkill(DOUBLE_ATTACK))
			return(false);
		
		if (GetClass() == MONK)
		{
			skill = GetSkill(DOUBLE_ATTACK)/2;
		} else {
			return(false);
		}
	}
	else
	{
		
		//should these stack with skill, or does that ever even happen?
		int aaskill = GetAA(aaBestialFrenzy)*25 + GetAA(aaHarmoniousAttack)*25;
		if (!aaskill && !HasSkill(DOUBLE_ATTACK))
		{
			return false;
		}
		skill = GetSkill(DOUBLE_ATTACK);
		if (aaskill > skill) {
			skill = aaskill;
		}
		
		//discipline effects
		skill += (spellbonuses.DoubleAttackChance + itembonuses.DoubleAttackChance) * 3;
		
		if(skill < 300)	//only gain if we arnt garunteed
			CheckIncreaseSkill(DOUBLE_ATTACK);
	}
	if(MakeRandomInt(0, 299) < skill)
	{
		return true;
	}
	return false;
}

void Mob::CommonDamage(Mob* attacker, sint32 &damage, const int16 spell_id, const SkillType skill_used, bool &avoidable, const sint8 buffslot, const bool iBuffTic) {
	
	mlog(COMBAT__HITS, "Applying damage %d done by %s with skill %d and spell %d, avoidable? %s, is %sa buff tic in slot %d",
		damage, attacker?attacker->GetName():"NOBODY", skill_used, spell_id, avoidable?"yes":"no", iBuffTic?"":"not ", buffslot);
	
	if (GetInvul() || DivineAura()) {
		mlog(COMBAT__DAMAGE, "Avoiding %d damage due to invulnerability.", damage);
		damage = -5;
	}
	
	if( spell_id != SPELL_UNKNOWN || attacker == NULL )
		avoidable = false;
	
    // only apply DS if physical damage (no spell damage)
    // damage shield calls this function with spell_id set, so its unavoidable
	if (attacker && damage > 0 && spell_id == SPELL_UNKNOWN) {
		this->DamageShield(attacker);
	}
	
/*	not sure what this was all about
    if ((spell_id != SPELL_UNKNOWN || (skill_used>200 && skill_used<250)) && damage>0) {
			// todo: exchange that for EnvDamage-Packets when we know how to do it
			char val1[20]={0};
			Message_StringID(4,attacker_HIT_NONMELEE,GetName(),ConvertArray(damage,val1));
			//Message(4,"%s was hit by non-melee for %d points of damage.", GetName(), damage);
    }
*/
	
	if(attacker && damage != -5) {	//no hate if we are completely immune, just laugh it off
		sint32 hate = 0;
		if (skill_used == ARCHERY)
			hate = 1;	// almost no aggro for archery
		else if(damage < 1)
			hate = 1;
		else if(spell_id != SPELL_UNKNOWN)
			hate = (damage+1) / 2;	// half aggro for spells
		else
			hate = damage; // normal aggro for everything else

		if(attacker->IsClient() && attacker->CastToClient()->GetFeigned()) {
			mlog(COMBAT__HITS, "Attacker %s avoids %d hate due to feign death", attacker->GetName(), hate);
		} else {
			mlog(COMBAT__HITS, "Generating hate %d towards %s", hate, attacker->GetName());
			// now add done damage to the hate list
			AddToHateList(attacker, hate, damage, true, false, iBuffTic);
		}
	}
    
	if(damage > 0) {
		//if there is some damage being done and theres an attacker involved
		if(attacker) {
			// if spell is lifetap add hp to the caster
			if (spell_id != SPELL_UNKNOWN && IsLifetapSpell( spell_id )) {
				
				mlog(COMBAT__DAMAGE, "Applying lifetap heal of %d to %s", damage, attacker->GetName());
				
				attacker->HealDamage(damage);
				
				//we used to do a message to the client, but its gone now.
				// emote goes with every one ... even npcs
				entity_list.MessageClose(this, true, 300, MT_Emote, "%s beams a smile at %s", attacker->GetCleanName(), this->GetCleanName() );
			}
			
			// if we got a pet, thats not already fighting something send it into battle
			if(HasPet()) {
				Mob *pet = GetPet();
			    if (pet && !pet->IsEngaged() && attacker != this) {
			    	mlog(PETS__AGGRO, "Sending pet %s into battle due to attack.", pet->GetName());
					pet->AddToHateList(attacker, 1);
					pet->SetTarget(attacker);
					Message_StringID(10, PET_ATTACKING, pet->GetCleanName(), attacker->GetCleanName());
				}
			}
			
		}	//end `if there is some damage being done and theres anattacker person involved`
	
		//see if any runes want to reduce this damage
		if(spell_id == SPELL_UNKNOWN) {
			damage = ReduceDamage(damage);
			mlog(COMBAT__HITS, "Melee Damage reduced to %d", damage);
		} else {
			sint32 origdmg = damage;
			damage = ReduceMagicalDamage(damage);
			mlog(COMBAT__HITS, "Melee Damage reduced to %d", damage);
			if (origdmg != damage && attacker && attacker->IsClient()) {
				if(attacker->CastToClient()->GetFilter(FILTER_DAMAGESHIELD) != FilterHide)
					attacker->Message(15, "The Spellshield absorbed %d of %d points of damage", origdmg - damage, origdmg);
			}
		}
		
		if (sneaking){
			sneaking = false;
			// FIXME Break sneak packet goes here.
		}
	
		//final damage has been determined.
		
		//check for death conditions
		if(IsClient()) {
			if((GetHP() - damage) <= -10) {
				Death(attacker, damage, spell_id, skill_used);
				return;
			}
		} else {
			if (damage >= GetHP()) {
				//killed...
				SetHP(-100);
				Death(attacker, damage, spell_id, skill_used);
				return;
			}
		}
		
		//not killed. Apply the damage
		SetHP(GetHP() - damage);
		
		
    	//fade mez if we are mezzed
		if (IsMezzed()) {
			mlog(COMBAT__HITS, "Breaking mez due to attack.");
			BuffFadeByEffect(SE_Mez);
		}
    	
    	//check stun chances if bashing
		if (skill_used == BASH && GetLevel() < 56) {
			int stun_resist = itembonuses.StunResist+spellbonuses.StunResist;
			if(stun_resist <= 0 || MakeRandomInt(0,99) >= stun_resist) {
				mlog(COMBAT__HITS, "Stunned. We had %dpercent resist chance.");
				Stun(0);
			} else {
				mlog(COMBAT__HITS, "Stun Resisted. We had %dpercent resist chance.");
			}
		}
		
		if(spell_id != SPELL_UNKNOWN) {
			//see if root will break
			if (IsRooted()) { // neotoyko: only spells cancel root
				if (MakeRandomInt(0, 99) < 20) {
					mlog(COMBAT__HITS, "Melee attack broke root! 20percent chance");
					BuffFadeByEffect(SE_Root, buffslot); // buff slot is passed through so a root w/ dam doesnt cancel itself
				} else {
					mlog(COMBAT__HITS, "Melee attack did not break root. 20 percent chance");
				}
			}
			
			//increment chances of interrupting
			if(IsCasting()) { //shouldnt interrupt on regular spell damage
				attacked_count++;
				mlog(COMBAT__HITS, "Melee attack while casting. Attack count %d", attacked_count);
			}
		}
		
		//send an HP update if we are hurt
		if(GetHP() < GetMaxHP())
			SendHPUpdate();
	}	//end `if damage was done`
	
    //send damage packet...
   	if(!iBuffTic) { //buff ticks do not send damage, instead they just call SendHPUpdate(), which is done below
		EQApplicationPacket* outapp = new EQApplicationPacket(OP_Damage, sizeof(CombatDamage_Struct));
		CombatDamage_Struct* a = (CombatDamage_Struct*)outapp->pBuffer;
		a->target = GetID();
		if (attacker == NULL)
			a->source = 0;
		else if (attacker->IsClient() && attacker->CastToClient()->GMHideMe())
			a->source = 0;
		else
			a->source = attacker->GetID();
	    a->type = SkillDamageTypes[skill_used]; // was 0x1c
		a->damage = damage;
//		if (attack_skill != 231)
//			a->spellid = SPELL_UNKNOWN;
//		else
			a->spellid = spell_id;
		
		//Note: if players can become pets, they will not receive damage messages of their own
		//this was done to simplify the code here (since we can only effectively skip one mob on queue)
		eqFilterType filter;
		Mob *skip = attacker;
		if(attacker && attacker->GetOwnerID()) {
			//attacker is a pet, let pet owners see their pet's damage
			Mob* owner = attacker->GetOwner();
			if (owner && owner->IsClient()) {
				if ((spell_id != SPELL_UNKNOWN) && damage>0) {
					//special crap for spell damage, looks hackish to me
					char val1[20]={0};
					owner->Message_StringID(4,OTHER_HIT_NONMELEE,GetCleanName(),ConvertArray(damage,val1));
			    } else {
			    	if(damage > 0) {
						if(spell_id != SPELL_UNKNOWN)
							filter = iBuffTic ? FilterDOT : FilterSpellDamage;
						else
							filter = FILTER_MYPETHITS;
					} else if(damage == -5)
						filter = FilterNone;	//cant filter invulnerable
					else
						filter = FILTER_MYPETMISSES;
					owner->CastToClient()->QueuePacket(outapp,true,CLIENT_CONNECTED,filter);
				}
			}
			skip = owner;
		} else {
			//attacker is not a pet, send to the attacker
			
			//if the attacker is a client, try them with the correct filter
			if(attacker && attacker->IsClient()) {
				if ((spell_id != SPELL_UNKNOWN) && damage>0) {
					//special crap for spell damage, looks hackish to me
					char val1[20]={0};
					attacker->Message_StringID(4,OTHER_HIT_NONMELEE,GetCleanName(),ConvertArray(damage,val1));
			    } else {
			    	if(damage > 0) {
						if(spell_id != SPELL_UNKNOWN)
							filter = iBuffTic ? FilterDOT : FilterSpellDamage;
						else
							filter = FilterNone;	//cant filter our own hits
					} else if(damage == -5)
						filter = FilterNone;	//cant filter invulnerable
					else
						filter = FILTER_MYMISSES;
					attacker->CastToClient()->QueuePacket(outapp, true, CLIENT_CONNECTED, filter);
				}
			}
			skip = attacker;
		}
		
		//send damage to all clients around except the specified skip mob (attacker or the attacker's owner) and ourself
		if(damage > 0) {
			if(spell_id != SPELL_UNKNOWN)
				filter = iBuffTic ? FilterDOT : FilterSpellDamage;
			else
				filter = FILTER_OTHERHITS;
		} else if(damage == -5)
			filter = FilterNone;	//cant filter invulnerable
		else
			filter = FILTER_OTHERMISSES;
		//make attacker (the attacker) send the packet so we can skip them and the owner
		//this call will send the packet to `this` as well (using the wrong filter) (will not happen until PC charm works)
//LogFile->write(EQEMuLog::Debug, "Queue damage to all except %s with filter %d (%d), type %d", skip->GetName(), filter, IsClient()?CastToClient()->GetFilter(filter):-1, a->type);
		entity_list.QueueCloseClients(this, outapp, true, 200, skip, true, filter);
		
		//send the damage to ourself if we are a client
		if(IsClient()) {
			//I dont think any filters apply to damage affecting us
			CastToClient()->QueuePacket(outapp);
		}
		
		safe_delete(outapp);
	} else {
        //else, it is a buff tic...
		// Everhood - So we can see our dot dmg like live shows it.
		if(spell_id != SPELL_UNKNOWN && damage > 0 && attacker && attacker != this && attacker->IsClient()) {
			//might filter on (attack_skill>200 && attack_skill<250), but I dont think we need it
			if(attacker->CastToClient()->GetFilter(FilterDOT) != FilterHide) {
				attacker->Message_StringID(MT_DoTDamage, OTHER_HIT_DOT, GetCleanName(),itoa(damage),spells[spell_id].name);
			}
		}
	} //end packet sending
}


void Mob::HealDamage(uint32 amount, Mob* caster) {
	uint32 maxhp = GetMaxHP();
	uint32 curhp = GetHP();
	uint32 acthealed = 0;
	if(amount > (maxhp - curhp))
		acthealed = (maxhp - curhp);
	else
		acthealed = amount;
		
	if(acthealed > 100){
		if(caster){
			Message(MT_NonMelee, "You have been healed by %s for %d points of damage.", caster->GetCleanName(), acthealed);
			if(caster != this){
				caster->Message(MT_NonMelee, "You have healed %s for %d points of damage.", GetCleanName(), acthealed);
			}
		}
		else{
			Message(MT_NonMelee, "You have been healed for %d points of damage.", acthealed);
		}	
	}		
		
	if (curhp < maxhp) {
		if ((curhp+amount)>maxhp)
			curhp=maxhp;
		else
			curhp+=amount;
		SetHP(curhp);

		SendHPUpdate();
	}
}

//proc chance includes proc bonus
float Mob::GetProcChances(float &ProcBonus, float &ProcChance) {
	int mydex = GetDEX();
	int AABonus = 0;
	ProcBonus = 0;
	if(IsClient()) {
		//increases based off 1 guys observed results.
		switch(CastToClient()->GetAA(aaWeaponAffinity)) {
			case 1:
				AABonus = 5;
				break;
			case 2:
				AABonus = 10;
				break;
			case 3:
				AABonus = 15;
				break;
			case 4:
				AABonus = 20;
				break;
			case 5:
				AABonus = 25;
				break;
		}
	}
	ProcBonus += float(itembonuses.ProcChance + spellbonuses.ProcChance) / 1000.0f;
	
	ProcChance = float(mydex) / 3020.0f;
	ProcBonus += (ProcChance * AABonus) / 100;
	ProcChance += ProcBonus;
	mlog(COMBAT__PROCS, "Proc chance %.2f (%.2f from bonuses)", ProcChance, ProcBonus);
	return ProcChance;
}


void Mob::TryWeaponProc(const ItemInst* weapon_g, Mob *on) {
	if(!weapon_g || !weapon_g->IsType(ItemClassCommon)) {
		TryWeaponProc((const Item_Struct*) NULL, on);
		return;
	}
	
	//do main procs
	TryWeaponProc(weapon_g->GetItem(), on);
	
	//we have to calculate these again, oh well
	int ourlevel = GetLevel();
	float ProcChance, ProcBonus;
	GetProcChances(ProcBonus, ProcChance);
	
	//do augment procs
	int r;
	for(r = 0; r < MAX_AUGMENT_SLOTS; r++) {
		const ItemInst* aug_i = weapon_g->GetAugment(r);
		if(!aug_i)
			continue;
		const Item_Struct* aug = aug_i->GetItem();
		if(!aug)
			continue;
		
		if (IsValidSpell(aug->Proc.Effect) 
			&& (aug->Proc.Type == ET_CombatProc)) {
			if (MakeRandomFloat(0, 1) < ProcChance) {	// 255 dex = 0.084 chance of proc. No idea what this number should be really.
				if(aug->Proc.Level > ourlevel) {
					Mob * own = GetOwner();
					if(own != NULL) {
						own->Message_StringID(13,PROC_PETTOOLOW);
					} else {
						Message_StringID(13,PROC_TOOLOW);
					}
				} else {
					ExecWeaponProc(aug->Proc.Effect, on);
				}
			}
		}
	}
}

void Mob::TryWeaponProc(const Item_Struct* weapon, Mob *on) {
	
	int ourlevel = GetLevel();
	float ProcChance, ProcBonus;
	GetProcChances(ProcBonus, ProcChance);
	
	//give weapon a chance to proc first.
	if(weapon != NULL) {
		if (IsValidSpell(weapon->Proc.Effect) && (weapon->Proc.Type == ET_CombatProc)) {
			float WPC = (weapon->ProcRate/100.0f) + ProcChance;
			if (MakeRandomFloat(0, 1) < WPC) {	// 255 dex = 0.084 chance of proc. No idea what this number should be really.
				if(weapon->Proc.Level > ourlevel) {
					mlog(COMBAT__PROCS, "Tried to proc (%s), but our level (%d) is lower than required (%d)", weapon->Name, ourlevel, weapon->Proc.Level);
					Mob * own = GetOwner();
					if(own != NULL) {
						own->Message_StringID(13,PROC_PETTOOLOW);
					} else {
						Message_StringID(13,PROC_TOOLOW);
					}
				} else {
					mlog(COMBAT__PROCS, "Attacking weapon (%s) successfully procing spell %d (%.2f percent chance)", weapon->Name, weapon->Proc.Effect, ProcChance*100);
					ExecWeaponProc(weapon->Proc.Effect, on);
					return;
				}
			} else {
				mlog(COMBAT__PROCS, "Attacking weapon (%s) did no proc (%.2f percent chance).", weapon->Name, ProcChance*100);
			}
		}
	}
	
	int ourclass = GetClass();
	
	//now try our proc arrays
	float procmod =  float(GetDEX()) / 100.0f + ProcBonus*100.0;	//did somebody think about this???
	uint32 i;
	for(i = 0; i < MAX_PROCS; i++) {
		if (PermaProcs[i].spellID != SPELL_UNKNOWN) {
			float chance = PermaProcs[i].chance * procmod;
			if(MakeRandomFloat(0,99) < chance) {
				int spelllevel = spells[PermaProcs[i].spellID].classes[ourclass-1];
				//pets must be high enough to cast the spell..?
				//this is kinda a screwed up rule...
				if(spelllevel < 127 && ourlevel < spelllevel && GetOwner() != NULL) {
					mlog(COMBAT__PROCS, "Failed to proc permanent proc %d, spell %d, our level (%d) is lower than required (%d)", i, PermaProcs[i].spellID, ourlevel, spelllevel);
					GetOwner()->Message_StringID(13,PROC_PETTOOLOW);
				} else {
					mlog(COMBAT__PROCS, "Permanent proc %d procing spell %d (%.2f percent chance(%d*(%.4f dex + %.4f bonus)))", i, PermaProcs[i].spellID, chance, PermaProcs[i].chance, procmod-ProcBonus, ProcBonus);
					ExecWeaponProc(PermaProcs[i].spellID, on);
					break;
				}
			} else {
				mlog(COMBAT__PROCS, "Permanent proc %d failed to proc %d (%.2f percent chance(%d*(%.4f dex + %.4f bonus)))", i, PermaProcs[i].spellID, chance, PermaProcs[i].chance, procmod-ProcBonus, ProcBonus);
			}
		}
		if (SpellProcs[i].spellID != SPELL_UNKNOWN) {
			float chance = SpellProcs[i].chance * procmod;
			if(MakeRandomFloat(0,99) < chance) {
				int spelllevel = spells[SpellProcs[i].spellID].classes[ourclass-1];
				//pets must be high enough to cast the spell..?
				//this is kinda a screwed up rule...
				if(spelllevel < 127 && ourlevel < spelllevel && GetOwner() != NULL) {
					mlog(COMBAT__PROCS, "Failed to proc permanent proc %d, spell %d, our level (%d) is lower than required (%d)", i, SpellProcs[i].spellID, ourlevel, spelllevel);
					GetOwner()->Message_StringID(13,PROC_PETTOOLOW);
				} else {
					mlog(COMBAT__PROCS, "Permanent proc %d procing spell %d (%.2f percent chance)", i, SpellProcs[i].spellID, chance);
					ExecWeaponProc(SpellProcs[i].spellID, on);
					break;
				}
			} else {
				mlog(COMBAT__PROCS, "Permanent proc %d failed to proc %d (%.2f percent chance)", i, SpellProcs[i].spellID, chance);
			}
		}
	}
}

void Mob::TryCriticalHit(Mob *defender, int16 skill, sint32 &damage)
{
	if(damage < 1) //We can't critical hit if we don't hit.
		return;
 
	float critChance = RuleR(Combat, BaseCritChance);
	if(IsClient())
		critChance += RuleR(Combat, ClientBaseCritChance);	
	//Use a real value because there are spells/skills that can up the crit mod by a percent and while
	//They are not implemented yet it seems like a good idea to keep it open for when they are.
	sint8 critMod = 2; 
	if((GetClass() == WARRIOR || GetClass() == BERSERKER) && GetLevel() >= 12 && IsClient()) 
	{
		if(CastToClient()->berserk)
		{
			critChance += RuleR(Combat, BerserkBaseCritChance);
			critMod = 4;
		}
		else
		{
			critChance += RuleR(Combat, WarBerBaseCritChance);
			critMod = 2;
		}
	}
 
	switch(GetAA(aaCombatFury))
	{
	case 1:
		critChance += 0.02f;
		break;
	case 2:
		critChance += 0.04f;
		break;
	case 3:
		critChance += 0.07f;
		break;
	default:
		break;
	}
	float CritBonus = spellbonuses.CriticalHitChance + itembonuses.CriticalHitChance;
	if(CritBonus > 0.0 && critChance < 0.01) //If we have a bonus to crit in items or spells but no actual chance to crit
		critChance = 0.01f; //Give them a small one so skills and items appear to have some effect.
 
	critChance += ((critChance) * (CritBonus) / 100.0f); //crit chance is a % increase to your reg chance
 
	if(critChance > 0){
		if(MakeRandomFloat(0, 1) <= critChance)
		{
			damage = (damage * critMod);
			if(IsClient() && CastToClient()->berserk)
			{
				entity_list.MessageClose(this, false, 200, 10, "%s lands a crippling blow!(%d)", GetCleanName(), damage);
			}
			else
			{
				entity_list.MessageClose(this, false, 200, 10, "%s scores a critical hit!(%d)", GetCleanName(), damage);
			}
		}
	}
}

void Mob::DoRiposte(Mob *defender){
		mlog(COMBAT__ATTACKS, "Preforming a riposte");
	    defender->Attack(this, 13, true);
	    
		//double riposte
		int DoubleRipChance = 0;
		switch(defender->GetAA(aaDoubleRiposte)) {
		case 1: 
			DoubleRipChance = 15;
			break;
		case 2:
			DoubleRipChance = 35;
			break;
		case 3:
			DoubleRipChance = 50;
			break;
		}
		if(DoubleRipChance >= MakeRandomInt(0, 100)) {
			mlog(COMBAT__ATTACKS, "Preforming a double riposed (%d percent chance)", DoubleRipChance);
			defender->Attack(this, 13, true);
		}
}
 
void Mob::ApplyMeleeDamageBonus(int16 skill, sint32 &damage){
	if(damage < 1)
		return;
 
	if(IsNPC()){ //across the board NPC damage bonuses.
 		//only account for STR here, assume their base STR was factored into their DB damages
		int dmgbonusmod = 0;
		dmgbonusmod += (100*(itembonuses.STR + spellbonuses.STR))/3;
		dmgbonusmod += (100*(spellbonuses.ATK + itembonuses.ATK))/5;
		mlog(COMBAT__DAMAGE, "Damage bonus: %d percent from ATK and STR bonuses.", (dmgbonusmod/100));
		damage += (damage*dmgbonusmod/10000);
	}
  
	if(spellbonuses.DamageModifierSkill == skill || spellbonuses.DamageModifierSkill == 255){
		damage += ((damage * spellbonuses.DamageModifier)/100);
	}
 
	if(itembonuses.DamageModifierSkill == skill || itembonuses.DamageModifierSkill == 255){
		damage += ((damage * itembonuses.DamageModifier)/100);
	}
}


