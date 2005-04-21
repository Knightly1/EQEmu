/*

EQ Extractor, by Father Nitwit 2005

*/
#include "Extractors.h"
//#include "../common/MiscFunctions.h"
//#include "ExtractDB.h"
#include <netinet/in.h>
//#include <mysql.h>

/*

More things to extract:
- recipe names and IDs from searching
- recipe ingredients from clicking
- seperate world containers and ground spawns
- tribute headers
- tribute descriptions
- task history
- task descriptions & activities

*/

#ifdef USE_CURRENT_STRUCTS
#include "../common/eq_packet_structs.h"
#else

#pragma pack(1)
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
	int32	unknown;	//backwards byte order!
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
	char title[0];
};

struct Titles_Struct {
	uint32  title_count;
//logically, but not valid due to dynamic lengths
//	TitleEntry_Struct titles[0];
};


struct TaskHistoryEntry_Struct {
	uint32	task_id;
	char	name[0];
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
/*000*/	int8	npc;	// 0=player,1=npc,2=pc corpse,3=npc corpse,4=???,5=unknown spawn,10=self
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
/*260*/ int8	unknown260[6];
/*266*/ int8	findable;	//can be found with find command.
/*268*/ int8	unknown267[40];
/*308*/ char	title[48];	//len might be wrong
/*355*/ int8	unknown355[16];
/*371*/ int8	unknown367[8];	//all 
/*379*/
};


#pragma pack()

#endif

ZoneInfoExtractor::ZoneInfoExtractor()
: ExtractBase() {
	zone_id = 0xFFFF;
}

void ZoneInfoExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len) {
	if(emu_op != OP_NewZone)
		return;
	if(len != sizeof(NewZone_Struct)) {
		printf("Size of newzone struct is invalid! Cannot get zone info.\n");
		return;
	}
	NewZone_Struct *i = (NewZone_Struct *) data;
	zone_id = i->zone_id;
	short_name = i->zone_short_name;
	long_name = i->zone_long_name;
//	fprintf(stderr, "Found zone info: %s (%s=%s) (%d, %d)\n", long_name.c_str(), short_name.c_str(), i->zone_short_name2, zone_id, i->zone_instance);
}


DoorExtractor::DoorExtractor(ZoneInfoExtractor *zi)
: ExtractCollector(OP_SpawnDoor, "doors")
{
	zone_info = zi;
	
	//fill out the field list
	//the primary key on this is not the true primary key due to fuzzy matching
	fields[DoorItem::doorid] = FieldInfo("doorid","", true, OnMissingError, vInt);
	fields[DoorItem::zone] = FieldInfo("zone","", true, OnMissingError, vString);
	fields[DoorItem::name] = FieldInfo("name","", false, OnMissingOmit, vString);
	fields[DoorItem::pos_x] = FieldInfo("pos_x","", false, OnMissingOmit, vFloat);
	fields[DoorItem::pos_y] = FieldInfo("pos_y","", false, OnMissingOmit, vFloat);
	fields[DoorItem::pos_z] = FieldInfo("pos_z","", false, OnMissingOmit, vFloat);
	fields[DoorItem::heading] = FieldInfo("heading","", false, OnMissingOmit, vFloat);
	fields[DoorItem::opentype] = FieldInfo("opentype","", false, OnMissingOmit, vIntUnsigned);
	fields[DoorItem::doorisopen] = FieldInfo("doorisopen","", false, OnMissingOmit, vIntUnsigned);
	fields[DoorItem::door_param] = FieldInfo("door_param","", false, OnMissingOmit, vIntUnsigned);
	fields[DoorItem::incline] = FieldInfo("incline","", false, OnMissingOmit, vIntUnsigned);
	fields[DoorItem::size] = FieldInfo("size","", false, OnMissingOmit, vIntUnsigned);
}
	
void DoorExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len) {
	if(emu_op != my_op)
		return;
	uint32 count = len / sizeof(Door_Struct);
	if(count*sizeof(Door_Struct) != len) {
		printf("Warning: door packet is length %d which is not a multiple of the door size %d\n", len, sizeof(Door_Struct));
	}
	SplitPacket(count, data, len);
}

uint32 DoorExtractor::DoorItem::FromPacket(unsigned char *packet, uint32 len) {
	if(len < sizeof(Door_Struct)) {
		printf("Packet of length %d is too short to be a door packet (len %d)\n", len, sizeof(Door_Struct));
		return(0);
	}
	Door_Struct *i = (Door_Struct *) packet;
	
	data[doorid] = ultoa(i->doorId + 1);	//door Id must always be > 0
	data[zone] = zone_info->GetShortName();
	data[name] = i->name;
	data[pos_x] = ftoa(i->xPos);
	data[pos_y] = ftoa(i->yPos);
	data[pos_z] = ftoa(i->zPos);
	data[heading] = ftoa(i->heading);
	data[doorisopen] = itoa(i->state_at_spawn);
	data[door_param] = itoa(i->door_param);
	data[incline] = itoa(i->incline);
	data[size] = itoa(i->size);
	data[opentype] = itoa(i->opentype);
	
	return(sizeof(Door_Struct));
}

FuzzyDoorExtractor::FuzzyDoorExtractor(ZoneInfoExtractor *zi)
: DoorExtractor(zi) {
	//rework the primary key to use the fuzzy match
	fields[DoorItem::doorid].primary = false;
	fields[DoorItem::zone].primary = true;
	fields[DoorItem::name].primary = true;
	fields[DoorItem::pos_x].primary = true;
	fields[DoorItem::pos_y].primary = true;
	fields[DoorItem::pos_z].primary = true;
	
	//raise the epsilon to match better
	float_epsilon = 0.1;
}

void FuzzyDoorExtractor::GenerateClauses(string &field_names, string &where_clause, ExtractItem *item) {
	//we want to try to fuzzy match the door based on name and location
	map<uint16, FieldInfo>::iterator cur, end;
	
	map<uint16, string>::iterator valres;
	
	bool first = true;
	
	//build a comma seperated list of field names we care about
	cur = fields.begin();
	end = fields.end();
	for(; cur != end; cur++) {
		if(first)
			first = false;
		else {
			field_names += ",";
		}
		field_names += cur->second.name;
	}
	
	//manually build our fuzzy where clause.
	string value;
	value = item->data[DoorItem::name];
	EscapeString(value, value.c_str(), value.length());
	where_clause = "lower(name)=lower('"+value+"') ";
	value = item->data[DoorItem::zone];
	EscapeString(value, value.c_str(), value.length());
	where_clause += " AND lower(zone)=lower('"+value+"') ";
	float x = atof(item->data[DoorItem::pos_x].c_str());
	float y = atof(item->data[DoorItem::pos_y].c_str());
	float z = atof(item->data[DoorItem::pos_z].c_str());
	string xs = ftoa(x);
	string ys = ftoa(y);
	string zs = ftoa(z);
	where_clause += " AND abs(pos_x-("+xs+"))<0.1";
	where_clause += " AND abs(pos_y-("+ys+"))<0.1";
	where_clause += " AND abs(pos_z-("+zs+"))<0.1";
}

