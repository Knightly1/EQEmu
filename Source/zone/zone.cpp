/*  EQEMu:  Everquest Server Emulator
	Copyright (C) 2001-2003  EQEMu Development Team (http://eqemulator.net)

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
#include <iostream>
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <time.h>
#include <math.h>

#ifdef WIN32
#include <process.h>
#define	 snprintf	_snprintf
#define  vsnprintf	_vsnprintf
#else
#include <pthread.h>
#include "../common/unix.h"
#endif

#include "masterentity.h"
#include "spawngroup.h"
#include "spawn2.h"
#include "zone.h"
#include "worldserver.h"
#include "npc.h"
#include "net.h"
#include "../common/seperator.h"
#include "../common/packet_dump_file.h"
#include "../common/EQNetwork.h"
#include "map.h"
#include "object.h"
#include "petitions.h"
#include "../common/files.h"
#include "parser.h"
#include "event_codes.h"

#ifdef WIN32
#define snprintf	_snprintf
#define strncasecmp	_strnicmp
#define strcasecmp  _stricmp
#endif

#ifdef GUILDWARS
#include "../GuildWars/GuildWars.h"
extern GuildWars guildwars;
extern GuildLocationList location_list;
#endif

extern Database database;
extern WorldServer worldserver;
extern Zone* zone;
extern int32 numclients;
extern NetConnection net;
extern PetitionList petition_list;
extern EQNetworkServer eqns;
Mutex MZoneShutdown;
extern bool staticzone;
Zone* zone = 0;
volatile bool ZoneLoaded = false;
extern Parser* parse;
void CleanupLoadZoneState(int32 spawn2_count, ZSDump_Spawn2** spawn2_dump, ZSDump_NPC** npc_dump, ZSDump_NPC_Loot** npcloot_dump, NPCType** gmspawntype_dump, Spawn2*** spawn2_loaded, NPC*** npc_loaded, MYSQL_RES** result);
extern std::list<timers*> TimerList;

bool Zone::Bootup(int32 iZoneID, bool iStaticZone) {
	const char* zonename = database.GetZoneName(iZoneID);
	
	if (iZoneID == 0 || zonename == 0)
		return false;
	srand(time(NULL));
	if (zone != 0 || ZoneLoaded) {
		cerr << "Error: Zone::Bootup call when zone already booted!" << endl;
		worldserver.SetZone(0);
		return false;
	}
	
	numclients = 0;
	zone = new Zone(iZoneID, zonename, net.GetZoneAddress(), net.GetZonePort());

   // Load all NPCs in for the current zone.
   database.GetNPCType (0);

	if (!zone->Init(iStaticZone)) {
		safe_delete(zone);
		cerr << "Zone->Init failed" << endl;
		worldserver.SetZone(0);
		return false;
	}
	if (!eqns.Open(net.GetZonePort())) {
		safe_delete(zone);
		cerr << "eqns.Open failed" << endl;
		worldserver.SetZone(0);
#ifdef GUILDWARS
		exit(1);
#endif
		return false;
	}
	if (!zone->LoadZoneCFG(zone->GetShortName(), true)) // try loading the zone name...
		zone->LoadZoneCFG(zone->GetFileName()); // if that fails, try the file name, then load defaults
	char tmp[10];
	char *tmp2; 
	//PlayerProfile_Struct* pp;
	//int char_num = 0;
	//unsigned long* lengths;
	char errbuf[MYSQL_ERRMSG_SIZE];
	if (database.GetVariable("loglevel",tmp, 9)) {
		int blah[4];
		if (atoi(tmp)>9){ //Server is using the new code
			for(int i=0;i<4;i++){
				if (((int)tmp[i]>=48) && ((int)tmp[i]<=57))
					blah[i]=(int)tmp[i]-48; //get the value to convert it to an int from the ascii value
				else
					blah[i]=0; //set to zero on a bogue char
			}
			zone->loglevelvar = blah[0];
			LogFile->write(EQEMuLog::Status, "General logging level: %i", zone->loglevelvar);
			zone->merchantvar = blah[1];
			LogFile->write(EQEMuLog::Status, "Merchant logging level: %i", zone->merchantvar);
			zone->tradevar = blah[2];
			LogFile->write(EQEMuLog::Status, "Trade logging level: %i", zone->tradevar);
			zone->lootvar = blah[3];
			LogFile->write(EQEMuLog::Status, "Loot logging level: %i", zone->lootvar);
		}
		else {
        	zone->loglevelvar = int8(atoi(tmp)); //continue supporting only command logging (for now)
			zone->merchantvar = 0;
			zone->tradevar = 0;
			zone->lootvar = 0;
		}
	}
	
	// Load zone objects
//	uint32 i = 0;
	char* query = NULL;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	uint32 len_query = MakeAnyLenString(&query, "SELECT "
		"id,zoneid,xpos,ypos,zpos,heading,itemid,charges,objectname,type,icon,"
		"linked_list_addr_01,linked_list_addr_02,unknown08,unknown10,unknown20,"
		"unknown24,unknown60,unknown64,unknown68,unknown72,unknown76,unknown88 "
		"from object where zoneid=%i", iZoneID);
	
	if (database.RunQuery(query, len_query, errbuf, &result)) {
		LogFile->write(EQEMuLog::Status, "Loading Objects from DB...");
		while ((row = mysql_fetch_row(result))) {
			Object_Struct data = {0};
			uint32 id = 0;
			uint32 icon = 0;
			uint32 type = 0;
			uint32 itemid = 0;
			uint32 idx = 0;
			sint8 charges = 0;
			
			id							= (uint32)atoi(row[idx++]);
			data.zone_id				= atoi(row[idx++]);
			data.x						= atof(row[idx++]);
			data.y						= atof(row[idx++]);
			data.z						= atof(row[idx++]);
			data.heading				= atof(row[idx++]);
			itemid						= (uint32)atoi(row[idx++]);
			charges						= (sint8)atoi(row[idx++]);
			strcpy(data.object_name, row[idx++]);
			type						= (int8)atoi(row[idx++]);
			icon						= (uint32)atoi(row[idx++]);
			data.object_type			= (uint32)atoi(row[idx++]);
			data.linked_list_addr[0]	= (uint32)atoi(row[idx++]);
			data.linked_list_addr[1]	= (uint32)atoi(row[idx++]);
			data.unknown008[0]			= (uint32)atoi(row[idx++]);
			data.unknown008[1]			= (uint32)atoi(row[idx++]);
			data.unknown020[0]			= (uint32)atoi(row[idx++]);
			data.unknown020[1]			= (uint32)atoi(row[idx++]);
			data.unknown060[0]			= (uint32)atoi(row[idx++]);
			data.unknown060[1]			= (uint32)atoi(row[idx++]);
			data.unknown060[2]			= (uint32)atoi(row[idx++]);
			data.unknown060[3]			= (uint32)atoi(row[idx++]);
			data.unknown060[4]			= (uint32)atoi(row[idx++]);
			data.unknown084[0]			= (uint32)atoi(row[idx++]);
			
			ItemInst* inst = NULL;
//FatherNitwit: this dosent seem to work...
//tradeskill containers do not have an itemid of 0... at least what I am seeing
			if (itemid == 0) {
				// Generic tradeskill container
				inst = new ItemContainerInst(ItemUseWorldContainer);
			}
			else {
				// Groundspawn object
				inst = ItemInst::Create(itemid);
			}
			
			//Father Nitwit's fix... not perfect...
			if(inst == NULL && type != OT_DROPPEDITEM) {
				inst = new ItemContainerInst(ItemUseWorldContainer);
			}
			
			// Load child objects if container
			if (inst && inst->IsType(ItemTypeContainer)) {
				database.LoadWorldContainer(id, (ItemContainerInst*)inst);
			}
			
			Object* object = new Object(id, type, icon, data, inst);
			entity_list.AddObject(object, false);
			safe_delete(inst);
		}
		mysql_free_result(result);
	}
	else {
		LogFile->write(EQEMuLog::Error, "Error Loading Objects from DB: %s",errbuf);
	}
	safe_delete_array(query);
	Ground_Spawns groundspawn={0};

	int gsindex=0;
	LogFile->write(EQEMuLog::Status, "Loading Ground Spawns from DB...");
	database.LoadGroundSpawns(iZoneID,&groundspawn);
	int32 ix=0;
	char* name=0;
	int32 gsnumber=0;
	for(gsindex=0;gsindex<50;gsindex++){
		if(groundspawn.spawn[gsindex].item>0 && groundspawn.spawn[gsindex].item<500000){
			ItemInst* inst = NULL;
			inst = ItemInst::Create(groundspawn.spawn[gsindex].item);
			gsnumber=groundspawn.spawn[gsindex].max_allowed;
			ix=0;
			if(inst){
				name=groundspawn.spawn[gsindex].name;
				for(ix=0;ix<gsnumber;ix++){
	//				printf("Spawning object %s at %f,%f,%f %f\n",name,gsx,gsy,gsz,gsheading);
					Object* object = new Object(inst,name,groundspawn.spawn[gsindex].max_x,groundspawn.spawn[gsindex].min_x,groundspawn.spawn[gsindex].max_y,groundspawn.spawn[gsindex].min_y,groundspawn.spawn[gsindex].max_z,groundspawn.spawn[gsindex].heading,groundspawn.spawn[gsindex].respawntimer);//new object with id of 10000+
					entity_list.AddObject(object, false);
				}
			}
		}
	}
	zone->LoadZoneDoors(zone->GetShortName());
	if(strncasecmp(zonename,"bazaar",6)==0)
		database.DeleteTraderItem(0);
	
	// Load exp modifier variables.
	// Set default value for EXP modifiers 
		zone->EXPMod = (double)1; 
		zone->GroupEXPBonus = 0.1; 
		zone->AAXPMod = (double)1; 
 
	// Look for database entries and use them if they exist 
		if (database.GetVariable("EXPMod", tmp, 9)) 
			zone->EXPMod = strtod((const char*)tmp, &tmp2); 
		if (database.GetVariable("GroupEXPBonus", tmp, 9)) 
			zone->GroupEXPBonus = strtod((const char*)tmp, &tmp2); 
		if (database.GetVariable("AAXPMod", tmp, 9)) 
			zone->AAXPMod = strtod((const char*)tmp, &tmp2); 

#if (EQDEBUG>=11)
			LogFile->write(EQEMuLog::Debug, "EXPMod set to:%i", zone->EXPMod);
			LogFile->write(EQEMuLog::Debug, "GroupEXPBonus set to:%i", zone->GroupEXPBonus);
			LogFile->write(EQEMuLog::Debug, "AAEXPMod set to:%i", zone->AAXPMod);
#endif
	LogFile->write(EQEMuLog::Normal, "Loading AAs...");
	zone->LoadAAs();
	//g_LogFile.write("AI LEVEL set to %d\n",iAILevel);
	petition_list.ClearPetitions();
	petition_list.ReadDatabase();
	ZoneLoaded = true;

	worldserver.SetZone(iZoneID);
	LogFile->write(EQEMuLog::Normal, "---- Zone server %s, listening on port:%i ----", zonename, net.GetZonePort());
	LogFile->write(EQEMuLog::Status, "Zone Bootup: %s (%i)", zonename, iZoneID);
	UpdateWindowTitle();
	zone->GetTimeSync();

	//This is a bad way of making it set the type to clear on bootup.
	int8 weather=database.GetZoneW(zone->GetZoneID());
	if(weather<1 || weather>3)
		zone->weather_type=1;
	else
		zone->weather_type=weather;
	LogFile->write(EQEMuLog::Debug, "Default weather for zone is:%i", zone->weather_type);
	return true;
}

void Zone::Shutdown(bool quite) {
std::map<uint32,NPCType *>::iterator itr;
	if (!ZoneLoaded)
		return;

   while(zone->npctable.size()) {
   	itr=zone->npctable.begin();
	free(itr->second);
	zone->npctable.erase(itr);
   }

	LogFile->write(EQEMuLog::Status, "Zone Shutdown: %s (%i)", zone->GetShortName(), zone->GetZoneID());
	if (worldserver.Connected()) {
		worldserver.SetZone(0);
	}
	petition_list.ClearPetitions();
	zone->GotCurTime(false);
	if (!quite)
		LogFile->write(EQEMuLog::Normal, "Zone shutdown: going to sleep");
	ZoneLoaded = false;
	char pzs[3] = "";
	if (database.GetVariable("PersistentZoneState", pzs, 2)) {
		if (pzs[0] == '1') {
			Sleep(100);
			LogFile->write(EQEMuLog::Normal, "Saving zone state");
			database.DumpZoneState();
		}
	}
	zone->ResetAuth();
	eqns.Close();
	safe_delete(zone);
	dbasync->CommitWrites();
	UpdateWindowTitle();
}

void Zone::LoadZoneDoors(const char* zone)
{
	LogFile->write(EQEMuLog::Status, "Loading doors for %s ...", zone);
	for(uint32 i=0;i<=database.MaxDoors();i++)
	{
		const Door* door = 0;
		door = database.GetDoorDBID(i);
		if(door == 0 || door->db_id == 0 || strcasecmp(door->zone_name, zone)){
			if(door&&!strcasecmp(door->zone_name, zone))
			LogFile->write(EQEMuLog::Error, "Database errror, Not Sending door: i:%i door:%p db_id:%i zone_name:%s zone:%s door_id:%i",
				i, door, door->db_id, door->zone_name, zone, door->door_id);
			continue;
		}
#if (EQDEBUG>=11)
			LogFile->write(EQEMuLog::Debug, "Sending door:%i", i);
#endif

		Doors* newdoor = new Doors(door);
		if(newdoor)
			entity_list.AddDoor(newdoor);
		else
			LogFile->write(EQEMuLog::Error,"Could not add door to entity list!");
	}
	LogFile->write(EQEMuLog::Status, "Done loading doors for %s ...", zone);
	
}

Zone::Zone(int32 in_zoneid, const char* in_short_name, const char* in_address, int16 in_port) {
	zoneid = in_zoneid;
	zone_weather = 0;
	spawn_group_list = 0;
	map  = Map::LoadMapfile(in_short_name);
	short_name = strcpy(new char[strlen(in_short_name)+1], in_short_name);
	strlwr(short_name);
	memset(file_name, 0, sizeof(file_name));
	long_name = 0;
	aggroedmobs =0;
	address = strcpy(new char[strlen(in_address)+1], in_address);
	port = in_port;

	psafe_x = 0;
	psafe_y = 0;
	psafe_z = 0;
	pMaxClients = 0;

	database.GetZoneLongName(short_name, &long_name, file_name, &psafe_x, &psafe_y, &psafe_z, &pMaxClients);

	if (long_name == 0) {
		long_name = strcpy(new char[18], "Long zone missing");
	}
	autoshutdown_timer = new Timer(ZONE_AUTOSHUTDOWN_DELAY);
	autoshutdown_timer->Start(AUTHENTICATION_TIMEOUT * 1000, false);
	Weather_Timer = new Timer(((rand()%7200-30)+30)*2000);
	Weather_Timer->Start();
	LogFile->write(EQEMuLog::Status, "Weather should change in %i seconds",Weather_Timer->GetRemainingTime()/1000);
	clientauth_timer = new Timer(AUTHENTICATION_TIMEOUT * 1000);
	zone_weather=weather_type-1;
#ifdef GUILDWARS
	database.LoadLocationInformation();
	guildwars.SetCurrentZone(in_zoneid);
	guildwars.Construct();
	db_update = new Timer(600000);
#endif
}

//Modified for timezones.
bool Zone::Init(bool iStaticZone) {
	SetStaticZone(iStaticZone);
	spawn_group_list = new SpawnGroupList();
	cout << "Init: Loading zone lists";
	if (!database.PopulateZoneLists(short_name, &zone_point_list, spawn_group_list))
	{
		cout << "ERROR: Couldn't load zone lists." << endl;
		return false;
	}
	char pzs[3] = "";
	cout << ", zone state or spawn list";
	if (database.GetVariable("PersistentZoneState", pzs, 2)) {
		if (pzs[0] == '1') {
			sint8 tmp = database.LoadZoneState(short_name, spawn2_list);
			if (tmp == 1) {
				cout << "Zone state loaded." << endl;
			}
			else if (tmp == -1) {
				cout << "Error loading zone state" << endl;
				return false;
			}
			else if (tmp == 0) {
				cout << "Zone state not found, loading from spawn lists." << endl;
				if (!database.PopulateZoneSpawnList(short_name, spawn2_list))
					return false;
			}
			else
				cout << "Unknown LoadZoneState return value" << endl;
		}
		else if (!database.PopulateZoneSpawnList(short_name, spawn2_list))
		{
			cout << "ERROR: Couldn't populate zone spawn list." << endl;
			return false;
		}
	}
	else if (!database.PopulateZoneSpawnList(short_name, spawn2_list))
	{
		cout << "Couldn't populate zone spawn list." << endl;
		return false;
	}
	cout << ", player corpses\n";
	if (!database.LoadPlayerCorpses(zoneid))
	{
		cout << "ERROR: Couldn't load player corpses." << endl;
		return false;
	}

	parse->ClearCache();
	cout << ", timezone data";
	zone->zone_time.setEQTimeZone(database.GetZoneTZ(zoneid));
	cout << " - Done. ZoneID = " << zoneid << "; Time Offset = " << zone->zone_time.getEQTimeZone() << endl;
	return true;
}

Zone::~Zone()
{
	spawn2_list.Clear();
	safe_delete(map);
	safe_delete(spawn_group_list);
	if (worldserver.Connected()) {
		worldserver.SetZone(0);
	}
	safe_delete_array(short_name);
	safe_delete_array(long_name);
	safe_delete_array(address);
	safe_delete(autoshutdown_timer);
	safe_delete(clientauth_timer);
	safe_delete(Weather_Timer);
	zone_point_list.Clear();
	entity_list.Clear();
	safe_delete(aa_buffer);
#ifdef GUILDWARS
	location_list.ClearLocations();
	guildwars.Deconstruct();
	safe_delete(db_update);
#endif
}
bool Zone::LoadZoneCFG(const char* filename, bool DontLoadDefault) {
	memset(&newzone_data, 0, sizeof(NewZone_Struct));
	NewZone_Struct* nsc = database.GetZoneCFG(database.GetZoneID(filename));
	if(nsc){
		memcpy(&newzone_data,nsc,sizeof(NewZone_Struct));
		strcpy(newzone_data.zone_short_name, GetShortName());
		strcpy(newzone_data.zone_long_name, GetLongName());
		strcpy(newzone_data.zone_short_name2,GetShortName());
		safe_delete(nsc);
		cout << "Successfully loaded Zone Config.\n";
		return true;
	}
	cout << "Error while loading Zone Config!\n";
	cout << "IF YOU HAVENT DONE SO, SOURCE THE ZONECFG.SQL FILE!!!!!!\n";
	cout << "IF YOU HAVENT DONE SO, SOURCE THE ZONECFG.SQL FILE!!!!!!\n";
	cout << "IF YOU HAVENT DONE SO, SOURCE THE ZONECFG.SQL FILE!!!!!!\n";
	cout << "IF YOU HAVENT DONE SO, SOURCE THE ZONECFG.SQL FILE!!!!!!\n";
	cout << "IF YOU HAVENT DONE SO, SOURCE THE ZONECFG.SQL FILE!!!!!!\n";
	cout << "IF YOU HAVENT DONE SO, SOURCE THE ZONECFG.SQL FILE!!!!!!\n";
	cout << "IF YOU HAVENT DONE SO, SOURCE THE ZONECFG.SQL FILE!!!!!!\n";
	cout << "IF YOU HAVENT DONE SO, SOURCE THE ZONECFG.SQL FILE!!!!!!\n";
	cout << "IF YOU HAVENT DONE SO, SOURCE THE ZONECFG.SQL FILE!!!!!!\n";
	cout << "IF YOU HAVENT DONE SO, SOURCE THE ZONECFG.SQL FILE!!!!!!\n";
	cout << "IF YOU HAVENT DONE SO, SOURCE THE ZONECFG.SQL FILE!!!!!!\n";
	return false;
}

bool Zone::SaveZoneCFG() {
	return database.SaveZoneCFG(GetZoneID(),&newzone_data);
}

void Zone::AddAuth(ServerZoneIncommingClient_Struct* szic) {
	ZoneClientAuth_Struct* zca = new ZoneClientAuth_Struct;
	memset(zca, 0, sizeof(ZoneClientAuth_Struct));
	zca->ip = szic->ip;
	zca->wid = szic->wid;
	zca->accid = szic->accid;
	zca->admin = szic->admin;
	zca->charid = szic->charid;
	zca->tellsoff = szic->tellsoff;
	strn0cpy(zca->charname, szic->charname, sizeof(zca->charname));
	strn0cpy(zca->lskey, szic->lskey, sizeof(zca->lskey));
	zca->stale = false;
	client_auth_list.Insert(zca);
}

void Zone::RemoveAuth(const char* iCharName)
{
	LinkedListIterator<ZoneClientAuth_Struct*> iterator(client_auth_list);

	iterator.Reset();
	while (iterator.MoreElements()) {
		ZoneClientAuth_Struct* zca = iterator.GetData();
		if (strcasecmp(zca->charname, iCharName) == 0) {
		iterator.RemoveCurrent();
		return;
		}
		iterator.Advance();
	}
}

void Zone::ResetAuth()
{
	LinkedListIterator<ZoneClientAuth_Struct*> iterator(client_auth_list);

	iterator.Reset();
	while (iterator.MoreElements()) {
		iterator.RemoveCurrent();
	}
}

bool Zone::GetAuth(int32 iIP, const char* iCharName, int32* oWID, int32* oAccID, int32* oCharID, sint16* oStatus, char* oLSKey, bool* oTellsOff) {
	LinkedListIterator<ZoneClientAuth_Struct*> iterator(client_auth_list);

	iterator.Reset();
	while (iterator.MoreElements()) {
		ZoneClientAuth_Struct* zca = iterator.GetData();
		if (strcasecmp(zca->charname, iCharName) == 0) {
				if(oWID)
				*oWID = zca->wid;
				if(oAccID)
				*oAccID = zca->accid;
				if(oCharID)
				*oCharID = zca->charid;
				if(oStatus)
				*oStatus = zca->admin;
				if(oTellsOff)
				*oTellsOff = zca->tellsoff;
				zca->stale = true;
			return true;
		}
		iterator.Advance();
	}
	return false;
}

int32 Zone::CountAuth() {
	LinkedListIterator<ZoneClientAuth_Struct*> iterator(client_auth_list);

	int x = 0;
	iterator.Reset();
	while (iterator.MoreElements()) {
		x++;
		iterator.Advance();
	}
	return x;
}

bool Zone::Process() {
	LockMutex lock(&MZoneLock);
	LinkedListIterator<Spawn2*> iterator(spawn2_list);

	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->Process()) {
			iterator.Advance();
		}
		else {
			iterator.RemoveCurrent();
		}
	}
	list<timers*>::iterator iterator1 = TimerList.begin();
	timers*p=0;
	while (iterator1 != TimerList.end())
	{
		p=*iterator1;
		if (p && p->Timer_->Enabled() && p->Timer_->Check())
		{
			if (p->mob)
				parse->Event(EVENT_TIMER,p->mob->GetNPCTypeID(),p->name.c_str(),p->mob,0);			
		}
		iterator1++;
	}
	if(!staticzone) {
		if (autoshutdown_timer->Check()) {
			StartShutdownTimer();
			if (numclients == 0) {
				return false;
			}
		}
	}
#ifdef GUILDWARS
	if(db_update->Check())
	guildwars.Update();
#endif
	if(Weather_Timer->Check()){
		Weather_Timer->Disable();
		int16 tmpweather =rand()%100;
		int8 type=weather_type-1;
		//0 normal, 1 rainy, 2 snowy
		if(type!=2){
			if(zone_weather==type && tmpweather>80)
				zone_weather=2;
			else if(zone_weather==type)
				zone_weather=type+1;
			else
				zone_weather=type; //default to zones default
		}
		else{
			if(zone_weather==2 && tmpweather>80)
				zone_weather=1;
			else if(zone_weather==2)
				zone_weather=0;
			else
				zone_weather=2; //default to zones default
		}
		this->weatherSend();
		safe_delete(Weather_Timer);
		if(zone_weather==type) //stopping, reset to large timer
			Weather_Timer= new Timer(((rand()%(7170))+30)*2000);
		else
			Weather_Timer= new Timer(((rand()%(570))+30)*1000);
		Weather_Timer->Start();
		LogFile->write(EQEMuLog::Status, "Weather should change in %i seconds",Weather_Timer->GetRemainingTime()/1000);
	}
	if (clientauth_timer->Check()) {
		LinkedListIterator<ZoneClientAuth_Struct*> iterator2(client_auth_list);

		iterator2.Reset();
		while (iterator2.MoreElements()) {
			if (iterator2.GetData()->stale)
				iterator2.RemoveCurrent();
			else {
				iterator2.GetData()->stale = true;
				iterator2.Advance();
			}
		}
	}

	return true;

}

void Zone::StartShutdownTimer(int32 set_time) {
	MZoneLock.lock();
	if (set_time > autoshutdown_timer->GetRemainingTime()) {
		autoshutdown_timer->Start(set_time, false);
	}
	MZoneLock.unlock();
}

bool Zone::Depop() {
std::map<uint32,NPCType *>::iterator itr;
	entity_list.Depop();

   // Refresh npctable, getting current info from database.
   while(npctable.size()) {
   	itr=npctable.begin();
	free(itr->second);
	npctable.erase(itr);
   }
   database.GetNPCType (0);

	return true;
}

void Zone::Repop(int32 delay) {
	
	if(Depop())
	{
	entity_list.Message(0, 0, "<SYSTEM_MSG>:Zone REPOP IMMINENT");

	LinkedListIterator<Spawn2*> iterator(spawn2_list);

	MZoneLock.lock();
	iterator.Reset();
	while (iterator.MoreElements())
	{
		iterator.RemoveCurrent();
	}

	if (!database.PopulateZoneSpawnList(short_name, spawn2_list, delay)) {
		entity_list.Message(0, 0, "Error in Zone::Repop: database.PopulateZoneSpawnList failed");
	}

	MZoneLock.unlock();
	}
}

void Zone::GetTimeSync()
{
	if (worldserver.Connected() && !gottime) {
		ServerPacket* pack = new ServerPacket;
		pack->size = 0;
		pack->opcode = ServerOP_GetWorldTime;
		pack->pBuffer = new uchar[pack->size];
		worldserver.SendPacket(pack);
		safe_delete(pack);
	}
}

void Zone::SetDate(int16 year, int8 month, int8 day, int8 hour, int8 minute)
{
	if (worldserver.Connected()) {
		ServerPacket* pack = new ServerPacket;
		pack->size = sizeof(eqTimeOfDay);
		pack->opcode = ServerOP_SetWorldTime;
		pack->pBuffer = new uchar[pack->size];
		eqTimeOfDay* eqtod = (eqTimeOfDay*)pack->pBuffer;
		eqtod->start_eqtime.minute=minute;
		eqtod->start_eqtime.hour=hour;
		eqtod->start_eqtime.day=day;
		eqtod->start_eqtime.month=month;
		eqtod->start_eqtime.year=year;
		eqtod->start_realtime=time(0);
		printf("Setting master date on world server to: %d-%d-%d %d:%d (%d)\n", year, month, day, hour, minute, (int)eqtod->start_realtime);
		worldserver.SendPacket(pack);
		safe_delete(pack);
	}
}

void Zone::SetTime(int8 hour, int8 minute)
{
	if (worldserver.Connected()) {
		ServerPacket* pack = new ServerPacket;
		pack->size = sizeof(eqTimeOfDay);
		pack->opcode = ServerOP_SetWorldTime;
		pack->pBuffer = new uchar[pack->size];
		eqTimeOfDay* eqtod = (eqTimeOfDay*)pack->pBuffer;
		zone_time.getEQTimeOfDay(time(0), &eqtod->start_eqtime);
		eqtod->start_eqtime.minute=minute;
		eqtod->start_eqtime.hour=hour;
		eqtod->start_realtime=time(0);
		printf("Setting master time on world server to: %d:%d (%d)\n", hour, minute, (int)eqtod->start_realtime);
		worldserver.SendPacket(pack);
		safe_delete(pack);
	}
}

ZonePoint* Zone::GetClosestZonePoint(float x, float y, float z, int32 to) {
	return GetClosestZonePoint(x, y, z, database.GetZoneName(to));
}

ZonePoint* Zone::GetClosestZonePoint(float x, float y, float z, const char* to_name) {
	LinkedListIterator<ZonePoint*> iterator(zone_point_list);
	ZonePoint* closest_zp = 0;
	float closest_dist = FLT_MAX;
	iterator.Reset();
	while(iterator.MoreElements())
	{
		ZonePoint* zp = iterator.GetData();
		if (to_name != 0 && zp->target_zone != 0 && strcmp(zp->target_zone, to_name) == 0)
		{
			float delta_x = zp->x - x;
			float delta_y = zp->y - y;

			float dist = sqrt(delta_x*delta_x+delta_y*delta_y);///*+(zp->z-z)*(zp->z-z)*/;
			if (dist < closest_dist)
			{
				closest_zp = zp;
				closest_dist = dist;
			}
		}
		iterator.Advance();
	}
	// z fix
	if(closest_dist>200.0f && closest_dist<50000.0f)
		LogFile->write(EQEMuLog::Status, "WARNING: Closest zone point for %s is %f, you might need to update your zone_points table if you dont arrive at the right spot.",to_name,closest_dist);
	//closest_zp->target_z = closest_zp->target_z;
	if(!closest_zp)
	closest_zp = GetClosestZonePointWithoutZone(x,y,z);

	return closest_zp;
}

