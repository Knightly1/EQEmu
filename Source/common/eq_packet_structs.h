/*  EQEMu:  Everquest Server Emulator
	Copyright (C) 2001-2003  EQEMu Development Team (http://eqemulator.net)

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
#ifndef EQ_PACKET_STRUCTS_H
#define EQ_PACKET_STRUCTS_H 

#include "types.h"
#include <string.h>
#include <time.h>

#define BUFF_COUNT 15

/*
** Item packet types
**
*/
enum ItemPacketType
{
	ItemPacketViewLink			= 0x00,
	ItemPacketTradeView			= 0x65,
	ItemPacketLoot				= 0x66,
	ItemPacketTrade				= 0x67,
	ItemPacketCharInventory		= 0x69,
	ItemPacketSummonItem		= 0x6A,
	ItemPacketMerchant			= 0x64,
	ItemPacketWorldContainer	= 0x6B
};

/*
** Item attributes
**
*/
enum ItemAttrib
{
	ItemAttribLore			= (1 << 0),
	ItemAttribArtifact		= (1 << 1),
	ItemAttribSummoned		= (1 << 2),
	ItemAttribMagic			= (1 << 3),
	ItemAttribAugment		= (1 << 4),
	ItemAttribPendingLore	= (1 << 5),
	ItemAttribNone			= 0,
	ItemAttribUnknown		= 0xFFFFFFFF
};

/*
** Item types
**
*/
enum ItemType
{
	ItemTypeCommon		= 0,
	ItemTypeContainer	= 1,
	ItemTypeBook		= 2
};

/*
** Compiler override to ensure
** byte aligned structures
*/
#pragma pack(1)
//#define INVERSEXY = 1

/* Name Approval Struct */
/* Len: */
/* Opcode: 0x8B20*/
struct NameApproval
{
	char name[64];
	int32 race;
	int32 class_;
	int32 deity;
};

/*
** Entity identification struct
** Size: 4 bytes
** OPCodes: OP_DeleteSpawn, OP_Assist
*/
struct EntityId_Struct
{
/*00*/	uint32	entity_id;
/*04*/
};

struct Duel_Struct
{
	int32 duel_initiator;
	int32 duel_target;
};

struct DuelResponse_Struct
{
	int32 target_id;
	int32 entity_id;
	int32 unknown;
};

/*
** Color_Struct
** Size: 4 bytes
** Used for convenience
** Merth: Gave struct a name so gcc 2.96 would compile
**
*/
struct Color_Struct
{
	union
	{
		struct
		{
			int8	blue;
			int8	green;
			int8	red;
			uint8	use_tint;	// if there's a tint this is FF
		} rgb;
		uint32 color;
	};
};

/*
** Character Selection Struct
** Length: 1660 Bytes
**
*/
struct CharacterSelect_Struct {
/*0000*/	char	name[10][64];		// Characters Names
/*0640*/	int8	level[10];			// Characters Levels
/*0650*/	int8	class_[10];			// Characters Classes
/*0660*/	int32	race[10];			// Characters Race
/*0700*/	int32	zone[10];			// Characters Current Zone
/*0740*/	int8	gender[10];			// Characters Gender
/*0750*/	int8	face[10];			// Characters Face Type
/*0760*/	int32	equip[10][9];		// 0=helm, 1=chest, 2=arm, 3=bracer, 4=hand, 5=leg, 6=boot, 7=melee1, 8=melee2
/*1120*/	Color_Struct	cs_colors[10][9];	// Characters Equipment Colors
/*1480*/	int32	deity[10];			// Characters Deity
/*1520*/	int32	melee[2][10];		// Characters primary and secondary IDFile number
/*1600*/	int8	haircolor[10];	// vesuvias - found values
/*1610*/	int8	beardcolor[10];
/*1620*/	int8	eyecolor2[10];
/*1630*/	int8	eyecolor1[10];
/*1640*/	int8	hair[10];
/*1650*/	int8	beard[10];
};

/*
** Generic Spawn Struct
** Length: 257 Bytes
** Fields from old struct not yet found:
**	float	size;
**	float	walkspeed;	// probably one of the ff 33 33 33 3f
**	float	runspeed;	// probably one of the ff 33 33 33 3f
**	int8	traptype;	// 65 is disarmable trap, 66 and 67 are invis triggers/traps
**	int8	npc_armor_graphic;	// 0xFF=Player, 0=none, 1=leather, 2=chain, 3=steelplate
**	int8	npc_helm_graphic;	// 0xFF=Player, 0=none, 1=leather, 2=chain, 3=steelplate
**
*/
struct Spawn_Struct
{
/*000*/	int8	npc;	// 0=player,1=npc,2=pc corpse,3=npc corpse,4=???,5=unknown spawn,10=self
/*001*/	int8	beard;			// vesuvias - appearance fix
/*002*/	int8	beardcolor;			// Player right eye color
/*003*/	int8	aa_title; // 0=none, 1=general, 2=archtype, 3=class
/*004*/	int8	unknown04[3];	// aa_title is probably an int32
/*007*/	Color_Struct	dye_rgb[9]; 			// armor dye colors
/*043*/ int8	class_; //class
/*044*/ int8	unknown044[2]; // *** Placeholder 
/*046*/ int8	cur_hp; //current hp
/*047*/	int8	afk; // 0=not afk, 1=afk
union {
/*048*/	int8	equip_chest2;// Second place in packet for chest texture (usually 0xFF in live packets)
// Not sure why there are 2 of them, but it effects chest texture!
/*048*/	int8	mount_color;// drogmor: 0=white, 1=black, 2=green, 3=red
// horse: 0=brown, 1=white, 2=black, 3=tan
};
/*049*/ int32	race; // race 
/*053*/ int8	eyecolor1;	// vesuvias
/*054*/ char	name[64]; // name 
/*118*/ int8	eyecolor2;	// vesuvias
/*119*/ int8	face;
/*120*/ int8	invis; // 0=visible,1=invisible 
/*121*/ int8	max_hp; // max hp 
/*122*/ int8	pvp; // 0=Not pvp,1=pvp  solar: this is wrong
/*123*/ int8	level; 
/*124*/ int8	lfg; // 0=Not lfg,1=lfg 
/*125*/ int32	heading:12; // spawn heading 
/*****/ int32	delta_heading:10; // change in heading 
/*****/ int32	animation:10; // animation id 
/*129*/ sint32	deltaX:13; 
/*****/ sint32	x:19; 
/*133*/ sint32	y:19; 
/*****/ sint32	deltaZ:13; 
/*137*/ sint32	deltaY:13; 
/*****/ sint32	z:19; 
/*141*/ int8	hairstyle;	// vesuvias
/*142*/ int8	haircolor;
/*143*/ int8	unknown143[8]; 
/*151*/ float	size; // Size 
/*155*/ int8	helm; 
/*156*/ float	runspeed; // 
/*160*/ int8	gm; // 0=not GM,1=GM 
/*161*/ float	walkspeed; // 
/*165*/ int32	guild_id; // GuildID 
/*169*/ int8	anon; // 0=normal,1=anon,2=roleplaying 
/*170*/ int8	gender; // 0=male,1=female,2=other 
/*171*/ int16	spawn_id; // Id of spawn 
/*173*/ int8	unknown173[3]; 
/*176*/ char	last_name[32]; // lastname 
/*208*/ int32	equipment[9]; 
/*244*/ int8	linkdead; // 0=Not LD, 1=LD 
/*245*/ uint32	bodytype; // Bodytype 
/*249*/	int8	guild_rank;
/*250*/ int8	unknown249[4]; 
/*254*/ uint32	pet_owner_id; 
/*258*/ int16	deity; 
/*260*/ int8	unknown260[31]; 
/*0291*/ int8	unknown291[80];
/*0367*/ int32	unknown367[2];
};

/*
** New Spawn
** Length: 176 Bytes
** OpCode: 4921
*/
struct NewSpawn_Struct
{
	struct Spawn_Struct spawn;	// Spawn Information
};

/*
** Server Zone Entry Struct
** Length: 452 Bytes
** OPCodes: OP_ServerZoneEntry
** Fields from old struct not yet found:
**	int8	title;				// AA Title
**	int8	skyradius;			// Skyradius
**	float	delta_x
**	float	delta_y
**	float	delta_z
**	float	heading
**	float	delta_heading
**
*/
struct ServerZoneEntry_Struct
{
	struct NewSpawn_Struct player;
};

struct NewZone_Struct {
/*0000*/	char	char_name[64];			// Character Name
/*0064*/	char	zone_short_name[32];	// Zone Short Name
/*0096*/	char	zone_long_name[278];	// Zone Long Name
/*0374*/	uint8	ztype;					// Zone type (usually FF)
/*0375*/	uint8	fog_red[4];				// Zone fog (red)
/*0379*/	uint8	fog_green[4];			// Zone fog (green)
/*0383*/	uint8	fog_blue[4];			// Zone fog (blue)
/*0387*/	uint8	unknown323;
/*0388*/	float	fog_minclip[4];
/*0404*/	float	fog_maxclip[4];
/*0420*/	float	walkspeed;
/*0424*/	int8	time_type;
/*0425*/	uint8	unknown360[49];
/*0474*/	uint8	sky;					// Sky Type
/*0475*/	uint8	unknown331[13];			// ***Placeholder
/*0488*/	float	zone_exp_multiplier;	// Experience Multiplier
/*0492*/	float	safe_x;					// Zone Safe X (Not Inversed)
/*0496*/	float	safe_y;					// Zone Safe Y (Not Inversed)
/*0500*/	float	safe_z;					// Zone Safe Z
/*0504*/	float	unknown440;			// ***Placeholder
/*0508*/	float	underworld;				// Underworld (Not Sure?)
/*0512*/	float	minclip;
	// Minimum View Distance
/*0516*/	float	maxclip;				// Maximum View DIstance
/*0520*/	int8	unknown_end[84];		// ***Placeholder
/*0604*/	char	zone_short_name2[72];
/*0672*/
};

/*
** New Zone Struct
** Length: 576 Bytes
** Last Updated: Jul-04-2003
*/
struct OLDZone_Struct {
/*0000*/	char	char_name[64];			// Character Name
/*0064*/	char	zone_short_name[32];	// Zone Short Name
/*0096*/	char	zone_long_name[278];	// Zone Long Name
/*0310*/	uint8	ztype;					// Zone type (usually FF)
/*0311*/	uint8	fog_red[4];				// Zone fog (red)
/*0315*/	uint8	fog_green[4];			// Zone fog (green)
/*0319*/	uint8	fog_blue[4];			// Zone fog (blue)
/*0374*/	uint8	unknown230[87];			// Unknown
/*0474*/	uint8	sky;					// Sky Type
/*0475*/	uint8	unknown331[13];			// ***Placeholder
/*0488*/	float	zone_exp_multiplier;	// Experience Multiplier
/*0492*/	float	safe_x;					// Zone Safe X (Not Inversed)
/*0496*/	float	safe_y;					// Zone Safe Y (Not Inversed)
/*0500*/	float	safe_z;					// Zone Safe Z
/*0504*/	float	unknown0504;			// ***Placeholder
/*0508*/	float	underworld;				// Underworld (Not Sure?)
/*0512*/	float	minclip;
	// Minimum View Distance
/*0516*/	float	maxclip;				// Maximum View DIstance
/*0520*/	int8	unknown_end[68];		// ***Placeholder
/*0588*/
};

struct CFGNewZone_Struct {
/*0000*/	char	char_name[64];			// Character Name
/*0064*/	char	zone_short_name[32];	// Zone Short Name
/*0096*/	char	zone_long_name[278];	// Zone Long Name
/*0278*/	int8	unknown230[100];		// ***Placeholder
/*0378*/	int8	sky;		  			// Sky Type
/*0379*/	int8	unknown331[9];			// ***Placeholder
/*0388*/	float	zone_exp_multiplier;	// Experience Multiplier
/*0392*/	float	safe_x;					// Zone Safe X (Not Inversed)
/*0396*/	float	safe_y;					// Zone Safe Y (Not Inversed)
/*0400*/	float	safe_z;					// Zone Safe Z
/*0404*/	float	unknown256;				// ***Placeholder
/*0408*/	float	underworld;				// Underworld (Not Sure?)
/*0412*/	float	minclip;				// Minimum View Distance
/*0416*/	float	maxclip;				// Maximum View DIstance
/*0420*/	int8	unknown_end[56];		// ***Placeholder
};

struct Discipline_Struct {
/*0000*/	int8 unknown0000[4];
/*0004*/	char  charname[64];
/*0068*/	char  charname2[64];
/*0132*/	int32 unknown0132;
};

/*
 * OP_Discipline from client size = 4
 */
struct ClientDiscipline_Struct {
    uint8	disc_id;	// There are only a few discs < 60
    uint8	unknown3[3];	// Which leaves room for ??
};

/*
** Memorize Spell Struct
** Length: 12 Bytes
**
*/
struct MemorizeSpell_Struct { 
int32 slot;     // Spot in the spell book/memorized slot 
int32 spell_id; // Spell id (200 or c8 is minor healing, etc) 
int32 scribing; // 1 if memorizing a spell, set to 0 if scribing to book, 2 if un-memming
int32 unknown12;
}; 

/*
** Spell Cast On Struct
** Length: 36 Bytes
**
*/
#if 0	// solar: see Action_Struct and CombatDamage_Struct
struct CastOn_Struct {

/*00*/	int16	target_id;
/*02*/	int16	source_id;
/*04*/	int8	source_level;
/*05*/	int8	unknown1;
/*06*/	int8	unknown2; // = 0A
/*07*/	int8	unknown_zero1[7];
/*16*/	float	heading;
/*20*/	int8	unknown_zero2[4];
/*22*/	int32	action;
/*26*/	int8	unknown3;
/*27*/	int16	spell_id;
/*29*/	int8	unknown29;
/*30*/	int8	unknown30; //always 04??
};
struct CastBuff_Struct{
/*00*/	int16	target_id;
/*02*/	int16	source_id;
/*04*/	int8	unknown4; //always E7??
/*05*/	int16	spell_id; //always E7??
/*07*/	int8	unknown_zero7[8];
/*17*/	float	heading2;
/*21*/	int8	unknown_zero21[4];
};
#endif



/*
** Make Charmed Pet
** Length: 12 Bytes
**
*/
struct Charm_Struct {
/*00*/	int32	owner_id;
/*02*/	int32	pet_id;
/*04*/	int32	command;    // 1: make pet, 0: release pet
};

struct InterruptCast_Struct
{
	int32 spawnid;
	int32 messageid;
	char	message[0];
};

struct DeleteSpell_Struct
{
/*000*/sint16	spell_slot;
/*002*/int8	unknowndss002[2];
/*004*/int8	success;
/*005*/int8	unknowndss006[3];
/*008*/
};

struct ManaChange_Struct
{                  
	int32	new_mana;                  // New Mana AMount
	int32	stamina;
	int32	spell_id;
	int32	unknown12;
};

struct SwapSpell_Struct 
{ 
	int32 from_slot; 
	int32 to_slot; 


}; 

struct BeginCast_Struct
{
	// len = 8
/*000*/	int16	caster_id;

/*002*/	int16	spell_id;
/*004*/	int32	cast_time;		// in miliseconds
};

#if 0 // solar: this is some old stuff
struct _Buff_Struct
{
/*000*/uint32 target_id;
/*004*/uint32 b_unknown1;
/*008*/uint16 spell_id;
/*010*/uint32 b_unknown2;
/*014*/uint16 b_unknown3;
/*016*/uint32 buff_slot;
/*020*/
};
#endif

struct CastSpell_Struct
{
	int32	slot;
	int32	spell_id;
	int32	inventoryslot;  // slot for clicky item, 0xFFFF = normal cast
	int32	target_id;
	int8    cs_unknown[4];
};

#define AT_Die				0	// this causes the client to keel over and zone to bind point
#define AT_WhoLevel		1	// the level that shows up on /who
#define AT_Invis			3	// 0 = visible, 1 = invisible
#define AT_PVP				4	// 0 = blue, 1 = pvp (red)
#define AT_Light			5	// light type emitted by player (lightstone, shiny shield)
#define AT_Anim				14	// 100=standing, 110=sitting, 111=ducking, 115=feigned, 105=looting
#define AT_Sneak			15	// 0 = normal, 1 = sneaking
#define AT_SpawnID		16	// server to client, sets player spawn id
#define AT_HP					17	// Client->Server, my HP has changed (like regen tic)
#define AT_Linkdead		18	// 0 = normal, 1 = linkdead
#define AT_Levitate		19	// 0=off, 1=flymode, 2=levitate
#define AT_GM					20	// 0 = normal, 1 = GM - all odd numbers seem to make it GM
#define AT_Anon				21	// 0 = normal, 1 = anon, 2 = roleplay
#define AT_GuildID		22
#define AT_GuildRank	23	// 0=member, 1=officer, 2=leader
#define AT_AFK				24	// 0 = normal, 1 = afk
#define AT_Split			28	// 0 = normal, 1 = autosplit on
#define AT_Size				29	// spawn's size
#define AT_NPCName		31	// change PC's name's color to NPC color 0 = normal, 1 = npc name
//#define AT_Trader			300  // Bazzar Trader Mode

// solar: animations for AT_Anim
#define ANIM_FREEZE	102
#define	ANIM_STAND		0x64
#define	ANIM_SIT		0x6e
#define	ANIM_CROUCH		0x6f
#define	ANIM_DEATH		0x73
#define ANIM_LOOT		0x69

/*
** SpawnAppearance_Struct
** Changes client appearance for all other clients in zone
** Size: 8 bytes
** Used in: OP_SpawnAppearance
**
*/
struct SpawnAppearance_Struct
{
/*0000*/ uint16 spawn_id;          // ID of the spawn
/*0002*/ uint16 type;              // Values associated with the type
/*0004*/ uint32 parameter;         // Type of data sent
/*0008*/
};


#if 1
// solar: this is used inside profile
struct SpellBuff_Struct
{
/*000*/	int8	slotid;
/*001*/ int8	level;
/*002*/	int16  effect;				// ***Placeholder
/*004*/	int32	spellid;
/*008*/ int32	duration;
/*012*/	int8	Unknown012[4];
};
#else
struct SpellBuff_Struct
{
/*000*/	int8  b_unknown1[5];		// ***Placeholder
/*005*/	int8  level;				// Level of person who casted buff
/*006*/	int16  effect;				// ***Placeholder
/*008*/	int32 spellid;				// Spell
/*0012*/	int32 duration;				// Duration in ticks
};
#endif

// Length: 24
struct SpellBuffFade_Struct {
/*000*/	uint32 entityid;
/*004*/	int8 slot;
/*005*/	int8 level;
/*006*/	int8 effect;
/*007*/	int8 unknown7;
/*008*/	uint32 spellid;
/*012*/	uint32 unknown012;
/*016*/	uint32 unknown016;
/*020*/	uint32 slotid;
/*024*/	uint32 bufffade;
/*028*/
};

// Length: 10
struct ItemProperties_Struct {

int8	unknown01[2];
int8	charges;
int8	unknown02[13];
};

struct GMTrainee_Struct
{
	/*000*/ uint32 npcid;
	/*004*/ uint32 playerid;
	/*008*/ uint32 skills[73];
	/*300*/ int8 unknown300[148];
	/*448*/
};

struct GMTrainEnd_Struct
{
	/*000*/ uint32 npcid;
	/*004*/ uint32 playerid;
	/*008*/
};

struct GMSkillChange_Struct {
/*000*/	int16		npcid;
/*002*/ int8		unknown1[2];    // something like PC_ID, but not really. stays the same thru the session though
/*002*/ int16       skillbank;      // 0 if normal skills, 1 if languages
/*002*/ int8		unknown2[2];
/*008*/ uint16		skill_id;
/*010*/ int8		unknown3[2];
};

/* 
** Diety List
*/
#define DEITY_UNKNOWN			0
#define DEITY_AGNOSTIC			396
#define DEITY_BRELL				202
#define DEITY_CAZIC				203
#define DEITY_EROL				204
#define DEITY_BRISTLE			205
#define DEITY_INNY				206
#define DEITY_KARANA			207
#define DEITY_MITH				208
#define DEITY_PREXUS			209
#define DEITY_QUELLIOUS			210
#define DEITY_RALLOS			211
#define DEITY_SOLUSEK			213
#define DEITY_TRIBUNAL			214
#define DEITY_TUNARE			215