AAExtractor::AAExtractor()
: ExtractCollector(OP_SendAATable, "altadv_vars")
{
	//fill out the field list
	fields[AAItem::aaid] = FieldInfo("skill_id","", true, OnMissingError);
	fields[AAItem::hotkey_sid] = FieldInfo("hotkey_sid","", false, OnMissingOmit);
	fields[AAItem::hotkey_sid2] = FieldInfo("hotkey_sid2","", false, OnMissingOmit);
	fields[AAItem::title_sid] = FieldInfo("title_sid","", false, OnMissingOmit);
	fields[AAItem::desc_sid] = FieldInfo("desc_sid","", false, OnMissingOmit);
	fields[AAItem::cost] = FieldInfo("cost","", false, OnMissingOmit);
	fields[AAItem::prereq_skill] = FieldInfo("prereq_skill","", false, OnMissingOmit);
	fields[AAItem::prereq_minpoints] = FieldInfo("prereq_minpoints","", false, OnMissingOmit);
	fields[AAItem::type] = FieldInfo("type","", false, OnMissingOmit);
	fields[AAItem::spellid] = FieldInfo("spellid","", false, OnMissingOmit);
	fields[AAItem::spell_type] = FieldInfo("spell_type","", false, OnMissingOmit);
	fields[AAItem::spell_refresh] = FieldInfo("spell_refresh","", false, OnMissingOmit);
	fields[AAItem::classes] = FieldInfo("classes","", false, OnMissingOmit);
	fields[AAItem::berserker] = FieldInfo("berserker","", false, OnMissingOmit);
	fields[AAItem::max_level] = FieldInfo("max_level","", false, OnMissingOmit);
	fields[AAItem::name] = FieldInfo("name","", false, OnMissingUseDefault);
}

void AAExtractor::GenerateAnInsert(FILE *into, bool make_replaces, ExtractItem *item) {
	ExtractCollector::GenerateAnInsert(into, make_replaces, item);
	//assume item is really an AAItem
	AAItem *aai = (AAItem *) item;
	aai->abilities.GenerateInserts(into, make_replaces);
}

void AAExtractor::GenerateAnUpdate(FILE *into, ExtractorDB *db, ExtractItem *item) {
	ExtractCollector::GenerateAnUpdate(into, db, item);
	//assume item is really an AAItem
	AAItem *aai = (AAItem *) item;
	aai->abilities.GenerateUpdates(into, db);
}

void AAExtractor::GenerateAText(FILE *into, ExtractorDB *db, ExtractItem *item) {
	ExtractCollector::GenerateAText(into, db, item);
	//assume item is really an AAItem
	AAItem *aai = (AAItem *) item;
	aai->abilities.GenerateTexts(into, db);
}

uint32 AAExtractor::AAItem::FromPacket(unsigned char *packet, uint32 len) {
	if(len < sizeof(SendAA_Struct)) {
		printf("Packet of length %d is too short to be an AA packet (len %d)\n", len, sizeof(SendAA_Struct));
		return(0);
	}
	SendAA_Struct *i = (SendAA_Struct *) packet;
	
	data[aaid] = itoa(i->id);
	data[hotkey_sid] = ultoa(i->hotkey_sid);
	data[hotkey_sid2] = ultoa(i->hotkey_sid2);
	data[title_sid] = ultoa(i->title_sid);
	data[desc_sid] = ultoa(i->desc_sid);
	data[prereq_skill] = ultoa(i->prereq_skill);
	data[prereq_minpoints] = ultoa(i->prereq_minpoints);
	data[type] = ultoa(i->type);
	data[spellid] = ultoa(i->spellid);
	data[spell_type] = ultoa(i->spell_type);
	data[spell_refresh] = ultoa(i->spell_refresh);
	data[classes] = ultoa(i->classes);
	data[berserker] = ultoa(i->berserker);
	data[max_level] = ultoa(i->max_level);
	
	//Pull out the dynamic length set of abilities using another extractor
	abilities.SetAAID(i->id);
	abilities.SplitPacket(i->total_abilities, (unsigned char *) i->abilities, len - sizeof(SendAA_Struct));
	
	return(len);	//assume we ate the whole thing
}


AAExtractor::AAAbilityExtractor::AAAbilityExtractor()
: ExtractCollector(OP_SendAATable, "aa_levels")
{
	aa_id = 0;
	
	//fill out the field list
	fields[AAAbilityItem::aa_id] = FieldInfo("aa_id","", true, OnMissingError);
	fields[AAAbilityItem::ability] = FieldInfo("ability","", true, OnMissingError);
	fields[AAAbilityItem::increase_amt] = FieldInfo("increase_amt","", false, OnMissingOmit);
	fields[AAAbilityItem::last_level] = FieldInfo("last_level","", false, OnMissingOmit);
	fields[AAAbilityItem::unknown08] = FieldInfo("unknown08","", false, OnMissingOmit);
	
}

ExtractCollector::ExtractItem *AAExtractor::AAAbilityExtractor::NewItem() {
	AAAbilityItem *i = new AAAbilityItem();
	i->data[AAAbilityItem::aa_id] = ultoa(aa_id);
	return(i);
}

uint32 AAExtractor::AAAbilityExtractor::AAAbilityItem::FromPacket(unsigned char *packet, uint32 len) {
	if(len < sizeof(AA_Ability)) {
		printf("Packet of length %d is too short to be an aa ability packet (len %d)\n", len, sizeof(AA_Ability));
		return(0);
	}
	AA_Ability *i = (AA_Ability *) packet;
	
	data[ability] = ultoa(i->skill_id);
	data[increase_amt] = ultoa(i->increase_amt);
	data[last_level] = ultoa(i->last_level);
	data[unknown08] = ultoa(i->unknown08);
	
	return(sizeof(AA_Ability));
}


