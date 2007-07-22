/*

EQ Extractor, by Father Nitwit 2005

*/
#include "../common/debug.h"
#include "Explorers.h"

#include "../common/classes.h"
#include "../common/eq_packet_structs.h"
//#include "patches/patch_051105.h"
//using namespace EQE_Patch_051105;

#include "../common/MiscFunctions.h"
#include "../common/packet_dump.h"

void ZoneHeaderExplorer::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
	if(emu_op != OP_NewZone)
		return;
	if(len != sizeof(NewZone_Struct)) {
		printf("Size of newzone struct is invalid! Cannot explore zone header.\n");
		return;
	}
	NewZone_Struct *i = (NewZone_Struct *) data;
	
	printf("\nNewZone_Struct (%s):\n", i->zone_short_name);
	PrintBlock(i, unknown323);
	PrintBlock(i, unknown360);
	PrintBlock(i, unknown331);
	PrintBlock(i, unknown_end);
	PrintBlock(i, unknown672);
	PrintBlock(i, unknown688);
//	PrintBlock(i, unknown692);
	PrintBlock(i, sky);
DumpPacket((unsigned char *) i, sizeof(NewZone_Struct));
	
/*	PrintFloats(i, unknown323);
	PrintFloats(i, unknown360);
	PrintFloats(i, unknown331);
	PrintFloats(i, unknown_end);
	PrintFloats(i, unknown672);*/
//	PrintFloatsReal("ns", (const char *) i, sizeof(NewZone_Struct));
}

void ObjectExplorer::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
	if(emu_op != OP_GroundSpawn)
		return;
	if(len != sizeof(Object_Struct)) {
		printf("Size of object struct is invalid! Cannot explore this object.\n");
		return;
	}
	Object_Struct *i = (Object_Struct *) data;
	
	printf("\nObject_Struct (%s at %f,%f,%f):\n", i->object_name, i->x, i->y, i->z);
	PrintBlock(i, unknown008);
	PrintBlock(i, unknown020);
	PrintBlock(i, unknown024);
	PrintBlock(i, unknown064);
	PrintBlock(i, unknown068);
	PrintBlock(i, unknown072);
	PrintBlock(i, unknown076);
	PrintBlock(i, unknown084);
}

/*struct Mask1 {
	sint32 n19:19,
		   n13:13;
};

struct Mask2 {
	sint32 n13:13,
		   n19:19;
};*/

void SpawnExplorer::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
	if(emu_op != OP_ZoneSpawns && emu_op != OP_NewSpawn && emu_op != OP_ZoneEntry)
		return;
	if(len < sizeof(Spawn_Struct)) {
		printf("Size of spawn struct is invalid! Cannot explore spawn packet.\n");
		return;
	}
	
	uint32 used = 0;
	while(used < len) {
		Spawn_Struct *i = (Spawn_Struct *) (data + used);
//printf("Packet: %d/%lu\n", sizeof(Spawn_Struct), len);
//DumpPacket((unsigned char *) i, sizeof(Spawn_Struct));
		
		printf("\nSpawn_Struct (%s <%s> #%d):\n", i->name, i->lastName, i->spawnId);
/*		PrintBlock(i, findable);
		PrintBlock(i, unknown);
		PrintBlock(i, unknown0114);
		PrintBlock(i, unknown0126);
		PrintBlock(i, unknown0132);
		PrintBlock(i, unknown0156);
		PrintBlock(i, unknown0159);
		PrintBlock(i, unknown0167);
		PrintBlock(i, unknown0213);
		PrintBlock(i, unknown0259);
		PrintBlock(i, unknown0264);
		PrintBlock(i, unknown0276);
		PrintBlock(i, unknown0381);
*/
		
/*		
		
		Mask1 *m1 = (Mask1 *) &i->deltaZ;
		Mask2 *m2 = (Mask2 *) &i->deltaZ;
		//28 bytes, 6 entries
		int r;
		for(r = 0; r < 6; r++) {
			printf("Masks: %d: 1(%f %f) 2(%f %f)\n", r, 
				EQ19toFloat(m1->n19), EQ13toFloat(m1->n13), 
				EQ19toFloat(m2->n19), EQ13toFloat(m2->n13) );
			m1++;
			m2++;
		}*/
		
		
		used += sizeof(Spawn_Struct);
	}
}