//Guessed:
#define DEITY_BERT				201	
#define DEITY_RODCET			212
#define DEITY_VEESHAN			216

/*
** Name Generator Struct
** Length: 72 bytes
** OpCode: 0x0290
*/
struct NameGeneration_Struct
{
/*0000*/	int32	race;
/*0004*/	int32	gender;
/*0008*/	char	name[64];
/*0072*/
};

/*
** Character Creation struct
** Length: 140 Bytes
** OpCode: 0x0113
*/
struct CharCreate_Struct
{
	/*0000*/	int32	class_; //guess
	/*0004*/	char	name[64];
	/*0068*/	int32	beardcolor;	// credit goes to vesuvias for appearance stuff
	/*0072*/	int32	beard;
	/*0076*/	int32	haircolor;
	/*0080*/	sint32	gender;
	/*0084*/	sint32	race;
	/*0088*/	sint32	start_zone;
	/*0092*/	sint32	hairstyle;
	/*0096*/	int32	deity;
	///*0072*/	sint32	deity;
	
	
	// 0 = odus
	// 1 = qeynos
	// 2 = halas
	// 3 = rivervale
	// 4 = freeport
	// 5 = neriak
	// 6 = gukta/grobb
	// 7 = ogguk
	// 8 = kaladim
	// 9 = gfay
	// 10 = felwithe
	// 11 = akanon
	// 12 = cabalis
	// 13 = shar vahl


/*0100*/	sint32	STR;
/*0104*/	sint32	STA;
/*0108*/	sint32	AGI;
/*0112*/	sint32	DEX;
/*0116*/	sint32	WIS;
/*0120*/	sint32	INT;
/*0124*/	sint32	CHA;
/*0128*/	int32	face;
/*0132*/	int32	eyecolor1;	//its possiable we could have these switched
/*0136*/	int32	eyecolor2;	//since setting one sets the other we really can't check
/*0140*/	int32	unknown140;
};

/*
 *Used in PlayerProfile
 */
struct AA_Array
{
	int32 AA;
	int32 value;	
};

struct Disciplines_Struct {
	uint32 disciplines[50];
};

/*
** Player Profile
**
** Length: 4308 bytes
** OpCode: 0x006a
 */
