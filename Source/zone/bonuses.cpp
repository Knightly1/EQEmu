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
#include <stdlib.h>
#include "../common/unix.h"
#endif

#include "StringIDs.h"

void Mob::CalcBonuses()
{	
	CalcSpellBonuses(&spellbonuses);
	
	CalcMaxHP();
	CalcMaxMana();
	
	rooted = FindType(SE_Root);
}

void Client::CalcBonuses()
{
	_ZP(Client_CalcBonuses);
	memset(&itembonuses, 0, sizeof(StatBonuses));
	CalcItemBonuses(&itembonuses);
	CalcEdibleBonuses(&itembonuses);
	
	RecalcWeight();
	
	CalcSpellBonuses(&spellbonuses);
	
	CalcMaxHP();
	CalcMaxMana();
	
	CalcAC();
	CalcATK();
	CalcHaste();
	
	CalcSTR();
	CalcSTA();
	CalcDEX();
	CalcAGI();
	CalcINT();
	CalcWIS();
	CalcCHA();
	
    CalcMR();
	CalcFR();
	CalcDR();
	CalcPR();
	CalcCR();
	
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
	//memset assumed to be done by caller.
	
	int i;
	for (i=0; i<21; i++) {
		const ItemInst* inst = m_inv[i];
		if(inst == 0)
			continue;
		AddItemBonuses(inst, newbon);
	}
	
	//tribute items
	for (i = 400; i < 404; i++) {
		const ItemInst* inst = m_inv[i];
		if(inst == 0)
			continue;
		AddItemBonuses(inst, newbon);
	}
	
	//caps
	if(newbon->ManaRegen > eqManaRegenItemCap)
		newbon->ManaRegen = eqManaRegenItemCap;
	if(newbon->HPRegen > eqHPRegenItemCap)
		newbon->HPRegen = eqHPRegenItemCap;
	
	
	SetAttackTimer();
}
		
void Client::AddItemBonuses(const ItemInst *inst, StatBonuses* newbon) {
	if(!inst || !inst->IsType(ItemTypeCommon))
		return;
	const Item_Struct *item = inst->GetItem();
	const ItemCommon_Struct& common = item->Common;
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
		newbon->MeleeMitigation += common.Shielding;
	}
	if(common.StunResist > 0) {
		newbon->StunResist += common.StunResist;
	}
	if(common.StrikeThrough > 0) {
		newbon->StrikeThrough += common.StrikeThrough;
	}
	if(common.Avoidance > 0) {
		newbon->AvoidMeleeChance += common.Avoidance;
	}
	if(common.Accuracy > 0) {
		newbon->HitChance += common.Accuracy;
	}
	if(common.CombatEffects > 0) {
		newbon->ProcChance += common.CombatEffects;
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
	
	if (common.SkillModValue != 0 && common.SkillModType < HIGHEST_SKILL){
		if (newbon->skillmod[common.SkillModType] < common.SkillModValue)
			newbon->skillmod[common.SkillModType] = (sint8)common.SkillModValue;
	}

	const ItemCommonInst *ci=(const ItemCommonInst *)inst;
	for(int i=0;i<5;i++) {
		AddItemBonuses(ci->GetAugment(i),newbon);
	}
}

void Client::CalcEdibleBonuses(StatBonuses* newbon) {
#if EQDEBUG >= 11
    cout<<"Client::CalcEdibleBonuses(StatBonuses* newbon)"<<endl;
#endif
  // Search player slots for skill=14(food) and skill=15(drink)
  	uint32 i;
  	
	bool food = false;
	bool drink = false;
	for (i = 22; i <= 29; i++)
	{
		if (food && drink)
			break;
		const ItemInst* inst = GetInv().GetItem(i);
		if (inst && inst->GetItem() && inst->IsType(ItemTypeCommon)) {
			const ItemCommon_Struct& common = inst->GetItem()->Common;
			if (common.ItemUse == ItemUseFood && !food)
				food = true;
			else if (common.ItemUse == ItemUseDrink && !drink)
				drink = true;
			else
				continue;
			AddItemBonuses(inst, newbon);
		}
	}
	for (i = 251; i <= 330; i++)
	{
		if (food && drink)
			break;
		const ItemInst* inst = GetInv().GetItem(i);
		if (inst && inst->GetItem() && inst->IsType(ItemTypeCommon)) {
			const ItemCommon_Struct& common = inst->GetItem()->Common;
			if (common.ItemUse == ItemUseFood && !food)
				food = true;
			else if (common.ItemUse == ItemUseDrink && !drink)
				drink = true;
			else
				continue;
			AddItemBonuses(inst, newbon);
		}
	}
}

