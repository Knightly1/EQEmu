#ifndef STRUCTMATCH_H
#define STRUCTMATCH_H


#define HISTORY_QUEUE_SIZE 100
#define FUTURE_QUEUE_SIZE 100

#include "../common/types.h"
#include "../common/emu_opcodes.h"
#include <math.h>

#include <map>
#include <vector>
#include <string>
#include <list>
using namespace std;

class MutableOpcodeManager;

//quick container class
class SMPacket {
public:
	SMPacket(uint16 eq_op, EmuOpcode op, uint32 len) {
		eq_opcode = eq_op;
		opcode = op;
		size = len;
		if(len > 0)
			pBuffer = new unsigned char[len];
		else
			pBuffer = NULL;
	}
	~SMPacket() {
		if(pBuffer != NULL)
			delete[] pBuffer;
	}
	uint16 eq_opcode;
	EmuOpcode opcode;
	uint32 size;
	unsigned char *pBuffer;
};

class MobInfoStruct {
public:
	string name;
	int _class;
	//most recently seen position
	float x;
	float y;
	float z;
	int npctype;
};


class StructMatcher {
	typedef bool (StructMatcher::*StructMatchProc)(const SMPacket *p);
public:
	StructMatcher(MutableOpcodeManager *op);
	~StructMatcher();
	
	//takes ownership of p
	void Process(SMPacket *p);
	
	//processes any packets in the queues.
	void Finish();
	
protected:
	//takes a packet with a known opcode and uses it to gather all the crap below:
	//takes ownership of p
	void ProcessKnown(SMPacket *p);
	void ProcessUnknown(SMPacket *p);
	
	bool im_sure;
	
	//Personal Information
	//our current position as best as we can tell
	string name;
	uint32 _class;
	uint16 entity_id;
	float x;
	float y;
	float z;
	MobInfoStruct self_info;
	
	//Entity List:
	map<uint16, MobInfoStruct> mobs;
	const MobInfoStruct *FindMob(uint16 id);
	
	//History:
	//a recent packet history for correlating things
	//the stl queue isnt iterable, so I use a list... stupid thing
	list<SMPacket *> history;
	map<EmuOpcode, SMPacket *> history_cache;
	//returns the most recent packet with this opcode
	const SMPacket *GetRecent(EmuOpcode op, uint32 &distance);
	
	//Future:
	list<SMPacket *> future;
	const SMPacket *GetFuture(StructMatchProc p, EmuOpcode op, uint16 eq_op, uint32 size);
	
	//a list of our avaliable struct matching functions
	map<EmuOpcode, StructMatchProc> matchers;
	MutableOpcodeManager *opcodes;
	
	//fill out our matchers list
	void MapOpcodes();

//preprocessor builds the prototypes
//raw packets do not match right now
#define IN_C(op, s) \
	bool Match_##op(const SMPacket *p);
#define IN_Cv(op, s) \
	bool Match_##op(const SMPacket *p);
#define IN_Cz(op) \
	bool Match_##op(const SMPacket *p);
#define IN_Cr(op) \
	//bool Match_##op(const SMPacket *p);
#define IN(op, s) \
	bool Match_##op(const SMPacket *p);
#define INv(op, s) \
	bool Match_##op(const SMPacket *p);
#define INz(op) \
	bool Match_##op(const SMPacket *p);
#define INr(op) \
	//bool Match_##op(const SMPacket *p);
#define OUT_C(op, s) \
	bool Match_##op(const SMPacket *p);
#define OUT_Cv(op, s) \
	bool Match_##op(const SMPacket *p);
#define OUT_Cz(op) \
	bool Match_##op(const SMPacket *p);
#define OUT_Cr(op) \
	//bool Match_##op(const SMPacket *p);
#define OUT(op, s) \
	bool Match_##op(const SMPacket *p);
#define OUTv(op, s) \
	bool Match_##op(const SMPacket *p);
#define OUTz(op) \
	bool Match_##op(const SMPacket *p);
#define OUTr(op) \
	//bool Match_##op(const SMPacket *p);
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
};



#define CheckFloat(f, min, max, mine, maxe) \
	{ float l10; \
	if(f < min || f > max || ( f != 0 && ((l10 = log10(f)) < mine || l10 > maxe))) \
		return(false); \
	}

//string may be empty
#define CheckStringEmpty(str, buflen) \
	{ unsigned char *ptr; uint32 len = buflen-1; \
		for(ptr = (unsigned char *) str; *ptr != '\0'; ptr++, len--) { \
			if(len == 0) return(false);	\
			if((*ptr < 20 || *ptr > 127) && *ptr != '\r' && *ptr != '\n') return(false); \
		} \
	}

//makes sure the string is printable ascii
#define CheckString(str, buflen) \
	if(str[0] == '\0') return(false); \
	CheckStringEmpty(str, buflen);

