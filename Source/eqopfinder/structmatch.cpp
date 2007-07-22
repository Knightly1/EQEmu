#include "structmatch.h"
#include "../common/opcodemgr.h"
#include "../common/emu_opcodes.h"
#include "../common/packet_dump.h"
#include "../common/MiscFunctions.h"
#include "../common/eq_constants.h"

#include "../common/patches/Anniversary_structs.h"
using namespace Anniversary::structs;


StructMatcher::StructMatcher(MutableOpcodeManager *op) {
	opcodes = op;
	MapOpcodes();
	
	history.resize(HISTORY_QUEUE_SIZE, NULL);
	future.resize(FUTURE_QUEUE_SIZE, NULL);
}

StructMatcher::~StructMatcher() {
	//todo: clean up history queue
}
	
void StructMatcher::Process(SMPacket *p) {
	//a linked list implement size() as:
	//distance(begin(), end()... thats rediculously slow
	future.push_front(p);
	SMPacket *past = future.back();
	future.pop_back();
	if(past != NULL)
		ProcessUnknown(past);
}

void StructMatcher::Finish() {
	list<SMPacket *>::reverse_iterator 
	cur(future.end()),
	end(future.begin());
	for(; cur != end; cur++) {
		SMPacket *p = *cur;
		if(p != NULL)
			ProcessUnknown(p);
	}
}