#define MAX_PP_LANGUAGE		28
#define MAX_PP_SPELLBOOK	400
#define MAX_PP_MEMSPELL		8
#define MAX_PP_SKILL		75
struct PlayerProfile_Struct
{
/*0000*/	uint32				checksum;			// Checksum from CRC32::SetEQChecksum
/*0004*/	char				name[64];			// Name of player sizes not right
/*0068*/	char				last_name[32];		// Last name of player sizes not right
/*0100*/	uint32				gender;				// Player Gender - 0 Male, 1 Female
/*0104*/	uint32				race;				// Player race
/*0108*/	uint32				class_;				// Player class
/*0112*/	uint32				unknown0112;		//
/*0116*/	uint32				level;				// Level of player (might be one byte)
/*0120*/	uint32				bind_zone_id;		// Zone player is bound in
/*0124*/	uint32				unknown0124[4];
/*0140*/	float				bind_x[4];				// Bind loc x coord
/*0156*/	float				zone_safe_x;
/*0160*/	float				bind_y[4];				// Bind loc y coord
/*0176*/	float				zone_safe_y;
/*0180*/	float				bind_z[4];				// Bind loc z coord
/*0196*/	float				zone_safe_z;
/*0200*/	float				bind_heading[4];		//
/*0216*/	float				zone_safe_heading;
/*0220*/	uint32				deity;				// deity
/*0224*/	uint32				guildid;				
/*0228*/	uint32				birthday;			// characters bday
/*0232*/	uint32				lastlogin;			// last login or zone time
/*0236*/	uint32				timeplayed;			// in minutes
/*0240*/	uint8				fatigue;			// Sta bar % depleted (ie, 30 = 70% sta)
/*0241*/	uint8				pvp;
/*0242*/	uint8				unknown162;
/*0243*/	uint8				gm;				// 1=gm, 0=not gm
/*0244*/	uint8				anon;		// 2=roleplay, 1=anon, 0=not anon
/*0245*/	uint8				guildrank;					
/*0246*/	uint8				unknown0165[46];	//
/*0292*/	uint8				haircolor;			// Player hair color
/*0293*/	uint8				beardcolor;			// Player beard color
/*0294*/	uint8				eyecolor1;			// Player left eye color
/*0295*/	uint8				eyecolor2;			// Player right eye color
/*0296*/	uint8				hairstyle;			// Player hair style
/*0297*/	uint8				beard;				// Beard type
/*0298*/	uint8				ability_time_seconds; //The following four spots are unknown right now.....
/*0299*/	uint8				ability_number; //ability used
/*0300*/	uint8				ability_time_minutes;
/*0301*/	uint8				ability_time_hours;//place holder
/*0302*/	uint8				unknown0218[2];		// @bp Spacer/Flag?
/*0304*/	uint32				item_material[9];	// Item texture/material of worn/held items
/*0340*/	uint8				unknown0256[48];
/*0388*/	Color_Struct		item_tint[9];
/*0424*/	AA_Array			aa_array[120];
/*1384*/	uint8				unknown1388[4];
/*1388*/	char				servername[100];		// length probably not right
/*1488*/	uint32				guildid2;		//
/*1492*/	uint32				exp;				// Current Experience
/*1496*/	uint32				unknown1496;		
/*1500*/	uint32				points;				// Unspent Practice points
/*1504*/	uint32				mana;				// current mana
/*1508*/	uint32				cur_hp;				// current hp
/*1512*/	uint32				unknown1512;		// 0x05
/*1516*/	uint32				STR;				// Strength
/*1520*/	uint32				STA;				// Stamina
/*1524*/	uint32				CHA;				// Charisma
/*1528*/	uint32				DEX;				// Dexterity
/*1532*/	uint32				INT;				// Intelligence
/*1536*/	uint32				AGI;				// Agility
/*1540*/	uint32				WIS;				// Wisdom
/*1544*/	uint8				face;				// Player face
/*1545*/	uint8				unknown1545[11];	// ?
/*1556*/	int32				unknown1556[9];
/*1592*/	uint8				languages[MAX_PP_LANGUAGE];
/*1620*/	uint32				unknown1620;
/*1624*/	int32				spell_book[MAX_PP_SPELLBOOK];
/*3224*/	uint8				unknown3224[448];	// all 0xff   
/*3672*/	int32				mem_spells[MAX_PP_MEMSPELL];
/*3704*/	int32				unknown3704[8];	//
/*3736*/	int32				unknown3736;		//
/*3740*/	float				y;					// Player y position
/*3744*/	float				x;					// Player x position
/*3748*/	float				z;					// Player z position
/*3752*/	float				heading;			// Direction player is facing
/*3756*/	uint32				unknown3756;		//
/*3760*/	sint32				platinum;			// Platinum Pieces on player
/*3764*/	sint32				gold;				// Gold Pieces on player
/*3768*/	sint32				silver;				// Silver Pieces on player
/*3772*/	sint32				copper;				// Copper Pieces on player
/*3776*/	sint32				platinum_bank;		// Platinum Pieces in Bank
/*3780*/	sint32				gold_bank;			// Gold Pieces in Bank
/*3784*/	sint32				silver_bank;		// Silver Pieces in Bank
/*3788*/	sint32				copper_bank;		// Copper Pieces in Bank
/*3792*/	sint32				platinum_cursor;	// Platinum on cursor
/*3796*/	sint32				gold_cursor;		// Gold on cursor
/*3800*/	sint32				silver_cursor;		// Silver on cursor
/*3804*/	sint32				copper_cursor;		// Copper on cursor
/*3808*/	sint32				platinum_shared;	// Platinum shared between characters
/*3812*/	uint8				unknown2972[16];   	// @bp unknown skills?
/*3832*/	uint32				skills[MAX_PP_SKILL];
/*4132*/	uint32				unknown_skills[45];	// @bp unknown skills?
/*4308*/	uint8				unknown3472[92];	//
/*4400*/	uint32				perAA;				// % on the AA exp bar
/*4404*/	uint8				unknown3564[12];	//28
/*4416*/	uint8				pvp2;	//
/*4417*/	uint8				unknown3591[15];	//
/*4432*/	uint32				zone_change_count;	// Number of times user has zoned in their career (guessing)
/*4436*/	uint8				unknown3596[20];	//
/*4456*/	int32				ability_down;
/*4460*/	uint8				unknown3620[20];	//
/*4480*/	int32				expAA;
/*4484*/	int32				unknown3644;
/*4488*/	int32				expansion;		// expansion setting
/*4492*/	sint32				unknown3648;
/*4496*/	char				unknown3656[16];	//
/*4512*/	sint32				hunger_level;
/*4516*/	sint32				thirst_level;
/*4520*/	int32				ability_up;
/*4524*/	char				unknown3688[16];				
/*4540*/	uint32				zone_id;			// Current zone of the player
/*4544*/	SpellBuff_Struct	buffs[BUFF_COUNT];			// Buffs currently on the player
/*4784*/	char 				groupMembers[6][64];		//
/*5168*/	uint8				unknown4328[4]; 		//
/*5172*/	uint32				ldon_guk_points;		// Earned Deepest Guk points - Doodman: guessing based on others that were known
/*5176*/	uint32				ldon_mirugal_points;		// Earned Mirugal' Mebagerie points - Verified by Doodman
/*5180*/	uint32				ldon_mistmoore_points;		// Earned Mismoore Catacombs Points - Verified by Doodman
/*5184*/	uint32				ldon_rujarkian_points;		// Earned Rujarkian Hills points - Verified by Doodman
/*5188*/	uint32				ldon_takish_points;		// Earned Takish points - Doodman: guessing based on others that were known
/*5192*/	uint8				unknown4352[24]; 		//
/*5216*/	uint32				ldon_available_points;		// Earned Rujarkian Hills points - Verified
/*5220*/	uint32				unknown4380[20];
/*5300*/	uint32				unknown4460[166];
/*5964*/	uint32				unknown5760; //0xFF FF FF FF
/*5968*/	uint32				unknown5968[16];
/*6032*/	Disciplines_Struct	disciplines;			//fathernitwit: 10-06-04
/*6232*/	uint32				unknown5764[1283]; //added in last patch, crazy bastards
/*11364*/	uint32				aapoints_spent;
/*11368*/	uint32				unknown11368;
/*11372*/	uint32				aapoints;
/*11376*/	uint32				unknown11376[10];
};
struct Before_Sep14th_PlayerProfile_Struct
{
/*0000*/	uint32				checksum;			// Checksum from CRC32::SetEQChecksum
/*0004*/	char				name[64];			// Name of player sizes not right
/*0068*/	char				last_name[32];		// Last name of player sizes not right
/*0100*/	uint32				gender;				// Player Gender - 0 Male, 1 Female
/*0104*/	uint32				race;				// Player race
/*0108*/	uint32				class_;				// Player class
/*0112*/	uint32				unknown0112;		//
/*0116*/	uint32				level;				// Level of player (might be one byte)
/*0120*/	uint32				bind_zone_id;		// Zone player is bound in
/*0124*/	float				bind_x;				// Bind loc x coord
/*0128*/	float				bind_y;				// Bind loc y coord
/*0132*/	float				bind_z;				// Bind loc z coord
/*0136*/	uint8				unknown0136[4];		//
/*0140*/	uint32				deity;				// deity
/*0144*/	uint32				guildid;				
/*0148*/	uint32				birthday;			// characters bday
/*0152*/	uint32				lastlogin;			// last login or zone time
/*0156*/	uint32				timeplayed;			// in minutes
/*0160*/	uint8				fatigue;			// Sta bar % depleted (ie, 30 = 70% sta)
/*0161*/	uint8				pvp;
/*0162*/	uint8				unknown162;
/*0163*/	uint8				gm;				// 1=gm, 0=not gm
/*0164*/	uint8				anon;		// 2=roleplay, 1=anon, 0=not anon
/*0165*/	uint8				guildrank;					
/*0166*/	uint8				unknown0165[42];	//
/*0208*/	uint8				ability_time_seconds;
/*0209*/	uint8				ability_number; //ability used
/*0210*/	uint8				ability_time_minutes;
/*0211*/	uint8				ability_time_hours;//place holder
/*0212*/	uint8				haircolor;			// Player hair color
/*0213*/	uint8				beardcolor;			// Player beard color
/*0214*/	uint8				eyecolor1;			// Player left eye color
/*0215*/	uint8				eyecolor2;			// Player right eye color
/*0216*/	uint8				hairstyle;			// Player hair style
/*0217*/	uint8				beard;				// Beard type

/*0218*/	uint8				unknown0218[2];		// @bp Spacer/Flag?
/*0220*/	uint32				item_material[9];	// Item texture/material of worn/held items
/*0256*/	uint8				unknown0256[52];
/*0308*/	Color_Struct		item_tint[9];
/*0344*/	AA_Array			aa_array[120];
/*1304*/	char				servername[64];		// length probably not right
/*1368*/	uint32				guildid2;		//
/*1372*/	uint32				exp;				// Current Experience
/*1376*/	uint32				points;				// Unspent Practice points
/*1380*/	uint32				mana;				// current mana
/*1384*/	uint32				cur_hp;				// current hp
/*1388*/	uint32				unknown0672;		// 0x05
/*1392*/	uint32				STR;				// Strength
/*1396*/	uint32				STA;				// Stamina
/*1400*/	uint32				CHA;				// Charisma
/*1404*/	uint32				DEX;				// Dexterity
/*1408*/	uint32				INT;				// Intelligence
/*1412*/	uint32				AGI;				// Agility
/*1416*/	uint32				WIS;				// Wisdom
/*1420*/	uint8				face;				// Player face
/*1421*/	uint8				unknown0705[11];	// ?
/*1432*/	int32				unknown712[9];		// RR GG BB 00
/*1468*/	uint8				languages[MAX_PP_LANGUAGE];
/*1496*/	uint8				unknown0780[4];		// All 0x00
/*1500*/	int32				spell_book[MAX_PP_SPELLBOOK];
/*3100*/	uint8				unknown2384[448];	// all 0xff   
/*3548*/	int32				mem_spells[MAX_PP_MEMSPELL];
/*3580*/	uint8				unknown2864[32];	//
/*3612*/	uint32				unknown2896;		//
/*3616*/	float				y;					// Player y position
/*3620*/	float				x;					// Player x position
/*3624*/	float				z;					// Player z position
/*3628*/	float				heading;			// Direction player is facing
/*3632*/	uint8				unknown2909[4];		//
/*3636*/	sint32				platinum;			// Platinum Pieces on player
/*3640*/	sint32				gold;				// Gold Pieces on player
/*3644*/	sint32				silver;				// Silver Pieces on player
/*3648*/	sint32				copper;				// Copper Pieces on player
/*3652*/	sint32				platinum_bank;		// Platinum Pieces in Bank
/*3656*/	sint32				gold_bank;			// Gold Pieces in Bank
/*3660*/	sint32				silver_bank;		// Silver Pieces in Bank
/*3664*/	sint32				copper_bank;		// Copper Pieces in Bank
/*3668*/	sint32				platinum_cursor;	// Platinum on cursor
/*3672*/	sint32				gold_cursor;		// Gold on cursor
/*3676*/	sint32				silver_cursor;		// Silver on cursor
/*3680*/	sint32				copper_cursor;		// Copper on cursor
/*3684*/	sint32				platinum_shared;	// Platinum shared between characters
/*3688*/	uint8				unknown2972[16];   	// @bp unknown skills?
/*3708*/	uint32				skills[MAX_PP_SKILL];
/*4008*/	uint32				unknown_skills[44];	// @bp unknown skills?
/*4188*/	uint8				unknown3472[96];	//
/*4276*/	uint32				perAA;				// % on the AA exp bar
/*4280*/	uint8				unknown3564[12];	//28
/*4292*/	uint8				pvp2;	//
/*4293*/	uint8				unknown3591[15];	//
/*4308*/	uint32				zone_change_count;	// Number of times user has zoned in their career (guessing)
/*4312*/	uint8				unknown3596[20];	//
/*4332*/	int32				ability_down;
/*4336*/	uint8				unknown3620[20];	//
/*4356*/	int32				expAA;
/*4360*/	int32				unknown3644;
/*4364*/	int32				expansion;		// expansion setting
/*4368*/	sint32				unknown3648;
/*4372*/	char				unknown3656[16];	//
/*4388*/	sint32				hunger_level;
/*4392*/	sint32				thirst_level;
/*4396*/	int32				ability_up;
/*4400*/	int32				aapoints;		// Wrong but needs to be put somewhere =P
/*4404*/	char				unknown3688[12];				
/*4416*/	uint32				zone_id;			// Current zone of the player
/*4420*/	SpellBuff_Struct		buffs[BUFF_COUNT];			// Buffs currently on the player
/*4660*/	char 				groupMembers[6][64];		//
/*5044*/	uint8				unknown4328[4]; 		//
/*5048*/	uint32				ldon_guk_points;		// Earned Deepest Guk points - Doodman: guessing based on others that were known
/*5052*/	uint32				ldon_mirugal_points;		// Earned Mirugal' Mebagerie points - Verified by Doodman
/*5056*/	uint32				ldon_mistmoore_points;		// Earned Mismoore Catacombs Points - Verified by Doodman
/*5060*/	uint32				ldon_rujarkian_points;		// Earned Rujarkian Hills points - Verified by Doodman
/*5064*/	uint32				ldon_takish_points;		// Earned Takish points - Doodman: guessing based on others that were known
/*5068*/	uint8				unknown4352[24]; 		//
/*5092*/	uint32				ldon_available_points;		// Earned Rujarkian Hills points - Verified
/*5096*/	uint32				unknown4380[20];
/*5176*/	uint32				unknown4460[146];
/*5760*/	uint32				unknown5760; //0xFF FF FF FF
/*5764*/	uint32				unknown5764[1339]; //added in last patch, crazy bastards
};
struct Before_Aug13th_PlayerProfile_Struct
{
/*0000*/	uint32				checksum;			// Checksum from CRC32::SetEQChecksum
/*0004*/	char				name[64];			// Name of player sizes not right
/*0068*/	char				last_name[32];		// Last name of player sizes not right
/*0100*/	uint32				gender;				// Player Gender - 0 Male, 1 Female
/*0104*/	uint32				race;				// Player race
/*0108*/	uint32				class_;				// Player class
/*0112*/	uint32				unknown0112;		//
/*0116*/	uint32				level;				// Level of player (might be one byte)
/*0120*/	uint32				bind_zone_id;		// Zone player is bound in
/*0124*/	float				bind_x;				// Bind loc x coord
/*0128*/	float				bind_y;				// Bind loc y coord
/*0132*/	float				bind_z;				// Bind loc z coord
/*0136*/	uint8				unknown0136[4];		//
/*0140*/	uint32				deity;				// deity
/*0144*/	uint32				guildid;				
/*0148*/	uint32				birthday;			// characters bday
/*0152*/	uint32				lastlogin;			// last login or zone time
/*0156*/	uint32				timeplayed;			// in minutes
/*0160*/	uint8				fatigue;			// Sta bar % depleted (ie, 30 = 70% sta)
/*0161*/	uint8				pvp;
/*0162*/	uint8				unknown162;
/*0163*/	uint8				gm;				// 1=gm, 0=not gm
/*0164*/	uint8				anon;		// 2=roleplay, 1=anon, 0=not anon
/*0165*/	uint8				guildrank;					
/*0166*/	uint8				unknown0165[42];	//
/*0208*/	uint8				ability_time_seconds;
/*0209*/	uint8				ability_number; //ability used
/*0210*/	uint8				ability_time_minutes;
/*0211*/	uint8				ability_time_hours;//place holder
/*0212*/	uint8				haircolor;			// Player hair color
/*0213*/	uint8				beardcolor;			// Player beard color
/*0214*/	uint8				eyecolor1;			// Player left eye color
/*0215*/	uint8				eyecolor2;			// Player right eye color
/*0216*/	uint8				hairstyle;			// Player hair style
/*0217*/	uint8				beard;				// Beard type

/*0218*/	uint8				unknown0218[2];		// @bp Spacer/Flag?
/*0220*/	uint32				item_material[9];	// Item texture/material of worn/held items
/*0256*/	uint8				unknown0256[52];
/*0308*/	Color_Struct		item_tint[9];
/*0344*/	int16				aa_array[122];		// Length may not be right but i assume no class has more than this


/*0588*/	char				servername[64];		// length probably not right
/*0652*/	uint32				guildid2;		//
/*0656*/	uint32				exp;				// Current Experience
/*0660*/	uint32				points;				// Unspent Practice points
/*0664*/	uint32				mana;				// current mana
/*0668*/	uint32				cur_hp;				// current hp
/*0672*/	uint32				unknown0672;		// 0x05
/*0676*/	uint32				STR;				// Strength
/*0680*/	uint32				STA;				// Stamina
/*0684*/	uint32				CHA;				// Charisma
/*0688*/	uint32				DEX;				// Dexterity
/*0692*/	uint32				INT;				// Intelligence
/*0696*/	uint32				AGI;				// Agility
/*0700*/	uint32				WIS;				// Wisdom
/*0704*/	uint8				face;				// Player face
/*0705*/	uint8				unknown0705[11];	// ?
/*0716*/	int32				unknown712[9];		// RR GG BB 00
/*0752*/	uint8				languages[MAX_PP_LANGUAGE];
/*0780*/	uint8				unknown0780[4];		// All 0x00
/*0784*/	int32				spell_book[MAX_PP_SPELLBOOK];
/*2384*/	uint8				unknown2384[448];	// all 0xff   
/*2832*/	int32				mem_spells[MAX_PP_MEMSPELL];
/*2864*/	uint8				unknown2864[32];	//
/*2896*/	uint32				unknown2896;		//
/*2900*/	float				y;					// Player y position
/*2904*/	float				x;					// Player x position
/*2908*/	float				z;					// Player z position
/*2912*/	float				heading;			// Direction player is facing
/*2916*/	uint8				unknown2909[4];		//
/*2920*/	sint32				platinum;			// Platinum Pieces on player
/*2924*/	sint32				gold;				// Gold Pieces on player
/*2928*/	sint32				silver;				// Silver Pieces on player
/*2932*/	sint32				copper;				// Copper Pieces on player
/*2936*/	sint32				platinum_bank;		// Platinum Pieces in Bank
/*2940*/	sint32				gold_bank;			// Gold Pieces in Bank
/*2944*/	sint32				silver_bank;		// Silver Pieces in Bank
/*2948*/	sint32				copper_bank;		// Copper Pieces in Bank
/*2952*/	sint32				platinum_cursor;	// Platinum on cursor
/*2956*/	sint32				gold_cursor;		// Gold on cursor
/*2960*/	sint32				silver_cursor;		// Silver on cursor
/*2964*/	sint32				copper_cursor;		// Copper on cursor
/*2968*/	sint32				platinum_shared;	// Platinum shared between characters
/*2972*/	uint8				unknown2972[16];   	// @bp unknown skills?
/*2992*/	uint32				skills[MAX_PP_SKILL];
/*3288*/	uint32				unknown_skills[46];	// @bp unknown skills?
/*3472*/	uint8				unknown3472[88];	//
/*3560*/	uint32				perAA;				// % on the AA exp bar
/*3564*/	uint8				unknown3564[12];	//28
/*3576*/	uint8				pvp2;	//
/*3591*/	uint8				unknown3591[15];	//
/*3592*/	uint32				zone_change_count;	// Number of times user has zoned in their career (guessing)
/*3596*/	uint8				unknown3596[20];	//
/*3616*/	int32				ability_down;
/*3620*/	uint8				unknown3620[20];	//
/*3640*/	int32				expAA;
/*3644*/	int32				unknown3644;
/*3648*/	int32				expansion;		// expansion setting
/*3652*/	sint32				unknown3648;
/*3656*/	char				unknown3656[16];	//
/*3672*/	sint32				hunger_level;
/*3676*/	sint32				thirst_level;
/*3680*/	int32				ability_up;
/*3684*/	int32				aapoints;		// Wrong but needs to be put somewhere =P
/*3688*/	char				unknown3688[12];				
/*3700*/	uint32				zone_id;			// Current zone of the player
/*3704*/	SpellBuff_Struct		buffs[BUFF_COUNT];			// Buffs currently on the player
/*3944*/	char 				groupMembers[6][64];		//
/*4328*/	uint8				unknown4328[4]; 		//
/*4332*/	uint32				ldon_guk_points;		// Earned Deepest Guk points - Doodman: guessing based on others that were known
/*4336*/	uint32				ldon_mirugal_points;		// Earned Mirugal' Mebagerie points - Verified by Doodman
/*4340*/	uint32				ldon_mistmoore_points;		// Earned Mismoore Catacombs Points - Verified by Doodman
/*4344*/	uint32				ldon_rujarkian_points;		// Earned Rujarkian Hills points - Verified by Doodman
/*4348*/	uint32				ldon_takish_points;		// Earned Takish points - Doodman: guessing based on others that were known
/*4352*/	uint8				unknown4352[24]; 		//
/*4376*/	uint32				ldon_available_points;		// Earned Rujarkian Hills points - Verified
/*4380*/	uint32				unknown4380[20];
/*4460*/	uint32				unknown4460[146];
/*5044*/	uint32				unknown5040; //0xFF FF FF FF
/*5048*/	uint32				unknown4384[132];
/*5576*/	uint32				unknown4644[46];
/*5760*/	uint32				unknown5760[1160]; //added in last patch, crazy bastards
};
struct PlayerProfile_Struct_Before_May26th
{
/*0000*/	uint32				checksum;			// Checksum from CRC32::SetEQChecksum
/*0004*/	char				name[64];			// Name of player sizes not right
/*0068*/	char				last_name[32];		// Last name of player sizes not right
/*0100*/	uint32				gender;				// Player Gender - 0 Male, 1 Female
/*0104*/	uint32				race;				// Player race
/*0108*/	uint32				class_;				// Player class
/*0112*/	uint32				unknown0112;		//
/*0116*/	uint32				level;				// Level of player (might be one byte)
/*0120*/	uint32				bind_zone_id;		// Zone player is bound in
/*0124*/	float				bind_x;				// Bind loc x coord
/*0128*/	float				bind_y;				// Bind loc y coord
/*0132*/	float				bind_z;				// Bind loc z coord
/*0136*/	uint8				unknown0136[4];		//
/*0140*/	uint32				deity;				// deity
/*0144*/	uint32				guildid;				
/*0148*/	uint32				birthday;			// characters bday
/*0152*/	uint32				lastlogin;			// last login or zone time
/*0156*/	uint32				timeplayed;			// in minutes
/*0160*/	uint8				fatigue;			// Sta bar % depleted (ie, 30 = 70% sta)
/*0161*/	uint8				pvp;
/*0162*/	uint8				unknown162;
/*0163*/	uint8				gm;				// 1=gm, 0=not gm
/*0164*/	uint8				anon;		// 2=roleplay, 1=anon, 0=not anon
/*0165*/	uint8				guildrank;					
/*0166*/	uint8				unknown0165[42];	//
/*0208*/	uint8				ability_time_seconds;
/*0209*/	uint8				ability_number; //ability used
/*0210*/	uint8				ability_time_minutes;
/*0211*/	uint8				ability_time_hours;//place holder
/*0212*/	uint8				haircolor;			// Player hair color
/*0213*/	uint8				beardcolor;			// Player beard color
/*0214*/	uint8				eyecolor1;			// Player left eye color
/*0215*/	uint8				eyecolor2;			// Player right eye color
/*0216*/	uint8				hairstyle;			// Player hair style
/*0217*/	uint8				beard;				// Beard type

/*0218*/	uint8				unknown0218[2];		// @bp Spacer/Flag?
/*0220*/	uint32				item_material[9];	// Item texture/material of worn/held items
/*0256*/	uint8				unknown0256[52];
/*0308*/	Color_Struct		item_tint[9];
/*0344*/	int16				aa_array[122];		// Length may not be right but i assume no class has more than this

/*0588*/	char				servername[64];		// length probably not right
/*0652*/	uint32				guildid2;		//
/*0656*/	uint32				exp;				// Current Experience
/*0660*/	uint32				points;				// Unspent Practice points
/*0664*/	uint32				mana;				// current mana
/*0668*/	uint32				cur_hp;				// current hp
/*0672*/	uint32				unknown0672;		// 0x05
/*0676*/	uint32				STR;				// Strength
/*0680*/	uint32				STA;				// Stamina
/*0684*/	uint32				CHA;				// Charisma
/*0688*/	uint32				DEX;				// Dexterity
/*0692*/	uint32				INT;				// Intelligence
/*0696*/	uint32				AGI;				// Agility
/*0700*/	uint32				WIS;				// Wisdom
/*0704*/	uint8				face;				// Player face
/*0705*/	uint8				unknown0705[11];	// ?
/*0716*/	int32				unknown712[9];		// RR GG BB 00
/*0752*/	uint8				languages[MAX_PP_LANGUAGE];
/*0780*/	uint8				unknown0780[4];		// All 0x00
/*0784*/	int32				spell_book[MAX_PP_SPELLBOOK];
/*2384*/	uint8				unknown2384[448];	// all 0xff   
/*2832*/	int32				mem_spells[MAX_PP_MEMSPELL];
/*2864*/	uint8				unknown2864[32];	//
/*2896*/	uint32				unknown2896;		//
/*2900*/	float				y;					// Player y position
/*2904*/	float				x;					// Player x position
/*2908*/	float				z;					// Player z position
/*2912*/	float				heading;			// Direction player is facing
/*2916*/	uint8				unknown2909[4];		//
/*2920*/	sint32				platinum;			// Platinum Pieces on player
/*2924*/	sint32				gold;				// Gold Pieces on player
/*2928*/	sint32				silver;				// Silver Pieces on player
/*2932*/	sint32				copper;				// Copper Pieces on player
/*2936*/	sint32				platinum_bank;		// Platinum Pieces in Bank
/*2940*/	sint32				gold_bank;			// Gold Pieces in Bank
/*2944*/	sint32				silver_bank;		// Silver Pieces in Bank
/*2948*/	sint32				copper_bank;		// Copper Pieces in Bank
/*2952*/	sint32				platinum_cursor;	// Platinum on cursor
/*2956*/	sint32				gold_cursor;		// Gold on cursor
/*2960*/	sint32				silver_cursor;		// Silver on cursor
/*2964*/	sint32				copper_cursor;		// Copper on cursor
/*2968*/	sint32				platinum_shared;	// Platinum shared between characters
/*2972*/	uint8				unknown2972[16];   	// @bp unknown skills?
/*2992*/	uint32				skills[MAX_PP_SKILL];
/*3288*/	uint32				unknown_skills[46];	// @bp unknown skills?
/*3472*/	uint8				unknown3472[88];	//
/*3560*/	uint32				perAA;				// % on the AA exp bar
/*3564*/	uint8				unknown3564[12];	//28
/*3576*/	uint8				pvp2;	//
/*3591*/	uint8				unknown3591[15];	//
/*3592*/	uint32				zone_change_count;	// Number of times user has zoned in their career (guessing)
/*3596*/	uint8				unknown3596[20];	//
/*3616*/	int32				ability_down;
/*3620*/	uint8				unknown3620[20];	//
/*3640*/	int32				expAA;
/*3644*/	int32				unknown3644;
/*3648*/	int32				expansion;		// expansion setting
/*3652*/	sint32				unknown3648;
/*3656*/	char				unknown3656[16];	//
/*3672*/	sint32				hunger_level;
/*3676*/	sint32				thirst_level;
/*3680*/	int32				ability_up;
/*3684*/	int32				aapoints;		// Wrong but needs to be put somewhere =P
/*3688*/	char				unknown3688[12];				
/*3700*/	uint32				zone_id;			// Current zone of the player
/*3704*/	SpellBuff_Struct		buffs[BUFF_COUNT];			// Buffs currently on the player
/*3944*/	char 				groupMembers[6][64];		//
/*4328*/	uint8				unknown4328[4]; 		//
/*4332*/	uint32				ldon_guk_points;		// Earned Deepest Guk points - Doodman: guessing based on others that were known
/*4336*/	uint32				ldon_mirugal_points;		// Earned Mirugal' Mebagerie points - Verified by Doodman
/*4340*/	uint32				ldon_mistmoore_points;		// Earned Mismoore Catacombs Points - Verified by Doodman
/*4344*/	uint32				ldon_rujarkian_points;		// Earned Rujarkian Hills points - Verified by Doodman
/*4348*/	uint32				ldon_takish_points;		// Earned Takish points - Doodman: guessing based on others that were known
/*4352*/	uint8				unknown4352[24]; 		//
/*4376*/	uint32				ldon_available_points;		// Earned Rujarkian Hills points - Verified
/*4380*/	uint32				unknown4380[20];
/*4460*/	uint32				unknown4460[146];
/*5044*/	uint32				unknown5040; //0xFF FF FF FF
/*5048*/	uint32				unknown4384[132];
/*5576*/	uint32				unknown4644[46];
/*5760*/	uint32				unknown5760[1158]; //added in last patch, crazy bastards
};
struct BeforeMay5th_PlayerProfile_Struct
{
/*0000*/	uint32				checksum;			// Checksum from CRC32::SetEQChecksum
/*0004*/	char				name[64];			// Name of player sizes not right
/*0068*/	char				last_name[32];		// Last name of player sizes not right
/*0100*/	uint32				gender;				// Player Gender - 0 Male, 1 Female
/*0104*/	uint32				race;				// Player race
/*0108*/	uint32				class_;				// Player class
/*0112*/	uint32				unknown0112;		//
/*0116*/	uint32				level;				// Level of player (might be one byte)
/*0120*/	uint32				bind_zone_id;		// Zone player is bound in
/*0124*/	float				bind_x;				// Bind loc x coord
/*0128*/	float				bind_y;				// Bind loc y coord
/*0132*/	float				bind_z;				// Bind loc z coord
/*0136*/	uint8				unknown0136[4];		//
/*0140*/	uint32				deity;				// deity
/*0144*/	uint32				guildid;				
/*0148*/	uint32				birthday;			// characters bday
/*0152*/	uint32				lastlogin;			// last login or zone time
/*0156*/	uint32				timeplayed;			// in minutes
/*0160*/	uint8				fatigue;			// Sta bar % depleted (ie, 30 = 70% sta)
/*0161*/	uint8				pvp;
/*0162*/	uint8				unknown162;
/*0163*/	uint8				gm;				// 1=gm, 0=not gm
/*0164*/	uint8				anon;		// 2=roleplay, 1=anon, 0=not anon
/*0165*/	uint8				guildrank;					
/*0166*/	uint8				unknown0165[42];	//
/*0208*/	uint8				ability_time_seconds;
/*0209*/	uint8				ability_number; //ability used
/*0210*/	uint8				ability_time_minutes;
/*0211*/	uint8				ability_time_hours;//place holder
/*0212*/	uint8				haircolor;			// Player hair color
/*0213*/	uint8				beardcolor;			// Player beard color
/*0214*/	uint8				eyecolor1;			// Player left eye color
/*0215*/	uint8				eyecolor2;			// Player right eye color
/*0216*/	uint8				hairstyle;			// Player hair style
/*0217*/	uint8				beard;				// Beard type

/*0218*/	uint8				unknown0218[2];		// @bp Spacer/Flag?
/*0220*/	uint32				item_material[9];	// Item texture/material of worn/held items
/*0256*/	uint8				unknown0256[52];
/*0308*/	Color_Struct		item_tint[9];
/*0344*/	int16			aa_array[122];		// Length may not be right but i assume no class has more than this

/*0588*/	char				servername[64];		// length probably not right
/*0652*/	uint32				guildid2;		//
/*0656*/	uint32				exp;				// Current Experience
/*0660*/	uint32				points;				// Unspent Practice points
/*0664*/	uint32				mana;				// current mana
/*0668*/	uint32				cur_hp;				// current hp
/*0672*/	uint32				unknown0672;		// 0x05
/*0676*/	uint32				STR;				// Strength
/*0680*/	uint32				STA;				// Stamina
/*0684*/	uint32				CHA;				// Charisma
/*0688*/	uint32				DEX;				// Dexterity
/*0692*/	uint32				INT;				// Intelligence
/*0696*/	uint32				AGI;				// Agility
/*0700*/	uint32				WIS;				// Wisdom
/*0704*/	uint8				face;				// Player face
/*0705*/	uint8				unknown0705[11];	// ?
/*0716*/	int32				unknown712[9];		// RR GG BB 00
/*0752*/	uint8				languages[MAX_PP_LANGUAGE];
/*0780*/	uint8				unknown0780[4];		// All 0x00
/*0784*/	int32				spell_book[MAX_PP_SPELLBOOK];
/*2384*/	uint8				unknown2384[448];	// all 0xff   
/*2832*/	int32				mem_spells[MAX_PP_MEMSPELL];
/*2864*/	uint8				unknown2864[32];	//
/*2896*/	uint32				unknown2896;		//
/*2900*/	float				y;					// Player y position
/*2904*/	float				x;					// Player x position
/*2908*/	float				z;					// Player z position
/*2912*/	float				heading;			// Direction player is facing
/*2916*/	uint8				unknown2909[4];		//
/*2920*/	sint32				platinum;			// Platinum Pieces on player
/*2924*/	sint32				gold;				// Gold Pieces on player
/*2928*/	sint32				silver;				// Silver Pieces on player
/*2932*/	sint32				copper;				// Copper Pieces on player
/*2936*/	sint32				platinum_bank;		// Platinum Pieces in Bank
/*2940*/	sint32				gold_bank;			// Gold Pieces in Bank
/*2944*/	sint32				silver_bank;		// Silver Pieces in Bank
/*2948*/	sint32				copper_bank;		// Copper Pieces in Bank
/*2952*/	sint32				platinum_cursor;	// Platinum on cursor
/*2956*/	sint32				gold_cursor;		// Gold on cursor
/*2960*/	sint32				silver_cursor;		// Silver on cursor
/*2964*/	sint32				copper_cursor;		// Copper on cursor
/*2968*/	sint32				platinum_shared;	// Platinum shared between characters
/*2972*/	uint8				unknown2972[16];   	// @bp unknown skills?
/*2992*/	uint32				skills[MAX_PP_SKILL];
/*3288*/	uint32				unknown_skills[46];	// @bp unknown skills?
/*3472*/	uint8				unknown3472[88];	//
/*3560*/	uint32				perAA;				// % on the AA exp bar
/*3564*/	uint8				unknown3564[12];	//28
/*3576*/	uint8				pvp2;	//
/*3591*/	uint8				unknown3591[15];	//
/*3592*/	uint32				zone_change_count;	// Number of times user has zoned in their career (guessing)
/*3596*/	uint8				unknown3596[20];	//
/*3616*/	int32				ability_down;
/*3620*/	uint8				unknown3620[20];	//
/*3640*/	int32				expAA;
/*3644*/	int32				unknown3644;
/*3648*/	int32				expansion;		// expansion setting
/*3652*/	sint32				unknown3648;
/*3656*/	char				unknown3656[16];	//
/*3672*/	sint32				hunger_level;
/*3676*/	sint32				thirst_level;
/*3680*/	int32				ability_up;
/*3684*/	int32				aapoints;		// Wrong but needs to be put somewhere =P
/*3688*/	char				unknown3688[12];				
/*3700*/	uint32				zone_id;			// Current zone of the player
/*3704*/	SpellBuff_Struct		buffs[BUFF_COUNT];			// Buffs currently on the player
/*3944*/	char 				groupMembers[6][64];		//
/*4328*/	uint8				unknown4328[4]; 		//
/*4332*/	uint32				ldon_guk_points;		// Earned Deepest Guk points - Doodman: guessing based on others that were known
/*4336*/	uint32				ldon_mirugal_points;		// Earned Mirugal' Mebagerie points - Verified by Doodman
/*4340*/	uint32				ldon_mistmoore_points;		// Earned Mismoore Catacombs Points - Verified by Doodman
/*4344*/	uint32				ldon_rujarkian_points;		// Earned Rujarkian Hills points - Verified by Doodman
/*4348*/	uint32				ldon_takish_points;		// Earned Takish points - Doodman: guessing based on others that were known
/*4352*/	uint8				unknown4352[24]; 		//
/*4376*/	uint32				ldon_available_points;		// Earned Rujarkian Hills points - Verified
/*4380*/	uint32				unknown4380[20];
/*4460*/	uint32				unknown4460[146];
/*5044*/	uint32				unknown5040; //0xFF FF FF FF
/*5048*/	uint32				unknown4384[132];
/*5576*/	uint32				unknown4644[46];
/*5760*/	uint32				unknown5760[1152]; //added in last patch, crazy bastards
};
struct BeforeApr21st_PlayerProfile_Struct
{
/*0000*/	uint32				checksum;			// Checksum from CRC32::SetEQChecksum
/*0004*/	char				name[64];			// Name of player sizes not right
/*0068*/	char				last_name[32];		// Last name of player sizes not right
/*0100*/	uint32				gender;				// Player Gender - 0 Male, 1 Female
/*0104*/	uint32				race;				// Player race
/*0108*/	uint32				class_;				// Player class
/*0112*/	uint32				unknown0112;		//
/*0116*/	uint32				level;				// Level of player (might be one byte)
/*0120*/	uint32				bind_zone_id;		// Zone player is bound in
/*0124*/	float				bind_x;				// Bind loc x coord
/*0128*/	float				bind_y;				// Bind loc y coord
/*0132*/	float				bind_z;				// Bind loc z coord
/*0136*/	uint8				unknown0136[4];		//
/*0140*/	uint32				deity;				// deity
/*0144*/	uint32				guildid;				
/*0148*/	uint32				birthday;			// characters bday
/*0152*/	uint32				lastlogin;			// last login or zone time
/*0156*/	uint32				timeplayed;			// in minutes
/*0160*/	uint8				fatigue;			// Sta bar % depleted (ie, 30 = 70% sta)
/*0161*/	uint8				pvp;
/*0162*/	uint8				unknown162;
/*0163*/	uint8				gm;				// 1=gm, 0=not gm
/*0164*/	uint8				anon;		// 2=roleplay, 1=anon, 0=not anon
/*0165*/	uint8				guildrank;					
/*0166*/	uint8				unknown0165[42];	//
/*0208*/	uint8				ability_time_seconds;
/*0209*/	uint8				ability_number; //ability used
/*0210*/	uint8				ability_time_minutes;
/*0211*/	uint8				ability_time_hours;//place holder
/*0212*/	uint8				haircolor;			// Player hair color
/*0213*/	uint8				beardcolor;			// Player beard color
/*0214*/	uint8				eyecolor1;			// Player left eye color
/*0215*/	uint8				eyecolor2;			// Player right eye color
/*0216*/	uint8				hairstyle;			// Player hair style
/*0217*/	uint8				beard;				// Beard type

/*0218*/	uint8				unknown0218[2];		// @bp Spacer/Flag?
/*0220*/	uint32				item_material[9];	// Item texture/material of worn/held items
/*0256*/	uint8				unknown0256[52];
/*0308*/	Color_Struct		item_tint[9];
/*0344*/	int16				aa_array[122];		// Length may not be right but i assume no class has more than this

/*0588*/	char				servername[64];		// length probably not right
/*0652*/	uint32				guildid2;		//
/*0656*/	uint32				exp;				// Current Experience
/*0660*/	uint32				points;				// Unspent Practice points
/*0664*/	uint32				mana;				// current mana
/*0668*/	uint32				cur_hp;				// current hp
/*0672*/	uint32				unknown0672;		// 0x05
/*0676*/	uint32				STR;				// Strength
/*0680*/	uint32				STA;				// Stamina
/*0684*/	uint32				CHA;				// Charisma
/*0688*/	uint32				DEX;				// Dexterity
/*0692*/	uint32				INT;				// Intelligence
/*0696*/	uint32				AGI;				// Agility
/*0700*/	uint32				WIS;				// Wisdom
/*0704*/	uint8				face;				// Player face
/*0705*/	uint8				unknown0705[11];	// ?
/*0716*/	int32				unknown712[9];		// RR GG BB 00
/*0752*/	uint8				languages[MAX_PP_LANGUAGE];
/*0780*/	uint8				unknown0780[4];		// All 0x00
/*0784*/	int32				spell_book[MAX_PP_SPELLBOOK];
/*2384*/	uint8				unknown2384[448];	// all 0xff   
/*2832*/	int32				mem_spells[MAX_PP_MEMSPELL];
/*2864*/	uint8				unknown2864[32];	//
/*2896*/	uint32				unknown2896;		//
/*2900*/	float				y;					// Player y position
/*2904*/	float				x;					// Player x position
/*2908*/	float				z;					// Player z position
/*2912*/	float				heading;			// Direction player is facing
/*2916*/	uint8				unknown2909[4];		//
/*2920*/	sint32				platinum;			// Platinum Pieces on player
/*2924*/	sint32				gold;				// Gold Pieces on player
/*2928*/	sint32				silver;				// Silver Pieces on player
/*2932*/	sint32				copper;				// Copper Pieces on player
/*2936*/	sint32				platinum_bank;		// Platinum Pieces in Bank
/*2940*/	sint32				gold_bank;			// Gold Pieces in Bank
/*2944*/	sint32				silver_bank;		// Silver Pieces in Bank
/*2948*/	sint32				copper_bank;		// Copper Pieces in Bank
/*2952*/	sint32				platinum_cursor;	// Platinum on cursor
/*2956*/	sint32				gold_cursor;		// Gold on cursor
/*2960*/	sint32				silver_cursor;		// Silver on cursor
/*2964*/	sint32				copper_cursor;		// Copper on cursor
/*2968*/	sint32				platinum_shared;	// Platinum shared between characters
/*2972*/	uint8				unknown2972[16];   	// @bp unknown skills?
/*2992*/	uint32				skills[MAX_PP_SKILL];
/*3288*/	uint32				unknown_skills[46];	// @bp unknown skills?
/*3472*/	uint8				unknown3472[88];	//
/*3560*/	uint32				perAA;				// % on the AA exp bar
/*3564*/	uint8				unknown3564[12];	//28
/*3576*/	uint8				pvp2;	//
/*3591*/	uint8				unknown3591[15];	//
/*3592*/	uint32				zone_change_count;	// Number of times user has zoned in their career (guessing)
/*3596*/	uint8				unknown3596[20];	//
/*3616*/	int32				ability_down;
/*3620*/	uint8				unknown3620[20];	//
/*3640*/	int32				expAA;
/*3644*/	int32				unknown3644;
/*3648*/	int32				expansion;		// expansion setting
/*3652*/	sint32				unknown3648;
/*3656*/	char				unknown3656[16];	//
/*3672*/	sint32				hunger_level;
/*3676*/	sint32				thirst_level;
/*3680*/	int32				ability_up;
/*3684*/	int32				aapoints;		// Wrong but needs to be put somewhere =P
/*3688*/	char				unknown3688[12];				
/*3700*/	uint32				zone_id;			// Current zone of the player
/*3704*/	SpellBuff_Struct		buffs[BUFF_COUNT];			// Buffs currently on the player
/*3944*/	char 				groupMembers[6][64];		//
/*4328*/	uint8				unknown4328[4]; 		//
/*4332*/	uint32				ldon_guk_points;		// Earned Deepest Guk points - Doodman: guessing based on others that were known
/*4336*/	uint32				ldon_mirugal_points;		// Earned Mirugal' Mebagerie points - Verified by Doodman
/*4340*/	uint32				ldon_mistmoore_points;		// Earned Mismoore Catacombs Points - Verified by Doodman
/*4344*/	uint32				ldon_rujarkian_points;		// Earned Rujarkian Hills points - Verified by Doodman
/*4348*/	uint32				ldon_takish_points;		// Earned Takish points - Doodman: guessing based on others that were known
/*4352*/	uint8				unknown4352[24]; 		//
/*4376*/	uint32				ldon_available_points;		// Earned Rujarkian Hills points - Verified
/*4380*/	uint32				unknown4380[20];
/*4460*/	uint32				unknown4460[146];
/*5044*/	uint32				unknown5040; //0xFF FF FF FF
/*5048*/	uint32				unknown4384[132];
/*5576*/	uint32				unknown4644[46];
};
struct BeforeApril14th_PlayerProfile_Struct
{
/*0000*/	uint32				checksum;			// Checksum from CRC32::SetEQChecksum
/*0004*/	char				name[64];			// Name of player sizes not right
/*0068*/	char				last_name[32];		// Last name of player sizes not right
/*0100*/	uint32				gender;				// Player Gender - 0 Male, 1 Female
/*0104*/	uint32				race;				// Player race
/*0108*/	uint32				class_;				// Player class
/*0112*/	uint32				unknown0112;		//
/*0116*/	uint32				level;				// Level of player (might be one byte)
/*0120*/	uint32				bind_zone_id;		// Zone player is bound in
/*0124*/	float				bind_x;				// Bind loc x coord
/*0128*/	float				bind_y;				// Bind loc y coord
/*0132*/	float				bind_z;				// Bind loc z coord
/*0136*/	uint8				unknown0136[4];		//
/*0140*/	uint32				deity;				// deity
/*0144*/	uint32				guildid;				
/*0148*/	uint32				birthday;			// characters bday
/*0152*/	uint32				lastlogin;			// last login or zone time
/*0156*/	uint32				timeplayed;			// in minutes
/*0160*/	uint8				fatigue;			// Sta bar % depleted (ie, 30 = 70% sta)
/*0161*/	uint8				pvp;
/*0162*/	uint8				unknown162;
/*0163*/	uint8				gm;				// 1=gm, 0=not gm
/*0164*/	uint8				anon;		// 2=roleplay, 1=anon, 0=not anon
/*0165*/	uint8				guildrank;					
/*0166*/	uint8				unknown0165[42];	//
/*0208*/	uint8				ability_time_seconds;
/*0209*/	uint8				ability_number; //ability used
/*0210*/	uint8				ability_time_minutes;
/*0211*/	uint8				ability_time_hours;//place holder
/*0212*/	uint8				haircolor;			// Player hair color
/*0213*/	uint8				beardcolor;			// Player beard color
/*0214*/	uint8				eyecolor1;			// Player left eye color
/*0215*/	uint8				eyecolor2;			// Player right eye color
/*0216*/	uint8				hairstyle;			// Player hair style
/*0217*/	uint8				beard;				// Beard type

/*0218*/	uint8				unknown0218[2];		// @bp Spacer/Flag?
/*0220*/	uint32				item_material[9];	// Item texture/material of worn/held items
/*0256*/	uint8				unknown0256[52];
/*0308*/	Color_Struct		item_tint[9];
/*0344*/	int16				aa_array[122];		// Length may not be right but i assume no class has more than this

/*0588*/	char				servername[64];		// length probably not right
/*0652*/	uint32				guildid2;		//
/*0656*/	uint32				exp;				// Current Experience
/*0660*/	uint32				points;				// Unspent Practice points
/*0664*/	uint32				mana;				// current mana
/*0668*/	uint32				cur_hp;				// current hp
/*0672*/	uint32				unknown0672;		// 0x05
/*0676*/	uint32				STR;				// Strength
/*0680*/	uint32				STA;				// Stamina
/*0684*/	uint32				CHA;				// Charisma
/*0688*/	uint32				DEX;				// Dexterity
/*0692*/	uint32				INT;				// Intelligence
/*0696*/	uint32				AGI;				// Agility
/*0700*/	uint32				WIS;				// Wisdom
/*0704*/	uint8				face;				// Player face
/*0705*/	uint8				unknown0705[11];	// ?
/*0716*/	int32				unknown712[9];		// RR GG BB 00
/*0752*/	uint8				languages[MAX_PP_LANGUAGE];
/*0780*/	uint8				unknown0780[4];		// All 0x00
/*0784*/	int32				spell_book[MAX_PP_SPELLBOOK];
/*2384*/	uint8				unknown2384[448];	// all 0xff   
/*2832*/	int32				mem_spells[MAX_PP_MEMSPELL];
/*2864*/	uint8				unknown2864[32];	//
/*2896*/	uint32				unknown2896;		//
/*2900*/	float				y;					// Player y position
/*2904*/	float				x;					// Player x position
/*2908*/	float				z;					// Player z position
/*2912*/	float				heading;			// Direction player is facing
/*2916*/	uint8				unknown2909[4];		//
/*2920*/	sint32				platinum;			// Platinum Pieces on player
/*2924*/	sint32				gold;				// Gold Pieces on player
/*2928*/	sint32				silver;				// Silver Pieces on player
/*2932*/	sint32				copper;				// Copper Pieces on player
/*2936*/	sint32				platinum_bank;		// Platinum Pieces in Bank
/*2940*/	sint32				gold_bank;			// Gold Pieces in Bank
/*2944*/	sint32				silver_bank;		// Silver Pieces in Bank
/*2948*/	sint32				copper_bank;		// Copper Pieces in Bank
/*2952*/	sint32				platinum_cursor;	// Platinum on cursor
/*2956*/	sint32				gold_cursor;		// Gold on cursor
/*2960*/	sint32				silver_cursor;		// Silver on cursor
/*2964*/	sint32				copper_cursor;		// Copper on cursor
/*2968*/	sint32				platinum_shared;	// Platinum shared between characters
/*2972*/	uint8				unknown2972[16];   	// @bp unknown skills?
/*2992*/	uint32				skills[MAX_PP_SKILL];
/*3288*/	uint32				unknown_skills[46];	// @bp unknown skills?
/*3472*/	uint8				unknown3472[88];	//
/*3560*/	uint32				perAA;				// % on the AA exp bar
/*3564*/	uint8				unknown3564[12];	//28
/*3576*/	uint8				pvp2;	//
/*3591*/	uint8				unknown3591[15];	//
/*3592*/	uint32				zone_change_count;	// Number of times user has zoned in their career (guessing)
/*3596*/	uint8				unknown3596[20];	//
/*3616*/	int32				ability_down;
/*3620*/	uint8				unknown3620[20];	//
/*3640*/	int32				expAA;
/*3644*/	int32				unknown3644;
/*3648*/	int32				expansion;		// expansion setting
/*3652*/	sint32				unknown3648;
/*3656*/	char				unknown3656[16];	//
/*3672*/	sint32				hunger_level;
/*3676*/	sint32				thirst_level;
/*3680*/	int32				ability_up;
/*3684*/	int32				aapoints;		// Wrong but needs to be put somewhere =P
/*3688*/	char				unknown3688[12];				
/*3700*/	uint32				zone_id;			// Current zone of the player
/*3704*/	SpellBuff_Struct		buffs[BUFF_COUNT];			// Buffs currently on the player
/*3944*/	char 				groupMembers[6][64];		//
/*4328*/	uint8				unknown4328[4]; 		//
/*4332*/	uint32				ldon_guk_points;		// Earned Deepest Guk points - Doodman: guessing based on others that were known
/*4336*/	uint32				ldon_mirugal_points;		// Earned Mirugal' Mebagerie points - Verified by Doodman
/*4340*/	uint32				ldon_mistmoore_points;		// Earned Mismoore Catacombs Points - Verified by Doodman
/*4344*/	uint32				ldon_rujarkian_points;		// Earned Rujarkian Hills points - Verified by Doodman
/*4348*/	uint32				ldon_takish_points;		// Earned Takish points - Doodman: guessing based on others that were known
/*4352*/	uint8				unknown4352[24]; 		//
/*4376*/	uint32				ldon_available_points;		// Earned Rujarkian Hills points - Verified
/*4380*/	uint32				unknown4380[20];
/*4460*/	uint32				unknown4460[146];
/*5044*/	uint32				unknown5040; //0xFF FF FF FF
/*5048*/	uint32				unknown4384[112];
/*5496*/	uint32				unknown4644[46];
};

