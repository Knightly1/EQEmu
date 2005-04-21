/*

EQ Extractor, by Father Nitwit 2005

*/
#include "Explorers.h"
#include "../common/eq_packet_structs.h"

void ZoneHeaderExplorer::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len) {
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
	PrintBlock(i, unknown672);
}

void ObjectExplorer::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len) {
	if(emu_op != OP_GroundSpawn)
		return;
	if(len != sizeof(Object_Struct)) {
		printf("Size of object struct is invalid! Cannot explore this object.\n");
		return;
	}
	Object_Struct *i = (Object_Struct *) data;
	
	printf("\nObject_Struct (%s):\n", i->object_name);
	PrintBlock(i, unknown008);
	PrintBlock(i, unknown020);
	PrintBlock(i, unknown060);
	PrintBlock(i, unknown084);
}

void SpawnExplorer::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len) {
	if(emu_op != OP_ZoneSpawns && emu_op != OP_NewSpawn)
		return;
	if(len < sizeof(Object_Struct)) {
		printf("Size of spawn struct is invalid! Cannot explore spawn packet.\n");
		return;
	}
	uint32 used = 0;
	while(used < len) {
		Spawn_Struct *i = (Spawn_Struct *) (data + used);
		
		printf("\nSpawn_Struct (%s <%s>):\n", i->name, i->last_name);
		PrintBlock(i, unknown032);
		PrintBlock(i, unknown044);
		PrintBlock(i, unknown144);
		PrintBlock(i, unknown173);
		PrintBlock(i, unknown249);
		PrintBlock(i, unknown260);
		PrintBlock(i, unknown272);
		PrintBlock(i, unknown355);
		PrintBlock(i, unknown367);
		
		used += sizeof(Spawn_Struct);
	}
}












