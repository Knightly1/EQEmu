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
#ifndef SPDAT_H
#define SPDAT_H

#include "../common/classes.h"
#include "mob.h"

#define SPELL_UNKNOWN 0xFFFF
#define SPELLBOOK_UNKNOWN 0xFFFFFFFF		//player profile spells are 32 bit

//some spell IDs which will prolly change, but are needed
#define SPELL_LEECH_TOUCH 6378
#define SPELL_LAY_ON_HANDS 87
#define SPELL_HARM_TOUCH 88
#define SPELL_HARM_TOUCH2 2821
#define SPELL_NPC_HARM_TOUCH 929


//#define SPDAT_SIZE		1824000
/* 
   solar: look at your spells_en.txt and find the id of the last spell.
   this number has to be 1 more than that.  if it's higher, your zone will
   NOT start up.  gonna autodetect this later..
*/
#define NEW_LoadSPDat

#define EFFECT_COUNT 12

enum RESISTTYPE
{
	RESIST_NONE = 0,
	RESIST_MAGIC = 1,
	RESIST_FIRE = 2,
	RESIST_COLD = 3,
	RESIST_POISON = 4,
	RESIST_DISEASE = 5,
	RESIST_CHROMATIC = 6,
	RESIST_PRISMATIC = 7,
	RESIST_PHYSICAL = 8	// see Muscle Shock, Back Swing
};

//This stuff is outdated and already in bodytypes.h

/*enum {	//body types
	bodyTypePerson = 1,	//dont know the right name for this...
	bodyTypeUndead = 3,
	bodyTypeSummoned = 8,	//this might be wrong... this is an older value
	bodyTypeAnimal = 21,
	bodyTypeFramiliar = 24,	//an NPC's framiliar had this type, dunno exact
	bodyTypeSwarmPet = 63
};
*/

//Target Type IDs
#define ST_TargetOptional	0x01 // Target is used if present, but not required. ex: Flare, Fireworks
#define ST_AECaster			0x04 // ae centered around caster
#define ST_Target			0x05 // single targetted
#define ST_Self				0x06 // self only
#define ST_AETarget			0x08 // ae around target
#define ST_AEBard			0x28 // ae friendly around self (ae bard song)
#define ST_Group			0x29 // group spell
#define ST_GroupTeleport	0x03
//#define ST_AlterPlane		0x3
#define ST_Undead			0x0a
#define ST_Tap				0x0d
#define ST_Pet				0x0e
#define ST_Animal           0x09
#define ST_Plant            0x10
#define ST_Dragon           0x12
#define ST_Giant            0x11
//#define ST_Unknown1			0x14
//#define ST_Unknown			0x18
//#define ST_Summoned			0x19
#define ST_Summoned			0x0b // NEOTOKYO: see spells_en.txt -> seems to be value 11 not 25
#define ST_Corpse			0x0f
#define ST_UndeadAE			0x18


