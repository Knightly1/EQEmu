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
#ifndef ZONE_H
#define ZONE_H

//#define ZONE_AUTOSHUTDOWN_DELAY		5000

#include "../common/Mutex.h"
#include "../common/linked_list.h"
#include "../common/types.h"
#include "../common/eqtime.h"
#include "../common/servertalk.h"
#include "features.h"
#include "spawngroup.h"
#include "mob.h"
#include "zonedump.h"

class Map;
struct ZonePoint {
	float x;
	float y;
	float z;
	float heading;
	int16 number;
	float target_x;
	float target_y;
	float target_z;
	float target_heading;
	char  target_zone[16];
};
struct ZoneClientAuth_Struct {
	int32	ip;			// client's IP address
	int32	wid;		// client's WorldID#
	int32	accid;
	sint16	admin;
	int32	charid;
	bool	tellsoff;
	char	charname[64];
	char	lskey[30];
	bool	stale;
};

extern EntityList entity_list;
class database;
class FearPathManager;

class database;

class Zone
{
public:
	static bool Zone::Bootup(int32 iZoneID, bool iStaticZone = false);
	static void Zone::Shutdown(bool quite = false);
	
	Zone(int32 in_zoneid, const char* in_short_name, const char* in_address, int16 in_port);
	~Zone();
	bool	Init(bool iStaticZone);
	bool	LoadZoneCFG(const char* filename, bool DontLoadDefault = false);
	bool	SaveZoneCFG();
	bool	IsLoaded();
	bool	IsPVPZone() { return pvpzone; }
	inline const char*	GetAddress()	{ return address; }
	inline const char*	GetLongName()	{ return long_name; }
	inline const char*	GetFileName()	{ return file_name; }
	inline const char*	GetShortName()	{ return short_name; }
	inline const int32&	GetZoneID()		{ return zoneid; }
	inline const int16&	GetPort()		{ return port; }

	inline const float&	safe_x()		{ return psafe_x; }
	inline const float&	safe_y()		{ return psafe_y; }
	inline const float&	safe_z()		{ return psafe_z; }
	inline const int32& GetMaxClients() { return pMaxClients; }

	void	LoadAAs();
	int		GetTotalAAs() { return totalAAs; }
	AA_List* GetAAList() { return aas; }
	SendAA_Struct* FindAA(int32 id);
	void	LoadZoneDoors(const char* zone);
	
	int32	CountSpawn2();
	ZonePoint* GetClosestZonePoint(float x, float y, float z, const char* to_name);
	ZonePoint* GetClosestZonePoint(float x, float y, float z, int32	to);
	ZonePoint* GetClosestZonePointWithoutZone(float x, float y, float z);
	SpawnGroupList spawn_group_list;

	bool RemoveSpawnEntry(uint32 spawnid);
	bool RemoveSpawnGroup(uint32 in_id);
	
	bool	Process();
	void	DumpAllSpawn2(ZSDump_Spawn2* spawn2dump, int32* spawn2index);
	int32	DumpSpawn2(ZSDump_Spawn2* spawn2dump, int32* spawn2index, Spawn2* spawn2);

	bool	Depop();
	void	Repop(int32 delay = 0);
	void	SpawnStatus(Mob* client);
	void	StartShutdownTimer(int32 set_time = ZONE_AUTOSHUTDOWN_DELAY);
	void	AddAuth(ServerZoneIncommingClient_Struct* szic);
	void	RemoveAuth(const char* iCharName);
	void	ResetAuth();
	bool	GetAuth(int32 iIP, const char* iCharName, int32* oWID = 0, int32* oAccID = 0, int32* oCharID = 0, sint16* oStatus = 0, char* oLSKey = 0, bool* oTellsOff = 0);
	int32	CountAuth();

	void		AddAggroMob()			{ aggroedmobs++; }
	void		DelAggroMob()			{ aggroedmobs--; }
	bool		AggroLimitReached()		{ return (aggroedmobs>10)?true:false; } // change this value, to allow more NPCs to autoaggro
	sint32		MobsAggroCount()		{ return aggroedmobs; }
	void		SetStaticZone(bool sz)	{ staticzone = sz; }
	inline bool	IsStaticZone()			{ return staticzone; }
	inline void	GotCurTime(bool time)	{ gottime = time; }
	void DBAWComplete(int8 workpt_b1, DBAsyncWork* dbaw);
	
	void	GetMerchantDataForZoneLoad();
	void	LoadNewMerchantData(uint32 merchantid);
	void	LoadTempMerchantData();
	void	LoadTempMerchantData_result(MYSQL_RES* result);
	void	LoadMerchantData_result(MYSQL_RES* result);
	int		SaveTempItem(int32 merchantid, int32 npcid, int32 item, sint32 charges, bool sold=false);
	
	map<uint32,NPCType *> npctable;
	map<uint32,std::list<MerchantList> > merchanttable;
	map<uint32,std::list<TempMerchantList> > tmpmerchanttable;
	Map*	map;
	FearPathManager *fear;
	NewZone_Struct	newzone_data;
//	uchar	zone_header_data[142];
	int8	zone_weather;

	EQTime	zone_time;
	void	GetTimeSync();
	void	SetDate(int16 year, int8 month, int8 day, int8 hour, int8 minute);
	void	SetTime(int8 hour, int8 minute);

	void	weatherProc();
	void	weatherSend();
	time_t	weather_timer;
	int8	weather_type;

	int8 loglevelvar;
	int8 merchantvar;
	int8 tradevar;
	int8 lootvar;

	double   GetGroupEXPBonus() { return GroupEXPBonus; } 
	double   GetEXPMod()        { return EXPMod; } 
	double   GetAAXPMod()      { return AAXPMod; } 

#ifdef GUILDWARS
	LinkedList<Spawn2*> spawn2_list; // CODER new spawn list
	Timer* db_update;
#endif
	LinkedList<ZonePoint*> zone_point_list;
	LinkedList<Object*> object_list;
	int32	numzonepoints;
protected:
	//friend class database;
#ifndef GUILDWARS
	LinkedList<Spawn2*> spawn2_list; // CODER new spawn list
#endif
	sint32	aggroedmobs;
private:
	int32	zoneid;
	char*	short_name;
	char	file_name[16];
	char*	long_name;
	char*	address;
	int		totalAAs;
	AA_List* aas;
	uchar*	aa_buffer;
	int16	port;
	bool pvpzone;
	float	psafe_x, psafe_y, psafe_z;
	int32	pMaxClients;

	double  GroupEXPBonus; 
	double  EXPMod; 
	double  AAXPMod;

	bool	staticzone;
	bool	gottime;
	
	int32 pQueuedMerchantsWorkID;
	int32 pQueuedTempMerchantsWorkID;

	Timer	autoshutdown_timer;
	Timer	clientauth_timer;
	Timer	spawn2_timer;
	Timer*  Weather_Timer;
//	LinkedList<Spawn*> spawn_list;
	LinkedList<ZoneClientAuth_Struct*> client_auth_list;
	
	Mutex	MZoneLock;
};

#endif