void Mob::CalcSpellBonuses(StatBonuses* newbon)
{
	int i;

	memset(newbon, 0, sizeof(StatBonuses));
	newbon->AggroRange = -1;
	newbon->AssistRange = -1;

	for(i = 0; i < BUFF_COUNT; i++)
	{
		ApplySpellsBonuses(buffs[i].spellid, buffs[i].casterlevel, newbon);
	}
	
	//this prolly suffer from roundoff error slightly...
	newbon->AC = newbon->AC * 10 / 34;	//ratio determined impirically from client.
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
					newbon->AggroRange == -1 ||
					effect_value < newbon->AggroRange
				)
				{
					newbon->AggroRange = effect_value;
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
				if (spells[spell_id].base[i] != 0) {
					newbon->CHA += effect_value;
				}
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
				if(effect_value > newbon->singingMod)
					newbon->singingMod = effect_value;
				break;
			}
			
			case SE_ChangeAggro:
			{
				newbon->hatemod = effect_value-100;
				break;
			}
			case SE_MeleeMitigation:
			{
				//for some reason... this value is negative for increased mitigation
				newbon->MeleeMitigation -= effect_value;
				break;
			}
			
			/*
				Assuming that none of these chances stack... they just pick the highest
				
				perhaps some smarter logic is needed here to handle the case
				where there is a chance increase and a decrease
				because right now, the increase will completely offset the decrease...
				
			*/
			case SE_CriticalHitChance:
			{
				if(newbon->CriticalHitChance < effect_value)
					newbon->CriticalHitChance = effect_value;
				break;
			}
				
			case SE_CrippBlowChance:
			{
				if(newbon->CrippBlowChance < effect_value)
					newbon->CrippBlowChance = effect_value;
				break;
			}
				
			case SE_AvoidMeleeChance:
			{
				//multiplier is to be compatible with item effects
				//watching for overflow too
				effect_value = effect_value<3000? effect_value * 10 : 30000;
				if(newbon->AvoidMeleeChance < effect_value)
					newbon->AvoidMeleeChance = effect_value;
				break;
			}
				
			case SE_RiposteChance:
			{
				if(newbon->RiposteChance < effect_value)
					newbon->RiposteChance = effect_value;
				break;
			}
				
			case SE_DodgeChance:
			{
				if(newbon->DodgeChance < effect_value)
					newbon->DodgeChance = effect_value;
				break;
			}
				
			case SE_ParryChance:
			{
				if(newbon->ParryChance < effect_value)
					newbon->ParryChance = effect_value;
				break;
			}
				
			case SE_DualWeildChance:
			{
				if(newbon->DualWeildChance < effect_value)
					newbon->DualWeildChance = effect_value;
				break;
			}
				
			case SE_DoubleAttackChance:
			{
				if(newbon->DoubleAttackChance < effect_value)
					newbon->DoubleAttackChance = effect_value;
				break;
			}
				
			case SE_MeleeLifetap:
			{
				newbon->MeleeLifetap = true;
				break;
			}
				
			case SE_AllInstrunmentMod:
			{
				if(effect_value > newbon->singingMod)
					newbon->singingMod = effect_value;
				if(effect_value > newbon->brassMod)
					newbon->brassMod = effect_value;
				if(effect_value > newbon->percussionMod)
					newbon->percussionMod = effect_value;
				if(effect_value > newbon->windMod)
					newbon->windMod = effect_value;
				if(effect_value > newbon->stringedMod)
					newbon->stringedMod = effect_value;
				break;
			}
				
			case SE_ResistSpellChance:
			{
				if(newbon->ResistSpellChance < effect_value)
					newbon->ResistSpellChance = effect_value;
				break;
			}
				
			case SE_ResistFearChance:
			{
				if(newbon->ResistFearChance < effect_value)
					newbon->ResistFearChance = effect_value;
				break;
			}
				
			case SE_HundredHands:
			{
				newbon->HundredHands = true;
				break;
			}
				
			case SE_MeleeSkillCheck:
			{
				if(newbon->MeleeSkillCheck < effect_value)
					newbon->MeleeSkillCheck = effect_value;
				break;
			}
				
			case SE_HitChance:
			{
				//multiplier is to be compatible with item effects
				//watching for overflow too
				effect_value = effect_value<2000? effect_value * 15 : 30000;
				if(newbon->HitChance < effect_value) {
					newbon->HitChance = effect_value;
					newbon->HitChanceSkill = spells[spell_id].base2[i]==-1?255:spells[spell_id].base2[i];
				}
				break;
			}
				
			case SE_DamageModifier:
			{
				if(newbon->DamageModifier < effect_value)
					newbon->DamageModifier = effect_value;
				break;
			}
				
			case SE_MinDamageModifier:
			{
				if(newbon->MinDamageModifier < effect_value)
					newbon->MinDamageModifier = effect_value;
				break;
			}
				
			case SE_StunResist:
			{
				if(newbon->StunResist < effect_value)
					newbon->StunResist = effect_value;
				break;
			}
				
			case SE_ProcChance:
			{
				//multiplier is to be compatible with item effects
				//watching for overflow too
				effect_value = effect_value<3000? effect_value * 10 : 30000;
				if(newbon->ProcChance < effect_value)
					newbon->ProcChance = effect_value;
				break;
			}
				
			case SE_ExtraAttackChance:
			{
				if(newbon->ExtraAttackChance < effect_value)
					newbon->ExtraAttackChance = effect_value;
				break;
			}
				
		}
	}
}

