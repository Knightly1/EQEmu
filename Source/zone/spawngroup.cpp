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
#include "entity.h"
#include <string.h>
#include <stdlib.h>
#include <iostream>
using namespace std;
#include "../common/types.h"
#include "../common/database.h"
#include "../common/MiscFunctions.h"

extern EntityList entity_list;

SpawnEntry::SpawnEntry( uint32 in_NPCType, int in_chance, uint8 in_group_spawn_limit, uint8 in_npc_spawn_limit ) 
{
	NPCType = in_NPCType;
	chance = in_chance;
	group_spawn_limit = in_group_spawn_limit;
	npc_spawn_limit = in_npc_spawn_limit;
}

SpawnGroup::SpawnGroup( uint32 in_id, char* name ) {
	id = in_id;
	strncpy( name_, name, 120);
}

uint32 SpawnGroup::GetNPCType() {
#if EQDEBUG >= 10
	LogFile->write(EQEMuLog::Debug, "SpawnGroup[%08x]::GetNPCType()", (int32) this);
#endif
	int npcType = 0;
	int totalchance = 0;
	
	list<SpawnEntry*>::iterator cur,end;
	list<SpawnEntry*> possible;
	cur = list_.begin();
	end = list_.end();
	for(; cur != end; cur++) {
		SpawnEntry *se = *cur;
		
		//check limits on this spawn group
		if(!entity_list.LimitCheckBoth(se->NPCType, id, se->group_spawn_limit, se->npc_spawn_limit))
			continue;
		
		totalchance += se->chance;
		possible.push_back(se);
	}
	if(totalchance == 0)
		return 0;
	
	
	sint32 roll = 0;
	roll = MakeRandomInt(0, totalchance);
	
	cur = possible.begin();
	end = possible.end();
	for(; cur != end; cur++) {
		SpawnEntry *se = *cur;
		if (roll < se->chance) {
			npcType = se->NPCType;
			break;
		} else {
			roll -= se->chance;
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



//bool Database::PopulateZoneLists(char* zone_name, NPCType* &npc_type_array, uint32 &max_npc_type, LinkedList<Spawn*>& spawn_list, LinkedList<ZonePoint*>& zone_point_list, LinkedList<Spawn2*> &spawn2_list, SpawnGroupList* spawn_group_list)
//bool Database::PopulateZoneLists(char* zone_name, NPCType* &npc_type_array, uint32 &max_npc_type, LinkedList<ZonePoint*>& zone_point_list, SpawnGroupList* spawn_group_list)
bool Database::PopulateZoneLists(const char* zone_name, LinkedList<ZonePoint*>* zone_point_list, SpawnGroupList* spawn_group_list) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
		
	if(!LoadStaticZonePoints(zone_point_list,zone_name))
		return false;
	// CODER new spawn code
	query = 0;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT DISTINCT(spawngroupID), spawngroup.name FROM spawn2,spawngroup WHERE spawn2.spawngroupID=spawngroup.ID and zone='%s'", zone_name), errbuf, &result))
	{
		safe_delete_array(query);
		while((row = mysql_fetch_row(result))) {
			SpawnGroup* newSpawnGroup = new SpawnGroup( atoi(row[0]), row[1]);
			spawn_group_list->AddSpawnGroup(newSpawnGroup);
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error2 in PopulateZoneLists query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}

	query = 0;
	if (RunQuery(query, MakeAnyLenString(&query, 
		"SELECT spawnentry.spawngroupID, npcid, chance, "
		" spawnentry.spawn_limit AS gsl, npc_types.spawn_limit AS sl "
		"FROM spawnentry, spawn2 LEFT JOIN npc_types ON spawnentry.npcID = npc_types.id "
		"WHERE spawnentry.spawngroupID=spawn2.spawngroupID "
		"AND zone='%s' ORDER by chance", zone_name), errbuf, &result)) {
		safe_delete_array(query);
		while((row = mysql_fetch_row(result)))
		{
			SpawnEntry* newSpawnEntry = new SpawnEntry( atoi(row[1]), atoi(row[2]), row[3]?atoi(row[3]):0, row[4]?atoi(row[4]):0);
			SpawnGroup *sg = spawn_group_list->GetSpawnGroup(atoi(row[0]));
			if (sg)
				sg->AddSpawnEntry(newSpawnEntry);
			else
				cout << "Error in SpawngroupID: " << row[0] << endl;
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error3 in PopulateZoneLists query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}

	// CODER end new spawn code
	return true;
}