//Spell Effect IDs
#define SE_CurrentHP				0	// Heals and nukes, repeates every tic if in a buff
#define SE_ArmorClass				1
#define SE_ATK						2
#define SE_MovementSpeed			3	// SoW, SoC, etc
#define SE_STR						4
#define SE_DEX						5
#define SE_AGI						6
#define SE_STA						7
#define SE_INT						8
#define SE_WIS						9
#define SE_CHA						10	// Often used as a spacer, who knows why
#define SE_AttackSpeed				11
#define SE_Invisibility				12
#define SE_SeeInvis					13
#define SE_WaterBreathing			14
#define SE_CurrentMana				15
#define SE_Lull						18	// see SE_Harmony
#define SE_AddFaction				19	// Alliance line
#define SE_Blind					20
#define SE_Stun						21
#define SE_Charm					22
#define SE_Fear						23
#define SE_Stamina					24	// Invigor and such
#define SE_BindAffinity				25
#define SE_Gate						26	// Gate to bind point
#define SE_CancelMagic				27
#define SE_InvisVsUndead			28
#define SE_InvisVsAnimals			29
#define SE_ChangeFrenzyRad			30
#define SE_Mez						31
#define SE_SummonItem				32
#define SE_SummonPet				33
#define SE_DiseaseCounter			35
#define SE_PoisonCounter			36
#define SE_DivineAura				40
#define SE_Destroy					41	// Disintegrate, Banishment of Shadows
#define SE_ShadowStep				42
#define SE_Lycanthropy				44
#define SE_ResistFire				46
#define SE_ResistCold				47
#define SE_ResistPoison				48
#define SE_ResistDisease			49
#define SE_ResistMagic				50
#define SE_SenseDead				52
#define SE_SenseSummoned			53
#define SE_SenseAnimals				54
#define SE_Rune						55
#define SE_TrueNorth				56
#define SE_Levitate					57
#define SE_Illusion					58
#define SE_DamageShield				59
#define SE_Identify					61
#define SE_WipeHateList				63
#define SE_SpinTarget				64
#define SE_InfraVision				65
#define SE_UltraVision				66
#define SE_EyeOfZomm				67
#define SE_ReclaimPet				68
#define SE_TotalHP					69
#define SE_NecPet					71
#define SE_BindSight				73
#define SE_FeignDeath				74
#define SE_VoiceGraft				75
#define SE_Sentinel					76
#define SE_LocateCorpse				77
#define SE_AbsorbMagicAtt			78	// rune for spells
#define SE_CurrentHPOnce			79	// Heals and nukes, non-repeating if in a buff
#define SE_Revive					81
#define SE_TestSpells				82
#define SE_Teleport					83
#define SE_TossUp					84	// Gravity Flux
#define SE_WeaponProc				85	// i.e. Call of Fire
#define SE_Harmony					86	// what is SE_Lull??
#define SE_MagnifyVision			87	// Telescope
#define SE_Succor					88	// Evacuate/Succor lines?
#define SE_ModelSize				89	// Shrink, Growth
#define SE_Cloak					90	// some kind of focus effect?
#define SE_SummonCorpse				91
#define SE_Calm						92	// Hate modifier. Enrageing blow
#define SE_StopRain					93	// Wake of Karana
#define SE_NegateIfCombat			94	// Component of Spirit of Scale
#define SE_Sacrifice				95
#define SE_Silence					96	// Cacophony
#define SE_ManaPool					97
#define SE_AttackSpeed2				98	// Melody of Ervaj
#define SE_Root						99
#define SE_HealOverTime				100
#define SE_CompleteHeal				101
#define SE_Fearless					102	// Valiant Companion
#define SE_CallPet					103	// Summon Companion
#define SE_Translocate				104
#define SE_AntiGate					105	// Translocational Anchor
#define SE_SummonBSTPet				106	// neotokyo: added BST pet support
#define SE_Familiar					108
#define SE_SummonItem2				109	// Summon Jewelry Bag - summons stuff into container
#define SE_ResistAll				111
#define SE_CastingLevel				112
#define	SE_SummonHorse				113
#define SE_ChangeAggro				114	// chanter spells Horrifying Visage ...
#define SE_Hunger					115	// Song of Sustenance
#define SE_CurseCounter				116
#define SE_MagicWeapon				117	// Magical Monologue
#define SE_SingingSkill				118	// Amplification
#define SE_AttackSpeed3				119	// Frenzied Burnout
#define SE_HealRate					120	// Packmaster's Curse - not sure what this is
#define SE_ReverseDS				121
#define SE_Screech					123	// Form of Defense
#define SE_ImprovedDamage			124
#define SE_ImprovedHeal				125
#define SE_IncreaseSpellHaste		127
#define SE_IncreaseSpellDuration	128
#define SE_IncreaseRange			129
#define SE_ReduceSpellHate			130
#define SE_ReduceReagentCost		131
#define SE_ReduceManaCost			132
#define SE_LimitMaxLevel			134
#define SE_LimitResist				135
#define SE_LimitTarget				136
#define SE_LimitEffect				137
#define SE_LimitSpellType			138
#define SE_LimitSpell				139
#define SE_LimitMinDur				140
#define SE_LimitInstant				141
#define SE_LimitMinLevel			142
#define SE_LimitCastTime			143
#define SE_Teleport2				145	// Banishment of the Pantheon
#define SE_PercentalHeal			147
// solar: 
// base is the effectid the command applies to
// max is the value to check against.. not sure if there's a formula for this
// calc is the effect slot number plus 200
#define SE_StackingCommand_Block	148
#define SE_StackingCommand_Overwrite 149
#define SE_DeathSave				150
#define SE_TemporaryPets			152	// Swarm of Fear III
#define SE_BalanceHP				153	// Divine Arbitration
#define SE_DispelDetrimental		154
#define SE_IllusionCopy				156	// Deception
#define SE_SpellDamageShield		157	// Petrad's Protection
#define SE_Reflect					158
#define SE_AllStats					159	// Aura of Destruction
#define SE_MeleeMitigation			168
#define SE_CriticalHitChance		169
#define SE_CrippBlowChance			171
#define SE_AvoidMeleeChance			172
#define SE_RiposteChance			173
#define SE_DodgeChance				174
#define SE_ParryChance				175
#define SE_DualWeildChance			176
#define SE_DoubleAttackChance		177
#define SE_MeleeLifetap				178
#define SE_AllInstrunmentMod		179
#define SE_ResistSpellChance		180
#define SE_ResistFearChance			181
#define SE_HundredHands				182
#define SE_MeleeSkillCheck			183
#define SE_HitChance				184
#define SE_DamageModifier			185
#define SE_MinDamageModifier		186
#define SE_FadingMemories			194
#define SE_StunResist				195
#define SE_ProcChance				200
#define SE_RangedProc				201	//not implemented
#define SE_Rampage					205
#define SE_AETaunt					206
#define SE_ReduceSkillTimer			227	//not implemented
#define SE_Blank					254
#define SE_ExtraAttackChance		266 //not implemented
#define SE_WakeTheDead				299
#define SE_Doppelganger				300
#define SE_NoCombatSkills			311
#define SE_DefensiveProc			323	//not implemented
#define SE_CriticalDamageMob		330	//not implemented
//silentfist + rogue backstab one, how does it tell??? skill #?

