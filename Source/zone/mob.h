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
#ifndef MOB_H
#define MOB_H

#include "features.h"

/* solar: macros for IsAttackAllowed, IsBeneficialAllowed */
#define _CLIENT(x) (x && x->IsClient() && !x->CastToClient()->IsBecomeNPC())
#define _NPC(x) (x && x->IsNPC() && !x->CastToMob()->GetOwnerID())
#define _BECOMENPC(x) (x && x->IsClient() && x->CastToClient()->IsBecomeNPC())
#define _CLIENTCORPSE(x) (x && x->IsCorpse() && x->CastToCorpse()->IsPlayerCorpse() && !x->CastToCorpse()->IsBecomeNPCCorpse())
#define _NPCCORPSE(x) (x && x->IsCorpse() && (x->CastToCorpse()->IsNPCCorpse() || x->CastToCorpse()->IsBecomeNPCCorpse()))
#define _CLIENTPET(x) (x && x->CastToMob()->GetOwner() && x->CastToMob()->GetOwner()->IsClient())
#define _NPCPET(x) (x && x->IsNPC() && x->CastToMob()->GetOwner() && x->CastToMob()->GetOwner()->IsNPC())
#define _BECOMENPCPET(x) (x && x->CastToMob()->GetOwner() && x->CastToMob()->GetOwner()->IsClient() && x->CastToMob()->GetOwner()->CastToClient()->IsBecomeNPC())

//Spell specialization parameters, not sure of a better place for them
#define SPECIALIZE_FIZZLE 11		//% fizzle chance reduce at 200 specialized
#define SPECIALIZE_MANA_REDUCE 12	//% mana cost reduction at 200 specialized

#define MAX_SHIELDERS 2		//I dont know if this is based on a client limit

#define APPEAR_VISIBLE	0x0003
#define APPEAR_ANIM		0x000e
#define APPEAR_SNEAK	0x000f
#define	APPEAR_AFK		0x0018
#define	APPEAR_ANON		0x0015
#define	APPEAR_SPLIT	0x001c
#define APPEAR_HEIGHT	0x001d
#define	APPEAR_HP_TIC	0x0011

#define CON_GREEN		2
#define CON_LIGHTBLUE	18
#define CON_BLUE		4
#define CON_WHITE		20
#define CON_YELLOW		15
#define CON_RED			13

//LOS Parameters:
#define HEAD_POSITION 0.9f	//ratio of GetSize() where NPCs see from
#define SEE_POSITION 0.5f	//ratio of GetSize() where NPCs try to see for LOS
#define CHECK_LOS_STEP 1.0f

 
#include "entity.h"
#include "spdat.h"
#include "event_codes.h"
#include "hate_list.h"
#include "../common/Kaiyodo-LList.h"
#include "../common/skills.h"
#include "../common/bodytypes.h"
#include "map.h"

#define SPELL_ATTACK_SKILL 231

enum FindSpellType {
	SPELLTYPE_SELF,
	SPELLTYPE_OFFENSIVE,
	SPELLTYPE_OTHER
};

enum {
	SPECATK_NONE = 0,
	SPECATK_SUMMON,		//S
	SPECATK_ENRAGE,		//E
	SPECATK_RAMPAGE,	//R
	SPECATK_FLURRY,		//F
	SPECATK_TRIPLE,		//T
	SPECATK_QUAD,		//Q
	UNSLOWABLE,			//U
	UNMEZABLE,			//M
	UNCHARMABLE,		//C
	UNSTUNABLE,			//N
	UNSNAREABLE,		//I
	UNFEARABLE,			//D
	IMMUNE_MEELE,		//A
	IMMUNE_MAGIC,		//B
	IMMUNE_FLEEING,		//f
	SPECATK_MAXNUM
				//X,Y,Z are old interactive NPC codes
};

typedef enum {	//fear states
	fearStateNotFeared = 0,
	fearStateRunning,		//I am running, hoping to find a grid at my WP
	fearStateRunningForever,	//can run straight until spell ends
	fearStateGrid,			//I am allready on a fear grid
	fearStateStuck			//I cannot move somehow...
} FearState;

struct TradeEntity;
class Trade;
enum TradeState {
	TradeNone,
	Trading,
	TradeAccepted,
	TradeCompleting
};

struct Buffs_Struct {
	int32	spellid;
	int8		casterlevel;
	int16	casterid;		// Maybe change this to a pointer sometime, but gotta make sure it's 0'd when it no longer points to anything
	int8		durationformula;
	sint32		ticsremaining;
	int8	poisoncounters;
	int8	diseasecounters;
	bool	client;  //True if the caster is a client
};

struct StatBonuses {
	sint16	AC;
	sint32	HP;
	sint32	HPRegen;
	sint32	ManaRegen;
	sint16	Mana;
	sint16	ATK;
	sint16	STR;
	sint16	STA;
	sint16	DEX;
	sint16	AGI;
	sint16	INT;
	sint16	WIS;
	sint16	CHA;
	sint16	MR;
	sint16	FR;
	sint16	CR;
	sint16	PR;
	sint16	DR;
	int		DamageShield; // this is damage done to mobs that attack this
	int		SpellDamageShield;
	int		ReverseDamageShield; // this is damage done to the mob when it attacks
	int		movementspeed;
	sint8		haste;
	float	AggroRange; // when calculate just replace original value with this
	float	AssistRange;
	int8	skillmod[HIGHEST_SKILL];
	int		effective_casting_level;
	int		reflect_chance;	// chance to reflect incoming spell
	int16	singingMod;
	int16	brassMod;
	int16	percussionMod;
	int16	windMod;
	int16	stringedMod;
	sint8	hatemod;

