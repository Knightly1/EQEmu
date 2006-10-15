#ifndef __IDGENSET_H_INCL__
#define __IDGENSET_H_INCL__

#include "IDGenerator.h"

//this class is the complete set of ID generators.
class database;
class IDGenSet {
public:
	IDGenSet(database *db);

	void Reset();
	void Restart();
	void SaveToFile();
	void ReadFromFile();
	void DataWritten();
	void SetZoneID(int zoneid);

	IDGenerator npc_types;
	IDGenerator spawngroup;
	IDGenerator spawn2;
	IDGenerator spawnentry;
	IDGenerator grid;
	IDGenerator merchantlist;
};




#endif




