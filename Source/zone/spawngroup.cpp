/*  EQEMu:  Everquest Server Emulator
Copyright (C) 2001-2002  EQEMu Development Team (http://eqemu.org)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.
  
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
	
	  You should have received a copy of the GNU General Public License
	  along with this program; if not, write to the Free Software
	  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "../common/debug.h"
#include "spawngroup.h"
#include <string.h>
#include <stdlib.h>
#include <iostream>
using namespace std;
#include "../common/types.h"
#include "../common/MiscFunctions.h"

SpawnEntry::SpawnEntry( uint32 in_NPCType, int in_chance ) 
{
	NPCType = in_NPCType;
	chance = in_chance;
}

SpawnGroup::SpawnGroup( uint32 in_id, char* name ) {
	id = in_id;
	strncpy( name_, name, 120);
}

uint32 SpawnGroup::GetNPCType() {
#if EQDEBUG >= 10
	LogFile->write(EQEMuLog::Debug, "SpawnGroup[%08x]::GetNPCType()", (int32) this);
#endif
	int npcType = -1;
	int totalchance = 0;
	
	list<SpawnEntry*>::iterator cur,end;
	cur = list_.begin();
	end = list_.end();
	for(; cur != end; cur++) {
		totalchance += (*cur)->chance;
	}
	sint32 roll = 0;
	if(totalchance != 0)
		roll = MakeRandomInt(0, totalchance);
	else
		return 0;
	
	cur = list_.begin();
	for(; cur != end; cur++) {
		if (roll < (*cur)->chance) {
			npcType = (*cur)->NPCType;
			break;
		}
		else {
			roll -= (*cur)->chance;
		}
	}
	//CODER  implement random table
	return npcType;
}

void SpawnGroup::AddSpawnEntry( SpawnEntry* newEntry ) {
	list_.push_back( newEntry );
}

SpawnGroup::~SpawnGroup() {
	list<SpawnEntry*>::iterator cur,end;
	cur = list_.begin();
	end = list_.end();
	for(; cur != end; cur++) {
		SpawnEntry* tmp = *cur;
		safe_delete(tmp);
	}
	list_.clear();
}

SpawnGroupList::~SpawnGroupList() {
	map<uint32, SpawnGroup*>::iterator cur,end;
	cur = groups.begin();
	end = groups.end();
	for(; cur != end; cur++) {
		SpawnGroup* tmp = cur->second;
		safe_delete(tmp);
	}
	groups.clear();
}

void SpawnGroupList::AddSpawnGroup(SpawnGroup* newGroup) {
	if(newGroup == NULL)
		return;
	groups[newGroup->id] = newGroup;
}

SpawnGroup* SpawnGroupList::GetSpawnGroup(uint32 in_id) {
	if(groups.count(in_id) != 1)
		return(false);
	return(groups[in_id]);
}

bool SpawnGroupList::RemoveSpawnGroup(uint32 in_id) {
	if(groups.count(in_id) != 1)
		return(false);
	
	groups.erase(in_id);
	return(true);
}