	//PoP effects:
	sint16   StrikeThrough;          // PoP: Strike Through %
//	sint16   CombatEffects; //AFAIK: Combat Effects == ProcChance
//	sint16   Shielding;     //AFAIK, Shielding == MeleeMitigation
//	sint16   Avoidance;		//AFAIK: Avoidance == AvoidMeleeChance
//	sint16   Accuracy;      //AFAIK: Accuracy == HitChance
	
	
	//discipline and PoP effects
	//everything is a straight percent increase unless noted.
	sint16 MeleeMitigation;	//i
	sint16 CriticalHitChance;	//i
	sint16 CrippBlowChance;
	sint16 AvoidMeleeChance;	//AvoidMeleeChance/10 == % chance i
	sint16 RiposteChance;	//i
	sint16 DodgeChance;		//i
	sint16 ParryChance;		//i
	sint16 DualWeildChance;		//i
	sint16 DoubleAttackChance;	//i
	sint16 ResistSpellChance;	//i
	sint16 ResistFearChance;	//i
	sint16 StunResist;		//i
	sint16 MeleeSkillCheck;	//i
	uint8  MeleeSkillCheckSkill;
	sint16 HitChance;			//HitChance/15 == % increase i
	uint8  HitChanceSkill;
	sint16 DamageModifier;		//needs to be thought about more and implemented
	uint8  DamageModifierSkill;
	sint16 MinDamageModifier;   //i
	sint16 ProcChance;			// ProcChance/10 == % increase i
	sint16 ExtraAttackChance;
	
	bool HundredHands;		//extra haste, stacks with all other haste  i
	bool MeleeLifetap;  //i
};

typedef struct
{
    int16 spellID;
    int8 chance;
    Timer *pTimer;
} tProc;

struct Shielders_Struct {
	int32   shielder_id;
	int16   shielder_bonus;
};

//eventually turn this into a typedef and 
//make DoAnim take it instead of int, to enforce its use.
enum {	//type arguments to DoAnim
	animKick				= 1,
	animPiercing			= 2,	//might be piercing?
	anim2HSlashing			= 3,
	anim2HWeapon			= 4,
	anim1HWeapon			= 5,
	animDualWeild			= 6,
	animTailRake			= 7,	//slam & Dpunch too
	animHand2Hand			= 8,
	animShootBow			= 9,
	animRoundKick			= 11,
	animSwarmAttack			= 20,	//dunno about this one..
	animFlyingKick			= 45,
	animTigerClaw			= 46,
	animEagleStrike			= 47,
	
};

class EGNode;
class MobFearState;

#define MAX_AISPELLS 16
class Mob : public Entity
{
public:
bool logpos;
	enum CLIENT_CONN_STATUS { CLIENT_CONNECTING, CLIENT_CONNECTED, CLIENT_LINKDEAD,
                          CLIENT_KICKED, DISCONNECTED, CLIENT_ERROR, CLIENT_CONNECTINGALL };
	enum eStandingPetOrder { SPO_Follow, SPO_Sit, SPO_Guard };
	struct AISpells_Struct {
		int16	type;			// 0 = never, must be one (and only one) of the defined values
		sint16	spellid;		// <= 0 = no spell
		sint16	manacost;		// -1 = use spdat, -2 = no cast time
		int32	time_cancast;	// when we can cast this spell next
		sint32	recast_delay;
		sint16	priority;
	};
	bool	IsFullHP;
	bool	moved;
	float	tarx;
	float	tary;
	float	tarz;
	int8	tar_ndx;
	float	tar_vector;
	float	tar_vx;
	float	tar_vy;
	float	tar_vz;
	float	test_vector;

	int32	GetPRange(float x, float y, float z);
	static	int32	RandomTimer(int min, int max);
	static	int8	GetDefaultGender(int16 in_race, int8 in_gender = 0xFF);
	static	void	CreateSpawnPacket(EQZonePacket* app, NewSpawn_Struct* ns);
//	static	int		CheckEffectIDMatch(int8 effectindex, int16 spellid1, int8 caster_level1, int16 spellid2, int8 caster_level2);
			int8	MaxSkill(int16 skillid, int16 class_, int16 level);
    inline	int8	MaxSkill(int16 skillid) { return MaxSkill(skillid, GetClass(), GetLevel()); }
    // Util functions for MaxSkill
    int8	MaxSkill_weapon(int16 skillid, int16 class_, int16 level);
    int8	MaxSkill_offensive(int16 skillid, int16 class_, int16 level);
    int8	MaxSkill_defensive(int16 skillid, int16 class_, int16 level);
    int8	MaxSkill_arcane(int16 skillid, int16 class_, int16 level);
    int8	MaxSkill_class(int16 skillid, int16 class_, int16 level);
	
	
	void	RogueBackstab(Mob* other, const Item_Struct* weapon, int8 bs_skill, bool min_damage = false);
	void	RogueAssassinate(Mob* other); // solar
	bool	BehindMob(Mob* other = 0, float playerx = 0.0f, float playery = 0.0f);
	
	Mob(const char*   in_name,
	    const char*   in_lastname,
	    sint32  in_cur_hp,
	    sint32  in_max_hp,
	    int8    in_gender,
	    uint16	in_race,
	    int8    in_class,
        bodyType    in_bodytype,
	    int8    in_deity,
	    int8    in_level,
		int32   in_npctype_id, // rembrant, Dec. 20, 2001
		const int8*	in_skills, // socket 12-29-01
		float	in_size,
		float	in_walkspeed,
		float	in_runspeed,
	    float   in_heading,
	    float	in_x_pos,
	    float	in_y_pos,
	    float	in_z_pos,
	    int8    in_light,
	    const	int32* in_equipment,
		int8	in_texture,
		int8	in_helmtexture,
		int16	in_ac,
		int16	in_atk,
		int8	in_str,
		int8	in_sta,
		int8	in_dex,
		int8	in_agi,
		int8	in_int,
		int8	in_wis,
		int8	in_cha,
		int8	in_haircolor,
		int8	in_beardcolor,
		int8	in_eyecolor1, // the eyecolors always seem to be the same, maybe left and right eye?
		int8	in_eyecolor2,
		int8	in_hairstyle,
// vesuvias - appearence fix
		int8	in_luclinface,
		int8	in_beard,
		int8	in_aa_title,

		float	in_fixedZ,
		int16	in_d_meele_texture1,
		int16	in_d_meele_texture2,
		int8	in_see_invis,			// Mongrel: see through invis
		int8	in_see_invis_undead,		// Mongrel: see through invis vs. undead
		int8	in_qglobal

	);
	virtual ~Mob();
	
