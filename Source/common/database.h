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
#ifndef DATABASE_H
#define DATABASE_H

#define AUTHENTICATION_TIMEOUT	60
#define INVALID_ID				0xFFFFFFFF
#define MAX_ITEM_ID				125000

// Disgrace: for windows compile
#ifdef WIN32
	#include <windows.h>
	#include <winsock.h>
#endif
#include <mysql.h>

#include "../common/dbcore.h"
#include "types.h"
#include "linked_list.h"
#include "eq_packet_structs.h"
#include "EQNetwork.h"
#include "../common/guilds.h"
#include "../common/MiscFunctions.h"
#include "../common/Mutex.h"
#include "../zone/loottable.h"
#include "../zone/faction.h"
#include "../zone/message.h"
#include "../zone/AA.h"
#include <string>
#include <vector>
#ifdef GUILDWARS
#include <map>
#include <list>
#endif
using namespace std;

//class Spawn;
class Spawn2;
class NPC;
class SpawnGroupList;
class Petition;
class Client;
struct Combine_Struct;
//struct Faction;
//struct FactionMods;
//struct FactionValue;
struct ZonePoint;
struct NPCType;
class Inventory;
class ItemInst;
class ItemContainerInst;

// Added By Hogie 
// INSERT into variables (varname,value) values('decaytime [minlevel] [maxlevel]','[number of seconds]');
// IE: decaytime 1 54 = Levels 1 through 54
//     decaytime 55 100 = Levels 55 through 100
// It will always put the LAST time for the level (I think) from the Database
struct EventLogDetails_Struct {
	int32	id;
	char	accountname[64];
	int32	account_id;
	sint16	status;
	char	charactername[64];
	char	targetname[64];
	char	timestamp[64];
	char	descriptiontype[64];
	char	details[128];
};

struct CharacterEventLog_Struct {
int32	count;
int8	eventid;
EventLogDetails_Struct eld[255];
};

struct npcDecayTimes_Struct {
	int16 minlvl;
	int16 maxlvl;
	int32 seconds;
};
// Added By Hogie -- End

struct VarCache_Struct {
	char varname[26];	// varname is char(25) in database
	char value[0];
};

struct DBnpcspells_entries_Struct {
	sint16	spellid;
	uint16	type;
	uint8	minlevel;
	uint8	maxlevel;
	sint16	manacost;
	sint32	recast_delay;
	sint16	priority;
};
struct wplist {
	int   index;
	float x;
	float y;
	float z;
	int	  pause;
};
struct DBnpcspells_Struct {
	int32	parent_list;
	sint16	attack_proc;
	int8	proc_chance;
	int32	numentries;
	DBnpcspells_entries_Struct entries[0];
};

struct DBTradeskillRecipe_Struct {
	sint16 skill_needed;
	uint16 trivial;
	bool nofail;
	vector< pair<uint32,uint8> > onsuccess;
	vector< pair<uint32,uint8> > onfail;
};