ZonePoint* Zone::GetClosestZonePointWithoutZone(float x, float y, float z) {
	LinkedListIterator<ZonePoint*> iterator(zone_point_list);
	ZonePoint* closest_zp = 0;
	float closest_dist = FLT_MAX;
	iterator.Reset();
	while(iterator.MoreElements())
	{
		ZonePoint* zp = iterator.GetData();
			float delta_x = zp->x - x;
			float delta_y = zp->y - y;

			float dist = sqrt(delta_x*delta_x+delta_y*delta_y);///*+(zp->z-z)*(zp->z-z)*/;
			if (dist < closest_dist)
			{
				closest_zp = zp;
				closest_dist = dist;
			}
		iterator.Advance();
	}
	if(closest_dist > 200.0f)
		closest_zp = 0;

	return closest_zp;
}

bool Database::LoadStaticZonePoints(LinkedList<ZonePoint*>* zone_point_list,const char* zonename)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	zone_point_list->Clear();
	zone->numzonepoints = 0;
	MakeAnyLenString(&query, "SELECT x,y,z,target_x,target_y,target_z,target_zone,heading,target_heading,number FROM zone_points WHERE zone='%s' order by number", zonename);
	if (RunQuery(query, strlen(query), errbuf, &result))
	{
		safe_delete_array(query);
		while((row = mysql_fetch_row(result)))
		{
			ZonePoint* zp = new ZonePoint;
			zp->x = atof(row[0]);
			zp->y = atof(row[1]);
			zp->z = atof(row[2]);
			zp->target_x = atof(row[3]);
			zp->target_y = atof(row[4]);
			zp->target_z = atof(row[5]);
			strncpy(zp->target_zone, row[6], 16);
			zp->heading=atof(row[7]);
			zp->target_heading=atof(row[8]);
			zp->number=atoi(row[9]);
			zone_point_list->Insert(zp);
			zone->numzonepoints++;
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error1 in PopulateZoneLists query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
return true;
}

//bool Database::PopulateZoneLists(char* zone_name, NPCType* &npc_type_array, uint32 &max_npc_type, LinkedList<Spawn*>& spawn_list, LinkedList<ZonePoint*>& zone_point_list, LinkedList<Spawn2*> &spawn2_list, SpawnGroupList* spawn_group_list)
//bool Database::PopulateZoneLists(char* zone_name, NPCType* &npc_type_array, uint32 &max_npc_type, LinkedList<ZonePoint*>& zone_point_list, SpawnGroupList* spawn_group_list)
bool Database::PopulateZoneLists(const char* zone_name, LinkedList<ZonePoint*>* zone_point_list, SpawnGroupList* spawn_group_list) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	if(!database.LoadStaticZonePoints(zone_point_list,zone_name))
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
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT spawnentry.spawngroupID, npcid, chance from spawnentry, spawn2 where spawnentry.spawngroupID=spawn2.spawngroupID and zone='%s' ORDER by chance", zone_name), errbuf, &result)) {
		safe_delete_array(query);
		while((row = mysql_fetch_row(result)))
		{
			SpawnEntry* newSpawnEntry = new SpawnEntry( atoi(row[1]), atoi(row[2]));
			if (spawn_group_list->GetSpawnGroup(atoi(row[0])))
				spawn_group_list->GetSpawnGroup(atoi(row[0]))->AddSpawnEntry(newSpawnEntry);
			else
				cout << "Error in SpawngroupID: " << atoi(row[0]) << endl;
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
		cerr << "Error in PopulateZoneLists query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return true;
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

bool Database::DumpZoneState() {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;

	if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM zone_state_dump WHERE zonename='%s'", zone->GetShortName()), errbuf)) {
		cerr << "Error in DumpZoneState query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	safe_delete_array(query);



	int32	spawn2_count = zone->CountSpawn2();
	int32	npc_count = 0;
	int32	npcloot_count = 0;
	int32	gmspawntype_count = 0;
	entity_list.CountNPC(&npc_count, &npcloot_count, &gmspawntype_count);

	cout << "DEBUG: spawn2count=" << spawn2_count << ", npc_count=" << npc_count << ", npcloot_count=" << npcloot_count << ", gmspawntype_count=" << gmspawntype_count << endl;

	ZSDump_Spawn2* spawn2_dump = 0;
	ZSDump_NPC*	npc_dump = 0;
	ZSDump_NPC_Loot* npcloot_dump = 0;
	NPCType* gmspawntype_dump = 0;
	if (spawn2_count > 0) {
		spawn2_dump = (ZSDump_Spawn2*) new uchar[spawn2_count * sizeof(ZSDump_Spawn2)];
		memset(spawn2_dump, 0, sizeof(ZSDump_Spawn2) * spawn2_count);
	}
	if (npc_count > 0) {
		npc_dump = (ZSDump_NPC*) new uchar[npc_count * sizeof(ZSDump_NPC)];
		memset(npc_dump, 0, sizeof(ZSDump_NPC) * npc_count);
		for (unsigned int i=0; i < npc_count; i++) {
			npc_dump[i].spawn2_dump_index = 0xFFFFFFFF;
			npc_dump[i].gmspawntype_index = 0xFFFFFFFF;
		}
	}
	if (npcloot_count > 0) {
		npcloot_dump = (ZSDump_NPC_Loot*) new uchar[npcloot_count * sizeof(ZSDump_NPC_Loot)];
		memset(npcloot_dump, 0, sizeof(ZSDump_NPC_Loot) * npcloot_count);
		for (unsigned int k=0; k < npcloot_count; k++)
			npcloot_dump[k].npc_dump_index = 0xFFFFFFFF;
	}
	if (gmspawntype_count > 0) {
		gmspawntype_dump = (NPCType*) new uchar[gmspawntype_count * sizeof(NPCType)];
		memset(gmspawntype_dump, 0, sizeof(NPCType) * gmspawntype_count);
	}

	entity_list.DoZoneDump(spawn2_dump, npc_dump, npcloot_dump, gmspawntype_dump);
    query = new char[512 + ((sizeof(ZSDump_Spawn2) * spawn2_count + sizeof(ZSDump_NPC) * npc_count + sizeof(ZSDump_NPC_Loot) * npcloot_count + sizeof(NPCType) * gmspawntype_count) * 2)];
	char* end = query;

    end += sprintf(end, "Insert Into zone_state_dump (zonename, spawn2_count, npc_count, npcloot_count, gmspawntype_count, spawn2, npcs, npc_loot, gmspawntype) values ('%s', %i, %i, %i, %i, ", zone->GetShortName(), spawn2_count, npc_count, npcloot_count, gmspawntype_count);
    *end++ = '\'';
	if (spawn2_dump != 0) {
		end += DoEscapeString(end, (char*)spawn2_dump, sizeof(ZSDump_Spawn2) * spawn2_count);
		safe_delete_array(spawn2_dump);
	}
    *end++ = '\'';
    end += sprintf(end, ", ");
    *end++ = '\'';
	if (npc_dump != 0) {
		end += DoEscapeString(end, (char*)npc_dump, sizeof(ZSDump_NPC) * npc_count);
		safe_delete_array(npc_dump);
	}
    *end++ = '\'';
    end += sprintf(end, ", ");
    *end++ = '\'';
	if (npcloot_dump != 0) {
		end += DoEscapeString(end, (char*)npcloot_dump, sizeof(ZSDump_NPC_Loot) * npcloot_count);
		safe_delete_array(npcloot_dump);
	}
    *end++ = '\'';
    end += sprintf(end, ", ");
    *end++ = '\'';
	if (gmspawntype_dump != 0) {
		end += DoEscapeString(end, (char*)gmspawntype_dump, sizeof(NPCType) * gmspawntype_count);
		safe_delete_array(gmspawntype_dump);
	}
    *end++ = '\'';
    end += sprintf(end, ")");

	int32 affected_rows = 0;
	if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows)) {
		//    if (DoEscapeString(query, (unsigned int) (end - query))) {
		safe_delete_array(query);
        cerr << "Error in ZoneDump query " << errbuf << endl;
		return false;
    }
	safe_delete_array(query);

	if (affected_rows == 0) {
		cerr << "Zone dump failed. (affected rows = 0)" << endl;
		return false;
	}
	return true;
}

