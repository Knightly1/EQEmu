/*

EQ Extractor, by Father Nitwit 2005

Extractors to add:
Recipe Lists
Recipe Details
Task History
Task Details


*/
extern PlayerProfile_Struct joe;
//no macro guard on purpose

#include "ExtractCollector.h"

//special extractor to get zone info needed in the primary key
//of many other exractors, but not avaliable in their packets
//inherits ExtractCollector to make my life easier
class ZoneInfoExtractor : public ExtractCollector {
public:
	ZoneInfoExtractor(const char *filename);
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
	
	uint16 GetZoneID() const { return(zone_id); }
	const char *GetShortName() const { return(short_name.c_str()); }
	const char *GetLongName() const { return(long_name.c_str()); }
	
	void EnableCatalog() { catalog_enabled = true; }
	
	//returns true if this extractor got all the info it needed
	bool Complete() {
		return(zone_id != 0xFFFF);
	}
	
	virtual ExtractItem *NewItem() { return(new CrapItem()); }
protected:
	class CrapItem : public ExtractCollector::ExtractItem {
		virtual uint32 FromPacket(unsigned char *data, uint32 len) {return(len);}
	};
	
	//these default to invalid things until the zone packet is seen.
	uint16 zone_id;
	string short_name;
	string long_name;
	string file_name;	//.pf file we are reading from
	bool catalog_enabled;
};

//special extractor to print a spawn list
//inherits ExtractCollector to make my life easier
class SpawnListExtractor : public ExtractCollector {
public:
	SpawnListExtractor();
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
	
	virtual ExtractItem *NewItem() { return(new CrapItem()); }
protected:
	class CrapItem : public ExtractCollector::ExtractItem {
		virtual uint32 FromPacket(unsigned char *data, uint32 len){return(len);}
	};
};

class DoorExtractor : public ExtractCollector {
public:
	DoorExtractor(ZoneInfoExtractor *zi);
	virtual ExtractItem *NewItem() { return(new DoorItem(zone_info)); }
	
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
	
	virtual void GenerateAnInsert(FILE *into, bool make_replaces, bool was_update, ExtractItem *item);
	virtual void GenerateAnUpdate(FILE *into, ExtractorDB *db, ExtractItem *item);
	virtual void GenerateAText(FILE *into, ExtractorDB *db, ExtractItem *item);
	
protected:
	ZoneInfoExtractor *zone_info;
	
	class zone_point {
	public:
		uint32 dest_zone;
		float x;
		float y;
		float z;
		float h;
	};
	map<uint32, zone_point> m_zonePoints;
	
	class DoorItem : public ExtractCollector::ExtractItem {
	public:
		ZoneInfoExtractor *zone_info;
		DoorItem(ZoneInfoExtractor *zi) {
			zone_info = zi;
		}
		
		void LookupDest(map<uint32, zone_point> &zonePoints);
		
		//enum in here to form a kind of 'namespace'
		enum {
			doorid = 0,	//must start at 0
			zone,
			name,
			pos_x,
			pos_y,
			pos_z,
			heading,
			opentype,
			doorisopen,
			door_param,
			incline,
			dest_zone,
			dest_x,
			dest_y,
			dest_z,
			dest_heading,
			size
		};
		virtual uint32 FromPacket(unsigned char *data, uint32 len);
	};
};

class FuzzyDoorExtractor : public DoorExtractor {
public:
	FuzzyDoorExtractor(ZoneInfoExtractor *zi);
	
protected:
	virtual void GenerateClauses(string &field_names, string &where_clause, ExtractItem *item);
};	


class AAExtractor : public ExtractCollector {
public:
	AAExtractor();
	virtual ExtractItem *NewItem() { return(new AAItem()); }
	
//	virtual void GivePacket(unsigned char *data, uint32 len);
	
	virtual void GenerateAnInsert(FILE *into, bool make_replaces, bool was_update, ExtractItem *item);
	virtual void GenerateAnUpdate(FILE *into, ExtractorDB *db, ExtractItem *item);
	virtual void GenerateAText(FILE *into, ExtractorDB *db, ExtractItem *item);
	
protected:
	
	/*
		nobody but the AAExtractor needs to know this exists
	*/
	class AAAbilityExtractor : public ExtractCollector {
	public:
		AAAbilityExtractor();
		virtual ExtractItem *NewItem();
		
		//virtual void GivePacket(unsigned char *data, uint32 len);
		