	inline virtual bool IsMob() { return true; }
	inline virtual bool InZone() { return true; }
	MyList <wplist> Waypoints;
	void	BuffProcess();
	virtual void SetLevel(uint8 in_level, bool command = false) { level = in_level; }
	
	virtual inline sint32 GetPrimaryFaction() { return 0; }
	virtual void SetSkill(int in_skill_num, int8 in_skill_value) { // socket 12-29-01
		if (in_skill_num <= HIGHEST_SKILL) { skills[in_skill_num + 1] = in_skill_value; } }
	virtual uint32 GetSkill(int skill_num) { if (skill_num <= HIGHEST_SKILL) { return skills[skill_num + 1]; } return 0; } // socket 12-29-01
	virtual void SendWearChange(int8 material_slot);
	virtual sint32 GetEquipment(int8 material_slot);	// returns item id
	virtual sint32 GetEquipmentMaterial(int8 material_slot);
	virtual sint32 GetEquipmentColor(int8 material_slot);

	void Warp( float x, float y, float z );
	inline virtual bool IsMoving() { return moving; }
	virtual void SetMoving(bool move) { moving = move; delta_x=0; delta_y=0; delta_z=0; delta_heading=0; }
	virtual void GoToBind() {}
	virtual void Gate();
	virtual bool Attack(Mob* other, int Hand = 13, bool FromRiposte = false) { return false; }		// 13 = Primary (default), 14 = secondary
	virtual void Damage(Mob* from, sint32 damage, int16 spell_id, int8 attack_skill = 0x04, bool avoidable = true, sint8 buffslot = -1, bool iBuffTic = false) {};
	virtual void Heal();
	virtual void HealDamage(uint32 ammount);
	virtual void SetMaxHP() { cur_hp = max_hp; }
	virtual void Death(Mob* killer, sint32 damage, int16 spell_id = 0xFFFF, int8 attack_skill = 0x04) {}
	static int32 GetLevelCon(int8 mylevel, int8 iOtherLevel);
	inline int32 GetLevelCon(int8 iOtherLevel) { return(this?GetLevelCon(GetLevel(), iOtherLevel):CON_GREEN); }
	
	inline virtual void SetHP(sint32 hp) { if (hp >= max_hp) cur_hp = max_hp; else cur_hp = hp;} 
	int16   equipment[9];
	bool ChangeHP(Mob* other, sint32 amount, int16 spell_id = 0, sint8 buffslot = -1, bool iBuffTic = false);
	int MonkSpecialAttack(Mob* other, int8 skill_used);
	void TryBackstab(Mob *other, const Item_Struct* weapon);
	void DoAnim(const int animnum, int type=0, bool ackreq = true, FilterType filter = FilterNone);
	
	void ChangeSize(float in_size, bool bNoRestriction = false);
	virtual void GMMove(float x, float y, float z, float heading = 0.01);
	void SendPosUpdate(int8 iSendToSelf = 0);
	void MakeSpawnUpdateNoDelta(PlayerPositionUpdateServer_Struct* spu);
	void MakeSpawnUpdate(PlayerPositionUpdateServer_Struct* spu);
	void SendPosition();
	void SendAllPosition();

	void CreateDespawnPacket(EQZonePacket* app);
	void CreateHorseSpawnPacket(EQZonePacket* app, const char* ownername, uint16 ownerid, Mob* ForWho = 0);
	void CreateSpawnPacket(EQZonePacket* app, Mob* ForWho = 0);
	virtual void FillSpawnStruct(NewSpawn_Struct* ns, Mob* ForWho);
	void CreateHPPacket(EQZonePacket* app);
	void SendHPUpdate();
		
	bool AddProcToWeapon(int16 spell_id, bool bPerma = false, int8 iChance = 3);
	bool RemoveProcFromWeapon(int16 spell_id, bool bAll = false);
	bool HasProcs();
	
	inline bool SeeInvisible() { return see_invis; }				// Mongrel: Now using the flags
	inline bool SeeInvisibleUndead() { return see_invis_undead; }   // Mongrel: Now using the flags
	bool CheckLos(Mob* other);
	bool CheckLosFN(Mob* other);
	inline bool GetQglobal() {return qglobal;}		// SCORPIOUS2K - return quest global flag

	bool IsInvisible(Mob* other = 0);
	void SetInvisible(bool state);
   
	bool AttackAnimation(int &attack_skill, int16 &skillinuse, int Hand, const ItemInst* weapon);
	bool AvoidDamage(Mob* attacker, sint32 &damage);
	bool CheckHitChance(Mob* attacker, int8 attack_skill, int Hand, int16 skillinuse);
	
	void	DamageShield(Mob* other);
	bool	FindBuff(int16 spellid);
	bool	FindType(int8 type, bool bOffensive = false, int16 threshold = 100);
	sint8	GetBuffSlotFromType(int8 type);
	
	void	MakePet(int16 spell_id, const char* pettype, const char *petname = NULL);
	inline void	MakePetType(int16 spell_id, const char* pettype, const char *petname = NULL) { MakePet(spell_id, pettype, petname); }	//for perl
	void	MakePet(int16 spell_id, int8 in_level, int8 in_class, int16 in_race, int8 in_texture = 0, int8 in_pettype = 0, float in_size = 0, int8 type = 0, int32 min_dmg = 0, int32 max_dmg = 0, const char *petname = NULL);
	
	bool	CombatRange(Mob* other);
	int8	flag[60];		//this is for quests or something...
	