class Database : public DBcore
{
public:
	Database();
	Database(const char* host, const char* user, const char* passwd, const char* database,int32 port);
	~Database();
	void	MakePet(Make_Pet_Struct* pet,int16 id,int16 type,float size=0);
	void	GetPetStats(NPCType* pet,int16 id);
	bool	GetStartZone(PlayerProfile_Struct* in_pp, CharCreate_Struct* in_cc);
	Ground_Spawns*	LoadGroundSpawns(int32 zone_id,Ground_Spawns* gs);
	void	LoadWorldContainer(uint32 parentid, ItemContainerInst* container);
	void	SaveWorldContainer(uint32 zone_id, uint32 parent_id, const ItemContainerInst* container);
	void	DeleteWorldContainer(uint32 parent_id,uint32 zone_id);
	uint32	AddObject(uint32 type, uint32 icon, const Object_Struct& object, const ItemInst* inst);
	void	UpdateObject(uint32 id, uint32 type, uint32 icon, const Object_Struct& object, const ItemInst* inst);
	void	DeleteObject(uint32 id);
	int32	AddPPoint(float x,float y,float z);
	int32	AddPConnect(int32 pp, int32 p_1,int32 p_2);
	int32	AddPRange(PRange_Struct* pr);
	void	LoadPRange(int32 zoneid, map<int32,PRange_Struct*> &prange);
	void	UpdateAndDeleteAATimers(int32 charid);
	int32	GetTimerRemaining(int32 charid,int32 ability);
	void	UpdateTimersClientConnected(int32 charid);
	void	UpdateAATimers(int32 charid,int32 endtime,int32 begintime,int32 ability);
	void	GetAATimers(int32 charid);
	void	SaveTraderItem(uint32 char_id,uint32 itemid,uint32 itemcost,int8 slot);
	void	DeleteTraderItem(uint32 char_id);
	void	DeleteTraderItem(uint32 char_id,int16 slot_id);
	Trader_Struct* LoadTraderItem(uint32 char_id);
	char	commands[200][200];
	sint16	commandslevels[200];
	int		maxcommandlevel;
	int GetMerchantSlot(int32 merchantid, int32 item);
#ifdef GUILDWARS
	int32	InsertGuildEventLog(int8 type,char* logone,char* logtwo);
	bool	SetupPointsTable(int32 specifiedid,int8 type,const char* name);
	bool	UpdatePointsTable(int32 specifiedid,int8 type,sint32 points,sint32 guildcolumns[6],sint32 playercolumns[4]);
	sint32	GetAvailablePoints(int32 specifiedid,int8 type);
	sint32	GetUsedPoints(int32 specifiedid,int8 type);
	sint32	GetTotalPoints(int32 specifiedid,int8 type);
	
	bool	UpdateZoneServerStats(int32 zoneid, int32 currentusers, int32 maxusers);
	bool	UpdateZoneOnlineStatus(int32 zoneid,int8 online);
	bool	ClearZoneOnlineStatus();
	void	GetZSStats(int32 zone_id);

	bool	InsertPurchasedNPC(int32 guildid,int32 locationid,int32 spawnid);
	int32	GetNPCLocationID(int32 spawnid);

	bool	GetNPC(int32 spawnid);

	int32	CreateLocation(int32 guildid,int8 locationtype,int32 x,int32 y,int32 z,int32 zoneid);
	bool	SetLocationOwner(int32 guildid,int32 location_id);
	void	LoadLocationInformation();
	bool	RemoveLocation(int32 location_id);
	bool	SetLocationProfit(int32 location_id,int32 profit);

	void	GuildTakeOverPoints(int32 guildid,sint32 points);
	bool	FindKillTimeStamp(const char* killer, const char* killed);
	bool	InsertKillTimeStamp(const char* killer, const char* killed, sint32 points,int8 usetime);
	bool	FindZoneKillTimeStamp(const char* killed,const char* zonename);