ObjectExtractor::ObjectExtractor()
: ExtractCollector(OP_GroundSpawn, "object")
{
	//fill out the field list
//	fields[ObjectItem::drop_id] = FieldInfo("id","", true, OnMissingError);
	fields[ObjectItem::zone_id] = FieldInfo("zoneid","", true, OnMissingOmit, vInt);
	fields[ObjectItem::heading] = FieldInfo("heading","", false, OnMissingOmit, vFloat);
	fields[ObjectItem::x] = FieldInfo("xpos","", true, OnMissingOmit, vFloat);
	fields[ObjectItem::y] = FieldInfo("ypos","", true, OnMissingOmit, vFloat);
	fields[ObjectItem::z] = FieldInfo("zpos","", true, OnMissingOmit, vFloat);
	fields[ObjectItem::object_name] = FieldInfo("objectname","", false, OnMissingOmit);
	fields[ObjectItem::object_type] = FieldInfo("type","", false, OnMissingOmit);
	fields[ObjectItem::unknown08] = FieldInfo("unknown08","", false, OnMissingOmit);
	fields[ObjectItem::unknown10] = FieldInfo("unknown10","", false, OnMissingOmit);
	fields[ObjectItem::unknown20] = FieldInfo("unknown20","", false, OnMissingOmit);
	fields[ObjectItem::unknown24] = FieldInfo("unknown24","", false, OnMissingOmit);
	fields[ObjectItem::unknown60] = FieldInfo("unknown60","", false, OnMissingOmit);
	fields[ObjectItem::unknown64] = FieldInfo("unknown64","", false, OnMissingOmit);
	fields[ObjectItem::unknown68] = FieldInfo("unknown68","", false, OnMissingOmit);
	fields[ObjectItem::unknown72] = FieldInfo("unknown72","", false, OnMissingOmit);
	fields[ObjectItem::unknown76] = FieldInfo("unknown76","", false, OnMissingOmit);
	fields[ObjectItem::unknown84] = FieldInfo("unknown84","", false, OnMissingOmit);
	
	//fields which we are ignoring in the DB (cause they dont pertain to packets):
	//itemid, charges, icon, linked_list_addr_01, linked_list_addr_02, unknown88
}

uint32 ObjectExtractor::ObjectItem::FromPacket(unsigned char *packet, uint32 len) {
	if(len < sizeof(Object_Struct)) {
		printf("Packet of length %d is too short to be an object packet (len %d)\n", len, sizeof(Object_Struct));
		return(0);
	}
	Object_Struct *i = (Object_Struct *) packet;
	
//	data[drop_id] = itoa(i->drop_id);
	data[zone_id] = itoa(i->zone_id);
	data[heading] = ftoa(i->heading);
	data[x] = ftoa(i->x);
	data[y] = ftoa(i->y);
	data[z] = ftoa(i->z);
	data[object_name] = i->object_name;
	data[object_type] = ultoa(i->object_type);
	data[unknown08] = ultoa(i->unknown008[0]);
	data[unknown10] = ultoa(i->unknown008[1]);
	data[unknown20] = ultoa(i->unknown020[0]);
	data[unknown24] = ultoa(i->unknown020[1]);
	data[unknown60] = ultoa(i->unknown060[0]);
	data[unknown64] = ultoa(i->unknown060[1]);
	data[unknown68] = ultoa(i->unknown060[2]);
	data[unknown72] = ultoa(i->unknown060[3]);
	data[unknown76] = ultoa(i->unknown060[4]);
	data[unknown84] = ultoa(i->unknown084[0]);
//	data[unknown88] = ultoa(i->spawn_id);	//spawn id, dosent go in the db
	
	return(sizeof(Object_Struct));
}

FuzzyObjectExtractor::FuzzyObjectExtractor()
: ObjectExtractor()
{

}

void FuzzyObjectExtractor::GenerateClauses(string &field_names, string &where_clause, ExtractItem *item) {
	//we want to try to fuzzy match the door based on name and location
	map<uint16, FieldInfo>::iterator cur, end;
	
	map<uint16, string>::iterator valres;
	
	bool first = true;
	
	//build a comma seperated list of field names we care about
	cur = fields.begin();
	end = fields.end();
	for(; cur != end; cur++) {
		if(first)
			first = false;
		else {
			field_names += ",";
		}
		field_names += cur->second.name;
	}
	
	//manually build our fuzzy where clause.
	string value;
	value = item->data[ObjectItem::object_name];
	EscapeString(value, value.c_str(), value.length());
	where_clause = "lower(objectname)=lower('"+value+"') ";
	value = item->data[ObjectItem::zone_id];
	where_clause += " AND zoneid="+itoa(atoi(value.c_str()));
	float x = atof(item->data[ObjectItem::x].c_str());
	float y = atof(item->data[ObjectItem::y].c_str());
	float z = atof(item->data[ObjectItem::z].c_str());
	string xs = ftoa(x);
	string ys = ftoa(y);
	string zs = ftoa(z);
	where_clause += " AND abs(xpos-("+xs+"))<0.1";
	where_clause += " AND abs(ypos-("+ys+"))<0.1";
	where_clause += " AND abs(zpos-("+zs+"))<0.1";
}


ZoneHeaderExtractor::ZoneHeaderExtractor()
: ExtractCollector(OP_NewZone, "zone")
{
//TODO: we need the zone ID number in here!!
	//fill out the field list
	fields[ZoneHeaderItem::zone_short_name] = FieldInfo("short_name","", true, OnMissingError);
	fields[ZoneHeaderItem::zone_long_name] = FieldInfo("long_name","", false, OnMissingOmit);
	fields[ZoneHeaderItem::ztype] = FieldInfo("ztype","", false, OnMissingOmit);
	fields[ZoneHeaderItem::fog_red1] = FieldInfo("fog_red1","", false, OnMissingOmit, vInt);
	fields[ZoneHeaderItem::fog_green1] = FieldInfo("fog_green1","", false, OnMissingOmit, vInt);
	fields[ZoneHeaderItem::fog_blue1] = FieldInfo("fog_blue1","", false, OnMissingOmit, vInt);
	fields[ZoneHeaderItem::fog_minclip1] = FieldInfo("fog_minclip1","", false, OnMissingOmit, vFloat);
	fields[ZoneHeaderItem::fog_maxclip1] = FieldInfo("fog_maxclip1","", false, OnMissingOmit, vFloat);
	fields[ZoneHeaderItem::fog_red2] = FieldInfo("fog_red2","", false, OnMissingOmit, vInt);
	fields[ZoneHeaderItem::fog_green2] = FieldInfo("fog_green2","", false, OnMissingOmit, vInt);
	fields[ZoneHeaderItem::fog_blue2] = FieldInfo("fog_blue2","", false, OnMissingOmit, vInt);
	fields[ZoneHeaderItem::fog_minclip2] = FieldInfo("fog_minclip2","", false, OnMissingOmit, vFloat);
	fields[ZoneHeaderItem::fog_maxclip2] = FieldInfo("fog_maxclip2","", false, OnMissingOmit, vFloat);
	fields[ZoneHeaderItem::fog_red3] = FieldInfo("fog_red3","", false, OnMissingOmit, vInt);
	fields[ZoneHeaderItem::fog_green3] = FieldInfo("fog_green3","", false, OnMissingOmit, vInt);
	fields[ZoneHeaderItem::fog_blue3] = FieldInfo("fog_blue3","", false, OnMissingOmit, vInt);
	fields[ZoneHeaderItem::fog_minclip3] = FieldInfo("fog_minclip3","", false, OnMissingOmit, vFloat);
	fields[ZoneHeaderItem::fog_maxclip3] = FieldInfo("fog_maxclip3","", false, OnMissingOmit, vFloat);
	fields[ZoneHeaderItem::fog_red4] = FieldInfo("fog_red4","", false, OnMissingOmit, vInt);
	fields[ZoneHeaderItem::fog_green4] = FieldInfo("fog_green4","", false, OnMissingOmit, vInt);
	fields[ZoneHeaderItem::fog_blue4] = FieldInfo("fog_blue4","", false, OnMissingOmit, vInt);
	fields[ZoneHeaderItem::fog_minclip4] = FieldInfo("fog_minclip4","", false, OnMissingOmit, vFloat);
	fields[ZoneHeaderItem::fog_maxclip4] = FieldInfo("fog_maxclip4","", false, OnMissingOmit, vFloat);
	fields[ZoneHeaderItem::walkspeed] = FieldInfo("walkspeed","", false, OnMissingOmit, vFloat);
	fields[ZoneHeaderItem::time_type] = FieldInfo("time_type","", false, OnMissingOmit);
	fields[ZoneHeaderItem::sky] = FieldInfo("sky","", false, OnMissingOmit);
	fields[ZoneHeaderItem::zone_exp_multiplier] = FieldInfo("zone_exp_multiplier","", false, OnMissingOmit, vFloat);
	fields[ZoneHeaderItem::safe_x] = FieldInfo("safe_x","", false, OnMissingOmit, vFloat);
	fields[ZoneHeaderItem::safe_y] = FieldInfo("safe_y","", false, OnMissingOmit, vFloat);
	fields[ZoneHeaderItem::safe_z] = FieldInfo("safe_z","", false, OnMissingOmit, vFloat);
	fields[ZoneHeaderItem::underworld] = FieldInfo("underworld","", false, OnMissingOmit, vFloat);
	fields[ZoneHeaderItem::minclip] = FieldInfo("minclip","", false, OnMissingOmit, vFloat);
	fields[ZoneHeaderItem::maxclip] = FieldInfo("maxclip","", false, OnMissingOmit, vFloat);
	fields[ZoneHeaderItem::zone_id] = FieldInfo("zoneidnumber","", false, OnMissingOmit, vInt);
	
	//fields which we are ignoring in the DB (cause they dont pertain to packets):
	//min_level, min_status, maxclients, weather, note
}