	virtual inline int16	GetBaseRace()		{ return base_race; }
	virtual inline int8	GetBaseGender()		{ return base_gender; }
	virtual inline int8	GetDeity()			{ return deity; }
	inline const int16&	GetRace()			{ return race; }
	inline const int8&	GetGender()			{ return gender; }
	inline const int8&	GetTexture()		{ return texture; }
	inline const int8&	GetHelmTexture()	{ return helmtexture; }
	inline const int8&	GetClass()			{ return class_; }
	inline const uint8&	GetLevel()			{ return level; }
	inline const char*	GetName()			{ return name; }
	const char *GetCleanName();
	inline Mob*			GetTarget()			{ return target; }
	virtual inline void	SetTarget(Mob* mob)	{ target = mob; }
	virtual inline float		GetHPRatio()		{ return max_hp == 0 ? 0 : ((float)cur_hp/max_hp*100); }
	
	bool IsWarriorClass();
	bool IsAttackAllowed(Mob *target);
	bool IsBeneficialAllowed(Mob *target);
	
	virtual inline const sint32&	GetHP()			{ return cur_hp; }
	virtual inline const sint32&	GetMaxHP()		{ return max_hp; }
	virtual inline sint32			CalcMaxHP()		{ return max_hp = (base_hp  + itembonuses.HP + spellbonuses.HP); }
	// need those cause SoW or Snare didnt work for mobs
	virtual float GetWalkspeed();
	virtual float GetRunspeed();
	virtual int GetCasterLevel(int16 spell_id);
	void ApplySpellsBonuses(int16 spell_id, int8 casterlevel, StatBonuses* newbon);
	
	virtual inline const sint32&	GetMaxMana()	{ return max_mana; }
	virtual inline const sint32&	GetMana()		{ return cur_mana; }
	virtual const sint32& SetMana(sint32 amount);
	virtual inline float			GetManaRatio()	{ return max_mana == 0 ? 100 : (((float)cur_mana/max_mana)*100); }
	void			SetZone(int32 zone_id);
	
	// neotokyo: moved from client to use in NPC too
	char GetCasterClass();
	virtual sint32 CalcMaxMana();
	
	inline virtual int16	GetAC()		{ return AC + itembonuses.AC + spellbonuses.AC; } // Quagmire - this is NOT the right math
	inline virtual int16	GetATK()	{ return ATK + itembonuses.ATK + spellbonuses.ATK; }
	inline virtual sint16	GetSTR()	{ return STR + itembonuses.STR + spellbonuses.STR; }
	inline virtual sint16	GetSTA()	{ return STA + itembonuses.STA + spellbonuses.STA; }
	inline virtual sint16	GetDEX()	{ return DEX + itembonuses.DEX + spellbonuses.DEX; }
	inline virtual sint16	GetAGI()	{ return AGI + itembonuses.AGI + spellbonuses.AGI; }
	inline virtual sint16	GetINT()	{ return INT + itembonuses.INT + spellbonuses.INT; }
	inline virtual sint16	GetWIS()	{ return WIS + itembonuses.WIS + spellbonuses.WIS; }
	inline virtual sint16	GetCHA()	{ return CHA + itembonuses.CHA + spellbonuses.CHA; }
	inline virtual sint16	GetMR() { return MR + itembonuses.MR + spellbonuses.MR; }
	inline virtual sint16	GetFR()	{ return FR + itembonuses.FR + spellbonuses.FR; }
	inline virtual sint16	GetDR()	{ return DR + itembonuses.DR + spellbonuses.DR; }
	inline virtual sint16	GetPR()	{ return PR + itembonuses.PR + spellbonuses.PR; }
	inline virtual sint16	GetCR() { return CR + itembonuses.CR + spellbonuses.CR; }
	
	inline virtual sint16  GetMaxSTR() { return GetSTR(); }
	inline virtual sint16  GetMaxSTA() { return GetSTA(); }
	inline virtual sint16  GetMaxDEX() { return GetDEX(); }
	inline virtual sint16  GetMaxAGI() { return GetAGI(); }
	inline virtual sint16  GetMaxINT() { return GetINT(); }
	inline virtual sint16  GetMaxWIS() { return GetWIS(); }
	inline virtual sint16  GetMaxCHA() { return GetCHA(); }
	
	virtual float GetActSpellRange(int16 spell_id, float range){ return range;}
	virtual sint32  GetActSpellDamage(int16 spell_id, sint32 value) { return value; }
	virtual sint32  GetActSpellHealing(int16 spell_id, sint32 value) { return value; }
	virtual sint32 GetActSpellCost(int16 spell_id, sint32 cost){ return cost;}
	virtual sint32 GetActSpellDuration(int16 spell_id, sint32 duration){ return duration;}
	virtual sint32 GetActSpellCasttime(int16 spell_id, sint32 casttime);
	float ResistSpell(int8 resist_type, int16 spell_id, Mob *caster);
	int GetSpecializeSkill(int16 spell_id);
	
	void ShowStats(Client* client);
	void ShowBuffs(Client* client);
	int32 GetNPCTypeID()			{ return npctype_id; } // rembrant, Dec. 20, 2001
	inline const int32& GetNPCSpellsID()	{ return npc_spells_id; }
	
	float Dist(const Mob &);
	float DistNoZ(const Mob &);
	float DistNoRoot(const Mob &);
	float DistNoRootNoZ(const Mob &);
	
	bool IsTargeted() { return targeted; }
	void IsTargeted(bool in_tar) { targeted = in_tar; }

	inline const float&	GetX()				{ return x_pos; }
	inline const float&	GetY()				{ return y_pos; }
	inline const float&	GetZ()				{ return z_pos; }
	inline const float&	GetHeading()		{ return heading; }
	inline const float&	GetSize()			{ return size; }
	inline void			SetChanged()		{ pLastChange = Timer::GetCurrentTime(); }
	inline const int32&	LastChange()		{ return pLastChange; }
	
	void	SetFollowID(int32 id) { follow = id; }
	int32	GetFollowID()		  { return follow; }
	
