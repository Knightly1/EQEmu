
#include "../debug.h"
#include "Live.h"
#include "../opcodemgr.h"
#include "../logsys.h"
#include "../EQStreamIdent.h"
#include "../crc32.h"

#include "../eq_packet_structs.h"
#include "../MiscFunctions.h"
#include "../Item.h"
#include "Live_structs.h"

namespace Live {

static const char *name = "Live";
static OpcodeManager *opcodes = NULL;
static Strategy struct_strategy;

//char *SerializeItem(ItemInst *inst, sint16 slot_id, uint32 *length, uint8 depth);
	
void Register(EQStreamIdentifier &into) {
	//create our opcode manager if we havent already
	if(opcodes == NULL) {
		//TODO: get this file name from the config file
		string opfile = "patch_";
		opfile += name;
		opfile += ".conf";
		//load up the opcode manager.
		//TODO: figure out how to support shared memory with multiple patches...
		opcodes = new RegularOpcodeManager();
		if(!opcodes->LoadOpcodes(opfile.c_str())) {
			_log(NET__OPCODES, "Error loading opcodes file %s. Not registering patch %s.", opfile.c_str(), name);
			return;
		}
	}
	
	//ok, now we have what we need to register.
	
	EQStream::Signature signature;
	string pname;
	
	//register our world signature.
	pname = string(name) + "_world";
	signature.ignore_eq_opcode = 0;
	signature.first_length = sizeof(structs::LoginInfo_Struct);
	signature.first_eq_opcode = opcodes->EmuToEQ(OP_SendLoginInfo);
	into.RegisterPatch(signature, pname.c_str(), &opcodes, &struct_strategy);
	
	//register our zone signature.
	pname = string(name) + "_zone";
	signature.ignore_eq_opcode = opcodes->EmuToEQ(OP_AckPacket);
	signature.first_length = sizeof(structs::ClientZoneEntry_Struct);
	signature.first_eq_opcode = opcodes->EmuToEQ(OP_ZoneEntry);
	into.RegisterPatch(signature, pname.c_str(), &opcodes, &struct_strategy);

	_log(NET__IDENTIFY, "Registered patch %s", name);
}

void Reload() {
	
	//we have a big problem to solve here when we switch back to shared memory
	//opcode managers because we need to change the manager pointer, which means
	//we need to go to every stream and replace it's manager.
	
	if(opcodes != NULL) {
		//TODO: get this file name from the config file
		string opfile = "patch_";
		opfile += name;
		opfile += ".conf";
		if(!opcodes->ReloadOpcodes(opfile.c_str())) {
			_log(NET__OPCODES, "Error reloading opcodes file %s for patch %s.", opfile.c_str(), name);
			return;
		}
		_log(NET__OPCODES, "Reloaded opcodes for patch %s", name);
	}
}



Strategy::Strategy()
: Titanium::Strategy()
{
	//all opcodes default to passthrough.
	#include "SSRegister.h"
	#include "Live_ops.h"
}

std::string Strategy::Describe() const {
	std::string r;
	r += "Patch ";
	r += name;
	return(r);
}


#include "SSDefine.h"


/*ENCODE(OP_PlayerProfile) {
	SETUP_DIRECT(PlayerProfile_Struct, structs::PlayerProfile_Struct);
	
	
	
	FINISH_DIRECT();
}

ENCODE(OP_NewZone) {
	SETUP_DIRECT(PlayerProfile_Struct, structs::PlayerProfile_Struct);
	
	
	
	FINISH_DIRECT();
}*/

/*
ENCODE(OP_SendAATable) {
	SETUP_DIRECT_ENCODE(SendAA_Struct, structs::SendAA_Struct);
	OUT(id);
	OUT(hotkey_sid);
	OUT(hotkey_sid2);
	eq->title_sid = emu->id;
	eq->desc_sid = emu->id;
	OUT(class_type);
	OUT(cost);
	OUT(seq);
	OUT(current_level);
	OUT(prereq_skill);
	OUT(prereq_minpoints);
	OUT(type);
	OUT(spellid);
	OUT(spell_type);
	OUT(spell_refresh);
	OUT(classes);
	OUT(berserker);
	OUT(max_level);
	OUT(last_id);
	OUT(next_id);
	OUT(cost2);
	OUT(unknown80[0]);
	OUT(unknown80[1]);
	OUT(total_abilities);
	unsigned int r;
	for(r = 0; r < emu->total_abilities; r++) {
		OUT(abilities[r].skill_id);
		OUT(abilities[r].increase_amt);
		OUT(abilities[r].unknown08);
		OUT(abilities[r].last_level);
	}
	FINISH_DIRECT_ENCODE();
}

DECODE(OP_SetServerFilter) {
	SETUP_DIRECT_DECODE(SetServerFilter_Struct, structs::SetServerFilter_Struct);
	int r;
	for(r = 0; r < 25; r++) {
		IN(filters[r]);
	}
	emu->filters[25] = 1;
	FINISH_DIRECT_DECODE();
}
*/

ENCODE(OP_SendCharInfo) {
	ENCODE_LENGTH_EXACT(CharacterSelect_Struct);
	SETUP_DIRECT_ENCODE(CharacterSelect_Struct, structs::CharacterSelect_Struct);
	int r;
	for(r = 0; r < 10; r++) {
		OUT(zone[r]);
		OUT(eyecolor1[r]);
		OUT(eyecolor2[r]);
		OUT(hair[r]);
		OUT(primary[r]);
		OUT(race[r]);
		OUT(class_[r]);
		OUT_str(name[r]);
		OUT(gender[r]);
		OUT(level[r]);
		OUT(secondary[r]);
		OUT(face[r]);
		OUT(beard[r]);
		int k;
		for(k = 0; k < 9; k++) {
			OUT(equip[r][k]);
			OUT(cs_colors[r][k].color);
		}
		OUT(haircolor[r]);
		OUT(gohome[r]);
		OUT(deity[r]);
		OUT(beardcolor[r]);
	}
	FINISH_ENCODE();
}

/*ENCODE(OP_ExpansionInfo) {
	ENCODE_LENGTH_EXACT(ExpansionInfo_Struct);
	SETUP_DIRECT_ENCODE(ExpansionInfo_Struct, structs::ExpansionInfo_Struct);
	OUT(Expansions);
	FINISH_ENCODE();
}*/



















} //end namespace Live