struct BeforeFeb18_PlayerProfile_Struct
{
/*0000*/	uint32				checksum;			// Checksum from CRC32::SetEQChecksum
/*0004*/	char				name[64];			// Name of player sizes not right
/*0068*/	char				last_name[32];		// Last name of player sizes not right
/*0100*/	uint32				gender;				// Player Gender - 0 Male, 1 Female
/*0104*/	uint32				race;				// Player race
/*0108*/	uint32				class_;				// Player class
/*0112*/	uint32				unknown0112;		//
/*0116*/	uint32				level;				// Level of player (might be one byte)
/*0120*/	uint32				bind_zone_id;		// Zone player is bound in
/*0124*/	float				bind_x;				// Bind loc x coord
/*0128*/	float				bind_y;				// Bind loc y coord
/*0132*/	float				bind_z;				// Bind loc z coord
/*0136*/	uint8				unknown0136[4];		//
/*0140*/	uint32				deity;				// deity
/*0144*/	uint32				guildid;				
/*0148*/	uint32				birthday;			// characters bday
/*0152*/	uint32				lastlogin;			// last login or zone time
/*0156*/	uint32				timeplayed;			// in minutes
/*0160*/	uint8				fatigue;			// Sta bar % depleted (ie, 30 = 70% sta)
/*0161*/	uint8				pvp;
/*0162*/	uint8				unknown162;
/*0163*/	uint8				anon;		// 2=roleplay, 1=anon, 0=not anon
/*0164*/	uint8				gm;				// 1=gm, 0=not gm
/*0165*/	uint8				guildrank;					
/*0166*/	uint8				unknown0165[42];	//
/*0208*/	uint8				ability_time_seconds;
/*0209*/	uint8				ability_number; //ability used
/*0210*/	uint8				ability_time_minutes;
/*0211*/	uint8				ability_time_hours;//place holder
/*0212*/	uint8				haircolor;			// Player hair color
/*0213*/	uint8				beardcolor;			// Player beard color
/*0214*/	uint8				eyecolor1;			// Player left eye color
/*0215*/	uint8				eyecolor2;			// Player right eye color
/*0216*/	uint8				hairstyle;			// Player hair style
/*0217*/	uint8				beard;				// Beard type

/*0218*/	uint8				unknown0218[2];		// @bp Spacer/Flag?
/*0220*/	uint32				item_material[9];	// Item texture/material of worn/held items
/*0256*/	uint8				unknown0256[52];
/*0308*/	Color_Struct		item_tint[9];
/*0344*/	int16				aa_array[122];		// Length may not be right but i assume no class has more than this

/*0588*/	char				servername[64];		// length probably not right
/*0652*/	uint32				guildid2;		//
/*0656*/	uint32				exp;				// Current Experience
/*0660*/	uint32				points;				// Unspent Practice points
/*0664*/	uint32				mana;				// current mana
/*0668*/	uint32				cur_hp;				// current hp
/*0672*/	uint32				unknown0672;		// 0x05
/*0676*/	uint32				STR;				// Strength
/*0680*/	uint32				STA;				// Stamina
/*0684*/	uint32				CHA;				// Charisma
/*0688*/	uint32				DEX;				// Dexterity
/*0692*/	uint32				INT;				// Intelligence
/*0696*/	uint32				AGI;				// Agility
/*0700*/	uint32				WIS;				// Wisdom
/*0704*/	uint8				face;				// Player face
/*0705*/	uint8				unknown0705[11];	// ?
/*0716*/	int32				unknown712[9];		// RR GG BB 00
/*0752*/	uint8				languages[MAX_PP_LANGUAGE];
/*0780*/	uint8				unknown0780[4];		// All 0x00
/*0784*/	int32				spell_book[MAX_PP_SPELLBOOK];
/*2384*/	uint8				unknown2384[448];	// all 0xff   
/*2832*/	int32				mem_spells[MAX_PP_MEMSPELL];
/*2864*/	uint8				unknown2864[32];	//
/*2896*/	uint32				unknown2896;		//
/*2900*/	float				y;					// Player y position
/*2904*/	float				x;					// Player x position
/*2908*/	float				z;					// Player z position
/*2912*/	float				heading;			// Direction player is facing
/*2916*/	uint8				unknown2909[4];		//
/*2920*/	uint32				platinum;			// Platinum Pieces on player
/*2924*/	uint32				gold;				// Gold Pieces on player
/*2928*/	uint32				silver;				// Silver Pieces on player
/*2932*/	uint32				copper;				// Copper Pieces on player
/*2936*/	uint32				platinum_bank;		// Platinum Pieces in Bank
/*2940*/	uint32				gold_bank;			// Gold Pieces in Bank
/*2944*/	uint32				silver_bank;		// Silver Pieces in Bank
/*2948*/	uint32				copper_bank;		// Copper Pieces in Bank
/*2952*/	uint32				platinum_cursor;	// Platinum on cursor
/*2956*/	uint32				gold_cursor;		// Gold on cursor
/*2960*/	uint32				silver_cursor;		// Silver on cursor
/*2964*/	uint32				copper_cursor;		// Copper on cursor
/*2968*/	uint32				platinum_shared;	// Platinum shared between characters
/*2972*/	uint8				unknown2972[16];   	// @bp unknown skills?
/*2992*/	uint32				skills[MAX_PP_SKILL];
/*3288*/	uint32				unknown_skills[46];	// @bp unknown skills?
/*3472*/	uint8				unknown3472[88];	//
/*3560*/	uint32				perAA;				// % on the AA exp bar
/*3564*/	uint8				unknown3564[12];	//28
/*3576*/	uint8				pvp2;	//
/*3591*/	uint8				unknown3591[15];	//
/*3592*/	uint32				zone_change_count;	// Number of times user has zoned in their career (guessing)
/*3596*/	uint8				unknown3596[20];	//
/*3616*/	int32				ability_down;
/*3620*/	uint8				unknown3620[20];	//
/*3640*/	int32				expAA;
/*3644*/	int32				unknown3644;
/*3648*/	int32				expansion;		// expansion setting
/*3652*/	sint32				unknown3648;
/*3656*/	char				unknown3656[16];	//
/*3672*/	sint32				hunger_level;
/*3676*/	sint32				thirst_level;
/*3680*/	int32				ability_up;
/*3684*/	int32				aapoints;		// Wrong but needs to be put somewhere =P
/*3688*/	char				unknown3688[12];				
/*3700*/	uint32				zone_id;			// Current zone of the player
/*3704*/	SpellBuff_Struct		buffs[BUFF_COUNT];			// Buffs currently on the player
/*3944*/	char 				groupMembers[6][64];		//
/*4328*/	uint8				unknown4328[4]; 		//
/*4332*/	uint32				ldon_guk_points;		// Earned Deepest Guk points - Doodman: guessing based on others that were known
/*4336*/	uint32				ldon_mirugal_points;		// Earned Mirugal' Mebagerie points - Verified by Doodman
/*4340*/	uint32				ldon_mistmoore_points;		// Earned Mismoore Catacombs Points - Verified by Doodman
/*4344*/	uint32				ldon_rujarkian_points;		// Earned Rujarkian Hills points - Verified by Doodman
/*4348*/	uint32				ldon_takish_points;		// Earned Takish points - Doodman: guessing based on others that were known
/*4352*/	uint8				unknown4352[24]; 		//
/*4376*/	uint32				ldon_available_points;		// Earned Rujarkian Hills points - Verified
/*4380*/	uint32				unknown4380[20];
/*4460*/	uint32				unknown4460[145];
/*5040*/	uint32				unknown5040; //0xFF FF FF FF
/*5044*/	uint32				unknown4384[111];
/*5488*/	uint32				unknown4644[46];
};
struct OldPlayerProfile_Struct
{
/*0000*/	uint32				checksum;			// Checksum from CRC32::SetEQChecksum
/*0004*/	char				name[64];			// Name of player sizes not right
/*0068*/	char				last_name[32];		// Last name of player sizes not right
/*0100*/	uint32				gender;				// Player Gender - 0 Male, 1 Female
/*0104*/	uint32				race;				// Player race
/*0108*/	uint32				class_;				// Player class
/*0112*/	uint32				unknown0112;		//
/*0116*/	uint32				level;				// Level of player (might be one byte)
/*0120*/	uint32				bind_zone_id;		// Zone player is bound in
/*0124*/	float				bind_x;				// Bind loc x coord
/*0128*/	float				bind_y;				// Bind loc y coord
/*0132*/	float				bind_z;				// Bind loc z coord
/*0136*/	uint8				unknown0136[4];		//
/*0140*/	uint32				deity;				// deity
/*0144*/	uint32				guildid;				
/*0148*/	uint32				birthday;			// characters bday
/*0152*/	uint32				lastlogin;			// last login or zone time
/*0156*/	uint32				timeplayed;			// in minutes
/*0160*/	uint8				pvp;			// Sta bar % depleted (ie, 30 = 70% sta)
/*0161*/	uint8				unknown0161;				// Level again.
/*0162*/	uint8				anon;		// 2=roleplay, 1=anon, 0=not anon
/*0163*/	uint8				gm;				// 1=gm, 0=not gm
/*0164*/	uint8				guildrank;					
/*0165*/	uint8				unknown0165[43];	//
/*0208*/	uint8				ability_time_seconds;
/*0209*/	uint8				ability_number; //ability used
/*0210*/	uint8				ability_time_minutes;
/*0211*/	uint8				ability_time_hours;//place holder
/*0212*/	uint8				haircolor;			// Player hair color
/*0213*/	uint8				beardcolor;			// Player beard color
/*0214*/	uint8				eyecolor1;			// Player left eye color
/*0215*/	uint8				eyecolor2;			// Player right eye color
/*0216*/	uint8				hairstyle;			// Player hair style
/*0217*/	uint8				beard;				// Beard type

/*0218*/	uint8				unknown0218;		// @bp Spacer/Flag?
/*0219*/	uint8				fatigue;			//wrong spot but needed to go somewhere
/*0220*/	uint32				item_material[9];	// Item texture/material of worn/held items
/*0256*/	uint8				unknown0256[52];
/*0308*/	Color_Struct		item_tint[9];
/*0344*/	int16				aa_array[122];		// Length may not be right but i assume no class has more than this

/*0588*/	char				servername[64];		// length probably not right
/*0652*/	uint32				guildid2;		//
/*0656*/	uint32				exp;				// Current Experience
/*0660*/	uint32				points;				// Unspent Practice points
/*0664*/	uint32				mana;				// current mana
/*0668*/	uint32				cur_hp;				// current hp
/*0672*/	uint32				unknown0672;		// 0x05
/*0676*/	uint32				STR;				// Strength
/*0680*/	uint32				STA;				// Stamina
/*0684*/	uint32				CHA;				// Charisma
/*0688*/	uint32				DEX;				// Dexterity
/*0692*/	uint32				INT;				// Intelligence
/*0696*/	uint32				AGI;				// Agility
/*0700*/	uint32				WIS;				// Wisdom
/*0704*/	uint8				face;				// Player face
/*0705*/	uint8				unknown0705[11];	// ?
/*0716*/	int32				unknown712[9];		// RR GG BB 00
/*0752*/	uint8				languages[MAX_PP_LANGUAGE];
/*0780*/	uint8				unknown0780[4];		// All 0x00
/*0784*/	int32				spell_book[MAX_PP_SPELLBOOK];
/*2384*/	uint8				unknown2384[448];	// all 0xff   
/*2832*/	int32				mem_spells[MAX_PP_MEMSPELL];
/*2864*/	uint8				unknown2864[32];	//
/*2896*/	uint32				unknown2896;		//
/*2900*/	float				y;					// Player y position
/*2904*/	float				x;					// Player x position
/*2908*/	float				z;					// Player z position
/*2912*/	float				heading;			// Direction player is facing
/*2916*/	uint8				unknown2909[4];		//
/*2920*/	uint32				platinum;			// Platinum Pieces on player
/*2922*/	uint32				gold;				// Gold Pieces on player
/*2928*/	uint32				silver;				// Silver Pieces on player
/*2932*/	uint32				copper;				// Copper Pieces on player
/*2936*/	uint32				platinum_bank;		// Platinum Pieces in Bank
/*2940*/	uint32				gold_bank;			// Gold Pieces in Bank
/*2944*/	uint32				silver_bank;		// Silver Pieces in Bank
/*2948*/	uint32				copper_bank;		// Copper Pieces in Bank
/*2952*/	uint32				platinum_cursor;	// Platinum on cursor
/*2956*/	uint32				gold_cursor;		// Gold on cursor
/*2960*/	uint32				silver_cursor;		// Silver on cursor
/*2964*/	uint32				copper_cursor;		// Copper on cursor
/*2968*/	uint32				platinum_shared;	// Platinum shared between characters
/*2972*/	uint8				unknown2972[16];   	// @bp unknown skills?
/*2992*/	uint32				skills[MAX_PP_SKILL];
/*3288*/	uint32				unknown_skills[46];	// @bp unknown skills?
/*3472*/	uint8				unknown3472[88];	//
/*3560*/	uint32				perAA;				// % on the AA exp bar
/*3564*/	uint8				unknown3564[12];	//28
/*3576*/	uint8				pvp2;	//
/*3591*/	uint8				unknown3591[15];	//
/*3592*/	uint32				zone_change_count;	// Number of times user has zoned in their career (guessing)
/*3596*/	uint8				unknown3596[20];	//
/*3616*/	int32				ability_down;
/*3620*/	uint8				unknown3620[20];	//
/*3640*/	int32				expAA;
/*3644*/	int32				unknown3644;
/*3648*/	int32				expansion;		// expansion setting
/*3652*/	sint32				unknown3648;
/*3656*/	char				unknown3656[16];	//
/*3672*/	sint32				hunger_level;
/*3676*/	sint32				thirst_level;
/*3680*/	int32				ability_up;
/*3684*/	int32				aapoints;		// Wrong but needs to be put somewhere =P
/*3688*/	char				unknown3688[12];				
/*3700*/	uint32				zone_id;			// Current zone of the player
/*3704*/	SpellBuff_Struct		buffs[15];			// Buffs currently on the player
/*3944*/	char 				groupMembers[6][64];		//
/*4328*/	uint8				unknown4328[4]; 		//
/*4332*/	uint32				ldon_guk_points;		// Earned Deepest Guk points - Doodman: guessing based on others that were known
/*4336*/	uint32				ldon_mirugal_points;		// Earned Mirugal' Mebagerie points - Verified by Doodman
/*4340*/	uint32				ldon_mistmoore_points;		// Earned Mismoore Catacombs Points - Verified by Doodman
/*4344*/	uint32				ldon_rujarkian_points;		// Earned Rujarkian Hills points - Verified by Doodman
/*4348*/	uint32				ldon_takish_points;		// Earned Takish points - Doodman: guessing based on others that were known
/*4352*/	uint8				unknown4352[24]; 		//
/*4376*/	uint32				ldon_available_points;		// Earned Rujarkian Hills points - Verified
/*4380*/	uint32				unknown4380[20]; //added in last patch
/*4460*/	uint32				unknown4460; //0xFF FF FF FF
/*4464*/	uint32				unknown4384[110];
/*4904*/	uint32				unknown4644[4];
};

