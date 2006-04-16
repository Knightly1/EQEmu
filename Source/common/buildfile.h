#ifndef BUILD_FILE_H
#define BUILD_FILE_H

#include "../common/types.h"
#include "../common/opcodemgr.h"
#include <stdio.h>

//constants used in the packet file header
#define BUILD_FILE_MAGIC 0x93a7b6f8

//our own type codes, in case they decide to change.
typedef enum {
	//Spawn stuff
	PF_OP_MobLocation = 0,		//OP_MobUpdate
	PF_OP_MobMovement,			//OP_ClientUpdate
	PF_OP_InitMobSpawn,			//OP_ZoneSpawns
	PF_OP_MobSpawn,				//OP_NewSpawn
	PF_OP_Death,				//OP_Death
	PF_OP_DeleteSpawn,			//OP_DeleteSpawn
	
	//spell stuff...
	PF_OP_CastSpell,			//OP_CastSpell
	
	//merchant stuff
	PF_OP_MerchantBegin,		//OP_ShopRequest
	PF_OP_MerchantEnd,			//OP_ShopEndConfirm
	PF_OP_MerchantItem,			//OP_Item
	
	//misc...
	PF_OP_NewZone,				//OP_NewZone
	
	_packetFileTypeCount
} PF_SectionType;



#pragma pack(1)
struct BuildFileHeader {
	uint32 build_file_magic;
};

struct BuildFileSection {
	uint16 type;
	uint16 len;
};

//NewZone_Struct
struct PF_NewZone {
	char zone_short_name[32];
};

//SpawnPositionUpdate_Struct
struct PF_MobLocation {
	uint16 spawn_id;
	float x;
	float y;
	float z;
	float heading;
};

//PlayerPositionUpdateServer_Struct
struct PF_MobMovement {
	uint16 spawn_id;
	float x;
	float y;
	float z;
	float heading;
	float delta_x;
	float delta_y;
	float delta_z;
	float delta_heading;
	uint16 animation;
};

//from Spawn_Struct
struct PF_MobSpawn {
	uint16	spawn_id;
	char	name[64];
	char	last_name[32];
	int8	npc;	// 0=player,1=npc,2=pc corpse,3=npc corpse,4=???,5=unknown spawn,10=self
	int8	beard;
	int8	beardcolor;
	int8	class_;
	int8	equip_chest2;
	int32	race;
	int8	eyecolor1;
	int8	eyecolor2;
	int8	face;
	int8	invis;
	int8	level;
	
	float	x;
	float	y;
	float	z;
	float	heading;
	float	delta_x;
	float	delta_y;
	float	delta_z;
	float	delta_heading;
	uint16	animation;
	
	int8	hairstyle;
	int8	haircolor;
	int8	light;
	float	size;
	int8	helm;
	float	runspeed;
	float	walkspeed;
	int8	gender;
	uint32	bodytype;
	uint32	pet_owner_id;
	int16	deity;
	
};

//Death_Struct
struct PF_Death {
	uint16 spawn_id;
};

//DeleteSpawn_Struct
struct PF_DeleteSpawn {
	uint16 spawn_id;
};

//BeginCast_Struct
struct PF_CastSpell {
	int16 caster_id;
	int16 spell_id;
};

//Merchant_Click_Struct
struct PF_MerchantBegin {
	uint16 spawn_id;
};

//Merchant_Click_Struct
struct PF_MerchantEnd {
	char nothing;
};

struct PF_MerchantItem {
	uint32 item_id;
	uint16 slot_id;
	uint16 merchant_slot;
};

#pragma pack()


class BuildFileReader {
public:
	BuildFileReader();
	virtual ~BuildFileReader();
	
	bool OpenFile(const char *name);
	void CloseFile();
	
	bool ReadSection(PF_SectionType &bf_op, uint16 &packlen, unsigned char *packet);

protected:
	
	//gzFile in;
	FILE *in;	
};

class BuildFileWriter {
public:
	BuildFileWriter();
	virtual ~BuildFileWriter();
	
	bool OpenFile(const char *name);
	void CloseFile();
	
	virtual void WritePacket(EmuOpcode emu_op, uint32 packlen, unsigned char *packet) = 0;

protected:
	bool _WriteBlock(PF_SectionType pf_op, const void *d, uint16 len);
	
	//gzFile out;
	FILE *out;	
};


#endif


