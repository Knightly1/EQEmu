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
#include "worldserver.h"

extern EntityList entity_list;
extern Zone* zone;
extern Database database;
extern WorldServer worldserver;

/*

CREATE TABLE spawn_conditions (
	zone VARCHAR(16) NOT NULL,
	id MEDIUMINT UNSIGNED NOT NULL DEFAULT '1',
	value MEDIUMINT NOT NULL DEFAULT '0',
	onchange TINYINT UNSIGNED NOT NULL DEFAULT '0',
	name VARCHAR(255) NOT NULL DEFAULT '',
	PRIMARY KEY(zone,id)
);

CREATE TABLE spawn_events (
	#identifiers
	id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
	zone VARCHAR(16) NOT NULL,
	cond_id MEDIUMINT UNSIGNED NOT NULL,
	name VARCHAR(255) NOT NULL DEFAULT '',
	
	#timing information
	period INT UNSIGNED NOT NULL,
	next_minute TINYINT UNSIGNED NOT NULL,
	next_hour TINYINT UNSIGNED NOT NULL,
	next_day TINYINT UNSIGNED NOT NULL,
	next_month TINYINT UNSIGNED NOT NULL,
	next_year INT UNSIGNED NOT NULL,
	enabled TINYINT NOT NULL DEFAULT '1',
	
	#action:
	action TINYINT UNSIGNED NOT NULL DEFAULT '0',
	argument MEDIUMINT NOT NULL DEFAULT '0'
);

*/

Spawn2::Spawn2(int32 in_spawn2_id, int32 spawngroup_id, 
	float in_x, float in_y, float in_z, float in_heading, 
	int32 respawn, int32 variance, int32 timeleft, int16 grid,
	uint16 in_cond_id, sint16 in_min_value)
: timer(100000)
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
	condition_id = in_cond_id;
	condition_min_value = in_min_value;
	npcthis = NULL;
	
	if(timeleft == 0xFFFFFFFF) {
		//special disable timeleft
		timer.Disable();
	} else if(timeleft != 0){
		//we have a timeleft from the DB or something
		timer.Start(timeleft);
	} else {
		//no timeleft at all, reset to 
		timer.Start(resetTimer());
		timer.Trigger();
	}
}

Spawn2::~Spawn2()
{
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
	if (timer.Check())	{
		timer.Disable();
		
		//first check our spawn condition, if this isnt active
		//then we reset the timer and try again next time.
		if(condition_id != SC_AlwaysEnabled 
			&& !zone->spawn_conditions.Check(condition_id, condition_min_value)) {
			Reset();
			return(true);
		}
		
		//grab our spawn group
		SpawnGroup* sg = zone->spawn_group_list.GetSpawnGroup(spawngroup_id_);
		if (sg == NULL)
			return false;
		
		//have the spawn group pick an NPC for us
		int32 npcid = sg->GetNPCType();
		if (npcid == 0) {
			Reset();	//try again later (why?)
			return(true);
		}
		
		//try to find our NPC type.
		const NPCType* tmp = database.GetNPCType(npcid);
		if (tmp == NULL) {
			Reset();	//try again later (why?)
			return(true);
		}
		
		currentnpcid = npcid;
		NPC* npc = new NPC(tmp, this, x, y, z, heading);
		npcthis = npc;
		npc->AddLootTable();
		npc->SetSp2(spawngroup_id_);
		if(zone->InstantGrids())
			LoadGrid();
		entity_list.AddNPC(npc);
	}
	return true;
}

void Spawn2::LoadGrid() {
	if(!npcthis)
		return;
	if(grid_ < 1)
		return;
	if(!entity_list.IsMobInZone(npcthis))
		return;
	//dont set an NPC's grid until its loaded for them.
	npcthis->SetGrid(grid_);
	npcthis->AssignWaypoints(grid_);
}


/*
	All three of these actions basically say that the mob which was
	associated with this spawn point is no longer relavent.
*/
void Spawn2::Reset() {
	timer.Start(resetTimer());
	npcthis = NULL;
}

void Spawn2::Depop() {
	timer.Disable();
	npcthis = NULL;
}

void Spawn2::Repop(int32 delay) {
	if (delay == 0)
		timer.Trigger();
	else
		timer.Start(delay);
	npcthis = NULL;
}