sint8 Database::LoadZoneState(const char* zonename, LinkedList<Spawn2*>& spawn2_list) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	int32 i;
	unsigned long* lengths;
	int32	elapsedtime = 0;
	int32	spawn2_count = 0;
	int32	npc_count = 0;
	int32	npcloot_count = 0;
	int32	gmspawntype_count = 0;
	ZSDump_Spawn2* spawn2_dump = 0;
	ZSDump_NPC*	npc_dump = 0;
	ZSDump_NPC_Loot* npcloot_dump = 0;
	NPCType* gmspawntype_dump = 0;
	Spawn2** spawn2_loaded = 0;
	NPC** npc_loaded = 0;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT spawn2_count, npc_count, npcloot_count, gmspawntype_count, spawn2, npcs, npc_loot, gmspawntype, (UNIX_TIMESTAMP()-UNIX_TIMESTAMP(time)) as elapsedtime FROM zone_state_dump WHERE zonename='%s'", zonename), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			cout << "Elapsed time: " << row[8] << endl;
			elapsedtime = atoi(row[8]) * 1000;
			lengths = mysql_fetch_lengths(result);
			spawn2_count = atoi(row[0]);
			cout << "Spawn2count: " << spawn2_count << endl;
			if (lengths[4] != (sizeof(ZSDump_Spawn2) * spawn2_count)) {
				cerr << "Error in LoadZoneState: spawn2_dump length mismatch l=" << lengths[4] << ", e=" << (sizeof(ZSDump_Spawn2) * spawn2_count) << endl;
				CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
				return -1;
			}
			else if (spawn2_count > 0) {
				spawn2_dump = new ZSDump_Spawn2[spawn2_count];
				spawn2_loaded = new Spawn2*[spawn2_count];
				memcpy(spawn2_dump, row[4], lengths[4]);
				for (i=0; i < spawn2_count; i++) {
					if (spawn2_dump[i].time_left == 0xFFFFFFFF) // npc spawned, timer should be disabled
						spawn2_loaded[i] = LoadSpawn2(spawn2_list, spawn2_dump[i].spawn2_id, 0xFFFFFFFF);
					else if (spawn2_dump[i].time_left <= elapsedtime)
						spawn2_loaded[i] = LoadSpawn2(spawn2_list, spawn2_dump[i].spawn2_id, 0);
					else
						spawn2_loaded[i] = LoadSpawn2(spawn2_list, spawn2_dump[i].spawn2_id, spawn2_dump[i].time_left - elapsedtime);
					if (spawn2_loaded[i] == 0) {
						cerr << "Error in LoadZoneState: spawn2_loaded[" << i << "] == 0" << endl;
						CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
						return -1;

					}
				}
			}

			gmspawntype_count = atoi(row[3]);
			cout << "gmspawntype_count: " << gmspawntype_count << endl;
			if (lengths[7] != (sizeof(NPCType) * gmspawntype_count)) {
				cerr << "Error in LoadZoneState: gmspawntype_dump length mismatch l=" << lengths[7] << ", e=" << (sizeof(NPCType) * gmspawntype_count) << endl;
				CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
				return -1;
			}
			else if (gmspawntype_count > 0) {
				gmspawntype_dump = new NPCType[gmspawntype_count];
				memcpy(gmspawntype_dump, row[7], lengths[7]);
			}

			npc_count = atoi(row[1]);
			cout << "npc_count: " << npc_count << endl;
			if (lengths[5] != (sizeof(ZSDump_NPC) * npc_count)) {
				cerr << "Error in LoadZoneState: npc_dump length mismatch l=" << lengths[5] << ", e=" << (sizeof(ZSDump_NPC) * npc_count) << endl;
				CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
				return -1;
			}
			else if (npc_count > 0) {
				npc_dump = new ZSDump_NPC[npc_count];
				npc_loaded = new NPC*[npc_count];
				for (i=0; i < npc_count; i++) {
					npc_loaded[i] = 0;
				}
				memcpy(npc_dump, row[5], lengths[5]);
				for (i=0; i < npc_count; i++) {
					if (npc_loaded[i] != 0) {
						cerr << "Error in LoadZoneState: npc_loaded[" << i << "] != 0" << endl;
						CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
						return -1;
					}
					Spawn2* tmp = 0;
					if (!npc_dump[i].corpse && npc_dump[i].spawn2_dump_index != 0xFFFFFFFF) {
						if (spawn2_loaded == 0 || npc_dump[i].spawn2_dump_index >= spawn2_count) {
							cerr << "Error in LoadZoneState: (spawn2_loaded == 0 || index >= count) && npc_dump[" << i << "].spawn2_dump_index != 0xFFFFFFFF" << endl;
							CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
							return -1;
						}
						tmp = spawn2_loaded[npc_dump[i].spawn2_dump_index];
						spawn2_loaded[npc_dump[i].spawn2_dump_index] = 0;
					}
					if (npc_dump[i].npctype_id == 0) {
						if (npc_dump[i].gmspawntype_index == 0xFFFFFFFF) {
							cerr << "Error in LoadZoneState: gmspawntype index invalid" << endl;
							safe_delete(tmp);

							CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
							return -1;
						}
						else {
							if (gmspawntype_dump == 0 || npc_dump[i].gmspawntype_index >= gmspawntype_count) {
								cerr << "Error in LoadZoneState: (gmspawntype_dump == 0 || index >= count) && npc_dump[" << i << "].npctype_id == 0" << endl;
								safe_delete(tmp);

								CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
								return -1;
							}
							npc_loaded[i] = new NPC(&gmspawntype_dump[npc_dump[i].gmspawntype_index], tmp, npc_dump[i].x, npc_dump[i].y, npc_dump[i].z, npc_dump[i].heading, npc_dump[i].corpse);
						}
					}
					else {
						const NPCType* crap = database.GetNPCType(npc_dump[i].npctype_id);
						if (crap != 0)
							npc_loaded[i] = new NPC(crap, tmp, npc_dump[i].x, npc_dump[i].y, npc_dump[i].z, npc_dump[i].heading, npc_dump[i].corpse);
						else {
							cerr << "Error in LoadZoneState: Unknown npctype_id: " << npc_dump[i].npctype_id << endl;
							safe_delete(tmp);
						}
					}
					if (npc_loaded[i] != 0) {
						npc_loaded[i]->AddCash(npc_dump[i].copper, npc_dump[i].silver, npc_dump[i].gold, npc_dump[i].platinum);
						//							if (npc_dump[i].corpse) {
						//								if (npc_dump[i].decay_time_left <= elapsedtime)
						//									npc_loaded[i]->SetDecayTimer(0);
						//								else
						//									npc_loaded[i]->SetDecayTimer(npc_dump[i].decay_time_left - elapsedtime);
						//							}
						entity_list.AddNPC(npc_loaded[i]);
					}
				}
			}

			npcloot_count = atoi(row[2]);
			cout << "npcloot_count: " << npcloot_count << endl;
			if (lengths[6] != (sizeof(ZSDump_NPC_Loot) * npcloot_count)) {
				cerr << "Error in LoadZoneState: npcloot_dump length mismatch l=" << lengths[6] << ", e=" << (sizeof(ZSDump_NPC_Loot) * npcloot_count) << endl;
				CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
				return -1;
			}
			else if (npcloot_count > 0) {
				if (npc_loaded == 0) {
					cerr << "Error in LoadZoneState: npcloot_count > 0 && npc_loaded == 0" << endl;
					CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
					return -1;
				}
				npcloot_dump = new ZSDump_NPC_Loot[npcloot_count];
				memcpy(npcloot_dump, row[6], lengths[6]);
				for (i=0; i < npcloot_count; i++) {
					if (npcloot_dump[i].npc_dump_index >= npc_count) {
						cerr << "Error in LoadZoneState: npcloot_dump[" << i << "].npc_dump_index >= npc_count" << endl;
						CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
						return -1;
					}
					if (npc_loaded[npcloot_dump[i].npc_dump_index] != 0) {
						npc_loaded[npcloot_dump[i].npc_dump_index]->AddItem(npcloot_dump[i].itemid, npcloot_dump[i].charges, npcloot_dump[i].equipSlot);
					}
				}
			}
			CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
		}
		else {
			CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
			return 0;
		}
		CleanupLoadZoneState(spawn2_count, &spawn2_dump, &npc_dump, &npcloot_dump, &gmspawntype_dump, &spawn2_loaded, &npc_loaded, &result);
	}
	else {
		cerr << "Error in LoadZoneState query '" << query << "' " << errbuf << endl;

		safe_delete_array(query);
		return -1;
	}

	return 1;
}