void SpellExplorer::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
	uint8 class_filter = BARD;	//0 means no filter
	
	switch(emu_op) {
	case OP_ZoneSpawns:
	case OP_NewSpawn:
	case OP_ZoneEntry: {
		if(len < sizeof(Spawn_Struct)) {
			if(len != sizeof(ClientZoneEntry_Struct))
				printf("Size of spawn struct (%d) is invalid (want %d)! Cannot explore this spawn list.\n", len, sizeof(Spawn_Struct));
			return;
		}
		
		uint32 used = 0;
		while(used < len) {
			Spawn_Struct *i = (Spawn_Struct *) (data + used);
			
			m_classMap[i->spawnId] = i->class_;
			m_nameMap[i->spawnId] = i->name;
			if(i->class_ == class_filter)
				printf("Found entity %s with id %d of class %d\n", i->name, i->spawnId, class_filter);
			used += sizeof(Spawn_Struct);
		}
		break;
	}
	case OP_Buff: {	//client->server
		SpellBuffFade_Struct *i = (SpellBuffFade_Struct *) data;
		if(class_filter != 0) {
			map<uint16, uint8>::iterator res = m_classMap.find(i->entityid);
			if(res == m_classMap.end() || res->second != class_filter)
				return;	//filtered
		}
		
		printf("OP_Buff %s: eid %d, slot %d, spell %d, dur %d, sid %d, bf %d, u7 %d, u16 %d, u20 %d\n",
			to_server?"To Server":"To Client", i->entityid,
			i->slot,
			i->spellid,
			i->duration,
			i->slotid,
			i->bufffade,
			i->unknown7,
			i->unknown016,
			i->unknown020 );
		break;
	}
	case OP_BuffFadeMsg: {
		BuffFadeMsg_Struct *i = (BuffFadeMsg_Struct *) data;
		printf("OP_BuffFadeMsg %s: %s\n",
			to_server?"To Server":"To Client", i->msg);
		break;
	}
	case OP_BeginCast: {
		BeginCast_Struct *i = (BeginCast_Struct *) data;
		if(class_filter != 0) {
			map<uint16, uint8>::iterator res = m_classMap.find(i->caster_id);
			if(res == m_classMap.end() || res->second != class_filter)
				return;	//filtered
		}
		printf("OP_BeginCast %s: Caster %d casting %d\n",
			to_server?"To Server":"To Client", i->caster_id, i->spell_id);
		break;
	}
	case OP_CastSpell: {	//client->server
		if(!to_server) {
			printf("OP_CastSpell\n");
		}
		/*CastSpell_Struct
		slot
		spell_id
		target_id*/
		break;
	}
	case OP_InterruptCast: {
		InterruptCast_Struct *i = (InterruptCast_Struct *) data;
		if(class_filter != 0) {
			map<uint16, uint8>::iterator res = m_classMap.find(i->spawnid);
			if(res == m_classMap.end() || res->second != class_filter)
				return;	//filtered
		}
		printf("OP_InterruptCast %s: Caster %d msg %d str %s\n",
			to_server?"To Server":"To Client", i->spawnid, i->messageid, i->message);
		
		break;
	}
	case OP_Action: {
		Action_Struct *i = (Action_Struct *) data;
		if(class_filter != 0) {
			map<uint16, uint8>::iterator res = m_classMap.find(i->source);
			if(res == m_classMap.end() || res->second != class_filter)
				return;	//filtered
		}
		printf("OP_Action %s: Caster %d, target %d, seq %d, type %d, spell %d\n"
				"\tuB %d, imod %d, u8 %d, u16 %d, u18 %d, u23 %d, u29 %d\n",
			to_server?"To Server":"To Client", 
			i->source, i->target, i->sequence, i->type, i->spell,
			i->buff_unknown, i->instrument_mod, i->unknown08, i->unknown16, i->unknown18, i->unknown23, i->unknown29);
		break;
	}
	case OP_Action2: {
		PrintBlockReal("OP_Action2", (const char *) data, len);
		break;
	}
	case OP_Animation: {
		Animation_Struct *i = (Animation_Struct *) data;
		printf("OP_Animation %s: Spawn %d, action %d, value %d\n",
			to_server?"To Server":"To Client", 
			i->spawnid, i->action, i->value);
		break;
	}
	case OP_EnvDamage: {
		EnvDamage2_Struct *i = (EnvDamage2_Struct *) data;
		printf("OP_EnvDamage %s: Spawn %d, dmg %d, type %d\n",
			to_server?"To Server":"To Client", 
			i->id, i->damage, i->dmgtype);
		PrintBlock(i, unknown4);
		PrintBlock(i, unknown10);
		PrintBlock(i, unknown2);
		PrintBlock(i, unknown29);
		break;
	}
	/*case OP_Action: {
		break;
	}
	case OP_Action: {
		break;
	}
	case OP_Action: {
		break;
	}*/
	default:
		break;
	}
}