uint32 ZoneHeaderExtractor::ZoneHeaderItem::FromPacket(unsigned char *packet, uint32 len) {
	if(len < sizeof(NewZone_Struct)) {
		printf("Packet of length %d is too short to be a zone packet (len %d)\n", len, sizeof(NewZone_Struct));
		return(0);
	}
	NewZone_Struct *i = (NewZone_Struct *) packet;
	
	//convert zone short name to lower case
	char *c = i->zone_short_name;
	for(; *c != '\0'; c++)
		if(*c >= 'A' && *c <= 'Z')
			*c += 'a' - 'A';
	
	data[zone_short_name] = i->zone_short_name;
	data[zone_long_name] = i->zone_long_name;
	data[ztype] = itoa(i->ztype);
	data[zone_id] = itoa(i->zone_id);
	
	data[fog_red1] = itoa(i->fog_red[0]);
	data[fog_green1] = itoa(i->fog_green[0]);
	data[fog_blue1] = itoa(i->fog_blue[0]);
	data[fog_minclip1] = ftoa(i->fog_minclip[0]);
	data[fog_maxclip1] = ftoa(i->fog_maxclip[0]);
	data[fog_red2] = itoa(i->fog_red[1]);
	data[fog_green2] = itoa(i->fog_green[1]);
	data[fog_blue2] = itoa(i->fog_blue[1]);
	data[fog_minclip2] = ftoa(i->fog_minclip[1]);
	data[fog_maxclip2] = ftoa(i->fog_maxclip[1]);
	data[fog_red3] = itoa(i->fog_red[2]);
	data[fog_green3] = itoa(i->fog_green[2]);
	data[fog_blue3] = itoa(i->fog_blue[2]);
	data[fog_minclip3] = ftoa(i->fog_minclip[2]);
	data[fog_maxclip3] = ftoa(i->fog_maxclip[2]);
	data[fog_red4] = itoa(i->fog_red[3]);
	data[fog_green4] = itoa(i->fog_green[3]);
	data[fog_blue4] = itoa(i->fog_blue[3]);
	data[fog_minclip4] = ftoa(i->fog_minclip[3]);
	data[fog_maxclip4] = ftoa(i->fog_maxclip[3]);
	
	data[walkspeed] = ftoa(walkspeed);
	data[time_type] = itoa(i->time_type);
	data[sky] = itoa(i->sky);
	data[zone_exp_multiplier] = ftoa(i->zone_exp_multiplier);
	data[safe_x] = ftoa(i->safe_x);
	data[safe_y] = ftoa(i->safe_y);
	data[safe_z] = ftoa(i->safe_z);
	data[underworld] = ftoa(i->underworld);
	data[minclip] = ftoa(i->minclip);
	data[maxclip] = ftoa(i->maxclip);
	
	return(sizeof(NewZone_Struct));
}

ZonePointExtractor::ZonePointExtractor(ZoneInfoExtractor *zi)
: ExtractCollector(OP_SendZonepoints, "zone_points")
{
	zone_info = zi;
	
	//fill out the field list
	fields[ZonePointItem::iterator] = FieldInfo("number","", true, OnMissingError, vInt);
	fields[ZonePointItem::zone_short] = FieldInfo("zone","", true, OnMissingError, vString);
	fields[ZonePointItem::x] = FieldInfo("x","", false, OnMissingOmit, vFloat);
	fields[ZonePointItem::y] = FieldInfo("y","", false, OnMissingOmit, vFloat);
	fields[ZonePointItem::z] = FieldInfo("z","", false, OnMissingOmit, vFloat);
	fields[ZonePointItem::heading] = FieldInfo("heading","", false, OnMissingOmit, vFloat);
	fields[ZonePointItem::target_zone] = FieldInfo("target_zone_id","", false, OnMissingOmit, vInt);
	
	//fields which we are ignoring in the DB (cause they dont pertain to packets):
	//target_x, target_y, target_z, target_heading, keep_x, keep_y
}
	
void ZonePointExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len) {
	if(emu_op != my_op)
		return;
	uint32 count = *((uint32 *) data);
	len -= sizeof(uint32);
	data += sizeof(uint32);
	SplitPacket(count, data, len);
}

uint32 ZonePointExtractor::ZonePointItem::FromPacket(unsigned char *packet, uint32 len) {
	if(len < sizeof(ZonePoint_Entry)) {
		printf("Packet of length %d is too short to be a zone point packet (len %d)\n", len, sizeof(ZonePoint_Entry));
		return(0);
	}
	ZonePoint_Entry *i = (ZonePoint_Entry *) packet;
	
	data[iterator] = ultoa(i->iterator);
	data[x] = ftoa(i->x);
	data[y] = ftoa(i->y);
	data[z] = ftoa(i->z);
	data[heading] = ftoa(i->heading);
	data[target_zone] = itoa(i->zoneid);
	data[zone_short] = zone_info->GetShortName();
	
	return(sizeof(ZonePoint_Entry));
}

