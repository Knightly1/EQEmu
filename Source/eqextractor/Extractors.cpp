/*

EQ Extractor, by Father Nitwit 2005

*/
//#include "Extractors.h"	//do not include this

//DO NOT INCLUDE ANY HEADERS HERE, put them in versions.h

using namespace std;
using namespace EQExtractor;

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

ZoneInfoExtractor::ZoneInfoExtractor(const char *filename)
: ExtractCollector(OP_GroundSpawn, "FAKE") {
	if(filename != NULL) {
		file_name = filename;
		catalog_enabled = true;
	} else {
		catalog_enabled = false;
	}
	zone_id = 0xFFFF;
}

void ZoneInfoExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
#ifdef NO_ZONE_ID_IN_NEWZONE
	if(emu_op == OP_GroundSpawn && len == sizeof(Object_Struct)) {
		Object_Struct *i = (Object_Struct *) data;
		zone_id = i->zone_id & 0xFFFF;
		fprintf(stderr, "# Found a ground spawn containing the zone ID (%d), ignore the previous warning.\n", zone_id);
		return;
	}
#endif
	if(emu_op == OP_NewZone) {
		if(len != sizeof(NewZone_Struct)) {
			printf("Size of newzone struct (%d) is invalid! (wanted %d) Cannot get zone info.\n", len, sizeof(NewZone_Struct));
			return;
		}
		NewZone_Struct *i = (NewZone_Struct *) data;
		short_name = i->zone_short_name;
		long_name = i->zone_long_name;
#ifndef NO_ZONE_ID_IN_NEWZONE
		zone_id = i->zone_id;
		fprintf(stderr, "# Found zone info: %s (%s=%s) (id# %d, instance %d)\n", long_name.c_str(), short_name.c_str(), i->zone_short_name2, zone_id, i->zone_instance);
#else
		zone_id = 9999;
		fprintf(stderr, "# Found zone info: %s (%s=%s)\n", long_name.c_str(), short_name.c_str(), i->zone_short_name2);
		fprintf(stderr, "# WARNING: Using old collect which is missing zone id, you need to replace 9999 with the zone id for '%s'\n", short_name.c_str());
#endif
		if(catalog_enabled)
			printf("Log %s contains zone '%s'\n", file_name.c_str(), i->zone_short_name);
	}
}

SpawnListExtractor::SpawnListExtractor()
: ExtractCollector(OP_SpawnAppearance, "FAKE") {
}

void SpawnListExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
	if(emu_op == OP_SpawnAppearance && len == sizeof(SpawnAppearance_Struct)) {
		SpawnAppearance_Struct* sa = (SpawnAppearance_Struct*) data;
		if(sa->type != AT_SpawnID)
			return;
		printf("# Spawn Appearance says my spawn ID is %d (net %02x %02x)\n",
			sa->parameter, sa->parameter&0xFF, (sa->parameter&0xFF00)>>8);
		return;
	}
	
	if(emu_op != OP_ZoneSpawns && emu_op != OP_NewSpawn && emu_op != OP_ZoneEntry)
		return;
	
	if(len < sizeof(Spawn_Struct)) {
		if(len != sizeof(ServerZoneEntry_Struct))
			printf("Size of spawn struct (%d) is invalid (want %d)! Cannot explore this spawn list.\n", len, sizeof(Spawn_Struct));
		return;
	}
	
	uint32 used = 0;
	while(used < len) {
		Spawn_Struct *i = (Spawn_Struct *) (data + used);
		
		printf("%04d(%02x %02x): %s %s (class %d, race %d, level %d) (%.2f,%.2f,%.2f)\n",
			i->spawnId, i->spawnId&0xFF, (i->spawnId&0xFF00)>>8,
			i->name, i->lastName, 
			i->class_, i->race, i->level,
			EQ19toFloat(i->x), EQ19toFloat(i->y), EQ19toFloat(i->z)
			);
		
		used += sizeof(Spawn_Struct);
	}
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
	fields[DoorItem::dest_zone] = FieldInfo("dest_zone","", false, OnMissingOmit, vIntUnsigned);
	fields[DoorItem::dest_x] = FieldInfo("dest_x","", false, OnMissingOmit, vFloat);
	fields[DoorItem::dest_y] = FieldInfo("dest_y","", false, OnMissingOmit, vFloat);
	fields[DoorItem::dest_z] = FieldInfo("dest_z","", false, OnMissingOmit, vFloat);
	fields[DoorItem::dest_heading] = FieldInfo("dest_heading","", false, OnMissingOmit, vFloat);
}
	
void DoorExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
	if(emu_op == OP_SendZonepoints) {
		//we have to hack out zone points to try to get in-zone warp destinations
		uint32 count = *((uint32 *) data);
		len -= sizeof(uint32);
		data += sizeof(uint32);
		
		ZonePoint_Entry *i = (ZonePoint_Entry *) data;
		zone_point p;
		while(count > 0) {
			p.x = i->x;
			p.y = i->y;
			p.z = i->z;
			p.h = i->heading;
			p.dest_zone = i->zoneid;
			
			m_zonePoints[i->iterator] = p;
			
			i++;
			count--;
		}
		
		return;
	}
	
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
	string zs = zone_info->GetShortName();
	transform(zs.begin(), zs.end(), zs.begin(), tolower);
	data[zone] = zs;
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

void DoorExtractor::GenerateAnInsert(FILE *into, bool make_replaces, bool was_update, ExtractItem *item) {
	DoorItem *door_item = (DoorItem *) item;
	door_item->LookupDest(m_zonePoints);
	ExtractCollector::GenerateAnInsert(into, make_replaces, was_update, item);
}

void DoorExtractor::GenerateAnUpdate(FILE *into, ExtractorDB *db, ExtractItem *item) {
	DoorItem *door_item = (DoorItem *) item;
	door_item->LookupDest(m_zonePoints);
	ExtractCollector::GenerateAnUpdate(into, db, item);
}

void DoorExtractor::GenerateAText(FILE *into, ExtractorDB *db, ExtractItem *item) {
	DoorItem *door_item = (DoorItem *) item;
	door_item->LookupDest(m_zonePoints);
	ExtractCollector::GenerateAText(into, db, item);
}

void DoorExtractor::DoorItem::LookupDest(map<uint32, zone_point> &zonePoints) {
	map<uint16, string>::iterator tgt;
	//see if we have a door_param
	tgt = data.find(door_param);
	if(tgt == data.end())
		return;
	
	uint32 param = atoi(tgt->second.c_str());
	
	//see if we have a zone_point for this value of door_param
	map<uint32, zone_point>::iterator res = zonePoints.find(param);
	if(res == zonePoints.end())
		return;
	
	const zone_point &zp = res->second;
	
	//we have one
	data[dest_zone] = itoa(zp.dest_zone);
	data[dest_x] = ftoa(zp.x);
	data[dest_y] = ftoa(zp.y);
	data[dest_z] = ftoa(zp.z);
	data[dest_heading] = ftoa(zp.h);
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
	fields[AAItem::aaid] = FieldInfo("skill_id","", true, OnMissingError, vInt);
	fields[AAItem::hotkey_sid] = FieldInfo("hotkey_sid","", false, OnMissingOmit, vIntUnsigned);
	fields[AAItem::hotkey_sid2] = FieldInfo("hotkey_sid2","", false, OnMissingOmit, vIntUnsigned);
	fields[AAItem::title_sid] = FieldInfo("title_sid","", false, OnMissingOmit, vIntUnsigned);
	fields[AAItem::desc_sid] = FieldInfo("desc_sid","", false, OnMissingOmit, vIntUnsigned);
	fields[AAItem::class_type] = FieldInfo("class_type","", false, OnMissingError, vIntUnsigned);
	fields[AAItem::cost] = FieldInfo("cost","", false, OnMissingOmit, vInt);
	fields[AAItem::prereq_skill] = FieldInfo("prereq_skill","", false, OnMissingOmit, vIntUnsigned);
	fields[AAItem::prereq_minpoints] = FieldInfo("prereq_minpoints","", false, OnMissingOmit, vIntUnsigned);
	fields[AAItem::type] = FieldInfo("type","", false, OnMissingOmit, vInt);
	fields[AAItem::spellid] = FieldInfo("spellid","", false, OnMissingOmit, vIntUnsigned);
	fields[AAItem::spell_type] = FieldInfo("spell_type","", false, OnMissingOmit, vInt);
	fields[AAItem::spell_refresh] = FieldInfo("spell_refresh","", false, OnMissingOmit, vInt);
	fields[AAItem::classes] = FieldInfo("classes","", false, OnMissingOmit, vIntUnsigned);
	fields[AAItem::berserker] = FieldInfo("berserker","", false, OnMissingOmit, vIntUnsigned);
	fields[AAItem::max_level] = FieldInfo("max_level","", false, OnMissingOmit, vInt);
	//	fields[AAItem::name] = FieldInfo("name","", false, OnMissingUseDefault, vString);
}