void ArrowExplorer::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
	if(emu_op != OP_SomeItemPacketMaybe)
		return;
	if(len != sizeof(Arrow_Struct)) {
		printf("Size of arrow struct is invalid! Cannot explore zone header.\n");
		return;
	}
	Arrow_Struct *i = (Arrow_Struct *) data;
	
	printf("\nArrow_Struct (%s):\n", i->model_name);
	printf("Floats: from (%f %f %f)\n", i->src_x, i->src_y, i->src_z);
	PrintBlock(i, unknown004);
	PrintBlock(i, unknown028);
	PrintBlock(i, unknown052);
	PrintBlock(i, unknown064);
	PrintBlock(i, unknown088);
	PrintBlock(i, unknown092);
	PrintBlock(i, unknown096);
	PrintBlock(i, unknown117);
}

void SpawnListExplorer::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
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
			i->name, i->lastName, i->class_, i->race, i->level,
			EQ19toFloat(i->x), EQ19toFloat(i->y), EQ19toFloat(i->z)
			);
		
		used += sizeof(Spawn_Struct);
	}
}

void MessageExplorer::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
	switch(emu_op) {
	case OP_FormattedMessage: {
		FormattedMessage_Struct *s = (FormattedMessage_Struct *) data;
		printf("Format %d: ", s->string_id);
		int len;
		const char *msg = s->message;
		len = strlen(msg);
		while(len > 0) {
			printf("'%s' ", msg);
			msg += len + 1;
			len = strlen(msg);
		}
		printf("\n");
		break;
	}
	case OP_SimpleMessage: {
		SimpleMessage_Struct *s = (SimpleMessage_Struct *) data;
		printf("Simple: %d\n", s->string_id);
		break;
	}
	case OP_ChannelMessage: {
		ChannelMessage_Struct *s = (ChannelMessage_Struct *) data;
		printf("Channel: %s->%s: \"%s\"\n", s->sender, s->targetname, s->message);
		break;
	}
	default:
		break;
	}
}

void DumpUnknownExplorer::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
/*	if(to_server) {
		printf("Client->Server: [ Opcode: %s Size: %d ]\n", );
	} else {
	}
*/	DumpPacket((unsigned char *) data, len);
}

void UnknownExplorer::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
	if(emu_op != OP_ExploreUnknown)
		return;
	
	PrintBlockReal("p", (const char *) data, len);
}