bool Database::PopulateZoneSpawnList(const char* zone_name, LinkedList<Spawn2*> &spawn2_list, int32 repopdelay) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	MakeAnyLenString(&query, "SELECT id, spawngroupID, x, y, z, heading, respawntime, variance, pathgrid, timeleft, condition, cond_value FROM spawn2 WHERE zone='%s'", zone_name);
	
	if (RunQuery(query, strlen(query), errbuf, &result))
	{
		safe_delete_array(query);
		while((row = mysql_fetch_row(result)))
		{
			Spawn2* newSpawn = 0;
			newSpawn = new Spawn2(atoi(row[0]), atoi(row[1]), atof(row[2]), atof(row[3]), atof(row[4]), atof(row[5]), atoi(row[6]), atoi(row[7]), atoi(row[9]), atoi(row[8]), atoi(row[10]), atoi(row[11]));
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

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, spawngroupID, x, y, z, heading, respawntime, variance, pathgrid, condition, cond_value FROM spawn2 WHERE id=%i", spawn2id), errbuf, &result))
	{
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			Spawn2* newSpawn = new Spawn2(atoi(row[0]), atoi(row[1]), atof(row[2]), atof(row[3]), atof(row[4]), atof(row[5]), atoi(row[6]), atoi(row[7]), timeleft, atoi(row[8]), atoi(row[9]), atoi(row[10]));
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

bool Database::CreateSpawn2(Client *c, int32 spawngroup, const char* zone, float heading, float x, float y, float z, int32 respawn, int32 variance, uint16 condition, sint16 cond_value)
{
	char errbuf[MYSQL_ERRMSG_SIZE];

    char *query = 0;
	int32 affected_rows = 0;
	
	//	if(GetInverseXY()==1) {
	//		float temp=x;
	//		x=y;
	//		y=temp;
	//	}
	if (RunQuery(query, MakeAnyLenString(&query, 
		"INSERT INTO spawn2 (spawngroupID,zone,x,y,z,heading,respawntime,variance,condition,cond_value) Values (%i, '%s', %f, %f, %f, %f, %i, %i, %u, %i)", 
		spawngroup, zone, x, y, z, heading, respawn, variance, condition, cond_value
		), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		if (affected_rows == 1) {
			if(c) c->LogSQL(query);
			return true;
		}
		else {
			return false;
		}
	}
	else {
		LogFile->write(EQEMuLog::Error, "Error in CreateSpawn2 query '%s': %s", query, errbuf);
		safe_delete_array(query);
		return false;
	}
	
	return false;
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
			spawn2dump[*spawn2index].time_left = iterator.GetData()->timer.GetRemainingTime();
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
		spawn2dump[*spawn2index].time_left = iterator.GetData()->timer.GetRemainingTime();
		(*spawn2index)++;
		iterator.RemoveCurrent();

	}
}

void Spawn2::SpawnConditionChanged(const SpawnCondition &c, sint16 old_value) {
	if(GetSpawnCondition() != c.condition_id)
		return;
	
	bool old_state = (old_value >= condition_min_value);
	bool new_state = (c.value >= condition_min_value);
	if(old_state == new_state)
		return;	//no change
	
	switch(c.on_change) {
	case SpawnCondition::DoNothing:
		//that was easy.
		break;
	case SpawnCondition::DoDepop:
		if(npcthis != NULL)
			npcthis->Depop(false);	//remove the current mob
		Reset();	//reset our spawn timer
		break;
	case SpawnCondition::DoRepop:
		if(npcthis != NULL)
			npcthis->Depop(false);	//remove the current mob
		Repop();	//repop
		break;
	default:
		if(c.on_change < SpawnCondition::DoSignalMin)
			return;	//unknown onchange action
		int signal_id = c.on_change - SpawnCondition::DoSignalMin;
		if(npcthis != NULL)
			npcthis->SignalNPC(signal_id);
	}
}

void Zone::SpawnConditionChanged(const SpawnCondition &c, sint16 old_value) {
	LinkedListIterator<Spawn2*> iterator(spawn2_list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		Spawn2 *cur = iterator.GetData();
		if(cur->GetSpawnCondition() == c.condition_id)
			cur->SpawnConditionChanged(c, old_value);
		iterator.Advance();
	}
}

SpawnCondition::SpawnCondition() {
	condition_id = 0;
	value = 0;
	on_change = DoNothing;
}

SpawnEvent::SpawnEvent() {
	id = 0;
	condition_id = 0;
	enabled = false;
	action = ActionSet;
	argument = 0;
	period = 0xFFFFFFFF;
	memset(&next, 0, sizeof(next));
}

SpawnConditionManager::SpawnConditionManager()
 : minute_timer(3000)	//1 eq minute
{
	memset(&next_event, 0, sizeof(next_event));
}

void SpawnConditionManager::Process() {
	if(spawn_events.empty())
		return;
	
	if(minute_timer.Check()) {
		//check each spawn event.
		
		//get our current time
		TimeOfDay_Struct tod;
		zone->zone_time.getEQTimeOfDay(&tod);
		
		//see if time is past our nearest event.
		if(EQTime::IsTimeBefore(&next_event, &tod))
			return;
		
		//at least one event should get triggered, 
		vector<SpawnEvent>::iterator cur,end;
		cur = spawn_events.begin();
		end = spawn_events.end();
		for(; cur != end; cur++) {
			SpawnEvent &cevent = *cur;
			
			if(!cevent.enabled)
				continue;
			
			if(EQTime::IsTimeBefore(&tod, &cevent.next)) {
printf("Executing event %d (period%d) (%d > %d)\n", cevent.id, cevent.period, tod.minute, cevent.next.minute);
				//this event has been triggered.
				//execute the event
				ExecEvent(cevent, true);
				//add the period of the event to the trigger time
				EQTime::AddMinutes(cevent.period, &cevent.next);
				//save the next event time in the DB
				UpdateDBEvent(cevent);
				//find the next closest event timer.
				FindNearestEvent();
				//minor optimization, if theres no more possible events,
				//then stop trying... I dunno how worth while this is.
				if(EQTime::IsTimeBefore(&next_event, &tod))
					return;
			}
		}
	}
}

void SpawnConditionManager::ExecEvent(SpawnEvent &event, bool send_update) {
	map<uint16, SpawnCondition>::iterator condi;
	condi = spawn_conditions.find(event.condition_id);
	if(condi == spawn_conditions.end())
		return;	//unable to find the spawn condition to operate on
	
	SpawnCondition &cond = condi->second;
	
	sint16 new_value = cond.value;
	
	//we have our event and our condition, do our stuff.
	switch(event.action) {
	case SpawnEvent::ActionSet:
		new_value = event.argument;
		break;
	case SpawnEvent::ActionAdd:
		new_value += event.argument;
		break;
	case SpawnEvent::ActionSubtract:
		new_value -= event.argument;
		break;
	case SpawnEvent::ActionMultiply:
		new_value *= event.argument;
		break;
	case SpawnEvent::ActionDivide:
		new_value /= event.argument;
		break;
	default:
		//print an error?
		return;
	}
	
	//now set the condition to the new value
	if(send_update)	//full blown update
		SetCondition(zone->GetShortName(), cond.condition_id, new_value);
	else	//minor update done while loading
		cond.value = new_value;
}

void SpawnConditionManager::UpdateDBEvent(SpawnEvent &event) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	int len;
	
	SpawnCondition cond;
	len = MakeAnyLenString(&query, 
	"UPDATE spawn_events SET "
	"next_minute=%d, next_hour=%d, next_day=%d, next_month=%d, "
	"next_year=%d, enabled=%d "
	"WHERE id=%d",
	event.next.minute, event.next.hour, event.next.day, event.next.month, 
	event.next.year, event.enabled?1:0, event.id
	);
	if(!database.RunQuery(query, len, errbuf)) {
		LogFile->write(EQEMuLog::Error, "Unable to update spawn event '%s': %s\n", query, errbuf);
	}
	safe_delete_array(query);
}

void SpawnConditionManager::UpdateDBCondition(const char* zone_name, uint16 cond_id, sint16 value) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	int len;
	
	SpawnCondition cond;
	len = MakeAnyLenString(&query, 
	"UPDATE spawn_conditions SET "
	"value=%d "
	"WHERE zone='%s' AND id=%d",
		value, zone_name, cond_id
	);
	if(!database.RunQuery(query, len, errbuf)) {
		LogFile->write(EQEMuLog::Error, "Unable to update spawn condition '%s': %s\n", query, errbuf);
	}
	safe_delete_array(query);
}

bool SpawnConditionManager::LoadDBEvent(uint32 event_id, SpawnEvent &event, string &zone_name) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int len;
	
	bool ret = false;
	
	len = MakeAnyLenString(&query, 
	"SELECT id,cond_id,period,next_minute,next_hour,next_day,next_month,next_year,enabled,action,argument,zone "
	"FROM spawn_events WHERE id=%d", event_id);
	if (database.RunQuery(query, len, errbuf, &result)) {
		safe_delete_array(query);
		if((row = mysql_fetch_row(result))) {
			event.id = atoi(row[0]);
			event.condition_id = atoi(row[1]);
			event.period = atoi(row[2]);
			
			event.next.minute = atoi(row[3]);
			event.next.hour = atoi(row[4]);
			event.next.day = atoi(row[5]);
			event.next.month = atoi(row[6]);
			event.next.year = atoi(row[7]);
			
			event.enabled = atoi(row[8])==0?false:true;
			event.action = (SpawnEvent::Action) atoi(row[9]);
			event.argument = atoi(row[10]);
			zone_name = row[11];
			
			ret = true;
		}
		mysql_free_result(result);
	} else {
		LogFile->write(EQEMuLog::Error, "Error in LoadDBEvent query '%s': %s", query, errbuf);
		safe_delete_array(query);
	}
	return(ret);
}

