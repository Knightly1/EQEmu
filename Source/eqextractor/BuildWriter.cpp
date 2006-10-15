/*

EQ Extractor, by Father Nitwit 2005

*/
//#include "BuildWriter.h"	//do not include this

using namespace std;


void PatchBuildFileWriter::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) {
	//simple exchange, comes in on one interface, needs to go in on the other
	WritePacket(emu_op, len, data);
}

void PatchBuildFileWriter::WritePacket(EmuOpcode emu_op, uint32 packlen, unsigned char *packet) {
	if(out == NULL)
		return;
	
	switch(emu_op) {
	case OP_NewZone: {
		if(packlen != sizeof(NewZone_Struct)) {
			printf("Error: invalid size of OP_NewZone packet\n");
			return;
		}
		NewZone_Struct *them = (NewZone_Struct *) packet;
		PF_NewZone us;
		
		strncpy(us.zone_short_name, them->zone_short_name, 32);
		
		_WriteBlock(PF_OP_NewZone, &us, sizeof(us));
		break;
	}
	case OP_NewSpawn:
	case OP_ZoneSpawns: {
		Spawn_Struct *them = (Spawn_Struct *) packet;
		PF_MobSpawn us;
		
		int count = packlen / sizeof(Spawn_Struct);
		int r;
		for(r = 0; r < count; r++, them++) {
			if(them->NPC != 1)
				continue;		//only extract NPCs
			
			strncpy(us.name, them->name, 64);
			
			us.class_ = them->class_;
			us.race = them->race;
			us.invis = them->invis;
			us.level = them->level;
			
			us.x = EQ19toFloat(them->x);
			us.y = EQ19toFloat(them->y);
			us.z = EQ19toFloat(them->z);
#ifndef WIN32
#warning INVALID USE OF HEADING IN BUILD FILE WRITER
#endif
//dh is also wrong
			us.heading = them->heading;
			
#ifdef OLD_DELTAS
			us.delta_x = EQ13toFloat(them->deltaX);
			us.delta_y = EQ13toFloat(them->deltaY);
			us.delta_z = EQ13toFloat(them->deltaZ);
#else
			us.delta_x = NewEQ13toFloat(them->deltaX);
			us.delta_y = NewEQ13toFloat(them->deltaY);
			us.delta_z = NewEQ13toFloat(them->deltaZ);
#endif

			us.spawn_id = them->spawnId;
			strncpy(us.last_name, them->lastName, 32);
			
			//this seems wrong, we dont understand dh yet...
			us.delta_heading = them->deltaHeading;
			us.npc = them->NPC;
			us.pet_owner_id = them->petOwnerId;
			
			us.animation = them->animation;
			us.equip_chest2 = them->equip_chest2;
			us.light = them->light;
			us.size = them->size;
			us.runspeed = them->runspeed;
			us.walkspeed = them->walkspeed;
			us.gender = them->gender;
			us.bodytype = them->bodytype;
			us.deity = them->deity;
			
//			us.beard = them->beard;
			us.beardcolor = them->beardcolor;
			us.eyecolor1 = them->eyecolor1;
			us.eyecolor2 = them->eyecolor2;
			us.face = them->face;
//			us.helm = them->helm;
//			us.hairstyle = them->hairstyle;
			us.haircolor = them->haircolor;
			
			if(emu_op == OP_ZoneSpawns)
				_WriteBlock(PF_OP_InitMobSpawn, &us, sizeof(us));
			else
				_WriteBlock(PF_OP_MobSpawn, &us, sizeof(us));
		}
		break;
	}
	case OP_MobUpdate: {
		if(packlen != sizeof(SpawnPositionUpdate_Struct)) {
			printf("Error: invalid size of OP_MobUpdate packet\n");
			return;
		}
		SpawnPositionUpdate_Struct *them = (SpawnPositionUpdate_Struct *) packet;
		PF_MobLocation us;
		
		us.spawn_id = them->spawn_id;
		us.x = EQ19toFloat(them->x);
		us.y = EQ19toFloat(them->y);
		us.z = EQ19toFloat(them->z);
#ifndef WIN32
#warning INVALID USE OF HEADING IN BUILD FILE WRITER
#endif
		us.heading = them->heading;
		
		_WriteBlock(PF_OP_MobLocation, &us, sizeof(us));
		break;
	}
	case OP_ClientUpdate: {
		if(packlen != sizeof(PlayerPositionUpdateServer_Struct)) {
			//printf("Error: invalid size of OP_ClientUpdate packet\n");
			return;
		}
		PlayerPositionUpdateServer_Struct *them = (PlayerPositionUpdateServer_Struct *) packet;
		PF_MobMovement us;
		
		us.spawn_id = them->spawn_id;
		us.x = EQ19toFloat(them->x_pos);
		us.y = EQ19toFloat(them->y_pos);
		us.z = EQ19toFloat(them->z_pos);
#ifndef WIN32
#warning INVALID USE OF HEADING IN BUILD FILE WRITER
#endif
//dh is wrong too
		us.heading = them->heading;
		
#ifdef OLD_DELTAS
		us.delta_x = EQ13toFloat(them->delta_x);
		us.delta_y = EQ13toFloat(them->delta_y);
		us.delta_z = EQ13toFloat(them->delta_z);
#else
		us.delta_x = NewEQ13toFloat(them->delta_x);
		us.delta_y = NewEQ13toFloat(them->delta_y);
		us.delta_z = NewEQ13toFloat(them->delta_z);
#endif
		us.delta_heading = them->delta_heading;
		
		us.animation = them->animation;
		
		_WriteBlock(PF_OP_MobMovement, &us, sizeof(us));
		break;
	}
	case OP_Death: {
		if(packlen != sizeof(Death_Struct)) {
			printf("Error: invalid size of OP_Death packet\n");
			return;
		}
		Death_Struct *them = (Death_Struct *) packet;
		PF_Death us;
		
		us.spawn_id = them->spawn_id;
		
		_WriteBlock(PF_OP_Death, &us, sizeof(us));
		break;
	}
	case OP_DeleteSpawn: {
		if(packlen != sizeof(DeleteSpawn_Struct)) {
			printf("Error: invalid size of OP_DeleteSpawn packet\n");
			return;
		}
		DeleteSpawn_Struct *them = (DeleteSpawn_Struct *) packet;
		PF_DeleteSpawn us;
		
		us.spawn_id = them->spawn_id;
		
		_WriteBlock(PF_OP_DeleteSpawn, &us, sizeof(us));
		break;
	}
	case OP_BeginCast: {
		if(packlen != sizeof(BeginCast_Struct)) {
			printf("Error: invalid size of OP_CastSpell packet\n");
			return;
		}
		BeginCast_Struct *them = (BeginCast_Struct *) packet;
		PF_CastSpell us;
		
		us.caster_id = them->caster_id;
		us.spell_id = them->spell_id;
		
		_WriteBlock(PF_OP_CastSpell, &us, sizeof(us));
		break;
	}
	case OP_ShopRequest: {
		if(packlen != sizeof(Merchant_Click_Struct)) {
			printf("Error: invalid size of OP_ShopRequest packet\n");
			return;
		}
		Merchant_Click_Struct *them = (Merchant_Click_Struct *) packet;
		PF_MerchantBegin us;
		
		us.spawn_id = them->npcid;
		
		_WriteBlock(PF_OP_MerchantBegin, &us, sizeof(us));
		break;
	}
	case OP_ShopEndConfirm: {
		if(packlen != 0) {
			printf("Error: invalid size of OP_ShopEndConfirm packet %d != %d\n", packlen, 0);
			return;
		}
//		Merchant_Click_Struct *them = (Merchant_Click_Struct *) packet;
		PF_MerchantEnd us;
		
		
		_WriteBlock(PF_OP_MerchantEnd, &us, sizeof(us));
		break;
	}
	case OP_ShopItem:
	case OP_ItemPacket: {
		ItemPacket_Struct *them = (ItemPacket_Struct *) packet;
		PF_MerchantItem us;
		
		if(them->PacketType != ItemPacketMerchant)
			break;
		
		
		map<int,map<int,string> > parsed_items;
		map<int,map<int,string> >::iterator cur, end;
		if (!ItemParse(them->SerializedItem,
			packlen-sizeof(ItemPacket_Struct),
			parsed_items, ITEM_ID_POS, ITEM_NAME_POS, ITEM_MAX_FIELDS)) {
			break;
		}
		
		end = parsed_items.end();
		for(cur = parsed_items.begin(); cur != end; cur++) {
			us.item_id = cur->first;
			us.slot_id = atoi(cur->second[ITEM_SLOT_POS].c_str());
			us.merchant_slot = atoi(cur->second[ITEM_MERCHANT_SLOT_POS].c_str());
			
			_WriteBlock(PF_OP_MerchantItem, &us, sizeof(us));
		}
		
		break;
	}
	default:
		return;
	}
}