	virtual void	Message(int32 type, const char* message, ...) {} // fake so dont have to worry about typecasting
	virtual void	Message_StringID(int32 type, int32 string_id, int32 distance = 0) {}
	virtual void	Message_StringID(int32 type, int32 string_id, const char* message,const char* message2=0,const char* message3=0,const char* message4=0,const char* message5=0,const char* message6=0,const char* message7=0,const char* message8=0, const char* message9=0, int32 distance = 0) {}
	void Say(const char *format, ...);
	void Say_StringID(int32 string_id, const char *message3 = 0, const char *message4 = 0, const char *message5 = 0, const char *message6 = 0, const char *message7 = 0, const char *message8 = 0, const char *message9 = 0);
	void Shout(const char *format, ...);
	void Emote(const char *format, ...);
	

	virtual void SpellProcess();
	bool CheckFizzle(int16 spell_id);
	void ZeroCastingVars();
	bool UseBardSpellLogic(int16 spell_id = 0xffff, int slot = -1);
	void InterruptSpell(int16 spellid = 0xFFFF);
	void InterruptSpell(int16, int16, int16 spellid = 0xFFFF);
	virtual void	CastSpell(int16 spell_id, int16 target_id, int16 slot = 10, sint32 casttime = -1, sint32 mana_cost = -1, int32* oSpellWillFinish = 0, int32 item_slot = 0xFFFFFFFF);
	virtual void	DoCastSpell(int16 spell_id, int16 target_id, int16 slot = 10, sint32 casttime = -1, sint32 mana_cost = -1, int32* oSpellWillFinish = 0, int32 item_slot = 0xFFFFFFFF);
	void	CastedSpellFinished(int16 spell_id, int32 target_id, int16 slot, int16 mana_used, int32 inventory_slot = 0xFFFFFFFF);
	bool	SpellFinished(int16 spell_id, int32 target_id, int16 slot = 10, int16 mana_used = 0);
	bool	SpellOnTarget(int16 spell_id, Mob* spelltar);
//	int	CheckAddBuff(Mob* caster, const int16& spell_id, const int& caster_level, int* buffdur, int ticsremaining = -1);
	int	AddBuff(Mob *caster, const int16 spell_id, int duration = 0);
	bool	SpellEffect(Mob* caster, int16 spell_id, double partial = 100);
	bool	IsImmuneToSpell(int16 spell_id, Mob *caster);
	void	DoBuffTic(int16 spell_id, int32 ticsremaining, int8 caster_level, Mob* caster = 0);
	void	BuffFadeBySpellID(int16 spell_id);
	void	BuffFadeByEffect(int effectid, int skipslot = -1);
	void	BuffFadeAll();
	void	BuffFadeBySlot(int slot, bool iRecalcBonuses = true);
	int	CanBuffStack(int16 spellid, int8 caster_level, bool iFailIfOverwrite = false);
	inline bool	IsCasting() { return((casting_spell_id != 0)); }
	int16	CastingSpellID() { return casting_spell_id; }
	
// vesuvias - appearence fix
	void	SendIllusionPacket(int16 in_race, int8 in_gender = 0xFF, int16 in_texture = 0xFFFF, int16 in_helmtexture = 0xFFFF, int8 in_haircolor = 0xFF, int8 in_beardcolor = 0xFF, int8 in_eyecolor1 = 0xFF, int8 in_eyecolor2 = 0xFF, int8 in_hairstyle = 0xFF, int8 in_luclinface = 0xFF, int8 in_beard = 0xFF, int8 in_aa_title = 0xFF);

	static	int32	GetAppearanceValue(EmuAppearance iAppearance);
	void	SendAppearancePacket(int32 type, int32 value, bool WholeZone = true, bool iIgnoreSelf = false);
	void	SetAppearance(EmuAppearance app, bool iIgnoreSelf = true);
	inline EmuAppearance	GetAppearance()	const { return _appearance; }
	inline const int8&	GetRunAnimSpeed()			{ return pRunAnimSpeed; }
	inline void			SetRunAnimSpeed(sint8 in)	{ if (pRunAnimSpeed != in) { pRunAnimSpeed = in; pLastChange = Timer::GetCurrentTime(); } }
	
	Mob*	GetPet();
	Mob*	GetFamiliar();
	void	SetPet(Mob* newpet);
	Mob*	GetOwner();
	Mob*	GetOwnerOrSelf();
	void	SetPetID(int16 NewPetID);
	inline int16	GetPetID()		const			{ return petid;  }
	void	SetFamiliarID(int16 NewPetID);
	inline int16	GetFamiliarID()	const			{ return familiarid;  }
	void	SetOwnerID(int16 NewOwnerID);
	inline int16	GetOwnerID()	const			{ return ownerid; }
	inline const	int16&	GetPetType()	const			{ return typeofpet; }
	bool IsFamiliar() const { return(typeofpet >= 1 && typeofpet <= 4); }
	inline bool HasOwner() const { return(GetOwnerID() != 0); }
	inline bool IsPet() const { return(GetOwnerID() != 0); }
	inline bool HasPet() const { return(GetPetID() != 0); }
	
    inline const	bodyType	GetBodyType() const	{ return bodytype; }
    int16   FindSpell(int16 classp, int16 level, int type, FindSpellType spelltype, float distance, sint32 mana_avail);
	void	CheckBuffs();
	bool	CheckSelfBuffs();
	void	CheckPet();
	
 	void    SendSpellBarDisable();
 	void    SendSpellBarEnable(int16 spellid);
 	virtual void    Stun(int duration);
	
	bool	invulnerable;
	bool	invisible, invisible_undead, sneaking;
	bool	see_invis, see_invis_undead;   // Mongrel: See Invis and See Invis vs. Undead 
	bool	qglobal;		// SCORPIOUS2K - qglobal flag

	void	Spin();
	void	Kill();
	
