
#include <stdio.h>
#include "opcodemgr.h"
#include "debug.h"
#include "emu_opcodes.h"

#ifdef SHARED_OPCODES
#include "EMuShareMem.h"
extern LoadEMuShareMemDLL EMuShareMemDLL;
#endif

#include <map>
#include <string>
using namespace std;


//#define DEBUG_TRANSLATE

//TODO: remove this when we change to new flagless code
#define StripFlags(x) (x&0x0FFF)


OpcodeManager::OpcodeManager() {
	loaded = false;
}

bool OpcodeManager::LoadOpcodesFile(const char *filename, OpcodeSetStrategy *s) {
	FILE *opf = fopen(filename, "r");
	if(opf == NULL) {
		LogFile->write(EQEMuLog::Error, "Unable to open opcodes file '%s'. Thats bad.", filename);
		return(false);
	}
	
	map<string, uint16> eq;
	
	//load the opcode file into eq, could swap in a nice XML parser here
	char line[2048];
	int lineno = 0;
	uint16 curop;
	while(!feof(opf)) {
		lineno++;
		line[0] = '\0';	//for blank line at end of file
		if(fgets(line, sizeof(line), opf) == NULL)
			break;
		
		//ignore any line that dosent start with OP_
		if(line[0] != 'O' || line[1] != 'P' || line[2] != '_')
			continue;
		
		char *num = line+3;	//skip OP_
		//look for the = sign
		while(*num != '=' && *num != '\0') {
			num++;
		}
		//make sure we found =
		if(*num != '=') {
			LogFile->write(EQEMuLog::Error, "Malformed opcode line at %s:%d", filename, lineno);
			continue;
		}
		*num = '\0';	//null terminate the name
		num++;			//num should point to the opcode
		
		//read the opcode
		if(sscanf(num, "0x%hx", &curop) != 1) {
			LogFile->write(EQEMuLog::Error, "Malformed opcode at %s:%d", filename, lineno);
			continue;
		}
		
		//we have a name and our opcode... stick it in the map
		eq[line] = curop;
	}
	fclose(opf);
	
	
	//do the mapping and store them in the shared memory array
	bool ret = true;
	EmuOpcode emu_op;
	map<string, uint16>::iterator res;
	//stupid enum wont let me ++ on it...
	for(emu_op = OP_Unknown; emu_op < _maxEmuOpcode; emu_op=(EmuOpcode)(emu_op+1)) {
		//get the name of this emu opcode
		const char *op_name = OpcodeNames[emu_op];
		
		//find the opcode in the file
		res = eq.find(op_name);
		if(res == eq.end()) {
			LogFile->write(EQEMuLog::Error, "Opcode %s is missing from %s", op_name, filename);
			ret = false;
			continue;	//continue to give them a list of all missing opcodes
		}
		
		//ship the mapping off to shared mem.
		s->Set(emu_op, res->second);
	}
	
	return(ret);
}

//convenience routines
const char *OpcodeManager::EmuToName(const EmuOpcode emu_op) {
	return(OpcodeNames[emu_op]);
}

const char *OpcodeManager::EQToName(const uint16 eq_op) {
	//first must resolve the eq op to an emu op
	EmuOpcode emu_op = EQToEmu(StripFlags(eq_op));
	return(OpcodeNames[emu_op]);
}

#ifdef SHARED_OPCODES
bool SharedOpcodeManager::LoadOpcodes(const char *filename) {
	if (!EMuShareMemDLL.Load())
		return false;
	MOpcodes.lock();
	
	loaded = true;
	
	bool ret = EMuShareMemDLL.Opcodes.DLLLoadOpcodes(DLLLoadOpcodesCallback, sizeof(uint16), MAX_EQ_OPCODE, _maxEmuOpcode, filename);
	
	MOpcodes.unlock();
	return ret;
}

bool SharedOpcodeManager::DLLLoadOpcodesCallback(const char *filename) {
	SharedMemStrategy s;
	return(LoadOpcodesFile(filename, &s));
}

bool SharedOpcodeManager::ReloadOpcodes(const char *filename) {
	if(!loaded)
		return(LoadOpcodes(filename));
	
	MOpcodes.lock();
	EMuShareMemDLL.Opcodes.ClearEQOpcodes();
	
	SharedMemStrategy s;
	bool ret = LoadOpcodesFile(filename, &s);
	
	MOpcodes.unlock();
	return(ret);
}