/*
** Client Target Struct
** Length: 2 Bytes
** OpCode: 6221
*/
struct ClientTarget_Struct {
/*000*/	int32	new_target;			// Target ID
};

/*
** Target Rejection Struct
** Length: 12 Bytes
** OpCode: OP_TargetReject
*/
struct TargetReject_Struct {
/*00*/	uint8	unknown00[12];
};

struct PetCommand_Struct {
/*000*/ int8	command;
/*000*/ int8	unknownpcs000[7];
};

/*
** Delete Spawn
** Length: 4 Bytes
** OpCode: OP_DeleteSpawn
*/
struct DeleteSpawn_Struct
{
/*00*/ int32 spawn_id;             // Spawn ID to delete
/*04*/
};

/*
** Channel Message received or sent
** Length: 144 Bytes + Variable Length + 1
** OpCode: OP_ChannelMessage
**
*/
struct ChannelMessage_Struct
{
	union {
/*000*/	char	targetname[64];		// Tell recipient
		uint32	type;
	};
/*064*/	char	sender[64];			// The senders name (len might be wrong)
/*128*/	int32	language;			// Language
/*132*/	int32	chan_num;			// Channel
/*136*/	int32	cm_unknown4[2];		// ***Placeholder
/*144*/	int32	skill_in_language;	// The players skill in this language? might be wrong
/*148*/	char	message[0];			// Variable length message
};

/*
** Special Message
** Length: 4 Bytes + Variable Text Length + 1
** OpCode: OP_SpecialMesg
**
*/
struct SpecialMesg_Struct
{
/*00*/	char	header[3];				// 04 04 00 <-- for #emote style msg
/*03*/	uint32	msg_type;				// Color of text (see MT_*** below)
/*07*/	uint32	target_spawn_id;		// Who is it being said to?
/*11*/	uint32	unknown11[3];
/*23*/	uint8	unknown23;
/*11*/	char	message[0];				// What is being said?
};

// msg_type's for custom usercolors 
#define MT_Say					256
#define MT_Tell					257
#define MT_Group				258
#define MT_Guild				259
#define MT_OOC					260
#define MT_Auction				261
#define MT_Shout				262
#define MT_Emote				263
#define MT_Spells				264
#define MT_YouHitOther			265
#define MT_OtherHitsYou			266
#define MT_YouMissOther			267
#define MT_OtherMissesYou		268
#define MT_Broadcasts			269
#define MT_Skills				270
#define MT_Disciplines			271
#define MT_CritMelee			301

/*
** When somebody changes what they're wearing
**      or give a pet a weapon (model changes)
** Length: 16 Bytes
** Opcode: 9220
*/
struct WearChange_Struct{
/*000*/ int16 spawn_id;
/*002*/ int16 material;
/*004*/ Color_Struct color;
/*009*/ int8 wear_slot_id;
};

/*
** Type:   Bind Wound Structure
** Length: 8 Bytes
*/
//Fixed for 7-14-04 patch
struct BindWound_Struct
{
/*002*/	int16	to;			// TargetID
/*004*/	int16	unknown2;		// ***Placeholder
/*006*/	int16	type;
/*008*/	int16	unknown6;
};
#if 0 // old stuff
struct BindWound_Struct {
// Size 4
    uint16  to; // entity id
    uint8    type; // This could be an int16
    // 0 or 1 complete, 2 Unknown, 3 ACK, 4 Died, 5 Left, 6 they moved, 7 you moved
    uint8    placeholder; //
};
#endif


/*
** Type:   Zone Change Request (before hand)
** Length: 70 Bytes-2 = 68 bytes 
** OpCode: a320
*/

#define ZONE_ERROR_NOMSG 0
#define ZONE_ERROR_NOTREADY -1
#define ZONE_ERROR_VALIDPC -2
#define ZONE_ERROR_STORYZONE -3
#define ZONE_ERROR_NOEXPANSION -6
#define ZONE_ERROR_NOEXPERIENCE -7

struct ZoneChange_Struct {
/*000*/	char	char_name[64];     // Character Name
/*064*/	uint32	zoneID;
/*068*/ int8	unknown0072[4];
/*072*/	sint32	success;		// =0 client->server, =1 server->client, -X=specific error
};

struct Animation_Struct {
	int16 spawn_id;
	int8 animation_speed;
	int8 animation;
};

#if 0
struct Action_Struct
{
	// len = 24
	int16 target;
	int16 source;
	int8 type;
	int16 spell;
	sint32 damage;
	int8 unknown1[12];
};
#endif
// solar: this is what causes the caster to animate and the target to
// get the particle effects around them when a spell is cast
// also causes a buff icon
struct Action_Struct
{
 /* 00 */	int16 target;	// id of target
 /* 02 */	int16 source;	// id of caster
 /* 04 */	uint16 level; // level of caster
 /* 06 */	uint16 unknown06;	// seems to be fixed to 0x0A
 /* 08 */	uint32 unknown08;
 /* 12 */	uint16 unknown16;
// some kind of sequence that's the same in both actions
// as well as the combat damage, to tie em together?
 /* 14 */	int32 sequence;	
 /* 18 */	uint32 unknown18;
 /* 22 */	int8 type;		// 231 (0xE7) for spells
 /* 23 */	uint32 unknown23;
 /* 27 */	int16 spell;	// spell id being cast
 /* 29 */	int8 unknown29;
// this field seems to be some sort of success flag, if it's 4
 /* 30 */	int8 buff_unknown;	// if this is 4, a buff icon is made
 /* 31 */
};

// solar: this is what prints the You have been struck. and the regular
// melee messages like You try to pierce, etc.  It's basically the melee
// and spell damage message
struct CombatDamage_Struct
{
/* 00 */	int16	target;
/* 02 */	int16	source;
/* 04 */	int8	type; //slashing, etc.  231 (0xE7) for spells
/* 05 */	int16	spellid;
/* 07 */	int32	damage;
/* 11 */	int32 unknown11;
/* 15 */	int32 sequence;	// see above notes in Action_Struct
/* 19 */	int32	unknown19;
/* 23 */
};
/*
** Consider Struct
** Length: 24 Bytes
** OpCode: 3721
*/
struct Consider_Struct{
/*000*/ uint32	playerid;               // PlayerID
/*004*/ uint32	targetid;               // TargetID
/*008*/ int32	faction;                // Faction
/*0012*/ int32	level;                  // Level
/*016*/ sint32	cur_hp;                  // Current Hitpoints
/*020*/ sint32	max_hp;                  // Maximum Hitpoints
/*024*/ int8 pvpcon;     // Pvp con flag 0/1
/*025*/ int8	unknown3[3];
};

/*
** Spawn Death Blow
** Length: 32 Bytes
** OpCode: 0114
*/
struct Death_Struct
{
/*000*/	int32	spawn_id;
/*004*/	int32	killer_id;
/*008*/	int32	unknown08;	// was corpseid
/*012*/	int32	unknown12;	// was type
/*016*/	int32	spell_id;
/*020*/ int32	attack_skill;
/*024*/	int32	damage;
/*028*/	int32	unknown028;
};

/*
** Generic Spawn Position Update
** Length: 18 Bytes
**
*/
struct SpawnPositionUpdate_Struct
{
/*0000*/ uint16		spawn_id;
/*0002*/ uint64		y:19, z:19, x:19, u3:7;
/*0010*/ unsigned short	heading:12,unused2:4;
/*0012*/
};

/*
** Spawn position update
** Length: 18 Bytes
**	Struct sent from server->client to update position of
**	another spawn's position update in zone (whether NPC or PC)
**
*/
struct PlayerPositionUpdateServer_Struct
{
/*00*/	uint16	spawn_id;
/*02*/	uint32	heading:12;
/*02*/	sint32	delta_heading:10;
/*02*/	uint32	animation:10;
/*06*/	sint32	delta_y:13;
/*06*/	sint32	y_pos:19;
/*10*/	sint32	x_pos:19;
/*10*/	sint32	delta_z:13;
/*14*/	sint32	delta_x:13;
/*14*/	sint32	z_pos:19;
/*18*/
};

/*
** Player position update
** Length: 30 bytes
**	Struct sent from client->server to update
**	player position on server
**
*/
struct PlayerPositionUpdateClient_Struct
{
/*00*/	uint16	spawn_id;
/*02*/	float	z_pos;
/*06*/	float	y_pos;
/*10*/	float	delta_y;
/*14*/	float	x_pos;
/*18*/	float	delta_x;
/*22*/	float	delta_z;
/*26*/	uint32	heading:12;
/*26*/	sint32	delta_heading:10;
/*26*/	uint32	animation:10;
};

/*
** Spawn Position Update
** Length: 6 Bytes + Number of Updates * 15 Bytes
** OpCode: a120
*/

struct SpawnPositionUpdates_Struct
{
/*0000*/ int32  num_updates;               // Number of SpawnUpdates
/*0004*/ struct SpawnPositionUpdate_Struct // Spawn Position Update
                     spawn_update[0];
};