void CleanupLoadZoneState(int32 spawn2_count, ZSDump_Spawn2** spawn2_dump, ZSDump_NPC** npc_dump, ZSDump_NPC_Loot** npcloot_dump, NPCType** gmspawntype_dump, Spawn2*** spawn2_loaded, NPC*** npc_loaded, MYSQL_RES** result) {
	safe_delete(*spawn2_dump);
	safe_delete(*spawn2_loaded);
	safe_delete(*gmspawntype_dump);
	safe_delete(*npc_dump);
	safe_delete(*npc_loaded);
	safe_delete(*npcloot_dump);
	if (*result) {
		mysql_free_result(*result);
		*result = 0;
	}
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

	cerr << "Error in LoadSpawn2 query '" << query << "' " << errbuf << endl;
	safe_delete_array(query);
	return 0;
}

void Zone::SpawnStatus(Mob* client) {
	LinkedListIterator<Spawn2*> iterator(spawn2_list);

	int32 x = 0;
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->timer->GetRemainingTime() == 0xFFFFFFFF)
			client->Message(0, "  %d:  %1.1f, %1.1f, %1.1f:  disabled", iterator.GetData()->GetID(), iterator.GetData()->GetX(), iterator.GetData()->GetY(), iterator.GetData()->GetZ());
		else
			client->Message(0, "  %d:  %1.1f, %1.1f, %1.1f:  %1.2f", iterator.GetData()->GetID(), iterator.GetData()->GetX(), iterator.GetData()->GetY(), iterator.GetData()->GetZ(), (float)iterator.GetData()->timer->GetRemainingTime() / 1000);

		x++;
		iterator.Advance();
	}
	client->Message(0, "%i spawns listed.", x);
}