	void	SetAgreementFlag(int32 acctid);
	bool	GetAgreementFlag(int32 acctid);
#endif
	NewZone_Struct* GetZoneCFG(int32 zoneid);
	bool	SaveZoneCFG(int32 zoneid,NewZone_Struct* zd);
	bool	SaveInventory(uint32 char_id, const ItemInst* inst, sint16 slot_id);
	bool    logevents(char* accountname,int32 accountid,int8 status,const char* charname,const char* target, const char* descriptiontype, const char* description,int event_nid);
	bool	MoveCharacterToZone(char* charname, const char* zonename);
	bool	MoveCharacterToZone(char* charname, const char* zonename,int32 zoneid);
	bool	MoveCharacterToZone(int32 iCharID, const char* iZonename);
	bool	SetGMSpeed(int32 account_id, int8 gmspeed);
	int8	GetGMSpeed(int32 account_id);
	void	DeletePetitionFromDB(Petition* wpet);
	void	ExtraOptions();
	void	SetPublicNote(int32 guildid,char* charname, char* note);
	sint16	CommandRequirement(const char* commandname);
	void	UpdatePetitionToDB(Petition* wpet);
	void	InsertPetitionToDB(Petition* wpet);
	void	RefreshPetitionsFromDB();
	bool	GetDecayTimes(npcDecayTimes_Struct* npcCorpseDecayTimes);
	int8	CheckWorldVerAuth(char* version);
	bool	PopulateZoneLists(const char* zone_name, LinkedList<ZonePoint*>* zone_point_list, SpawnGroupList* spawn_group_list);
	bool	PopulateZoneSpawnList(const char* zone_name, LinkedList<Spawn2*> &spawn2_list, int32 repopdelay = 0);
	Spawn2*	LoadSpawn2(LinkedList<Spawn2*> &spawn2_list, int32 spawn2id, int32 timeleft);
	bool	DumpZoneState();
	int16	GetFreeGrid(int16 zoneid);
	sint8	LoadZoneState(const char* zonename, LinkedList<Spawn2*>& spawn2_list);
	int32	CreatePlayerCorpse(int32 charid, const char* charname, int32 zoneid, uchar* data, int32 datasize, float x, float y, float z, float heading);
	int32	UpdatePlayerCorpse(int32 dbid, int32 charid, const char* charname, int32 zoneid, uchar* data, int32 datasize, float x, float y, float z, float heading);
	sint32	DeleteStalePlayerCorpses();
	sint32	DeleteStalePlayerBackups();
	void	DeleteGrid(int32 sg2, int16 grid_num, bool grid_too,int16 zoneid);
	void	DeleteWaypoint(int16 grid_num, int32 wp_num,int16 zoneid);
//	int32	AddWP(int32 sg2, int16 grid_num, int8 wp_num, float xpos, float ypos, float zpos, int32 pause, float xpos1, float ypos1, float zpos1, int type1, int type2,int16 zoneid);
	void	AddWP(int32 gridid, int8 wpnum, float xpos, float ypos, float zpos, int32 pause, int16 zoneid);
	int32	AddWPForSpawn(int32 spawn2id, float xpos, float ypos, float zpos, int32 pause, int type1, int type2, int16 zoneid);
	bool	DeletePlayerCorpse(int32 dbid);
	bool	LoadPlayerCorpses(int32 iZoneID);
	bool	GetZoneLongName(const char* short_name, char** long_name, char* file_name = 0, float* safe_x = 0, float* safe_y = 0, float* safe_z = 0, int32* maxclients = 0);
//	int32	GetAuthentication(const char* char_name, const char* zone_name, int32 ip);
//	bool	SetAuthentication(int32 account_id, const char* char_name, const char* zone_name, int32 ip);
//	bool	GetAuthentication(int32 account_id, char* char_name, char* zone_name, int32 ip);
//	bool	ClearAuthentication(int32 account_id);
	int8	GetServerType();
	bool	GetGuildNameByID(int32 guilddbid, char * name);
	bool	UpdateTempPacket(char* packet,int32 lsaccount_id);
	bool	UpdateLiveChar(char* charname,int32 lsaccount_id);
	bool	GetLiveChar(int32 lsaccount_id, char* cname);
	int32	GetGuildDBID(int32 eqid);
	bool	NoRentExpired(const char* name);
	bool	GetTempPacket(int32 lsaccount_id, char* packet);
	
	void	GetEventLogs(const char* name,char* target,int32 account_id=0,int8 eventid=0,char* detail=0,char* timestamp=0, CharacterEventLog_Struct* cel=0);
	
	bool	SetServerFilters(char* name, ServerSideFilters_Struct *ssfs);
	int32	GetServerFilters(char* name, ServerSideFilters_Struct *ssfs);
	
	int32	NPCSpawnDB(int8 command, const char* zone, NPC* spawn = 0, int32 extra = 0); // 0 = Create 1 = Add; 2 = Update; 3 = Remove; 4 = Delete
	