TributeExtractor::TributeExtractor()
: ExtractCollector(OP_TributeInfo, "tributes")
{
	//fill out the field list
	fields[TributeItem::tribute_id] = FieldInfo("id","", true, OnMissingError, vInt);
	fields[TributeItem::isguild] = FieldInfo("isguild","", true, OnMissingOmit, vInt);
	fields[TributeItem::unknown] = FieldInfo("unknown","", false, OnMissingOmit, vInt);
	fields[TributeItem::name] = FieldInfo("name","", false, OnMissingOmit, vString);
}

//we have to watch two different opcodes.
void TributeExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len) {
	if(emu_op != OP_TributeInfo && emu_op != OP_GuildTributeInfo)
		return;	//not interested
	ExtractItem *item = NewItem();
	if(emu_op == OP_GuildTributeInfo) {
		item->data[TributeItem::isguild] = "1";
		data += sizeof(uint32);	//skip unknown at head of packet... maybe guild ID?
	} else {
		item->data[TributeItem::isguild] = "0";
	}
	if(item->FromPacket(data, len) == 0) {
		safe_delete(item);
		return;
	}
	collected.push_back(item);
}

void TributeExtractor::GenerateAnInsert(FILE *into, bool make_replaces, ExtractItem *item) {
	ExtractCollector::GenerateAnInsert(into, make_replaces, item);
	//assume item is really a TributeItem
	TributeItem *ti = (TributeItem *) item;
	ti->abilities.GenerateInserts(into, make_replaces);
}

void TributeExtractor::GenerateAnUpdate(FILE *into, ExtractorDB *db, ExtractItem *item) {
	ExtractCollector::GenerateAnUpdate(into, db, item);
	//assume item is really an TributeItem
	TributeItem *aai = (TributeItem *) item;
	aai->abilities.GenerateUpdates(into, db);
}

void TributeExtractor::GenerateAText(FILE *into, ExtractorDB *db, ExtractItem *item) {
	ExtractCollector::GenerateAText(into, db, item);
	//assume item is really an TributeItem
	TributeItem *aai = (TributeItem *) item;
	aai->abilities.GenerateTexts(into, db);
}

uint32 TributeExtractor::TributeItem::FromPacket(unsigned char *packet, uint32 len) {
	if(len < sizeof(TributeAbility_Struct)) {
		printf("Packet of length %d is too short to be a tribute packet (len %d)\n", len, sizeof(SendAA_Struct));
		return(0);
	}
	TributeAbility_Struct *i = (TributeAbility_Struct *) packet;
	
	//stupid backwards byte order
	i->tribute_id = ntohl(i->tribute_id);
	i->unknown = ntohl(i->unknown);
	
	data[tribute_id] = ultoa(i->tribute_id);
	data[unknown] = ultoa(i->unknown);
	data[name] = i->name;
	
	//Pull out the dynamic length set of abilities using another extractor
	abilities.SetTributeID(i->tribute_id);
	abilities.SplitPacket(MAX_TRIBUTE_TIERS, (unsigned char *) i->tiers, MAX_TRIBUTE_TIERS*sizeof(TributeLevel_Struct));
	
	return(len);	//assume we ate the whole thing
}


TributeExtractor::TributeAbilityExtractor::TributeAbilityExtractor()
: ExtractCollector(OP_TributeInfo, "tribute_levels")
{
	tribute_id = 0;
	
	//fill out the field list
	fields[TributeAbilityItem::tribute_id] = FieldInfo("tribute_id","", true, OnMissingError, vInt);
	fields[TributeAbilityItem::level] = FieldInfo("level","", true, OnMissingError, vInt);
	fields[TributeAbilityItem::cost] = FieldInfo("cost","", false, OnMissingOmit, vInt);
	fields[TributeAbilityItem::item_id] = FieldInfo("item_id","", false, OnMissingOmit, vInt);
	
}

ExtractCollector::ExtractItem *TributeExtractor::TributeAbilityExtractor::NewItem() {
	TributeAbilityItem *i = new TributeAbilityItem();
	i->data[TributeAbilityItem::tribute_id] = ultoa(tribute_id);
	return(i);
}

uint32 TributeExtractor::TributeAbilityExtractor::TributeAbilityItem::FromPacket(unsigned char *packet, uint32 len) {
	if(len < sizeof(TributeLevel_Struct)) {
		printf("Packet of length %d is too short to be a tribute ability block (len %d)\n", len, sizeof(AA_Ability));
		return(0);
	}
	TributeLevel_Struct *i = (TributeLevel_Struct *) packet;
	
	//stupid backwards byte order
	i->level = ntohl(i->level);
	i->cost = ntohl(i->cost);
	i->tribute_item_id = ntohl(i->tribute_item_id);
	
	
	data[level] = ultoa(i->level);
	data[cost] = ultoa(i->cost);
	data[item_id] = ultoa(i->tribute_item_id);
	
	return(sizeof(TributeLevel_Struct));
}

TributeTextExtractor::TributeTextExtractor()
: ExtractCollector(OP_SelectTribute, "tributes")
{
	//fill out the field list
	fields[TributeTextItem::id] = FieldInfo("id","", true, OnMissingOmit, vInt);
	fields[TributeTextItem::description] = FieldInfo("descr","", false, OnMissingOmit, vFloat);
}

uint32 TributeTextExtractor::TributeTextItem::FromPacket(unsigned char *packet, uint32 len) {
	if(len < sizeof(SelectTributeReply_Struct)) {
		printf("Packet of length %d is too short to be a tribute desc packet (len %d)\n", len, sizeof(Object_Struct));
		return(0);
	}
	SelectTributeReply_Struct *i = (SelectTributeReply_Struct *) packet;
	
	data[id] = itoa(i->tribute_id);
	data[description] = i->desc;
	
	return(sizeof(Object_Struct));
}

BookTextExtractor::BookTextExtractor()
: ExtractCollector(OP_ReadBook, "books")
{
	//fill out the field list
	fields[BookTextItem::name] = FieldInfo("name","", true, OnMissingError, vString);
	fields[BookTextItem::text] = FieldInfo("txtfile","", false, OnMissingOmit, vString);
}

ExtractCollector::ExtractItem *BookTextExtractor::NewItem() {
	return(new BookTextItem(&last_name));
}

BookTextExtractor::BookTextItem::BookTextItem(string *ln) {
	last_name = ln;
}

uint32 BookTextExtractor::BookTextItem::FromPacket(unsigned char *packet, uint32 len) {
	if(len < sizeof(BookText_Struct)) {
		printf("Packet of length %d is too short to be a book packet (len %d)\n", len, sizeof(Object_Struct));
		return(0);
	}
	BookText_Struct *i = (BookText_Struct *) packet;
	
	if(last_name->length() == 0) {
		if(i->booktext[0] == '0' && i->booktext[1] == '\0')
			return(0);
		//this must be a request.
		if(len > 48) {
			printf("Long book request packet. We prolly missed the actual request.\n");
			return(0);
		}
		*last_name = i->booktext;
		return(0);
	} else {
		data[name] = *last_name;
		data[text] = i->booktext;
		
		*last_name = "";
	}
	
	return(len);
}