bool SpawnConditionManager::LoadSpawnConditions(const char* zone_name) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int len;
	
	//clear out old stuff..
	spawn_conditions.clear();
	
	//load spawn conditions	
	SpawnCondition cond;
	len = MakeAnyLenString(&query, "SELECT id,value,onchange FROM spawn_conditions WHERE zone='%s'", zone_name);
	if (database.RunQuery(query, len, errbuf, &result)) {
		safe_delete_array(query);
		while((row = mysql_fetch_row(result))) {
			cond.condition_id = atoi(row[0]);
			cond.value = atoi(row[1]);
			cond.on_change = (SpawnCondition::OnChange) atoi(row[2]);
			spawn_conditions[cond.condition_id] = cond;
		}
		mysql_free_result(result);
	} else {
		LogFile->write(EQEMuLog::Error, "Error in LoadSpawnConditions query '%s': %s", query, errbuf);
		safe_delete_array(query);
		return false;
	}
	
	//load spawn events
	SpawnEvent event;
	len = MakeAnyLenString(&query, 
	"SELECT id,cond_id,period,next_minute,next_hour,next_day,next_month,next_year,enabled,action,argument "
	"FROM spawn_events WHERE zone='%s'", zone_name);
	if (database.RunQuery(query, len, errbuf, &result)) {
		safe_delete_array(query);
		while((row = mysql_fetch_row(result))) {
			event.id = atoi(row[0]);
			event.condition_id = atoi(row[1]);
			event.period = atoi(row[2]);
			if(event.period == 0) {
				LogFile->write(EQEMuLog::Error, "Refusing to load spawn event #%d because ti has a period of 0\n", event.id);
				continue;
			}
			
			event.next.minute = atoi(row[3]);
			event.next.hour = atoi(row[4]);
			event.next.day = atoi(row[5]);
			event.next.month = atoi(row[6]);
			event.next.year = atoi(row[7]);
			
			event.enabled = atoi(row[8])==0?false:true;
			event.action = (SpawnEvent::Action) atoi(row[9]);
			event.argument = atoi(row[10]);
			
			spawn_events.push_back(event);
		}
		mysql_free_result(result);
	} else {
		LogFile->write(EQEMuLog::Error, "Error in LoadSpawnConditions events query '%s': %s", query, errbuf);
		safe_delete_array(query);
		return false;
	}
	
	//now we need to catch up on events that happened  while we were away
	//and use them to alter just the condition variables.
	
	//each spawn2 will then use its correct condition value when
	//it decides what to do. This essentially forces a 'depop' action
	//on spawn points which are turned off, and a 'repop' action on
	//spawn points which get turned on. Im too lazy to figure out a
	//better solution, and I just dont care thats much.
	//get our current time
	TimeOfDay_Struct tod;
	zone->zone_time.getEQTimeOfDay(&tod);
	
	vector<SpawnEvent>::iterator cur,end;
	cur = spawn_events.begin();
	end = spawn_events.end();
	bool ran;
	for(; cur != end; cur++) {
		SpawnEvent &cevent = *cur;
		
		if(!cevent.enabled)
			continue;
		
		//watch for special case of all 0s, which means to reset next to now
		if(cevent.next.year == 0 && cevent.next.month == 0 && cevent.next.day == 0 && cevent.next.hour == 0 && cevent.next.minute == 0) {
			memcpy(&cevent.next, &tod, sizeof(cevent.next));
			//add one period
			EQTime::AddMinutes(cevent.period, &cevent.next);
			//save it in the db.
			UpdateDBEvent(cevent);
			continue;	//were done with this event.
		}
		
		ran = false;
		while(EQTime::IsTimeBefore(&tod, &cevent.next)) {
			//this event has been triggered.
			//execute the event
			ExecEvent(cevent, false);
			//add the period of the event to the trigger time
			EQTime::AddMinutes(cevent.period, &cevent.next);
			ran = true;
		}
		//only write it out if the event actually ran
		if(ran) {
			//save the event in the DB
			UpdateDBEvent(cevent);
		}
	}
	
	//now our event timers are all up to date, find our closest event.
	FindNearestEvent();
	
	return(true);
}

