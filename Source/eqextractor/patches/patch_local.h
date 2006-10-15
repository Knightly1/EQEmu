#ifndef EQE_PATCH_LOCAL_H
#define EQE_PATCH_LOCAL_H

#include "../common/types.h"

//this needs to be outside of the namespaces.
#include "ExtractCollector.h"
#include "patches/versions.h"
#include "../common/buildfile.h"
#include "BuildWriterInterface.h"

namespace EQE_Patch_Local {
	using namespace std;
	using namespace EQExtractor;
	
/**********************************************************************************************************
	eq_packet_structs.h
**********************************************************************************************************/	

#pragma pack(1)
static const uint32 BUFF_COUNT = 25;

struct Door_Struct
{
/*0000*/ char    name[16];            // Filename of Door // Was 10char long before... added the 6 in the next unknown to it: Daeken M. BlackBlade
/*0016*/ char    unknown0016[16];
/*0032*/ float   yPos;               // y loc
/*0036*/ float   xPos;               // x loc
/*0040*/ float   zPos;               // z loc
/*0044*/ float	 heading;
/*0048*/ int32   incline;	// rotates the whole door
/*0052*/ int16   size;			// 100 is normal, smaller number = smaller model
/*0054*/ int8    unknown0038[6];
/*0060*/ uint8   doorId;             // door's id #
/*0061*/ uint8   opentype;
/*
 *  Open types:
 * 66 = PORT1414 (Qeynos)
 * 55 = BBBOARD (Qeynos)
 * 100 = QEYLAMP (Qeynos)
 * 56 = CHEST1 (Qeynos)
 * 5 = DOOR1 (Qeynos)
 */
/*0062*/ uint8  state_at_spawn;
/*0063*/ uint8  invert_state;	// if this is 1, the door is normally open
/*0064*/ int32  door_param;
/*0068*/ uint8  unknown0052[12]; // mostly 0s, the last 3 bytes are something tho
/*0080*/
};


struct AA_Ability {
/*00*/	int32 skill_id;
/*04*/	int32 increase_amt;
/*08*/	int32 unknown08;
/*12*/	int32 last_level;
};
struct SendAA_Struct {
/*0000*/	int32 id;
/*0004*/	int32 hotkey_sid;
/*0008*/	int32 hotkey_sid2;
/*0012*/	int32 title_sid;
/*0016*/	int32 desc_sid;
/*0020*/	int32 class_type;
/*0024*/	int32 cost;
/*0028*/	int32 seq;
/*0032*/	int32 current_level; //1s
/*0036*/	int32 prereq_skill;
/*0040*/	int32 prereq_minpoints; //min points in the prereq
/*0044*/	int32 type;
/*0048*/	int32 spellid;
/*0052*/	int32 spell_type;
/*0056*/	int32 spell_refresh;
/*0060*/	int16 classes;
/*0062*/	int16 berserker; //seems to be 1 if its a berserker ability
/*0064*/	int32 max_level;
/*0068*/	int32 last_id;
/*0072*/	int32 next_id;
/*0076*/	int32 cost2;
/*0080*/	int32 unknown80[2]; //0s
/*0084*/	int32 total_abilities;
/*0088*/	AA_Ability abilities[0];
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
/*0512*/	float	minclip;				// Minimum View Distance
/*0516*/	float	maxclip;				// Maximum View DIstance
/*0520*/	int8	unknown_end[84];		// ***Placeholder
/*0604*/	char	zone_short_name2[68];
/*0672*/	char	unknown672[12];
/*0684*/	uint16	zone_id;
/*0686*/	uint16	zone_instance;
};

struct Object_Struct {
/*00*/	uint32	linked_list_addr[2];// <Zaphod> They are, get this, prev and next, ala linked list
/*08*/	uint16	unknown008[2];		//
/*12*/	uint32	drop_id;			// Unique object id for zone
/*16*/	uint16	zone_id;			// Redudant, but: Zone the object appears in
/*18*/	uint16	zone_instance;		//
/*20*/	uint32	unknown020;			//
/*24*/	uint32	unknown024;			//
/*28*/	float	heading;			// heading
/*32*/	float	z;					// z coord
/*36*/	float	y;					// y coord
/*40*/	float	x;					// x coord
/*44*/	char	object_name[20];	// Name of object, usually something like IT63_ACTORDEF
/*64*/	uint32	unknown064;			//
/*68*/	uint32	unknown068;			//
/*72*/	uint32	unknown072;			//
/*76*/	uint32	unknown076;			//
/*80*/	uint32	object_type;		// Type of object, not directly translated to OP_OpenObject
/*84*/	uint32	unknown084;			//set to 0xFF
/*88*/	uint32	spawn_id;			// Spawn Id of client interacting with object
/*92*/
};

struct ZonePoint_Entry {
/*0000*/	int32	iterator;
/*0004*/	float	y;
/*0008*/	float	x;
/*0012*/	float	z;
/*0016*/	float	heading;
/*0020*/	int16	zoneid;
/*0022*/	int16	zoneinstance; // LDoN instance
};

#define MAX_TRIBUTE_TIERS 10
struct TributeLevel_Struct {
   uint32	level;	//backwards byte order!
   int32	tribute_item_id;	//backwards byte order!
   int32	cost;	//backwards byte order!
};

struct TributeAbility_Struct {
	int32	tribute_id;	//backwards byte order!
	int32	tier_count;	//backwards byte order!
	TributeLevel_Struct tiers[MAX_TRIBUTE_TIERS];
	char	name[0];
};

struct SelectTributeReply_Struct {
   int32	client_id;	//echoed from request.
   uint32	tribute_id;
   char	desc[0];
};

struct BookText_Struct {
	uint8 unknown0; //always 0xFF
	uint8 type;             //type: 0=scroll, 1=book.. prolly others.
	char booktext[0]; // Variable Length
};

struct TitleEntry_Struct {
	uint32 skill_id;
	uint32 skill_value;
	char title[1];
};

struct Titles_Struct {
	uint32  title_count;
//logically, but not valid due to dynamic lengths
	TitleEntry_Struct titles[0];
};


struct TaskHistoryEntry_Struct {
	uint32	task_id;
	char	name[1];
	uint32	completed_time;
};
struct TaskHistory_Struct {
	uint32 completed_count;
	TaskHistoryEntry_Struct entries[0];
};

struct Color_Struct{
	union {
		struct {
			int8	blue;
			int8	green;
			int8	red;
			uint8	use_tint;	// if there's a tint this is FF
		} rgb;
		uint32 color;
	};
};

struct Spawn_Struct
{
/*000*/	int8	NPC;	// 0=player,1=npc,2=pc corpse,3=npc corpse,4=???,5=unknown spawn,10=self
/*001*/	int8	beard;			// vesuvias - appearance fix
/*002*/	int8	beardcolor;			// Player right eye color
/*003*/	int8	aa_title; // 0=none, 1=general, 2=archtype, 3=class
/*004*/	Color_Struct	dye_rgb[7]; 			// armor dye colors
/*032*/ int8	unknown032[11];
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
/*122*/ int8	unknown122; // 0=Not pvp,1=pvp  solar: this is wrong
/*123*/ int8	level; 
/*124*/ int8	lfg; // 0=Not lfg,1=lfg 
/*125*/ int32	heading:12; // spawn heading 
/*****/ int32	deltaHeading:10; // change in heading 
/*****/ int32	animation:10; // animation id 
/*129*/ sint32	deltaX:13; 
/*****/ sint32	x:19; 
/*133*/ sint32	y:19; 
/*****/ sint32	deltaZ:13; 
/*137*/ sint32	deltaY:13; 
/*****/ sint32	z:19; 
/*141*/ int8	hairstyle;	// vesuvias
/*142*/ int8	haircolor;
/*143*/ int8	invis2;		//not sure...
/*144*/ int8	unknown144[5];
/*149*/ int8	pvp;	//according to Wiz --verified (LE)
/*150*/ int8	light;
/*151*/ float	size; // Size 
/*155*/ int8	helm; 
/*156*/ float	runspeed; // 
/*160*/ int8	gm; // 0=not GM,1=GM 
/*161*/ float	walkspeed; // 
/*165*/ int32	guild_id; // GuildID 
/*169*/ int8	anon; // 0=normal,1=anon,2=roleplaying 
/*170*/ int8	gender; // 0=male,1=female,2=other 
/*171*/ int16	spawnId; // Id of spawn 
/*173*/ int8	unknown173[3]; 
/*176*/ char	lastName[32]; // lastname 
/*208*/ int32	equipment[9]; 
/*244*/ int8	linkdead; // 0=Not LD, 1=LD 
/*245*/ uint32	bodytype; // Bodytype 
/*249*/	int8	guild_rank;
/*250*/ int8	unknown249[4]; 
/*254*/ uint32	petOwnerId; 
/*258*/ int16	deity; 
/*260*/ int8	unknown260[6];
/*266*/ int8	findable;	//can be found with find command.
/*268*/ int8	unknown267[40];
/*308*/ char	title[48];	//len might be wrong
/*355*/ int8	unknown355[16];
/*371*/ int8	unknown367[8];	//all 
/*379*/
};

struct SpawnPositionUpdate_Struct
{
/*0000*/ uint16		spawn_id;
/*0002*/ sint64		y:19, z:19, x:19, u3:7;
/*0010*/ unsigned short	heading:12,unused2:4;
/*0012*/
};

struct PlayerPositionUpdateServer_Struct
{
/*00*/ uint16		spawn_id;
/*02*/ sint32		delta_z:13,
					animation:10,
					unknown02:9;
/*06*/ sint32   delta_x:13,
				y_pos:19;
/*10*/ sint32	delta_y:13,
				z_pos:19;
/*14*/ sint32	heading:12,
				delta_heading:10,
				unknownbits:10;		//seems to be 0 or -1
/*18*/ sint32   x_pos:19,
				unknown18:13;
/*22*/
};

struct Death_Struct
{
/*000*/	int32	spawn_id;
/*004*/	int32	killer_id;
/*008*/	int32	corpseid;	// was corpseid
/*012*/	int32	attack_skill;	// was type
/*016*/	int32	spell_id;
/*020*/ int32	bindzoneid;	//bindzoneid?
/*024*/	int32	damage;
/*028*/	int32	unknown028;
};


struct AA_Array
{
	int32 AA;
	int32 value;	
};

// solar: this is used inside profile
struct SpellBuff_Struct
{
/*000*/	int8	slotid;		//badly named... seems to be 2 for a real buff, 0 otherwise
/*001*/ int8	level;
/*002*/	int8	bard_modifier;
/*003*/	int8	effect;			//not real
/*004*/	int32	spellid;
/*008*/ int32	duration;
/*012*/	int16	dmg_shield_remaining;
//these last four bytes are really the caster's global player ID for wearoff
/*013*/ int8	diseasecounters;	//prolly not real
/*014*/ int8	poisoncounters;		//proll not real
/*012*/	int8	Unknown012[4];
};

static const uint32 MAX_PP_DISCIPLINES = 100;

struct Disciplines_Struct {
	uint32 values[MAX_PP_DISCIPLINES];
};

static const uint32 MAX_PLAYER_TRIBUTES = 5;
static const uint32 MAX_PLAYER_BANDOLIER = 4;
static const uint32 MAX_PLAYER_BANDOLIER_ITEMS = 4;
static const uint32 TRIBUTE_NONE = 0xFFFFFFFF;
struct Tribute_Struct {
	uint32 tribute;
	uint32 tier;
};

//len = 72
struct BandolierItem_Struct {
	uint32 item_id;
	uint32 icon;
	char item_name[64];
};

//len = 320
enum { //bandolier item positions
	bandolierMainHand = 0,
	bandolierOffHand,
	bandolierRange,
	bandolierAmmo
};
struct Bandolier_Struct {
	char name[32];
	BandolierItem_Struct items[MAX_PLAYER_BANDOLIER_ITEMS];
};
struct PotionBelt_Struct {
	BandolierItem_Struct items[MAX_PLAYER_BANDOLIER_ITEMS];
};
	
static const uint32 MAX_GROUP_LEADERSHIP_AA_ARRAY = 16;
static const uint32 MAX_RAID_LEADERSHIP_AA_ARRAY = 16;
static const uint32 MAX_LEADERSHIP_AA_ARRAY = (MAX_GROUP_LEADERSHIP_AA_ARRAY+MAX_RAID_LEADERSHIP_AA_ARRAY);
struct LeadershipAA_Struct {
	uint32 ranks[MAX_LEADERSHIP_AA_ARRAY];
};
struct GroupLeadershipAA_Struct {
	uint32 ranks[MAX_GROUP_LEADERSHIP_AA_ARRAY];
};
struct RaidLeadershipAA_Struct {
	uint32 ranks[MAX_RAID_LEADERSHIP_AA_ARRAY];
};

/*
** Player Profile
**
** Length: 4308 bytes
** OpCode: 0x006a
 */
static const uint32 MAX_PP_LANGUAGE		= 28;
static const uint32 MAX_PP_SPELLBOOK	= 400;
static const uint32 MAX_PP_MEMSPELL		= 9;
static const uint32 MAX_PP_SKILL		= 75;
static const uint32 MAX_PP_AA_ARRAY		= 240;
static const uint32 MAX_GROUP_MEMBERS	= 6;
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
/*0224*/	uint32				guild_id;				
/*0228*/	uint32				birthday;			// characters bday
/*0232*/	uint32				lastlogin;			// last login or zone time
/*0236*/	uint32				timePlayedMin;			// in minutes
/*0240*/	uint8				pvp;			
/*0241*/	uint8				level2; //no idea why this is here, but thats how it is on live
/*0242*/	uint8				anon;		// 2=roleplay, 1=anon, 0=not anon
/*0243*/	uint8				gm;
/*0244*/	uint8				guildrank;
/*0245*/	uint8				unknown0245[7];	//
/*0252*/	uint32				intoxication;		
/*0256*/	uint32				spellSlotRefresh[MAX_PP_MEMSPELL];	//in ms
/*0292*/	uint8				unknown0392[4];
/*0296*/	uint8				haircolor;			// Player hair color
/*0297*/	uint8				beardcolor;			// Player beard color
/*0298*/	uint8				eyecolor1;			// Player left eye color
/*0299*/	uint8				eyecolor2;			// Player right eye color
/*0300*/	uint8				hairstyle;			// Player hair style
/*0301*/	uint8				beard;				// Beard type
/*0302*/	uint8				ability_time_seconds; //The following four spots are unknown right now.....
/*0303*/	uint8				ability_number; //ability used
/*0304*/	uint8				ability_time_minutes;
/*0305*/	uint8				ability_time_hours;//place holder
/*0306*/	uint8				unknown0306[6];		// @bp Spacer/Flag?
/*0312*/	uint32				item_material[9];	// Item texture/material of worn/held items
/*0348*/	uint8				unknown0256[44];
/*0396*/	Color_Struct		item_tint[9];
/*0432*/	AA_Array			aa_array[MAX_PP_AA_ARRAY];
/*2348*/	float				unknown2348;		//seen ~128, ~47
/*2352*/	char				servername[32];		// length probably not right
/*2384*/	char				title[32];			//length might be wrong
/*2416*/	char				suffix[32];			//length might be wrong
/*2448*/	uint32				guildid2;		//
/*2452*/	uint32				exp;				// Current Experience
/*2456*/	uint32				unknown1496;		
/*2460*/	uint32				points;				// Unspent Practice points
/*2464*/	uint32				mana;				// current mana
/*2468*/	uint32				cur_hp;				// current hp
/*2472*/	uint32				unknown1512;		// 0x05
/*2476*/	uint32				STR;				// Strength
/*2480*/	uint32				STA;				// Stamina
/*2484*/	uint32				CHA;				// Charisma
/*2488*/	uint32				DEX;				// Dexterity
/*2492*/	uint32				INT;				// Intelligence
/*2496*/	uint32				AGI;				// Agility
/*2500*/	uint32				WIS;				// Wisdom
/*2504*/	uint8				face;				// Player face
/*2505*/	uint8				unknown1545[47];	// ?
/*2552*/	uint8				languages[MAX_PP_LANGUAGE];
/*2580*/	uint8				unknown1620[4];
/*2584*/	int32				spell_book[MAX_PP_SPELLBOOK];
/*4184*/	uint8				unknown3224[448];	// all 0xff   
/*4632*/	int32				mem_spells[MAX_PP_MEMSPELL];
/*4668*/	uint8				unknown3704[32];	//
/*4700*/	float				y;					// Player y position
/*4704*/	float				x;					// Player x position
/*4708*/	float				z;					// Player z position
/*4712*/	float				heading;			// Direction player is facing
/*4716*/	uint8				unknown3756[4];		//
/*4720*/	sint32				platinum;			// Platinum Pieces on player
/*4724*/	sint32				gold;				// Gold Pieces on player
/*4728*/	sint32				silver;				// Silver Pieces on player
/*4732*/	sint32				copper;				// Copper Pieces on player
/*4736*/	sint32				platinum_bank;		// Platinum Pieces in Bank
/*4740*/	sint32				gold_bank;			// Gold Pieces in Bank
/*4744*/	sint32				silver_bank;		// Silver Pieces in Bank
/*4748*/	sint32				copper_bank;		// Copper Pieces in Bank
/*4752*/	sint32				platinum_cursor;	// Platinum on cursor
/*4756*/	sint32				gold_cursor;		// Gold on cursor
/*4760*/	sint32				silver_cursor;		// Silver on cursor
/*4764*/	sint32				copper_cursor;		// Copper on cursor
/*4768*/	sint32				platinum_shared;        // Platinum shared between characters
/*4772*/	uint8				unknown3812[24];        // @bp unknown skills?
/*4796*/	uint32				skills[MAX_PP_SKILL];
/*5096*/	uint8				unknown5096[284];     // @bp unknown skills?
/*5380*/	int32				pvp2;	//
/*5384*/	int32				unknown4420;	//
/*5388*/	int32				pvptype;	//
/*5392*/	int32				unknown4428;	//
/*5396*/	uint32				ability_down;			// Doodman - Guessing
/*5400*/	uint8				unknown4436[8];	//
/*5408*/	uint32				autosplit;			//not used right now
/*5412*/	uint8				unknown4448[8];
/*5420*/	int32				zone_change_count;      // Number of times user has zoned in their career (guessing)
/*5424*/	uint8				unknown4460[28];	//
/*5452*/	int32				expansions;		// expansion setting, bit field of expansions avaliable
/*5456*/	sint32				toxicity;	//from drinking potions, seems to increase by 3 each time you drink
/*5460*/	char				unknown4496[16];	//
/*5476*/	sint32				hunger_level;
/*5480*/	sint32				thirst_level;
/*5484*/	int32				ability_up;
/*5488*/	char				unknown4524[16];				
/*5504*/	uint16				zone_id;			// Current zone of the player
/*5506*/	uint16				zoneInstance;			// Instance ID
/*5508*/	SpellBuff_Struct	buffs[BUFF_COUNT];			// Buffs currently on the player
/*6008*/	char 				groupMembers[6][64];		//
/*6392*/	char				unknown6392[668];
/*7060*/	sint32				ldon_points_guk;		//client uses these as signed
/*7064*/	sint32				ldon_points_mir;
/*7068*/	sint32				ldon_points_mmc;
/*7072*/	sint32				ldon_points_ruj;
/*7076*/	sint32				ldon_points_tak;
/*7080*/	sint32				ldon_points_available;
/*7084*/	uint8				unknown5940[112];
/*7196*/	uint32				tribute_time_remaining;	//in miliseconds
/*7200*/	uint32				unknown6048;
/*7204*/	uint32				career_tribute_points;
/*7208*/	uint32				unknown6056;
/*7212*/	uint32				tribute_points;
/*7216*/	uint32				unknown6064;
/*7220*/	uint32				tribute_active;		//1=active
/*7224*/	Tribute_Struct		tributes[MAX_PLAYER_TRIBUTES];
/*7264*/	Disciplines_Struct	disciplines;			//fathernitwit: 10-06-04
/*7664*/	char				unknown7464[240];
/*7904*/	uint32				endurance;
/*7908*/	uint32				group_leadership_exp;	//0-1000
/*7912*/	uint32				raid_leadership_exp;	//0-2000
/*7916*/	uint32				group_leadership_points;
/*7920*/	uint32				raid_leadership_points;
/*7924*/	LeadershipAA_Struct	leader_abilities;
/*8052*/	uint8				unknown8052[132];
/*8184*/	uint32				air_remaining;
/*8188*/	uint8				unknown8188[4608];
/*12796*/	uint32				aapoints_spent;
/*12800*/	uint32				expAA;
/*12804*/	uint32				aapoints;	//avaliable, unspent
/*12808*/	uint8				unknown12808[36];
/*12844*/	Bandolier_Struct	bandoliers[MAX_PLAYER_BANDOLIER];
/*14124*/	uint8				unknown14124[5120];
/*19244*/	PotionBelt_Struct	potionbelt;	//there should be 3 more of these
/*19532*/	uint8				unknown19532[8];
/*19540*/	uint32				currentRadCrystals; 	// Current count of radiant crystals
/*19544*/	uint32				careerRadCrystals; // Total count of radiant crystals ever
/*19548*/	uint32				currentEbonCrystals;		// Current count of ebon crystals
/*19552*/	uint32				careerEbonCrystals;	// Total count of ebon crystals ever
/*19556*/	uint8				groupAutoconsent;   // 0=off, 1=on
/*19557*/	uint8				raidAutoconsent;    // 0=off, 1=on
/*19558*/	uint8				guildAutoconsent;   // 0=off, 1=on
/*19559*/	uint8				unknown19559[5];    // ***Placeholder (6/29/2005)
/*19564*/	uint32				unknown15964;
/*19568*/
};