TitleExtractor::TitleExtractor()
: ExtractCollector(OP_CustomTitles, "titles")
{
	//fill out the field list
	fields[TitleItem::skill_id] = FieldInfo("skill_id","", true, OnMissingError, vInt);
	fields[TitleItem::skill_value] = FieldInfo("skill_value","", true, OnMissingError, vInt);
//we have to include this in the primary key because the other two do not form a unique key
//this just means that updates are worthless, only inserts will be generated.
	fields[TitleItem::title] = FieldInfo("title","", true, OnMissingOmit, vString);
}

void TitleExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len) {
	if(emu_op != my_op)
		return;
	
	Titles_Struct *s = (Titles_Struct *) data;
	SplitPacket(s->title_count, data+sizeof(Titles_Struct), len - sizeof(Titles_Struct));
}

uint32 TitleExtractor::TitleItem::FromPacket(unsigned char *packet, uint32 len) {
	if(len < sizeof(TitleEntry_Struct)) {
		printf("Packet of length %d is too short to be a title packet (len %d)\n", len, sizeof(TitleEntry_Struct));
		return(0);
	}
	TitleEntry_Struct *i = (TitleEntry_Struct *) packet;
	
	data[skill_id] = itoa(i->skill_id);
	data[skill_value] = itoa(i->skill_value);
	data[title] = i->title;
	
	return(sizeof(TitleEntry_Struct) + strlen(i->title) + 1);
}

/*
	The recipe extractor is kinda on hold right now because we need to make
	a way for it to look for existing recipes in the database, which is not
	supported in the current framework.
*/
RecipeExtractor::RecipeExtractor()
: ExtractCollector(OP_RecipeReply, "tradeskill_recipes")
{
	//fill out the field list
	fields[RecipeItem::recipe_id] = FieldInfo("recipe_id","", true, OnMissingError, vInt);
	fields[RecipeItem::tradeskill] = FieldInfo("tradeskill","", false, OnMissingOmit, vInt);
	fields[RecipeItem::trivial] = FieldInfo("trivial","", false, OnMissingOmit, vInt);
	fields[RecipeItem::name] = FieldInfo("name","", false, OnMissingOmit, vString);
}

/*//
void RecipeExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len) {
	if(emu_op != OP_RecipeReply && emu_op != OP_GuildRecipeInfo)
		return;	//not interested
	ExtractItem *item = NewItem();
	if(item->FromPacket(data, len) == 0) {
		safe_delete(item);
		return;
	}
	collected.push_back(item);
}*/

void RecipeExtractor::GenerateAnInsert(FILE *into, bool make_replaces, ExtractItem *item) {
	ExtractCollector::GenerateAnInsert(into, make_replaces, item);
	//assume item is really a RecipeItem
	RecipeItem *ti = (RecipeItem *) item;
	ti->items.GenerateInserts(into, make_replaces);
}

void RecipeExtractor::GenerateAnUpdate(FILE *into, ExtractorDB *db, ExtractItem *item) {
	/*ExtractCollector::GenerateAnUpdate(into, db, item);
	//assume item is really an RecipeItem
	RecipeItem *aai = (RecipeItem *) item;
	aai->abilities.GenerateUpdates(into, db);*/
	printf("ERROR: Recipe extractor dosent support updates right now.");
}

void RecipeExtractor::GenerateAText(FILE *into, ExtractorDB *db, ExtractItem *item) {
	/*ExtractCollector::GenerateAText(into, db, item);
	//assume item is really an RecipeItem
	RecipeItem *aai = (RecipeItem *) item;
	aai->abilities.GenerateTexts(into, db);
	*/
	printf("ERROR: Recipe extractor dosent support text mode right now.");
}

uint32 RecipeExtractor::RecipeItem::FromPacket(unsigned char *packet, uint32 len) {
	/*if(len < sizeof(RecipeItem_Struct)) {
		printf("Packet of length %d is too short to be a Recipe packet (len %d)\n", len, sizeof(RecipeItem_Struct));
		return(0);
	}
	RecipeItem_Struct *i = (RecipeItem_Struct *) packet;
	
	//stupid backwards byte order
	i->Recipe_id = ntohl(i->Recipe_id);
	i->unknown = ntohl(i->unknown);
	
	data[Recipe_id] = ultoa(i->Recipe_id);
	data[unknown] = ultoa(i->unknown);
	data[name] = i->name;
	
	//Pull out the dynamic length set of abilities using another extractor
	items.SplitPacket(MAX_Recipe_TIERS, (unsigned char *) i->tiers, MAX_Recipe_TIERS*sizeof(RecipeLevel_Struct));
	*/
	
	//TODO: place a success item and a container into items.
	
	return(len);	//assume we ate the whole thing
}


RecipeExtractor::RecipeItemExtractor::RecipeItemExtractor()
: ExtractCollector(OP_RecipeReply, "tradeskill_recipe_entries")
{
	//fill out the field list
	fields[RecipeItemItem::recipe_id] = FieldInfo("recipe_id","", true, OnMissingError, vInt);
	fields[RecipeItemItem::item_id] = FieldInfo("item_id","", true, OnMissingError, vInt);
	fields[RecipeItemItem::successcount] = FieldInfo("successcount","", false, OnMissingOmit, vInt);
	fields[RecipeItemItem::componentcount] = FieldInfo("componentcount","", false, OnMissingOmit, vInt);
	fields[RecipeItemItem::iscontainer] = FieldInfo("iscontainer","", false, OnMissingOmit, vInt);
	
}

ExtractCollector::ExtractItem *RecipeExtractor::RecipeItemExtractor::NewItem() {
	RecipeItemItem *i = new RecipeItemItem();
//	i->data[RecipeItemItem::recipe_id] = ultoa(recipe_id);
	return(i);
}

uint32 RecipeExtractor::RecipeItemExtractor::RecipeItemItem::FromPacket(unsigned char *packet, uint32 len) {
	/*
	if(len < sizeof(RecipeLevel_Struct)) {
		printf("Packet of length %d is too short to be a Recipe Item block (len %d)\n", len, sizeof(AA_Item));
		return(0);
	}
	RecipeLevel_Struct *i = (RecipeLevel_Struct *) packet;
	
	//stupid backwards byte order
	i->level = ntohl(i->level);
	i->cost = ntohl(i->cost);
	i->Recipe_item_id = ntohl(i->Recipe_item_id);
	
	
	data[level] = ultoa(i->level);
	data[cost] = ultoa(i->cost);
	data[item_id] = ultoa(i->Recipe_item_id);
	*/
	//return(sizeof(RecipeLevel_Struct));
	return(len);
}