void AAExtractor::GenerateAnInsert(FILE *into, bool make_replaces, bool was_update, ExtractItem *item) {
	ExtractCollector::GenerateAnInsert(into, make_replaces, was_update, item);
	if(!was_update) {
		//assume item is really an AAItem
		AAItem *aai = (AAItem *) item;
		aai->abilities.GenerateInserts(into, make_replaces);
	}
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
		printf("# Packet of length %d is too short to be an AA packet (len %d)\n", len, sizeof(SendAA_Struct));
		return(0);
	}
	SendAA_Struct *i = (SendAA_Struct *) packet;
	
	data[aaid] = itoa(i->id);
	data[hotkey_sid] = ultoa(i->hotkey_sid);
	data[hotkey_sid2] = ultoa(i->hotkey_sid2);
	data[title_sid] = ultoa(i->title_sid);
	data[desc_sid] = ultoa(i->desc_sid);
	data[class_type] = ultoa(i->class_type);
	data[prereq_skill] = ultoa(i->prereq_skill);
	data[prereq_minpoints] = ultoa(i->prereq_minpoints);
	data[type] = ultoa(i->type);
	data[spellid] = ultoa(i->spellid);
	data[spell_type] = ultoa(i->spell_type);
	data[spell_refresh] = ultoa(i->spell_refresh);
	data[classes] = ultoa(i->classes);
	data[berserker] = ultoa(i->berserker);
	data[max_level] = ultoa(i->max_level);
	data[cost] = ultoa(i->cost);
	
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
	fields[AAAbilityItem::aa_id] = FieldInfo("aa_id","", true, OnMissingError, vInt);
	fields[AAAbilityItem::ability] = FieldInfo("ability","", false, OnMissingError, vInt);
	fields[AAAbilityItem::increase_amt] = FieldInfo("increase_amt","", false, OnMissingOmit, vIntUnsigned);
	fields[AAAbilityItem::last_level] = FieldInfo("level","", true, OnMissingOmit, vIntUnsigned);
	fields[AAAbilityItem::unknown08] = FieldInfo("unknown08","", false, OnMissingOmit, vIntUnsigned);
	
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
	data[unknown20] = ultoa(i->unknown020);
	data[unknown24] = ultoa(i->unknown024);
	//bullshit for now until I convert the DB fields from ints to floats
	data[unknown60] = ultoa(*((uint32 *)&i->unknown064));
	data[unknown64] = ultoa(*((uint32 *)&i->unknown068));
	data[unknown68] = ultoa(*((uint32 *)&i->unknown072));
	data[unknown72] = ultoa(i->unknown076);
//	data[unknown76] = ultoa(i->unknown060);
	data[unknown84] = ultoa(i->unknown084);
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


ZoneHeaderExtractor::ZoneHeaderExtractor(ZoneInfoExtractor *zi)
: ExtractCollector(OP_NewZone, "zone")
{
	zone_info = zi;
	
	//fill out the field list
	fields[ZoneHeaderItem::zone_short_name] = FieldInfo("short_name","", true, OnMissingError);
	fields[ZoneHeaderItem::zone_long_name] = FieldInfo("long_name","", false, OnMissingOmit);
	fields[ZoneHeaderItem::ztype] = FieldInfo("ztype","", false, OnMissingOmit);
	fields[ZoneHeaderItem::fog_red1] = FieldInfo("fog_red","", false, OnMissingOmit, vInt);
	fields[ZoneHeaderItem::fog_green1] = FieldInfo("fog_green","", false, OnMissingOmit, vInt);
	fields[ZoneHeaderItem::fog_blue1] = FieldInfo("fog_blue","", false, OnMissingOmit, vInt);
	fields[ZoneHeaderItem::fog_minclip1] = FieldInfo("fog_minclip","", false, OnMissingOmit, vFloat);
	fields[ZoneHeaderItem::fog_maxclip1] = FieldInfo("fog_maxclip","", false, OnMissingOmit, vFloat);
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
	data[zone_id] = itoa(zone_info->GetZoneID());	//not all NewZone structs had this in it
	
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

//missing in current structs	
//	data[walkspeed] = ftoa(walkspeed);
	data[time_type] = itoa(i->time_type);
	data[sky] = itoa(i->sky);
	data[zone_exp_multiplier] = ftoa(i->zone_exp_multiplier);
	data[safe_x] = ftoa(i->safe_x);
	data[safe_y] = ftoa(i->safe_y);
	data[safe_z] = ftoa(i->safe_z);
	data[underworld] = ftoa(i->underworld);
	data[minclip] = ftoa(i->minclip);
	data[maxclip] = ftoa(i->maxclip);
//not in DB right now:
	//i->gravity
	
	return(sizeof(NewZone_Struct));
}

