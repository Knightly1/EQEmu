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

	#include "../common/unix.h"
	#include <dlfcn.h>
    #define GetProcAddress(a,b) dlsym(a,b)
	#define LoadLibrary(a) dlopen(a, RTLD_NOW) 
	#define  FreeLibrary(a) dlclose(a)
	#define GetLastError() dlerror()
#endif

LoadEMuShareMemDLL EMuShareMemDLL;

#ifndef WIN32
int32 LoadEMuShareMemDLL::refCount = 0;
#endif

LoadEMuShareMemDLL::LoadEMuShareMemDLL() {
	hDLL = 0;
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
#ifdef WIN32
	DWORD load_error = 0;
	SetLastError(0);
#else
	const char* load_error = 0;
#endif
	if (Loaded())
	{
		return true;
	}
	hDLL = LoadLibrary(EmuLibName);
#ifdef WIN32
	if(!hDLL) {
		load_error = GetLastError();
		LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to load library '%s'.  Error=%i", EmuLibName, load_error);
	    return false;
	}
    else { SetLastError(0); } // Clear the win9x error
#else
	if(!hDLL || ((load_error = GetLastError()) != NULL) ) {
		LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to load library '%s'.  Error=%s", EmuLibName, load_error?load_error:"Null Return, no error");
	    return false;
	}
#endif
	
	if (Loaded()) {
		Items.GetItem = (DLLFUNC_GetItem) GetProcAddress(hDLL, "GetItem");
		Items.IterateItems = (DLLFUNC_IterateItems) GetProcAddress(hDLL, "IterateItems");
		Items.cbAddItem = (DLLFUNC_AddItem) GetProcAddress(hDLL, "AddItem");
		Items.DLLLoadItems = (DLLFUNC_DLLLoadItems) GetProcAddress(hDLL, "DLLLoadItems");
		Doors.GetDoor = (DLLFUNC_GetDoor) GetProcAddress(hDLL, "GetDoor");
		Doors.cbAddDoor = (DLLFUNC_AddDoor) GetProcAddress(hDLL, "AddDoor");
		Doors.DLLLoadDoors = (DLLFUNC_DLLLoadDoors) GetProcAddress(hDLL, "DLLLoadDoors");
		Spells.DLLLoadSPDat = (DLLFUNC_DLLLoadSPDat) GetProcAddress(hDLL, "DLLLoadSPDat");
		NPCFactionList.DLLLoadNPCFactionLists = (DLLFUNC_DLLLoadNPCFactionLists) GetProcAddress(hDLL, "DLLLoadNPCFactionLists");
		NPCFactionList.GetNPCFactionList = (DLLFUNC_GetNPCFactionList) GetProcAddress(hDLL, "GetNPCFactionList");
		NPCFactionList.cbAddNPCFactionList = (DLLFUNC_AddNPCFactionList) GetProcAddress(hDLL, "AddNPCFactionList");
		NPCFactionList.cbSetFaction = (DLLFUNC_SetFaction) GetProcAddress(hDLL, "SetNPCFaction");
		Loot.DLLLoadLoot = (DLLFUNC_DLLLoadLoot) GetProcAddress(hDLL, "DLLLoadLoot");
		Loot.cbAddLootTable = (DLLFUNC_AddLootTable) GetProcAddress(hDLL, "AddLootTable");
		Loot.cbAddLootDrop = (DLLFUNC_AddLootDrop) GetProcAddress(hDLL, "AddLootDrop");
		Loot.GetLootTable = (DLLFUNC_GetLootTable) GetProcAddress(hDLL, "GetLootTable");
		Loot.GetLootDrop = (DLLFUNC_GetLootDrop) GetProcAddress(hDLL, "GetLootDrop");
		Opcodes.GetEQOpcode = (DLLFUNC_GetEQOpcode) GetProcAddress(hDLL, "GetEQOpcode");
		Opcodes.GetEmuOpcode = (DLLFUNC_GetEmuOpcode) GetProcAddress(hDLL, "GetEmuOpcode");
		Opcodes.SetOpcodePair = (DLLFUNC_SetOpcodePair) GetProcAddress(hDLL, "SetOpcodePair");
		Opcodes.DLLLoadOpcodes = (DLLFUNC_DLLLoadOpcodes) GetProcAddress(hDLL, "DLLLoadOpcodes");
		Opcodes.ClearEQOpcodes = (DLLFUNC_ClearEQOpcodes) GetProcAddress(hDLL, "ClearEQOpcodes");
		if ((!Items.GetItem)
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
#ifndef WIN32
			|| ((load_error = GetLastError()) != NULL)
#else
			&& ((load_error = GetLastError()) != 0)
#endif
			) {
#ifdef WIN32
			load_error = GetLastError();
#endif
			Unload();
			LogFile->write(EQEMuLog::Error, "LoadEMuShareMemDLL::Load() failed to attach a function.  Error=%i", load_error);
			return false;
		}
		
		LogFile->write(EQEMuLog::Status, "%s loaded", EmuLibName);
		return true;
	}
	else {
#ifdef WIN32
		if ((load_error = GetLastError()) != 0)
#else
		if ((load_error = GetLastError()) != NULL)
#endif 
			LogFile->write(EQEMuLog::Error, "LoadLibrary() FAILED!  Error=%i", load_error);
		else
			LogFile->write(EQEMuLog::Error, "LoadLibrary() FAILED!  Error=(unknown)", load_error);
	}
	return false;
}

void LoadEMuShareMemDLL::Unload() {
	ClearFunc();
	if (this->hDLL) {
		FreeLibrary(this->hDLL);
#ifndef WIN32
		const char* error;
		if ((error = GetLastError()) != NULL)
			LogFile->write(EQEMuLog::Error, "FreeLibrary() error = %s", error);
#endif
	}
	hDLL = 0;
}

void LoadEMuShareMemDLL::ClearFunc() {
	Items.GetItem = 0;
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