/*
TaskExtractor::TaskExtractor()
: ExtractCollector(OP_TaskInfo, "tasks")
{
	//fill out the field list
	fields[TaskItem::Task_id] = FieldInfo("id","", true, OnMissingError, vInt);
	fields[TaskItem::isguild] = FieldInfo("isguild","", true, OnMissingOmit, vInt);
	fields[TaskItem::unknown] = FieldInfo("unknown","", false, OnMissingOmit, vInt);
	fields[TaskItem::name] = FieldInfo("name","", false, OnMissingOmit, vString);
}

//we have to watch two different opcodes.
void TaskExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len) {
	if(emu_op != OP_TaskInfo && emu_op != OP_GuildTaskInfo)
		return;	//not interested
	ExtractItem *item = NewItem();
	if(emu_op == OP_GuildTaskInfo) {
		item->data[TaskItem::isguild] = "1";
		data += sizeof(uint32);	//skip unknown at head of packet... maybe guild ID?
	} else {
		item->data[TaskItem::isguild] = "0";
	}
	if(item->FromPacket(data, len) == 0) {
		safe_delete(item);
		return;
	}
	collected.push_back(item);
}

void TaskExtractor::GenerateAnInsert(FILE *into, bool make_replaces, ExtractItem *item) {
	ExtractCollector::GenerateAnInsert(into, make_replaces, item);
	//assume item is really a TaskItem
	TaskItem *ti = (TaskItem *) item;
	ti->abilities.GenerateInserts(into, make_replaces);
}

void TaskExtractor::GenerateAnUpdate(FILE *into, ExtractorDB *db, ExtractItem *item) {
	ExtractCollector::GenerateAnUpdate(into, db, item);
	//assume item is really an TaskItem
	TaskItem *aai = (TaskItem *) item;
	aai->abilities.GenerateUpdates(into, db);
}

void TaskExtractor::GenerateAText(FILE *into, ExtractorDB *db, ExtractItem *item) {
	ExtractCollector::GenerateAText(into, db, item);
	//assume item is really an TaskItem
	TaskItem *aai = (TaskItem *) item;
	aai->abilities.GenerateTexts(into, db);
}

uint32 TaskExtractor::TaskItem::FromPacket(unsigned char *packet, uint32 len) {
	if(len < sizeof(TaskActivity_Struct)) {
		printf("Packet of length %d is too short to be a Task packet (len %d)\n", len, sizeof(SendAA_Struct));
		return(0);
	}
	TaskActivity_Struct *i = (TaskActivity_Struct *) packet;
	
	//stupid backwards byte order
	i->Task_id = ntohl(i->Task_id);
	i->unknown = ntohl(i->unknown);
	
	data[Task_id] = ultoa(i->Task_id);
	data[unknown] = ultoa(i->unknown);
	data[name] = i->name;
	
	//Pull out the dynamic length set of abilities using another extractor
	abilities.SetTaskID(i->Task_id);
	abilities.SplitPacket(MAX_Task_TIERS, (unsigned char *) i->tiers, MAX_Task_TIERS*sizeof(TaskLevel_Struct));
	
	return(len);	//assume we ate the whole thing
}


TaskExtractor::TaskActivityExtractor::TaskActivityExtractor()
: ExtractCollector(OP_TaskInfo, "Task_levels")
{
	Task_id = 0;
	
	//fill out the field list
	fields[TaskActivityItem::Task_id] = FieldInfo("Task_id","", true, OnMissingError, vInt);
	fields[TaskActivityItem::level] = FieldInfo("level","", true, OnMissingError, vInt);
	fields[TaskActivityItem::cost] = FieldInfo("cost","", false, OnMissingOmit, vInt);
	fields[TaskActivityItem::item_id] = FieldInfo("item_id","", false, OnMissingOmit, vInt);
	
}

ExtractCollector::ExtractItem *TaskExtractor::TaskActivityExtractor::NewItem() {
	TaskActivityItem *i = new TaskActivityItem();
	i->data[TaskActivityItem::Task_id] = ultoa(Task_id);
	return(i);
}

uint32 TaskExtractor::TaskActivityExtractor::TaskActivityItem::FromPacket(unsigned char *packet, uint32 len) {
	if(len < sizeof(TaskLevel_Struct)) {
		printf("Packet of length %d is too short to be a Task Activity block (len %d)\n", len, sizeof(AA_Activity));
		return(0);
	}
	TaskLevel_Struct *i = (TaskLevel_Struct *) packet;
	
	//stupid backwards byte order
	i->level = ntohl(i->level);
	i->cost = ntohl(i->cost);
	i->Task_item_id = ntohl(i->Task_item_id);
	
	
	data[level] = ultoa(i->level);
	data[cost] = ultoa(i->cost);
	data[item_id] = ultoa(i->Task_item_id);
	
	return(sizeof(TaskLevel_Struct));
}
*/


TaskHistoryExtractor::TaskHistoryExtractor()
: ExtractCollector(OP_CompletedTasks, "tasks")
{
	//fill out the field list
	fields[TaskHistoryItem::task_id] = FieldInfo("task_id","", true, OnMissingError, vInt);
	fields[TaskHistoryItem::task_name] = FieldInfo("task_name","", false, OnMissingOmit, vString);
}

void TaskHistoryExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len) {
	if(emu_op != my_op)
		return;
	
	TaskHistory_Struct *s = (TaskHistory_Struct *) data;
	SplitPacket(s->completed_count, data+sizeof(TaskHistory_Struct), len - sizeof(TaskHistory_Struct));
}

uint32 TaskHistoryExtractor::TaskHistoryItem::FromPacket(unsigned char *packet, uint32 len) {
	if(len < sizeof(TaskHistoryEntry_Struct)) {
		printf("Packet of length %d is too short to be a task history packet (len %d)\n", len, sizeof(TaskHistoryEntry_Struct));
		return(0);
	}
	TaskHistoryEntry_Struct *i = (TaskHistoryEntry_Struct *) packet;
	
	data[task_id] = itoa(i->task_id);
	data[task_name] = i->name;
	
	return(sizeof(TaskHistoryEntry_Struct) + strlen(i->name) + 1);
}