#pragma pack(1)
struct u2struct {
	float f1;
	float f2;
	float f3;
	uint8 unknown12[5];
};
#pragma pack()
void Unknown2Explorer::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
	if(emu_op != OP_ExploreUnknown)
		return;
	
	printf("\nNew Packet:\n");
	u2struct *p = (u2struct *) data;
	int count = (len-1)/sizeof(u2struct);
	for(; count > 0; count--) {
		printf("unknown: (%f, %f, %f):\n", p->f1, p->f2, p->f3);
		PrintBlock(p, unknown12);
	}
}

void ClientUpdateExplorer::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
	if(emu_op != OP_ClientUpdate)
		return;
	
	if(len == sizeof(PlayerPositionUpdateServer_Struct)) {
		PlayerPositionUpdateServer_Struct *i = (PlayerPositionUpdateServer_Struct *) data;
		
		if(i->spawn_id == forid) {
			printf("To-Client(%d): (%f %f %f @%f) d(%f %f %f @%f) a=%x\n",
				i->spawn_id,
				EQ19toFloat(i->x_pos),
				EQ19toFloat(i->y_pos),
				EQ19toFloat(i->z_pos),
				//EQ19toFloat(i->heading),
				float(i->heading),
				NewEQ13toFloat(i->delta_x),
				NewEQ13toFloat(i->delta_y),
				NewEQ13toFloat(i->delta_z),
				//EQ13toFloat(i->delta_heading),
				float(i->delta_heading),
				i->animation);
//DumpPacket((unsigned char *) data, len);
			
//			PrintBlock(i, unknown02);
		}
		
	} else if(len == sizeof(PlayerPositionUpdateClient_Struct)) {
		PlayerPositionUpdateClient_Struct *i = (PlayerPositionUpdateClient_Struct *) data;
		
		if(forid == 0)
			forid = i->spawn_id;
/*		printf("To-Server(%d): (%f %f %f @%f) d(%f %f %f @%f) a=%d,%d\n",
			i->spawn_id, 
			i->x_pos, i->y_pos, i->z_pos, float(i->heading),
			i->delta_x, i->delta_y, i->delta_z, float(i->delta_heading),
			i->animation, i->unknown20
			);
*/		
/*		printf("Heading: %d->%f, d %d->%f\n", 
			i->heading, EQ13toFloat(i->heading), 
			i->delta_heading, EQ19toFloat(i->delta_heading));
*/	
//		PrintBlockLen(i, unknown0006+14, 4);
//		PrintBlock(i, u20);
//		PrintBlockLen(i, delta_x, 8);

//DumpPacket((unsigned char *) data, len);			
//	PrintBlockReal("p", (const char *) data, len);
		
	} else  {
		printf("Size of client update (%u) is invalid! Cannot explore this packet.\n", len);
		
printf("Packet: %u need (%d,%d)\n", len, sizeof(PlayerPositionUpdateClient_Struct), sizeof(PlayerPositionUpdateServer_Struct));
DumpPacket((unsigned char *) data, len);
		return;
	}
}


void PlayerProfileExplorer::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
	if(emu_op != OP_PlayerProfile)
		return;
	if(len != sizeof(PlayerProfile_Struct)) {
		printf("Size mismatch on player profile while explorng, want %d got %d.\n", sizeof(PlayerProfile_Struct), len);
		return;
	}
	PlayerProfile_Struct *i = (PlayerProfile_Struct *) data;
	
	int r;
	for(r = 0; r < BUFF_COUNT; r++) {
		const SpellBuff_Struct &c = i->buffs[r];
		printf("%d: id %d, lvl %d, bard %d, effect %d, dur %d, DS %d. PlayerID %d\n",
			c.slotid, c.spellid, c.level, c.bard_modifier, c.effect, c.duration, c.dmg_shield_remaining, c.player_id);
	}
//	DumpPacket(((unsigned char *) i->buffs)-16, sizeof(i->buffs)+32);
}









