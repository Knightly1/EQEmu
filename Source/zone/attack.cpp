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
#ifdef GUILDWARS
#include "../GuildWars/GuildWars.h"
#include "../common/guilds.h"
extern GuildRanks_Struct guilds[512];
extern GuildLocationList location_list;
extern GuildWars guildwars;
#endif

#ifdef WIN32
#define snprintf	_snprintf
#define strncasecmp	_strnicmp
#define strcasecmp  _stricmp
#endif

extern Database database;
extern EntityList entity_list;
#ifndef NEW_LoadSPDat
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif

#ifdef RAIDADDICTS
#include "RaidAddicts.h"
extern RaidAddicts raidaddicts;
#endif


extern Zone* zone;

bool Mob::AttackAnimation(int &attack_skill, int16 &skillinuse, int Hand, const ItemInst* weapon)
{
	// Determine animation
	int type = 0;
	if (weapon && weapon->IsType(ItemTypeCommon)) {
		const Item_Struct* item = weapon->GetItem();
#if EQDEBUG >= 11
			LogFile->write(EQEMuLog::Debug, "Weapon skill:%i", item->Common.ItemUse);
#endif		
		switch (item->Common.ItemUse)
		{
		case ItemUse1HS: // 1H Slashing
		{
			attack_skill = 1;
			skillinuse = _1H_SLASHING;
			type = anim1HWeapon;
			break;
		}
		case ItemUse2HS: // 2H Slashing
		{
			attack_skill = 1;
			skillinuse = _2H_SLASHING;
			type = anim2HSlashing;
			break;
		}
		case ItemUsePierce: // Piercing
		{
			attack_skill = 36;
			skillinuse = PIERCING;
			type = animPiercing;
			break;
		}
		case ItemUse1HB: // 1H Blunt
		{
			attack_skill = 0;
			skillinuse = _1H_BLUNT;
			type = anim1HWeapon;
			break;
		}
		case ItemUse2HB: // 2H Blunt
		{
			attack_skill = 0;
			skillinuse = _2H_BLUNT;
			type = anim2HWeapon;
			break;
		}
		case ItemUse2HPierce: // 2H Piercing
		{
			attack_skill = 36;
			skillinuse = PIERCING;
			type = anim2HWeapon;
			break;
		}
		case ItemUseHand2Hand:
		{
			attack_skill = 4;
			skillinuse = HAND_TO_HAND;
			type = animHand2Hand;
			break;
		}
		default:
		{
			attack_skill = 4;
			skillinuse = HIGHEST_SKILL+1;
			type = animHand2Hand;
			break;
		}
		}// switch
	}
	else
	{
		attack_skill = 4;
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
bool Mob::CheckHitChance(Mob* other, int8 attack_skill, int Hand, int16 skillinuse)
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
	chancetohit += (float) level_difference * 2;

	#if EQDEBUG>=11
		LogFile->write(EQEMuLog::Debug, "1 chance to hit is: %5.2f", chancetohit);
	#endif
		
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
		chancetohit += (float)((float)attacker->GetSkill(OFFENSE) * 0.20f);
		chancetohit -= (float)((float)defender->GetSkill(DEFENSE) * 0.05f);
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
	}

#if ATTACK_DEBUG>=15
		LogFile->write(EQEMuLog::Debug, "%s::AvoidDamage(%s) pre stats %f", GetName(), other->GetName(), chancetohit);
#endif
	sint16 defender_agi = defender->GetAGI();
	// skill points over 200 are 1/5 as effective
	// at max stat of 252 this is a 10.52% bonus
	defender_agi = (defender_agi <= 200) ? defender_agi : defender_agi + ((defender_agi-200)/10);
	chancetohit -= (float)((float)defender_agi * 0.05f);
	
	// this comes out to about 1% for every 7 dex over 50
	// so someone with 105 dex gets 8.25% added here
	sint16 attacker_dex = attacker->GetDEX();
	attacker_dex = (attacker_dex <= 200) ? attacker_dex : attacker_dex + ((attacker_dex-200)/25);
	attacker_dex -= 50;
	chancetohit += (float) ((float)attacker_dex * 0.15f);

	#if EQDEBUG>=11
		LogFile->write(EQEMuLog::Debug, "2 chance to hit after accounting for skills, agi, and dex: %5.2f", chancetohit);
	#endif
	
	
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
	chancetohit += attacker->spellbonuses.MeleeSkillCheck + attacker->itembonuses.MeleeSkillCheck;
	
	
	//does the hit chance cap apply to spell bonuses from disciplines?
	chancetohit += (attacker->spellbonuses.HitChance + attacker->itembonuses.HitChance) / 15;
	chancetohit -= (defender->spellbonuses.AvoidMeleeChance + defender->itembonuses.AvoidMeleeChance) / 10;
	
	// Chance to hit;   Max 95%, Min 30%
	//if chance to hit is crazy high, that means a discipline is in use, and let it stay there
	if(chancetohit < 9000)
		chancetohit = (chancetohit > 95) ? 95 : ( (chancetohit < 30) ? 30 : chancetohit );
	
	//I dont know the best way to handle a garunteed hit discipline being used
	//agains a garunteed riposte (for example) discipline... for now, garunteed hit wins
	
		
	#if EQDEBUG>=11
		LogFile->write(EQEMuLog::Debug, "3 FINAL calculated chance to hit is: %5.2f", chancetohit);
	#endif

	//
	// Did we hit?
	//

	float tohit_roll = MakeRandomFloat(0, 100);
	if (tohit_roll > chancetohit)
	{
#if ATTACK_DEBUG>=11
		LogFile->write(EQEMuLog::Debug, "AvoidDamage(%s) attacked by %s  - %s MISS %5.2f : %5.2f", defender->GetName(), attacker->GetName(), (attacker->IsClient()?"PC":"NPC"), tohit_roll, chancetohit);
#endif
	  return(false);
 	}
	
	return(true);
}