void SpawnConditionManager::FindNearestEvent() {
	//set a huge year whihc should never get reached normally
	next_event.year = 0xFFFFFF;
	
	vector<SpawnEvent>::iterator cur,end;
	cur = spawn_events.begin();
	end = spawn_events.end();
	for(; cur != end; cur++) {
		SpawnEvent &cevent = *cur;
		
		if(!cevent.enabled)
			continue;
		
		//see if this event is before our last nearest
		if(EQTime::IsTimeBefore(&next_event, &cevent.next)) {
			memcpy(&next_event, &cevent.next, sizeof(next_event));
		}
	}
}

void SpawnConditionManager::SetCondition(const char *zone_short, uint16 condition_id, sint16 new_value, bool world_update) {
	if(world_update) {
		//this is an update coming from another zone, they
		//have allready updated the DB, just need to update our
		//memory, and check for condition changes
		map<uint16, SpawnCondition>::iterator condi;
		condi = spawn_conditions.find(condition_id);
		if(condi == spawn_conditions.end())
			return;	//unable to find the spawn condition
	
		SpawnCondition &cond = condi->second;
		
		sint16 old_value = cond.value;
		
		//set our local value
		cond.value = new_value;
		
		//now we have to test each spawn point to see if it changed.
		zone->SpawnConditionChanged(cond, old_value);
	} else if(!strcasecmp(zone_short, zone->GetShortName())) {
		//this is a local spawn condition, we need to update the DB,
		//our memory, then notify spawn points of the change.
		map<uint16, SpawnCondition>::iterator condi;
		condi = spawn_conditions.find(condition_id);
		if(condi == spawn_conditions.end())
			return;	//unable to find the spawn condition
		
		SpawnCondition &cond = condi->second;
	
		sint16 old_value = cond.value;
		
		//set our local value
		cond.value = new_value;
		//save it in the DB too
		UpdateDBCondition(zone_short, condition_id, new_value);
		
		//now we have to test each spawn point to see if it changed.
		zone->SpawnConditionChanged(cond, old_value);
	} else {
		//this is a remote spawn condition, update the DB and send
		//an update packet to the zone if its up
		UpdateDBCondition(zone_short, condition_id, new_value);
		
		ServerPacket* pack = new ServerPacket(ServerOP_SpawnCondition, sizeof(ServerSpawnEvent_Struct));
		ServerSpawnCondition_Struct* ssc = (ServerSpawnCondition_Struct*)pack->pBuffer;
		
		ssc->zoneID = database.GetZoneID(zone_short);
		ssc->condition_id = condition_id;
		ssc->value = new_value;
		
		worldserver.SendPacket(pack);
		safe_delete(pack);
	}
}