uint16 SharedOpcodeManager::EmuToEQ(const EmuOpcode emu_op) {
	//opcode is checked for validity in GetEQOpcode
	uint16 res;
	MOpcodes.lock();
	res = EMuShareMemDLL.Opcodes.GetEQOpcode((uint16)emu_op);
	MOpcodes.unlock();
#ifdef DEBUG_TRANSLATE
	LogFile->write(EQEMuLog::Debug, "S Translate Emu %s (%d) to EQ 0x%.4x", OpcodeNames[emu_op], emu_op, res);
#endif
	return(res);
}

EmuOpcode SharedOpcodeManager::EQToEmu(const uint16 eq_op) {
	//opcode is checked for validity in GetEmuOpcode
	if(eq_op > MAX_EQ_OPCODE)
		return(OP_Unknown);
	uint16 res;
	MOpcodes.lock();
	res = EMuShareMemDLL.Opcodes.GetEmuOpcode(eq_op);
	MOpcodes.unlock();
#ifdef DEBUG_TRANSLATE
	LogFile->write(EQEMuLog::Debug, "S Translate EQ 0x%.4x to Emu %s (%d)", eq_op, OpcodeNames[res], res);
#endif
	return((EmuOpcode)res);
}


void SharedOpcodeManager::SharedMemStrategy::Set(EmuOpcode emu_op, uint16 eq_op) {
	EMuShareMemDLL.Opcodes.SetOpcodePair((uint16)emu_op, eq_op);
}
#endif


RegularOpcodeManager::RegularOpcodeManager()
: OpcodeManager()
{
	emu_to_eq = NULL;
	eq_to_emu = NULL;
}

RegularOpcodeManager::~RegularOpcodeManager() {
	safe_delete(emu_to_eq);
	safe_delete(eq_to_emu);
}

bool RegularOpcodeManager::LoadOpcodes(const char *filename) {
	NormalMemStrategy s;
	MOpcodes.lock();
	
	loaded = true;
	eq_to_emu = new EmuOpcode[MAX_EQ_OPCODE];
	emu_to_eq = new uint16[_maxEmuOpcode];
	
	//dont need to set eq_to_emu cause every element should get a value
	memset(eq_to_emu, 0, sizeof(uint16)*MAX_EQ_OPCODE);
	
	bool ret = LoadOpcodesFile(filename, &s);
	MOpcodes.unlock();
	return ret;
}

bool RegularOpcodeManager::ReloadOpcodes(const char *filename) {
	if(!loaded)
		return(LoadOpcodes(filename));
	
	NormalMemStrategy s;
	MOpcodes.lock();
	
	memset(eq_to_emu, 0, sizeof(uint16)*MAX_EQ_OPCODE);
	
	bool ret = LoadOpcodesFile(filename, &s);
	
	MOpcodes.unlock();
	return(ret);
}


uint16 RegularOpcodeManager::EmuToEQ(const EmuOpcode emu_op) {
	//opcode is checked for validity in GetEQOpcode
	uint16 res;
	MOpcodes.lock();
	res = emu_to_eq[emu_op];
	MOpcodes.unlock();
#ifdef DEBUG_TRANSLATE
	LogFile->write(EQEMuLog::Debug, "M Translate Emu %s (%d) to EQ 0x%.4x", OpcodeNames[emu_op], emu_op, res);
#endif
	return(res);
}

EmuOpcode RegularOpcodeManager::EQToEmu(const uint16 eq_op) {
	//opcode is checked for validity in GetEmuOpcode
	if(eq_op > MAX_EQ_OPCODE)
		return(OP_Unknown);
	EmuOpcode res;
	MOpcodes.lock();
	res = eq_to_emu[eq_op];
	MOpcodes.unlock();
#ifdef DEBUG_TRANSLATE
	LogFile->write(EQEMuLog::Debug, "M Translate EQ 0x%.4x to Emu %s (%d)", eq_op, OpcodeNames[res], res);
#endif
	return(res);
}


void RegularOpcodeManager::NormalMemStrategy::Set(EmuOpcode emu_op, uint16 eq_op) {
	if(uint32(emu_op) >= it->EmuOpcodeCount || eq_op >= it->EQOpcodeCount)
		return;
	it->emu_to_eq[emu_op] = eq_op;
	it->eq_to_emu[eq_op] = emu_op;
}