// solar: called when a mob is attacked, does the checks to see if it's a hit
// and does other mitigation checks.  'this' is the mob being attacked.
bool Mob::AvoidDamage(Mob* other, sint32 &damage)
{
	float skill;
	Mob *attacker=other;
	Mob *defender=this;
/*
	////////////////////////////////////////////////////////
	// Mitigation goes here
	////////////////////////////////////////////////////////
	if (damage > 1) {
	    if(this->IsClient()){
	    	if (damage > 1 && spell_id == 0xFFFF){
                double acMod = GetAC()/100;
                double acDam = (damage/100)*(acMod*1.5);
                damage = (sint32)((float)damage-acDam);
                if(damage <= 0)
					damage = 1;
			}
	    }
	    else {
	    	if (damage > 1 && spell_id == 0xFFFF){
				double acMod = GetAC()/100;
                double acDam = (damage/100)*acMod;
                damage = (sint32)((float)damage-acDam);
                if(damage <= 0)
					damage = 1;
			}
	    }
	}
*/	
	bool ghit = false;
	if((attacker->spellbonuses.MeleeSkillCheck + attacker->itembonuses.MeleeSkillCheck) > 500)
		ghit = true;
	
#if ATTACK_DEBUG>=15
		LogFile->write(EQEMuLog::Debug, "%s::AvoidDamage(%s) before disciplines damage=%i", GetName(), other->GetName(), damage);
#endif
	if(damage > 0) {
	//handle discipline mitigation and shielding
		sint16 mitigation = defender->spellbonuses.MeleeMitigation + defender->itembonuses.MeleeMitigation;
		if(mitigation > 0) {
			damage = damage * (100 - mitigation) / 100;
		}
		
		//handle damage increase diciplines + items
		int mod = attacker->spellbonuses.DamageModifier + attacker->itembonuses.DamageModifier;
		if(mod < -99)
			damage = 0;	//all absorbed, should this be legal?
		else
			damage = damage * (100 + mod) / 100;
	}
	
#if ATTACK_DEBUG>=15
		LogFile->write(EQEMuLog::Debug, "%s::AvoidDamage(%s) before skills damage=%i", GetName(), other->GetName(), damage);
#endif
	
	
	//////////////////////////////////////////////////////////
	// make enrage same as riposte
	/////////////////////////////////////////////////////////
	if (IsEnraged() && !other->BehindMob(this, other->GetX(), other->GetY()) )
		damage = -3;	
	
	/////////////////////////////////////////////////////////
	// riposte
	/////////////////////////////////////////////////////////
	if (damage > 0 && CanThisClassRiposte() && !other->BehindMob(this, other->GetX(), other->GetY()) )
	{
		if (IsClient()) {
        	skill = CastToClient()->GetSkill(RIPOSTE);
        	if (GetLevelCon(other->GetLevel()) != CON_GREEN)
				this->CastToClient()->CheckIncreaseSkill(RIPOSTE);
		}
		else {
        	skill = this->MaxSkill(RIPOSTE);
		}
		

#if ATTACK_DEBUG >= 11
			LogFile->write(EQEMuLog::Debug,"%s::AvoidDamage(%s) Riposte skill is : %i", GetName(), other->GetName(),(int)skill);
#endif
		
		float bonus = (defender->spellbonuses.RiposteChance + defender->itembonuses.RiposteChance) / 100.0f;
		if (!ghit) {	//if they are not using a garunteed hit discipline
			if(MakeRandomFloat(0,1) < (skill/4000.0 + bonus))
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
            && !other->BehindMob(this, other->GetX(), other->GetY()))
	{
		if (IsClient()) {
			skill = CastToClient()->GetSkill(BLOCKSKILL);
			if (GetLevelCon(other->GetLevel()) != CON_GREEN)
				this->CastToClient()->CheckIncreaseSkill(BLOCKSKILL);
		}
		else {
        	skill = this->MaxSkill(BLOCKSKILL);
		}
		
#if ATTACK_DEBUG >= 11
			LogFile->write(EQEMuLog::Debug,"%s::AvoidDamage(%s) Block skill is : %i", GetName(), other->GetName(), (int)skill);
#endif
		
		if (!ghit) {	//if they are not using a garunteed hit discipline
			if (MakeRandomFloat(0,1) < skill/3500.0)
				damage = -1;
		}
	}
	
	//////////////////////////////////////////////////////		
	// parry
	//////////////////////////////////////////////////////
	if (damage > 0 && CanThisClassParry() && !other->BehindMob(this, other->GetX(), other->GetY()) )
	{
        
		if (IsClient()) {
        		skill = CastToClient()->GetSkill(PARRY);
			if (GetLevelCon(other->GetLevel()) != CON_GREEN)
				this->CastToClient()->CheckIncreaseSkill(PARRY); 
		}
		else {
        		skill = this->MaxSkill(PARRY);
		}
		
#if ATTACK_DEBUG >= 11
			LogFile->write(EQEMuLog::Debug,"%s::AvoidDamage(%s) Parry skill is : %i", GetName(), other->GetName(), (int)skill);
#endif
		
		float bonus = (defender->spellbonuses.ParryChance + defender->itembonuses.ParryChance) / 100.0f;
		if (!ghit) {	//if they are not using a garunteed hit discipline
			if(MakeRandomFloat(0,1) < (skill/4000.0 + bonus))
				damage = -2;
		}
	}
	
	////////////////////////////////////////////////////////
	// dodge
	////////////////////////////////////////////////////////
	if (damage > 0 && CanThisClassDodge() && !other->BehindMob(this, other->GetX(), other->GetY()) )
	{
	
		if (IsClient()) {
        		skill = CastToClient()->GetSkill(DODGE);
			if (GetLevelCon(other->GetLevel()) != CON_GREEN)
				this->CastToClient()->CheckIncreaseSkill(DODGE);
		}
		else {
        		skill = this->MaxSkill(DODGE);

		}
		
#if ATTACK_DEBUG >= 11
			LogFile->write(EQEMuLog::Debug,"%s::AvoidDamage(%s) Dodge skill is : %i", GetName(), other->GetName(), (int)skill);
#endif
		
		float bonus = (defender->spellbonuses.DodgeChance + defender->itembonuses.DodgeChance) / 100.0f;
		if (!ghit) {	//if they are not using a garunteed hit discipline
			if(MakeRandomFloat(0,1) < (skill/4000.0 + bonus))
				damage = -4;
		}
	}
	
#if ATTACK_DEBUG>=15
		LogFile->write(EQEMuLog::Debug, "%s::AvoidDamage(%s) before AC damage=%i", GetName(), other->GetName(), damage);
#endif
	
	////////////////////////////////////////////////////////
// Scorpious2k: Include AC in the calculation

// use serverop variables to set values
	int myac = GetAC();
	if (myac!=0)
	{
		float acreduction=1;
		int acrandom=300;
		int acfail=1000;
		char tmp[10];
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

		if (database.GetVariable("ACfail", tmp, 9))
		{
			acfail = (int) (atof(tmp) * 100);
			if (acfail>100) acfail=100;
		}

		if (acfail<=0 || rand()%101>acfail)
		{
			if (damage>0)
			{
				if (acreduction>0)
				{
					damage -= (int) (GetAC()/(100.0f/acreduction));
				}		
				if (acrandom>0)
				{
					acrandom=rand()%acrandom;
					if (acrandom>0)
					{
						damage -= (myac * acrandom / 10000);
					}

				}
				if (damage<1) damage=1;
			}
		}
	}



#if ATTACK_DEBUG>=11
		LogFile->write(EQEMuLog::Debug, "%s::AvoidDamage(%s) out damage=%i", GetName(), other->GetName(), damage);
#endif

	if (damage < 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}


bool Client::Attack(Mob* other, int Hand, bool bRiposte)
{
	_ZP(Client_Attack);
	//SetAttackTimer();
	if (IsCasting() && GetClass() != BARD)
		return false; // Only bards can attack while casting
	if(!other)
		return false;
	if((IsClient() && CastToClient()->dead) || (other->IsClient() && other->CastToClient()->dead))
		return false;
	if(GetHP() < 0)
		return false;
	if(!IsAttackAllowed(other))
		return false;
	if(DivineAura() && !GetGM()) {//cant attack while invulnerable unless your a gm
		Message(10,"You can't attack while invulnerable!");
		return false;
	}

	int16 skillinuse = HIGHEST_SKILL+1;
	int min_hit = 0;
	int max_hit = 0;
	int weapon_damage = 0;
	int attack_skill=4;
	
	ItemInst* weapon = GetInv().GetItem(SLOT_PRIMARY);
	if (Hand == 14)	// Kaiyodo - Pick weapon from the attacking hand
		weapon = GetInv().GetItem(SLOT_SECONDARY);
	
	const Item_Struct *weapon_item = NULL;
	if(weapon != NULL)
		weapon_item = weapon->GetItem();
	
	// calculate attack_skill and skillinuse depending on hand and weapon
	// also send Packet to near clients
	AttackAnimation(attack_skill, skillinuse, Hand, weapon);
	/// Now figure out damage
	int damage = 0;
	
	int8 otherlevel = other->GetLevel();
	int8 mylevel = this->GetLevel();
	
	otherlevel = otherlevel ? otherlevel : 1;
	mylevel = mylevel ? mylevel : 1;
	
	// Determine players ability to hit based on:
	// mob level difference, skillinuse, randomness
	if (skillinuse == HIGHEST_SKILL+1) { // the fallthru, only do 1 damage
		damage = 1;
	}
	else
	{
		if ( damage >= 0 ) {
			CheckIncreaseSkill(skillinuse, -10);
			CheckIncreaseSkill(OFFENSE, -10);

			if(skillinuse == 28 || !weapon) // weapon is hand-to-hand
			{
				//dont some weapons use the hand to hand skill?
				if(GetClass() == MONK || GetClass() == BEASTLORD)
					weapon_damage = GetMonkHandToHandDamage();	// Damage changes based on level
				else
					weapon_damage = 2; // This isn't quite right, something more like level/10 is more appropriate
			}
			else //not hand to hand and we have a weapon
			{
				if (weapon->IsWeapon()) {
					weapon_damage = weapon->GetItem()->Common.Damage;
					if (weapon_damage < 1)
						weapon_damage = 1;
				} else {
					weapon_damage = 1;
				}
			}
			
			/*#if 0 // Racial bane damage
						if (weapon && weapon->Common.BaneDmgAmt && weapon->Common.BaneDmgRace && other && other->GetRace() == weapon->common.BaneDMGRace) {
							weapon_damage += weapon->common.BaneDMG;
						}
			#endif // Racial bane damage*/
			
			/*#if 0 // Body bane damage
						if (weapon && weapon->Common.BaneDmgAmt && weapon->Common.bBaneDMGBody && other && other->GetBodyType() == weapon->common.BaneDMGBody) {
							weapon_damage += weapon->common.BaneDMG;
						}
			#endif // Body bane damage*/
			
			min_hit = 1;
			max_hit = (int) (weapon_damage * (( ((float)GetSTR()*2) + (float)GetSkill(skillinuse)*1.5+ (float)mylevel) / 100));	// Apply damage formula
			/*#if 0 // Weighted MDF type damage
				int magic_number = 0;
				int weighted = 0;
				if (GetLevel() >= 25) {
					max_hit =  (int)(weapon_damage * (( ((float)GetSTR()) + (float)GetSkill(skillinuse)+ (float)mylevel) / 100));	// Apply damage formula
					min_hit = (GetLevel()-25)/3; // FIXME Brutal hack for Damage bonus this is here somewhere but
					if (Hand != 13)
						min_hit = 1;
					magic_number = 2* weapon_damage + (level-25)/3;
					weighted = (int)(0.9 * (weapon_damage+min_hit) + 0.1 * max_hit);
				}
			#endif // Weighted MDF type damage*/
			
			// Only apply the damage bonus to the main hand
			if(Hand == 13)	// Kaiyodo - If we're not using the DWDA stuff, will always be the primary hand
			{
				int damage_bonus = GetWeaponDamageBonus(weapon_item);	// Can be NULL, will then assume fists
				min_hit += damage_bonus;
				max_hit += damage_bonus;
			}
			
			min_hit = min_hit * (100 + itembonuses.MinDamageModifier + spellbonuses.MinDamageModifier) / 100;
		
			if(max_hit <= min_hit)
				damage = min_hit;
			else
				damage = (int32)min_hit + MakeRandomInt(0, max_hit - min_hit + 1);
			
			//this still isnt the right place for this...
			//also, is this damage supposed to be seperate??
			// Elemental damage
			if(weapon_item && weapon_item->Common.ElemDmg) {
				float resist = other->ResistSpell(weapon_item->Common.ElemDmgType, 0, this);
				if(resist > 0) {
					damage += (int)( weapon_item->Common.ElemDmg * resist / 100.0f);
				} //else: print message?
			}

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
		} // End (damage >= 0)

#if EQDEBUG>=11 
			LogFile->write(EQEMuLog::Debug,"Client::Attack(): min_hit:%i max_hit:%i weapon_damage:%i damage:%i mod:%f",
				min_hit, max_hit, weapon_damage, damage, (( ((float)GetSTR()*2) + (float)GetSkill(skillinuse)+ (float)mylevel) / 100) );
#endif

	} // End skill set?
	if (damage > 0) {
	
			//check to see if we hit..
			if(other) {
				if(!other->CheckHitChance(this, attack_skill, Hand, skillinuse))
					damage = 0;
				else	//we hit, try to avoid it
					other->AvoidDamage(this, damage);
			}
			
        	if (bRiposte && damage == -3)	//cannot riposte a riposte
                	return false;
        	if (damage <= 0) {
    			other->Damage(this, damage, 0xffff, attack_skill);
				//return false;
			}
		
		//////////////////////////////////////////////////////////
		/////////	Finishing Blow
		/////////////////////////////////////////////////////////
		//kathgar: Made it so players cannot be finishing blowed.. something wanky was going on
		//			Added level limits and fixed the chances to the correct values?
		uint16 aa_item = GetAA(aaFinishingBlow);
		if(damage > 0 && aa_item>0 && other->GetHPRatio() < 10 && (other->GetLevel()<=48) && !other->IsClient())	//Don't finishing blow players.. at least for now)
		{
			float tempchancerand = MakeRandomFloat(0, 100);
			if
			(
				(aa_item==1 && (tempchancerand<=2)&& other->GetLevel() <= 50) ||
				(aa_item==2 && (tempchancerand<=5)&& other->GetLevel() <= 52) ||
				(aa_item==3 && (tempchancerand<=7)&& other->GetLevel() <= 54)
			)
			{
			//should be we returnign true before we do anything?????
				return true;
				entity_list.MessageClose_StringID(this, false, 200, MT_CritMelee, FINISHING_BLOW, GetName());
				other->Damage(this,32000,0xffff,attack_skill);
			}
		}
		
		///////////////////////////////////////////////////
		/////   Critical Hits
		//////////////////////////////////////////////////
		float critChance = 0.0f;
		int critMod = 4;
		
		critChance += (spellbonuses.CriticalHitChance + itembonuses.CriticalHitChance) / 100.0f;
		//critChance += (mydex * 0.0001); // Factor in DEX
		
		if(GetClass() == WARRIOR && mylevel >= 12) {
			// Factor in AA Skill
			int amount = GetAA(aaCombatFury);
			switch( amount )
			{
			case 0:
				critChance += 0.03f;
				break;
			case 1:
				critChance += 0.05f;
				break;
			case 2:
				critChance += 0.07f;
				break;
			case 3:
				critChance += 0.10f;
				break;
			}

			// 3x base crit chance when berserk
			if( this->berserk )
				critChance += 0.06f;
			
			if(berserk)
				critMod = 10;	//crippling blow
			else
				critMod = 5;
			
		} else {	//non warrior crits
			int amount = GetAA(aaCombatFury);
			if( amount != 0 ) // Ensure they have the skill
			{

				// Factor in AA Skill
				int amount = GetAA(aaCombatFury);
				switch( amount )
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
				}
			} else {
				//not warrior and no AA, so no crits
				critChance = 0;
			}
		} //!warrior
		
		//if we can critical hit...
		if (damage > 0 && critChance > 0) {
			float critRand = MakeRandomFloat(0, 1);

			//see if we did in fact hit.
			if( critRand <= critChance )
			{
				int RAND_CRIT = rand()%critMod;
					
				uint8 item_dmg = 0;
				if (weapon_item && weapon->IsWeapon())
					item_dmg = weapon_item->Common.Damage;
				damage += ((( mylevel / 4) + item_dmg) * RAND_CRIT);
				if(damage < 0)
					damage = 0;
				
				if(other && (other->GetInvul() || other->DivineAura()))
					damage = -5;
				else if(berserk)
					entity_list.MessageClose(this, false, 200, 10, "%s lands a crippling blow!(%d)", name,damage);
				else
					entity_list.MessageClose(this, false, 200, 10, "%s scores a critical hit!(%d)", name,damage);
			}
		}
	}
	else { // Make sure damage math doesn't give us a negative
		damage = 0;
	}
	///////////////////////////////////////////////////////////
	//////    Send Attack Damage
	///////////////////////////////////////////////////////////
   	if (damage >= 1) {
		other->Damage(this, damage, 0xffff, attack_skill);
		if(spellbonuses.MeleeLifetap || itembonuses.MeleeLifetap) {
			//heal self for damage done..
			SetHP(GetHP() + damage);
		}
	}
	
	////////////////////////////////////////////////////////////
	////////  PROC CODE
	////////  Kaiyodo - Check for proc on weapon based on DEX
	///////////////////////////////////////////////////////////
	if(other && (other->GetHP() > -10)) {
		TryWeaponProc(weapon_item, other);
	}
   	if (damage <= 0) {
		return false;
	}
	if( other && other->IsNPC() && (other->IsEnraged() || damage == -3) && !BehindMob(other, GetX(), GetY()) )
	{
		//other->CastToNPC()->FaceTarget(); //Causes too much lag?? Disabled. -image
		other->Attack(this, 13, true);
        	return false;
	}
	
	if (damage > 0)
        return true;
	else
		return false;
}

// solar: this is only used by #heal
void Mob::Heal()
{
	SetMaxHP();
	SendHPUpdate();
	LogFile->write(EQEMuLog::Normal,"%s healed via #heal", name);
}

void NPC::Heal() {
	SetMaxHP();
	SendHPUpdate();
	LogFile->write(EQEMuLog::Normal,"%s healed via #heal", name);
}

void Client::Damage(Mob* other, sint32 damage, int16 spell_id, int8 attack_skill, bool avoidable, sint8 buffslot, bool iBuffTic)
{
	if(spell_id==0)
		spell_id = SPELL_UNKNOWN;
	adverrorinfo = 411;
#ifdef GUILDWARS
	if(other && other->IsClient() && other->CastToClient()->GuildDBID() != 0)
	{
		float percent = 0;
		if(spell_id != 0)
		{
			percent = location_list.GetCasterAttackBonus(other->CastToClient()->GuildDBID());
		}
		else
		{
			percent = location_list.GetMeleeAttackBonus(other->CastToClient()->GuildDBID());
		}
		if(percent != 0)
			damage += (damage*percent);
	}
#endif

	attack_flag = true;

    if(other && IsAIControlled()) {
        if (attack_skill == ARCHERY)
	        AddToHateList(other, 1, damage, iBuffTic); // almost no aggro for archery
        else if (spell_id != SPELL_UNKNOWN)
	        AddToHateList(other, damage / 2 , damage, iBuffTic); // half aggro for spells
        else
	        AddToHateList(other, damage, damage, iBuffTic);   // normal aggro for everything else
    }
    // if we got a pet, thats not already fighting something send it into battle
    Mob *pet = GetPet();
    if (other && pet && !pet->IsEngaged() && other->GetID() != GetID())
    {
        if (pet)
            pet->AddToHateList(other, 1, 0,true,false,iBuffTic);
        else
        {
            // todo: do whatever is necessary for clients as pets
        }
        pet->SetTarget(other);
		char mypetname[64]={0};
		strncpy(mypetname,pet->GetName(),strlen(pet->GetName())-2);
		char pettarget[64]={0};
		if(other->IsClient())
			strcpy(pettarget,other->GetName());
		else
			strncpy(pettarget,other->GetName(),strlen(other->GetName())-2);
		Message_StringID(10,PET_ATTACKING,mypetname,pettarget);
        //Message(10,"%s tells you, 'Attacking %s, Master.'", pet->GetName(), other->GetName());
    }
	
	if( spell_id != SPELL_UNKNOWN || other == NULL )
        avoidable = false;
	
    if (GetInvul() || DivineAura())
        damage=-5;
	
    // damage shield calls this function with spell_id set, so its unavoidable
	if (other && damage > 0 && spell_id == SPELL_UNKNOWN) {
		this->DamageShield(other);
	}

/*
    if ((spell_id != 0xFFFF || (attack_skill>200 && attack_skill<250)) && damage>0) {
			// todo: exchange that for EnvDamage-Packets when we know how to do it
			char val1[20]={0};
			Message_StringID(4,OTHER_HIT_NONMELEE,GetName(),ConvertArray(damage,val1));
			//Message(4,"%s was hit by non-melee for %d points of damage.", GetName(), damage);
    }
*/
	// if spell is lifetap add hp to the caster
	if (other && IsLifetapSpell( spell_id ))
	{
		int32 healedhp;

		// check if healing would be greater than max hp
		// use temp var to store actual healing value
		if ( other && other->GetHP() + damage > other->GetMaxHP())
		{
			healedhp = other->GetMaxHP() - other->GetHP();
			other->SetHP(other->GetMaxHP());
		}
		else
		{
			healedhp = damage;
			if(other != 0)
    			other->SetHP(other->GetHP() + damage);
		}


		// if client was casting the spell there need to be some messages
		if (other->IsClient())
		{
			other->CastToClient()->Message(4,"You have been healed for %d points of damage.", healedhp);
		}
			
		// emote goes with every one ... even npcs
		
		entity_list.MessageClose(this, true, 300, MT_Emote, "%s beams a smile at %s", other->GetName(), this->GetName() );
	}

	
	if (damage > 0) {
		//only if this is damage:
		
		if (GetRune() > 0)
			damage = ReduceDamage(damage, GetRune());
		if (sneaking){
			sneaking = false;
			// FIXME Break sneak packet goes here.
		}
		if ( (GetHP() - damage) <= -10){
			Death(other, damage, spell_id, attack_skill);
			return;
		}
		else
			SetHP(GetHP()-damage);
		
		//if the other is not green, and this is not a spell
		if (other && other->IsNPC() && GetLevelCon(other->GetLevel()) != CON_GREEN
			&& (spell_id == 0xFFFF || spell_id == 0))
			CheckIncreaseSkill(DEFENSE, -10);
	}


	APPLAYER* outapp = new APPLAYER(OP_Damage, sizeof(CombatDamage_Struct));
	
	//outapp->pBuffer = new uchar[outapp->size];
	//memset(outapp->pBuffer, 0, outapp->size);
	CombatDamage_Struct* a = (CombatDamage_Struct*)outapp->pBuffer;
	adverrorinfo = 412;
	a->target = GetID();
	
	if (other == 0)
		a->source = 0;
	else if (other->IsClient() && other->CastToClient()->GMHideMe())
		a->source = 0;
	else
		a->source = other->GetID();
		
	a->type = attack_skill;
	a->spellid = spell_id;
	a->damage = damage;

	if(damage>0){//this for for server side filters
		if(spell_id == SPELL_UNKNOWN || GetFilter(FILTER_SPELLDAMAGE)!=0) {
			if(iBuffTic) {	//send new HPs instead of damage packet
							//this lets you sit down while DOTed
				SendHPUpdate();
			} else {
				QueuePacket(outapp);
			}
		}
			
		if(spell_id == SPELL_UNKNOWN)
			entity_list.QueueCloseClients(this, outapp, true, 200, other,true,FILTER_OTHERHITS);
		else if(!iBuffTic)	//only send damage packet if not a DOT..
							//this lets you sit down while DOTed
			entity_list.QueueCloseClients(this, outapp, true, 200, other,true,FILTER_SPELLDAMAGE);
	}
	else if(damage==-5){//invulnerable sent regardless of filters
		entity_list.QueueCloseClients(this, outapp, false, 200, other);
	}
	else{
		QueuePacket(outapp,true,CLIENT_CONNECTINGALL,FILTER_ATKMISSESME);
		entity_list.QueueCloseClients(this, outapp, true, 200, other,true,FILTER_OTHERMISSES);
	}
	if (other && other->IsClient() && other!=this && !iBuffTic){
		//dont send to the player if they caused the damage, its already sent above
		//also, nobody should get DOT damage except the target
		if(damage>0 || damage==-5){
			if(damage>0 && spell_id != SPELL_UNKNOWN)
				other->CastToClient()->QueuePacket(outapp,true,CLIENT_CONNECTINGALL,FILTER_SPELLDAMAGE);
			else
				other->CastToClient()->QueuePacket(outapp); //my hits cant be filtered
		}
		else
			other->CastToClient()->QueuePacket(outapp,true,CLIENT_CONNECTINGALL,FILTER_MYMISSES);
	}
	
	safe_delete(outapp);
	
	adverrorinfo = 413;
	if(IsFullHP==false || cur_hp<max_hp)
		SendHPUpdate();
	if (damage != 0) {
		if (IsMezzed())
			this->BuffFadeByEffect(SE_Mez);
		if (attack_skill == BASH && GetLevel() < 56) {
			if((itembonuses.StunResist+spellbonuses.StunResist) <= 0 || rand()%101 >= (itembonuses.StunResist+spellbonuses.StunResist))
				Stun(0);
		}
		if (IsRooted() && spell_id != 0xFFFF) { // neotoyko: only spells cancel root
			if ((float)rand()/RAND_MAX > 0.8f)
				this->BuffFadeByEffect(SE_Root, buffslot); // buff slot is passed through so a root w/ dam doesnt cancel itself
		}
		if(this->casting_spell_id != 0 && spell_id == 0xFFFF) { //shouldnt interrupt on regular spell damage
			attacked_count++;
			isattacked = true;
		}
		
#if EQDEBUG >= 11
    LogFile->write(EQEMuLog::Debug," %s hit for %i, %i left", name, damage, GetHP());
#endif
		adverrorinfo = 41;
	}
}

void Client::Death(Mob* other, sint32 damage, int16 spell, int8 attack_skill)
{
	if(dead)
		return;	//cant die more than once...
	int exploss;

	LogFile->write(EQEMuLog::Debug, "%s::Death(%i)", GetName(), damage);
	LogFile->write(EQEMuLog::Normal,"Player %s has died", name);

	//
	// #1: Send death packet to everyone
	//

	if(!spell) spell = 0xffff;
	// make death packet
	APPLAYER app(OP_Death, sizeof(Death_Struct));
	Death_Struct* d = (Death_Struct*)app.pBuffer;
	d->spawn_id = GetID();
	d->killer_id = other ? other->GetID() : 0;
	d->unknown12 = 1;
	d->spell_id = spell == 0xffff ? 0xffffffff : spell;
	d->attack_skill = spell != 0xffff ? 0xe7 : attack_skill;
	d->damage = damage;
	app.priority = 6;
	entity_list.QueueClients(this, &app);


	//
	// #2: figure out things that affect the player dying and mark them dead
	//

	InterruptSpell();
	SetPet(0);
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


	//
	// #3: exp loss and corpse generation
	//

	// figure out if they should lose exp
	exploss = (int)(GetLevel() * (GetLevel() / 18.0) * 12000);

	if( (GetLevel() < 9) || IsBecomeNPC() )
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

	if(spell != 0xFFFF)
	{
		for(int buffIt = 0; buffIt < BUFF_COUNT; buffIt++)
		{
			if(buffs[buffIt].spellid == spell && buffs[buffIt].client)
			{
				exploss = 0;	// no exp loss for pvp dot
				buffIt = BUFF_COUNT;
			}
		}
	}
	
	// now we apply the exp loss, unmem their spells, and make a corpse
	// unless they're a GM (or less than lvl 10
	if(!GetGM() && m_pp.level > 9)
	{
		if(exploss)
			SetEXP(GetEXP() - exploss > 1 ? GetEXP() - exploss : 1, GetAAXP());

		BuffFadeAll();
		UnmemSpellAll(false);

		// check db variable 'leavecorpses'
		char tmp[20] = {0};
		database.GetVariable("leavecorpses", tmp, 20);
		int leavecorpses = atoi(tmp);
		if(leavecorpses)
		{
			// creating the corpse takes the cash/items off the player too
			Corpse *new_corpse = new Corpse(this, exploss);
			database.GetVariable("ServerType", tmp, 9);
			if(atoi(tmp)==1 && other->IsClient()){
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
		}

//		if(!IsLD())//Todo: make it so an LDed client leaves corpse if its enabled
//			MakeCorpse(exploss);
	}





#ifdef GUILDWARS
Mob* tempkiller = other;
if(other && other->GetOwner() != 0)
other = other->GetOwner();
	if(!IsBecomeNPC() && other != NULL && other->IsClient() && other != this && (!(IsDueling() && other->CastToClient()->GetDuelTarget() == GetID())))
	{
			if (!other->CastToClient()->isgrouped && entity_list.GetGroupByMob(other) == 0)
			{
			sint32 leveldiff = other->GetLevel() - GetLevel();
			sint32 points = guildwars.PlayerPointsEarned(other->CastToClient(),this->CastToClient());
			if(leveldiff > EXPLOSSLVLDIFF) // Experience loss
			{
				other->CastToClient()->SetEXP((int32)(other->CastToClient()->GetEXP() - other->GetLevel()*((float)other->GetLevel()/18)*3000 > 0)? (int32)(other->CastToClient()->GetEXP() - other->GetLevel()*((float)other->GetLevel()/18)*3000) : 1,other->CastToClient()->GetAAXP());
			points = 0;
			}
			else if(leveldiff > EXPHNOCHGLVLDIFF) // No experience
			points = 0;
			else
				other->CastToClient()->AddEXP(guildwars.CalculateEXPEarning(other->GetLevel(),GetLevel(),false,true));

				guildwars.DeathPointUpdate(other->CastToClient()->CharacterID(),CastToClient()->CharacterID());

				if(points > 0)
				{
				other->CastToClient()->UpdateLDoNPoints(points,0);

				other->CastToClient()->Message(0,"You received %i points killing %s",points,GetName());
				}
				else if(points == -1)
				other->CastToClient()->Message(0,"You have killed %s within the last 10 minutes, you receive no points.",GetName());
				else if(points == 0)
				other->CastToClient()->Message(0,"You received no points killing %s.",GetName());

				//TODO: Faction addition/subtraction
				/*Faction should get the top 3 and lowest 3
				Using the guildid of the person who died:
					Lose faction from this GuildID
					column1 = what you think of them
					column2 = what they think of you
					Losing faction would use column 1
					Gaining faction would use column 2
					Search through the faction list with this guildid as column2 searching for a guild that has the highest faction with this guild
					the killers guild loses faction to those guilds (max of 3)
					Search through the faction list with this guildid as column1 searching for a guild that has the lowest faction with this guild
					the killers guild gains faction to those guilds (max of 3)
				*/

			}
			else if(other->CastToClient()->isgrouped && entity_list.GetGroupByMob(other) != 0)
			{
				int32 highlvl = entity_list.GetGroupByMob(other->CastToClient())->GetHighestLevel();
				sint32 leveldiff = highlvl - GetLevel();

			if(leveldiff > EXPLOSSLVLDIFF) // Experience loss
			entity_list.GetGroupByMob(other)->CauseEXPLoss();
			else if(leveldiff > EXPHNOCHGLVLDIFF); // No experience
			else if(leveldiff < EXPLNOCHGLVLDIFF); // No Experience
			else
				entity_list.GetGroupByMob(other->CastToClient())->SplitExp(guildwars.CalculateEXPEarning(highlvl,GetLevel(),true,true), this);


				entity_list.GetGroupByMob(other->CastToClient())->GivePoints(this);

			}
	}
other = tempkiller;
#endif


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

	m_pp.zone_id = m_pp.bind_zone_id;
	database.MoveCharacterToZone(this->CharacterID(), database.GetZoneName(m_pp.zone_id));

	zonesummon_x = m_pp.bind_x[0];
	zonesummon_y = m_pp.bind_y[0];
	zonesummon_z = m_pp.bind_z[0];
	heading = 0;

	Save();
	
	if(isgrouped) {
		Group *g = GetGroup();
		if(g)
			g->MemberZoned(this);
	}
}

#if 0 // solar: old code
void Client::MakeCorpse(int32 exploss)
{
	if (this->GetID() == 0)
		return;
	
	if ( (!GetGM() && m_pp.level > 9) || IsBecomeNPC())
	{
		// Check to see if we are suppose to make a corpse
		// Via the database setting (anything in leavecorpses)
		char tmp[20];
		memset(tmp,0,sizeof(tmp));
		database.GetVariable("leavecorpses", tmp, 20);
		int leavecorpses = atoi(tmp);
		if (tmp2 >= 1) {
#if EQDEBUG >= 5
				LogFile->write(EQEMuLog::Debug,"Creating corpse for %s at x=%f y=%f z=%f h=%f", GetName(), GetX(), GetY(), GetZ(), GetHeading());
#endif
			entity_list.AddCorpse(new Corpse(this, &m_pp, exploss, tmp2), this->GetID());
			this->SetID(0);
		}
	}
}
#endif

bool NPC::Attack(Mob* other, int Hand, bool bRiposte)	 // Kaiyodo - base function has changed prototype, need to update overloaded version
{
	_ZP(NPC_Attack);
	int damage = 0;
	
	if (!other)
	{
    	SetTarget(NULL);
    	return false;
	}
	
	if (!target)
    	SetTarget(other);
	
	SetAttackTimer();
	float calcheading=CalculateHeadingToTarget(target->GetX(), target->GetY());
	if((calcheading)!=GetHeading()){
		SetHeading(calcheading);
		FaceTarget(target, true);
	}
	/*if(moved){
		SetHeading(this->GetHeading()*8);
		SendPosition();
		SetMoving(false);
		moved=false;
	}*/
	
	if (!IsAttackAllowed(other)) {
		if (this->GetOwnerID())
			entity_list.MessageClose(this, 1, 200, 10, "%s says, 'That is not a legal target master.'", this->GetName());
		if(other)
			RemoveFromHateList(other);
		return false;
	}
	else
	{
		int16 skillinuse;
		int attack_skill;
		
		const Item_Struct* weapon = NULL;
		if (Hand == 13 && equipment[7] > 0)
		    weapon = database.GetItem(equipment[7]);
		else if (equipment[8])
		    weapon = database.GetItem(equipment[8]);

		if (Hand == 14 && weapon && weapon->Common.ItemUse == ItemUseShield)
			return false; // <Rogean> Cant Dual Wield with Shields
		
		ItemCommonInst weapon_inst(weapon,0);
		AttackAnimation(attack_skill, skillinuse, Hand, &weapon_inst);
		
		int8 otherlevel = other->GetLevel();
		int8 mylevel = this->GetLevel();
		
		otherlevel = otherlevel ? otherlevel : 1;
		mylevel = mylevel ? mylevel : 1;

		float dmgbonusmod = 0;
		float clmod = (float)GetClassLevelFactor()/22;
		float basedamage;
		int AC_adjust=12;	// value to adjust default because of AC being added
		//float basedefend = 0;
		//float currenthit = 0;
		
		// set min_dmg max_dmg here based on level if they are not set already
		// FIXME database cache lookup fancy like stuff needed here
		float level_mod = 1.5f;
		if (mylevel >= 66) {
		    level_mod = 4.5f; // mod4
		    if (min_dmg==0)
		    min_dmg = 220;
		    if (max_dmg==0)
			max_dmg = (int16)((((220*level_mod)*(mylevel-64))/4.0f)*AC_adjust/10);
//			max_dmg = (int16)(((220*level_mod)*(mylevel-64))/4.0f);
			// 66 = 495, 67 = 742, 68 = 990, 69 = 1237, 70 = 1485
		}
		else if (mylevel >= 60 && mylevel <= 65){
		    level_mod = 4.25f;
		    if(min_dmg==0)
		    min_dmg = (mylevel+(mylevel/3));
		    if(max_dmg==0)
		    max_dmg = (mylevel*3)*AC_adjust/10;
//		    max_dmg = (mylevel*3);
		    // 60 = 180, 65 = 195
		}
		else if (mylevel >= 51 && mylevel <= 59){
		    level_mod = 3.75f;
		    if(min_dmg==0)
		    min_dmg = (mylevel+(mylevel/3));
		    // 51 = 68, 59 = 78
		    if(max_dmg==0)
		    max_dmg = (mylevel*3)*AC_adjust/10;
//		    max_dmg = (mylevel*3);
		    // 51 = 153, 59 = 177
		}
		else if (mylevel >= 40 && mylevel <= 50) {
			if (min_dmg==0)
				min_dmg = mylevel;
			if(max_dmg==0)
				max_dmg = (mylevel*3)*AC_adjust/10;
//				max_dmg = (mylevel*3);
		    // 40 = 120 , 50 = 150
		}
		else if (mylevel >= 28 && mylevel <= 39) {
		    if (min_dmg==0)
			min_dmg = mylevel / 2; // 14-17
		    if (max_dmg==0)
			max_dmg = ((mylevel*2)+2)*AC_adjust/10;
		    // 28 = 58, 39 = 80
		}
		else if (mylevel <= 27) {
		    if (min_dmg==0)
			min_dmg=1;
		    if (max_dmg==0)
			max_dmg = (mylevel*2)*AC_adjust/10;
		    // 1 = 2, 27 = 54
		}
		
		if(max_dmg != 0 && min_dmg <= max_dmg) {
			basedamage = RandomTimer(min_dmg,max_dmg)*((clmod >= 1.0f) ? clmod:1.0f);  // npc only get bonus for class no negatives
		}
		else if (other->GetOwnerID()!=0) {
			// FIXME Shouldn't nerf the damage of charmed pets
			basedamage = mylevel*1.9f*clmod;
		}
		else { // Default calculation
			basedamage = mylevel*level_mod*clmod;
		}
		
		dmgbonusmod += (float)(this->itembonuses.STR + this->spellbonuses.STR)/3;
		dmgbonusmod += (float)(this->spellbonuses.ATK + this->itembonuses.ATK)/5;
		basedamage += (float)basedamage/100*dmgbonusmod;
		
		damage = (int)basedamage;
		
		if(other->IsClient() && min_dmg != 0 && damage == min_dmg && dmgbonusmod > 0)
		    damage += (int)dmgbonusmod;
		if(min_dmg != 0 && damage < min_dmg)
		    damage = min_dmg;
		if(max_dmg != 0 && damage > max_dmg)
		    damage = max_dmg;
		
		//THIS IS WHERE WE CHECK TO SEE IF WE HIT:
		if (other) {
			if(!other->CheckHitChance(this, attack_skill, Hand, skillinuse))
				damage = 0;	//miss
			else	//hit, check for damage avoidance
				other->AvoidDamage(this, damage);
		}
		
		
		if(other->IsClient() && other->CastToClient()->IsSitting())
			damage = max_dmg;
		if(other->IsClient() && GetOwnerID()!=0 && GetOwner()->IsClient()) //pets do half damage to clients in pvp
			damage=damage/2;
		
		//cant riposte a riposte
		if (bRiposte && damage == -3)
		    return false;
		
		adverrorinfo = 1292;
		if(other != 0 && this != 0 && GetHP() > 0 && other->GetHP() >= -11){
#if EQDEBUG >= 11
				LogFile->write(EQEMuLog::Debug,"NPC::Attack() basedamage:%f basedefend:%f dmgbonusmod:%f clmod:%f currenthit:%f damage:%i", basedamage, basedefend, dmgbonusmod, clmod, currenthit, damage);
#endif
		    other->Damage(this, damage, 0xffff, attack_skill, false); // Not avoidable client already had thier chance to Avoid
	    }
		adverrorinfo = 1293;
	}
	if (!target) return true; //We killed them
	// Kaiyodo - Check for proc on weapon based on DEX
	if( other && other->GetHP() > 0 ) {
		TryWeaponProc(NULL, other);	//no weapon
	}
	
	// now check ripostes
	if (other && damage == -3) // riposting
	{
	    other->Attack(this, 13, true);
    	// todo: double riposte
		return false;
	}
	
	if (damage > 0)
        return true;
	else
        return false;
}

void NPC::Damage(Mob* other, sint32 damage, int16 spell_id, int8 attack_skill, bool avoidable, sint8 buffslot, bool iBuffTic) {
    // now add done damage to the hate list
    if (!other)
      return;
	if (attack_event == 0)
	{
		parse->Event(EVENT_ATTACK, this->GetNPCTypeID(), 0, this, other);
	}
	attack_event = 1;
	attacked_timer.Start(12000,true);

#ifdef GUILDWARS
	if(other->IsClient() && other->CastToClient()->GuildDBID() != 0)
	{
		float percent = 0;
		if(spell_id != 0)
		{
			percent = location_list.GetCasterAttackBonus(other->CastToClient()->GuildDBID());
		}
		else
		{
			percent = location_list.GetMeleeAttackBonus(other->CastToClient()->GuildDBID());
		}
		if(percent != 0)
			damage += (damage*percent);
	}
#endif

	if(other) {
        if (attack_skill == ARCHERY)
	        AddToHateList(other, 1, damage,true,false,iBuffTic); // almost no aggro for archery
        else if (spell_id != SPELL_UNKNOWN)
	        AddToHateList(other, damage / 2 , damage,true,false, iBuffTic); // half aggro for spells
        else
	        AddToHateList(other, damage, damage,true,false, iBuffTic);   // normal aggro for everything else
    }
    
	if(SpecAttacks[IMMUNE_MEELE]) {
		damage = -5;
	}

    // only apply DS if physical damage (no spell damage)
    if (other && damage > 0 && spell_id == 0xFFFF) {
		this->DamageShield(other);
	}
	if ((spell_id != SPELL_UNKNOWN || (attack_skill>200 && attack_skill<250)) && damage>0)
    {
        // todo: exchange that for EnvDamage-Packets when we know how to do it
		char val1[20]={0};
		if (other && other->IsClient())
			other->Message_StringID(4,OTHER_HIT_NONMELEE,GetName(),ConvertArray(damage,val1));
			//other->CastToClient()->Message(4,"%s was hit by non-melee for %d points of damage.", this->GetName(), damage);
    }

	// if spell is lifetap add hp to the caster
	if (other && IsLifetapSpell( spell_id ))
	{
		int32 healedhp;

		// check if healing would be greater than max hp
		// use temp var to store actual healing value
		if ( other && other->GetHP() + damage > other->GetMaxHP())
		{
			healedhp = other->GetMaxHP() - other->GetHP();
			other->SetHP(other->GetMaxHP());
		}
		else
		{
			healedhp = damage;
			if(other != 0)
    			other->SetHP(other->GetHP() + damage);
		}


		// if client was casting the spell there need to be some messages
		if (other->IsClient())
		{
			other->CastToClient()->Message(4,"You have been healed for %d points of damage.", healedhp);
		}
			
		// emote goes with every one ... even npcs
		
		entity_list.MessageClose(this, true, 300, MT_Emote, "%s beams a smile at %s", other->GetName(), this->GetName() );
	}

    if (damage > 0 && GetRune() > 0)
	{
		damage = ReduceDamage(damage, GetRune());
	}
	//unsigned char test[sizeof(Action_Struct)];
	//memset(test,0x0,sizeof(Action_Struct));
	if (damage >= GetHP())
	{
		SetHP(-100);
		Death(other, damage, spell_id, attack_skill);
		return;
	}
	APPLAYER* outapp = new APPLAYER(OP_Damage, sizeof(CombatDamage_Struct));
	//outapp->pBuffer = new uchar[outapp->size];
	//memset(outapp->pBuffer, 0, sizeof(CombatDamage_Struct));
	CombatDamage_Struct* a = (CombatDamage_Struct*)outapp->pBuffer;
	adverrorinfo = 412;
	a->target = GetID();
	if (other == 0)
		a->source = 0;
	else if (other->IsClient() && other->CastToClient()->GMHideMe())
		a->source = 0;
	else
		a->source = other->GetID();
		
    a->type = attack_skill; // was 0x1c
    if (attack_skill == 231)
   		a->spellid = 0xFFFF;
    else
   		a->spellid = spell_id;
	a->damage = damage;		
	outapp->priority = 5;
	if (other && other->GetOwnerID()) { // let pet owners see their pet's damage
		Mob* owner = other->GetOwner();
		if (owner && owner->IsClient()){
			if(damage>0){
				if(spell_id!=0xFFFF)
					owner->CastToClient()->QueuePacket(outapp,true,CLIENT_CONNECTINGALL,FILTER_SPELLDAMAGE);
				else
					owner->CastToClient()->QueuePacket(outapp,true,CLIENT_CONNECTINGALL,FILTER_MYPETHITS);
			}
			else if(damage==-5)//cant filter invulnerable
				owner->CastToClient()->QueuePacket(outapp);
			else
				owner->CastToClient()->QueuePacket(outapp,true,CLIENT_CONNECTINGALL,FILTER_MYPETMISSES);
		}
		entity_list.QueueCloseClients(owner, outapp, true, 200, other);//make owner send the packet so we can skip them
	}
	else{//not a pet
		if(damage>0){
			if(spell_id!=0xFFFF)
				entity_list.QueueCloseClients(this, outapp, false, 200, other,true,FILTER_SPELLDAMAGE);
			else
				entity_list.QueueCloseClients(this, outapp, false, 200, other,true,FILTER_OTHERHITS);
		}
		else if(damage==-5)//invulnerable, cant be filtered
			entity_list.QueueCloseClients(this, outapp, false, 200, other);
		else
			entity_list.QueueCloseClients(this, outapp, false, 200, other,true,FILTER_OTHERMISSES);
	}

	if (other && other->IsClient()){
		if(damage>0 || damage==-5) //cant be filtered
			other->CastToClient()->QueuePacket(outapp);
		else
			other->CastToClient()->QueuePacket(outapp,true,CLIENT_CONNECTINGALL,FILTER_MYMISSES);
	}
	safe_delete(outapp);
	
	
		
	SetHP(GetHP() - damage);

	if (!IsEngaged())
        zone->AddAggroMob();
    if (other && damage > 0)
        AddRampage(other);
			
	if (damage > 0)
	{
		if (IsMezzed()) {

			this->BuffFadeByEffect(SE_Mez);

		}
		if (attack_skill == BASH && GetLevel() < 56) {
			if((itembonuses.StunResist+spellbonuses.StunResist) <= 0 || rand()%101 >= (itembonuses.StunResist+spellbonuses.StunResist))
				Stun(0);
		}
		if (IsRooted() && spell_id != 0xFFFF) // neotoyko: only spells cancel root
		{
			if ((float)rand()/RAND_MAX > 0.8f)
				this->BuffFadeByEffect(SE_Root, buffslot);
		}
		if(this->casting_spell_id != 0)
		{
			attacked_count++;
			isattacked = true;
		}
	}
	if(!IsFullHP || cur_hp<max_hp)
		other->SendHPUpdate();
}

void NPC::Death(Mob* other, sint32 damage, int16 spell, int8 attack_skill)
{
	
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
	
	APPLAYER* app= new APPLAYER(OP_Death,sizeof(Death_Struct));
	Death_Struct* d = (Death_Struct*)app->pBuffer;
	d->spawn_id = GetID();
	d->killer_id = other ? other->GetID() : 0;
	d->unknown12 = 1;
	d->spell_id = spell == 0xffff ? 0xffffffff : spell;
	d->attack_skill = spell != 0xffff ? 0xe7 : attack_skill;
	d->damage = damage;
	app->priority = 6;
	entity_list.QueueClients(other, app, true);

	if(this->respawn2 && (this->respawn2->spawn2_id!=0 && this->respawn2->respawn_!=0)){
	    database.UpdateTimeleft(this->CastToNPC()->respawn2->spawn2_id,this->CastToNPC()->respawn2->respawn_);
	}

	if (other)
	{
		if (other->IsClient())
			other->CastToClient()->QueuePacket(app);
		hate_list.Add(other, damage);
	}

	safe_delete(app);

#ifdef GUILDWARS

	Client* guildmember = 0;
	if(other && other->IsClient())
	guildmember = other->CastToClient();
	if(other->IsNPC() && other->GetOwner() != 0 && other->GetOwner()->IsClient())
	guildmember = other->GetOwner()->CastToClient();
	if(guildmember != 0 && guildmember->IsClient() && guildmember->CastToClient()->GuildDBID() != 0 && CastToNPC()->GetGuildLocationID() != 0 && CastToNPC()->respawn2 != 0)
	{
		GuildLocation* loc = location_list.FindLocationByID(CastToNPC()->GetGuildLocationID());
		if(loc != 0)
		{
			GuildNPCs* current = loc->FindGuildNPCBySpawnID(CastToNPC()->respawn2->GetID());
			if(current != 0)
			{
				current->KilledByGuildID(other->CastToClient()->GuildDBID());
				current->SetDead();
				loc->SetGuardsKilled(loc->GetGuardsKilled()+1);
			}
		}
	}
	else if(CastToNPC()->GetGuildLocationID() != 0)
	{
		GuildLocation* loc = location_list.FindLocationByID(CastToNPC()->GetGuildLocationID());
		if(loc != 0)
		{
			GuildNPCs* current = loc->FindGuildNPCBySpawnID(CastToNPC()->respawn2->GetID());
			if(current != 0)
			{
				current->KilledByGuildID(1);
				current->SetDead();
				loc->SetGuardsKilled(loc->GetGuardsKilled()+1);
			}
		}
	}
#endif
	
	
	Mob *give_exp = hate_list.GetDamageTop(this);
	if(give_exp == NULL)
		give_exp = killer;
	if(give_exp && give_exp->GetOwner() != 0)
		give_exp = give_exp->GetOwner();
	
#ifndef RAIDADDICTS // If we aren't Raid Addicts
#ifdef IPC
	if (give_exp && (give_exp->IsClient() || give_exp->CastToNPC()->IsInteractive()) && !IsCorpse() )
#else
    if (give_exp && (give_exp->IsClient() && !IsCorpse() ) && MerchantType == 0)
#endif
	{
		Group *kg = entity_list.GetGroupByClient(give_exp->CastToClient());
		if (give_exp->CastToClient()->isgrouped && kg != NULL)
		{
#ifdef GUILDWARS
			level = kg->GetHighestLevel();
			kg->SplitExp(guildwars.CalculateEXPEarning(level, GetLevel(), true), this);
#else
			if(give_exp->CastToClient()->GetAdventureID()>0){
				AdventureInfo AF=database.GetAdventureInfo(give_exp->CastToClient()->GetAdventureID());
				if(zone->GetZoneID() == AF.zonedungeonid && AF.type==ADVENTURE_MASSKILL)
					give_exp->CastToClient()->SendAdventureUpdate();
				else if(zone->GetZoneID() == AF.zonedungeonid && AF.type==ADVENTURE_NAMED && (AF.Objetive==GetNPCTypeID() || AF.ObjetiveValue==GetNPCTypeID()))
					give_exp->CastToClient()->SendAdventureFinish(1,AF.points,true);
			}
			kg->SplitExp((level*level*75*35/10), this);
#endif
		}
		else
        {
        	int conlevel = give_exp->GetLevelCon(GetLevel());
            if (conlevel != CON_GREEN)
            {
#ifdef GUILDWARS
			    give_exp->CastToClient()->AddEXP(guildwars.CalculateEXPEarning(give_exp->GetLevel(),GetLevel())); // Pyro: Comment this if NPC death crashes zone
#else
			    give_exp->CastToClient()->AddEXP((level*level*75*35/10), conlevel); // Pyro: Comment this if NPC death crashes zone
#endif
            }
		}
		hate_list.DoFactionHits(GetNPCFactionID());
	}

#else // IF We are Raid Addicts
	hate_list.DoFactionHits(GetNPCFactionID());
	raidaddicts.NPCDeath(this, give_exp);

	/* Old 3.0 Raid Addicts Code
	if (give_exp->GetLevelCon(GetLevel()) != CON_GREEN && MerchantType == 0) { // Check if Mob is not Green
		if (give_exp->CastToClient()->isgrouped && entity_list.GetGroupByClient(give_exp->CastToClient()) != 0) { // Check if Player is Grouped.
			entity_list.GetGroupByClient(give_exp->CastToClient())->RASplitPoints(GetNPCTypeID());
		} else {
			if (!raidaddicts.NPCDeathProcess(GetNPCTypeID(), give_exp)) {
				give_exp->CastToClient()->Message(13,"Point System Error");
			}
		}
	} else { give_exp->CastToClient()->Message(15,"You do not recieve points from a Green Creature."); }
	*/
#endif // End Raid Addicts
	
	if (respawn2 != 0) {
		respawn2->Reset();
	}
	
	if (class_ != 32 && this->ownerid == 0 && this->flag[3]!=3 && CastToNPC()->MerchantType == 0 && killer && (killer->IsClient() || (killer->GetOwner() != 0 && killer->GetOwner()->IsClient())) ) {
		Corpse* corpse = new Corpse(this, &itemlist, GetNPCTypeID(), &NPCTypedata);
		entity_list.AddCorpse(corpse, this->GetID());
		this->SetID(0);
		if(killer->GetOwner() != 0 && killer->GetOwner()->IsClient())
			killer = killer->GetOwner();
		if(killer != 0 && killer->IsClient()) {
			corpse->AllowMobLoot(killer, 0);
			if(killer->CastToClient()->isgrouped) {
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
	if(other && other->GetOwner())
		parse->Event(EVENT_DEATH, this->GetNPCTypeID(),0, this, other->GetOwner());
	else
		parse->Event(EVENT_DEATH, this->GetNPCTypeID(),0, this, other);
	this->WhipeHateList();
	p_depop = true;
	if(other) other->SetTarget(0);

}

// solar: (notes to self) hmm this is called in a few spots..
// CurrentHP and HealOverTime from bufftics, CurrentHP in SpellEffect,
// and DS/DSRev from DamageShield
bool Mob::ChangeHP(Mob* other, sint32 amount, int16 spell_id, sint8 buffslot, bool iBuffTic)
{
	//dont bother if nothing changed...
	if(amount == 0)
		return(true);
	
	if (IsCorpse())
		return false;

	if (amount < 0)
	{
		// cut all PVP spell damage to 2/3 -solar
		if(other && IsClient() && other->IsClient() && this != other)
			amount = amount * 67 / 100;
		
		if(amount < 0)
			amount = 0 - amount;
		
		Damage(other, amount, spell_id, 231, false, buffslot, iBuffTic);
		return true;
	}
	else
	{
		int curhp=GetHP(),maxhp=GetMaxHP();
		if (curhp!=maxhp) {
			if ((curhp+amount)>maxhp)
				curhp=maxhp;
			else
				curhp+=amount;
			SetHP(curhp);

			SendHPUpdate();
		}
	}

	return false;
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
	if (type == saFlyingKick) {
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
	else if (type == saTigerClaw) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (4 * ackwardtest) * (GetSkill(TIGER_CLAW) + GetSTR() + level) / 700);
		if(other->IsClient())
			ndamage = ndamage * 7 / 10;
		if(candamage)
			other->Damage(this, ndamage, 0xffff, 0x34);
		DoAnim(animTigerClaw);
		reuse = TigerClawReuseTime * 1000;
	}
	else if (type == saRoundKick) {
		ndamage = (sint32) (((level/10) + hitmodifier) * (6 * ackwardtest) * (GetSkill(ROUND_KICK) + GetSTR() + level) / 600);
		if(other->IsClient())
			ndamage = ndamage * 9 / 10;
		if(candamage)
			other->Damage(this, ndamage, 0xffff, 0x26);
		DoAnim(animRoundKick);
		reuse = RoundKickReuseTime * 1000;
	}
	else if (type == saEagleStrike) {
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
	else if (type == saTailRake) {
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
	else if (type == saKick) {
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
    if (other == myowner)
			return;
	if (owner) { // Other has a pet, add him and it
			hate_list.Add(other, hate, 0, bFrenzy, !iBuffTic);
			hate_list.Add(owner, 1, damage, false, !iBuffTic);
	}
	else { // Other has no pet, add other
			hate_list.Add(other, hate, damage, false, !iBuffTic);
	}
	if (mypet) { // I have a pet, add other to it
			mypet->hate_list.Add(other, 1, 0, bFrenzy);
	}
    else if (myowner) { // I am a pet, add other to owner if it's NPC/LD
            if (myowner->IsAIControlled())
                myowner->hate_list.Add(other, 1, 0, bFrenzy);
    }
 if (!wasengaged) { 
	if(IsNPC())
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
void Mob::DamageShield(Mob* other)
{
	int DS = 0;
//	int DSRev = 0;
	int16 spellid = 0xFFFF;
	int effect_value, dmg, i, z;

	for (i=0; i < BUFF_COUNT; i++)
	{
		if (buffs[i].spellid != SPELL_UNKNOWN)
		{
			for (z=0; z < EFFECT_COUNT; z++)
			{
				if(IsBlankSpellEffect(buffs[i].spellid, z))
					continue;

				effect_value = CalcSpellEffectValue(buffs[i].spellid, z, buffs[i].casterlevel);

				switch(spells[buffs[i].spellid].effectid[z])
				{
					case SE_DamageShield:
					{
						dmg = effect_value;
						DS += dmg;
						spellid = buffs[i].spellid;
						break;
					}
/*
					case SE_ReverseDS:
					{
						dmg = effect_value;
						DSRev += dmg;
						spellid = buffs[i].spellid;
						break;
					}
*/
				}
			}
		}
	}
	// there's somewhat of an issue here that the last (slot wise) damage shield
	// will be the one whose spellid is set here
	if (DS)
	{
		other->ChangeHP(this, DS, spellid);
		// todo: send EnvDamage packet to the other
		//entity_list.MessageClose(other, 0, 200, 10, "%s takes %d damage from %s's damage shield (%s)", other->GetName(), -spells[buffs[i].spellid].base[1], this->GetName(), spells[buffs[i].spellid].name);
	}
/*
	if (DSRev)
	{
		this->ChangeHP(other, DSRev, spellid);
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
	
	// If we have no weapon, or only a single handed weapon, just return the default
	// damage bonus of (Level - 25) / 3
	ItemCommonInst inst(Weapon);
	if (!inst.IsType(ItemTypeCommon))
		return BasicBonus;
	
	const ItemCommon_Struct& common = Weapon->Common;
	if ((common.ItemUse == ItemUse1HS) || (common.ItemUse == ItemUsePierce) || (common.ItemUse == ItemUse1HB))
		return BasicBonus;
	
	// Things get more complicated with 2 handers, the bonus is based on the delay of
	// the weapon as well as a number stored inside the weapon.
	int WeaponBonus = 0;	// How do you find this out?
	
	// Data for this, again, from www.monkly-business.com
	if (common.Delay <= 27)
		return (WeaponBonus + BasicBonus + 1);
	if (common.Delay <= 39)
		return (WeaponBonus + BasicBonus + ((GetLevel()-27) / 4));
	if (common.Delay <= 42)
		return (WeaponBonus + BasicBonus + ((GetLevel()-27) / 4) + 1);
	// Weapon must be > 42 delay
	return (WeaponBonus + BasicBonus + ((GetLevel()-27) / 4) + ((common.Delay-34) / 3));
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

//heko: backstab
void Mob::RogueBackstab(Mob* other, const ItemInst* weapon, int8 bs_skill)
{
	int ndamage = 0;
	int max_hit, min_hit;
	float skillmodifier = 0.0;
	int8 primaryweapondamage;
	if (weapon && weapon->IsType(ItemTypeCommon))
		primaryweapondamage = weapon->GetItem()->Common.Damage; //backstab uses primary weapon
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
	ndamage = (int)min_hit + (rand()%((max_hit-min_hit)+1));	// TODO: better formula, consider mob level vs player level, strength/atk
	if (!BehindMob(other, GetX(), GetY()))
		ndamage = min_hit;
	other->Damage(this, ndamage, 0xffff, BACKSTAB);
	DoAnim(animPiercing);	//piercing animation
}

// solar - assassinate
void Mob::RogueAssassinate(Mob* other)
{
	other->Damage(this, 32000, 0xffff, BACKSTAB);
	DoAnim(animPiercing);	//piercing animation
}

// neotokyo 14-Nov-02


sint16 Mob::ReduceMagicalDamage(sint16 damage, int16 in_rune)

{
	if (in_rune >= abs(damage))
	{
		in_rune -= abs(damage);
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

sint16 Mob::ReduceDamage(sint16 damage, int16 in_rune)
{
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

bool Mob::HasProcs()
{
    for (int i = 0; i < MAX_PROCS; i++)
        if (PermaProcs[i].spellID != 0xFFFF || SpellProcs[i].spellID != 0xFFFF)
            return true;
    return false;
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

void Mob::Taunt(NPC* who, bool always_succeed) {
	if (who == NULL)
		return;
	
	if (!always_succeed && IsClient())
		CastToClient()->CheckIncreaseSkill(TAUNT);
	
	int level = GetLevel();
	
	// Check to see if we're already at the top of the target's hate list
	if ((target->GetHateTop() != this) && (target->GetLevel() < level)) {
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
			newhate = who->CastToNPC()->GetNPCHate(who->CastToNPC()->GetHateTop()) - who->CastToNPC()->GetNPCHate(this) + tauntvalue;
			// add the hate
			who->CastToNPC()->AddToHateList(this, newhate);
		}
	}
}

bool Client::CheckDoubleAttack(bool AAadd, bool Triple) {
	//i dont know if this double riposte stuff belongs in here...
	int skill = 0;
	if (Triple)
	{
		if ((GetClass() != MONK || !CanUseSkill(DOUBLE_ATTACK)) && (GetClass() != BEASTLORD || !GetAA(aaDoubleRiposte)))
		{
			return false;
		}
		if (GetClass() == MONK)
		{
			skill = GetSkill(DOUBLE_ATTACK)/2;
		}
		else if (GetClass() == BEASTLORD)
		{
			switch (GetAA(aaDoubleRiposte))
			{
				case 1:
					skill = 20;
					break;
				case 2:
					skill = 40;
					break;
				case 3:
					skill = 80;
					break;
				default:
					return false;
			}
		}
	}
	else
	{
		
		//should these stack with skill, or does that ever even happen?
		int aaskill = GetAA(aaBestialFrenzy)*25 + GetAA(aaHarmoniousAttack)*25;
		if (!aaskill && !CanUseSkill(DOUBLE_ATTACK))
		{
			return false;
		}
		skill = GetSkill(DOUBLE_ATTACK);
		if (aaskill > skill) {
			skill = aaskill;
		}
		if (AAadd && !aaskill && GetClass() != BEASTLORD)
		{
			switch (GetAA(aaDoubleRiposte))
			{
				case 1:
					skill += 25;
					break;
				case 2:
					skill += 50;
					break;
				case 3:
					return true;
			}
		}
		//discipline effects
		skill += (spellbonuses.DoubleAttackChance + itembonuses.DoubleAttackChance) * 3;
		
		if(skill < 300)	//only gain if we arnt garunteed
			CheckIncreaseSkill(DOUBLE_ATTACK);
	}
	if(rand()%300 < skill)
	{
		return true;
	}
	return false;
}