	void	SetAttackTimer();
	inline void	SetInvul(bool invul) { invulnerable=invul; }
	inline bool	GetInvul(void) { return invulnerable; }
	inline void	SetExtraHaste(int Haste) { ExtraHaste = Haste; }
	virtual int GetHaste();

	int		GetWeaponDamageBonus(const Item_Struct* Weapon);
	int		GetMonkHandToHandDamage(void);
	
	bool	CanThisClassDoubleAttack(void);
	bool	CanThisClassDualWield(void);
	bool	CanThisClassRiposte(void);
	bool	CanThisClassDodge(void);
	bool	CanThisClassParry(void);
	
	int	GetMonkHandToHandDelay(void);
	int8	GetClassLevelFactor();
	void	Mesmerize();
	inline bool	IsMezzed() const { return mezzed; }
	inline bool	IsStunned() const { return stunned; }
	inline int16	GetErrorNumber() const {return adverrorinfo;}
	
	inline int16	GetRune() const { return rune; }
	inline void	SetRune(int16 in_rune) { rune = in_rune; }
	
	sint16	ReduceDamage(sint16 damage);
	sint16  ReduceMagicalDamage(sint16 damage);

	
   	inline int16 GetMagicRune() const { return magicrune; }
	void	SetMagicRune(int16 in_rune) { magicrune = in_rune; }
	
    bool SpecAttacks[SPECATK_MAXNUM];
#define MAX_RAMPAGE_TARGETS 3
#define MAX_FLURRY_HITS 2
    char RampageArray[MAX_RAMPAGE_TARGETS][64];
    bool Flurry();
    bool Rampage();
    bool AddRampage(Mob*);
	
    void StartEnrage();
    bool IsEnraged();
	void Taunt(NPC* who, bool always_succeed);
	
	virtual void		AI_Init();
	virtual void		AI_Start(int32 iMoveDelay = 0);
	virtual void		AI_Stop();
	void				AI_Process();
	void				AI_Event_Engaged(Mob* attacker, bool iYellForHelp = true);
	void				AI_Event_NoLongerEngaged();
	void				AI_Event_SpellCastFinished(bool iCastSucceeded, int8 slot);
	bool				AI_AddNPCSpells(int32 iDBSpellsID);
	void				AI_SetRoambox(float iDist, float iRoamDist, int32 iDelay = 2500);
	void				AI_SetRoambox(float iDist, float iMaxX, float iMinX, float iMaxY, float iMinY, int32 iDelay = 2500);
// quest wandering commands
	void				StopWandering();
	void				ResumeWandering();
	void				PauseWandering(int pausetime);
	void				MoveTo(float mtx, float mty, float mtz);
	virtual FACTION_VALUE GetReverseFactionCon(Mob* iOther) { return FACTION_INDIFFERENT; }
	FACTION_VALUE		GetSpecialFactionCon(Mob* iOther);
	inline const bool&	IsAIControlled() { return pAIControlled; }
    inline const float&	GetGuardX() { return guard_x; }
    inline const float&	GetGuardY() { return guard_y; }
    inline const float&	GetGuardZ() { return guard_z; }
	
	void	SetGuardXYZ(float x, float y, float z) { guard_x = x; guard_y = y; guard_z = z; }
	
	inline const float&	GetGuardHeading() { return guard_heading; }
    inline const float&	GetSpawnX() { return spawn_x; }
    inline const float&	GetSpawnY() { return spawn_y; }
    inline const float&	GetSpawnZ() { return spawn_z; }
    inline const float&	GetSpawnHeading() { return spawn_heading; }
	inline const float GetAggroRange() { return (spellbonuses.AggroRange == -1) ? pAggroRange : spellbonuses.AggroRange; }
	inline const float GetAssistRange() { return (spellbonuses.AssistRange == -1) ? pAssistRange : spellbonuses.AssistRange; }
    void				SaveGuardSpot(bool iClearGuardSpot = false);
    void				SaveSpawnSpot();
	
	void				UpdateWaypoint(int wp_index);
	bool				AICastSpell(Mob* tar, int8 iChance, int16 iSpellTypes);
	void				AIDoSpellCast(int8 i, Mob* tar, sint32 mana_cost, int32* oDontDoAgainBefore = 0);
	inline void			SetPetOrder(eStandingPetOrder i) { pStandingPetOrder = i; }
	inline const eStandingPetOrder& GetPetOrder() { return pStandingPetOrder; }
	inline const bool&	IsRoamer() { return roamer; }
	inline const bool   IsRooted() { return rooted || permarooted; }

	void				SetWaypointPause();
	bool				RemoveFromHateList(Mob* mob);
    void				AddToHateList(Mob* other, sint32 hate = 0, sint32 damage = 0, bool iYellForHelp = true, bool bFrenzy = false, bool iBuffTic = false);
	void				SetHate(Mob* other, sint32 hate = 0, sint32 damage = 0) {hate_list.Set(other,hate,damage);}
	int32				GetHateAmount(Mob* tmob, bool is_dam = false)  {return hate_list.GetEntHate(tmob,is_dam);}
	int32				GetDamageAmount(Mob* tmob)  {return hate_list.GetEntHate(tmob, true);}
	Mob*				GetHateTop()  {return hate_list.GetTop();}
	Mob*				GetHateDamageTop(Mob* other)  {return hate_list.GetDamageTop(other);}
	Mob*				GetHateRandom()  {return hate_list.GetRandom();}
	bool				IsEngaged()   {return(!hate_list.IsEmpty()); }
	bool				HateSummon();
	void				FaceTarget(Mob* MobToFace = 0, bool update = false);
	void				SetHeading(float iHeading) { if (heading != iHeading) { pLastChange = Timer::GetCurrentTime(); heading = iHeading; } }
	void				WhipeHateList(); //Wipe?
	
	int					GetMaxWp(){ return max_wp; }
	int					GetCurWp(){ return cur_wp; }
#ifdef ENABLE_FEAR_PATHING
	void SetFeared(Mob *caster, int32 duration, bool flee = false);
	float GetFearSpeed();
#ifdef FLEE_HP_RATIO
	inline void StartFleeing() { SetFeared(GetHateTop(), FLEE_RUN_DURATION, true); }
	void ProcessFlee();
	void CheckFlee();
#endif
#endif
	
