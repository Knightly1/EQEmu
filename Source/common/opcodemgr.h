
#ifndef OPCODE_MANAGER_H
#define OPCODE_MANAGER_H

#include "types.h"
#include "Mutex.h"
#include "emu_opcodes.h"

//enable the use of shared mem opcodes for world and zone only
#ifdef ZONE
#define SHARED_OPCODES
#endif
#ifdef WORLD
#define SHARED_OPCODES
#endif


class OpcodeManager {
public:
	OpcodeManager();
	virtual ~OpcodeManager() {}
	
	virtual bool LoadOpcodes(const char *filename) = 0;
	virtual bool ReloadOpcodes(const char *filename) = 0;
	
	virtual uint16 EmuToEQ(const EmuOpcode emu_op) = 0;
	virtual EmuOpcode EQToEmu(const uint16 eq_op) = 0;
	
	static const char *EmuToName(const EmuOpcode emu_op);
	const char *EQToName(const uint16 emu_op);
	
protected:
	bool loaded;
	Mutex MOpcodes;	//this only protects the local machine
					//in a shared manager, this dosent protect others
	
	class OpcodeSetStrategy {
	public:
		virtual void Set(EmuOpcode emu_op, uint16 eq_op) = 0;
	};
	
	static bool LoadOpcodesFile(const char *filename, OpcodeSetStrategy *s);
};

#ifdef SHARED_OPCODES	//quick toggle since only world and zone should possibly use this
//keeps opcodes in shared memory
class SharedOpcodeManager : public OpcodeManager {
public:
	virtual ~SharedOpcodeManager() {}

	virtual bool LoadOpcodes(const char *filename);
	virtual bool ReloadOpcodes(const char *filename);
	
	virtual uint16 EmuToEQ(const EmuOpcode emu_op);
	virtual EmuOpcode EQToEmu(const uint16 eq_op);
	
protected:
	class SharedMemStrategy : public OpcodeSetStrategy {
	public:
		void Set(EmuOpcode emu_op, uint16 eq_op);
	};
	static bool DLLLoadOpcodesCallback(const char *filename);
};
#endif //SHARED_OPCODES

//keeps opcodes in regular heap memory
class RegularOpcodeManager : public OpcodeManager {
public:
	RegularOpcodeManager();
	virtual ~RegularOpcodeManager();
	
	virtual bool LoadOpcodes(const char *filename);
	virtual bool ReloadOpcodes(const char *filename);
	
	virtual uint16 EmuToEQ(const EmuOpcode emu_op);
	virtual EmuOpcode EQToEmu(const uint16 eq_op);
protected:
	class NormalMemStrategy : public OpcodeSetStrategy {
	public:
		RegularOpcodeManager *it;
		void Set(EmuOpcode emu_op, uint16 eq_op);
	};
	
	uint16 *emu_to_eq;
	EmuOpcode *eq_to_emu;
	uint32 EQOpcodeCount;
	uint32 EmuOpcodeCount;
};


#endif