    int32	CheckLogin(const char* name, const char* password, sint16* oStatus = 0);
	sint16	CheckStatus(int32 account_id);
	int32	CreateAccount(const char* name, const char* password, sint16 status, int32 lsaccount_id = 0);
	bool	DeleteAccount(const char* name);
	bool	SetGMFlag(const char* name, sint16 status);
	bool	SetSpecialAttkFlag(int8 id, const char* flag);
	bool	CheckZoneserverAuth(const char* ipaddr);
	bool	UpdateName(const char* oldname, const char* newname);
	bool	DoorIsOpen(int8 door_id,const char* zone_name);
	void	SetDoorPlace(int8 value,int8 door_id,const char* zone_name);
	void	GetGuildMembers(int32 guildid,GuildMember_Struct* gms);
	int32	NumberInGuild(int32 guilddbid);
	bool	SetHackerFlag(const char* accountname, const char* charactername, const char* hacked);
    void	GetCharSelectInfo(int32 account_id, CharacterSelect_Struct*);
	bool	GetPlayerProfile(uint32 account_id, char* name, PlayerProfile_Struct* pp, Inventory* inv, char* current_zone = 0);
	bool	SetPlayerProfile(uint32 account_id, uint32 charid, PlayerProfile_Struct* pp, Inventory* inv, uint32 current_zone = 0);
	int32	SetPlayerProfile_MQ(char** query, uint32 account_id, uint32 charid, PlayerProfile_Struct* pp, Inventory* inv, uint32 current_zone = 0);
	bool	GetSharedBank(uint32 id, Inventory* inv, bool is_charid);
	bool	GetInventory(uint32 char_id, Inventory* inv);
	bool	GetInventory(uint32 account_id, char* name, Inventory* inv);
	bool	CreateSpawn2(int32 spawngroup, const char* zone, float heading, float x, float y, float z, int32 respawn, int32 variance);
	bool	CheckNameFilter(const char* name);
	bool	AddToNameFilter(const char* name);
	bool	CheckUsedName(const char* name);
	bool	ReserveName(int32 account_id, char* name);
	bool	CreateCharacter(uint32 account_id, char* name, int16 gender, int16 race, int16 class_, int8 str, int8 sta, int8 cha, int8 dex, int8 int_, int8 agi, int8 wis, int8 face);
	bool	StoreCharacter(uint32 account_id, PlayerProfile_Struct* pp, Inventory* inv);
	bool	DeleteCharacter(char* name);
	//bool    SetStartingItems(PlayerProfile_Struct* pp, Inventory* inv, int16 si_race, int8 si_class, int16 si_deity, int16 si_current_zone, char* si_name, sint16 GM_FLAG = 0);
	bool    SetStartingItems(PlayerProfile_Struct* pp, Inventory* inv, uint32 si_race, uint32 si_class, uint32 si_deity, uint32 si_current_zone, char* si_name, int admin);
	
	int32	GetAccountIDByChar(const char* charname, int32* oCharID = 0);
	uint32	GetAccountIDByChar(uint32 char_id);
	int32	GetAccountIDByName(const char* accname, sint16* status = 0, int32* lsid = 0);
	void	GetAccountName(int32 accountid, char* name, int32* oLSAccountID = 0);
	int32	GetCharacterInfo(const char* iName, int32* oAccID = 0, int32* oZoneID = 0, float* oX = 0, float* oY = 0, float* oZ = 0);
	bool	GetAccountInfoForLogin(int32 account_id, sint16* admin = 0, char* account_name = 0, int32* lsaccountid = 0, int8* gmspeed = 0, bool* revoked = 0);
	bool	GetAccountInfoForLogin_result(MYSQL_RES* result, sint16* admin = 0, char* account_name = 0, int32* lsaccountid = 0, int8* gmspeed = 0, bool* revoked = 0);
	bool	GetCharacterInfoForLogin(const char* name, uint32* character_id = 0, char* current_zone = 0, PlayerProfile_Struct* pp = 0, Inventory* inv = 0, uint32* pplen = 0, PlayerAA_Struct* aa = 0, uint32* aalen = 0, uint32* guilddbid = 0, int8* guildrank = 0);
	int32	GetGroupID(const char* name);
	void	SetGroupID(const char* name,int32 id);
	char*	GetGroupLeaderForLogin(const char* name,char* leaderbuf);
	bool	GetCharacterInfoForLogin_result(MYSQL_RES* result, uint32* character_id = 0, char* current_zone = 0, PlayerProfile_Struct* pp = 0, Inventory* inv = 0, uint32* pplen = 0, PlayerAA_Struct* aa = 0, uint32* aalen = 0, uint32* guilddbid = 0, int8* guildrank = 0);
	bool	SetLocalPassword(uint32 accid, const char* password);
	
	bool	InsertNewsPost(int8 type,char* logone,char* logtwo,int32 levelone,int32 leveltwo);
	