ZonePointExtractor::ZonePointExtractor(ZoneInfoExtractor *zi)
: ExtractCollector(OP_SendZonepoints, "zone_points")
{
	zone_info = zi;
	
	//fill out the field list
	fields[ZonePointItem::iterator] = FieldInfo("number","", true, OnMissingError, vInt);
	fields[ZonePointItem::zone_short] = FieldInfo("zone","", true, OnMissingError, vString);
	fields[ZonePointItem::target_x] = FieldInfo("target_x","", false, OnMissingOmit, vFloat);
	fields[ZonePointItem::target_y] = FieldInfo("target_y","", false, OnMissingOmit, vFloat);
	fields[ZonePointItem::target_z] = FieldInfo("target_z","", false, OnMissingOmit, vFloat);
	fields[ZonePointItem::target_heading] = FieldInfo("target_heading","", false, OnMissingOmit, vFloat);
	fields[ZonePointItem::target_zone] = FieldInfo("target_zone_id","", false, OnMissingOmit, vInt);
	
	//fields which we are ignoring in the DB (cause they dont pertain to packets):
	//target_x, target_y, target_z, target_heading, keep_x, keep_y
}
	
void ZonePointExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
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
	data[target_x] = ftoa(i->x);
	data[target_y] = ftoa(i->y);
	data[target_z] = ftoa(i->z);
	data[target_heading] = ftoa(i->heading);
	data[target_zone] = itoa(i->zoneid);
	
	string zs = zone_info->GetShortName();
	transform(zs.begin(), zs.end(), zs.begin(), tolower);
	data[zone_short] = zs;
	
	return(sizeof(ZonePoint_Entry));
}

TributeExtractor::TributeExtractor()
: ExtractCollector(OP_TributeInfo, "tributes")
{
	//fill out the field list
	fields[TributeItem::tribute_id] = FieldInfo("id","", true, OnMissingError, vInt);
	fields[TributeItem::isguild] = FieldInfo("isguild","", true, OnMissingOmit, vInt);
	fields[TributeItem::tier_count] = FieldInfo("unknown","", false, OnMissingOmit, vInt);
	fields[TributeItem::name] = FieldInfo("name","", false, OnMissingOmit, vString);
}

//we have to watch two different opcodes.
void TributeExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
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

void TributeExtractor::GenerateAnInsert(FILE *into, bool make_replaces, bool was_update, ExtractItem *item) {
	ExtractCollector::GenerateAnInsert(into, make_replaces, was_update, item);
	if(!was_update) {
		//assume item is really a TributeItem
		TributeItem *ti = (TributeItem *) item;
		ti->abilities.GenerateInserts(into, make_replaces);
	}
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
	i->tier_count = ntohl(i->tier_count);
	
	data[tribute_id] = ultoa(i->tribute_id);
	data[tier_count] = ultoa(i->tier_count);
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

void TitleExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
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
	
	return(sizeof(TitleEntry_Struct) + strlen(i->title));
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
void RecipeExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
	if(emu_op != OP_RecipeReply && emu_op != OP_GuildRecipeInfo)
		return;	//not interested
	ExtractItem *item = NewItem();
	if(item->FromPacket(data, len) == 0) {
		safe_delete(item);
		return;
	}
	collected.push_back(item);
}*/