struct DeleteSpawn_Struct
{
/*00*/ int32 spawn_id;             // Spawn ID to delete
/*04*/
};

struct BeginCast_Struct
{
	// len = 8
/*000*/	int16	caster_id;

/*002*/	int16	spell_id;
/*004*/	int32	cast_time;		// in miliseconds
};

struct Merchant_Click_Struct {
/*000*/ int32	npcid;			// Merchant NPC's entity id
/*004*/ int32	playerid;
/*008*/ int8	unknown[8]; 
};

enum ItemPacketType
{
	ItemPacketViewLink			= 0x00,
	ItemPacketTradeView			= 0x65,
	ItemPacketLoot				= 0x66,
	ItemPacketTrade				= 0x67,
	ItemPacketCharInventory		= 0x69,
	ItemPacketSummonItem		= 0x6A,
	ItemPacketTributeItem		= 0x6C,
	ItemPacketMerchant			= 0x64,
	ItemPacketWorldContainer	= 0x6B
};
struct ItemPacket_Struct
{
/*00*/	ItemPacketType	PacketType;
/*04*/	char			SerializedItem[1];
/*xx*/
};


struct NewSpawn_Struct
{
	struct Spawn_Struct spawn;	// Spawn Information
};

struct ServerZoneEntry_Struct
{
	struct NewSpawn_Struct player;
};

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

//SpawnAppearance types:
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



#pragma pack()

#include "cleardefs.h"

/**********************************************************************************************************
     Methods:
**********************************************************************************************************/	

	//declare all the extractors
	#include "Extractors.h"
	
	//our packet file writer
	#include "BuildWriter.h"

	//Declare the factory
	#include "factory.h"

};	//end namespace

#endif