	bool	OpenQuery(char* zonename);
	bool	GetVariable(const char* varname, char* varvalue, int16 varvalue_len);
	bool	SetVariable(const char* varname, const char* varvalue);
	bool	LoadGuilds(GuildRanks_Struct* guilds);
	bool	GetGuildRanks(int32 guildeqid, GuildRanks_Struct* gr);
	int32	GetGuildEQID(int32 guilddbid);
	bool	SetGuild(int32 charid, int32 guilddbid, int8 guildrank);
	bool	SetGuild(char* name, int32 guilddbid, int8 guildrank);
	int32	GetFreeGuildEQID();
	int32	CreateGuild(const char* name, int32 leader);
	bool	DeleteGuild(int32 guilddbid);
	bool	RenameGuild(int32 guilddbid, const char* name);
	bool	EditGuild(int32 guilddbid, int8 ranknum, GuildRankLevel_Struct* grl);
	int32	GetGuildDBIDbyLeader(int32 leader);
	bool	SetGuildLeader(int32 guilddbid, int32 leader);
	bool	SetGuildMOTD(int32 guilddbid, const char* motd);
	char*	GetGuildMOTD(int32 guilddbid);
	int32	GetMerchantData(int32 merchantid, int32 slot);
	int32	GetMerchantListNumb(int32 merchantid);
	bool	GetSafePoints(const char* short_name, float* safe_x = 0, float* safe_y = 0, float* safe_z = 0, sint16* minstatus = 0, int8* minlevel = 0);
	bool	GetSafePoints(int32 zoneID, float* safe_x = 0, float* safe_y = 0, float* safe_z = 0, sint16* minstatus = 0, int8* minlevel = 0) { return GetSafePoints(GetZoneName(zoneID), safe_x, safe_y, safe_z, minstatus, minlevel); }
	
	sint32	GetItemsCount(int32* oMaxID = 0);
	sint32	GetNPCTypesCount(int32* oMaxID = 0);
	sint32	GetDoorsCount(int32* oMaxID = 0);
	sint32	GetNPCFactionListsCount(int32* oMaxID = 0);
	bool	LoadItems();
#ifdef SHAREMEM
	const Item_Struct* IterateItems(uint32* NextIndex);
	bool	DBLoadItems(sint32 iItemCount, uint32 iMaxItemID);
	bool	DBLoadNPCTypes(sint32 iNPCTypeCount, uint32 iMaxNPCTypeID);
	bool	DBLoadDoors(sint32 iDoorCount, uint32 iMaxDoorID);
	bool	DBLoadNPCFactionLists(sint32 iNPCFactionListCount, uint32 iMaxNPCFactionListID);
	bool	DBLoadLoot();
	//bool	DBLoadSkills();
#else
	void	LoadAnItem(uint32 item_id, unsigned int* texture, unsigned int* color);
	bool	UpdateItem(uint16 item_id,Item_Struct* is);
	bool	SaveItemToDatabase(Item_Struct* ItemToSave);
	bool	SetItemAtt(char* att, char * value, unsigned int ItemIndex);
#endif
	bool	LoadVariables();
	int32	LoadVariables_MQ(char** query);
	bool	LoadVariables_result(MYSQL_RES* result);
	bool	LoadNPCTypes();
	bool	LoadZoneNames();
	bool	LoadDoors();
	bool	LoadLoot();
	bool	LoadNPCFactionLists();
	//bool LoadSkills();
	int16 GetTrainlevel(int16 eqclass, int8 skill_id);
	inline const int32&	GetMaxItem()			{ return max_item; }
	inline const int32&	GetMaxLootTableID()		{ return loottable_max; }
	inline const int32&	GetMaxLootDropID()		{ return lootdrop_max; }
	inline const int32&	GetMaxNPCType()			{ return max_npc_type; }
	inline const int32& GetMaxNPCFactionList()	{ return npcfactionlist_max; }
	const Item_Struct*		GetItem(uint32 id);
	const NPCType*			GetNPCType(uint32 id);
	const NPCFactionList*	GetNPCFactionList(uint32 id);
	const Door*				GetDoor(int8 door_id, const char* zone_name);
	const Door*				GetDoorDBID(uint32 db_id);
	int32	GetZoneID(const char* zonename);
	const char*	GetZoneName(int32 zoneID, bool ErrorUnknown = false);
	const LootTable_Struct* GetLootTable(int32 loottable_id);
	const LootDrop_Struct* GetLootDrop(int32 lootdrop_id);
	
