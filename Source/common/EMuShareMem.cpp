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
		GuildList.GetGuild = (DLLFUNC_GetGuild) GetSym("GetGuild");
		GuildList.GetMaxGuildID = (DLLFUNC_GetMaxGuildID) GetSym("GetMaxGuildID");
		GuildList.cbAddGuild = (DLLFUNC_AddGuild) GetSym("AddGuild");
		GuildList.DLLLoadGuildList = (DLLFUNC_DLLLoadGuildList) GetSym("DLLLoadGuildList");
		if(Items.GetItem == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach Items.GetItem");
			return(false);
		}

		if(Items.GetItemSerialization == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach Items.GetItemSerialization");
			return(false);
		}

		if(Items.IterateItems == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach Items.IterateItems");
			return(false);
		}

		if(Items.cbAddItem == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach Items.cbAddItem");
			return(false);
		}

		if(Items.DLLLoadItems == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach Items.DLLLoadItems");
			return(false);
		}

		if(Doors.GetDoor == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach Doors.GetDoor");
			return(false);
		}

		if(Doors.cbAddDoor == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach Doors.cbAddDoor");
			return(false);
		}

		if(Doors.DLLLoadDoors == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach Doors.DLLLoadDoors");
			return(false);
		}

		if(Spells.DLLLoadSPDat == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach Spells.DLLLoadSPDat");
			return(false);
		}

		if(NPCFactionList.DLLLoadNPCFactionLists == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach NPCFactionList.DLLLoadNPCFactionLists");
			return(false);
		}

		if(NPCFactionList.GetNPCFactionList == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach NPCFactionList.GetNPCFactionList");
			return(false);
		}

		if(NPCFactionList.cbAddNPCFactionList == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach NPCFactionList.cbAddNPCFactionList");
			return(false);
		}

		if(NPCFactionList.cbSetFaction == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach NPCFactionList.cbSetFaction");
			return(false);
		}

		if(Loot.DLLLoadLoot == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach Loot.DLLLoadLoot");
			return(false);
		}

		if(Loot.cbAddLootTable == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach Loot.cbAddLootTable");
			return(false);
		}

		if(Loot.cbAddLootDrop == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach Loot.cbAddLootDrop");
			return(false);
		}

		if(Loot.GetLootTable == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach Loot.GetLootTable");
			return(false);
		}

		if(Loot.GetLootDrop == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach Loot.GetLootDrop");
			return(false);
		}

		if(Opcodes.GetEQOpcode == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach Opcodes.GetEQOpcode");
			return(false);
		}

		if(Opcodes.GetEmuOpcode == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach Opcodes.GetEmuOpcode");
			return(false);
		}

		if(Opcodes.SetOpcodePair == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach Opcodes.SetOpcodePair");
			return(false);
		}

		if(Opcodes.DLLLoadOpcodes == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach Opcodes.DLLLoadOpcodes");
			return(false);
		}

		if(Opcodes.ClearEQOpcodes == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach Opcodes.ClearEQOpcodes");
			return(false);
		}
		if(GuildList.GetGuild == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach GuildList.GetGuild");
			return(false);
		}
		if(GuildList.GetMaxGuildID == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach GuildList.GetMaxGuildID");
			return(false);
		}
		if(GuildList.cbAddGuild == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach GuildList.cbAddGuild");
			return(false);
		}
		if(GuildList.DLLLoadGuildList == NULL) {
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach GuildList.DLLLoadGuildList");
			return(false);
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
	GuildList.GetMaxGuildID = NULL;
	GuildList.GetGuild = 0;
	GuildList.cbAddGuild = 0;
	GuildList.DLLLoadGuildList = 0;
}