bool Zone::RemoveSpawnEntry(uint32 spawnid)
{
	LinkedListIterator<Spawn2*> iterator(spawn2_list);


	iterator.Reset();
	while(iterator.MoreElements())
	{
		if(iterator.GetData()->GetID() == spawnid)
		{
			iterator.RemoveCurrent();
			return true;
		}
		else
		iterator.Advance();
	}
return false;
}

bool Zone::RemoveSpawnGroup(uint32 in_id) {
if(spawn_group_list->RemoveSpawnGroup(in_id))
return true;
else
return false;
}


// Added By Hogie
bool Database::GetDecayTimes(npcDecayTimes_Struct* npcCorpseDecayTimes) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	int i = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT varname, value FROM variables WHERE varname like 'decaytime%%' ORDER BY varname"), errbuf, &result)) {
		safe_delete_array(query);
		while((row = mysql_fetch_row(result))) {
			Seperator sep(row[0]);
			npcCorpseDecayTimes[i].minlvl = atoi(sep.arg[1]);
			npcCorpseDecayTimes[i].maxlvl = atoi(sep.arg[2]);
			if (atoi(row[1]) > 7200)
				npcCorpseDecayTimes[i].seconds = 720;
			else
				npcCorpseDecayTimes[i].seconds = atoi(row[1]);
			i++;
		}
		mysql_free_result(result);
	}
	else {
		safe_delete_array(query);
		return false;
	}
	return true;
}// Added By Hogie -- End


