
#include "stdafx.h"
#include "EQBuilder.h"
#include "IDGenSet.h"
#include "database.h"


IDGenSet::IDGenSet(database *db)
:	npc_types	("npc_types", 	 "SELECT MAX(id)+1 FROM npc_types", db),
	spawngroup	("spawngroup", 	 "SELECT MAX(id)+1 FROM spawngroup", db),
	spawn2		("spawn2", 		 "SELECT MAX(id)+1 FROM spawn2", db),
	spawnentry	("spawnentry", 	 "SELECT MAX(id)+1 FROM spawnentry", db),
	grid		("grid", 		 "SELECT MAX(id)+1 FROM grid", db),
	merchantlist("merchantlist", "SELECT MAX(id)+1 FROM merchantlist", db)
{
	npc_types.SetToZone();
	npc_types.SetZoneMultiple(1000);
	spawngroup.SetToZone();
	spawngroup.SetZoneMultiple(10000);
	spawn2.SetToZone();
	spawn2.SetZoneMultiple(10000);
	spawnentry.SetToZone();
	spawnentry.SetZoneMultiple(10000);
	grid.SetToFixed();
	grid.SetFixedID(1);
}

void IDGenSet::Reset() {
	npc_types.Reset();
	spawngroup.Reset();
	spawn2.Reset();
	spawnentry.Reset();
	grid.Reset();
	merchantlist.Reset();
}

void IDGenSet::Restart() {
	npc_types.Restart();
	spawngroup.Restart();
	spawn2.Restart();
	spawnentry.Restart();
	grid.Restart();
	merchantlist.Restart();
}

void IDGenSet::ReadFromFile() {
	npc_types.ReadFromFile();
	spawngroup.ReadFromFile();
	spawn2.ReadFromFile();
	spawnentry.ReadFromFile();
	grid.ReadFromFile();
	merchantlist.ReadFromFile();
}

void IDGenSet::SaveToFile() {
	npc_types.SaveToFile();
	spawngroup.SaveToFile();
	spawn2.SaveToFile();
	spawnentry.SaveToFile();
	grid.SaveToFile();
	merchantlist.SaveToFile();
}

void IDGenSet::DataWritten() {
	npc_types.DataWritten();
	spawngroup.DataWritten();
	spawn2.DataWritten();
	spawnentry.DataWritten();
	grid.DataWritten();
	merchantlist.DataWritten();
}

void IDGenSet::SetZoneID(int zoneid) {
	npc_types.SetZoneID(zoneid);
	spawngroup.SetZoneID(zoneid);
	spawn2.SetZoneID(zoneid);
	spawnentry.SetZoneID(zoneid);
	grid.SetZoneID(zoneid);
	merchantlist.SetZoneID(zoneid);
}






