/*
** Spawn HP Update
** Length: 6 Bytes
** OpCode: OP_HPUpdate
*/
struct SpawnHPUpdate_Struct
{
/*00*/ uint16	cur_hp;               // Id of spawn to update
/*02*/ sint16	spawn_id;                 // Current hp of spawn
/*04*/ sint16	max_hp;                 // Maximum hp of spawn
/*06*/
};
struct SpawnHPUpdate_Struct2
{
/*01*/ sint16	spawn_id;
/*00*/ int8	hp;
};
/*
** Stamina
** Length: 8 Bytes
** OpCode: 5721
*/
struct Stamina_Struct {
/*00*/ int32 food;                     // (low more hungry 127-0)
/*02*/ int32 water;                    // (low more thirsty 127-0)
};

/*
** Level Update
** Length: 14 Bytes
** OpCode: 9821
*/
struct LevelUpdate_Struct
{
/*00*/ uint32 level;                  // New level
/*04*/ uint32 level_old;              // Old level
/*08*/ uint32 exp;                    // Current Experience
};

/*
** Experience Update
** Length: 14 Bytes
** OpCode: 9921
*/
struct ExpUpdate_Struct
{
/*0000*/ uint32 exp;                    // Current experience value
/*0004*/ uint32 aaxp; // @BP ??
};

/*
** Child struct of Item_Struct:
**	Common item data
**
*/
struct ItemCommon_Struct {
/*index*/
/*018*/	sint32	Unknown018;
/*019*/	uint32	Unknown019;
/*020*/	uint32	Unknown020;
/*021*/	bool	Tradeskills;		// Is this a tradeskill item?
/*022*/	sint8	SvCold;				// Save vs Cold
/*023*/	sint8	SvDisease;			// Save vs Disease
/*024*/	sint8	SvPoison;			// Save vs Poison
/*025*/	sint8	SvMagic;			// Save vs Magic
/*026*/	sint8	SvFire;				// Save vs Fire
/*027*/	sint8	STR;				// Strength
/*028*/	sint8	STA;				// Stamina
/*029*/	sint8	AGI;				// Agility
/*030*/	sint8	DEX;				// Dexterity
/*031*/	sint8	CHA;				// Charisma
/*032*/	sint8	INT;				// Intelligence
/*033*/	sint8	WIS;				// Wisdom
/*034*/	sint32	HP;					// HP
/*035*/	sint32	Mana;				// Mana
/*036*/	sint32	AC;					// AC
/*037*/	uint32	Deity;				// Bitmask of Deities that can equip this item
/*038*/	sint32	SkillModValue;		// % Mod to skill specified in SkillModType
/*039*/	uint32	SkillModType;		// Type of skill for SkillModValue to apply to
/*040*/	uint32	BaneDmgRace;		// Bane Damage Race
/*041*/	sint8	BaneDmg;			// Bane Damage
/*042*/	uint32	BaneDmgBody;		// Bane Damage Body
/*043*/	bool	Magic;				// True=Magic Item, False=not
/*044*/	sint32	casttime2;
/*045*/	uint8	ProcLevel;			// Proc lvl
/*046*/	uint8	RequiredLevel;		// Required Level to use item
/*047*/	uint32	BardSkillType;		// Bard Skill Type
/*048*/	sint32	BardSkillAmt;		// Bard Skill Amount
/*049*/	sint8	Light;				// Light
/*050*/	uint8	Delay;				// Delay * 10
/*051*/	uint8	RecommendedLevel;	// Recommended level to use item
/*052*/	uint8	RecommendedSkill;	// Recommended skill to use item (refers to primary skill of item)
/*053*/	uint8	ElemDmgType;		// Elemental Damage Type (1=magic, 2=fire)
/*054*/	uint8	ElemDmg;			// Elemental Damage
/*055*/	uint8	EffectType;			// Effect Type: 0=combat proc, 1=clicky, 2=worn, 3=expendable charges, 4=must equip clicky, 5=clicky (again?)
/*056*/	uint8	Range;				// Range of item
/*057*/	uint8	Damage;				// Delay between item usage (in 0.1 sec increments)
/*058*/	uint32	Color;				// RR GG BB 00 <-- as it appears in pc
/*059*/	uint32	Classes;			// Bitfield of classes that can equip item (1 << class#)
/*060*/	uint32	Races;				// Bitfield of races that can equip item (1 << race#)
/*061*/	uint32	Unknown061;
/*062*/	sint32	SpellId;			// Spell Id of effect, if item has one
/*063*/	sint16	MaxCharges;			// Maximum charges items can hold: -1 if not a chargeable item
/*064*/	uint8	Skill;				// Skill (1hs, 2hs, 1hb, etc)
/*065*/	uint8	Material;			// Item material type
/*066*/	float	SellRate;			// Sell rate
/*067*/	uint32	Unknown067;
	union {
/*068*/	uint32	Fulfilment;			// Food fulfilment (How long it lasts)
/*068*/	sint16	CastTime;			// Cast Time for clicky effects, in milliseconds
	};
/*069*/	uint32	Unknown069;
/*070*/	uint32	ProcRateMod;
/*071*/	sint32	FocusId;			// Focus Effect Id
/*072*/	sint8	CombatEffects;		// PoP: Combat Effects +
/*073*/	sint8	Shielding;			// PoP: Shielding %
/*074*/	sint8	StunResist;			// PoP: Stun Resist %
/*075*/	sint8	StrikeThrough;		// PoP: Strike Through %
/*076*/	uint32	CombatSkill;
/*077*/	uint32	CombatSkillDmg;
/*078*/	sint8	SpellShield;		// PoP: Spell Shield %
/*079*/	sint8	Avoidance;			// PoP: Avoidance +
/*080*/	sint8	Accuracy;			// PoP: Accuracy +
/*081*/	uint32	Unknown081;
/*081*/	sint32	FactionMod1;		// Faction Mod 1
/*082*/	sint32	FactionMod2;		// Faction Mod 2
/*083*/	sint32	FactionMod3;		// Faction Mod 3
/*084*/	sint32	FactionMod4;		// Faction Mod 4
/*085*/	sint32	FactionAmt1;		// Faction Amt 1
/*086*/	sint32	FactionAmt2;		// Faction Amt 2
/*087*/	sint32	FactionAmt3;		// Faction Amt 3
/*088*/	sint32	FactionAmt4;		// Faction Amt 4
/*090*/	char	CharmFile[32];		// ?
/*091*/	uint32	augtype;
/*092*/	uint8	AugSlot1Type;		// LDoN: Augment Slot 1 Type
/*093*/	uint8	AugSlot2Type;		// LDoN: Augment Slot 2 Type
/*094*/	uint8	AugSlot3Type;		// LDoN: Augment Slot 3 Type
/*095*/	uint8	AugSlot4Type;		// LDoN: Augment Slot 4 Type
/*096*/	uint8	AugSlot5Type;		// LDoN: Augment Slot 5 Type
/*097*/	uint32	ldonpointtheme;
/*098*/	uint32	ldonpointcost;
/*099*/	uint32	ldonsold;
};

/*
** Child struct of Item_Struct:
**	Book item data
**
*/
struct ItemBook_Struct {
/*index*/
/*104*/	uint8	BookType;			// 0=rolled up note, 1=book
/*105*/	uint32	Unknown105;
/*106*/	char	File[15];			// ?
};

/*
** Child struct of Item_Struct:
**	Container item data
**
*/
struct ItemContainer_Struct { 
/*index*/
/*100*/	uint8	PackType;			// 0:Small Bag, 1:Large Bag, 2:Quiver, 3:Belt Pouch ... there are 50 types
/*101*/	uint8	Slots;				// Number of slots: can only be 2, 4, 6, 8, or 10
/*102*/	uint8	SizeCapacity;		// 0:TINY, 1:SMALL, 2:MEDIUM, 3:LARGE, 4:GIANT 
/*103*/	uint8	WeightReduction;	// 0->100

// @merth: From old struct, we still haven't located these values:
// Open		// ?
// Combine	// Whether combine button exists or not
};

/*
** Item data
** Items are no longer sent as a struct; they are sent as a |-delimited string
** The following is the order in which they appear - some lines have been
** commented out to show the element should go there, but doesn't belong here
**
*/
struct Item_Struct {
	void SetCache() {
		// Caching calculated values
		attribs = ItemAttribNone;
		
		if (strstr(LoreName, "&") != 0 || summonedflag)
			attribs = (ItemAttrib)(attribs | ItemAttribSummoned);
		if (strstr(LoreName, "*") != 0 || loreflag)
			attribs = (ItemAttrib)(attribs | ItemAttribLore);
		if (strstr(LoreName, "#") != 0 || artifactflag)
			attribs = (ItemAttrib)(attribs | ItemAttribArtifact);
		if (strstr(LoreName, "~") != 0 || pendingloreflag)
			attribs = (ItemAttrib)(attribs | ItemAttribPendingLore);
		if ((ItemClass == ItemTypeCommon) && Common.Magic)
			attribs = (ItemAttrib)(attribs | ItemAttribMagic);
	};
	
	// Non packet based fields
	uint8		minstatus;
	ItemAttrib	attribs;
	


	// Packet based fields
/*index*/
/*001*/	sint16	Charges;			// Instance data (just here as FYI)
/*002*/	uint32	Unknown002;
/*003*/	sint16	CurrentEquipSlot;	// Instance data (just here as FYI)
/*004*/	uint32	MerchantPrice;	// Instance data (just here as FYI)
/*005*/	uint32	Unknown005;
/*006*/	uint32	Unknown006;
/*007*/	uint32	Unknown007;
/*008*/	uint32	Unknown008;		 //added in patch 
/*008*/	uint32	Unknown009;		 //added in patch 
/*008*/	uint8	ItemClass;				// Item Type: 0=common, 1=container, 2=book (quote precedes field - dunno why)
/*009*/	char	Name[64];			// Name
/*010*/	char	LoreName[80];		// Lore Name: *=lore, &=summoned, #=artifact, ~=pending lore
/*011*/	char	IDFile[30];			// Visible model
/*012*/	uint32	ItemNumber;			// Unique ID (also PK for DB)
/*013*/	uint8	Weight;				// Item weight * 10
/*014*/	uint8	NoRent;				// No Rent: 0=norent, 255=not norent
/*015*/	uint8	NoDrop;				// No Drop: 0=nodrop, 255=not nodrop
/*016*/	uint8	Size;				// Size: 0=tiny, 1=small, 2=medium, 3=large, 4=giant
/*017*/	uint32	EquipSlots;			// Bitfield for which slots this item can be used in
/*018*/	uint32	Cost;				// Item cost (?)
/*019*/	uint32	IconNumber;			// Icon Number

			union   {
/*020->100*/	ItemCommon_Struct		Common;
/*101->104*/	ItemContainer_Struct	Container;
/*105->107*/	ItemBook_Struct			Book;
            };

/*107*/	uint32	banedmgamt2;
/*108*/	uint32	augmentrestriction;
/*109*/	bool	loreflag;
/*110*/	bool	pendingloreflag;
/*111*/	bool	artifactflag;
/*112*/	bool	summonedflag;
/*113*/	uint32	tribute;
/*114*/ bool	gm;
/*115*/	uint32	endur;
/*116*/	uint32	dotshielding;
/*117*/	uint32	attackbonus;
/*118*/	uint32	hpregen;
/*119*/	uint32	manaregen;
/*120*/	uint32	hastepercent;
/*121*/	uint32	damageshield;
/*122*/	uint32	unknown122;		//prolly recastdelay, per lucy
/*123*/	uint32	unknown123;		//prolly recasttype
/*124*/	uint32	unknown124;
/*125*/	uint32	unknown125;
/*126*/	uint32	unknown126;
/*127*/	uint32	unknown127;
/*128*/	uint32	unknown128;
/*129*/	uint32	unknown129;
/*130*/	uint32	unknown130;
/*131*/	uint32	unknown131;
/*132*/	uint32	unknown132;
/*133*/	uint32	unknown133;
};

/*
** Item Packet Struct - Works on a variety of opcodes
** Packet Types: See ItemPacketType enum
**
*/
struct ItemPacket_Struct
{
/*00*/	ItemPacketType	PacketType;
/*04*/	char			SerializedItem[0];
/*xx*/
};

struct BulkItemPacket_Struct
{
/*00*/	char			SerializedItem[0];
/*xx*/
};

struct Consume_Struct
{
/*0000*/ int32 slot;
/*0004*/ int32 auto_consumed; // 0xffffffff when auto eating e7030000 when right click
/*0008*/ int8  c_unknown1[4];
/*0012*/ int8  type; // 0x01=Food 0x02=Water
/*0013*/ int8  unknown13[3];
};


struct MoveItem_Struct
{
/*0000*/ uint32 from_slot;
/*0004*/ uint32 to_slot;
/*0008*/ uint32 number_in_stack;
};

//
// from_slot/to_slot
// -1 - destroy
//  0 - cursor
//  1 - inventory
//  2 - bank
//  3 - trade
//  4 - shared bank
//
// cointype
// 0 - copeer
// 1 - silver
// 2 - gold
// 3 - platinum
//
#define COINTYPE_PP 3
#define COINTYPE_GP 2
#define COINTYPE_SP 1
#define COINTYPE_CP 0

struct MoveCoin_Struct
{
		 sint32 from_slot;
		 sint32 to_slot;
		 sint32 cointype1;
		 sint32 cointype2;
		 sint32	amount;
};
struct TradeCoin_Struct{
	int32	trader;
	int8	slot;
	int16	unknown5;
	int8	unknown7;
	int32	amount;
};
struct TradeMoneyUpdate_Struct{
	int32	trader;
	int32	type;
	int32	amount;
};
/*
** Surname struct
** Size: 100 bytes
*/
struct Surname_Struct 
{ 
/*0000*/	char name[64];
/*0064*/	uint32 unknown0064;
/*0068*/	char lastname[32];
/*0100*/
}; 

struct GuildsListEntry_Struct {
	char name[64];
};

struct GuildsList_Struct {
	int8 head[64]; // First on guild list seems to be empty...
	GuildsListEntry_Struct Guilds[512];
};

//#define CODE_NEW_GUILD                  0x7b21
struct GuildUpdate_Struct {
	int32	guildID;
	GuildsListEntry_Struct entry;
};

/*
** Money Loot
** Length: 22 Bytes
** OpCode: 5020
*/
struct moneyOnCorpseStruct {
/*0000*/ uint8	response;		// 0 = someone else is, 1 = OK, 2 = not at this time
/*0001*/ uint8	unknown1;		// = 0x5a
/*0002*/ uint8	unknown2;		// = 0x40
/*0003*/ uint8	unknown3;		// = 0
/*0004*/ uint32	platinum;		// Platinum Pieces
/*0008*/ uint32	gold;			// Gold Pieces

/*0012*/ uint32	silver;			// Silver Pieces
/*0016*/ uint32	copper;			// Copper Pieces
};

//opcode = 0x5220
// size 292


struct LootingItem_Struct {
/*000*/	int32	lootee;
/*002*/	int32	looter;
/*004*/	int16	slot_id;
/*006*/	int8	unknown3[2];
/*008*/	int32	auto_loot;
};

struct GuildManageStatus_Struct{
	int32	guildid;
	int32	oldrank;
	int32	newrank;
	char	name[64];
};
// Guild invite, remove
struct GuildJoin_Struct{
	int32	guildid;
	int32	unknown04;
	int32	level;
	int32	class_;
	int32	rank;//0 member, 1 officer, 2 leader
	int32	zoneid;
	int32	unknown24;
	char	name[64];
};
struct GuildInviteAccept_Struct {
	char inviter[64];
	char newmember[64];
	int32 response;
	int32 guildeqid;
};
struct GuildManageRemove_Struct {
	int32 guildeqid;
	char member[64];
};
struct GuildCommand_Struct {
	char othername[64];
	char myname[64];
	int16 guildeqid;
	int8 unknown[2]; // for guildinvite all 0's, for remove 0=0x56, 2=0x02
	int32 officer;
};

// Opcode OP_GMZoneRequest
// Size = 88 bytes
struct GMZoneRequest_Struct {
/*0000*/	char	charname[64];
/*0064*/	int32	zone_id;
/*0068*/	float	x;
/*0072*/	float	y;
/*0076*/	float	z;
/*0080*/	char	unknown0080[4];
/*0084*/	int32	success;		// 0 if command failed, 1 if succeeded?
/*0088*/
//	/*072*/	sint8	success;		// =0 client->server, =1 server->client, -X=specific error
//	/*073*/	int8	unknown0073[3]; // =0 ok, =ffffff error
};

struct GMSummon_Struct {
/*  0*/	char    charname[64];
/* 30*/	char    gmname[64];
/* 60*/ int32	success;
/* 61*/	int32	zoneID;
/*92*/	sint32  y;
/*96*/	sint32  x;
/*100*/ sint32  z;
/*104*/	int32 unknown2; // E0 E0 56 00
};

struct GMGoto_Struct { // x,y is swapped as compared to summon and makes sense as own packet
/*  0*/	char    charname[64];

/* 64*/	char    gmname[64];
/* 128*/ int32	success;
/* 132*/	int32	zoneID;

/*136*/	sint32  y;
/*140*/	sint32  x;
/*144*/ sint32  z;
/*148*/	int32 unknown2; // E0 E0 56 00
};

struct GMLastName_Struct {
	char name[64];
	char gmname[64];
	char lastname[64];
	int16 unknown[4];	// 0x00, 0x00
					    // 0x01, 0x00 = Update the clients
};

//Combat Abilities
struct CombatAbility_Struct {
	int32 m_id;
	int32 m_atk;
	int32 m_type;
};

//Instill Doubt
struct Instill_Doubt_Struct {
	int8 i_id;
	int8 ia_unknown;
	int8 ib_unknown;
	int8 ic_unknown;
	int8 i_atk;

	int8 id_unknown;
	int8 ie_unknown;
	int8 if_unknown;
	int8 i_type;
	int8 ig_unknown;
	int8 ih_unknown;
	int8 ii_unknown;
};

struct GiveItem_Struct {
	uint16 to_entity;
	sint16 to_equipSlot;
	uint16 from_entity;
	sint16 from_equipSlot;
};

struct RandomReq_Struct {
	int32 low;
	int32 high;
};

/* solar: 9/23/03 reply to /random command; struct from Zaphod */
struct RandomReply_Struct {
/* 00 */	int32 low;
/* 04 */	int32 high;
/* 08 */	int32 result;
/* 12 */	char name[64];
/* 76 */
};

struct LFG_Struct {
/*
Wrong size on OP_LFG. Got: 80, Expected: 68
   0: 00 00 00 00 01 00 00 00 - 00 00 00 00 64 00 00 00  | ............d...
  16: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
  32: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
  48: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
  64: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
Wrong size on OP_LFG. Got: 80, Expected: 68
   0: 00 00 00 00 01 00 00 00 - 3F 00 00 00 41 00 00 00  | ........?...A...
  16: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
  32: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
  48: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
  64: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
Wrong size on OP_LFG. Got: 80, Expected: 68
   0: 00 00 00 00 01 00 00 00 - 3F 00 00 00 41 00 00 00  | ........?...A...
  16: 46 72 75 62 20 66 72 75 - 62 20 66 72 75 62 00 00  | Frub frub frub..
  32: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
  48: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
  64: 00 00 00 00 00 00 00 00 - 00 00 00 00 00 00 00 00  | ................
*/
/*000*/	uint32 unknown000;
/*004*/	uint32 value; // 0x00 = off 0x01 = on
/*008*/	uint32 unknown008;
/*012*/	uint32 unknown012;
/*016*/	char	name[64];
};

/*
** LFG_Appearance_Struct
** Packet sent to clients to notify when someone in zone toggles LFG flag
** Size: 8 bytes
** Used in: OP_LFGAppearance
**
*/
struct LFG_Appearance_Struct
{
/*0000*/ uint32 spawn_id;		// ID of the client
/*0004*/ uint8 lfg;				// 1=LFG, 0=Not LFG
/*0005*/ char unknown0005[3];	//
/*0008*/
};


// EverQuest Time Information:
// 72 minutes per EQ Day
// 3 minutes per EQ Hour
// 6 seconds per EQ Tick (2 minutes EQ Time)
// 3 seconds per EQ Minute

struct TimeOfDay_Struct {
	int8	hour;
	int8	minute;
	int8	day;
	int8	month;
	int32	year;
};

