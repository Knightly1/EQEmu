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
#ifndef EQ_OLD_STRUCTS_H
#define EQ_OLD_STRUCTS_H 

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
/*07*/	sint32	damage;
/*11*/	int8	unknown_zero7[4];
/*17*/	float	heading2;
/*21*/	int8	unknown_zero21[4];
};
#endif


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


#if 0
struct SpellBuff_Struct
{
/*000*/	int8  b_unknown1[5];		// ***Placeholder
/*005*/	int8  level;				// Level of person who casted buff
/*006*/	int16  effect;				// ***Placeholder
/*008*/	int32 spellid;				// Spell
/*0012*/	int32 duration;				// Duration in ticks
};
#endif


struct Before_Dec15th_PlayerProfile_Struct
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
/*0240*/	uint8				pvp;			
/*0241*/	uint8				level2; //no idea why this is here, but thats how it is on live
/*0242*/	uint8				pvpon;
/*0243*/	uint8				gm;				// 1=gm, 0=not gm
/*0244*/	uint8				anon;		// 2=roleplay, 1=anon, 0=not anon
/*0245*/	uint8				guildrank;
/*0246*/	uint8				fatigue;  // Sta bar % depleted (ie, 30 = 70% sta)
/*0246*/	uint8				unknown0165[45];	//
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
/*1384*/	uint8				unknown1388[8];
/*1392*/	char				servername[32];
/*1424*/	char				title[64];
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
/*1592*/	uint8				languages[28];
/*1620*/	uint32				unknown1620;
/*1624*/	int32				spell_book[400];
/*3224*/	uint8				unknown3224[448];	// all 0xff   
/*3672*/	int32				mem_spells[8];
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
/*3832*/	uint32				skills[75];
/*4132*/	uint32				unknown_skills[45];	// @bp unknown skills?
/*4308*/	uint8				unknown3472[92];	//
/*4400*/	uint32				perAA;				// % on the AA exp bar
/*4404*/	uint8				unknown3564[12];	//28
/*4416*/	int32				pvp2;	//
/*4420*/	int32				unknown4420;	//
/*4424*/	int32				pvptype;	//
/*4428*/	int32				unknown4428;	//
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
/*4544*/	SpellBuff_Struct	buffs[20];			// Buffs currently on the player
/*4864*/	char 				groupMembers[6][64];		//
/*5248*/	int32				unknown5248;
/*5252*/	uint32				unknown5252;
/*5256*/	uint32				unknown4380[11];	//one word became pet stuff above
/*5300*/	uint32				adventure_id;		//this is WRONG!
/*5304*/	uint32				unknown4460[165];
/*5964*/	uint32				tribute_time_remaining;	//in miliseconds
/*5968*/	uint32				unknown5968;
/*5972*/	uint32				career_tribute_points;
/*5976*/	uint32				unknown5976;
/*5980*/	uint32				tribute_points;
/*5984*/	uint32				unknown5984;
/*5988*/	uint32				tribute_active;		//1=active
/*5992*/	Tribute_Struct		tributes[5];
/*6032*/	Disciplines_Struct	disciplines;			//fathernitwit: 10-06-04
/*6232*/	uint32				unknown5764[130];
/*6752*/	uint32				air_remaining;
/*6756*/	uint32				unknown6756[1152]; //added in last patch, crazy bastards
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
/*4420*/	SpellBuff_Struct	buffs[15];			// Buffs currently on the player
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
/*3704*/	SpellBuff_Struct	buffs[15];			// Buffs currently on the player
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
/*3704*/	SpellBuff_Struct	buffs[15];			// Buffs currently on the player
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
/*3704*/	SpellBuff_Struct	buffs[15];			// Buffs currently on the player
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
/*3704*/	SpellBuff_Struct	buffs[15];			// Buffs currently on the player
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
/*3704*/	SpellBuff_Struct	buffs[15];			// Buffs currently on the player
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
/*3704*/	SpellBuff_Struct	buffs[15];			// Buffs currently on the player
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
/*3704*/	SpellBuff_Struct	buffs[15];			// Buffs currently on the player
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

#if 0 // old stuff
struct BindWound_Struct {
// Size 4
    uint16  to; // entity id
    uint8    type; // This could be an int16
    // 0 or 1 complete, 2 Unknown, 3 ACK, 4 Died, 5 Left, 6 they moved, 7 you moved
    uint8    placeholder; //
};
#endif

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

/*struct DyeColorStruct{
	int8	blue;
	int8	green;
	int8	red;
	int8	unknown;
};*/

#endif