void SpawnConditionManager::ReloadEvent(uint32 event_id) {
	string zone_short_name;
	
	//first look for the event in our local event list
	vector<SpawnEvent>::iterator cur,end;
	cur = spawn_events.begin();
	end = spawn_events.end();
	for(; cur != end; cur++) {
		SpawnEvent &cevent = *cur;
		
		if(cevent.id == event_id) {
			//load the event into the old event slot
			if(!LoadDBEvent(event_id, cevent, zone_short_name)) {
				//unable to find the event in the database...
				return;
			}
			//sync up our nearest event
			FindNearestEvent();
			return;
		}
	}
	
	//if we get here, it is a new event...
	SpawnEvent e;
	if(!LoadDBEvent(event_id, e, zone_short_name)) {
		//unable to find the event in the database...
		return;
	}
	
	//we might want to check the next timer like we do on
	//regular load events, but we are assuming this is a new event
	//and anyways, this will get handled (albeit not optimally)
	//naturally by the event handling code.
	
	spawn_events.push_back(e);
	
	//sync up our nearest event
	FindNearestEvent();
}

void SpawnConditionManager::ToggleEvent(uint32 event_id, bool enabled, bool reset_base) {
	
	//first look for the event in our local event list
	vector<SpawnEvent>::iterator cur,end;
	cur = spawn_events.begin();
	end = spawn_events.end();
	for(; cur != end; cur++) {
		SpawnEvent &cevent = *cur;
		
		if(cevent.id == event_id) {
			//make sure were actually changing something
			if(cevent.enabled != enabled || reset_base) {
				cevent.enabled = enabled;
				if(reset_base) {
					//start with the time now
					zone->zone_time.getEQTimeOfDay(&cevent.next);
					//advance the next time by our period
					EQTime::AddMinutes(cevent.period, &cevent.next);
				}
				
				//save the event in the DB
				UpdateDBEvent(cevent);
				
				//sync up our nearest event
				FindNearestEvent();
			}
			//even if we dont change anything, we still found it
			return;
		}
	}
	
	//if we get here, the condition must be in another zone.
	//first figure out what zone it applies to.
	//update the DB and send and send an update packet to 
	//the zone if its up
	
	//we need to load the event from the DB and then update 
	//the values in the DB, because we do not know if the zone
	//is up or not. The message to the zone will just tell it to
	//update its in-memory event list
	SpawnEvent e;
	string zone_short_name;
	if(!LoadDBEvent(event_id, e, zone_short_name)) {
		//unable to find the event in the database...
		return;
	}
	if(e.enabled == enabled && !reset_base)
		return;	//no changes.
	
	e.enabled = enabled;
	if(reset_base) {
		//start with the time now
		zone->zone_time.getEQTimeOfDay(&e.next);
		//advance the next time by our period
		EQTime::AddMinutes(e.period, &e.next);
	}
	//save the event in the DB
	UpdateDBEvent(e);
	
	
	//now notify the zone
	ServerPacket* pack = new ServerPacket(ServerOP_SpawnEvent, sizeof(ServerSpawnEvent_Struct));
	ServerSpawnEvent_Struct* sse = (ServerSpawnEvent_Struct*)pack->pBuffer;
	
	sse->zoneID = database.GetZoneID(zone_short_name.c_str());
	sse->event_id = event_id;
	
	worldserver.SendPacket(pack);
	safe_delete(pack);
}