// Darvik: shopkeeper structs
struct Merchant_Click_Struct {
/*000*/ int32	npcid;			// Merchant NPC's entity id
/*004*/ int32	playerid;
/*008*/ int8	unknown[8]; /*
0 is e7 from 01 to // MAYBE SLOT IN PURCHASE
1 is 03
2 is 00
3 is 00
4 is ??
5 is ??
6 is 00 from a0 to
7 is 00 from 3f to */
/*
0 is F6 to 01
1 is CE CE
4A 4A
00 00
00 E0
00 CB
00 90
00 3F
*/

};			

struct Merchant_Sell_Struct {
/*000*/	int32	npcid;			// Merchant NPC's entity id
/*004*/	int32	playerid;		// Player's entity id
/*008*/	int32	itemslot;
		int32	unknown12;
/*016*/	int8	quantity;		// Already sold
/*017*/ int8	Unknown016[3];
/*020*/ int32	price;
};
struct Merchant_Purchase_Struct {
/*000*/	int32	npcid;			// Merchant NPC's entity id
/*004*/	int32	itemslot;		// Player's entity id
/*008*/	int32	quantity;
/*012*/	int32	price;
};

struct Adventure_Purchase_Struct {
/*000*/	int32	npcid;
/*004*/	int32	itemid;
/*008*/	int32	variable;
};

struct AdventurePoints_Update_Struct {
/*000*/	uint32				ldon_available_points;		// Total available points
/*004*/ uint8				unkown_apu004[20];
/*024*/	uint32				ldon_guk_points;		// Earned Deepest Guk points
/*028*/	uint32				ldon_mirugal_points;		// Earned Mirugal' Mebagerie points
/*032*/	uint32				ldon_mistmoore_points;		// Earned Mismoore Catacombs Points
/*036*/	uint32				ldon_rujarkian_points;		// Earned Rujarkian Hills points
/*040*/	uint32				ldon_takish_points;		// Earned Takish points
/*044*/	uint8				unknown_apu042[216];
};

struct Item_Shop_Struct {
	uint16 merchantid;
	int8 itemtype;
	Item_Struct item;
	int8 iss_unknown001[6];
};

/*
** Illusion_Struct
** Changes client visible features
** Size: 168 bytes
** Used In: OP_Illusion, #face, Mob::SendIllusionPacket()
** Fields from the deprecated struct:
**	int8	unknown_26; //Always 26
**	int8	haircolor;
**	int8	beardcolor;
**	int8	eyecolor1; // the eyecolors always seem to be the same, maybe left and right eye?
**	int8	eyecolor2;
**	int8	hairstyle;
**	int8	aa_title;
**	int8	luclinface; // and beard
** Updated by Father Nitwit for 7-14-04 patch
**
*/
struct Illusion_Struct {
/*000*/	uint32	spawnid;
		char charname[64];		//fix for 7-14-04 patch
/**/	uint16	race;
/**/	char	unknown006[2];
/**/	uint8	gender;
/**/	uint8	texture;	
/**/	uint8	helmtexture;
/**/	uint8	unknown011;
/**/	uint32	face;
/**/	char	unknown020[88];
/**/
};

struct MerchantItemD_Struct {
/*0000*/	int8 unknown0000[4];
/*0004*/	Item_Struct item;
/*0500*/
};

struct ItemOnCorpse_Struct {
/*00*/	int16 count;
/*xx*/	struct MerchantItemD_Struct item[0];
};

struct MerchantItem_Struct {
/*00*/	MerchantItemD_Struct packets[0];
};

struct ZonePoint_Entry {
/*0000*/	int32	iterator;
/*0004*/	float	x;
/*0008*/	float	y;
/*0012*/	float	z;
/*0016*/	float	heading;
/*0020*/	int16	zoneid;
/*0022*/	int16	zoneinstance; // LDoN instance
};

struct ZonePoints {
/*0000*/	int32	count;
/*0004*/	struct	ZonePoint_Entry zpe[0]; // Always add one extra to the end after all zonepoints
};

struct SkillUpdate_Struct {
/*00*/	uint32 skillId;
/*04*/	uint32 value;
/*08*/
};

struct ZoneUnavail_Struct {
	//This actually varies, but...
	char zonename[16];
	short int unknown[4];
};

struct GroupGeneric_Struct {
	char name1[64];
	char name2[64];
};

struct GroupUpdate_Struct {	
/*0000*/	int32	action;
/*0004*/	char	yourname[64];
/*0068*/	char	membername[5][64];
/*0388*/	char	leadersname[64];
};

struct GroupUpdate2_Struct {	
/*0000*/	int32	action;
/*0004*/	char	yourname[64];
/*0068*/	char	membername[5][64];
/*0388*/	char	leadersname[64];
/*0452*/	int8	unknown[316];
};
struct GroupJoin_Struct {	
/*0000*/	int32	action;
/*0004*/	char	yourname[64];
/*0068*/	char	membername[64];
/*0132*/	int8	unknown[84];
};
struct FaceChange_Struct {
/*000*/	int8	haircolor;
/*001*/	int8	beardcolor;
/*002*/	int8	eyecolor1; // the eyecolors always seem to be the same, maybe left and right eye?
/*003*/	int8	eyecolor2;
/*004*/	int8	hairstyle;
/*005*/	int8	beard;	// vesuvias
/*006*/	int8	face; 
//vesuvias:
//there are only 10 faces for barbs changing woad just
//increase the face value by ten so if there were 8 woad 
//designs then there would be 80 barb faces
};

/*
** Trade request from one client to another
** Used to initiate a trade
** Size: 8 bytes
** Used in: OP_TradeRequest
*/
struct TradeRequest_Struct {
/*00*/	uint32 to_mob_id;
/*04*/	uint32 from_mob_id;
/*08*/
};

/*
** Item to trade struct
** User is placing an item into the trade window
** Size: 512 bytes
** Used In: OP_ItemToTrade
**
*/
struct ItemToTrade_Struct {
/*0000*/	uint32 player_id;
/*0004*/	int16 to_slot;
/*0006*/	char unknown0006[3];
/*0009*/	Item_Struct item;
/*0505*/	char unknown0505[7];
/*0512*/
};

/*
** Cancel Trade struct
** Sent when a player cancels a trade
** Size: 8 bytes
** Used In: OP_CancelTrade
**
*/
struct CancelTrade_Struct {
/*00*/	int32 fromid;
/*04*/	int32 action;
/*08*/
};

struct PetitionUpdate_Struct {
	int32 petnumber;    // Petition Number
	int32 color;		// 0x00 = green, 0x01 = yellow, 0x02 = red
	int32 status;
	time_t senttime;    // 4 has to be 0x1F
	char accountid[32];
	char gmsenttoo[64];
	long quetotal;
	char charname[64];
};

/*struct Petition_Struct {
	int32 petnumber;
	int32 urgency;
	char accountid[32];
	char lastgm[64];
	int32	zone;
	//char zone[32];
	char charname[32];
	int32 charlevel;
	int32 charclass;
	int32 charrace;
	time_t senttime; // Time?
	int32 checkouts;
	int32 unavail;
	int8 unknown5[4];
	char petitiontext[1024];
};*/

struct Petition_Struct {
	int32 petnumber;
	int32 urgency;
	char accountid[32];
	char lastgm[32];
	int32	zone;
	//char zone[32];
	char charname[64];
	int32 charlevel;
	int32 charclass;
	int32 charrace;
	int32 unknown;
	//time_t senttime; // Time?
	int32 checkouts;
	int32 unavail;
	//int8 unknown5[4];
	time_t senttime;
	int32 unknown2;
	char petitiontext[1024];
	char gmtext[1024];
};


struct Who_All_Struct { // 76 length total
/*000*/	char	whom[64];
/*064*/	int32	wrace;		// FF FF = no race

/*066*/	int32	wclass;		// FF FF = no class
/*068*/	int32	lvllow;		// FF FF = no numbers
/*070*/	int32	lvlhigh;	// FF FF = no numbers
/*072*/	int32	gmlookup;	// FF FF = not doing /who all gm
/*074*/	int32	unknown074;
/*076*/	int8	unknown076[64];
/*140*/
};

struct Stun_Struct { // 4 bytes total 
	int32 duration; // Duration of stun
};

/*
** New Combine Struct
** Client requesting to perform a tradeskill combine
** Size: 4 bytes
** Used In: OP_TradeSkillCombine
** Last Updated: Oct-15-2003
**
*/
struct NewCombine_Struct { 
/*00*/	sint16	container_slot;
/*02*/	char	unknown02[2];
/*04*/
};

// OP_Emote
struct Emote_Struct {
/*0000*/	int32 unknown01;
/*0004*/	char message[1024];
/*1028*/
};

// OP_EmoteAnim
struct EmoteAnim_Struct {
/*00*/	int16 spawnid;
/*02*/	int8 action;
/*03*/	int8 value;
/*04*/
};

// Inspect 
struct Inspect_Struct { 
	int16 TargetID; 
	int16 PlayerID; 
}; 
//OP_InspectAnswer
struct InspectResponse_Struct{//Cofruben:need to send two of this for the inspect response.
/*000*/	int32 TargetID;
/*004*/	int32 playerid;
/*008*/	char itemnames[21][64];
/*1352*/char unknown_zero[64];//fill with zero's.
/*1416*/int32 itemicons[21];
/*1500*/int32 unknown_zero2;
/*1504*/char text[288];
};

//OP_SetDataRate
struct SetDataRate_Struct {
	float newdatarate;
};

//OP_SetServerFilter
struct SetServerFilter_Struct {
	/*0000*/ int32	damageshield;
	/*0004*/ int32	npcspells;
	/*0008*/ int32	pcspells;
	/*0012*/ int32	bardsongs;
	/*0016*/ int32	unknown;
	/*0020*/ int32	guildsay;
	/*0024*/ int32	socials;
	/*0028*/ int32	group;
	/*0032*/ int32	shout;
	/*0036*/ int32	auction;
	/*0040*/ int32	ooc;
	/*0044*/ int32	mymisses;
	/*0048*/ int32	othermisses;
	/*0052*/ int32	otherhits;
	/*0056*/ int32	atkmissesme;
	/*0060*/ int32	critspells;
	/*0064*/ int32	critmelee;
	/*0068*/ int32	spelldamage;
	/*0072*/ int32	dotdamage;
	/*0076*/ int32	mypethits;
	/*0080*/ int32	mypetmisses;
};
#define FILTER_DAMAGESHIELD 0
#define FILTER_NPCSPELLS	1
#define FILTER_PCSPELLS		2
#define FILTER_BARDSONGS	3
#define FILTER_GUILDSAY		4
#define FILTER_SOCIALS		5
#define FILTER_GROUP		6
#define FILTER_SHOUT		7
#define FILTER_AUCTION		8
#define FILTER_OOC			9
#define FILTER_MYMISSES		10
#define FILTER_OTHERMISSES	11
#define FILTER_OTHERHITS	12
#define FILTER_ATKMISSESME	13
#define FILTER_CRITSPELLS	14
#define FILTER_CRITMELEE	15
#define FILTER_SPELLDAMAGE	16
#define FILTER_DOTDAMAGE	17
#define FILTER_MYPETHITS	18
#define FILTER_MYPETMISSES	19
//Op_SetServerFilterAck
struct SetServerFilterAck_Struct {
	int8 blank[8];
};
struct IncreaseStat_Struct{
	/*0000*/	int8	unknown0;
	/*0001*/	int8	str;
	/*0002*/	int8	sta;
	/*0003*/	int8	agi;
	/*0004*/	int8	dex;
	/*0005*/	int8	int_;
	/*0006*/	int8	wis;
	/*0007*/	int8	cha;
	/*0008*/	int8	fire;
	/*0009*/	int8	cold;
	/*0010*/	int8	magic;
	/*0011*/	int8	poison;
	/*0012*/	int8	disease;
	/*0013*/	char	unknown13[116];
	/*0129*/	int8	str2;
	/*0130*/	int8	sta2;
	/*0131*/	int8	agi2;
	/*0132*/	int8	dex2;
	/*0133*/	int8	int_2;
	/*0134*/	int8	wis2;
	/*0135*/	int8	cha2;
	/*0136*/	int8	fire2;
	/*0137*/	int8	cold2;
	/*0138*/	int8	magic2;
	/*0139*/	int8	poison2;
	/*0140*/	int8	disease2;
};
#define	STAT_STR		0
#define	STAT_STA		1
#define	STAT_AGI		2
#define	STAT_DEX		3
#define	STAT_INT		4
#define	STAT_WIS		5
#define	STAT_CHA		6
#define	STAT_FIRE		7
#define	STAT_COLD		8
#define	STAT_MAGIC		9
#define	STAT_POISON		10
#define	STAT_DISEASE	11

struct GMName_Struct {
	char oldname[64];
	char gmname[64];
	char newname[64];
	int8 badname;
	int8 unknown[3];
};

struct GMDelCorpse_Struct {
	char corpsename[64];
	char gmname[64];
	int8 unknown;
};

struct GMKick_Struct {
	char name[64];
	char gmname[64];
	int8 unknown;
};


struct GMKill_Struct {
	char name[64];
	char gmname[64];
	int8 unknown;
};


struct GMEmoteZone_Struct {
	char text[512];
};

struct Unknown_Struct {
	int8 unknown1[60*4];
};

// This is where the Text is sent to the client.
// Use ` as a newline character in the text.
// Variable length.
struct BookText_Struct {
	int16 unknown0;
	char* booktext; // Variable Length
};
// This is the request to read a book.
// This is just a "text file" on the server
// or in our case, the 'name' column in our books table.
struct BookRequest_Struct {
	int16 unknown0;
	char txtfile[20];
};

/*
** Object/Ground Spawn struct
** Used for Forges, Ovens, ground spawns, items dropped to ground, etc
** Size: 92 bytes
** OpCodes: OP_CreateObject
** Last Updated: Oct-17-2003
**
*/
struct Object_Struct {
/*00*/	uint32	linked_list_addr[2];// <Zaphod> They are, get this, prev and next, ala linked list
/*08*/	uint16	unknown008[2];		//
/*12*/	uint32	drop_id;			// Unique object id for zone
/*16*/	uint32	zone_id;			// Redudant, but: Zone the object appears in
/*20*/	uint32	unknown020[2];		//
/*28*/	float	heading;			// heading
/*32*/	float	z;					// z coord
/*36*/	float	y;					// y coord
/*40*/	float	x;					// x coord
/*44*/	char	object_name[16];	// Name of object, usually something like IT63_ACTORDEF
/*60*/	uint32	unknown060[5];		//
/*80*/	uint32	object_type;		// Type of object, not directly translated to OP_OpenObject
/*84*/	uint32	unknown084[1];		//
/*88*/	uint32	spawn_id;			// Spawn Id of client interacting with object
/*92*/
};
//<Zaphod> 01 = generic drop, 02 = armor, 19 = weapon
//[13:40] <Zaphod> and 0xff seems to be indicative of the tradeskill/openable items that end up returning the old style item type in the OP_OpenObject

/*
** Click Object Struct
** Client clicking on zone object (forge, groundspawn, etc)
** Size: 8 bytes
** Last Updated: Oct-17-2003
**
*/
struct ClickObject_Struct {
/*00*/	uint32 drop_id;
/*04*/	uint32 player_id;
/*08*/
};

/*
** Click Object Acknowledgement Struct
** Response to client clicking on a World Container (ie, forge)
** Size: 24 bytes
** Last Updated: Oct-17-2003
**
*/
struct ClickObjectAck_Struct {
/*00*/	uint32	player_id;	// Entity Id of player who clicked object
/*04*/	uint32	drop_id;	// Zone-specified unique object identifier
/*08*/	uint32	open;		// 1=opening, 0=closing
/*12*/	uint32	type;		// See object.h, "Object Types"
/*16*/	uint32	unknown16;	//
/*20*/	uint32	icon;		// Icon to display for tradeskill containers
/*24*/
};

/*
** Struct with two uses
**
*/
struct CloseContainer_Struct : public ClickObjectAck_Struct {
};

/*
** Generic Door Struct
** Length: 52 Octets
** Used in: 
**    cDoorSpawnsStruct(f721)
**
*/
struct Door_Struct
{
/*0000*/ char    name[16];            // Filename of Door // Was 10char long before... added the 6 in the next unknown to it: Daeken M. BlackBlade
/*0016*/ float   yPos;               // y loc
/*0020*/ float   xPos;               // x loc
/*0024*/ float   zPos;               // z loc
/*0028*/ float	 heading;
/*0032*/ int32   incline;	// rotates the whole door
/*0036*/ int16   size;			// 100 is normal, smaller number = smaller model
/*0038*/ int8    unknown0038[6];
/*0044*/ uint8   doorId;             // door's id #
/*0045*/ uint8   opentype;
/*
 *  Open types:
 * 66 = PORT1414 (Qeynos)
 * 55 = BBBOARD (Qeynos)
 * 100 = QEYLAMP (Qeynos)
 * 56 = CHEST1 (Qeynos)
 * 5 = DOOR1 (Qeynos)
 */
/*0046*/ uint8  state_at_spawn;
/*0047*/ uint8  invert_state;	// if this is 1, the door is normally open
/*0048*/ int32  door_param;
/*0052*/ uint8  unknown0052[12]; // mostly 0s, the last 3 bytes are something tho
};



struct DoorSpawns_Struct	//SEQ
{
	struct Door_Struct doors[0];
};

/*
 OP Code: 	Op_ClickDoor
 Size:		16

 10/10/2003-Doodman	Filled in struct
*/
struct ClickDoor_Struct {
/*000*/	int8	doorid;
/*001*/	int8	unknown001[3];
/*004*/	int8	picklockskill;
/*005*/	int8	unknown005[3];
/*008*/ int32	item_id;
/*012*/ int16	player_id;
/*014*/ int8	unknown014[2];
};

struct MoveDoor_Struct {
	int8	doorid;
	int8	action;
};


struct BecomeNPC_Struct {
	int32 id;
	long maxlevel;
};

struct Underworld_Struct {
	float speed;
	float y;
	float x;
	float z;
};

struct Resurrect_Struct	//160
{
	int16	unknown_01;            
	char	zone[15];
	int8	unknown_02[19];
	float	y;
	float	x;
	float	z;
	int32	unknown_02_1;
	char	your_name[64];
	int8	unknown_03[6];
	char	rezzer_name[64];
	int8	unknown_04[2];
	int16	spellid;
	char	corpse_name[64];
	int8	unknown_05[4];
	int32	action;
};

struct SetRunMode_Struct {
	int8 mode;
	int8 unknown[3];
};

//EnvDamage is EnvDamage2 without a few bytes at the end.

struct EnvDamage2_Struct {
/*0000*/	int32 id;
/*0004*/	int16 unknown4;
/*0006*/	int32 damage;
/*0010*/	int8 unknown10[12];
/*0022*/	int8 dmgtype; //FA = Lava; FC = Falling
/*0023*/	int8 unknown2[4];
/*0027*/	int16 constant; //Always FFFF
/*0029*/	int16 unknown29;
};

//Bazaar Stuff =D

struct BazaarWindowStart_Struct {
	int8   action;
	int8   unknown1;
	int16  unknown2;
};


struct BazaarWelcome_Struct {
	BazaarWindowStart_Struct beginning;
	int32  traders;
	int32  items;
	int8   unknown1[8];
};

struct BazaarSearch_Struct {
	BazaarWindowStart_Struct beginning;
	int32	traderid;
	int32  class_;
	int32  race;
	int32  stat;
	int32  slot;
	int32  type;
	char   name[64];
	int32	minprice;
	int32	maxprice;
};
struct BazaarInspect_Struct{
	int32 item_id;
	int32 unknown;
	char name[64];
};
struct BazaarReturnDone_Struct{
	int32 type;
	int32 traderid;
	int32 unknown8;
	int32 unknown12;
	int32 unknown16;
};
struct BazaarSearchResults_Struct {
	BazaarWindowStart_Struct beginning;
	int32	numitems;
	int32	item_id;
	int32	seller_nr;
	int32	cost;
	int32	unknown20;
	char	name[64];
};
struct BazaarInspectItem_Struct {
	int8   action;
	int8   unknown1;
#define SIZEOFITEMSTRUCT sizeof(Item_Struct)
	int8   itemdata[SIZEOFITEMSTRUCT];
};