#define DF_Permanent		50

//
// solar: note this struct is historical, we don't actually need it to be
// aligned to anything, but for maintaining it it is kept in the order that
// the fields in the text file are.  the numbering is not offset, but field
// number.  note that the id field is counted as 0, this way the numbers
// here match the numbers given to sep in the loading function net.cpp
//
struct SPDat_Spell_Struct
{
/* 000 */	int			id;	// not used
/* 001 */	char		name[32]; // Name of the spell
/* 002 */	char		player_1[32]; // "PLAYER_1"
/* 003 */	char		teleport_zone[32];	// Teleport zone, or item summoned
/* 004 */	char		you_cast[64]; // Message when you cast
/* 005 */	char		other_casts[64]; // Message when other casts
/* 006 */	char		cast_on_you[64]; // Message when spell is cast on you 
/* 007 */	char		cast_on_other[64]; // Message when spell is cast on someone else
/* 008 */	char		spell_fades[64]; // Spell fades
/* 009 */	float		range;
/* 010 */	float		aoerange;
/* 011 */	float		pushback;
/* 012 */	float		pushup;
/* 013 */	int32		cast_time; // Cast time
/* 014 */	int32		recovery_time; // Recovery time
/* 015 */	int32		recast_time; // Recast same spell time
/* 016 */	int32		buffdurationformula;
/* 017 */	int32		buffduration;
/* 018 */	int32		AEDuration;	// sentinel, rain of something
/* 019 */	int16		mana; // Mana Used
/* 020 */	sint16		base[EFFECT_COUNT];
/* 032 */	int			unknown[12];
/* 044 */	sint16		max[EFFECT_COUNT];
/* 056 */	int16		icon; // Spell icon
/* 057 */	int16		memicon; // Icon on membarthing
/* 058 */	sint16		components[4]; // reagents
/* 062 */	int			component_counts[4]; // amount of regents used
/* 066 */	signed		NoexpendReagent[4];	// focus items (Need but not used; Flame Lick has a Fire Beetle Eye focus.)
											// If it is a number between 1-4 it means components[number] is a focus and not to expend it
											// If it is a valid itemid it means this item is a focus as well
/* 070 */	int16		formula[EFFECT_COUNT]; // Spell's value formula
/* 082 */	int			LightType; // probaly another effecttype flag
/* 083 */	int			goodEffect; // 1= very good ;) 2 = Translocate etc unknown4[1]
/* 084 */	int			Activated; // probaly another effecttype flag	
/* 085 */	int			resisttype;
/* 086 */	int			effectid[EFFECT_COUNT];	// Spell's effects
/* 098 */	int			targettype;	// Spell's Target
/* 099 */	int			basediff; // base difficulty fizzle adjustment
/* 100 */	int			skill;
/* 101 */	sint16		zonetype;
/* 102 */	int16		EnvironmentType;
/* 103 */	int			TimeOfDay;
/* 104 */	int8		classes[PLAYER_CLASS_COUNT]; // Classes
/* 120 */	int8		CastingAnim;
/* 121 */	int8		TargetAnim;
/* 122 */	int32		TravelType;
/* 123 */	int16		SpellAffectIndex;
/* 124 */	int16		Spacing2[23];
/* 147 */	sint16		ResistDiff;
/* 148 */	int16		Spacing3[2];
/* 150 */	int16		RecourseLink;
/* 151 */	int			Spacing4[4];
/* 155 */	int			descnum; // eqstr of description of spell
/* 156 */	int			typedescnum; // eqstr of type description
/* 157 */	int			effectdescnum; // eqstr of effect description
/* 158 */	int			Spacing5[17];		
/* 175 */	// last field is 174
};