	int32	GetMaxNPCSpellsID();
	DBnpcspells_Struct* GetNPCSpells(int32 iDBSpellsID);
	
	static const char*	GetItemLink(const Item_Struct* item);
	void	LoadItemStatus();
	inline int8	GetItemStatus(uint32 id) { if (id < MAX_ITEM_ID) { return item_minstatus[id]; } return 0; }
	inline void SetItemStatus(uint32 id, int8 status) { if (id < MAX_ITEM_ID) { item_minstatus[id] = status; } }
	bool	DBSetItemStatus(int32 id, int8 status);
	bool	GetNPCFactionList(int32 npcfaction_id, sint32* faction_id, sint32* value, sint32* primary_faction = 0);
	bool	GetFactionData(FactionMods* fd, uint32 class_mod, uint32 race_mod, uint32 deity_mod, sint32 faction_id); //rembrant, needed for factions Dec, 16 2001
	bool	GetFactionName(sint32 faction_id, char* name, int32 buflen); // rembrant, needed for factions Dec, 16 2001
	bool	GetFactionIdsForNPC(sint32 nfl_id, LinkedList<struct NPCFaction*> *faction_list, sint32* primary_faction = 0); // neotokyo: improve faction handling
	bool	SetCharacterFactionLevel(int32 char_id, sint32 faction_id, sint32 value,LinkedList<FactionValue*>* val_list); // rembrant, needed for factions Dec, 16 2001
	bool	LoadFactionData();
	bool	LoadFactionValues(int32 char_id, LinkedList<FactionValue*>* val_list);
	bool	LoadFactionValues_result(MYSQL_RES* result, LinkedList<FactionValue*>* val_list);
	
	int32	GetAccountIDFromLSID(int32 iLSID, char* oAccountName = 0, sint16* oStatus = 0);
	
	bool	SetLSAuthChange(int32 account_id, const char* ip);
	bool	UpdateLSAccountAuth(int32 account_id, int8* auth);
	int32	GetLSLoginInfo(char* iUsername, char* oPassword = 0, int8* lsadmin = 0, int* lsstatus = 0, bool* verified = 0, sint16* worldadmin = 0);
//	int32	GetLSLoginInfo(char* iUsername, char* oPassword = 0, char* md5pass = 0);
	int32	GetLSAuthentication(int8* auth);
	int32	GetLSAuthChange(const char* ip);
	bool	AddLoginAccount(char* stationname, char* password, char* chathandel, char* cdkey, char* email);
	int8	ChangeLSPassword(int32 accountid, const char* newpassword, const char* oldpassword = 0);
	bool	ClearLSAuthChange();
	bool	RemoveLSAuthChange(int32 account_id);
	bool	GetLSAccountInfo(int32 account_id, char* name, int8* lsadmin, int* status, bool* verified, sint16* worldadmin = 0);
	sint8	CheckWorldAuth(const char* account, const char* password, int32* account_id, int32* admin_id, bool* GreenName, bool* ShowDown);
	bool	UpdateWorldName(int32 accountid, const char* name);
	string	GetBook(char txtfile[20]);
	void	LoadWorldList();
	void	UpdateBug(BugStruct* bug);
	void	UpdateBug(PetitionBug_Struct* bug);

	//reworked for new tradeskill system
	bool	GetTradeRecipe(const ItemContainerInst* container, uint8 c_type, uint8 tradeskill, DBTradeskillRecipe_Struct *spec);
	bool	GetTradeRecipe(uint32 recipe_id, uint8 c_type, uint8 tradeskill, DBTradeskillRecipe_Struct *spec);

	bool	MakeDoorSpawnPacket(const char* zone,APPLAYER* app);
	bool	CheckGuildDoor(int8 doorid,int16 guildid, const char* zone);
	bool	SetGuildDoor(int8 doorid,int16 guildid, const char* zone);
	bool	LoadStaticZonePoints(LinkedList<ZonePoint*>* zone_point_list,const char* zonename);
	bool	InsertStats(int32 in_players, int32 in_zones, int32 in_servers);
	