void RecipeExtractor::GenerateAnInsert(FILE *into, bool make_replaces, bool was_update, ExtractItem *item) {
	ExtractCollector::GenerateAnInsert(into, make_replaces, was_update, item);
	if(!was_update) {
		//assume item is really a RecipeItem
		RecipeItem *ti = (RecipeItem *) item;
		ti->items.GenerateInserts(into, make_replaces);
	}
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
void TaskExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
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

void TaskExtractor::GenerateAnInsert(FILE *into, bool make_replaces, bool was_update, ExtractItem *item) {
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

void TaskHistoryExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
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
	
	return(sizeof(TaskHistoryEntry_Struct) + strlen(i->name));
}

/*

	Things we can extract from spawns:
	- npc_types data.
	- Movement
	- Merchant conents.
	  - Including 'diffing' them to find normal stock.
	- Loot Items (need to split by tables, and set drop rate)
	- Min and Max damage
	- Spells Cast (need to classify type)
	- Text said (and in reply to what)
	- Handins (and rewards)

*/

SpawnExtractor::SpawnExtractor(ZoneInfoExtractor *zi)
: ExtractCollector(OP_ZoneSpawns, "npc_types")
{
	zone_info = zi;
	
	max_id = 0;
	
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
	fields[SpawnItem::beard] = FieldInfo("luclin_beard","", false, OnMissingOmit, vInt);
	fields[SpawnItem::eyecolor1] = FieldInfo("luclin_eyecolor","", false, OnMissingOmit, vInt);
	fields[SpawnItem::eyecolor2] = FieldInfo("luclin_eyecolor2","", false, OnMissingOmit, vInt);
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
	
void SpawnExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
	switch(emu_op) {
	case OP_ZoneSpawns: {
		//bulk spawn packet
		uint32 count = len / sizeof(Spawn_Struct);
		if(count*sizeof(Spawn_Struct) != len) {
			printf("Warning: Spawn packet is length %d which is not a multiple of the Spawn size %d\n", len, sizeof(Spawn_Struct));
		}
		SplitPacket(count, data, len);
	}
	break;
	case OP_NewSpawn: {
		//a single spawn packet.
		ExtractItem *item = NewItem();
		if(item->FromPacket(data, len) == 0) {
			safe_delete(item);
			return;
		}
		collected.push_back(item);
	}
	break;
	case OP_ShopRequest: {
		if(!to_server || len != sizeof(Merchant_Click_Struct))
			return;
		
//		Merchant_Click_Struct *mcs = (Merchant_Click_Struct *) data;
		//merchant id: mcs->npcid;
	}
	break;
	case OP_ItemPacket: {
		if(to_server || len < sizeof(ItemPacket_Struct))
			return;
		
		ItemPacket_Struct *i = (ItemPacket_Struct *) data;
		if(i->PacketType != ItemPacketMerchant)
			return;
		//merchant id: mcs->npcid;
	}
	break;
	case OP_ClientUpdate: {
		if(len != sizeof(PlayerPositionUpdateServer_Struct))
			return;
		
/*		PlayerPositionUpdateServer_Struct *spu = 
(PlayerPositionUpdateServer_Struct *) data;
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
*/
	}
	break;
	default:
		break;
	}
}

void SpawnExtractor::GenerateAnInsert(FILE *into, bool make_replaces, bool was_update, ExtractItem *itemo) {
	SpawnItem *item = (SpawnItem *) itemo;
	
	//get and NPC ID for this guy
	item->npc_id = GetNextNPCID(NULL);
	item->data[SpawnItem::id] = ultoa(item->npc_id);
	ExtractCollector::GenerateAnInsert(into, make_replaces, was_update, item);
	
	item->GenerateSpawnInserts();
}

void SpawnExtractor::GenerateAnUpdate(FILE *into, ExtractorDB *db, ExtractItem *itemo) {
//	SpawnItem *item = (SpawnItem *) itemo;
	
//	item->LearnNPCID();
}

void SpawnExtractor::RegisterSpawnID(uint16 spawn_id, SpawnItem *si) {
	spawns[spawn_id] = si;
}

uint32 SpawnExtractor::GetNextNPCID(ExtractorDB *db) {
	if(max_id == 0) {
		//needs to query the max spawn ID based on this zone
		int zoneid = zone_info->GetZoneID();
		
		if(db == NULL) {
			max_id = zoneid * 1000;
		} else {
			//"SELECT max(id) FROM npc_types WHERE id >= %d AND id < %d",
			// zoneid*1000, zoneid*1000+1000
			//max_id = atoi(row[0]) + 1;
		}
	}
	return(max_id++);
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
	
//	PathPoint &spawn_point = *cur;

	//spit up the spawn2, spawn group, spawn entry
}

uint32 SpawnExtractor::SpawnItem::FromPacket(unsigned char *packet, uint32 len) {
	if(len < sizeof(Spawn_Struct)) {
		printf("Packet of length %d is too short to be a Spawn packet (len %d)\n", len, sizeof(Spawn_Struct));
		return(0);
	}
	Spawn_Struct *i = (Spawn_Struct *) packet;
	
	bool we_care = true;
	
	spawn_id = i->spawnId;
	if(i->NPC != 1 || i->petOwnerId != 0 || i->name[0] == '\0')
		we_care = false;
	
	if(!we_care)
	{
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
	data[level] = itoa(i->level);
	data[race] = itoa(i->race);
	data[class_] = itoa(i->class_);
	data[gender] = itoa(i->gender);
	data[bodytype] = itoa(i->bodytype);
	data[beardcolor] = itoa(i->beardcolor);
	data[beard] = itoa(i->beard);		//no place in DB for this
	data[eyecolor1] = itoa(i->eyecolor1);
	data[eyecolor2] = itoa(i->eyecolor2);	//no place in DB for this
	data[face] = itoa(i->face);
	data[hairstyle] = itoa(i->hairstyle);
	data[haircolor] = itoa(i->haircolor);
	data[size] = ftoa(i->size);
	data[findable] = itoa(i->findable);
	data[equip_chest2] = itoa(i->equip_chest2);
	data[helm] = itoa(i->helm == 255? 0 : i->helm);
	data[runspeed] = ftoa(i->runspeed);
	data[walkspeed] = ftoa(i->walkspeed);
	data[last_name] = i->lastName;

#ifdef SPAWN_DEBUG_MODE
	//printf("Name: '%s'\n", i->name);
	//printf("	beardcolor	= 0x%02x\n", i->beardcolor);
	//printf("	beard 		= 0x%02x\n", i->beard);
	//printf("	eyecolor1	= 0x%02x\n", i->eyecolor1);
	//printf("	eyecolor2	= 0x%02x\n", i->eyecolor2);
	//printf("	face		= 0x%02x\n", i->face);
	//printf("	hairstyle	= 0x%02x\n", i->hairstyle);
	//printf("	haircolor	= 0x%02x\n", i->haircolor);
	//printf("	texture		= 0x%02x\n", i->texture);
	//printf("	helm		= 0x%02x\n", i->helm);
#endif
	
	//record initial position
	PathPoint p;
	p.x = EQ19toFloat(i->x);
	p.y = EQ19toFloat(i->y);
	p.z = EQ19toFloat(i->z);
	p.h = EQ19toFloat(i->heading);
	p.dx = NewEQ13toFloat(i->deltaX);
	p.dy = NewEQ13toFloat(i->deltaY);
	p.dz = NewEQ13toFloat(i->deltaZ);
	p.dh = EQ13toFloat(i->deltaHeading);
	positions.push_back(p);
	
	parent->RegisterSpawnID(spawn_id, this);
	
	return(sizeof(Spawn_Struct));
}


CharacterExtractor::CharacterExtractor(uint32 char_id, ZoneInfoExtractor *zi)
: ExtractCollector(OP_PlayerProfile, "character_"),
  charid(char_id)
{
	zone_info = zi;
	memset(&m_pp, 0, sizeof(m_pp));
	got_it = false;
}
	
void CharacterExtractor::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
	if(emu_op == OP_PlayerProfile) {
		if(len != sizeof(m_pp)) {
			printf("# size mismatch on OP_PlayerProfile (want %d, got %d), not extracting.\n", sizeof(m_pp), len);
			return;
		}
		got_it = true;
		memcpy(&m_pp, data, sizeof(m_pp));
	}
}

void CharacterExtractor::GenerateInserts(FILE *into, bool make_replaces) {
	if(!got_it)
		return;
	
	fprintf(into, "# Insert mode player profile not currently supported\n");
}

void CharacterExtractor::GenerateUpdates(FILE *into, ExtractorDB *db) {
	if(!got_it)
		return;
	
	char *query = new char[376 + sizeof(PlayerProfile_Struct)*2];
	char* end = query;

	end += sprintf(end, "UPDATE character_ SET timelaston=unix_timestamp(now()),"
						"name=\'%s\', x = %f, y = %f, z = %f, profile=\'", 
				m_pp.name, 
				m_pp.x, m_pp.y, m_pp.z);
	end += db->DoEscapeString(end, (char*)&m_pp, sizeof(PlayerProfile_Struct));
	end += sprintf(end,"\',class=%d,level=%d WHERE id=%u\n", m_pp.class_, m_pp.level, 
		charid);


	fprintf(into, query);
	delete[] query;
}

void CharacterExtractor::GenerateTexts(FILE *into, ExtractorDB *db) {
	if(!got_it)
		return;
	
	fprintf(into, "# Text mode player profile not currently supported\n");
}