#ifdef NEW_LoadSPDat
	extern const SPDat_Spell_Struct* spells; 
	extern sint32 SPDAT_RECORDS;
#else
	#define SPDAT_RECORDS	3602
#endif

bool IsSacrificeSpell(int16 spell_id);
bool IsLifetapSpell(int16 spell_id);
bool IsMezSpell(int16 spell_id);
bool IsStunSpell(int16 spell_id);
bool IsSlowSpell(int16 spell_id);
bool IsHasteSpell(int16 spell_id);
bool IsPercentalHealSpell(int16 spell_id);
bool IsGroupOnlySpell(int16 spell_id);
bool IsBeneficialSpell(int16 spell_id);
bool IsDetrimentalSpell(int16 spell_id);
bool IsInvulnerabilitySpell(int16 spell_id);
bool IsCHDurationSpell(int16 spell_id);
bool IsPoisonCounterSpell(int16 spell_id);
bool IsDiseaseCounterSpell(int16 spell_id);
bool IsSummonItemSpell(int16 spell_id);
bool IsSummonSkeletonSpell(int16 spell_id);
bool IsSummonPetSpell(int16 spell_id);
bool IsCharmSpell(int16 spell_id);
bool IsBlindSpell(int16 spell_id);
bool IsEffectHitpointsSpell(int16 spell_id);
bool IsReduceCastTimeSpell(int16 spell_id);
bool IsIncreaseDurationSpell(int16 spell_id);
bool IsReduceManaSpell(int16 spell_id);
bool IsExtRangeSpell(int16 spell_id);
bool IsImprovedHealingSpell(int16 spell_id);
bool IsImprovedDamageSpell(int16 spell_id);
bool IsAEDurationSpell(int16 spell_id);
bool IsPureNukeSpell(int16 spell_id);
bool IsPartialCapableSpell(int16 spell_id);
bool IsResistableSpell(int16 spell_id);
bool IsGroupSpell(int16 spell_id);
bool IsTGBCompatibleSpell(int16 spell_id);
bool IsBardSong(int16 spell_id);
bool IsEffectInSpell(int16 spellid, int effect);
bool IsBlankSpellEffect(int16 spellid, int effect_index);
bool IsValidSpell(int16 spellid);
bool IsSummonSpell(int16 spellid);
bool IsEvacSpell(int16 spellid);
bool IsDamageSpell(int16 spellid);
bool IsFearSpell(int16 spellid);
bool BeneficialSpell(int16 spell_id);
bool GroupOnlySpell(int16 spell_id);
bool NoMerchantSpell(int16 spell_id);
int GetSpellEffectIndex(int16 spell_id, int effect);
int CanUseSpell(int16 spellid, int classa, int level);

int CalcBuffDuration(Mob *caster, Mob *target, int16 spell_id);
int CalcBuffDuration_formula(int level, int formula, int duration);


int CalcPetHp(int levelb, int classb, int STA = 75);
char *GetRandPetName();

#endif