	inline bool			CheckAggro(Mob* other) {return hate_list.IsOnHateList(other);}
    sint8				CalculateHeadingToTarget(float in_x, float in_y);
    bool				CalculateNewPosition(float x, float y, float z, float speed, bool checkZ = false);
	bool				CalculateNewPosition2(float x, float y, float z, float speed, bool checkZ = false);
    float				CalculateDistance(float x, float y, float z);
	void				CalculateNewWaypoint();
	int8				CalculateHeadingToNextWaypoint();
//	float				CalculateDistanceToNextWaypoint();
	void				AssignWaypoints(int16 grid);
	void				SendTo(float new_x, float new_y, float new_z);
	void				SendToFixZ(float new_x, float new_y, float new_z);
	void				NPCSpecialAttacks(const char* parse, int permtag);
	inline int32&		DontHealMeBefore() { return pDontHealMeBefore; }
	inline int32&		DontBuffMeBefore() { return pDontBuffMeBefore; }
	inline int32&		DontDotMeBefore() { return pDontDotMeBefore; }
	inline int32&		DontRootMeBefore() { return pDontRootMeBefore; }
	inline int32&		DontSnareMeBefore() { return pDontSnareMeBefore; }
	
	// calculate interruption of spell via movement of mob
	void SaveSpellLoc() {spell_x = x_pos; spell_y = y_pos; spell_z = z_pos; }
	inline float GetSpellX() {return spell_x;}
	inline float GetSpellY() {return spell_y;}
	inline float GetSpellZ() {return spell_z;}
	inline bool	IsGrouped()	{ return isgrouped; } //Why have an accessor for a public variable?
	
	bool CheckWillAggro(Mob *mob);
	
	void	InstillDoubt(Mob *who);
	sint16	GetResist(int8 type);
	void	StopSong();
	Mob*	GetShieldTarget()			{ return shield_target; }
	void	SetShieldTarget(Mob* mob)	{ shield_target = mob; }
	Mob*	GetSongTarget()			{ return bardsong_target; }
	void	SetSongTarget(Mob* mob)	{ bardsong_target = mob; }
	bool	Charmed() { return charmed; }
	int32	GetLevelHP(int8 tlevel);
	int16	CheckAggroAmount(int16 spellid);
	int16	CheckHealAggroAmount(int16 spellid);
	virtual int32 GetAA(int32 aa_id) { return(0); }
	
	int16	GetInstrumentMod(int16 spell_id);
	int CalcSpellEffectValue(int16 spell_id, int effect_id, int caster_level = 1, Mob *caster = NULL);
	int CalcSpellEffectValue_formula(int formula, int base, int max, int caster_level, int16 spell_id);
	int CheckStackConflict(int16 spellid1, int caster_level1, int16 spellid2, int caster_level2);

	inline EGNode *GetEGNode() { return(_egnode); }
	inline void SetEGNode(EGNode *s) { _egnode = s; }
	
	bool	isgrouped; //These meant to be private?
	bool	pendinggroup;
	int16	d_meele_texture1;
	int16	d_meele_texture2;
//	float	wp_x[50]; //X of waypoint
//	float	wp_y[50]; //Y of waypoint
//	float	wp_z[50]; //Z of waypoint
//	int32	wp_s[50]; //Pause of waypoint
//	int16	wp_a[6]; //0 = Amount of waypoints, 1 = Wandering Type, 2 = Pause Type, 3 = Current Waypoint, 4 = Grid Number, 5 = Used for patrol grids
	int8	texture;
	int8	helmtexture;
	Shielders_Struct shielder[MAX_SHIELDERS];
	
	Trade* trade;
	
  // HP Event 
   inline int& GetNextHPEvent() { return nexthpevent; } 
   void SetNextHPEvent( int hpevent );
	void SendItemAnimation(Mob *to, const Item_Struct *item);
	
	bool DivineAura();
	
	//temporary:
	bool fix_pathing;
	inline float GetCWPX() { return(cur_wp_x); }
	inline float GetCWPY() { return(cur_wp_y); }
	inline float GetCWPZ() { return(cur_wp_z); }
	inline float GetCWPP() { return(cur_wp_pause); }
	inline int GetCWP() { return(cur_wp); }
	inline int GetMWP() { return(max_wp); }
	
protected:
	void CommonDamage(Mob* other, sint32 &damage, const int16 spell_id, const int8 attack_skill, bool &avoidable, const sint8 buffslot, const bool iBuffTic);

	int	AC;
	int	ATK;
	int	STR;
	int	STA;
	int	DEX;
	int	AGI;
	int	INT;
	int	WIS;
	int	CHA;
	int MR;
	int CR;
	int FR;
	int DR;
	int PR;
	bool moving;
	bool targeted;
	bool findable;
	sint32  cur_hp;
	sint32  max_hp;
	sint32	base_hp;
	sint32	cur_mana;
	sint32	last_reported_mana;
	sint32	max_mana;
	sint32	hp_regen;
	sint32	mana_regen;
	Buffs_Struct	buffs[BUFF_COUNT];
	StatBonuses		itembonuses;
	StatBonuses		spellbonuses;
	int16			petid;
    int16           familiarid;
	int16			ownerid;
	int16			typeofpet; // 0xFF = charmed
	
	int32			follow;
	
	int8    gender;
	int16	race;
	int8	base_gender;
	int16	base_race;
	int8    class_;
	bodyType    bodytype;
	int16	deity;
	uint8    level;
	int32   npctype_id; // rembrant, Dec. 20, 2001
	int8    skills[75];
	float	x_pos;
	float	y_pos;
	float	z_pos;
	float	heading;
	uint16	animation;
	float	size;
	float	walkspeed;
	float	runspeed;
	int32 pLastChange;
	void CalcSpellBonuses(StatBonuses* newbon);
	virtual void CalcBonuses();
	void TryWeaponProc(const Item_Struct* weapon, Mob *on);
	void TryWeaponProc(const ItemInst* weapon, Mob *on);
	void ExecWeaponProc(uint16 spell_id, Mob *on);
	float GetProcChances(float &ProcBonus, float &ProcChance);
	