		void SetAAID(uint32 aa_) { aa_id = aa_; }
	protected:
		uint32 aa_id;
		class AAAbilityItem : public ExtractCollector::ExtractItem {
		public:
			//enum in here to form a kind of 'namespace'
			enum {
				aa_id = 0,	//must start at 0
				ability,
				increase_amt,
				last_level,
				unknown08
			};
			virtual uint32 FromPacket(unsigned char *data, uint32 len);
		};
		
	};
	
	
	
	class AAItem : public ExtractCollector::ExtractItem {
	public:
		//enum in here to form a kind of 'namespace'
		enum {
			aaid = 0,	//must start at 0
			hotkey_sid,
			hotkey_sid2,
			title_sid,
			desc_sid,
			class_type,
			cost,
			//seq?
			//current_level
			prereq_skill,
			prereq_minpoints,
			type,
			spellid,
			spell_type,
			spell_refresh,
			classes,
			berserker,
			max_level,
			//name,
			//last_id,?
			//next_id,?
		};
		virtual uint32 FromPacket(unsigned char *data, uint32 len);
		AAAbilityExtractor abilities;
	};
	
};


class ZoneHeaderExtractor : public ExtractCollector {
public:
	ZoneHeaderExtractor(ZoneInfoExtractor *zi);
	virtual ExtractItem *NewItem() { return(new ZoneHeaderItem(zone_info)); }
	
protected:
	ZoneInfoExtractor *zone_info;
	
	class ZoneHeaderItem : public ExtractCollector::ExtractItem {
	public:
		ZoneInfoExtractor *zone_info;
		ZoneHeaderItem(ZoneInfoExtractor *zi) {
			zone_info = zi;
		}
		
		//enum in here to form a kind of 'namespace'
		enum {
			zone_short_name = 0,	//must start at 0
			zone_long_name,
			ztype,
			fog_red1,
			fog_green1,
			fog_blue1,
			fog_minclip1,
			fog_maxclip1,
			fog_red2,
			fog_green2,
			fog_blue2,
			fog_minclip2,
			fog_maxclip2,
			fog_red3,
			fog_green3,
			fog_blue3,
			fog_minclip3,
			fog_maxclip3,
			fog_red4,
			fog_green4,
			fog_blue4,
			fog_minclip4,
			fog_maxclip4,
			walkspeed,
			time_type,
			sky,
			zone_exp_multiplier,
			safe_x,
			safe_y,
			safe_z,
			underworld,
			minclip,
			maxclip,
			zone_id
		};
		virtual uint32 FromPacket(unsigned char *data, uint32 len);
	};
	
};


class ZonePointExtractor : public ExtractCollector {
public:
	ZonePointExtractor(ZoneInfoExtractor *zi);
	virtual ExtractItem *NewItem() { return(new ZonePointItem(zone_info)); }
	
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
	
protected:
	ZoneInfoExtractor *zone_info;
	
	class ZonePointItem : public ExtractCollector::ExtractItem {
	public:
		ZoneInfoExtractor *zone_info;
		ZonePointItem(ZoneInfoExtractor *zi) {
			zone_info = zi;
		}
		
		//enum in here to form a kind of 'namespace'
		enum {
			iterator = 0,	//must start at 0
			target_x,
			target_y,
			target_z,
			target_heading,
			target_zone,		//destination zone
			zone_short		//zone the line is in
		};
		virtual uint32 FromPacket(unsigned char *data, uint32 len);
	};
	
};


class ObjectExtractor : public ExtractCollector {
public:
	ObjectExtractor();
	virtual ExtractItem *NewItem() { return(new ObjectItem()); }
	
protected:
	
	class ObjectItem : public ExtractCollector::ExtractItem {
	public:
		//enum in here to form a kind of 'namespace'
		enum {
			zone_id = 0,	//must start at 0
//			drop_id,
			heading,
			x,
			y,
			z,
			object_name,
			object_type,
			unknown08,
			unknown10,
			unknown20,
			unknown24,
			unknown60,
			unknown64,
			unknown68,
			unknown72,
			unknown76,
			unknown84,
			unknown88,
			linked_list_addr_01,
			linked_list_addr_02
  
		};
		virtual uint32 FromPacket(unsigned char *data, uint32 len);
	};
	
};

class FuzzyObjectExtractor : public ObjectExtractor {
public:
	FuzzyObjectExtractor();
	
protected:
	virtual void GenerateClauses(string &field_names, string &where_clause, ExtractItem *item);
};

class TributeExtractor : public ExtractCollector {
public:
	TributeExtractor();
	virtual ExtractItem *NewItem() { return(new TributeItem()); }
	
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
	
	virtual void GenerateAnInsert(FILE *into, bool make_replaces, bool was_update, ExtractItem *item);
	virtual void GenerateAnUpdate(FILE *into, ExtractorDB *db, ExtractItem *item);
	virtual void GenerateAText(FILE *into, ExtractorDB *db, ExtractItem *item);
	
protected:
	