struct SpecialMsg_Struct {
	int8 unknown[6]; //Always: 00 00 37 04 0a 00 for syswide
	char message[];
};

struct ServerSideFilters_Struct {
int8	clientattackfilters; // 0) No, 1) All (players) but self, 2) All (players) but group
int8	npcattackfilters;	 // 0) No, 1) Ignore NPC misses (all), 2) Ignore NPC Misses + Attacks (all but self), 3) Ignores NPC Misses + Attacks (all but group)
int8	clientcastfilters;	 // 0) No, 1) Ignore PC Casts (all), 2) Ignore PC Casts (not directed towards self)
int8	npccastfilters;		 // 0) No, 1) Ignore NPC Casts (all), 2) Ignore NPC Casts (not directed towards self)
};

/*
** Client requesting item statistics
** Size: 32 bytes
** Used In: OP_ItemLinkClick
** Last Updated: Sept-19-2003
**
*/
struct	ItemViewRequest_Struct {
/*000*/	uint32	item_id;
/*004*/	char	unknown004[28];
/*076*/
};

struct ItemViewResponse_Struct {
struct Item_Struct item;
};

/*
 *  Client to server packet
 */
struct PickPocket_Struct {
// Size 18
    uint32 to;
    uint32 from;
    uint8 myskill;
    uint8 unknown0;
    uint8 type; // -1 you are being picked, 0 failed , 1 = plat, 2 = gold, 3 = silver, 4 = copper, 5 = item
    uint8 unknown1; // 0 for response, unknown for input
    uint32 coin;
    uint8 lastsix[2];
};
/*
 * Server to client packet
 */

struct sPickPocket_Struct {
	// Size 28 = coin/fail
	uint32 to;
	uint32 from;
	uint32 myskill;
	uint32 type;
	uint32 coin;
	uint32 unknowns[2];
};
struct sItem_PickPocket_Struct {
	// Size 524 = Item
	uint32 to;
	uint32 from;
	uint32 myskill;
	uint32 type;
	uint32 coin;
	Item_Struct item;
	uint32 unknowns[2];
};

struct LogServer_Struct {
// Size 252
// Size 252
// Op_Code OP_LOGSERVER
    uint8   unknown252[256];
};

struct ApproveWorld_Struct {
// Size 544
// Op_Code OP_ApproveWorld
    uint8 unknown544[544];
};

struct ClientError_Struct
{
/*00000*/	char	unknown0000[69];
/*00069*/	char	character_name[64];
/*00133*/	char	message[32067];
/*32136*/
};

struct MobHealth
{
	/*0000*/	int8	hp; //health percent
	/*0001*/	int16	id;//mobs id
};

struct ItemLink_Struct {
/*0000*/	int8	unknown[12];
/*0012*/	int8	begin; //always 0x12
/*0013*/	char*	itemnumber;
/*0020*/	int16	unknown20[4];
/*0028*/	char *	itemname;
/*0029*/	int8	end; //always 0x12
};

struct Track_Struct {
	int16 entityid;
	int16 y;
	int16 x;
	int16 z;
};

struct Tracking_Struct {
	Track_Struct Entrys[0];
};

























// @Bigpull: 0.4.4 item struct for InjectToRaw()
struct OLDItem_Struct
{
/*0000*/ char      name[64];        // Name of item
/*0064*/ char      lore[80];        // Lore text
/*0144*/ char      idfile[30];      // This is the filename of the item graphic when held/worn.
									// Changed to 30 bytes for correct item conversion
/*0174*/ uint8	   weight;          // Weight of item
/*0175*/ sint8     nosave;          // Nosave flag 1=normal, 0=nosave, -1=spell?
/*0176*/ sint8     nodrop;          // Nodrop flag 1=normal, 0=nodrop, -1=??
/*0177*/ uint8     size;            // Size of item
/*0178*/ int8      type;
/*0179*/ uint8     unknown0178;     // ***Placeholder
/*0180*/ uint16    item_nr;         // Unique Item number
/*0182*/ uint16    icon_nr;         // Icon Number
/*0184*/ sint16    equipSlot;       // Current Equip slot
/*0186*/ uint8     unknwn0186[2];   // Equip slot cont.?
/*0188*/ uint32    equipableSlots;  // Slots where this item goes
/*0192*/ sint32    cost;            // Item cost in copper
/*0196*/ uint8     unknown0196[32]; // ***Placeholder
union
{
	struct
	{
	// 0228- have different meanings depending on flags
	/*0228*/ sint8    STR;              // Strength
	/*0229*/ sint8    STA;              // Stamina
	/*0230*/ sint8    CHA;              // Charisma
	/*0231*/ sint8    DEX;              // Dexterity
	/*0232*/ sint8    INT;              // Intelligence
	/*0233*/ sint8    AGI;              // Agility
	/*0234*/ sint8    WIS;              // Wisdom
	/*0235*/ sint8    MR;               // Magic Resistance
	/*0236*/ sint8    FR;               // Fire Resistance
	/*0237*/ sint8    CR;               // Cold Resistance
	/*0238*/ sint8    DR;               // Disease Resistance
	/*0239*/ sint8    PR;               // Poison Resistance
	/*0240*/ sint16   HP;               // Hitpoints
	/*0242*/ sint16   MANA;             // Mana
	/*0244*/ sint16   AC;               // Armor Class
	/*0246*/ uint8    MaxCharges;       // Maximum number of charges, for rechargable? (Sept 25, 2002)
	/*0247*/ sint8    GMFlag;           // GM flag 0  - normal item, -1 - gm item (Sept 25, 2002)
	/*0248*/ uint8    light;            // Light effect of this item
	/*0249*/ uint8    delay;            // Weapon Delay
	/*0250*/ uint8    damage;           // Weapon Damage
	/*0251*/ sint8    effecttype0;      // 0=combat, 1=click anywhere w/o class check, 2=latent/worn, 3=click anywhere EXPENDABLE, 4=click worn, 5=click anywhere w/ class check, -1=no effect
	/*0252*/ uint8    range;            // Range of weapon
	/*0253*/ uint8    skill;            // Skill of this weapon, refer to weaponskill chart
	/*0254*/ sint8    magic;            // Magic flag
                        //   00  (0000)  =   ???
                        //   01  (0001)  =  magic
                        //   12  (1100)  =   ???
                        //   14  (1110)  =   ???
                        //   15  (1111)  =   ???
	/*0255*/ sint8    level0;           // Casting level
	/*0256*/ uint8    material;         // Material?
	/*0257*/ uint8    unknown0258[3];   // ***Placeholder
	/*0260*/ uint32   color;            // Amounts of RGB in original color
	/*0264*/ uint8    unknown0264[2];   // ***Placeholder (Asiel: Has to do with Diety, will unwrap later)
	/*0266*/ uint16   spellId0;         // SpellID of special effect
	/*0268*/ uint16   classes;          // Classes that can use this item
	/*0270*/ uint8    unknown0270[2];   // ***Placeholder
	union
	{
		struct
		{
		/*0272*/ uint16   races;            // Races that can use this item
		/*0274*/ sint8    unknown0274[2];   // ***Placeholder
		/*0276*/ sint8    stackable;        //  1= stackable, 3 = normal, 0 = ? (not stackable)
		} normal;
	};
	/*0277*/ uint8    level;            // Casting level
	union // 0278 has different meanings depending on an stackable
	{
	/*0278*/ sint8    number;          // Number of items in stack
	/*0278*/ int8    charges;         // Number of charges (-1 = unlimited)
	};
	/*0279*/ sint8    effecttype;      // 0=combat, 1=click anywhere w/o class check, 2=latent/worn, 3=click anywhere EXPENDABLE, 4=click worn, 5=click anywhere w/ class check, -1=no effect
		 uint16   spellId;         // spellId of special effect
		 uint8    unknown0282[10]; // ***Placeholder 0288
		 uint32   casttime;        // Cast time of clicky item in miliseconds
		 uint8    unknown0296[16]; // ***Placeholder
		 uint16   skillModId;
		 sint16   skillModPercent;
		 sint16   BaneDMGRace;
		 sint16   BaneDMGBody;
			 // 1 Humanoid, 2 Lycanthrope, 3 Undead, 4 Giant, 5 Construct, 6 Extraplanar, 7 Magical
		 uint8    BaneDMG;
		 uint8    unknown0316[3];
		 uint8    RecLevel;         // max should be 65
		 uint8    RecSkill;         // Max should be 252
		 uint8    unknown0325[2];
		 uint8    ElemDmgType; 
			// 1 Magic, 2 Fire, 3 Cold, 4 Poison, 5 Disease
		 uint8    ElemDmg;
		 uint8    unknown0330[22];
		 uint8    ReqLevel; // Required level
		 uint8    unknown0352[5];
	/*0358*/ int16    focusspellId;
	} common;
	struct // Book Structure (flag == 0x7669)
	{
	/*0228*/ sint8    unknown0172[6];      // ***Placeholder
	/*0234*/ char     file[15];            // Filename of book text on server
	/*0249*/ sint8    unknown0190[15];    // ***Placeholder
	} book;
	struct // containers flag == 0x5400 or 0x5450
	{
		/*0228*/ sint8    unknown0212[41];     // ***Placeholder
		/*0269*/ uint8    numSlots;        // number of slots in container
		/*0270*/ sint8    unknown0214;     // ***Placeholder
		/*0271*/ sint8    sizeCapacity;    // Maximum size item container can hold
		/*0272*/ uint8    weightReduction; // % weight reduction of container
		/*0273*/ uint8    unknown0273[3];     // ***Placeholder
		} container;
};
	inline bool	IsNormal() const		{ return (bool) (type == 0x00); } // ie, not book, not bag
	inline bool	IsBook() const			{ return (bool) (type == 0x02); }
	inline bool	IsBag() const			{ return (bool) (type == 0x01); }
	inline bool	IsStackable() const		{ return (bool) ((type == 0x00) && (common.normal.stackable == 1 || (common.effecttype == 0 && common.normal.stackable == 2))); }
inline bool	IsEquipable(int16 race, int16 class_) const		{
	 if (!this) { /*LogFile->write(EQEMuLog::Debug, "Database error: IsEquipable() called on null item.");*/ return false; }
	 bool israce = false,isclass = false;
	 if (type != 0x00 && equipableSlots == 0) {
	   //LogFile->write(EQEMuLog::Debug, "IsEquipable() type != 0x00 && equipableSlots == 0");
      return false;
     }
     else if (common.classes == 0) {
	   //LogFile->write(EQEMuLog::Debug, "IsEquipable() common.classes == 0");
         return false;
     }
     else if(common.classes == 32767 && common.normal.races == 32767){
         return true;
     }
  else {
     int16 classes_ = common.classes;
     int16 races_ = common.normal.races;

     if(common.classes == 32767) {
	      isclass = true;
     }
     if(common.normal.races == 32767) {
   	      israce = true;
     }
     for (int cur_class = 1; cur_class <= 15; cur_class++) {
        if (classes_ % 2 == 1) {
    	    if(cur_class == class_) {
    	        isclass = true;
	        }
        }
        classes_ = classes_/2;
     }
     for (int cur_race = 1; cur_race <= 14; cur_race++) {
        if (races_ %2 == 1) {
    	    if(cur_race == race) {
    		    israce = true;
   		    }
        }
        races_ = races_/2;
     }
  }
  //LogFile->write(EQEMuLog::Debug, "IsEquipable(%i,%i) israce:%i:%i isclass:%i:%i", race, class_, israce,common.normal.races, isclass, common.classes);
     if(israce && isclass)
          return true;
     else
          return false;
	}
	inline bool	IsExpendable() const	{ return (bool) (common.skill == 21 || common.skill == 11); }
	inline bool	IsGM() const			{ return (bool) (common.GMFlag == -1); }
	inline bool	IsLore() const			{ return (bool) (strstr(lore, "*") != 0); }
	inline bool	IsPendingLore() const	{ return (bool) (strstr(lore, "~") != 0); }
	inline bool	IsArtifact() const		{ return (bool) (strstr(lore, "#") != 0); }
	inline bool IsWeapon() const	{ return (bool) common.damage; }
};

/*
** ZoneServerInfo_Struct
** Zone server information
** Size: 130 bytes
** Used In: OP_ZoneServerInfo
**
*/
struct ZoneServerInfo_Struct
{
/*0000*/	char	ip[128];
/*0128*/	uint16	port;
};

struct CrossServerTell_Struct
{
/*0000*/	char*	ip;
/*0015*/	char	comma;
/*0016*/	int16	port;
/*0021*/	char	comma2;
/*0022*/	char*	servername;
/*0042*/	char	dot;
/*0043*/	char*	name;
/*0107*/	char	comma3;
/*0108*/	char	unknown108[8];
};

struct WhoAllPlayer{
	int32	formatstring;
	int32	pidstring;
	char*	name;
	int32	rankstring;
	char*	guild;
	int32	unknown80[2];
	int32	zonestring;
	int32	zone;
	int32	class_;
	int32	level;
	int32	race;
	char*	account;
	int32	unknown100;
};

struct WhoAllReturnStruct {
	int32	id;
	int32	playerineqstring;
	char	line[27];
	int8	unknown35; //0A
	int32	unknown36;//0s
	int32	playersinzonestring;
	int32	unknown44[2]; //0s
	int32	unknown52;//1
	int32	unknown56;//1
	int32	playercount;//1
	struct WhoAllPlayer player[];
};

struct Trader_Struct {
	int32	code;
	int32	itemid[160];
	int32	unknown;
	int32	itemcost[80];
};

struct ClickTrader_Struct {
	int32	code;
	int32	unknown[161];//damn soe this is totally pointless :/ but at least your finally using memset! Good job :) -LE
	int32	itemcost[80];
};

struct GetItems_Struct{
	int32	items[80];
};

struct BecomeTrader_Struct{
	int32 id;
	int32 code;
};

struct Trader_ShowItems_Struct{
	int32 code;
	int32 traderid;
	int32 unknown08[3];
};

struct TraderBuy_Struct{
	int32 unknown0;
	int32 traderid;
	int32 itemid;
	int32 unknown8;
	int32 price;
	int32 quantity;
	int32 slot_num;
	char  itemname[60];
};

struct TraderItemUpdate_Struct{
	int32 unknown0;
	int32 traderid;
	int8  fromslot;
	int8  toslot; //7?
	int16 charges;
};

struct MoneyUpdate_Struct{
	sint32 platinum;
	sint32 gold;
	sint32 silver;
	sint32 copper;
};

struct TraderDelItem_Struct{
	int32 slotid;
	int32 quantity;
	int32 unknown;
};

struct TraderClick_Struct{
	int32 traderid;
	int32 unknown4[2];
	int32 approval;
};

struct FormattedMessage_Struct{
	int32	unknown0;
	int32	string_id;
	int32	type;
	char	message[0];
};
struct SimpleMessage_Struct{
	int32	string_id;
	int32	color;
	int32	unknown8;
};
struct GuildMember{
	char	name[64];
	int32	level;
	int32	class_;
	int32	timelaston;
	int32	rank;
	char	publicnote[100];//whatever this is...
	int8	zoneid;
};
struct GuildMember_Struct { //not an eqlive sturct just one for us
	int32 count;
	int32 length;
	struct GuildMember member[];
};
struct GuildMOTD_Struct{
/*0000*/	int32	unknown0;
/*0004*/	char	name[64];
/*0068*/	int32	unknown65[17];
/*0136*/	char	motd[512];
};
struct GuildUpdate_PublicNote{
	int32	unknown0;
	char	name[64];
	char	target[64];
	char	note[100]; //we are cutting this off at 100, actually around 252
};
struct GuildDemoteStruct{
	char	name[64];
	char	target[64];
};
struct GuildRemoveStruct{
	char	target[64];
	char	name[64];
	int32	unknown128;
	int32	leaderstatus; //?
};
struct GuildMakeLeader{
	char	name[64];
	char	target[64];
};
struct BugStruct{
/*0000*/	char	chartype[64];
/*0064*/	char	name[96];
/*0160*/	char	ui[128];
/*0288*/	float	x;
/*0292*/	float	y;
/*0296*/	float	z;
/*0300*/	float	heading;
/*0304*/	int32	unknown304;
/*0308*/	int32	type;
/*0312*/	char	unknown312[2144];
/*2456*/	char	bug[1024];
/*3480*/	char	placeholder[2];
/*3482*/	char	system_info[4098];
};
struct Make_Pet_Struct { //Simple struct for getting pet info
	int8 level;
	int8 class_;
	int16 race;
	int8 texture;
	int8 pettype;
	float size;
	int8 type;
	int32 min_dmg;
	int32 max_dmg;
};
struct Ground_Spawn{
	float max_x;
	float max_y;
	float min_x;
	float min_y;
	float max_z;
	float heading;
	char name[16];
	int32 item;
	int32 max_allowed;
	int32 respawntimer;
};
struct Ground_Spawns {
	struct Ground_Spawn spawn[50]; //Assigned max number to allow
};
struct PetitionBug_Struct{
	int32	petition_number;
	int32	unknown4;
	char	accountname[64];
	int32	zoneid;
	char	name[64];
	int32	level;
	int32	class_;
	int32	race;
	int32	unknown152[3];
	int32	time;
	int32	unknown168;
	char	text[1028];
};

struct DyeStruct
{
	union
	{
		struct
		{
			struct Color_Struct head;
			struct Color_Struct chest;
			struct Color_Struct arms;
			struct Color_Struct wrists;
			struct Color_Struct hands;
			struct Color_Struct legs;
			struct Color_Struct feet;
			struct Color_Struct primary;	// you can't actually dye this
			struct Color_Struct secondary;	// or this
		}
		dyes;
		struct Color_Struct dye[9];
	};
};

struct ApproveZone_Struct {
	char	name[64];
	int32	zoneid;
	int32	approve;
};
struct ZoneInSendName_Struct {
	int32	unknown0;
	char	name[64];
	char	name2[64];
	int32	unknown132;
};
struct ZoneInSendName_Struct2 {
	int32	unknown0;
	char	name[64];
	int32	unknown68[145];
};
struct StartTribute_Struct{
	int32	client_id;
	int32	npc_id;
	int32	response;
};
struct Tribute{
	int32	cost;
	int32	level;
	int32	tribute_id;
};
struct TributeAbility_Struct {
	int32	list_id;
	Tribute tribute[10];
	int32	unknown124;
	char	name[0];
};

struct Split_Struct
{
	uint32	platinum;
	uint32	gold;
	uint32	silver;
	uint32	copper;
};
struct PRange_Struct{
	float rx1;
	float rx2;
	float rx3;
	float rx4;
	float ry1;
	float ry2;
	float ry3;
	float ry4;
	float rz1;
	float rz2;
	float rz3;
	float rz4;
	bool p1set;
	bool p2set;
	bool p3set;
	bool p4set;
};

//client requesting favorite recipies
struct TradeskillFavorites_Struct {
	unsigned long object_type;
	unsigned long some_id;
	unsigned long favorite_recipes[100];
};

//search request
struct RecipesSearch_Struct {
	unsigned long object_type;	//same as in favorites
	unsigned long some_id;			//same as in favorites
	unsigned long mintrivial;
	unsigned long maxtrivial;
	char query[56];
	unsigned long unknown4;	//is set to 00 03 00 00
	unsigned long unknown5; //is set to 4C DD 12 00
};

//one sent for each item, from server in reply to favorites or search
struct RecipeReply_Struct {
	unsigned long object_type;
	unsigned long some_id;	 //same as in favorites
	unsigned long component_count;
	unsigned long recipe_id;
	unsigned long trivial;
	char recipe_name[64];
};

//received and sent back as an ACK with different reply_code
struct RecipeAutoCombine_Struct {
	unsigned long object_type;
	unsigned long some_id;
	unsigned long unknown1;		//echoed in reply
	unsigned long recipe_id;
	unsigned long reply_code;		// 93 64 e1 00 in request
								// 00 00 00 00 in successful reply
								// f5 ff ff ff in 'you dont have all the stuff' reply
};




// Restore structure packing to default
#pragma pack()

#endif