	enum {MAX_PROCS = 4};
	tProc PermaProcs[MAX_PROCS];
	tProc SpellProcs[MAX_PROCS];
	
	char    name[64];
	char		clean_name[64];
	char    lastname[70];
	
    bool bEnraged;
    Timer *SpecAttackTimers[SPECATK_MAXNUM];
	
	sint32	delta_heading;
    float	delta_x;
	float	delta_y;
	float	delta_z;
	uint32	guildeqid; // guild's EQ ID, 0-511, 0xFFFFFFFF = none
	
	int8    light;
	
	float	fixedZ;
	EmuAppearance    _appearance;
	int8	pRunAnimSpeed;
	
	Mob*	target;
	Timer	attack_timer;
	Timer	attack_dw_timer;
	Timer	ranged_timer;
	float	attack_speed;		//% increase/decrease in attack speed (not haste)
	Timer	tic_timer;
	Timer	mana_timer;
	
	Timer spellend_timer;
	int16	casting_spell_id;
	int8	casting_spell_AIindex;
	
    float spell_x, spell_y, spell_z;
	
	bool	isattacked;
	int	attacked_count;
	bool	delaytimer;
	int16 casting_spell_targetid;
	int16 casting_spell_slot;
	int16 casting_spell_mana;
	int32 casting_spell_inventory_slot;
	int8	haircolor;
	int8	beardcolor;
	int8	eyecolor1; // the eyecolors always seem to be the same, maybe left and right eye?
	int8	eyecolor2;
	int8	hairstyle;
	int8	luclinface; // and beard
// vesuvias - appearence fix
	int8	beard;

	int8	aa_title;

	int16	bardsong;
	int8	bardsong_slot;
	Mob*	bardsong_target;
	Mob*	shield_target;
	
	int16	rune;
	int16	magicrune;
	int ExtraHaste;	// for the #haste command
	bool	mezzed;
	bool	stunned;
	bool	charmed;	//this isnt fully implemented yet
	bool	rooted;
//	Timer mezzed_timer;
	Timer  stunned_timer;
	Timer	bardsong_timer;
	int16	adverrorinfo;
	
	// MobAI stuff
	eStandingPetOrder pStandingPetOrder;
    float guard_x, guard_y, guard_z, guard_heading;
    float spawn_x, spawn_y, spawn_z, spawn_heading;
	int32	minLastFightingDelayMoving;
	int32	maxLastFightingDelayMoving;
	float	pAggroRange;
	float	pAssistRange;
	Timer*	AIthink_timer;
	Timer*	AImovement_timer;
	bool	movetimercompleted;
	bool   permarooted;
	Timer*	AIautocastspell_timer;
	Timer*	AIscanarea_timer;
	Timer*	AIwalking_timer;
	int32	pLastFightingDelayMoving;
	int32	npc_spells_id;
	AISpells_Struct	AIspells[MAX_AISPELLS]; // expected to be pre-sorted, best at low index
	HateList hate_list;
	
	
#ifdef ENABLE_FEAR_PATHING
	void CalculateFearPosition();
	bool FearTryStraight(Mob *caster, int32 duration, bool flee, VERTEX &hit, VERTEX &fv);
//	VERTEX fear_vector;
	FearState fear_state;
	MobFearState *fear_path_state;
	bool flee_mode;
#ifdef FLEE_HP_RATIO
	Timer flee_timer;
#endif
#endif
	
	bool	pAIControlled;
	bool	roamer;
	
	int		wandertype;
	int		pausetype;
	
	int		max_wp;
	int		cur_wp;
// used by quest wandering commands
	int		save_wp;
	
	float		cur_wp_x;
	float		cur_wp_y;
	float		cur_wp_z;
	int		cur_wp_pause;
	
	int		patrol;
	
	int32	pDontHealMeBefore;
	int32	pDontBuffMeBefore;
	int32	pDontDotMeBefore;
	int32	pDontRootMeBefore;
	int32	pDontSnareMeBefore;
	int32*	pDontCastBefore_casting_spell;
	
	float roambox_max_x;
	float roambox_max_y;
	float roambox_min_x;
	float roambox_min_y;
	float roambox_distance;
	float roambox_movingto_x;
	
	float roambox_movingto_y;
	int32 roambox_delay;
	
	// Bind wound
	Timer  bindwound_timer;
	Mob*    bindwound_target;
	// hp event
	int nexthpevent;

	EGNode *_egnode;	//the EG node we are in
};

// All data associated with a single trade
class Trade
{
public:
	Trade(Mob* in_owner);
	virtual ~Trade();
	
	void Reset();
	void SetTradeCash(uint32 in_pp, uint32 in_gp, uint32 in_sp, uint32 in_cp);
	
	// Initiate a trade with another mob
	// Also puts other mob into trader mode with this mob
	void Start(uint32 mob_id, bool initiate_with=true);
	
	// Mob the owner is trading with
	Mob* With();
	
	// Add item from cursor slot to trade bucket (automatically does bag data too)
	void AddEntity(int16 from_slot_id, int16 trade_slot_id);
	
	// Audit trade
	void LogTrade();
	
	// Debug only method
	#if (EQDEBUG >= 9)
		void DumpTrade();
	#endif
	
public:
	// Object state
	TradeState state;
	sint32 pp;
	sint32 gp;
	sint32 sp;
	sint32 cp;
	
private:
	// Send item data for trade item to other person involved in trade
	void SendItemData(const ItemInst* inst, sint16 dest_slot_id);
	
	uint32 with_id;
	Mob* owner;
};

#endif