	/*
		nobody but the TributeExtractor needs to know this exists
	*/
	class TributeAbilityExtractor : public ExtractCollector {
	public:
		TributeAbilityExtractor();
		virtual ExtractItem *NewItem();
		
		//virtual void GivePacket(unsigned char *data, uint32 len);
		
		void SetTributeID(uint32 v) { tribute_id = v; }
	protected:
		uint32 tribute_id;
		class TributeAbilityItem : public ExtractCollector::ExtractItem {
		public:
			//enum in here to form a kind of 'namespace'
			enum {
				tribute_id = 0,	//must start at 0
				level,
				cost,
				item_id
			};
			virtual uint32 FromPacket(unsigned char *data, uint32 len);
		};
		
	};
	
	
	class TributeItem : public ExtractCollector::ExtractItem {
	public:
		//enum in here to form a kind of 'namespace'
		enum {
			tribute_id = 0,	//must start at 0
			tier_count,
			name,
			isguild
		};
		virtual uint32 FromPacket(unsigned char *data, uint32 len);
		TributeAbilityExtractor abilities;
	};
	
};


class TributeTextExtractor : public ExtractCollector {
public:
	TributeTextExtractor();
	virtual ExtractItem *NewItem() { return(new TributeTextItem()); }
	
protected:
	
	class TributeTextItem : public ExtractCollector::ExtractItem {
	public:
		//enum in here to form a kind of 'namespace'
		enum {
			id = 0,	//must start at 0
			description
		};
		virtual uint32 FromPacket(unsigned char *data, uint32 len);
	};
	
};


class BookTextExtractor : public ExtractCollector {
public:
	BookTextExtractor();
	virtual ExtractItem *NewItem();
	
//	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len);
	
protected:
	string last_name;	//name of the last book request seen.
	
	class BookTextItem : public ExtractCollector::ExtractItem {
	public:
		BookTextItem(string *ln);
		//enum in here to form a kind of 'namespace'
		enum {
			name = 0,	//must start at 0
			text
		};
		virtual uint32 FromPacket(unsigned char *data, uint32 len);
		
		string *last_name;
	};
};


class TitleExtractor : public ExtractCollector {
public:
	TitleExtractor();
	virtual ExtractItem *NewItem() {  return(new TitleItem()); }
	
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
	
protected:
	
	class TitleItem : public ExtractCollector::ExtractItem {
	public:
		//enum in here to form a kind of 'namespace'
		enum {
			skill_id = 0,	//must start at 0
			skill_value,
			title
		};
		virtual uint32 FromPacket(unsigned char *data, uint32 len);
	};
};

class RecipeExtractor : public ExtractCollector {
public:
	RecipeExtractor();
	virtual ExtractItem *NewItem() { return(new RecipeItem()); }
	
	//virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len);
	
	virtual void GenerateAnInsert(FILE *into, bool make_replaces, bool was_update, ExtractItem *item);
	virtual void GenerateAnUpdate(FILE *into, ExtractorDB *db, ExtractItem *item);
	virtual void GenerateAText(FILE *into, ExtractorDB *db, ExtractItem *item);
	
protected:
	
	/*
		nobody but the RecipeExtractor needs to know this exists
	*/
	class RecipeItemExtractor : public ExtractCollector {
	friend class RecipeExtractor;	//so it can add components
	public:
		RecipeItemExtractor();
		virtual ExtractItem *NewItem();
		
		//virtual void GivePacket(unsigned char *data, uint32 len);
		
	protected:
		class RecipeItemItem : public ExtractCollector::ExtractItem {
		public:
			//enum in here to form a kind of 'namespace'
			enum {
				recipe_id = 0,	//must start at 0
				item_id,
				successcount,
				componentcount,
				iscontainer
			};
			virtual uint32 FromPacket(unsigned char *data, uint32 len);
		};
		
	};
	
	
	class RecipeItem : public ExtractCollector::ExtractItem {
	public:
		//enum in here to form a kind of 'namespace'
		enum {
			recipe_id = 0,	//must start at 0
			name,
			tradeskill,
			trivial,
		};
		virtual uint32 FromPacket(unsigned char *data, uint32 len);
		RecipeItemExtractor items;
	};
	
};



class TaskExtractor : public ExtractCollector {
public:
	TaskExtractor();
	virtual ExtractItem *NewItem() { return(new TaskActivity()); }
	
	//virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len);
	