SpawnExtractor::SpawnExtractor(ZoneInfoExtractor *zi)
: ExtractCollector(OP_ZoneSpawns, "npc_types")
{
	zone_info = zi;
	
	//fill out the field list
	//the primary key on this is not the true primary key due to fuzzy matching
	fields[SpawnItem::name] = FieldInfo("name","", true, OnMissingError, vString);
	fields[SpawnItem::last_name] = FieldInfo("lastname","", true, OnMissingOmit, vString);
	fields[SpawnItem::level] = FieldInfo("level","", true, OnMissingOmit, vInt);
	fields[SpawnItem::race] = FieldInfo("race","", true, OnMissingOmit, vInt);
	fields[SpawnItem::class_] = FieldInfo("class","", true, OnMissingOmit, vInt);
	fields[SpawnItem::gender] = FieldInfo("gender","", true, OnMissingOmit, vInt);
	fields[SpawnItem::size] = FieldInfo("size","", false, OnMissingOmit, vFloat);
	fields[SpawnItem::bodytype] = FieldInfo("bodytype","", false, OnMissingOmit, vInt);
	fields[SpawnItem::beardcolor] = FieldInfo("luclin_beardcolor","", false, OnMissingOmit, vInt);
//	fields[SpawnItem::beard] = FieldInfo("luclin_beard","", false, OnMissingOmit, vInt);
	fields[SpawnItem::eyecolor1] = FieldInfo("luclin_eyecolor","", false, OnMissingOmit, vInt);
//	fields[SpawnItem::eyecolor2] = FieldInfo("luclin_eyecolor2","", false, OnMissingOmit, vInt);
	fields[SpawnItem::face] = FieldInfo("face","", false, OnMissingOmit, vInt);
	fields[SpawnItem::hairstyle] = FieldInfo("luclin_hairstyle","", false, OnMissingOmit, vInt);
	fields[SpawnItem::haircolor] = FieldInfo("luclin_haircolor","", false, OnMissingOmit, vInt);
	fields[SpawnItem::findable] = FieldInfo("findable","", false, OnMissingOmit, vInt);
	fields[SpawnItem::equip_chest2] = FieldInfo("texture","", false, OnMissingOmit, vInt);
	fields[SpawnItem::helm] = FieldInfo("helmtexture","", false, OnMissingOmit, vInt);
	fields[SpawnItem::runspeed] = FieldInfo("runspeed","", false, OnMissingOmit, vFloat);
	fields[SpawnItem::walkspeed] = FieldInfo("walkspeed","", false, OnMissingOmit, vFloat);
	fields[SpawnItem::id] = FieldInfo("id","", false, OnMissingOmit, vInt);
}
	
void SpawnExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len) {
	if(emu_op == OP_ZoneSpawns) {
		//bulk spawn packet
		uint32 count = len / sizeof(Spawn_Struct);
		if(count*sizeof(Spawn_Struct) != len) {
			printf("Warning: Spawn packet is length %d which is not a multiple of the Spawn size %d\n", len, sizeof(Spawn_Struct));
		}
		SplitPacket(count, data, len);
	} else if(emu_op == OP_NewSpawn) {
		//a single spawn packet.
		ExtractItem *item = NewItem();
		if(item->FromPacket(data, len) == 0) {
			safe_delete(item);
			return;
		}
		collected.push_back(item);
	} else if(emu_op == OP_ClientUpdate) {
		PlayerPositionUpdateServer_Struct *spu = (PlayerPositionUpdateServer_Struct *) data;
		map<uint16, SpawnItem *>::iterator sii = spawns.find(spu->spawn_id);
		if(sii == spawns.end())
			return;	//spawn not found.
		SpawnItem *si = *sii;
		
		PathPoint p;
		p.x = EQ19toFloat(spu->x);
		p.y = EQ19toFloat(spu->y);
		p.z = EQ19toFloat(spu->z);
		p.h = EQ19toFloat(spu->heading);
		p.dx = EQ13toFloat(spu->delta_x);
		p.dy = EQ13toFloat(spu->delta_y);
		p.dz = EQ13toFloat(spu->delta_z);
		p.dh = EQ13toFloat(spu->delta_heading);
		si->positions.push_back(p);
	}
}

void SpawnExtractor::GenerateAnInsert(FILE *into, bool make_replaces, ExtractItem *item) {
	//get and NPC ID for this guy
	item->spawn_id = GetNextNPCID();
	item->data[SpawnItem::id] = ultoa(item->spawn_id);
	ExtractCollector::GenerateAnInsert(into, make_replaces, item);
	
	item->GenerateSpawnInserts();
}

void SpawnExtractor::SpawnItem::GenerateSpawnInserts() {
	//first we want to find a spawn position.
	vector<PathPoint>::iterator cur, end;
	cur = positions.begin();
	end = positions.end();
	for(; cur != end; cur++) {
		PathPoint &p = *cur;
		//I dont imagine a mob will every be purely moving in z and we dont
		//care about movement in h
		if(p.dx != 0 || p.dy != 0) {
			continue;	//initially moving, try to find a point where they are still.
		}
		break;
	}
	if(cur == end)	//no still point found, use initial position
		cur = positions.begin();
	
	PathPoint &spawn_point = *cur;

	//spit up the spawn2, spawn group, spawn entry
}

uint32 SpawnExtractor::SpawnItem::FromPacket(unsigned char *packet, uint32 len) {
	if(len < sizeof(Spawn_Struct)) {
		printf("Packet of length %d is too short to be a Spawn packet (len %d)\n", len, sizeof(Spawn_Struct));
		return(0);
	}
	Spawn_Struct *i = (Spawn_Struct *) packet;
	
	
	if(i->npc != 1 || i->pet_owner_id != 0 || i->name[0] == '\0') {
		//printf("# %s - %s is of type %d with owner %d\n", i->name, i->last_name, i->npc, i->pet_owner_id);
		//consume the struct even though we dont want it
		valid = false;
		return(sizeof(Spawn_Struct));
	}
	
	if(strstr(i->name, "`s_Mount")) {
		//consume the struct even though we dont want it
		valid = false;
		return(sizeof(Spawn_Struct));
	}
	
	//truncate mob names when they become numeric
	int r;
	for(r = 0; r < 64; r++) {
		if(i->name[r] == '\0')
			break;
		if(i->name[r] >= '0' && i->name[r] <= '9') {
			i->name[r] = '\0';
			break;
		}
	}
	
	data[name] = i->name;
	data[last_name] = i->last_name;
	data[level] = itoa(i->level);
	data[race] = itoa(i->race);
	data[class_] = itoa(i->class_);
	data[gender] = itoa(i->gender);
	data[bodytype] = itoa(i->bodytype);
	data[beardcolor] = itoa(i->beardcolor);
//	data[beard] = itoa(i->beard);
	data[eyecolor1] = itoa(i->eyecolor1);
//	data[eyecolor2] = itoa(i->eyecolor2);
	data[face] = itoa(i->face);
	data[hairstyle] = itoa(i->hairstyle);
	data[haircolor] = itoa(i->haircolor);
	data[size] = ftoa(i->size);
	data[findable] = itoa(i->findable);
	data[equip_chest2] = itoa(i->equip_chest2);
	data[helm] = itoa(i->helm);
	data[runspeed] = ftoa(i->runspeed);
	data[walkspeed] = ftoa(i->walkspeed);
	
	//record initial position
	PathPoint p;
	p.x = EQ19toFloat(i->x);
	p.y = EQ19toFloat(i->y);
	p.z = EQ19toFloat(i->z);
	p.h = EQ19toFloat(i->heading);
	p.dx = EQ13toFloat(i->delta_x);
	p.dy = EQ13toFloat(i->delta_y);
	p.dz = EQ13toFloat(i->delta_z);
	p.dh = EQ13toFloat(i->delta_heading);
	positions.push_back(p);
	
	parent->RegisterSpawnID(i->spawn_id, this);
	
	return(sizeof(Spawn_Struct));
}