void StructMatcher::ProcessUnknown(SMPacket *p) {

	//see if we know what it is first
	if(p->opcode != OP_Unknown) {
		ProcessKnown(p);
		return;
	}
	
	//do not know what it is...
	//try each matcher now.
	map<EmuOpcode, StructMatchProc>::iterator cur,end;
	cur = matchers.begin();
	end = matchers.end();
	EmuOpcode op = OP_Unknown;
	bool conflict = false;
	
	//check them all to watch for conflicts
	im_sure = false;
	for(; cur != end; cur++) {
		StructMatchProc proc = cur->second;
		bool found = (this->*proc)(p);
		
		if(!found)
			continue;
		
		if(im_sure) {
			if(conflict) {
				printf("Conflict resolved by definate match.\n");
			}
			op = cur->first;
			conflict = false;
			break;
		} else if(op != OP_Unknown) {
			printf("Conflict: packet identified as '%s' first, then as '%s', rejecting second.\n", opcodes->EmuToName(op), opcodes->EmuToName(cur->first));
			conflict = true;
		} else {
			op = cur->first;
		}
	}
	
	if(conflict) {
		printf("Conflicting packet contents (EQ op 0x%04x):\n", p->eq_opcode);
		DumpPacket(p->pBuffer, p->size);
		printf("\n");
	}
	
	if(op != OP_Unknown) {
		//we have discovered a new opcode (even if it conflicted)
		p->opcode = op;
		
		//remember the new opcode
		opcodes->SetOpcode(op, p->eq_opcode);
		
		//process it since we know what it is now
		ProcessKnown(p);
		
		//remove this matcher, it will not be used again
		//even if there is another form, we allready know the opcode
		matchers.erase(op);
		
		//print a nice message
		printf("Discovered new opcode:\n");
		printf("%s=0x%04x\n", opcodes->EmuToName(op), p->eq_opcode);
	} else {
		//we were unable to figure out what this packet was
		printf("Unable to match packet (EQ op 0x%04x Size: %d):\n", p->eq_opcode, p->size);
//		if(p->size <= 16)
//			DumpPacket(p->pBuffer, p->size);
//		DumpPacket(p->pBuffer, p->size>1000?1000:p->size);
//		printf("\n");
		delete p;
	}
}
	
	
//takes a packet with a known opcode and uses it to gather all the crap below:
//takes ownership of p
void StructMatcher::ProcessKnown(SMPacket *p) {
	//first we add it to the begining of the history queue, removing the oldest one
	//skip the annoying unknown because it causes major saturation issues with history.
	if(p->opcode != OP_AnnoyingZoneUnknown
	&& p->opcode != OP_ClientUpdate) {
		history.push_front(p);
//	printf("pushing 0x%04x %s to front\n", p->eq_opcode, opcodes->EmuToName(p->opcode));
		SMPacket *tmp =  history.back();
		history.pop_back();
		safe_delete(tmp);
		history_cache.clear();
	}
	
	switch(p->opcode) {
	case OP_NewSpawn:
	case OP_ZoneSpawns: {
		MobInfoStruct cm;
		
		if ((p->size % sizeof(NewSpawn_Struct)) != 0) {
			printf("Invalid length of spawn struct: %d\n", p->size);
			break;
		}
		
		uint32 spawn_count = (p->size / sizeof(Spawn_Struct));
		uchar* buffer=(uchar*)p->pBuffer;
		uint32 i;
printf("Processing %d spawns.\n", spawn_count);
		for(i=0;i<spawn_count;i++){
			Spawn_Struct* spawn=(Spawn_Struct*)buffer;
			
			if(name == spawn->name) {
				entity_id = spawn->spawnId;
			}
			
			cm.name = spawn->name;
			cm._class = spawn->class_;
			cm.npctype = spawn->NPC;
			cm.x = EQ19toFloat(spawn->x);
			cm.y = EQ19toFloat(spawn->y);
			cm.z = EQ19toFloat(spawn->z);
			
			mobs[spawn->spawnId] = cm;
			
			buffer+=sizeof(Spawn_Struct);
		}
		break;
	}
//	case OP_MobUpdate:
	case OP_ClientUpdate: {
		uint16 sid;
		float x,y,z;
		if(p->size == sizeof(PlayerPositionUpdateClient_Struct)) {
			PlayerPositionUpdateClient_Struct *pu = (PlayerPositionUpdateClient_Struct *) p->pBuffer;
			sid = pu->spawn_id;
			x = pu->x_pos;
			y = pu->y_pos;
			z = pu->z_pos;
		} else if(p->size == sizeof(PlayerPositionUpdateServer_Struct)) {
			PlayerPositionUpdateServer_Struct *pu = (PlayerPositionUpdateServer_Struct *) p->pBuffer;
			sid = pu->spawn_id;
			x = EQ19toFloat(pu->x_pos);
			y = EQ19toFloat(pu->y_pos);
			z = EQ19toFloat(pu->z_pos);
		/*} else if(p->size == sizeof(SpawnPositionUpdate_Struct)) {
			SpawnPositionUpdate_Struct *pu = (SpawnPositionUpdate_Struct *) p->pBuffer;
			sid = pu->spawn_id;
			x = EQ19toFloat(pu->x);
			y = EQ19toFloat(pu->y);
			z = EQ19toFloat(pu->z);
		*/
		} else {
			break;
		}
		map<uint16, MobInfoStruct>::iterator r = mobs.find(sid);
		if(r == mobs.end())
			break;	//shouldent happen
		r->second.x = x;
		r->second.y = y;
		r->second.z = z;
	}
	case OP_ZoneEntry: {
		if(p->size == sizeof(ClientZoneEntry_Struct)) {
			ClientZoneEntry_Struct *s = (ClientZoneEntry_Struct *) p->pBuffer;
			name = s->char_name;
		} else if(p->size == sizeof(ServerZoneEntry_Struct)) {
			ServerZoneEntry_Struct *s = (ServerZoneEntry_Struct *) p->pBuffer;
			_class = s->player.spawn.class_;
			
			self_info.name = s->player.spawn.name;
			self_info._class = s->player.spawn.class_;
			self_info.npctype = 0;
			self_info.x = EQ19toFloat(s->player.spawn.x);
			self_info.y = EQ19toFloat(s->player.spawn.y);
			self_info.z = EQ19toFloat(s->player.spawn.z);
			
		} else {
			break;
		}
	}
	case OP_SpawnAppearance : {
		SpawnAppearance_Struct *sa = (SpawnAppearance_Struct *) p->pBuffer;
		if(sa->spawn_id == 0 && sa->type == AT_SpawnID) {
			entity_id = sa->parameter;
			
			mobs[entity_id] = self_info;
		}
	}
	default:
		break;
	}
}

const MobInfoStruct *StructMatcher::FindMob(uint16 sid) {
	map<uint16, MobInfoStruct>::iterator r = mobs.find(sid);
	if(r == mobs.end())
		return(NULL);
	return(&r->second);
}