/*void Zone::weatherProc()
{
	if(time(0)>=weather_timer && weather_type != 0x00)
	{
		if(zone_weather==0)
			weather_timer=time(0)+(rand()%(600-30))+30;
		else
			weather_timer=time(0)+(rand()%(3600-30))+30;
		//weather_timer=time(0)+15;
		cout << "Weather changes in " << weather_timer-time(0) << " seconds. (weather is now " << (long)zone_weather << ")" << endl;
		if(zone_weather>0)
			zone_weather=0;
		else
			zone_weather=weather_type;
		weatherSend();
	}
}
*/
void Zone::weatherSend()
{
	/*switch(zone_weather)
	{
	case 0:
		entity_list.Message(0, 0, "The sky clears.");
		break;
	case 1:
		entity_list.Message(0, 0, "Raindrops begin to fall from the sky.");
		break;
	case 2:
		entity_list.Message(0, 0, "Snowflakes begin to fall from the sky.");
		break;
	default:
		entity_list.Message(0, 0, "Strange weather patterns form in the sky. (%i)", zone_weather);
		break;
	}*/
	APPLAYER* outapp = new APPLAYER(OP_Weather, 8);
	if(zone_weather>0)
		outapp->pBuffer[0] = zone_weather-1;
	if(zone_weather>0)
		outapp->pBuffer[4] = 0x10+(rand()%10); // This number changes in the packets, intensity?
	entity_list.QueueClients(0, outapp);
	safe_delete(outapp);
}
