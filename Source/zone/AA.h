#define MAX_AAS 120
#define MANA_BURN 664
#define Innate_Fire_Protection 37
#define Innate_Cold_Protection 42
#define Innate_Magic_Protection 47
#define Innate_Poison_Protection 52
#define Innate_Disease_Protection 57



struct AltAdvStats_Struct {
/*000*/  uint32 experience;
/*004*/  uint16 unspent;
/*006*/  uint16	unknown006;
/*008*/  int8	percentage;
/*009*/  int8	unknown009[3];
};

struct UseAA_Struct {
	int32 begin;
	int32 ability;
	int32 end;
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
/*0080*/	int32 unknown80; //0s
/*0084*/	int32 total_abilities;
/*0088*/	AA_Ability abilities[0];
};
struct AA_List{
	SendAA_Struct* aa[0];
};
struct AA_Action{
/*00*/	int32	action;
/*04*/	int32	ability;
/*08*/	int32	unknown08[2];
};
//  New Alternate Advancement table.  holds all the skill levels for the AA skills.
//  Length: 309 Bytes
//  OpCode: 1422
struct AA_Skills{
/*00*/	int32	aa_skill; 
/*04*/	int32	aa_value;
};
struct PlayerAA_Struct {
	AA_Skills aa_list[MAX_AAS];
};