//returns the most recent packet with this opcode
const SMPacket *StructMatcher::GetRecent(EmuOpcode op, uint32 &distance) {
	map<EmuOpcode, SMPacket *>::iterator res = history_cache.find(op);
	if(res != history_cache.end())
		return(res->second);
	
	list<SMPacket *>::iterator cur,end;
	uint32 r;
	SMPacket *p;
	cur = history.begin();
	end = history.end();
	for(r = 0; cur != end && r < distance; r++, cur++) {
		p = *cur;
		if(p == NULL)
			break;	//might want continue here... not sure
		if(p->opcode == op) {
			history_cache[op] = p;
			distance = r;
			return(p);
		}
	}
	return(NULL);
}


const SMPacket *StructMatcher::GetFuture(
	StructMatchProc p, EmuOpcode op, 
	uint16 eq_op, uint32 size) {
	uint32 distance = 300;
	
//NOT DONE AT ALL.
	uint32 r;
	list<SMPacket *>::reverse_iterator 
	cur(future.end()),
	end(future.begin());
	for(r = 0; cur != end && r < distance; r++, cur++) {
		SMPacket *p = *cur;
		
		if(p == NULL)
			break;	//might want continue here... not sure
		if(p->opcode == op) {
//			future_cache[op] = p;
			distance = r;
			return(p);
		}
	}
	return(NULL);
}
	
//fill out our matchers list
void StructMatcher::MapOpcodes() {

//preprocessor assigns the matchers to the opcodes
//raw packets do not match right now
#define IN_C(op, s) \
	if(opcodes->EmuToEQ(op) == 0) \
		matchers[op] = &StructMatcher::Match_##op;
#define IN_Cv(op, s) \
	if(opcodes->EmuToEQ(op) == 0) \
	matchers[op] = &StructMatcher::Match_##op;
#define IN_Cz(op) \
	if(opcodes->EmuToEQ(op) == 0) \
	matchers[op] = &StructMatcher::Match_##op;
#define IN_Cr(op) \
	/*if(opcodes->EmuToEQ(op) == 0) \
	//matchers[op] = &StructMatcher::Match_##op;*/
#define IN(op, s) \
	if(opcodes->EmuToEQ(op) == 0) \
	matchers[op] = &StructMatcher::Match_##op;
#define INv(op, s) \
	if(opcodes->EmuToEQ(op) == 0) \
	matchers[op] = &StructMatcher::Match_##op;
#define INz(op) \
	if(opcodes->EmuToEQ(op) == 0) \
	matchers[op] = &StructMatcher::Match_##op;
#define INr(op) \
	/*if(opcodes->EmuToEQ(op) == 0) \
	//matchers[op] = &StructMatcher::Match_##op;*/
#define OUT_C(op, s) \
	if(opcodes->EmuToEQ(op) == 0) \
	matchers[op] = &StructMatcher::Match_##op;
#define OUT_Cv(op, s) \
	if(opcodes->EmuToEQ(op) == 0) \
	matchers[op] = &StructMatcher::Match_##op;
#define OUT_Cz(op) \
	if(opcodes->EmuToEQ(op) == 0) \
	matchers[op] = &StructMatcher::Match_##op;
#define OUT_Cr(op) \
	/*if(opcodes->EmuToEQ(op) == 0) \
	//matchers[op] = &StructMatcher::Match_##op;*/
#define OUT(op, s) \
	if(opcodes->EmuToEQ(op) == 0) \
	matchers[op] = &StructMatcher::Match_##op;
#define OUTv(op, s) \
	if(opcodes->EmuToEQ(op) == 0) \
	matchers[op] = &StructMatcher::Match_##op;
#define OUTz(op) \
	if(opcodes->EmuToEQ(op) == 0) \
	matchers[op] = &StructMatcher::Match_##op;
#define OUTr(op) \
	/*if(opcodes->EmuToEQ(op) == 0) \
	//matchers[op] = &StructMatcher::Match_##op;*/
#include "../common/opcode_dispatch.h"
#undef IN_C
#undef IN_Cr
#undef IN_Cv
#undef IN_Cz
#undef IN
#undef INr
#undef INv
#undef INz
#undef OUT_C
#undef OUT_Cr
#undef OUT_Cv
#undef OUT_Cz
#undef OUT
#undef OUTr
#undef OUTv
#undef OUTz
}