//string may be empty
#define CheckStringExact(str, buflen) \
	{ unsigned char *ptr; uint32 len = buflen-1; \
		for(ptr = (unsigned char *) str; *ptr != '\0'; ptr++, len--) { \
			if(len == 0) return(false);	\
			if((*ptr < 20 || *ptr > 127) && *ptr != '\r' && *ptr != '\n') return(false); \
		} \
		if(len != 0) return(false); \
	}

//assumes string is valid
#define CheckCharCount(str, c, count) \
	{ unsigned char *ptr; uint32 n = 0; \
		for(ptr = (unsigned char *) str; *ptr != '\0'; ptr++) { \
			if(*ptr == c) n++; \
			if(n >= count) break; \
		} \
		if(n < count) return(false); \
	}

#define CheckNotCharCount(str, c, count) \
	{ unsigned char *ptr; uint32 n = 0; \
		for(ptr = (unsigned char *) str; *ptr != '\0'; ptr++) { \
			if(*ptr == c) n++; \
			if(n >= count) break; \
		} \
		if(n >= count) return(false); \
	}

#define CheckSigned(v, min, max) \
	if(v < min || v > max) return(false);

#define CheckUnsigned(v, min, max) \
	if(v < min || v > max) return(false);

//for unsigned with a lower bound of 0, to shut the compiler up
#define CheckUnsignedLimit(v, max) \
	if(v > max) return(false);

#define CheckBool(v) \
	if(uint32(v) != 0 && uint32(v) != 1) return(false);

//these specific things all assume the int is unsigned and hence >=0
//could actually check the spell file
#define CheckSpellID(id) \
	if(id < 1 || id > 7000) return(false);

//could actually check the database, this is really quit loose
//ignroe qeynos to be more useful
#define CheckZoneID(id) \
	if(id < 1 || id > 1005) return(false);

#define CheckItemID(item_id) \
	if(item_id < 1000 || item_id > MMF_EQMAX_ITEMS) return(false);

#define CheckSkillID(id) \
	if(id > HIGHEST_SKILL) return(false);

//lots of room to be more specific in this one:
#define CheckItemSlot(slot) \
	if(slot > 3179) return(false);

//could improve this to use the map
#define CheckHCoord(v) \
	CheckFloat(v, -720, 720, -3, 3);
#define CheckCoord(v) \
	CheckFloat(v, -10000, 10000, -5, 5);
#define CheckZCoord(v) \
	CheckFloat(v, -1000, 3000, -5, 5);
#define CheckCoord19(v) \
	{ float val = EQ19toFloat(v); \
		CheckCoord(val); }
#define CheckZCoord19(v) \
	{ float val = EQ19toFloat(v); \
		CheckZCoord(val); }
#define CheckHCoord13(v) \
	{ float val = EQ13toFloat(v); \
		CheckHCoord(val); }

//const SMPacket *old;
//int maxdist = CLOSE;
//takes a const SMPacket * in var and leaves the packet in it
//same for the maxdist var, takes the name of a int var
#define CheckHistory(opcode, maxdist, var) \
	{	var = GetRecent(opcode, maxdist); \
		if(var == NULL) return(false); }
		
#define CheckHistoryExists(opcode, maxdist) \
	{	uint32 md = maxdist; \
		const SMPacket *var = GetRecent(opcode, md); \
		if(var == NULL) return(false); }
		
//CheckHistoryExists but looks for two opcodes
#define CheckHistoryExists2(opcode, opcode2, maxdist) \
	{	uint32 md = maxdist; \
		const SMPacket *var = GetRecent(opcode, md); \
		md = maxdist; \
		const SMPacket *var2 = GetRecent(opcode2, md); \
		if(var == NULL && var2 == NULL) return(false); }
		
#define CheckFutureExists(opcode, maxdist) \
	/*{	uint32 md = maxdist; \
		const SMPacket *var = GetRecent(opcode, md); \
		if(var == NULL) return(false); }*/




#define CheckSelf(eid) \
		if(eid != entity_id) return(false);


#define CheckMobExists(entity_id) \
	{	const MobInfoStruct *mob = FindMob(entity_id); \
		if(mob == NULL) return(false); }

//const CollectMobStruct *mob;
//takes a const CollectMobStruct* in var and leavs the mob info in it
#define CheckMob(entity_id, var) \
	var = FindMob(entity_id); \
	if(var == NULL) return(false);

#define CheckMobClass(entity_id, cla) \
	{	const MobInfoStruct *mob = FindMob(entity_id); \
		if(mob == NULL) return(false); \
		if(mob->_class != cla) return(false); }

#define CheckPlayer(entity_id) \
	{	const MobInfoStruct *mob = FindMob(entity_id); \
		if(mob == NULL) return(false); \
		if(mob->npctype != 0) return(false); }

#define CheckNPC(entity_id) \
	{	const MobInfoStruct *mob = FindMob(entity_id); \
		if(mob == NULL) return(false); \
		if(mob->npctype != 1) return(false); }

#define CheckCorpse(entity_id) \
	{	const MobInfoStruct *mob = FindMob(entity_id); \
		if(mob == NULL) return(false); \
		if(mob->npctype != 2 && mob->npctype != 3) return(false); }

#endif












