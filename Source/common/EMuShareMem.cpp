#include <iostream>
using namespace std;
#include "../common/types.h"
#include "../common/debug.h"
#include "EMuShareMem.h"

#ifdef WIN32
	#define snprintf	_snprintf
	#define vsnprintf	_vsnprintf
	#define strncasecmp	_strnicmp
	#define strcasecmp  _stricmp

	#define EmuLibName "EMuShareMem"
#else
	#define EmuLibName "libEMuShareMem.so"
#endif

LoadEMuShareMemDLL EMuShareMemDLL;

#ifndef WIN32
int32 LoadEMuShareMemDLL::refCount = 0;
#endif

LoadEMuShareMemDLL::LoadEMuShareMemDLL() {
	ClearFunc();
#ifndef WIN32
    refCountU();
#endif
}

LoadEMuShareMemDLL::~LoadEMuShareMemDLL() {
#ifndef WIN32
    if (refCountD() <= 0) {
#endif
	Unload();
#ifndef WIN32
    }
#endif
}

bool LoadEMuShareMemDLL::Load() {
	if(!SharedLibrary::Load(EmuLibName))
		return(false);
	
	if (Loaded()) {
		Items.GetItem = (DLLFUNC_GetItem) GetSym("GetItem");
		Items.GetItemSerialization = (DLLFUNC_GetItemSerialization) GetSym("GetItemSerialization");
		Items.IterateItems = (DLLFUNC_IterateItems) GetSym("IterateItems");
		Items.cbAddItem = (DLLFUNC_AddItem) GetSym("AddItem");
		Items.DLLLoadItems = (DLLFUNC_DLLLoadItems) GetSym("DLLLoadItems");
		Doors.GetDoor = (DLLFUNC_GetDoor) GetSym("GetDoor");
		Doors.cbAddDoor = (DLLFUNC_AddDoor) GetSym("AddDoor");
		Doors.DLLLoadDoors = (DLLFUNC_DLLLoadDoors) GetSym("DLLLoadDoors");
		Spells.DLLLoadSPDat = (DLLFUNC_DLLLoadSPDat) GetSym("DLLLoadSPDat");
		NPCFactionList.DLLLoadNPCFactionLists = (DLLFUNC_DLLLoadNPCFactionLists) GetSym("DLLLoadNPCFactionLists");
		NPCFactionList.GetNPCFactionList = (DLLFUNC_GetNPCFactionList) GetSym("GetNPCFactionList");
		NPCFactionList.cbAddNPCFactionList = (DLLFUNC_AddNPCFactionList) GetSym("AddNPCFactionList");
		NPCFactionList.cbSetFaction = (DLLFUNC_SetFaction) GetSym("SetNPCFaction");
		Loot.DLLLoadLoot = (DLLFUNC_DLLLoadLoot) GetSym("DLLLoadLoot");
		Loot.cbAddLootTable = (DLLFUNC_AddLootTable) GetSym("AddLootTable");
		Loot.cbAddLootDrop = (DLLFUNC_AddLootDrop) GetSym("AddLootDrop");
		Loot.GetLootTable = (DLLFUNC_GetLootTable) GetSym("GetLootTable");
		Loot.GetLootDrop = (DLLFUNC_GetLootDrop) GetSym("GetLootDrop");
		Opcodes.GetEQOpcode = (DLLFUNC_GetEQOpcode) GetSym("GetEQOpcode");
		Opcodes.GetEmuOpcode = (DLLFUNC_GetEmuOpcode) GetSym("GetEmuOpcode");
		Opcodes.SetOpcodePair = (DLLFUNC_SetOpcodePair) GetSym("SetOpcodePair");
		Opcodes.DLLLoadOpcodes = (DLLFUNC_DLLLoadOpcodes) GetSym("DLLLoadOpcodes");
		Opcodes.ClearEQOpcodes = (DLLFUNC_ClearEQOpcodes) GetSym("ClearEQOpcodes");
		if ((!Items.GetItem)
			|| (!Items.GetItemSerialization)
			|| (!Items.IterateItems)
			|| (!Items.cbAddItem)
			|| (!Items.DLLLoadItems)
			|| (!Doors.GetDoor)
			|| (!Doors.cbAddDoor)
			|| (!Doors.DLLLoadDoors)
			|| (!Spells.DLLLoadSPDat)
			|| (!NPCFactionList.DLLLoadNPCFactionLists)
			|| (!NPCFactionList.GetNPCFactionList)
			|| (!NPCFactionList.cbAddNPCFactionList)
			|| (!NPCFactionList.cbSetFaction)
			|| (!Loot.DLLLoadLoot)
			|| (!Loot.cbAddLootTable)
			|| (!Loot.cbAddLootDrop)
			|| (!Loot.GetLootTable)
			|| (!Loot.GetLootDrop)
			|| (!Opcodes.GetEQOpcode)
			|| (!Opcodes.GetEmuOpcode)
			|| (!Opcodes.SetOpcodePair)
			|| (!Opcodes.DLLLoadOpcodes)
			|| (!Opcodes.ClearEQOpcodes)
		) {
			const char *err;
			if((err = GetError()) != NULL) {
				LogFile->write(EQEMuLog::Error, "Function Attach Error: %s\n", err);
			}
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach a function.");
			return false;
		}
		
		LogFile->write(EQEMuLog::Status, "%s loaded", EmuLibName);
		return true;
	}
	else {
		LogFile->write(EQEMuLog::Error, "%s was not loaded, but did not report an error.", EmuLibName);
	}
	return false;
}

void LoadEMuShareMemDLL::Unload() {
	ClearFunc();
	SharedLibrary::Unload();
}

void LoadEMuShareMemDLL::ClearFunc() {
	Items.GetItem = 0;
	Items.GetItemSerialization = 0;
	Items.IterateItems = 0;
	Items.cbAddItem = 0;
	Items.DLLLoadItems = 0;
	Doors.GetDoor = 0;
	Doors.cbAddDoor = 0;
	Doors.DLLLoadDoors = 0;
	NPCFactionList.DLLLoadNPCFactionLists = 0;
	NPCFactionList.GetNPCFactionList = 0;
	NPCFactionList.cbAddNPCFactionList = 0;
	NPCFactionList.cbSetFaction = 0;
	Loot.DLLLoadLoot = 0;
	Loot.cbAddLootTable = 0;
	Loot.cbAddLootDrop = 0;
	Loot.GetLootTable = 0;
	Loot.GetLootDrop = 0;
	Opcodes.GetEQOpcode = NULL;
	Opcodes.GetEmuOpcode = NULL;
	Opcodes.SetOpcodePair = NULL;
	Opcodes.DLLLoadOpcodes = NULL;
	Opcodes.ClearEQOpcodes = NULL;
}
