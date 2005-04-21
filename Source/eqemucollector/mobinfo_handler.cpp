#include "../common/debug.h"
#include "../common/eq_packet_structs.h"
#include "PacketHandler.h"
#include "../common/EQPacket.h"
#include "EQStreamPair.h"
#include "mobinfo_handler.h"


//mob watching info
map<int32, CollectMobStruct> CollectMobList;
//bool CollectMobInfo;

CollectMobHandler::CollectMobHandler(const EQStreamPair *s)
: StreamPacketHandler(DelHandler, s)
{
	//not the best thing to do, but with it global, theres nothing better to do.
	//it needs to get cleared somewhere, this makes the most sense.
	//the destructor is bad since the old connection could time out after
	//the new session is underway.
	CollectMobList.clear();
}

CollectMobHandler::~CollectMobHandler() {
}

void CollectMobHandler::ToClientPacket(EQStreamType type, uint16 eq_opcode, EmuOpcode emu_opcode, const EQApplicationPacket *app) {
	switch (emu_opcode) {
		case OP_NewSpawn:
		case OP_ZoneSpawns: {
			CollectMobStruct cm;
			
			if ((app->size % sizeof(NewSpawn_Struct)) != 0)
				break;
			
			uint32 spawn_count = (app->size / sizeof(Spawn_Struct));
			uchar* buffer=(uchar*)app->pBuffer;
			for(int i=0;i<spawn_count;i++){
				Spawn_Struct* spawn=(Spawn_Struct*)buffer;
				if(spawn->npc>0 && spawn->npc!=2){
					cm.name = spawn->name;
					cm._class = spawn->class_;
					CollectMobList[spawn->spawn_id] = cm;
				}
				buffer+=sizeof(Spawn_Struct);
			}
			break;
		}
		//these two assume spawn ID is the first thing in these packets....
		case OP_MobUpdate: {
			if(app->size < 2)
				break;
			//mob updates are effectively waypoints
			int16 *spawn_id = (int16 *) app->pBuffer;
			if(CollectMobList.count(*spawn_id) == 1) {
				CollectMobList[*spawn_id].wp_count++;
			}
			break;
		}
		case OP_ClientUpdate: {
			if(app->size < 2)
				break;
			//badly labeled client update updates are generally movement between waypoints
			int16 *spawn_id = (int16 *) app->pBuffer;
			if(CollectMobList.count(*spawn_id) == 1) {
				CollectMobList[*spawn_id].move_count++;
			}
			break;
		}
		default:
			break;
	}
}

void CollectMobHandler::ToServerPacket(EQStreamType type, uint16 eq_opcode, EmuOpcode emu_opcode, const EQApplicationPacket *app) {
	if(emu_opcode != OP_ShopRequest)
		return;	//only need this one op

	if(app->size < sizeof(Merchant_Click_Struct))
		return;
	
	Merchant_Click_Struct *them = (Merchant_Click_Struct *) app->pBuffer;
	if(CollectMobList.count(them->npcid) == 1) {
		CollectMobList[them->npcid].found_merchant = true;
	}
}

StreamPacketHandler *CollectMobCreateHandler(EQStreamType type, const EQStreamPair *sp) {
	if(type != ZoneStream)
		return(NULL);	//not interested
	
	CollectMobHandler *pfh = new CollectMobHandler(sp);
	
	return(pfh);
}