sint16 SpawnConditionManager::GetCondition(const char *zone_short, uint16 condition_id) {
	if(!strcasecmp(zone_short, zone->GetShortName())) {
		//this is a local spawn condition
		map<uint16, SpawnCondition>::iterator condi;
		condi = spawn_conditions.find(condition_id);
		if(condi == spawn_conditions.end())
			return(false);	//unable to find the spawn condition
	
		SpawnCondition &cond = condi->second;
		return(cond.value);
	} else {
		//this is a remote spawn condition, grab it from the DB
		char errbuf[MYSQL_ERRMSG_SIZE];
		char* query = 0;
		MYSQL_RES *result;
		MYSQL_ROW row;
		int len;
		
		sint16 value;
		
		//load spawn conditions	
		SpawnCondition cond;
		len = MakeAnyLenString(&query, "SELECT value FROM spawn_conditions WHERE zone='%s' AND id=%d", zone_short, condition_id);
		if (database.RunQuery(query, len, errbuf, &result)) {
			safe_delete_array(query);
			if((row = mysql_fetch_row(result))) {
				value = atoi(row[0]);
			} else {
				value = 0;	//dunno a better thing to do...
			}
			mysql_free_result(result);
		} else {
			safe_delete_array(query);
			value = 0;	//dunno a better thing to do...
		}
		return(value);
	}
}

bool SpawnConditionManager::Check(uint16 condition, sint16 min_value) {
	map<uint16, SpawnCondition>::iterator condi;
	condi = spawn_conditions.find(condition);
	if(condi == spawn_conditions.end())
		return(false);	//unable to find the spawn condition
	
	SpawnCondition &cond = condi->second;
	
	return(cond.value >= min_value);
}