	void	AddLootTableToNPC(NPC* npc,int32 loottable_id, ItemList* itemlist, int32* copper, int32* silver, int32* gold, int32* plat);
	bool	UpdateZoneSafeCoords(const char* zonename, float x, float y, float z);
	int8	GetUseCFGSafeCoords();
	int8	CopyCharacter(const char* oldname, const char* newname, int32 acctid);
	int32	GetPlayerAlternateAdv(int32 account_id, char* name, PlayerAA_Struct* aa);
	SendAA_Struct*	GetAASkillVars(int32 skill_id);
	int8	GetTotalAALevels(int32 skill_id);
	int32	GetSizeAA();
	int32	CountAALevels();
	int32	CountAAs();
	void	LoadAAs(AA_List* load);
	void	RetrieveAALevels(SendAA_Struct* aa_struct);
	bool	SetPlayerAlternateAdv(int32 account_id, char* name, PlayerAA_Struct* aa);
	bool	SetLSAdmin(int32 account_id, int8 in_status);
	void	FindAccounts(char* whom, Client* from);
	float	GetSafePoint(const char* short_name, const char* which);
	
	int32   GetZoneForage(int32 ZoneID, int8 skill);    /* for foraging - BoB */
	void	ModifyGrid(bool remove, int16 id, int8 type = 0, int8 type2 = 0,int16 zoneid = 0);
	void    ModifyWP(int16 grid_id, int16 wp_num, float xpos, float ypos, float zpos, int32 script=0,int16 zoneid =0);
	int8    GetGridType(int16 grid,int16 zoneid);
	int8    GetGridType2(int16 grid,int16 zoneid);
	bool    GetWaypoints(int16 grid, int16 zoneid, int16 num, wplist* wp);
	void	AssignGrid(Client *client, float x, float y, int32 id);
	
	//New timezone functions
	int32	GetZoneTZ(int32 zoneid);
	bool	SetZoneTZ(int32 zoneid, int32 tz);
	//End new timezone functions
	void	AddLootDropToNPC(NPC* npc,int32 lootdrop_id, ItemList* itemlist);
	
	int8	GetZoneW(int32 zoneid);
	bool	SetZoneW(int32 zoneid, int8 w);
	bool	InjectToRaw();
	void	UpdateTimeleftWorld();
	void	UpdateTimeleft(int32 id,int32 timeleft);
	void	HandleMysqlError(int32 errnum);
	
	uint32  MaxDoors() { return max_door_type; }
	
	void ConvertItemBlob(); //@merth: just here temporarily
	
protected:
	//bool	RunQuery(const char* query, int32 querylen, char* errbuf = 0, MYSQL_RES** result = 0, int32* affected_rows = 0, int32* errnum = 0, bool retry = true);
	
private:
	void InitVars();
	
	int32				max_zonename;
	char**				zonename_array;
	uint32				max_item;
	uint32				max_npc_type;
	uint32				max_door_type;
	uint32				npcfactionlist_max;
#ifndef SHAREMEM
	Item_Struct**		item_array;
	NPCType**			npc_type_array;
    Door**				door_array;
	NPCFactionList**	npcfactionlist_array;
	LootTable_Struct**	loottable_array;
	sint8*				loottable_inmem;
	LootDrop_Struct**	lootdrop_array;
	bool*				lootdrop_inmem;
#endif
	int8				item_minstatus[MAX_ITEM_ID];
	int8				door_isopen_array[255];
	uint32				max_faction;
	Faction**			faction_array;
	uint32				loottable_max;
	uint32				lootdrop_max;

	int32				npc_spells_maxid;
	DBnpcspells_Struct** npc_spells_cache;
	bool*				npc_spells_loadtried;

	Mutex				Mvarcache;
	uint32				varcache_max;
	VarCache_Struct**	varcache_array;
	uint32				varcache_lastupdate;
};

#ifdef BUGTRACK
class BugDatabase
{
public:
	BugDatabase();
	BugDatabase(const char* host, const char* user, const char* passwd, const char* database);

	~BugDatabase();
	bool UploadBug(const char* bugdetail, const char* version, const char* loginname);
protected:
	bool	RunQuery(const char* query, int32 querylen, char* errbuf = 0, MYSQL_RES** result = 0, int32* affected_rows = 0, int32* errnum = 0, bool retry = true);
	int32	DoEscapeString(char* tobuf, const char* frombuf, int32 fromlen);

private:
	MYSQL	mysqlbug;
	Mutex	MBugDatabase;

};
#endif

#endif
