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
#include <stdlib.h>
#include "spawn2.h"
#include "entity.h"
#include "masterentity.h"
#include "zone.h"
#include "spawngroup.h"
#include "../common/database.h"

extern EntityList entity_list;
extern Zone* zone;
extern Database database;


Spawn2::Spawn2(int32 in_spawn2_id, int32 spawngroup_id, float in_x, float in_y, float in_z, float in_heading, int32 respawn, int32 variance, int32 timeleft, int16 grid):
gridtimer(10000)
{
	spawn2_id = in_spawn2_id;
	spawngroup_id_ = spawngroup_id;
	x = in_x;
	y = in_y;
	z = in_z;
	heading = in_heading;
    respawn_ = respawn;
	variance_ = variance;
	grid_ = grid;
	npcthis = NULL;
	if(timeleft!=0){
		//printf("Timeleft: %i\n",timeleft);
		timer = new Timer( timeleft );
	}
	else
		timer = new Timer( resetTimer() );
	if (timeleft == 0xFFFFFFFF) {
		timer->Disable();
	}
	else if (timeleft == 0) {
		timer->Trigger();
	}
	else {
		timer->Start(0);
	}
}

Spawn2::~Spawn2()
{
	safe_delete(timer);
}

int32 Spawn2::resetTimer()
{
	int32 rspawn = respawn_ * 1000;
	
	if (variance_ != 0) {
		int32 vardir = (rand()%50);
		int32 variance = (rand()%variance_);
		float varper = variance*0.01;
		float varvalue = varper*(rspawn);
		if (vardir < 50)
		{
			varvalue = varvalue * -1;
		}
		
		rspawn += (int32) varvalue;
	}
	
	return (rspawn);
	
}

bool Spawn2::Process() {
	_ZP(Spawn2_Process);
	if (timer->Check())	{
		timer->Disable();
		
		SpawnGroup* sg = zone->spawn_group_list.GetSpawnGroup(spawngroup_id_);
		if (sg == NULL)
			return false;
		
		int32 npcid = sg->GetNPCType();
		if (npcid) {
			const NPCType* tmp = database.GetNPCType(npcid);
			if (tmp) {
				currentnpcid = npcid;
				NPC* npc = new NPC(tmp, this, x, y, z, heading);
				npcthis = npc;
				npc->AddLootTable();
				npc->SetGrid(grid_);
				npc->SetSp2(spawngroup_id_);
				entity_list.AddNPC(npc);
			}
		}
		else {
			Reset();
		} 
	}
	if(gridtimer.Check() && npcthis){
		gridtimer.Disable();
		if (grid_ > 0 && entity_list.IsMobInZone(npcthis))
			npcthis->AssignWaypoints(grid_);
	}
	return true;
}

void Spawn2::Reset()
{
	timer->Start(resetTimer());
}

void Spawn2::Depop() {
	timer->Disable();
}

void Spawn2::Repop(int32 delay) {
	if (delay == 0)
		timer->Trigger();
	else
		timer->Start(delay);
}

bool Database::PopulateZoneSpawnList(const char* zone_name, LinkedList<Spawn2*> &spawn2_list, int32 repopdelay) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	MakeAnyLenString(&query, "SELECT id, spawngroupID, x, y, z, heading, respawntime, variance, pathgrid, timeleft FROM spawn2 WHERE zone='%s'", zone_name);
	
	if (RunQuery(query, strlen(query), errbuf, &result))
	{
		safe_delete_array(query);
		while((row = mysql_fetch_row(result)))
		{
			Spawn2* newSpawn = 0;
						//if(GetInverseXY()==1) {
						//	newSpawn = new Spawn2(atoi(row[0]), atoi(row[1]), atof(row[3]), atof(row[2]), atof(row[4]), atof(row[5]), atoi(row[6]), atoi(row[7]));
						//}
						//else {
			newSpawn = new Spawn2(atoi(row[0]), atoi(row[1]), atof(row[2]), atof(row[3]), atof(row[4]), atof(row[5]), atoi(row[6]), atoi(row[7]), atoi(row[9]), atoi(row[8]));
						//}
			//newSpawn->Repop(repopdelay);
			spawn2_list.Insert( newSpawn );
		}
		mysql_free_result(result);
	}
	else
	{
		LogFile->write(EQEMuLog::Error, "Error in PopulateZoneLists query '%s': %s", query, errbuf);
		safe_delete_array(query);
		return false;
	}
	
	return true;
}


Spawn2* Database::LoadSpawn2(LinkedList<Spawn2*> &spawn2_list, int32 spawn2id, int32 timeleft) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, spawngroupID, x, y, z, heading, respawntime, variance, pathgrid FROM spawn2 WHERE id=%i", spawn2id), errbuf, &result))
	{
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			Spawn2* newSpawn = new Spawn2(atoi(row[0]), atoi(row[1]), atof(row[2]), atof(row[3]), atof(row[4]), atof(row[5]), atoi(row[6]), atoi(row[7]), timeleft, atoi(row[8]));
			spawn2_list.Insert( newSpawn );
			mysql_free_result(result);
			safe_delete_array(query);
			return newSpawn;
		}
		mysql_free_result(result);
	}

	LogFile->write(EQEMuLog::Error, "Error in LoadSpawn2 query '%s': %s", query, errbuf);
	safe_delete_array(query);
	return 0;
}


int32 Zone::CountSpawn2() {
	LinkedListIterator<Spawn2*> iterator(spawn2_list);
	int32 count = 0;

	iterator.Reset();
	while(iterator.MoreElements())
	{
		count++;
		iterator.Advance();
	}
	return count;
}

int32 Zone::DumpSpawn2(ZSDump_Spawn2* spawn2dump, int32* spawn2index, Spawn2* spawn2) {
	if (spawn2 == 0)
		return 0;
	LinkedListIterator<Spawn2*> iterator(spawn2_list);
	//	int32	index = 0;

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData() == spawn2) {
			spawn2dump[*spawn2index].spawn2_id = iterator.GetData()->spawn2_id;
			spawn2dump[*spawn2index].time_left = iterator.GetData()->timer->GetRemainingTime();
			iterator.RemoveCurrent();
			return (*spawn2index)++;
		}
		iterator.Advance();
	}
	return 0xFFFFFFFF;
}

void Zone::DumpAllSpawn2(ZSDump_Spawn2* spawn2dump, int32* spawn2index) {
	LinkedListIterator<Spawn2*> iterator(spawn2_list);
	//	int32	index = 0;

	iterator.Reset();
	while(iterator.MoreElements())
	{
		spawn2dump[*spawn2index].spawn2_id = iterator.GetData()->spawn2_id;
		spawn2dump[*spawn2index].time_left = iterator.GetData()->timer->GetRemainingTime();
		(*spawn2index)++;
		iterator.RemoveCurrent();

	}
}