	virtual void GenerateAnInsert(FILE *into, bool make_replaces, bool was_update, ExtractItem *item);
	virtual void GenerateAnUpdate(FILE *into, ExtractorDB *db, ExtractItem *item);
	virtual void GenerateAText(FILE *into, ExtractorDB *db, ExtractItem *item);
	
protected:
	
	/*
		nobody but the TaskExtractor needs to know this exists
	*/
	class TaskActivityExtractor : public ExtractCollector {
	friend class TaskExtractor;	//so it can add components
	public:
		TaskActivityExtractor();
		virtual ExtractItem *NewItem();
		
		//virtual void GivePacket(unsigned char *data, uint32 len);
		
	protected:
		class TaskActivityItem : public ExtractCollector::ExtractItem {
		public:
			//enum in here to form a kind of 'namespace'
			enum {
				task_id = 0,	//must start at 0
				item_id,
				successcount,
				componentcount,
				iscontainer
			};
			virtual uint32 FromPacket(unsigned char *data, uint32 len);
		};
		
	};
	
	
	class TaskActivity : public ExtractCollector::ExtractItem {
	public:
		//enum in here to form a kind of 'namespace'
		enum {
			task_id = 0,	//must start at 0
			name,
			tradeskill,
			trivial,
		};
		virtual uint32 FromPacket(unsigned char *data, uint32 len);
		TaskActivityExtractor items;
	};
	
};




class TaskHistoryExtractor : public ExtractCollector {
public:
	TaskHistoryExtractor();
	virtual ExtractItem *NewItem() {  return(new TaskHistoryItem()); }
	
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
	
protected:
	
	class TaskHistoryItem : public ExtractCollector::ExtractItem {
	public:
		//enum in here to form a kind of 'namespace'
		enum {
			task_id = 0,	//must start at 0
			task_name
		};
		virtual uint32 FromPacket(unsigned char *data, uint32 len);
	};
};



class SpawnExtractor : public ExtractCollector {
	class SpawnItem;
public:
	SpawnExtractor(ZoneInfoExtractor *zi);
	virtual ExtractItem *NewItem() { return(new SpawnItem(zone_info, this)); }
	
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
	
	virtual void GenerateAnInsert(FILE *into, bool make_replaces, bool was_update, ExtractItem *item);
	virtual void GenerateAnUpdate(FILE *into, ExtractorDB *db, ExtractItem *item);
//	virtual void GenerateAText(FILE *into, ExtractorDB *db, ExtractItem *item);
	
	void RegisterSpawnID(uint16 spawn_id, SpawnItem *si);
	
protected:
	ZoneInfoExtractor *zone_info;
	
	class PathPoint {
	public:
		float x,y,z,h;
		float dx, dy, dz, dh;	//deltas
	};
	
	class SpawnItem : public ExtractCollector::ExtractItem {
	public:
		ZoneInfoExtractor *zone_info;
		SpawnExtractor *parent;
		vector<PathPoint> positions;
		
		SpawnItem(ZoneInfoExtractor *zi, SpawnExtractor *p) {
			zone_info = zi;
			parent = p;
		}
		//enum in here to form a kind of 'namespace'
		enum {
			name = 0,	//must start at 0
			beardcolor,
			class_,
			equip_chest2,
			race,
			eyecolor1,
			eyecolor2,
			beard,
			face,
			level,
			hairstyle,
			haircolor,
			size,
			helm,
			runspeed,
			walkspeed,
			gender,
			last_name,
			bodytype,
			findable,
			id
		};
		virtual uint32 FromPacket(unsigned char *data, uint32 len);
		
		void GenerateSpawnInserts();
		uint16 spawn_id;
		uint32 npc_id;
	};
	
	map<uint16, SpawnItem *> spawns;
	
	uint32 GetNextNPCID(ExtractorDB *db);
	uint32 max_id;
};



class CharacterExtractor : public ExtractCollector {
public:
	CharacterExtractor(uint32 charid, ZoneInfoExtractor *zi);
	virtual ExtractItem *NewItem() { return(new CrapItem(zone_info)); }
	
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);

	virtual void GenerateInserts(FILE *into, bool make_replaces);
	virtual void GenerateUpdates(FILE *into, ExtractorDB *db);
	virtual void GenerateTexts(FILE *into, ExtractorDB *db);
	
protected:
	ZoneInfoExtractor *zone_info;
	
	const uint32 charid;
	PlayerProfile_Struct m_pp;
	bool got_it;
	
	class CrapItem : public ExtractCollector::ExtractItem {
	public:
		ZoneInfoExtractor *zone_info;
		CrapItem(ZoneInfoExtractor *zi) {
			zone_info = zi;
		}
		
		virtual uint32 FromPacket(unsigned char *data, uint32 len) { return(len); }
	};
};







