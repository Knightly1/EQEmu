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
#include <errmsg.h>
#include <mysqld_error.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>

// Disgrace: for windows compile
#ifdef WIN32
#include <windows.h>
#define snprintf	_snprintf
#define strncasecmp	_strnicmp
#define strcasecmp	_stricmp
#else
#include "unix.h"
#include <netinet/in.h>
#endif

#include "database.h"
#include "EQNetwork.h"
#include "packet_functions.h"
#include "eq_opcodes.h"
#include "../common/classes.h"
#include "../common/races.h"
#include "../common/files.h"
#include "../common/EQEMuError.h"
#include "../common/Item.h"
#include "../common/packet_dump.h"
#ifdef SHAREMEM
#include "../common/EMuShareMem.h"
extern LoadEMuShareMemDLL EMuShareMemDLL;
#endif
#ifdef ZONE
#include <math.h>
#include "../zone/entity.h"
#include "../zone/masterentity.h"
extern EntityList entity_list;

#ifdef GUILDWARS
#include "../common/guilds.h"
#include "../GuildWars/GuildWars.h"
#include "../zone/StringIDs.h"
extern GuildLocationList location_list;
extern GuildWars guildwars;
extern GuildRanks_Struct guilds[512];
#endif
#endif // ZONE

#ifdef ZONE
extern Zone *zone;
#endif

// change starting zone
const char* ZONE_NAME = "qeynos";

extern Database database;
extern Client client;
#define buguploadhost "mysql.eqemu.net"
#define buguploaduser "eqemu_bug"
#define buguploadpassword "bugssuck"
#define buguploaddatabase "eqemubug"

/*
This is the amount of time in seconds the client has to enter the zone
server after the world server, or inbetween zones when that is finished
*/

/*
Establish a connection to a mysql database with the supplied parameters

  Added a very simple .ini file parser - Bounce
  
	Modify to use for win32 & linux - misanthropicfiend
*/
Database::Database ()
{
	InitVars();
	char host[200], user[200], passwd[200], database[200], buf[200], type[200];
	char cport[5]={0};
	int32 port=0;
	bool compression = false;
	int items[5] = {0, 0, 0, 0};
	FILE *f;
	
	if (!(f = fopen (DB_INI_FILE, "r")))
	{
		printf ("Couldn't open '%s'.\n", DB_INI_FILE);
		printf ("Read README.TXT!\n");
		exit (1);
	}
	
	do
	{
		fgets (buf, 199, f);
		if (feof (f))
		{
			printf ("[Database] block not found in DB.INI.\n");
			printf ("Read README.TXT!\n");
			exit (1);
		}
	}
	while (strncasecmp (buf, "[Database]\n", 11) != 0 && strncasecmp (buf, "[Database]\r\n", 12) != 0);
	
	while (!feof (f))
	{
#ifdef WIN32
		if (fscanf (f, "%[^=]=%[^\n]\n", type, buf) == 2)
#else	
			if (fscanf (f, "%[^=]=%[^\r\n]\n", type, buf) == 2)
#endif
			{
				if (!strncasecmp (type, "host", 4))
				{
					strncpy (host, buf, 199);
					items[0] = 1;
				}
				if (!strncasecmp (type, "user", 4))
				{
					strncpy (user, buf, 199);
					items[1] = 1;
				}
				if (!strncasecmp (type, "pass", 4))
				{
					strncpy (passwd, buf, 199);
					items[2] = 1;
				}
				if (!strncasecmp (type, "data", 4))
				{
					strncpy (database, buf, 199);
					items[3] = 1;
				}
				if (!strncasecmp (type, "port", 4))
				{
					strncpy(cport,buf,4);
					port=atoi(cport);
				}
				if (!strncasecmp (type, "compression", 11)) {
					if (strcasecmp(buf, "on") == 0) {
						compression = true;
						cout << "DB Compression on." << endl;
					}
					else if (strcasecmp(buf, "off") != 0)
						cout << "Unknown 'compression' setting in db.ini. Expected 'on' or 'off'." << endl;
				}
			}
	}
	
	if (!items[0] || !items[1] || !items[2] || !items[3])
	{
		printf ("Incomplete DB.INI file.\n");
		printf ("Read README.TXT!\n");
		exit (1);
	}
	
	fclose (f);
	int32 errnum = 0;
	char errbuf[MYSQL_ERRMSG_SIZE];
	if (!Open(host, user, passwd, database,port, &errnum, errbuf)) {
		LogFile->write(EQEMuLog::Error, "Failed to connect to database: Error: %s", errbuf);
		HandleMysqlError(errnum);
	}
	else
	{
		LogFile->write(EQEMuLog::Status, "Using database '%s' at %s", database, host);
	}
}

/*
Establish a connection to a mysql database with the supplied parameters
*/

Database::Database(const char* host, const char* user, const char* passwd, const char* database, int32 port)
{
	InitVars();
	int32 errnum= 0;
	char errbuf[MYSQL_ERRMSG_SIZE];
	if (!Open(host, user, passwd, database, port, &errnum, errbuf))
	{
		cerr << "Failed to connect to database: Error: " << errbuf << endl;
		HandleMysqlError(errnum);
	}
	else
	{
		cout << "Using database '" << database << "' at " << host << endl;
	}
}

void Database::HandleMysqlError(int32 errnum) {
	switch(errnum) {
		case 0:
			break;
		case 1045: // Access Denied
		case 2001: {
			AddEQEMuError(EQEMuError_Mysql_1405, true);
			break;
		}
		case 2003: { // Unable to connect
			AddEQEMuError(EQEMuError_Mysql_2003, true);
			break;
		}
		case 2005: { // Unable to connect
			AddEQEMuError(EQEMuError_Mysql_2005, true);
			break;
		}
		case 2007: { // Unable to connect
			AddEQEMuError(EQEMuError_Mysql_2007, true);
			break;
		}
	}
}
int32 Database::AddPConnect(int32 pp, int32 p_1,int32 p_2){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 last_insert_id=0;
	if (!RunQuery(query, MakeAnyLenString(&query, "Insert into p_connect (pp,p_1,p_2) values(%i,%i,%i)",pp,p_1,p_2),errbuf,0,0,&last_insert_id))	{
		LogFile->write(EQEMuLog::Error, "Error in AddPConnect query %s: %s", query, errbuf);
	}
	safe_delete_array(query);
	return last_insert_id;
}
int32 Database::AddPPoint(float x,float y,float z){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 last_insert_id=0;
	if (!RunQuery(query, MakeAnyLenString(&query, "Insert into p_points (x,y,z) values(%f,%f,%f)",x,y,z),errbuf,0,0,&last_insert_id))	{
		LogFile->write(EQEMuLog::Error, "Error in AddPPoint query %s: %s", query, errbuf);
	}
	safe_delete_array(query);
	return last_insert_id;
}
bool Database::SaveZoneCFG(int32 zoneid,NewZone_Struct* zd){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "update zone set underworld=%f,minclip=%f,maxclip=%f,fog_minclip=%f,fog_maxclip=%f,fog_blue=%i,fog_red=%i,fog_green=%i,sky=%i,ztype=%i,zone_exp_multiplier=%f,walkspeed=%f,safe_x=%f,safe_y=%f,safe_z=%f where zoneidnumber=%i",
		zd->underworld,zd->minclip,zd->maxclip,zd->fog_minclip[0],zd->fog_maxclip[0],zd->fog_blue[0],zd->fog_red[0],zd->fog_green[0],zd->sky,zd->ztype,zd->zone_exp_multiplier,zd->walkspeed,zd->safe_x,zd->safe_y,zd->safe_z,zoneid),errbuf))	{
		LogFile->write(EQEMuLog::Error, "Error in SaveZoneCFG query %s: %s", query, errbuf);
		safe_delete_array(query);
		return false;
	}
	safe_delete_array(query);
	return true;
}
NewZone_Struct* Database::GetZoneCFG(int32 zoneid){
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int i=0;
	NewZone_Struct* zone_data = NULL;
	if (database.RunQuery(query, MakeAnyLenString(&query, "SELECT ztype,fog_red,fog_green,fog_blue,fog_minclip,fog_maxclip,sky,zone_exp_multiplier,safe_x,safe_y,safe_z,underworld,minclip,maxclip,walkspeed,time_type from zone where zoneidnumber=%i",zoneid), errbuf, &result)) {
		safe_delete_array(query);
		while((row = mysql_fetch_row(result))) {	
			zone_data = new NewZone_Struct();
			memset(zone_data,0,sizeof(NewZone_Struct));
			zone_data->ztype=atoi(row[0]);
			for(i=0;i<4;i++){
				zone_data->fog_red[i]=atoi(row[1]);
				zone_data->fog_green[i]=atoi(row[2]);
				zone_data->fog_blue[i]=atoi(row[3]);
				zone_data->fog_minclip[i]=atof(row[4]);
				zone_data->fog_maxclip[i]=atof(row[5]);
			}
			zone_data->sky=atoi(row[6]);
			zone_data->zone_exp_multiplier=atof(row[7]);
			zone_data->safe_x=atof(row[8]);
			zone_data->safe_y=atof(row[9]);
			zone_data->safe_z=atof(row[10]);
			zone_data->underworld=atof(row[11]);
			zone_data->minclip=atof(row[12]);
			zone_data->maxclip=atof(row[13]);
			zone_data->walkspeed=atof(row[14]);
			zone_data->time_type=atoi(row[15]);
		}
	}
	else
		LogFile->write(EQEMuLog::Error, "Error in GetZoneCFG query %s: %s", query, errbuf);
	safe_delete_array(query);
	return zone_data;
}
void Database::LoadPRange(int32 zoneid,map<int32,PRange_Struct*> &prange){
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (database.RunQuery(query, MakeAnyLenString(&query, "SELECT rx1,rx2,rx3,rx4,ry1,ry2,ry3,ry4,rz1,rz2,rz3,rz4,id from p_range where zoneid=%i",zoneid), errbuf, &result)) {
		safe_delete_array(query);
		while((row = mysql_fetch_row(result))) {
			PRange_Struct* pran = new PRange_Struct;
			pran->rx1=atof(row[0]);
			pran->rx2=atof(row[1]);
			pran->rx3=atof(row[2]);
			pran->rx4=atof(row[3]);
			pran->ry1=atof(row[4]);
			pran->ry2=atof(row[5]);
			pran->ry3=atof(row[6]);
			pran->ry4=atof(row[7]);
			pran->rz1=atof(row[8]);
			pran->rz2=atof(row[9]);
			pran->rz3=atof(row[10]);
			pran->rz4=atof(row[11]);
			prange[atoi(row[12])]=pran;
		}
		mysql_free_result(result);
	} 
	else
		safe_delete_array(query);
}
int32 Database::AddPRange(PRange_Struct* pr){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 last_insert_id=0;
	if (!RunQuery(query, MakeAnyLenString(&query, "Insert into p_range (rx1,rx2,rx3,rx4,ry1,ry2,ry3,ry4,rz1,rz2,rz3,rz4) values(%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f)",pr->rx1,pr->rx2,pr->rx3,pr->rx4,pr->ry1,pr->ry2,pr->ry3,pr->ry4,pr->rz1,pr->rz2,pr->rz3,pr->rz4),errbuf,0,0,&last_insert_id))	{
		LogFile->write(EQEMuLog::Error, "Error in AddPRange query %s: %s", query, errbuf);
	}
	safe_delete_array(query);
	return last_insert_id;
}
void Database::UpdateTimeleftWorld()
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "Update spawn2 set timeleft=(timeleft-300000) where timeleft>=300000",errbuf)))	{
		LogFile->write(EQEMuLog::Error, "Error in UpdateTimeLeftWorld query(1) %s: %s", query, errbuf);
	}
	else if (!RunQuery(query, MakeAnyLenString(&query, "Update spawn2 set timeleft=0 where timeleft<300000",errbuf)))	{
		LogFile->write(EQEMuLog::Error, "Error in UpdateTimeLeftWorld query(2) %s: %s", query, errbuf);
	}
	safe_delete_array(query);
	return;
}

void Database::UpdateTimeleft(int32 id, int32 timeleft)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	//printf("id: %i timeleft: %i\n",id,timeleft);
	if (!RunQuery(query, MakeAnyLenString(&query, "Update spawn2 set timeleft=(%i*1000) where id=%i",timeleft,id, errbuf)))	{
		LogFile->write(EQEMuLog::Error, "Error in UpdateTimeLeft query %s: %s", query, errbuf);
	}
	safe_delete_array(query);
	return;
}

void Database::InitVars() {
#ifndef SHAREMEM
	item_array = 0;
	npc_type_array = 0;
    door_array = 0;
	loottable_array = 0;
	loottable_inmem = 0;
	lootdrop_array = 0;
	lootdrop_inmem = 0;
#endif
	memset(item_minstatus, 0, sizeof(item_minstatus));
	memset(door_isopen_array, 0, sizeof(door_isopen_array));
	max_item = 0;
	max_npc_type = 0;
	max_faction = 0;
	faction_array = 0;
	max_zonename = 0;
	zonename_array = 0;





	loottable_max = 0;
	lootdrop_max = 0;
	max_door_type = 0;
	npc_spells_maxid = 0;
	npc_spells_cache = 0;
	npc_spells_loadtried = 0;
	npcfactionlist_max = 0;
	varcache_array = 0;
	varcache_max = 0;
	varcache_lastupdate = 0;
}

bool Database::logevents(char* accountname,int32 accountid,int8 status,const char* charname, const char* target,const char* descriptiontype, const char* description,int event_nid){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	uint32 len = strlen(description);
	uint32 len2 = strlen(target);
	char* descriptiontext = new char[2*len+1];
	char* targetarr = new char[2*len2+1];
	memset(descriptiontext, 0, 2*len+1);
	memset(targetarr, 0, 2*len2+1);
	DoEscapeString(descriptiontext, description, len);
	DoEscapeString(targetarr, target, len2);
	if (!RunQuery(query, MakeAnyLenString(&query, "Insert into eventlog (accountname,accountid,status,charname,target,descriptiontype,description,event_nid) values('%s',%i,%i,'%s','%s','%s','%s','%i')", accountname,accountid,status,charname,targetarr,descriptiontype,descriptiontext,event_nid), errbuf))	{
		cerr << "Error in logevents" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	safe_delete_array(query);
	safe_delete_array(descriptiontext);
	safe_delete_array(targetarr);
	return true;
}

sint16 Database::CommandRequirement(const char* commandname) {
	for(int i=0; i<maxcommandlevel; i++)
	{
		if((strcasecmp(commandname, commands[i]) == 0)) {
			return commandslevels[i];
		}
	}
	return 255;
}

void Database::ExtraOptions()
{
	FILE* f;
	int open = 1;
	char buf[200];
	char type[200];
	maxcommandlevel = 0;



	if (!(f = fopen (ADDON_INI_FILE, "r")))
	{
		printf ("Couldn't open '%s'.\n", ADDON_INI_FILE);
		return;
	}
	do
	{
		fgets (buf, 199, f);
		if (feof (f))
		{
			printf ("[CommandLevels] block not found in ADDON.INI.\n");
			return;
		}
	}
	
	while (strncasecmp (buf, "[CommandLevels]\n", 11) != 0 && strncasecmp (buf, "[CommandLevels]\r\n", 12) != 0 && open == 1);
	
	while (!feof (f) && open == 1)
	{
#ifdef WIN32
		if (fscanf (f, "%[^=]=%[^\n]\n", type, buf) == 2)
#else	
			if (fscanf (f, "%[^=]=%[^\r\n]\n", type, buf) == 2)
#endif
			{
				if(sizeof(type) > 0 && maxcommandlevel < 200) {
					snprintf(commands[maxcommandlevel], 200, "%s", type);
					commandslevels[maxcommandlevel] = atoi(buf);
					maxcommandlevel++;
				}
			}
	}
	
	fclose (f);
}

/*

Close the connection to the database
*/
Database::~Database()
{
	unsigned int x;
#ifndef SHAREMEM
	if (item_array != 0) {
		for (x=0; x <= max_item; x++) {
			if (item_array[x] != 0)
				safe_delete(item_array[x]);
		}
		safe_delete_array(item_array);
	}
	if (npc_type_array != 0) {
		for (x=0; x <= max_npc_type; x++) {
			if (npc_type_array[x] != 0)
				safe_delete(npc_type_array[x]);
		}
		safe_delete_array(npc_type_array); // neotokyo: fix
	}
	if (door_array != 0) {
		for (x=0; x <= max_door_type; x++) {
			if (door_array[x] != 0)
				safe_delete(door_array[x]);
		}
		safe_delete_array(door_array);
	}
	if (loottable_array) {
		for (x=0; x<=loottable_max; x++) {
			safe_delete(loottable_array[x]);
		}
		safe_delete_array(loottable_array);
	}
	
	safe_delete(loottable_inmem);
	if (lootdrop_array) {
		for (x=0; x<=lootdrop_max; x++) {
			safe_delete(lootdrop_array[x]);
		}
		safe_delete_array(lootdrop_array);
	}
	safe_delete_array(lootdrop_inmem);
#endif
	if (faction_array != 0) {
		for (x=0; x <= max_faction; x++) {
			if (faction_array[x] != 0)
				safe_delete(faction_array[x]);
		}
		safe_delete_array(faction_array);
	}
	if (zonename_array) {
		for (x=0; x<=max_zonename; x++) {
			if (zonename_array[x])
				safe_delete_array(zonename_array[x]);
		}
		safe_delete_array(zonename_array);
	}
	if (npc_spells_cache) {
		for (x=0; x<=npc_spells_maxid; x++) {
			safe_delete_array(npc_spells_cache[x]);
		}
		safe_delete_array(npc_spells_cache);
	}
	safe_delete_array(npc_spells_loadtried);
	if (varcache_array) {
		for (x=0; x<varcache_max; x++) {
			safe_delete_array(varcache_array[x]);
		}
		safe_delete_array(varcache_array);
	}
}
void Database::UpdateBug(BugStruct* bug){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	uint32 len = strlen(bug->bug);
	char* bugtext = new char[2*len+1];
	memset(bugtext, 0, 2*len+1);
	DoEscapeString(bugtext, bug->bug, len);
	if (!RunQuery(query, MakeAnyLenString(&query, "Insert into bugs (type,name,bugtext,flag,x,y,z,heading) values('%s','%s','%s',%i,%f,%f,%f,%f)",bug->chartype,bug->name,bugtext,bug->type,bug->x,bug->y,bug->z,bug->heading), errbuf))	{
		cerr << "Error in UpdateBug" << query << "' " << errbuf << endl;
	}
	safe_delete_array(query);
	safe_delete_array(bugtext);
}
void Database::UpdateBug(PetitionBug_Struct* bug){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	uint32 len = strlen(bug->text);
	char* bugtext = new char[2*len+1];
	memset(bugtext, 0, 2*len+1);
	DoEscapeString(bugtext, bug->text, len);
	if (!RunQuery(query, MakeAnyLenString(&query, "Insert into bugs (type,name,bugtext,flag) values('%s','%s','%s',%i)","Petition",bug->name,bugtext,25), errbuf))	{
		cerr << "Error in UpdateBug" << query << "' " << errbuf << endl;
	}
	safe_delete_array(query);
	safe_delete_array(bugtext);
}
void Database::MakePet(Make_Pet_Struct* pet,int16 id,int16 type,float size){
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (database.RunQuery(query, MakeAnyLenString(&query, "SELECT level,class, race, texture, size, min_dmg, max_dmg from pets where id=%i",id), errbuf, &result)) {
		safe_delete_array(query);
		if((row = mysql_fetch_row(result))) {
			if(size<=0)
				size=atof(row[4]);
			pet->level=atoi(row[0]);
			pet->class_=atoi(row[1]);
			pet->race=atoi(row[2]);
			pet->texture=atoi(row[3]);
			pet->size=size;
			pet->type=type;
			pet->pettype=id;
			pet->min_dmg = atoi(row[4]);
			pet->max_dmg = atoi(row[5]);
		}
		mysql_free_result(result);
	} 
	else
		safe_delete_array(query);
}
void Database::GetPetStats(NPCType* pet,int16 id){
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (database.RunQuery(query, MakeAnyLenString(&query, "SELECT max_hp , cur_hp, min_dmg, max_dmg from pets where id=%i",id), errbuf, &result)) {
		safe_delete_array(query);
		if((row = mysql_fetch_row(result))) {
			pet->max_hp = atoi(row[0]);//hmmm
			pet->cur_hp = atoi(row[1]);
			pet->min_dmg = atoi(row[2]);
			pet->max_dmg = atoi(row[3]);
		}
		mysql_free_result(result);
	}
	else
		safe_delete_array(query);
}
/*
Check if the character with the name char_name from ip address ip has
permission to enter zone zone_name. Return the account_id if the client
has the right permissions, otherwise return zero.
Zero will also be returned if there is a database error.
*/
/*int32 Database::GetAuthentication(const char* char_name, const char* zone_name, int32 ip) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT account_id FROM authentication WHERE char_name='%s' AND zone_name='%s' AND ip=%u AND UNIX_TIMESTAMP()-UNIX_TIMESTAMP(time) < %i", char_name, zone_name, ip, AUTHENTICATION_TIMEOUT), errbuf, &result))
	{
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 account_id = atoi(row[0]);
			mysql_free_result(result);
			return account_id;
		}
		else {
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetAuthentication query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return 0;
	}
	
	return 0;
}*/

/*
Give the account with id "account_id" permission to enter zone "zone_name" from ip address "ip"
with character "char_name". Return true if successful.
False will be returned if there is a database error.
*/
/*bool Database::SetAuthentication(int32 account_id, const char* char_name, const char* zone_name, int32 ip) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM authentication WHERE account_id=%i", account_id), errbuf))
	{
		cerr << "Error in SetAuthentication query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO authentication SET account_id=%i, char_name='%s', zone_name='%s', ip=%u", account_id, char_name, zone_name, ip), errbuf, 0, &affected_rows))
	{
		cerr << "Error in SetAuthentication query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	safe_delete_array(query);
	
	if (affected_rows == 0) {
		return false;
	}
	
	return true;
}*/

/*
This function will return the zone name in the "zone_name" parameter.
This is used when a character changes zone, the old zone server sets
the authentication record, the world server reads this new zone	name.
If there was a record return true, otherwise false.
False will also be returned if there is a database error.
*/
/*bool Database::GetAuthentication(int32 account_id, char* char_name, char* zone_name, int32 ip) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT char_name, zone_name FROM authentication WHERE account_id=%i AND ip=%u AND UNIX_TIMESTAMP()-UNIX_TIMESTAMP(time) < %i", account_id, ip, AUTHENTICATION_TIMEOUT), errbuf, &result))
	{
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			strcpy(char_name, row[0]);
			strcpy(zone_name, row[1]);
			mysql_free_result(result);
			return true;
		}
		else {
			mysql_free_result(result);
			return false;
		}




		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetAuthentication query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return false;
}*/


/*
This function will remove the record in the authentication table for
the account with id "accout_id"
False will also be returned if there is a database error.
*/
/*bool Database::ClearAuthentication(int32 account_id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "DELETE FROM authentication WHERE account_id=%i", account_id), errbuf))
	{
		cerr << "Error in ClearAuthentication query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	safe_delete_array(query);
	
	return true;
}*/

/*
Check if there is an account with name "name" and password "password"
Return the account id or zero if no account matches.
Zero will also be returned if there is a database error.
*/
int32 Database::CheckLogin(const char* name, const char* password, sint16* oStatus) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	char tmpUN[100];
	char tmpPW[100];
	DoEscapeString(tmpUN, name, strlen(name));
	DoEscapeString(tmpPW, password, strlen(password));
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, status FROM account WHERE name='%s' AND (password='%s' or password=MD5('%s'))", tmpUN, tmpPW, tmpPW), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			int32 id = atoi(row[0]);
			if (oStatus)
				*oStatus = atoi(row[1]);
			mysql_free_result(result);
			return id;
		}
		else
		{
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in CheckLogin query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return 0;
}

int8 Database::GetGMSpeed(int32 account_id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT gmspeed FROM account where id='%i'", account_id), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			int8 gmspeed = atoi(row[0]);
			mysql_free_result(result);
			return gmspeed;
		}
		else
		{
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
	}
	else
	{
		
		cerr << "Error in GetGMSpeed query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return 0;
	

}

bool Database::SetGMSpeed(int32 account_id, int8 gmspeed)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE account SET gmspeed = %i where id = %i", gmspeed, account_id), errbuf)) {
		cerr << "Error in SetGMSpeed query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	safe_delete_array(query);
	return true;
	
}

/*
 solar: this is never actually called, client_process starts an async query
 instead and uses GetAccountInfoForLogin_result to process it..
 */
bool Database::GetAccountInfoForLogin(int32 account_id, sint16* admin, char* account_name, int32* lsaccountid, int8* gmspeed, bool* revoked) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT status, name, lsaccount_id, gmspeed, revoked FROM account WHERE id=%i", account_id), errbuf, &result)) {
		safe_delete_array(query);
		bool ret = GetAccountInfoForLogin_result(result, admin, account_name, lsaccountid, gmspeed, revoked);
		mysql_free_result(result);
		return ret;
	}
	else
	{
		cerr << "Error in GetAccountInfoForLogin query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return false;
}

bool Database::GetAccountInfoForLogin_result(MYSQL_RES* result, sint16* admin, char* account_name, int32* lsaccountid, int8* gmspeed, bool* revoked) {
    MYSQL_ROW row;
	if (mysql_num_rows(result) == 1) {
		row = mysql_fetch_row(result);
		if (admin)
			*admin = atoi(row[0]);
		if (account_name)
			strcpy(account_name, row[1]);
		if (lsaccountid) {

			if (row[2])
				*lsaccountid = atoi(row[2]);
			else
				*lsaccountid = 0;


		}
		if (gmspeed)
			*gmspeed = atoi(row[3]);
		if (revoked)
			*revoked = atoi(row[4]);
		return true;
	}
	else {
		return false;
	}
}

sint16 Database::CheckStatus(int32 account_id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT status FROM account WHERE id='%i'", account_id), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			sint16 status = atoi(row[0]);

			mysql_free_result(result);
			return status;
		}
		else
		{
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in CheckStatus query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return 0;
}

int32 Database::CreateAccount(const char* name, const char* password, sint16 status, int32 lsaccount_id) {	
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 querylen;
	int32 last_insert_id;
	
	if (password)
		querylen = MakeAnyLenString(&query, "INSERT INTO account SET name='%s', password='%s', status=%i, lsaccount_id=%i;",name,password,status, lsaccount_id);
	else
		querylen = MakeAnyLenString(&query, "INSERT INTO account SET name='%s', status=%i, lsaccount_id=%i;",name, status, lsaccount_id);
	
	cerr << "Account Attempting to be created:" << name << " " << (sint16) status << endl;
	if (!RunQuery(query, querylen, errbuf, 0, 0, &last_insert_id)) {
		cerr << "Error in CreateAccount query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return 0;
	}
	safe_delete_array(query);

	if (last_insert_id == 0) {
		cerr << "Error in CreateAccount query '" << query << "' " << errbuf << endl;
		return 0;
	}
	
	return last_insert_id;
}

bool Database::DeleteAccount(const char* name) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	cerr << "Account Attempting to be deleted:" << name << endl;
	if (RunQuery(query, MakeAnyLenString(&query, "DELETE FROM account WHERE name='%s';",name), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		if (affected_rows == 1) {
			return true;
		}
	}
	else {
		
		cerr << "Error in DeleteAccount query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	
	return false;
}

bool Database::SetLocalPassword(int32 accid, const char* password) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE account SET password=MD5('%s') where id=%i;", password, accid), errbuf)) {
		cerr << "Error in SetLocalPassword query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	safe_delete_array(query);
	return true;
}

bool Database::UpdateLiveChar(char* charname,int32 lsaccount_id) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE account SET charname='%s' WHERE id=%i;",charname, lsaccount_id), errbuf)) {
		cerr << "Error in UpdateLiveChar query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	safe_delete_array(query);
	return true;
}

bool Database::GetLiveChar(int32 account_id, char* cname) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT charname FROM account WHERE id=%i", account_id), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			strcpy(cname,row[0]);
			mysql_free_result(result);
			return true;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetLiveChar query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	
	return false;
}

bool Database::UpdateTempPacket(char* packet, int32 lsaccount_id) {
	
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[256+sizeof(CharCreate_Struct)*2+1];
	char* end = query;
	
	end += sprintf(end, "UPDATE account SET packencrypt=");
	*end++ = '\'';
    end += DoEscapeString(end, packet, strlen(packet));
    *end++ = '\'';
    end += sprintf(end," WHERE id=%i", lsaccount_id);
	
	int32 affected_rows = 0;
    if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows)) {
        cerr << "Error in UpdateTempPacket query " << errbuf << endl;
		return false;
    }
	
	if (affected_rows == 0) {
		return false;
	}
	
	return true;
}

bool Database::GetTempPacket(int32 lsaccount_id, char* packet)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;

    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT packencrypt FROM account WHERE id=%i", lsaccount_id), errbuf, &result)) {
		safe_delete_array(query);
		
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			memcpy(packet, row[0], strlen(row[0]));
			mysql_free_result(result);
			return true;
		}
		mysql_free_result(result);
	}
	else {
		safe_delete_array(query);
	}
	
	return false;
}

bool Database::SetGMFlag(const char* name, sint16 status) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int32	affected_rows = 0;
	
	cout << "Account being GM Flagged:" << name << ", Level: " << (sint16) status << endl;
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE account SET status=%i WHERE name='%s';", status, name), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		return false;
	}
	safe_delete_array(query);
	
	if (affected_rows == 0) {
		cout << "Account: " << name << " does not exist, therefore it cannot be flagged\n";
		return false;
	}

	return true;
}

bool Database::SetSpecialAttkFlag(int8 id, const char* flag) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32	affected_rows = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE npc_types SET npcspecialattks='%s' WHERE id=%i;",flag,id), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		return false;
	}
	safe_delete_array(query);
	
	if (affected_rows == 0) {
		return false;
	}
	
	return true;
}

bool Database::DoorIsOpen(int8 door_id,const char* zone_name)
{
	if(door_isopen_array[door_id] == 0) {
		SetDoorPlace(1,door_id,zone_name);
		return false;
	}
	else {
		SetDoorPlace(0,door_id,zone_name);
		return true;
	}

	/*
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT doorisopen from doors where zone='%s' AND doorid=%i", zone_name,door_id), errbuf, &result)) {
		safe_delete_array(query);
		
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			int8 open = atoi(row[0]);
			mysql_free_result(result);
			
			if(open == 0) {
				SetDoorPlace(1,door_id,zone_name);
				return false;
			}
			else {
				SetDoorPlace(0,door_id,zone_name);
				return true;
			}
		}

		else
			mysql_free_result(result);
	}
	else {
		safe_delete_array(query);
	}
	
	return false;*/
}

void Database::SetDoorPlace(int8 value,int8 door_id,const char* zone_name)
{
	door_isopen_array[door_id] = value;
/*	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	if(value == 99)
	{
	if (RunQuery(query, MakeAnyLenString(&query, "update doors set doorisopen=0 where zone='zone_name'",zone_name), errbuf, 0, &affected_rows))
		safe_delete_array(query);
	}
	else
	{
	if (RunQuery(query, MakeAnyLenString(&query, "update doors set doorisopen=%i where zone='%s' AND doorid=%i",value,zone_name,door_id), errbuf, 0, &affected_rows))
		safe_delete_array(query);
	}*/
}

void Database::GetEventLogs(const char* name,char* target,int32 account_id,int8 eventid,char* detail,char* timestamp, CharacterEventLog_Struct* cel)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	int32 count = 0;
	char modifications[200];
	if(strlen(name) != 0)
		sprintf(modifications,"charname=\'%s\'",name);
	else if(account_id != 0)
		sprintf(modifications,"accountid=%i",account_id);
	
	if(strlen(target) != 0)
		sprintf(modifications,"%s AND target like \'%%%s%%\'",modifications,target);
	
	if(strlen(detail) != 0)
		sprintf(modifications,"%s AND description like \'%%%s%%\'",modifications,detail);
	
	if(strlen(timestamp) != 0)
		sprintf(modifications,"%s AND time like \'%%%s%%\'",modifications,timestamp);
	
	if(eventid == 0)
		eventid =1;
	sprintf(modifications,"%s AND event_nid=%i",modifications,eventid);
	
	MakeAnyLenString(&query, "SELECT id,accountname,accountid,status,charname,target,time,descriptiontype,description FROM eventlog where %s",modifications);	
	if (RunQuery(query, strlen(query), errbuf, &result))
	{
		safe_delete_array(query);
		while((row = mysql_fetch_row(result)))
		{
			if(count > 255)
				break;
			cel->eld[count].id = atoi(row[0]);
			strncpy(cel->eld[count].accountname,row[1],64);
			cel->eld[count].account_id = atoi(row[2]);
			cel->eld[count].status = atoi(row[3]);
			strncpy(cel->eld[count].charactername,row[4],64);
			strncpy(cel->eld[count].targetname,row[5],64);
			sprintf(cel->eld[count].timestamp,"%s",row[6]);
			strncpy(cel->eld[count].descriptiontype,row[7],64);
			strncpy(cel->eld[count].details,row[8],128);
			cel->eventid = eventid;
			count++;
			cel->count = count;
		}
		mysql_free_result(result);
	}
	else
	{
		// TODO: Invalid item length in database
		safe_delete_array(query);
	}
}

// solar: the current stuff is at the bottom of this function
void Database::GetCharSelectInfo(int32 account_id, CharacterSelect_Struct* cs) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	Inventory *inv;
	
	for (int i=0; i<10; i++) {
		strcpy(cs->name[i], "<none>");
		cs->zone[i] = 0;
		cs->level[i] = 0;
	}
	
	int char_num = 0;
	unsigned long* lengths;
	
	// Populate character info
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT name,profile,zonename FROM character_ WHERE account_id=%i order by name limit 10", account_id), errbuf, &result)) {
		safe_delete_array(query);
		while ((row = mysql_fetch_row(result))) {
			lengths = mysql_fetch_lengths(result);
			if ((lengths[1] == sizeof(OldPlayerProfile_Struct))) {
				strcpy(cs->name[char_num], row[0]);
				OldPlayerProfile_Struct* pp = (OldPlayerProfile_Struct*)row[1];
				// Character information
				cs->level[char_num]				= pp->level;
				cs->class_[char_num]			= pp->class_;
				cs->race[char_num]				= pp->race;
				cs->gender[char_num]			= pp->gender;
				cs->deity[char_num]				= pp->deity;
				cs->zone[char_num]				= GetZoneID(row[2]);
				cs->face[char_num]				= pp->face;
				cs->haircolor[char_num]		= pp->haircolor;
				cs->beardcolor[char_num]	= pp->beardcolor;
				cs->eyecolor2[char_num] 	= pp->eyecolor2;
				cs->eyecolor1[char_num] 	= pp->eyecolor1;
				cs->hair[char_num]				= pp->hairstyle;
				cs->beard[char_num]				= pp->beard;
				for (uint8 material = 0; material <= 8; material++){
					cs->equip[char_num][material] = pp->item_material[material];
					cs->cs_colors[char_num][material].color = pp->item_tint[material].color;
					if ((material==MATERIAL_PRIMARY) || (material==MATERIAL_SECONDARY)) {
						uint32 melee_idx = (material==MATERIAL_PRIMARY) ? 0 : 1;
						cs->melee[melee_idx][char_num] = cs->equip[char_num][material];
					}
				}
				
				if (++char_num > 10)
					break;

			//Conversion process
			}
			else if(lengths[1] == sizeof(Before_Sep14th_PlayerProfile_Struct)){
				Before_Sep14th_PlayerProfile_Struct* pp = (Before_Sep14th_PlayerProfile_Struct*)row[1];
				strcpy(cs->name[char_num], row[0]);
				// Character information
				cs->level[char_num]				= pp->level;
				cs->class_[char_num]			= pp->class_;
				cs->race[char_num]				= pp->race;
				cs->gender[char_num]			= pp->gender;
				cs->deity[char_num]				= pp->deity;
				cs->zone[char_num]				= GetZoneID(row[2]);
				cs->face[char_num]				= pp->face;
				cs->haircolor[char_num]		= pp->haircolor;
				cs->beardcolor[char_num]	= pp->beardcolor;
				cs->eyecolor2[char_num] 	= pp->eyecolor2;
				cs->eyecolor1[char_num] 	= pp->eyecolor1;
				cs->hair[char_num]				= pp->hairstyle;
				cs->beard[char_num]				= pp->beard;
				for (uint8 material = 0; material <= 8; material++){
					cs->equip[char_num][material] = pp->item_material[material];
					cs->cs_colors[char_num][material].color = pp->item_tint[material].color;
					if ((material==MATERIAL_PRIMARY) || (material==MATERIAL_SECONDARY)) {
						uint32 melee_idx = (material==MATERIAL_PRIMARY) ? 0 : 1;
						cs->melee[melee_idx][char_num] = cs->equip[char_num][material];
					}
				}
				if (++char_num > 10)
					break;
			}
			else if(lengths[1] == sizeof(BeforeFeb18_PlayerProfile_Struct)){
				BeforeFeb18_PlayerProfile_Struct* pp = (BeforeFeb18_PlayerProfile_Struct*)row[1];
				strcpy(cs->name[char_num], row[0]);
				// Character information
				cs->level[char_num]				= pp->level;
				cs->class_[char_num]			= pp->class_;
				cs->race[char_num]				= pp->race;
				cs->gender[char_num]			= pp->gender;
				cs->deity[char_num]				= pp->deity;
				cs->zone[char_num]				= GetZoneID(row[2]);
				cs->face[char_num]				= pp->face;
				cs->haircolor[char_num]		= pp->haircolor;
				cs->beardcolor[char_num]	= pp->beardcolor;
				cs->eyecolor2[char_num] 	= pp->eyecolor2;
				cs->eyecolor1[char_num] 	= pp->eyecolor1;
				cs->hair[char_num]				= pp->hairstyle;
				cs->beard[char_num]				= pp->beard;
				for (uint8 material = 0; material <= 8; material++){
					cs->equip[char_num][material] = pp->item_material[material];
					cs->cs_colors[char_num][material].color = pp->item_tint[material].color;
					if ((material==MATERIAL_PRIMARY) || (material==MATERIAL_SECONDARY)) {
						uint32 melee_idx = (material==MATERIAL_PRIMARY) ? 0 : 1;
						cs->melee[melee_idx][char_num] = cs->equip[char_num][material];
					}
				}
				if (++char_num > 10)
					break;
			}
			else if((lengths[1] == sizeof(BeforeApril14th_PlayerProfile_Struct))) {
				BeforeApril14th_PlayerProfile_Struct* pp = (BeforeApril14th_PlayerProfile_Struct*)row[1];
				strcpy(cs->name[char_num], row[0]);
				// Character information
				cs->level[char_num]				= pp->level;
				cs->class_[char_num]			= pp->class_;
				cs->race[char_num]				= pp->race;
				cs->gender[char_num]			= pp->gender;
				cs->deity[char_num]				= pp->deity;
				cs->zone[char_num]				= GetZoneID(row[2]);
				cs->face[char_num]				= pp->face;
				cs->haircolor[char_num]		= pp->haircolor;
				cs->beardcolor[char_num]	= pp->beardcolor;
				cs->eyecolor2[char_num] 	= pp->eyecolor2;
				cs->eyecolor1[char_num] 	= pp->eyecolor1;
				cs->hair[char_num]				= pp->hairstyle;
				cs->beard[char_num]				= pp->beard;
				for (uint8 material = 0; material <= 8; material++){
					cs->equip[char_num][material] = pp->item_material[material];
					cs->cs_colors[char_num][material].color = pp->item_tint[material].color;
					if ((material==MATERIAL_PRIMARY) || (material==MATERIAL_SECONDARY)) {
						uint32 melee_idx = (material==MATERIAL_PRIMARY) ? 0 : 1;
						cs->melee[melee_idx][char_num] = cs->equip[char_num][material];
					}
				}
				if (++char_num > 10)
					break;
			}
			else if ((lengths[1] == sizeof(PlayerProfile_Struct_Before_May26th))) {
				PlayerProfile_Struct_Before_May26th* pp = (PlayerProfile_Struct_Before_May26th*)row[1];
				strcpy(cs->name[char_num], row[0]);
				// Character information
				cs->level[char_num]				= pp->level;
				cs->class_[char_num]			= pp->class_;
				cs->race[char_num]				= pp->race;
				cs->gender[char_num]			= pp->gender;
				cs->deity[char_num]				= pp->deity;
				cs->zone[char_num]				= GetZoneID(row[2]);
				cs->face[char_num]				= pp->face;
				cs->haircolor[char_num]		= pp->haircolor;
				cs->beardcolor[char_num]	= pp->beardcolor;
				cs->eyecolor2[char_num] 	= pp->eyecolor2;
				cs->eyecolor1[char_num] 	= pp->eyecolor1;
				cs->hair[char_num]				= pp->hairstyle;
				cs->beard[char_num]				= pp->beard;
				for (uint8 material = 0; material <= 8; material++){
					cs->equip[char_num][material] = pp->item_material[material];
					cs->cs_colors[char_num][material].color = pp->item_tint[material].color;
					if ((material==MATERIAL_PRIMARY) || (material==MATERIAL_SECONDARY)) {
						uint32 melee_idx = (material==MATERIAL_PRIMARY) ? 0 : 1;
						cs->melee[melee_idx][char_num] = cs->equip[char_num][material];
					}
				}
				if (++char_num > 10)
					break;
			}
			else if ((lengths[1] == sizeof(BeforeMay5th_PlayerProfile_Struct))) {
				BeforeMay5th_PlayerProfile_Struct* pp = (BeforeMay5th_PlayerProfile_Struct*)row[1];
				strcpy(cs->name[char_num], row[0]);
				// Character information
				cs->level[char_num]				= pp->level;
				cs->class_[char_num]			= pp->class_;
				cs->race[char_num]				= pp->race;
				cs->gender[char_num]			= pp->gender;
				cs->deity[char_num]				= pp->deity;
				cs->zone[char_num]				= GetZoneID(row[2]);
				cs->face[char_num]				= pp->face;
				cs->haircolor[char_num]		= pp->haircolor;
				cs->beardcolor[char_num]	= pp->beardcolor;
				cs->eyecolor2[char_num] 	= pp->eyecolor2;
				cs->eyecolor1[char_num] 	= pp->eyecolor1;
				cs->hair[char_num]				= pp->hairstyle;
				cs->beard[char_num]				= pp->beard;
				for (uint8 material = 0; material <= 8; material++){
					cs->equip[char_num][material] = pp->item_material[material];
					cs->cs_colors[char_num][material].color = pp->item_tint[material].color;
					if ((material==MATERIAL_PRIMARY) || (material==MATERIAL_SECONDARY)) {
						uint32 melee_idx = (material==MATERIAL_PRIMARY) ? 0 : 1;
						cs->melee[melee_idx][char_num] = cs->equip[char_num][material];
					}
				}
				if (++char_num > 10)
					break;
			}
			else if ((lengths[1] == sizeof(BeforeApr21st_PlayerProfile_Struct))) {
				BeforeApr21st_PlayerProfile_Struct* pp = (BeforeApr21st_PlayerProfile_Struct*)row[1];
				strcpy(cs->name[char_num], row[0]);
				// Character information
				cs->level[char_num]				= pp->level;
				cs->class_[char_num]			= pp->class_;
				cs->race[char_num]				= pp->race;
				cs->gender[char_num]			= pp->gender;
				cs->deity[char_num]				= pp->deity;
				cs->zone[char_num]				= GetZoneID(row[2]);
				cs->face[char_num]				= pp->face;
				cs->haircolor[char_num]		= pp->haircolor;
				cs->beardcolor[char_num]	= pp->beardcolor;
				cs->eyecolor2[char_num] 	= pp->eyecolor2;
				cs->eyecolor1[char_num] 	= pp->eyecolor1;
				cs->hair[char_num]				= pp->hairstyle;
				cs->beard[char_num]				= pp->beard;
				for (uint8 material = 0; material <= 8; material++){
					cs->equip[char_num][material] = pp->item_material[material];
					cs->cs_colors[char_num][material].color = pp->item_tint[material].color;
					if ((material==MATERIAL_PRIMARY) || (material==MATERIAL_SECONDARY)) {
						uint32 melee_idx = (material==MATERIAL_PRIMARY) ? 0 : 1;
						cs->melee[melee_idx][char_num] = cs->equip[char_num][material];
					}
				}
				if (++char_num > 10)
					break;
			}
			else if ((lengths[1] == sizeof(Before_Aug13th_PlayerProfile_Struct))) {
				Before_Aug13th_PlayerProfile_Struct* pp = (Before_Aug13th_PlayerProfile_Struct*)row[1];
				strcpy(cs->name[char_num], row[0]);
				// Character information
				cs->level[char_num]				= pp->level;
				cs->class_[char_num]			= pp->class_;
				cs->race[char_num]				= pp->race;
				cs->gender[char_num]			= pp->gender;
				cs->deity[char_num]				= pp->deity;
				cs->zone[char_num]				= GetZoneID(row[2]);
				cs->face[char_num]				= pp->face;
				cs->haircolor[char_num]		= pp->haircolor;
				cs->beardcolor[char_num]	= pp->beardcolor;
				cs->eyecolor2[char_num] 	= pp->eyecolor2;
				cs->eyecolor1[char_num] 	= pp->eyecolor1;
				cs->hair[char_num]				= pp->hairstyle;
				cs->beard[char_num]				= pp->beard;
				for (uint8 material = 0; material <= 8; material++){
					cs->equip[char_num][material] = pp->item_material[material];
					cs->cs_colors[char_num][material].color = pp->item_tint[material].color;
					if ((material==MATERIAL_PRIMARY) || (material==MATERIAL_SECONDARY)) {
						uint32 melee_idx = (material==MATERIAL_PRIMARY) ? 0 : 1;
						cs->melee[melee_idx][char_num] = cs->equip[char_num][material];
					}
				}
				if (++char_num > 10)
					break;
			}
////////////
////////////	This is the current one, the other are for converting
////////////
			else if ((lengths[1] == sizeof(PlayerProfile_Struct))) {
				strcpy(cs->name[char_num], row[0]);
				PlayerProfile_Struct* pp = (PlayerProfile_Struct*)row[1];
				
				// Character information
				cs->level[char_num]				= pp->level;
				cs->class_[char_num]			= pp->class_;
				cs->race[char_num]				= pp->race;
				cs->gender[char_num]			= pp->gender;
				cs->deity[char_num]				= pp->deity;
				cs->zone[char_num]				= GetZoneID(row[2]);
				cs->face[char_num]				= pp->face;
				cs->haircolor[char_num]		= pp->haircolor;
				cs->beardcolor[char_num]	= pp->beardcolor;
				cs->eyecolor2[char_num] 	= pp->eyecolor2;
				cs->eyecolor1[char_num] 	= pp->eyecolor1;
				cs->hair[char_num]				= pp->hairstyle;
				cs->beard[char_num]				= pp->beard;
				
				// Character's equipped items
				// @merth: Haven't done bracer01/bracer02 yet.
				// Also: this needs a second look after items are a little more solid
				// solar: color and all works right now.
				// NOTE: items don't have a color, players MAY have a tint, if the
				// use_tint part is set.  otherwise use the regular color
				inv = new Inventory;
				if(GetInventory(account_id, cs->name[char_num], inv))
				{
					for (uint8 material = 0; material <= 8; material++)
					{
						uint32 color;
						ItemInst *item = inv->GetItem(Inventory::CalcSlotFromMaterial(material));
						if(item == 0)
							continue;

						cs->equip[char_num][material] = item->GetItem()->Common.Material;

						if(pp->item_tint[material].rgb.use_tint)	// they have a tint (LoY dye)
							color = pp->item_tint[material].color;
						else	// no tint, use regular item color
							color = item->GetItem()->Common.Color;

						cs->cs_colors[char_num][material].color = color;

						// the weapons are kept elsewhere
						if ((material==MATERIAL_PRIMARY) || (material==MATERIAL_SECONDARY))
						{
							uint32 melee_idx = (material==MATERIAL_PRIMARY) ? 0 : 1;
							if(strlen(item->GetItem()->IDFile) > 2)
								cs->melee[melee_idx][char_num] = atoi(&item->GetItem()->IDFile[2]);
						}
					}
				}
				else
				{
					printf("Error loading inventory for %s\n", cs->name[char_num]);
				}
				safe_delete(inv);
				
				if (++char_num > 10)
					break;
			}
			else
			{
				cout << "Got a bogus character (" << row[0] << ") Ignoring!!!" << endl;
				cout << "PP length ="<<lengths[1]<<endl;
				//DeleteCharacter(row[0]);
			}
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in GetCharSelectInfo query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return;
	}
	
	return;
}

// Load child objects for a world container (i.e., forge, bag dropped to ground, etc)
void Database::LoadWorldContainer(uint32 parentid, ItemContainerInst* container)
{
	if (!container) {
		LogFile->write(EQEMuLog::Error, "Programming error: LoadWorldContainer passed NULL pointer");
		return;
	}
	
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	//const Item_Struct* item = NULL;
	//ItemInst* inst = NULL;
	
	uint32 len_query =  MakeAnyLenString(&query, "select "
		"bagidx,itemid,charges from object_contents where parentid=%i", parentid);
	
	if (database.RunQuery(query, len_query, errbuf, &result)) {
		while ((row = mysql_fetch_row(result))) {
			uint8 index = (uint8)atoi(row[0]);
			uint32 item_id = (uint32)atoi(row[1]);
			sint8 charges = (sint8)atoi(row[2]);
			
			ItemInst* inst = ItemInst::Create(item_id, charges);
			if (inst) {
				// Put item inside world container
				container->PutItem(index, *inst);
				safe_delete(inst);
			}
		}
		mysql_free_result(result);
	}
	else {
		LogFile->write(EQEMuLog::Error, "Error in DB::LoadWorldContainer: %s", errbuf);
	}
	
	safe_delete_array(query);
}

// Save child objects for a world container (i.e., forge, bag dropped to ground, etc)
void Database::SaveWorldContainer(uint32 zone_id, uint32 parent_id, const ItemContainerInst* container)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
	
	// Since state is not saved for each world container action, we'll just delete
	// all and save from scratch .. we may come back later to optimize
	//DeleteWorldContainer(parent_id);
	
	if (!container) {
		return;
	}
	//Delete all items from container
	DeleteWorldContainer(parent_id,zone_id);
	// Save all 10 items, if they exist
	for (uint8 index=0; index<10; index++) {
		ItemInst* inst = container->GetItem(index);
		if (inst && (int32)inst->GetItem()!=0xFEEEFEEE) {
			uint32 item_id = inst->GetItem()->ItemNumber;
			uint32 len_query = MakeAnyLenString(&query,
				"replace into object_contents values(%i,%i,%i,%i,%i,now())",
				zone_id, parent_id, index, item_id, inst->GetCharges());
			
			if (!RunQuery(query, len_query, errbuf)) {
				LogFile->write(EQEMuLog::Error, "Error in Database::SaveWorldContainer: %s", errbuf);
			}
			safe_delete_array(query);
		}

	}
}

// Remove all child objects inside a world container (i.e., forge, bag dropped to ground, etc)
void Database::DeleteWorldContainer(uint32 parent_id,uint32 zone_id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
	
	uint32 len_query = MakeAnyLenString(&query,
		"delete from object_contents where parentid=%i and zoneid=%i", parent_id,zone_id);
	if (!RunQuery(query, len_query, errbuf)) {
		LogFile->write(EQEMuLog::Error, "Error in Database::DeleteWorldContainer: %s", errbuf);
	}
	
	safe_delete_array(query);
}

// Add new Zone Object (theoretically only called for items dropped to ground)
uint32 Database::AddObject(uint32 type, uint32 icon, const Object_Struct& object, const ItemInst* inst)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
	
	uint32 database_id = 0;
	uint32 item_id = 0;
	sint8 charges = 0;
	
	if (inst && inst->GetItem()) {
		item_id = inst->GetItem()->ItemNumber;
		charges = inst->GetCharges();
	}
	
	// SQL Escape object_name
	uint32 len = strlen(object.object_name) * 2 + 1;
	char* object_name = new char[len];
	DoEscapeString(object_name, object.object_name, strlen(object.object_name));
	
	// Construct query
	uint32 len_query = MakeAnyLenString(&query,
		"insert into object (zoneid, xpos, ypos, zpos, heading, itemid, charges, objectname, "
		"type, icon) values (%i, %f, %f, %f, %f, %i, %i, '%s', %i, %i)",
		object.zone_id, object.x, object.y, object.z, object.heading,
		item_id, charges, object_name, type, icon);
	
	// Save new record for object
	if (!RunQuery(query, len_query, errbuf, NULL, NULL, &database_id)) {
		LogFile->write(EQEMuLog::Error, "Unable to insert object: %s", errbuf);
	}
	else {
		// Save container contents, if container
		if (inst && inst->IsType(ItemTypeContainer)) {
			SaveWorldContainer(object.zone_id, database_id, (const ItemContainerInst*)inst);
		}
	}
	
	safe_delete_array(object_name);
	safe_delete_array(query);
	return database_id;
}

// Update information about existing object in database
void Database::UpdateObject(uint32 id, uint32 type, uint32 icon, const Object_Struct& object, const ItemInst* inst)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
	
	uint32 item_id = 0;
	sint8 charges = 0;
	
	if (inst && inst->GetItem()) {
		item_id = inst->GetItem()->ItemNumber;
		charges = inst->GetCharges();
	}
	
	// SQL Escape object_name
	uint32 len = strlen(object.object_name) * 2 + 1;
	char* object_name = new char[len];
	DoEscapeString(object_name, object.object_name, strlen(object.object_name));
	
	// Construct query
	uint32 len_query = MakeAnyLenString(&query,
		"update object set zoneid=%i, xpos=%f, ypos=%f, zpos=%f, heading=%f, "
		"itemid=%i, charges=%i, objectname='%s', type=%i, icon=%i where id=%i",
		object.zone_id, object.x, object.y, object.z, object.heading,
		item_id, charges, object_name, type, icon, id);
	
	// Save new record for object
	if (!RunQuery(query, len_query, errbuf)) {
		LogFile->write(EQEMuLog::Error, "Unable to update object: %s", errbuf);
	}
	else {
		// Save container contents, if container
		if (inst && inst->IsType(ItemTypeContainer)) {
			SaveWorldContainer(object.zone_id, id, (const ItemContainerInst*)inst);
		}
	}
	
	safe_delete_array(object_name);
	safe_delete_array(query);
}
Ground_Spawns*	Database::LoadGroundSpawns(int32 zone_id,Ground_Spawns* gs){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT max_x,max_y,max_z,min_x,min_y,heading,name,item,max_allowed,respawn_timer from ground_spawns where zoneid=%i limit 50", zone_id), errbuf, &result))
	{
		safe_delete_array(query);
		int i=0;
		while( (row=mysql_fetch_row(result) ) ) {
			gs->spawn[i].max_x=atof(row[0]);
			gs->spawn[i].max_y=atof(row[1]);
			gs->spawn[i].max_z=atof(row[2]);
			gs->spawn[i].min_x=atof(row[3]);
			gs->spawn[i].min_y=atof(row[4]);
			gs->spawn[i].heading=atof(row[5]);
			strcpy(gs->spawn[i].name,row[6]);
			gs->spawn[i].item=atoi(row[7]);
			gs->spawn[i].max_allowed=atoi(row[8]);
			gs->spawn[i].respawntimer=atoi(row[9]);
			i++;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in LoadGroundSpawns query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	return gs;
}
void Database::DeleteObject(uint32 id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
	
	// Construct query
	uint32 len_query = MakeAnyLenString(&query,
		"delete from object where id=%i", id);
	
	// Save new record for object
	if (!RunQuery(query, len_query, errbuf)) {
		LogFile->write(EQEMuLog::Error, "Unable to delete object: %s", errbuf);
	}
	//else {
		// Delete contained items, if any
	//	DeleteWorldContainer(id);
	//}
	
	safe_delete_array(query);
}

Trader_Struct* Database::LoadTraderItem(uint32 char_id){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	Trader_Struct* loadti = new Trader_Struct;
	memset(loadti,0,sizeof(Trader_Struct));
	if (RunQuery(query,MakeAnyLenString(&query, "select * from trader where char_id=%i order by slot_id limit 80",char_id),errbuf,&result)){
		safe_delete_array(query);
		loadti->code=11;
		while ((row = mysql_fetch_row(result))) {
			if(atoi(row[3])>=80 || atoi(row[3])<0)
				printf("Bad Slot number when trying to load trader information!\n");
			else{
				loadti->itemid[atoi(row[3])]=atoi(row[1]);
				loadti->itemcost[atoi(row[3])]=atoi(row[2]);
			}
		}
		mysql_free_result(result);
	}
	else{
		safe_delete_array(query);
		printf("Failed to load trader information!\n");
	}
	return loadti;
}
void Database::SaveTraderItem(uint32 char_id,uint32 itemid,uint32 itemcost,int8 slot){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
	if (!(RunQuery(query,MakeAnyLenString(&query, "replace INTO trader VALUES(%i,%i,%i,%i)",char_id, itemid, itemcost, slot),errbuf)))
		printf("Failed to save trader item: %i for char_id: %i, the error was: %s\n",itemid,char_id,errbuf);
	safe_delete_array(query);
}
void Database::DeleteTraderItem(uint32 char_id){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
	if(char_id==0){
		if (!(RunQuery(query,MakeAnyLenString(&query, "delete from trader"),errbuf)))
			printf("Failed to delete all trader items data, the error was: %s\n",errbuf);
	}
	else{
		if (!(RunQuery(query,MakeAnyLenString(&query, "delete from trader where char_id=%i",char_id),errbuf)))
			printf("Failed to delete trader item data for char_id: %i, the error was: %s\n",char_id,errbuf);
	}
	safe_delete_array(query);
}
void Database::DeleteTraderItem(uint32 char_id,int16 slot_id){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
	if (!(RunQuery(query,MakeAnyLenString(&query, "delete from trader where char_id=%i and slot_id=%i",char_id,slot_id),errbuf)))
		printf("Failed to delete trader item data for char_id: %i, the error was: %s\n",char_id,errbuf);
	safe_delete_array(query);
}
bool Database::SaveInventory(uint32 char_id, const ItemInst* inst, sint16 slot_id) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
	bool ret = false;
	
	if (slot_id>=2500 && slot_id<=2600) { // Shared bank inventory
		if (!inst) {
			// Delete item
			uint32 account_id = GetAccountIDByChar(char_id);
			uint32 len_query = MakeAnyLenString(&query, "DELETE FROM sharedbank WHERE acctid=%i AND slotid=%i",
				account_id, slot_id);
			
			ret = RunQuery(query, len_query, errbuf);
			
			// Delete bag slots, if need be
			if (ret && Inventory::SupportsContainers(slot_id)) {
				safe_delete_array(query);
				sint16 base_slot_id = Inventory::CalcSlotId(slot_id, 0);
				ret = RunQuery(query, MakeAnyLenString(&query, "DELETE FROM inventory WHERE charid=%i AND slotid>=%i AND slotid<%i",
					char_id, base_slot_id, (base_slot_id+10)), errbuf);
			}
			
			// @merth: need to delete augments here
		}
		else {
			// Update/Insert item
			uint32 account_id = GetAccountIDByChar(char_id);
			uint32 len_query =  MakeAnyLenString(&query, "REPLACE INTO sharedbank VALUES(%i,%i,%i,%i)",
				account_id, slot_id, inst->GetItem()->ItemNumber, inst->GetCharges());
			
			ret = RunQuery(query, len_query, errbuf);
		}
	}
	else { // All other inventory
		if (!inst) {
			// Delete item
			ret = RunQuery(query, MakeAnyLenString(&query, "DELETE FROM inventory WHERE charid=%i AND slotid=%i",
				char_id, slot_id), errbuf);
			
			// Delete bag slots, if need be
			if (ret && Inventory::SupportsContainers(slot_id)) {
				safe_delete_array(query);
				sint16 base_slot_id = Inventory::CalcSlotId(slot_id, 0);
				ret = RunQuery(query, MakeAnyLenString(&query, "DELETE FROM inventory WHERE charid=%i AND slotid>=%i AND slotid<%i",
					char_id, base_slot_id, (base_slot_id+10)), errbuf);
			}
			
			// @merth: need to delete augments here
		}
		else {
			// Update/Insert item
			// solar: color isn't needed
			uint32 len_query = MakeAnyLenString(&query, "REPLACE INTO inventory (charid,slotid,itemid,charges,color) VALUES(%i,%i,%i,%i,%i)",
				char_id, slot_id, inst->GetItem()->ItemNumber, inst->GetCharges(),/*inst->GetColor()*/ 0 );
			
			ret = RunQuery(query, len_query, errbuf);
		}
	}
	
	if (!ret)
		LogFile->write(EQEMuLog::Error, "SaveInventory query '%s': %s", query, errbuf);
	safe_delete_array(query);
	
	// Save bag contents, if slot supports bag contents
	if (inst && inst->IsType(ItemTypeContainer) && Inventory::SupportsContainers(slot_id)) {
		const ItemContainerInst* bag = (const ItemContainerInst*)inst;
		for (uint8 idx=0; idx<10; idx++) {
			const ItemInst* baginst = bag->GetItem(idx);
			SaveInventory(char_id, baginst, Inventory::CalcSlotId(slot_id, idx));
		}
	}
	
	// @merth: need to save augments here
	
	return ret;
}

bool Database::ReserveName(int32 account_id, char* name)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;

	
	//if (strlen(name) > 15)
	//	return false;
	
	/*for (int i=0; i<strlen(name); i++)
	{
	if ((name[i] < 'a' || name[i] > 'z') && 
	(name[i] < 'A' || name[i] > 'Z'))
	return 0;
	if (i > 0 && name[i] >= 'A' && name[i] <= 'Z')
	{
	name[i] = name[i]+'a'-'A';
	}
}*/
	
	if (!RunQuery(query, MakeAnyLenString(&query, "INSERT into character_ SET account_id=%i, name='%s', profile=NULL", account_id, name), errbuf)) {
		cerr << "Error in ReserveName query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	safe_delete_array(query);
	return true;
}

/*
Delete the character with the name "name"
returns false on failure, true otherwise
*/
bool Database::DeleteCharacter(char *name)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query=0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int charid, matches;
	int32 affected_rows;

	if(!name ||	!strlen(name))
	{
		printf("DeleteCharacter: request to delete without a name (empty char slot)\n");
		return false;
	}

// SCORPIOUS2K - get id from character_ before deleting record so we can clean up inventory and qglobal

#if DEBUG >= 5
	printf("DeleteCharacter: Attempting to delete '%s'\n", name);
#endif
	RunQuery(query, MakeAnyLenString(&query, "SELECT id from character_ WHERE name='%s'", name), errbuf, &result);
	if (query)
	{
		safe_delete_array(query);
		query = NULL;
	}
	matches = mysql_num_rows(result);
	if(matches == 1)
	{
		row = mysql_fetch_row(result);
		charid = atoi(row[0]);
#if DEBUG >= 5
		printf("DeleteCharacter: found '%s' with char id: %d\n", name, charid);
#endif
	}
	else
	{
		printf("DeleteCharacter: error: got %d rows matching '%s'\n", matches, name);
		if(result)
		{
			mysql_free_result(result);
			result = NULL;
		}
		return false;
	}

	if(result)
	{
		mysql_free_result(result);
		result = NULL;
	}



#if DEBUG >= 5
	printf("DeleteCharacter: deleting '%s' (id %d): ", name, charid);
	printf(" quest_globals");
#endif
	RunQuery(query, MakeAnyLenString(&query, "DELETE from quest_globals WHERE charid='%d'", charid), errbuf, NULL, &affected_rows);
	if(query)
	{
		safe_delete_array(query);
		query = NULL;
	}

#if DEBUG >= 5
	printf(" inventory");
#endif
	RunQuery(query, MakeAnyLenString(&query, "DELETE from inventory WHERE charid='%d'", charid), errbuf, NULL, &affected_rows);
	if(query)
	{
		safe_delete_array(query);
		query = NULL;
	}

#if DEBUG >= 5
	printf(" _character");
#endif
	RunQuery(query, MakeAnyLenString(&query, "DELETE from character_ WHERE id='%d'", charid), errbuf, NULL, &affected_rows);
	if(query)
	{
		safe_delete_array(query);
		query = NULL;
	}
	if(affected_rows != 1)	// here we have to have a match or it's an error
	{
		LogFile->write(EQEMuLog::Error, "DeleteCharacter: error: delete operation affected %d rows\n", affected_rows);
		return false;
	}

#if DEBUG >= 5
	printf("\n");
#endif
	printf("DeleteCharacter: successfully deleted '%s' (id %d)\n", name, charid);
	
	return true;
}

// Store new character information into the character_ and inventory tables
bool Database::StoreCharacter(uint32 account_id, PlayerProfile_Struct* pp, Inventory* inv)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char query[256+sizeof(PlayerProfile_Struct)*2+sizeof(PlayerAA_Struct)*2+5];
	char* end = query;
	char playeraa[500]={0};
	int32 affected_rows = 0;
	int i;
	int32 charid = 0;
	char* charidquery = 0;
	char* invquery = 0;
	MYSQL_RES *result;
	MYSQL_ROW row = 0;
	char zone[50];
	float x, y, z;

	// get the char id (used in inventory inserts below)
	RunQuery
	(
		charidquery,
		MakeAnyLenString
		(
			&charidquery,
			"SELECT id FROM character_ where name=\'%s\'",
			pp->name
		),
		errbuf,
		&result
	);
	safe_delete_array(charidquery);

	if(mysql_num_rows(result) == 1)
	{
		row = mysql_fetch_row(result);
		if(row[0])
			charid = atoi(row[0]);
	}

	if(!charid)
	{
		LogFile->write(EQEMuLog::Error, "StoreCharacter: no character id");
		return false;
	}


	strncpy(zone, GetZoneName(pp->zone_id), 49);
	x=pp->x;
	y=pp->y;
	z=pp->z;

	// construct the character_ query
	end += sprintf(
		end,
		"UPDATE character_ SET timelaston=0, guild=0, "
		"zonename=\'%s\', x=%f, y=%f, z=%f, profile=\'",
		zone, x, y, z
	);
	end += DoEscapeString(end, (char*)pp, sizeof(PlayerProfile_Struct));
	end += sprintf(end, "\', alt_adv=\'");
	end += DoEscapeString(end, (char*)playeraa, sizeof(PlayerAA_Struct));
	end += sprintf(
		end, 
		"\' WHERE account_id=%d AND name='%s'",
		account_id, pp->name
	);
	
	RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows);
	
	if(!affected_rows)
	{
		LogFile->write(EQEMuLog::Error, "StoreCharacter query '%s' %s", query, errbuf);
		return false;
	}

	affected_rows = 0;


	// now the inventory
	for (i=22; i<=29; i++)
	{
		const ItemInst* newinv = inv->GetItem((sint16)i);
		if (newinv)
		{
			MakeAnyLenString
			(
				// solar: color isn't needed
				&invquery,
				"INSERT INTO inventory SET "
				"charid=%d, slotid=%d, itemid=%d, charges=%d, color=%d",
				charid, i, newinv->GetItem()->ItemNumber, 
				newinv->GetCharges(), /*newinv->GetColor()*/ 0
			);

			RunQuery(invquery, strlen(invquery), errbuf, 0, &affected_rows);
			safe_delete_array(invquery);
			if(!affected_rows)
			{
				LogFile->write(EQEMuLog::Error, "StoreCharacter inventory failed.  Query '%s' %s", invquery, errbuf);
			}
#if EQDEBUG >= 9
			else
			{
				LogFile->write(EQEMuLog::Debug, "StoreCharacter inventory succeeded.  Query '%s' %s", invquery, errbuf);
			}
#endif
		} 
	}
	return true;
}

bool Database::SetStartingItems(PlayerProfile_Struct* pp, Inventory* inv, uint32 si_race, uint32 si_class, uint32 si_deity, uint32 si_current_zone, char* si_name, int admin_level)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	int invslot;
	MYSQL_RES *result;
	MYSQL_ROW row;
	const Item_Struct* myitem;
	
	RunQuery
	(
		query,
		MakeAnyLenString
		(
			&query,
			"SELECT itemid, item_charges FROM starting_items "
			"WHERE (race = %i or race = 0) AND (class = %i or class = 0) AND "
			"(deityid = %i or deityid=0) AND (zoneid = %i or zoneid = 0) AND "
			"gm <= %i ORDER BY id",
			si_race, si_class, si_deity, si_current_zone, admin_level
		),
		errbuf,
		&result
	);
	safe_delete_array(query);

	if(mysql_num_rows(result))
	{
		for(invslot = 22; (row = mysql_fetch_row(result)) && invslot < 30; invslot++)
		{
			if(row[0] && row[1])
			{
				myitem = database.GetItem(atoi(row[0]));
				if(myitem && myitem->ItemClass == ItemTypeCommon)
				{
					ItemCommonInst mycommonitem(myitem, atoi(row[1]));
					// could use invslot here
					inv->PutItem(inv->FindFreeSlot(0,0), mycommonitem);
				}
			}
		}
	}

	if(result) mysql_free_result(result);

	return true;
}


void  Database::SetGroupID(const char* name,int32 id){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "update character_ set groupid=%i where name='%s'",id,name), errbuf))
		printf("Unable to get group id: %s\n",errbuf);	
	safe_delete_array(query);
}
int32 Database::GetGroupID(const char* name){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
	MYSQL_ROW row;
	int32 groupid=0;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT groupid from character_ where name='%s'", name), errbuf, &result)) {
		if((row = mysql_fetch_row(result)))
		{
			if(row[0])
				groupid=atoi(row[0]);
		}
		else
			printf("Unable to get group id, char not found!\n");
		mysql_free_result(result);
	}
	else
			printf("Unable to get group id: %s\n",errbuf);
	safe_delete_array(query);
	return groupid;
}
char* Database::GetGroupLeaderForLogin(const char* name,char* leaderbuf){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
	MYSQL_ROW row;
	PlayerProfile_Struct pp;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT profile from character_ where name='%s'", name), errbuf, &result)) {
		row = mysql_fetch_row(result);
		unsigned long* lengths = mysql_fetch_lengths(result);
		if (lengths[0] == sizeof(PlayerProfile_Struct)) {
			memcpy(&pp, row[0], sizeof(PlayerProfile_Struct));
			strcpy(leaderbuf,pp.groupMembers[0]);
		}
		mysql_free_result(result);
	}
	else{
			printf("Unable to get leader name: %s\n",errbuf);
	}
	safe_delete_array(query);
	return leaderbuf;
}
bool Database::GetCharacterInfoForLogin(const char* name, uint32* character_id, char* current_zone, PlayerProfile_Struct* pp, Inventory* inv, uint32* pplen, PlayerAA_Struct* aa, int32* aalen, uint32* guilddbid, int8* guildrank) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 querylen;
    MYSQL_RES *result;
	
	bool ret = false;
	
	if (character_id && *character_id) {
		// searching by ID should be a lil bit faster
		querylen = MakeAnyLenString(&query, "SELECT id, profile, zonename, x, y, z, alt_adv, guild, guildrank FROM character_ WHERE id=%i", *character_id);
	}
	else {
		querylen = MakeAnyLenString(&query, "SELECT id, profile,zonename, x, y, z, alt_adv, guild, guildrank FROM character_ WHERE name='%s'", name);
	}
	
	if (RunQuery(query, querylen, errbuf, &result)) {
		ret = GetCharacterInfoForLogin_result(result, character_id, current_zone, pp, inv, pplen, aa, aalen, guilddbid, guildrank);
		mysql_free_result(result);
	}
	else {
		LogFile->write(EQEMuLog::Error, "GetCharacterInfoForLogin query '%s' %s", query, errbuf);
	}
	
	safe_delete_array(query);
	return ret;
}
void Database::UpdateAndDeleteAATimers(int32 charid){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	char *query2 = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "delete from aa_timers where charid=%i and UNIX_TIMESTAMP(now())>=end",charid), errbuf)) {
		LogFile->write(EQEMuLog::Error, "UpdateAATimers query '%s' %s", query, errbuf);
	}
	if (!RunQuery(query2, MakeAnyLenString(&query2, "update aa_timers set end=end-(UNIX_TIMESTAMP(now())-begin),begin=UNIX_TIMESTAMP(now()) where charid=%i",charid), errbuf)) {
		LogFile->write(EQEMuLog::Error, "UpdateAATimers query '%s' %s", query2, errbuf);
	}
	safe_delete_array(query);
	safe_delete_array(query2);
}
void Database::UpdateTimersClientConnected(int32 charid){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "update aa_timers set end=(UNIX_TIMESTAMP(now())+(end-begin)),begin=UNIX_TIMESTAMP(now()) where charid=%i",charid), errbuf)) {
		LogFile->write(EQEMuLog::Error, "UpdateAATimers query '%s' %s", query, errbuf);
	}
	safe_delete_array(query);
}
#ifdef ZONE
void Database::GetAATimers(int32 charid){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
	MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT ability,begin,end from aa_timers WHERE charid=%i", charid), errbuf, &result)) {
		while( ( row = mysql_fetch_row(result) ) ){
			UseAA_Struct* uaa=new UseAA_Struct();
			uaa->ability=atoi(row[0]);
			uaa->begin=atoi(row[1]);
			uaa->end=atoi(row[2]);
			entity_list.SendAATimer(charid,uaa);
			safe_delete(uaa);
		}
		mysql_free_result(result);
	}
	else {
		LogFile->write(EQEMuLog::Error, "Database::GetAATimers query '%s' %s", query, errbuf);
	}	
	safe_delete_array(query);
}
#endif
int32 Database::GetTimerRemaining(int32 charid,int32 ability){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
	MYSQL_ROW row;
	int32 remain=0;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT end-begin from aa_timers WHERE charid=%i and ability=%i", charid,ability), errbuf, &result)) {
		if((row=mysql_fetch_row(result))){
			remain=atoi(row[0]);
		}
		mysql_free_result(result);
	}
	else {
		LogFile->write(EQEMuLog::Error, "Database::GetTimerRemaining query '%s' %s", query, errbuf);
	}	
	safe_delete_array(query);
	return remain;
}
void Database::UpdateAATimers(int32 charid,int32 endtime,int32 begintime,int32 ability){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	if(begintime==0){
		if (!RunQuery(query, MakeAnyLenString(&query, "replace into aa_timers (charid,end,begin,ability) values(%i,UNIX_TIMESTAMP(now())+%i,UNIX_TIMESTAMP(now()),%i)",charid,endtime,ability), errbuf)) {
			LogFile->write(EQEMuLog::Error, "UpdateAATimers query '%s' %s", query, errbuf);
		}
	}
	else{
		if (!RunQuery(query, MakeAnyLenString(&query, "replace into aa_timers (charid,end,begin,ability) values(%i,%i,%i,%i)",charid,endtime,begintime,ability), errbuf)) {
			LogFile->write(EQEMuLog::Error, "UpdateAATimers query '%s' %s", query, errbuf);
		}
	}
	safe_delete_array(query);
}
// Process results of GetCharacterInfoForLogin()
// Query this processes: SELECT id,profile,zonename,x,y,z,alt_adv,guild,guildrank FROM character_ WHERE id=%i
bool Database::GetCharacterInfoForLogin_result(MYSQL_RES* result, int32* character_id, char* current_zone, PlayerProfile_Struct* pp, Inventory* inv, uint32* pplen, PlayerAA_Struct* aa, uint32* aalen, uint32* guilddbid, int8* guildrank) {
    MYSQL_ROW row;
	unsigned long* lengths;
	
	if (mysql_num_rows(result) == 1) {
		row = mysql_fetch_row(result);
		lengths = mysql_fetch_lengths(result);
		if (pp && pplen) {
			if (lengths[1] == sizeof(PlayerProfile_Struct)) {
				memcpy(pp, row[1], sizeof(PlayerProfile_Struct));
			}
			else if(lengths[1] == sizeof(OldPlayerProfile_Struct)) {
				LogFile->write(EQEMuLog::Status, "1. Player profile being converted from a VERY old patch date now....");
				OldPlayerProfile_Struct* ops = (OldPlayerProfile_Struct*)row[1];
				memcpy(pp,ops,sizeof(OldPlayerProfile_Struct));
			}
			else if(lengths[1] == sizeof(BeforeFeb18_PlayerProfile_Struct)) {
				LogFile->write(EQEMuLog::Status, "2. Player profile being converted from the February 18th patch now....");
				BeforeFeb18_PlayerProfile_Struct* ops = (BeforeFeb18_PlayerProfile_Struct*)row[1];
				memcpy(pp,ops,sizeof(BeforeFeb18_PlayerProfile_Struct));
			}
			else if(lengths[1] == sizeof(BeforeApril14th_PlayerProfile_Struct)) {
				LogFile->write(EQEMuLog::Status, "3. Player profile being converted from the April 14th patch now....");
				BeforeApril14th_PlayerProfile_Struct* ops = (BeforeApril14th_PlayerProfile_Struct*)row[1];
				memcpy(pp,ops,sizeof(BeforeApril14th_PlayerProfile_Struct));
			}
			else if(lengths[1] == sizeof(BeforeApr21st_PlayerProfile_Struct)) {
				LogFile->write(EQEMuLog::Status, "4. Player profile being converted from the April 21st patch now....");
				BeforeApr21st_PlayerProfile_Struct* ops = (BeforeApr21st_PlayerProfile_Struct*)row[1];
				memcpy(pp,ops,sizeof(BeforeApr21st_PlayerProfile_Struct));
			}
			else if(lengths[1] == sizeof(BeforeMay5th_PlayerProfile_Struct)) {
				LogFile->write(EQEMuLog::Status, "5. Player profile being converted from the May 5th patch now....");
				BeforeMay5th_PlayerProfile_Struct* ops = (BeforeMay5th_PlayerProfile_Struct*)row[1];
				memcpy(pp,ops,sizeof(BeforeMay5th_PlayerProfile_Struct));
			}
			else if(lengths[1] == sizeof(PlayerProfile_Struct_Before_May26th)) {
				LogFile->write(EQEMuLog::Status, "6. Player profile being converted from the May 26th patch now....");
				PlayerProfile_Struct_Before_May26th* ops = (PlayerProfile_Struct_Before_May26th*)row[1];
				memcpy(pp,ops,sizeof(PlayerProfile_Struct_Before_May26th));
			}
			else if(lengths[1] == sizeof(Before_Aug13th_PlayerProfile_Struct)) {
				LogFile->write(EQEMuLog::Status, "7. Player profile being converted from the Aug 13th patch now....");
				Before_Aug13th_PlayerProfile_Struct* ops = (Before_Aug13th_PlayerProfile_Struct*)row[1];
				uchar* ptr=(uchar*)pp;
				uchar* c_ptr=(uchar*)ops;
				memcpy(ptr,c_ptr,588);
				c_ptr+=588;
				ptr+=1304;
				memcpy(ptr,c_ptr,4440);
			}
			else if(lengths[1] == sizeof(Before_Sep14th_PlayerProfile_Struct)) {
				LogFile->write(EQEMuLog::Status, "8. Player profile being converted from the Sep 14th patch now....");
				Before_Sep14th_PlayerProfile_Struct* oldpp =(Before_Sep14th_PlayerProfile_Struct*)row[1];
				uchar* ptr=(uchar*)pp;
				uchar* s_ptr=(uchar*)oldpp;
				memcpy(ptr,s_ptr,124);
				s_ptr+=140;
				ptr+=220;
				memcpy(ptr,s_ptr,88);
				s_ptr+=88;
				ptr+=92;
				memcpy(ptr,s_ptr,1140);
				s_ptr+=1140;
				ptr+=1176;
				memcpy(ptr,s_ptr,8);
				s_ptr+=8;
				ptr+=12;
				memcpy(ptr,s_ptr,3700);
				//s_ptr+=3020;
				//ptr+=3024;
				//memcpy(ptr,s_ptr,680);
				pp->bind_x[0]=oldpp->bind_x;
				pp->bind_y[0]=oldpp->bind_y;
				pp->bind_z[0]=oldpp->bind_z;
			}
			else {
				LogFile->write(EQEMuLog::Error, "Player profile length mismatch in GetCharacterInfo Expected: %i, Got: %i",
					sizeof(PlayerProfile_Struct), lengths[1]);
				return false;
			}
			
			*pplen = lengths[1];
			pp->zone_id = GetZoneID(row[2]);
			
			pp->x = atof(row[3]);
			pp->y = atof(row[4]);
			pp->z = atof(row[5]);
			
			if (pp->x == -1 && pp->y == -1 && pp->z == -1)
				GetSafePoints(pp->zone_id, &pp->x, &pp->y, &pp->z);
		}
		
		uint32 char_id = atoi(row[0]);
		if (character_id)
			*character_id = char_id;
		if (current_zone)
			strcpy(current_zone, row[2]);
		if (aa && aalen) {
			if(row[6] && (lengths[6] >= sizeof(PlayerAA_Struct))) {
				memcpy(aa, row[6], sizeof(PlayerAA_Struct));
				*aalen = result->lengths[6];
			}
			else { // let's support ghetto-ALTERed databases that don't contain any data in the alt_adv column
				memset(aa, 0, sizeof(PlayerAA_Struct));
				*aalen = sizeof(PlayerAA_Struct);
				LogFile->write(EQEMuLog::Error, "Warning: Invalid PlayerAA_Struct size found in database");
			}
		}
		if (guilddbid)
			*guilddbid = atoi(row[7]);
		if (guildrank)
			*guildrank = atoi(row[8]);
		
		// Retrieve character inventory
		return GetInventory(char_id, inv);
	}
	
	return false;
}

// Retrieve shared bank inventory based on either account or character
bool Database::GetSharedBank(uint32 id, Inventory* inv, bool is_charid) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
    uint32 len_query = 0;
	MYSQL_RES *result;
    MYSQL_ROW row;
	bool ret = false;
	
	if (is_charid) {
		len_query = MakeAnyLenString(&query,
			"SELECT sb.slotid,sb.itemid,sb.charges from sharedbank sb "
			"INNER JOIN character_ ch ON ch.account_id=sb.acctid "
			"WHERE ch.id=%i", id);
	}
	else {
		len_query = MakeAnyLenString(&query,
			"SELECT slotid,itemid,charges from sharedbank WHERE acctid=%i", id);
	}
	
	if (RunQuery(query, len_query, errbuf, &result)) {
		while ((row = mysql_fetch_row(result))) {
			sint16 slot_id	= (sint16)atoi(row[0]);
			uint32 item_id	= (uint32)atoi(row[1]);
			sint8 charges	= (sint8)atoi(row[2]);
			const Item_Struct* item = GetItem(item_id);
			
			if (item) {
				sint16 put_slot_id = SLOT_INVALID;
				
				if (item->ItemClass == ItemTypeCommon) {
					ItemCommonInst common(item, charges);
					put_slot_id = inv->PutItem(slot_id, (ItemInst&)common);
				}
				else if (item->ItemClass == ItemTypeContainer) {
					ItemContainerInst bag(item, charges);
					put_slot_id = inv->PutItem(slot_id, (ItemInst&)bag);
				}
				else if (item->ItemClass == ItemTypeBook) {
					ItemBookInst book(item, charges);
					put_slot_id = inv->PutItem(slot_id, (ItemInst&)book);
				}
				
				// Save ptr to item in inventory
				if (put_slot_id == SLOT_INVALID) {
					LogFile->write(EQEMuLog::Error,
						"Warning: Invalid slot_id for item in shared bank inventory: %s=%i, item_id=%i, slot_id=%i",
						((is_charid==true) ? "charid" : "acctid"), id, item_id, slot_id);
				}
			}
			else {
				LogFile->write(EQEMuLog::Error,
					"Warning: %s %i has an invalid item_id %i in inventory slot %i",
					((is_charid==true) ? "charid" : "acctid"), id, item_id, slot_id);
			}
		}
		
		mysql_free_result(result);
		ret = true;
	}
	else
		LogFile->write(EQEMuLog::Error, "Database::GetSharedBank(int32 account_id): %s", errbuf);
	
	safe_delete_array(query);
	return ret;
}

// Get the player profile and inventory for the given account "account_id" and
// character name "name".  Return true if the character was found, otherwise false.
// False will also be returned if there is a database error.
bool Database::GetPlayerProfile(int32 account_id, char* name, PlayerProfile_Struct* pp, Inventory* inv, char* current_zone) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
    MYSQL_RES* result;
    MYSQL_ROW row;
	bool ret = false;
	
	unsigned long* lengths;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT profile,zonename,x,y,z FROM character_ WHERE account_id=%i AND name='%s'", account_id, name), errbuf, &result)) {
		if (mysql_num_rows(result) == 1) {	
			row = mysql_fetch_row(result);
			lengths = mysql_fetch_lengths(result);
			if (lengths[0] == sizeof(PlayerProfile_Struct)) {
				memcpy(pp, row[0], sizeof(PlayerProfile_Struct));
				
				if (current_zone)
					strcpy(current_zone, row[1]);
				pp->zone_id = GetZoneID(row[1]);
				pp->x = atof(row[2]);
				pp->y = atof(row[3]);
				pp->z = atof(row[4]);
				if (pp->x == -1 && pp->y == -1 && pp->z == -1)
					GetSafePoints(pp->zone_id, &pp->x, &pp->y, &pp->z);
				
				// Retrieve character inventory
				ret = GetInventory(account_id, name, inv);
			}
			else {
				LogFile->write(EQEMuLog::Error, "Player profile length mismatch in GetPlayerProfile. Found: %i, Expected: %i",
					lengths[0], sizeof(PlayerProfile_Struct));
			}
		}
		
		mysql_free_result(result);
	}
	else {
		LogFile->write(EQEMuLog::Error, "GetPlayerProfile query '%s' %s", query, errbuf);
	}
	
	safe_delete_array(query);
	return ret;
}

// Overloaded: Retrieve character inventory based on character id
bool Database::GetInventory(uint32 char_id, Inventory* inv) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
    MYSQL_RES* result;
    MYSQL_ROW row;
	bool ret = false;
	
	// Retrieve character inventory
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT slotid,itemid,charges FROM inventory WHERE charid=%i ORDER BY slotid", char_id), errbuf, &result)) {

		while ((row = mysql_fetch_row(result))) {	
			sint16 slot_id	= (sint16)atoi(row[0]);
			uint32 item_id	= (uint32)atoi(row[1]);
			sint8 charges	= (uint8)atoi(row[2]);
			const Item_Struct* item = GetItem(item_id);
			
			if (item) {
				sint16 put_slot_id = SLOT_INVALID;
				
				if (item->ItemClass == ItemTypeCommon) {
					ItemCommonInst common(item, charges);
					put_slot_id = inv->PutItem(slot_id, (ItemInst&)common);
				}
				else if (item->ItemClass == ItemTypeContainer) {
					ItemContainerInst bag(item, charges);
					put_slot_id = inv->PutItem(slot_id, (ItemInst&)bag);
				}
				else if (item->ItemClass == ItemTypeBook) {
					ItemBookInst book(item, charges);
					put_slot_id = inv->PutItem(slot_id, (ItemInst&)book);
				}
				
				// Save ptr to item in inventory
				if (put_slot_id == SLOT_INVALID) {
					LogFile->write(EQEMuLog::Error,
						"Warning: Invalid slot_id for item in inventory: charid=%i, item_id=%i, slot_id=%i",
						char_id, item_id, slot_id);
				}
			}
			else {
				LogFile->write(EQEMuLog::Error,
					"Warning: charid %i has an invalid item_id %i in inventory slot %i",
					char_id, item_id, slot_id);
			}
		}
		mysql_free_result(result);
		
		// Retrieve shared inventory
		ret = GetSharedBank(char_id, inv, true);
	}
	else
		LogFile->write(EQEMuLog::Error, "GetInventory query '%s' %s", query, errbuf);
	
	safe_delete_array(query);
	return ret;
}

// Overloaded: Retrieve character inventory based on account_id and character name
bool Database::GetInventory(uint32 account_id, char* name, Inventory* inv) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
    MYSQL_RES* result;
    MYSQL_ROW row;
	bool ret = false;
	
	// Retrieve character inventory
#ifdef WORLD
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT slotid,itemid,charges FROM inventory INNER JOIN character_ ch ON ch.id=charid WHERE ch.name='%s' AND ch.account_id=%i AND slotid<22 ORDER BY slotid", name, account_id), errbuf, &result)) {
#else
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT slotid,itemid,charges FROM inventory INNER JOIN character_ ch ON ch.id=charid WHERE ch.name='%s' AND ch.account_id=%i ORDER BY slotid", name, account_id), errbuf, &result)) {
#endif
		while ((row = mysql_fetch_row(result))) {
			sint16 slot_id	= (sint16)atoi(row[0]);
			uint32 item_id	= (uint32)atoi(row[1]);
			sint8 charges	= (sint8)atoi(row[2]);
			
			const Item_Struct* item = GetItem(item_id);
			sint16 put_slot_id = SLOT_INVALID;
			if(!item)
				continue;
			if (item->ItemClass == ItemTypeCommon) {
				ItemCommonInst common(item, charges);
				put_slot_id = inv->PutItem(slot_id, (ItemInst&)common);
			}
			else if (item->ItemClass == ItemTypeContainer) {
				ItemContainerInst bag(item, charges);
				put_slot_id = inv->PutItem(slot_id, (ItemInst&)bag);
			}
			else if (item->ItemClass == ItemTypeBook) {
				ItemBookInst book(item, charges);
				put_slot_id = inv->PutItem(slot_id, (ItemInst&)book);
			}
			
			// Save ptr to item in inventory
			if (put_slot_id == SLOT_INVALID) {
				LogFile->write(EQEMuLog::Error,
					"Warning: Invalid slot_id for item in inventory: acctid=%i, item_id=%i, slot_id=%i",
					account_id, item_id, slot_id);
			}
		}
		mysql_free_result(result);
		
		// Retrieve shared inventory
		ret = GetSharedBank(account_id, inv, false);
	}
	else
		LogFile->write(EQEMuLog::Error, "GetInventory query '%s' %s", query, errbuf);
	
	safe_delete_array(query);
	return ret;
}

bool Database::SetPlayerProfile(uint32 account_id, uint32 charid, PlayerProfile_Struct* pp, Inventory* inv, uint32 current_zone) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	int32 affected_rows = 0;
	bool ret = false;
    
	if (RunQuery(query, SetPlayerProfile_MQ(&query, account_id, charid, pp, inv, current_zone), errbuf, 0, &affected_rows)) {
		ret = (affected_rows != 0);
	}
	
	if (!ret) {
		LogFile->write(EQEMuLog::Error, "SetPlayerProfile query '%s' %s", query, errbuf);
    }
	
	safe_delete_array(query);
	return ret;
}

// Generate SQL for updating player profile/inventory
int32 Database::SetPlayerProfile_MQ(char** query, uint32 account_id, uint32 charid, PlayerProfile_Struct* pp, Inventory* inv, uint32 current_zone) {
    *query = new char[256 + sizeof(PlayerProfile_Struct)*2 + 1];
	char* end = *query;
	if (!current_zone)
		current_zone = pp->zone_id;

	if(strlen(pp->name) == 0) // Sanity check in case pp never loaded
		return false;
	
	end += sprintf(end, "UPDATE character_ SET timelaston=unix_timestamp(now()),name=\'%s\', zonename=\'%s\', zoneid=%u, x = %f, y = %f, z = %f, profile=\'", pp->name, GetZoneName(current_zone), current_zone, pp->x, pp->y, pp->z);
	end += DoEscapeString(end, (char*)pp, sizeof(PlayerProfile_Struct));
    end += sprintf(end,"\' WHERE id=%u", charid);
	
	// @merth: come back later to save entire inventory
	// Not vital at the moment, as each move action saves individual item to inventory in DB
	// Could be a problem if something calculates an inventory change, but doesn't call SaveInventory()
	// .. May not even have to do if inventory is saved each time it changes
	
	return (int32) (end - (*query));
}

/*
This function returns the account_id that owns the character with
the name "name" or zero if no character with that name was found
Zero will also be returned if there is a database error.
*/
int32 Database::GetAccountIDByChar(const char* charname, int32* oCharID) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT account_id, id FROM character_ WHERE name='%s'", charname), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			int32 tmp = atoi(row[0]); // copy to temp var because gotta free the result before exitting this function
			if (oCharID)
				*oCharID = atoi(row[1]);
			mysql_free_result(result);
			return tmp;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetAccountIDByChar query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	
	return 0;
}

// Retrieve account_id for a given char_id
uint32 Database::GetAccountIDByChar(uint32 char_id) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	uint32 ret = 0;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT account_id FROM character_ WHERE id=%i", char_id), errbuf, &result)) {
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			ret = atoi(row[0]); // copy to temp var because gotta free the result before exitting this function
		}
		mysql_free_result(result);
	}
	else {
		LogFile->write(EQEMuLog::Error, "Error in GetAccountIDByChar query '%s': %s", query, errbuf);
	}
	
	safe_delete_array(query);
	return ret;
}

int32 Database::GetAccountIDByName(const char* accname, sint16* status, int32* lsid) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;

	
	for (unsigned int i=0; i<strlen(accname); i++) {
		if ((accname[i] < 'a' || accname[i] > 'z') && 
			(accname[i] < 'A' || accname[i] > 'Z') && 
			(accname[i] < '0' || accname[i] > '9'))
			return 0;
	}
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, status, lsaccount_id FROM account WHERE name='%s'", accname), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 tmp = atoi(row[0]); // copy to temp var because gotta free the result before exitting this function
			if (status)
				*status = atoi(row[1]);
			if (lsid) {
				if (row[2])
					*lsid = atoi(row[2]);
				else
					*lsid = 0;
			}
			mysql_free_result(result);
			return tmp;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetAccountIDByAcc query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	
	return 0;
}

void Database::GetAccountName(int32 accountid, char* name, int32* oLSAccountID) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT name, lsaccount_id FROM account WHERE id='%i'", accountid), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);

			strcpy(name, row[0]);
			if (row[1] && oLSAccountID) {
				*oLSAccountID = atoi(row[1]);
			}
		}

		mysql_free_result(result);
	}
	else {
		safe_delete_array(query);
		cerr << "Error in GetAccountName query '" << query << "' " << errbuf << endl;
	}
}

int32 Database::GetCharacterInfo(const char* iName, int32* oAccID, int32* oZoneID, float* oX, float* oY, float* oZ) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, account_id, zonename, x, y, z FROM character_ WHERE name='%s'", iName), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 charid = atoi(row[0]);
			if (oAccID)
				*oAccID = atoi(row[1]);
			if (oZoneID)
				*oZoneID = GetZoneID(row[2]);
			if (oX)
				*oX = atof(row[3]);
			if (oY)
				*oY = atof(row[4]);
			if (oZ)
				*oZ = atof(row[5]);
			mysql_free_result(result);
			return charid;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetCharacterInfo query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	return 0;
}

bool Database::LoadVariables() {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
	
	if (RunQuery(query, LoadVariables_MQ(&query), errbuf, &result)) {
		safe_delete_array(query);
		bool ret = LoadVariables_result(result);
		mysql_free_result(result);
		return ret;
	}
	else {
		cerr << "Error in LoadVariables query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	return false;
}

int32 Database::LoadVariables_MQ(char** query) {
	LockMutex lock(&Mvarcache);
	return MakeAnyLenString(query, "SELECT varname, value, unix_timestamp() FROM variables where unix_timestamp(ts) >= %d", varcache_lastupdate);
}

bool Database::LoadVariables_result(MYSQL_RES* result) {
	int32 i;
    MYSQL_ROW row;
	LockMutex lock(&Mvarcache);
	if (mysql_num_rows(result) > 0) {
		if (!varcache_array) {
			varcache_max = mysql_num_rows(result);
			varcache_array = new VarCache_Struct*[varcache_max];
			for (i=0; i<varcache_max; i++)
				varcache_array[i] = 0;
		}
		else {
			int32 tmpnewmax = varcache_max + mysql_num_rows(result);
			VarCache_Struct** tmp = new VarCache_Struct*[tmpnewmax];
			for (i=0; i<tmpnewmax; i++)
				tmp[i] = 0;
			for (i=0; i<varcache_max; i++)
				tmp[i] = varcache_array[i];
			VarCache_Struct** tmpdel = varcache_array;
			varcache_array = tmp;
			varcache_max = tmpnewmax;
			delete tmpdel;
		}
		while ((row = mysql_fetch_row(result))) {
			varcache_lastupdate = atoi(row[2]);
			for (i=0; i<varcache_max; i++) {
				if (varcache_array[i]) {
					if (strcasecmp(varcache_array[i]->varname, row[0]) == 0) {
						delete varcache_array[i];
						varcache_array[i] = (VarCache_Struct*) new int8[sizeof(VarCache_Struct) + strlen(row[1]) + 1];
						strn0cpy(varcache_array[i]->varname, row[0], sizeof(varcache_array[i]->varname));
						strcpy(varcache_array[i]->value, row[1]);
						break;
					}
				}
				else {
					varcache_array[i] = (VarCache_Struct*) new int8[sizeof(VarCache_Struct) + strlen(row[1]) + 1];
					strcpy(varcache_array[i]->varname, row[0]);
					strcpy(varcache_array[i]->value, row[1]);
					break;
				}
			}
		}
		int32 max_used = 0;
		for (i=0; i<varcache_max; i++) {
			if (varcache_array[i]) {
				if (i > max_used)
					max_used = i;
			}
		}
		max_used++;
		varcache_max = max_used;
	}
	return true;
}

// Gets variable from 'variables' table

bool Database::GetVariable(const char* varname, char* varvalue, int16 varvalue_len) {
	LockMutex lock(&Mvarcache);
	if (strlen(varname) <= 1)
		return false;
	for (int32 i=0; i<varcache_max; i++) {

		if (varcache_array[i]) {
			if (strcasecmp(varcache_array[i]->varname, varname) == 0) {
				snprintf(varvalue, varvalue_len, "%s", varcache_array[i]->value);
				varvalue[varvalue_len-1] = 0;
				return true;
			}
		}
		else
			return false;
	}
	return false;
/*
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT value FROM variables WHERE varname like '%s'", varname), errbuf, &result))
	{
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			snprintf(varvalue, varvalue_len, "%s", row[0]);
			varvalue[varvalue_len-1] = 0;
			mysql_free_result(result);
			return true;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetVariable query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	return false;
*/
}

bool Database::SetVariable(const char* varname_in, const char* varvalue_in) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	char *varname,*varvalue;
	
	varname=(char *)malloc(strlen(varname_in)*2+1);
	varvalue=(char *)malloc(strlen(varvalue_in)*2+1);
	DoEscapeString(varname, varname_in, strlen(varname_in));
	DoEscapeString(varvalue, varvalue_in, strlen(varvalue_in));
	
	if (RunQuery(query, MakeAnyLenString(&query, "Update variables set value='%s' WHERE varname like '%s'", varvalue, varname), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		if (affected_rows == 1) {
			LoadVariables(); // refresh cache
			free(varname);
			free(varvalue);
			return true;
		}
		else {
			if (RunQuery(query, MakeAnyLenString(&query, "Insert Into variables (varname, value) values ('%s', '%s')", varname, varvalue), errbuf, 0, &affected_rows)) {
				safe_delete_array(query);
				if (affected_rows == 1) {
					LoadVariables(); // refresh cache
					free(varname);
					free(varvalue);
					return true;
				}
			}
		}
	}
	else {
		cerr << "Error in SetVariable query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	free(varname);
	free(varvalue);
	return false;
}

bool Database::CheckZoneserverAuth(const char* ipaddr) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT * FROM zoneserver_auth WHERE '%s' like host", ipaddr), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) >= 1) {
			mysql_free_result(result);
			return true;
		}
		else {
			mysql_free_result(result);
			return false;
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in CheckZoneserverAuth query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	return false;
}

bool Database::GetGuildRanks(int32 guildeqid, GuildRanks_Struct* gr) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, eqid, name, leader, minstatus, rank0title, rank1, rank1title, rank2, rank2title, rank3, rank3title, rank4, rank4title, rank5, rank5title from guilds where eqid=%i;", guildeqid), errbuf, &result))
	{
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			gr->leader = atoi(row[3]);
			gr->databaseID = atoi(row[0]);
			gr->minstatus = atoi(row[4]);
			strcpy(gr->name, row[2]);
			for (int i = 0; i <= GUILD_MAX_RANK; i++) {
				strcpy(gr->rank[i].rankname, row[5 + (i*2)]);
				if (i == 0) {
					gr->rank[i].heargu = 1;
					gr->rank[i].speakgu = 1;
					gr->rank[i].invite = 1;
					gr->rank[i].remove = 1;
					gr->rank[i].promote = 1;
					gr->rank[i].demote = 1;
					gr->rank[i].motd = 1;
					gr->rank[i].warpeace = 1;
				}
				else if (strlen(row[4 + (i*2)]) >= 8) {
					gr->rank[i].heargu = (row[4 + (i*2)][GUILD_HEAR] == '1');
					gr->rank[i].speakgu = (row[4 + (i*2)][GUILD_SPEAK] == '1');
					gr->rank[i].invite = (row[4 + (i*2)][GUILD_INVITE] == '1');
					gr->rank[i].remove = (row[4 + (i*2)][GUILD_REMOVE] == '1');
					gr->rank[i].promote = (row[4 + (i*2)][GUILD_PROMOTE] == '1');
					gr->rank[i].demote = (row[4 + (i*2)][GUILD_DEMOTE] == '1');
					gr->rank[i].motd = (row[4 + (i*2)][GUILD_MOTD] == '1');
					gr->rank[i].warpeace = (row[4 + (i*2)][GUILD_WARPEACE] == '1');
				}
				else {
					gr->rank[i].heargu = 1;
					gr->rank[i].speakgu = 1;
					gr->rank[i].invite = 0;
					gr->rank[i].remove = 0;
					gr->rank[i].promote = 0;
					gr->rank[i].demote = 0;
					gr->rank[i].motd = 0;
					gr->rank[i].warpeace = 0;
				}
				
				if (gr->rank[i].rankname[0] == 0)
					snprintf(gr->rank[i].rankname, 100, "Guild Rank %i", i);
			}
		}
		else {
			gr->leader = 0;
			gr->databaseID = 0;
			gr->minstatus = 0;
			memset(gr->name, 0, sizeof(gr->name));
			for (int i = 0; i <= GUILD_MAX_RANK; i++) {
				snprintf(gr->rank[i].rankname, 100, "Guild Rank %i", i);
				if (i == 0) {
					gr->rank[i].heargu = 1;
					gr->rank[i].speakgu = 1;
					gr->rank[i].invite = 1;
					gr->rank[i].remove = 1;
					gr->rank[i].promote = 1;
					gr->rank[i].demote = 1;
					gr->rank[i].motd = 1;
					gr->rank[i].warpeace = 1;
				}
				else {
					gr->rank[i].heargu = 0;
					gr->rank[i].speakgu = 0;
					gr->rank[i].invite = 0;
					gr->rank[i].remove = 0;
					gr->rank[i].promote = 0;
					gr->rank[i].demote = 0;
					gr->rank[i].motd = 0;

					gr->rank[i].warpeace = 0;
				}
			}
		}
		mysql_free_result(result);
		return true;
	}
	else {
		cerr << "Error in GetGuildRank query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return false;
}

bool Database::LoadGuilds(GuildRanks_Struct* guilds) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	//	int i;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	for (int a = 0; a < 512; a++) {
		guilds[a].leader = 0;
		guilds[a].databaseID = 0;
		memset(guilds[a].name, 0, sizeof(guilds[a].name));
		for (int i = 0; i <= GUILD_MAX_RANK; i++) {
			snprintf(guilds[a].rank[i].rankname, 100, "Guild Rank %i", i);
			if (i == 0) {
				guilds[a].rank[i].heargu = 1;
				guilds[a].rank[i].speakgu = 1;
				guilds[a].rank[i].invite = 1;
				guilds[a].rank[i].remove = 1;
				guilds[a].rank[i].promote = 1;
				guilds[a].rank[i].demote = 1;
				guilds[a].rank[i].motd = 1;
				guilds[a].rank[i].warpeace = 1;
			}
			else {
				guilds[a].rank[i].heargu = 0;
				guilds[a].rank[i].speakgu = 0;
				guilds[a].rank[i].invite = 0;
				guilds[a].rank[i].remove = 0;
				guilds[a].rank[i].promote = 0;
				guilds[a].rank[i].demote = 0;
				guilds[a].rank[i].motd = 0;
				guilds[a].rank[i].warpeace = 0;
			}
		}
		Sleep(0);
	}

	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, eqid, name, leader, minstatus, rank0title, rank1, rank1title, rank2, rank2title, rank3, rank3title, rank4, rank4title, rank5, rank5title from guilds"), errbuf, &result)) {

		safe_delete_array(query);
		int32 guildeqid = 0xFFFFFFFF;
		while ((row = mysql_fetch_row(result))) {
			guildeqid = atoi(row[1]);
			if (guildeqid < 512) {
				guilds[guildeqid].leader = atoi(row[3]);
				guilds[guildeqid].databaseID = atoi(row[0]);
				guilds[guildeqid].minstatus = atoi(row[4]);
				strcpy(guilds[guildeqid].name, row[2]);
				for (int i = 0; i <= GUILD_MAX_RANK; i++) {
					strcpy(guilds[guildeqid].rank[i].rankname, row[5 + (i*2)]);
					if (i == 0) {
						guilds[guildeqid].rank[i].heargu = 1;
						guilds[guildeqid].rank[i].speakgu = 1;
						guilds[guildeqid].rank[i].invite = 1;
						guilds[guildeqid].rank[i].remove = 1;
						guilds[guildeqid].rank[i].promote = 1;
						guilds[guildeqid].rank[i].demote = 1;
						guilds[guildeqid].rank[i].motd = 1;
						guilds[guildeqid].rank[i].warpeace = 1;
					}
					else if (strlen(row[4 + (i*2)]) >= 8) {
						guilds[guildeqid].rank[i].heargu = (row[4 + (i*2)][GUILD_HEAR] == '1');
						guilds[guildeqid].rank[i].speakgu = (row[4 + (i*2)][GUILD_SPEAK] == '1');
						guilds[guildeqid].rank[i].invite = (row[4 + (i*2)][GUILD_INVITE] == '1');
						guilds[guildeqid].rank[i].remove = (row[4 + (i*2)][GUILD_REMOVE] == '1');
						guilds[guildeqid].rank[i].promote = (row[4 + (i*2)][GUILD_PROMOTE] == '1');
						guilds[guildeqid].rank[i].demote = (row[4 + (i*2)][GUILD_DEMOTE] == '1');
						guilds[guildeqid].rank[i].motd = (row[4 + (i*2)][GUILD_MOTD] == '1');
						guilds[guildeqid].rank[i].warpeace = (row[4 + (i*2)][GUILD_WARPEACE] == '1');
					}
					else {

						guilds[guildeqid].rank[i].heargu = 1;
						guilds[guildeqid].rank[i].speakgu = 1;
						guilds[guildeqid].rank[i].invite = 0;

						guilds[guildeqid].rank[i].remove = 0;
						guilds[guildeqid].rank[i].promote = 0;
						guilds[guildeqid].rank[i].demote = 0;
						guilds[guildeqid].rank[i].motd = 0;
						guilds[guildeqid].rank[i].warpeace = 0;
					}
					
					if (guilds[guildeqid].rank[i].rankname[0] == 0)
						snprintf(guilds[guildeqid].rank[i].rankname, 100, "Guild Rank %i", i);
				}
			}
			Sleep(0);
		}
		mysql_free_result(result);
		return true;
	}
	else
	{
		cerr << "Error in LoadGuilds query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return false;
}

// Pyro: Get zone starting points from DB
bool Database::GetSafePoints(const char* short_name, float* safe_x, float* safe_y, float* safe_z, sint16* minstatus, int8* minlevel) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	//	int buf_len = 256;
	//    int chars = -1;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT safe_x, safe_y, safe_z, minium_status, minium_level FROM zone WHERE short_name='%s'", short_name), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if (safe_x != 0)
				*safe_x = atof(row[0]);
			if (safe_y != 0)
				*safe_y = atof(row[1]);
			if (safe_z != 0)
				*safe_z = atof(row[2]);
			if (minstatus != 0)
				*minstatus = atoi(row[3]);
			if (minlevel != 0)
				*minlevel = atoi(row[4]);
			mysql_free_result(result);
			return true;
		}

		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in GetSafePoint query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	return false;
}
void Database::SetPublicNote(int32 guildid,char* charname, char* note){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	char* notebuf = new char[(strlen(note)*2)+3];
	DoEscapeString(notebuf, note, strlen(note)) ;
	if (!RunQuery(query, MakeAnyLenString(&query, "update character_ set publicnote='%s' where name='%s' and guild=%i", notebuf,charname,guildid), errbuf)) {
		cerr << "Error running SetPublicNote query: " << errbuf << endl;
	}
	safe_delete_array(query);
	safe_delete_array(notebuf);
}
bool Database::NoRentExpired(const char* name){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "Select (UNIX_TIMESTAMP(NOW())-timelaston) from character_ where name='%s'", name), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 seconds = atoi(row[0]);
			mysql_free_result(result);
			return (seconds>1800);
		}
	}
	return false;
}
int32 Database::GetGuildDBID(int32 eqid){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "Select id from guilds where eqid=%i", eqid), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 ret = atoi(row[0]);
			mysql_free_result(result);
			return ret;
		}
	}
	return 0;
}
void Database::GetGuildMembers(int32 guildid,GuildMember_Struct* gms){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int32 count=0;
	int32 length=0;
	if (RunQuery(query, MakeAnyLenString(&query, "Select name,profile,timelaston,guildrank,publicnote from character_ where guild=%i", guildid), errbuf, &result)) {
		safe_delete_array(query);
		while( ( row = mysql_fetch_row(result) ) ){
			strcpy(gms->member[count].name,row[0]);
			length+=strlen(row[0])+strlen(row[4]);
			PlayerProfile_Struct* pps=(PlayerProfile_Struct*)row[1];
			gms->member[count].level=htonl(pps->level);
			gms->member[count].zoneid=pps->zone_id;
			gms->member[count].timelaston=htonl(atol(row[2]));
			gms->member[count].class_=htonl(pps->class_);
			gms->member[count].rank=atoi(row[3]);
			strcpy(gms->member[count].publicnote,row[4]);
			count++;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetGuildMembers query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	gms->count=count;
	gms->length=length;
}
int32 Database::NumberInGuild(int32 guilddbid) {
    	char errbuf[MYSQL_ERRMSG_SIZE];
    	char *query = 0;
		MYSQL_RES *result;
		MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "Select count(id) from character_ where guild=%i", guilddbid), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 ret = atoi(row[0]);
			mysql_free_result(result);
			return ret;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in NumberInGuild query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return 0;
	}
	return 0;
}
bool Database::SetGuild(char* name, int32 guilddbid, int8 guildrank) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	if (RunQuery(query, MakeAnyLenString(&query, "UPDATE character_ SET guild=%i, guildrank=%i WHERE name='%s'", guilddbid, guildrank, name), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else {
		cerr << "Error in SetGuild query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	return false;
}
bool Database::SetGuild(int32 charid, int32 guilddbid, int8 guildrank) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	if (RunQuery(query, MakeAnyLenString(&query, "UPDATE character_ SET guild=%i, guildrank=%i WHERE id=%i", guilddbid, guildrank, charid), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		if (affected_rows == 1)
			return true;

		else
			return false;
	}
	else {
		cerr << "Error in SetGuild query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return false;
}

int32 Database::GetFreeGuildEQID()
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[100];
    MYSQL_RES *result;
	
	for (int x = 1; x < 512; x++) {
		snprintf(query, 100, "SELECT eqid FROM guilds where eqid=%i;", x);
		
		if (RunQuery(query, strlen(query), errbuf, &result)) {
			if (mysql_num_rows(result) == 0) {
				mysql_free_result(result);
				return x;
			}
			mysql_free_result(result);
		}
		else {
			cerr << "Error in GetFreeGuildEQID query '" << query << "' " << errbuf << endl;
		}
	}
	
	return 0xFFFFFFFF;
}

int32 Database::CreateGuild(const char* name, int32 leader) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	char buf[65];
	int32 affected_rows = 0;
	DoEscapeString(buf, name, strlen(name)) ;
	
	int32 tmpeqid = GetFreeGuildEQID();
	if (tmpeqid == 0xFFFFFFFF) {
		cout << "Error in Database::CreateGuild: unable to find free eqid" << endl;
		return 0xFFFFFFFF;
	}
	
	if (RunQuery(query, MakeAnyLenString(&query, "INSERT INTO guilds (name, leader, eqid) Values ('%s', %i, %i)", buf, leader, tmpeqid), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		if (tmpeqid > 0) {
			return tmpeqid;
		}
		else {
			return 0xFFFFFFFF;
		}
	}
	else {
		cerr << "Error in CreateGuild query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return 0xFFFFFFFF;
	}
	
	return 0xFFFFFFFF;
}

bool Database::DeleteGuild(int32 guilddbid)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	char *query2 = 0;
	int32 affected_rows = 0;
	
	if (RunQuery(query, MakeAnyLenString(&query, "DELETE FROM guilds WHERE id=%i;", guilddbid), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		if (affected_rows == 1){
			if(!RunQuery(query2, MakeAnyLenString(&query2, "update character_ set guild=0,guildrank=0 where guild=%i", guilddbid), errbuf, 0, &affected_rows))
				cerr << "Error in DeleteGuild query '" << query2 << "': " << errbuf << endl;
			safe_delete_array(query2);
			return true;
		}
		else
			return false;
	}
	else {
		cerr << "Error in DeleteGuild query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return false;
}

bool Database::RenameGuild(int32 guilddbid, const char* name) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	char buf[65];
	DoEscapeString(buf, name, strlen(name)) ;
	
	if (RunQuery(query, MakeAnyLenString(&query, "Update guilds set name='%s' WHERE id=%i;", buf, guilddbid), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else {
		cerr << "Error in RenameGuild query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return false;
}



bool Database::EditGuild(int32 guilddbid, int8 ranknum, GuildRankLevel_Struct* grl)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    int chars = 0;
	int32 affected_rows = 0;
	char buf[203];
	char buf2[8];
	DoEscapeString(buf, grl->rankname, strlen(grl->rankname)) ;
	buf2[GUILD_HEAR] = grl->heargu + '0';
	buf2[GUILD_SPEAK] = grl->speakgu + '0';
	buf2[GUILD_INVITE] = grl->invite + '0';
	buf2[GUILD_REMOVE] = grl->remove + '0';
	buf2[GUILD_PROMOTE] = grl->promote + '0';
	buf2[GUILD_DEMOTE] = grl->demote + '0';
	buf2[GUILD_MOTD] = grl->motd + '0';
	buf2[GUILD_WARPEACE] = grl->warpeace + '0';
	
	if (ranknum == 0)
		chars = MakeAnyLenString(&query, "Update guilds set rank%ititle='%s' WHERE id=%i;", ranknum, buf, guilddbid);
	else
		chars = MakeAnyLenString(&query, "Update guilds set rank%ititle='%s', rank%i='%s' WHERE id=%i;", ranknum, buf, ranknum, buf2, guilddbid);
	
	if (RunQuery(query, chars, errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else {
		cerr << "Error in EditGuild query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return false;
}

bool Database::GetGuildNameByID(int32 guilddbid, char * name) {
	if (!name || !guilddbid) return false;
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	MYSQL_RES *result;
    MYSQL_ROW row;	
	
	if (RunQuery(query, MakeAnyLenString(&query, "select * from guilds where id='%i'", guilddbid), errbuf, &result)) {
		safe_delete_array(query);
		row = mysql_fetch_row(result);
		if (row[2]) sprintf(name,"%s",row[2]);
		mysql_free_result(result);
		return true;
	}
	else {
		cerr << "Error in RenameGuild query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return false;
}


bool Database::GetZoneLongName(const char* short_name, char** long_name, char* file_name, float* safe_x, float* safe_y, float* safe_z, int32* maxclients) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT long_name, file_name, safe_x, safe_y, safe_z, maxclients FROM zone WHERE short_name='%s'", short_name), errbuf, &result))
	{
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if (long_name != 0) {
				*long_name = strcpy(new char[strlen(row[0])+1], row[0]);
			}
			if (file_name != 0) {
				if (row[1] == 0)
					strcpy(file_name, short_name);
				else
					strcpy(file_name, row[1]);
			}
			if (safe_x != 0)
				*safe_x = atof(row[2]);
			if (safe_y != 0)
				*safe_y = atof(row[3]);
			if (safe_z != 0)
				*safe_z = atof(row[4]);
			if (maxclients)
				*maxclients = atoi(row[5]);
			mysql_free_result(result);
			return true;
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in GetZoneLongName query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return false;
}

int32 Database::GetGuildDBIDbyLeader(int32 leader)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id FROM guilds WHERE leader=%i", leader), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			int32 tmp = atoi(row[0]);
			mysql_free_result(result);
			return tmp;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetGuildDBIDbyLeader query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	
	return 0;
}

bool Database::SetGuildLeader(int32 guilddbid, int32 leader)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	if (RunQuery(query, MakeAnyLenString(&query, "UPDATE guilds SET leader=%i WHERE id=%i", leader, guilddbid), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else {
		cerr << "Error in SetGuildLeader query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return false;
}

/*#ifndef SHAREMEM
bool Database::UpdateItem(uint32 item_id, const Item_Struct* is)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[256+sizeof(Item_Struct)*2+1];
	char* end = query;
	
	end += sprintf(end, "UPDATE items SET raw_data=");
	*end++ = '\'';
    end += DoEscapeString(end, (char*)is, sizeof(Item_Struct));
    *end++ = '\'';
    end += sprintf(end," WHERE id=%i", item_id);
	
	item_array[item_id] = is;
	
	int32 affected_rows = 0;
    if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows)) {
        cerr << "Error in UpdateItem query " << errbuf << endl;
		return false;
    }
	
	if (affected_rows == 0) {
		return false;
	}
	
	return true;
}
#endif*/

bool Database::SetGuildMOTD(int32 guilddbid, const char* motd) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	char* motdbuf = 0;
	int32 affected_rows = 0;
	
	motdbuf = new char[(strlen(motd)*2)+3];

	DoEscapeString(motdbuf, motd, strlen(motd)) ;
	
	if (RunQuery(query, MakeAnyLenString(&query, "Update guilds set motd='%s' WHERE id=%i;", motdbuf, guilddbid), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		delete motdbuf;
		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else
	{
		cerr << "Error in SetGuildMOTD query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		delete motdbuf;
		return false;
	}
	
	return false;
}

char* Database::GetGuildMOTD(int32 guilddbid)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	char* motd = new char[599];
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT motd FROM guilds WHERE id=%i", guilddbid), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if (row[0] == 0)
				strcpy(motd, "");
			else
				strcpy(motd, row[0]);
			mysql_free_result(result);
			return motd;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetGuildMOTD query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	return motd;
}

sint32 Database::GetItemsCount(int32* oMaxID) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
	sint32 ret = -1;
	
	char query[] = "SELECT MAX(id),count(*) FROM items";
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		row = mysql_fetch_row(result);
		if (row != NULL && row[1] != 0) {
			ret = atoi(row[1]);
			if (oMaxID) {
				if (row[0])
					*oMaxID = atoi(row[0]);
				else
					*oMaxID = 0;
			}
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetItemsCount query '" << query << "' " << errbuf << endl;
	}
	
	return ret;
}

sint32 Database::GetNPCTypesCount(int32* oMaxID) {

	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(id), count(*) FROM npc_types");
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete_array(query);
		row = mysql_fetch_row(result);
		if (row != NULL && row[1] != 0) {
			sint32 ret = atoi(row[1]);
			if (oMaxID) {
				if (row[0])
					*oMaxID = atoi(row[0]);
				else
					*oMaxID = 0;
			}
			mysql_free_result(result);
			return ret;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetNPCTypesCount query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);

		return -1;
	}
	
	return -1;
}

sint32 Database::GetDoorsCount(int32* oMaxID) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;

    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(id), count(*) FROM doors");
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete_array(query);
		row = mysql_fetch_row(result);
		if (row != NULL && row[1] != 0) {
			sint32 ret = atoi(row[1]);
			if (oMaxID) {
				if (row[0])
					*oMaxID = atoi(row[0]);
				else
					*oMaxID = 0;
			}
			mysql_free_result(result);
			return ret;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetDoorsCount query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return -1;
	}
	
	return -1;
}

void Database::LoadItemStatus() {
	memset(item_minstatus, 0, sizeof(item_minstatus));
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int32 tmp;
	if (RunQuery(query, MakeAnyLenString(&query, "Select id, minstatus from items where minstatus > 0"), errbuf, &result)) {
		safe_delete_array(query);
		while ((row = mysql_fetch_row(result)) && row[0] && row[1]) {
			tmp = atoi(row[0]);
			if (tmp < MAX_ITEM_ID)
				item_minstatus[tmp] = atoi(row[1]);
		}
		mysql_free_result(result);
	}
	else {
		cout << "Error in LoadItemStatus query: '" << query << "'" << endl;
		safe_delete_array(query);
	}
}

bool Database::DBSetItemStatus(int32 id, int8 status) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int32 affected_rows = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "Update items set minstatus=%u where id=%u", status, id), errbuf, 0, &affected_rows)) {
		cout << "Error in LoadItemStatus query: '" << query << "'" << endl;
	}
	safe_delete_array(query);
	return (bool) (affected_rows == 1);
}

#ifdef SHAREMEM
extern "C" bool extDBLoadItems(sint32 iItemCount, int32 iMaxItemID) { return database.DBLoadItems(iItemCount, iMaxItemID); }
bool Database::LoadItems() {
	if (!EMuShareMemDLL.Load())
		return false;
	sint32 tmp = 0;
	tmp = GetItemsCount(&max_item);
	if (tmp == -1) {
		cout << "Error: Database::LoadItems() (sharemem): GetItemsCount() returned -1" << endl;
		return false;
	}
	bool ret = EMuShareMemDLL.Items.DLLLoadItems(&extDBLoadItems, sizeof(Item_Struct), &tmp, &max_item);
#if defined(GOTFRAGS) || defined(_EQDEBUG)
	if (ret)
		LoadItemStatus();
#endif
	return ret;
}

// Load all database items into cache
bool Database::DBLoadItems(sint32 iItemCount, uint32 iMaxItemID) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	MYSQL_RES* result;
	MYSQL_ROW row;
	bool ret = false;
	
	LogFile->write(EQEMuLog::Status, "Loading items from database: count=%i, max id=%i", iItemCount, iMaxItemID);
	
	// Make sure enough memory was alloc'd in cache
	sint32 item_count = GetItemsCount(&max_item);
	if (item_count != iItemCount) {
		LogFile->write(EQEMuLog::Error, "Insufficient shared memory to load items (actual=%i, allocated=%i)", item_count, iItemCount);
		return ret;
	}
	else if (max_item != iMaxItemID) {
		LogFile->write(EQEMuLog::Error, "Insufficient shared memory to load items (max item=%i, allocated=%i).  Increase MMF_EQMAX_ITEMS define", max_item, iMaxItemID);
		return ret;
	}
	
	#ifdef FIELD_ITEMS
		// Retrieve all items from database
		char query[] =
			"SELECT charges,unknown002,unknown003,merchantprice,unknown005,"
			"unknown006,unknown007,unknown008,itemclass,name,lore,idfile,id,weight,norent,"
			"nodrop,size,slots,cost,icon,unknown018,unknown019,unknown020,"
			"tradeskills,cr,dr,pr,mr,fr,astr,asta,aagi,adex,acha,aint,awis,hp,"
			"mana,ac,deity,skillmodvalue,skillmodtype,banedmgrace,banedmgamt,"
			"banedmgbody,magic,casttime2,hasteproclvl,reqlevel,bardtype,bardvalue,"
			"light,delay,reclevel,recskill,elemdmgtype,elemdmgamt,effecttype,"
			"range,damage,color,classes,races,unknown061,spellid,maxcharges,"
			"itemtype,material,sellrate,unknown067,casttime,unknown069,proc_rate_mod,"
			"focusid,combateffects,shielding,stunresist,strikethrough,combatskill,"
			"combatskilldmg,spellshield,avoidance,accuracy,unknown081,factionmod1,"
			"factionmod2,factionmod3,factionmod4,factionamt1,factionamt2,factionamt3,"
			"factionamt4,charmfile,augtype,augslot1type,augslot2type,augslot3type,"
			"augslot4type,augslot5type,ldonpointtheme,ldonpointcost,ldonsold,bagtype,"
			"bagslots,bagsize,bagwr,unknown105,booktype,filename,banedmgamt2,"
			"augmentrestriction,loreflag,pendingloreflag,artifactflag,summonedflag,"
			"tribute,gmflag,endur,dotshielding,attackbonus,hpregen,manaregen,"
			"hastepercent,damageshield,minstatus "
			"FROM items ORDER BY id";
		
		if (RunQuery(query, sizeof(query), errbuf, &result)) {
			while((row = mysql_fetch_row(result))) {
#if EQDEBUG >= 5
					LogFile->write(EQEMuLog::Status, "Loading %s:%i:%i", row[15], atoi(row[5]),atoi(row[72]));
#endif				
				Item_Struct item;
				memset(&item, 0, sizeof(Item_Struct));
				uint32 idx = 0;
				
				// Base Item_Struct members 
				item.Charges					= (uint8)atoi(row[idx++]);
				item.Unknown002					= (uint32)atoi(row[idx++]);
				item.CurrentEquipSlot			= (sint16)atoi(row[idx++]);
				item.MerchantPrice				= (uint32)atoi(row[idx++]);
				item.Unknown005					= (uint32)atoi(row[idx++]);
				item.Unknown006					= (uint32)atoi(row[idx++]);
				item.Unknown007					= (uint32)atoi(row[idx++]);
				item.Unknown008					= (uint32)atoi(row[idx++]);
				item.ItemClass					= (uint8)atoi(row[idx++]);
				strcpy(item.Name, row[idx++]);
				strcpy(item.LoreName,row[idx++]);
				strcpy(item.IDFile,row[idx++]);
				item.ItemNumber					= (uint32)atoi(row[idx++]);
				item.Weight						= (uint8)atoi(row[idx++]);
				item.NoRent						= (uint8)atoi(row[idx++]);
				item.NoDrop						= (uint8)atoi(row[idx++]);
				item.Size						= (int8)atoi(row[idx++]);
				item.EquipSlots					= (uint32)atoi(row[idx++]);
				item.Cost						= atoi(row[idx++]);
				item.IconNumber					= atoi(row[idx++]);
				if (item.ItemClass == ItemTypeBook) { // Books
					idx = 106;
					item.Book.Unknown105			= (uint32)atoi(row[idx++]);
					item.Book.BookType				= (uint8)atoi(row[idx++]);
					strcpy(item.Book.File, row[idx++]);
				}
				else if (item.ItemClass == ItemTypeContainer) { // Containers
					idx = 101;
					item.Container.PackType			= (int8)atoi(row[idx++]);
					item.Container.Slots			= (int8)atoi(row[idx++]);
					item.Container.SizeCapacity		= (int8)atoi(row[idx++]);
					item.Container.WeightReduction	= (int8)atoi(row[idx++]);
				}
				else if (item.ItemClass == ItemTypeCommon) { // Common
					item.Common.Unknown018			= (uint32)atoi(row[idx++]);
					item.Common.Unknown019			= (uint32)atoi(row[idx++]);
					item.Common.Unknown020			= (uint32)atoi(row[idx++]);
					item.Common.Tradeskills			= (atoi(row[idx++])==0) ? false : true;
					item.Common.SvCold				= (sint8)atoi(row[idx++]);
					item.Common.SvDisease			= (sint8)atoi(row[idx++]);
					item.Common.SvPoison			= (sint8)atoi(row[idx++]);
					item.Common.SvMagic				= (sint8)atoi(row[idx++]);
					item.Common.SvFire				= (sint8)atoi(row[idx++]);
					item.Common.STR					= (sint8)atoi(row[idx++]);
					item.Common.STA					= (sint8)atoi(row[idx++]);
					item.Common.AGI					= (sint8)atoi(row[idx++]);
					item.Common.DEX					= (sint8)atoi(row[idx++]);
					item.Common.CHA					= (sint8)atoi(row[idx++]);
					item.Common.INT					= (sint8)atoi(row[idx++]);
					item.Common.WIS					= (sint8)atoi(row[idx++]);
					item.Common.HP					= atoi(row[idx++]);
					item.Common.Mana				= atoi(row[idx++]);
					item.Common.AC					= atoi(row[idx++]);
					item.Common.Deity				= (uint32)atoi(row[idx++]);
					item.Common.SkillModValue		= atoi(row[idx++]);
					item.Common.SkillModType		= (uint32)atoi(row[idx++]);
					item.Common.BaneDmgRace			= (uint32)atoi(row[idx++]);
					item.Common.BaneDmg				= (sint8)atoi(row[idx++]);
					item.Common.BaneDmgBody			= (sint8)atoi(row[idx++]);
					item.Common.Magic				= (atoi(row[idx++])==0) ? false : true;
					item.Common.casttime2			= (uint32)atoi(row[idx++]);
					item.Common.ProcLevel			= (uint8)atoi(row[idx++]);
					item.Common.RequiredLevel		= atoi(row[idx++]);
					item.Common.BardSkillType		= (uint32)atoi(row[idx++]);
					item.Common.BardSkillAmt		= atoi(row[idx++]);
					item.Common.Light				= (uint8)atoi(row[idx++]);
					item.Common.Delay				= (uint8)atoi(row[idx++]);
					item.Common.RecommendedLevel	= (uint8)atoi(row[idx++]);
					item.Common.RecommendedSkill	= (uint8)atoi(row[idx++]);
					item.Common.ElemDmgType			= (uint8)atoi(row[idx++]);
					item.Common.ElemDmg				= (uint8)atoi(row[idx++]);
					item.Common.EffectType			= (uint8)atoi(row[idx++]);
					item.Common.Range				= (uint8)atoi(row[idx++]);
					item.Common.Damage				= (uint8)atoi(row[idx++]);
					item.Common.Color				= (uint32)atoi(row[idx++]);
					item.Common.Classes				= (uint32)atoi(row[idx++]);
					item.Common.Races				= (uint32)atoi(row[idx++]);
					item.Common.Unknown061			= (uint32)atoi(row[idx++]);
					item.Common.SpellId				= (sint16)atoi(row[idx++]);
					item.Common.MaxCharges			= (sint8)atoi(row[idx++]);
					item.Common.Skill				= (uint8)atoi(row[idx++]);
					item.Common.Material			= (uint8)atoi(row[idx++]);
					item.Common.SellRate			= (float)atof(row[idx++]);
					item.Common.Unknown067			= (uint32)atoi(row[idx++]);
					item.Common.CastTime			= (uint32)atoi(row[idx++]);
					item.Common.Unknown069			= (uint32)atoi(row[idx++]);
					item.Common.ProcRateMod			= (uint32)atoi(row[idx++]);
					item.Common.FocusId				= atoi(row[idx++]);
					item.Common.CombatEffects		= (sint8)atoi(row[idx++]);
					item.Common.Shielding			= (sint8)atoi(row[idx++]);
					item.Common.StunResist			= (sint8)atoi(row[idx++]);
					item.Common.StrikeThrough		= (sint8)atoi(row[idx++]);
					item.Common.CombatSkill			= (uint32)atoi(row[idx++]);
					item.Common.CombatSkillDmg		= (uint32)atoi(row[idx++]);
					item.Common.SpellShield			= (sint8)atoi(row[idx++]);
					item.Common.Avoidance			= (sint8)atoi(row[idx++]);
					item.Common.Accuracy			= (sint8)atoi(row[idx++]);
					item.Common.Unknown081			= (uint32)atoi(row[idx++]);
					item.Common.FactionMod1			= atoi(row[idx++]);
					item.Common.FactionMod2			= atoi(row[idx++]);
					item.Common.FactionMod3			= atoi(row[idx++]);
					item.Common.FactionMod4			= atoi(row[idx++]);
					item.Common.FactionAmt1			= atoi(row[idx++]);
					item.Common.FactionAmt2			= atoi(row[idx++]);
					item.Common.FactionAmt3			= atoi(row[idx++]);
					item.Common.FactionAmt4			= atoi(row[idx++]);
					strcpy(item.Common.CharmFile, row[idx++]);
					item.Common.augtype				= (int8)atoi(row[idx++]);
					item.Common.AugSlot1Type		= (int8)atoi(row[idx++]);
					item.Common.AugSlot2Type		= (int8)atoi(row[idx++]);
					item.Common.AugSlot3Type		= (int8)atoi(row[idx++]);
					item.Common.AugSlot4Type		= (int8)atoi(row[idx++]);
					item.Common.AugSlot5Type		= (int8)atoi(row[idx++]);
					item.Common.ldonpointtheme		= (uint32)atoi(row[idx++]);
					item.Common.ldonpointcost		= (uint32)atoi(row[idx++]);
					item.Common.ldonsold			= (uint32)atoi(row[idx++]);
					idx+=7;
					item.banedmgamt2				= (uint32)atoi(row[idx++]);
					item.augmentrestriction			= (uint32)atoi(row[idx++]);
					item.loreflag					= (atoi(row[idx++])==0) ? false : true;
					item.pendingloreflag			= (atoi(row[idx++])==0) ? false : true;
					item.artifactflag				= (atoi(row[idx++])==0) ? false : true;
					item.summonedflag				= (atoi(row[idx++])==0) ? false : true;
					item.tribute					= (uint32)atoi(row[idx++]);
					item.gm							= (atoi(row[idx++])==0) ? false : true;
					item.endur						= (uint32)atoi(row[idx++]);
					item.dotshielding				= (uint32)atoi(row[idx++]);
					item.attackbonus				= (uint32)atoi(row[idx++]);
					item.hpregen					= (uint32)atoi(row[idx++]);
					item.manaregen					= (uint32)atoi(row[idx++]);
					item.hastepercent				= (uint32)atoi(row[idx++]);
					item.damageshield				= (uint32)atoi(row[idx++]);
					item.minstatus					= (int8)atoi(row[idx++]);
				}
				// Set cached item properties, then cache the item itself
				item.SetCache();
				if (!EMuShareMemDLL.Items.cbAddItem(item.ItemNumber, &item)) {
					LogFile->write(EQEMuLog::Error, "Database::DBLoadItems: Failure reported from EMuShareMemDLL.Items.cbAddItem(%i)", item.ItemNumber);
					break;
				}
			}
			
			mysql_free_result(result);
			ret = true;
		}
		else {
			LogFile->write(EQEMuLog::Error, "DBLoadItems query '%s', %s", query, errbuf);
		}
	#else
		
		// Using blob fields
		char* query = new char[256];
		strcpy(query, "SELECT MAX(id), count(*) FROM items");
		
		if (RunQuery(query, strlen(query), errbuf, &result)) {
			safe_delete_array(query);
			row = mysql_fetch_row(result);
			if (row != 0 && row[0] > 0) {
				if (row[0])
					max_item = atoi(row[0]);
				else
					max_item = 0;
				if (atoi(row[1]) != iItemCount) {
					cout << "Error: Insufficient shared memory to load items." << endl;
					cout << "Count(id): " << atoi(row[1]) << ", iItemCount: " << iItemCount << endl;
					return false;
				}
				if ((int32)atoi(row[0]) != iMaxItemID) {
					cout << "Error: Insufficient shared memory to load items." << endl;
					cout << "Max(id): " << atoi(row[0]) << ", iMaxItemID: " << iMaxItemID << endl;
					cout << "Fix this by increasing the MMF_EQMAX_ITEMS define statement" << endl;
					return false;
				}
				mysql_free_result(result);
				
				MakeAnyLenString(&query, "SELECT id,raw_data FROM items");
				
				if (RunQuery(query, strlen(query), errbuf, &result)) {
					safe_delete_array(query);
					while((row = mysql_fetch_row(result))) {
						unsigned long* lengths;
						lengths = mysql_fetch_lengths(result);
						if (lengths[1] == sizeof(Item_Struct)) {
							// neotokyo: dirty fix for recommended skill 255
							//if (((Item_Struct*)(row[1]))->Common.RecSkill > 252)
							//	((Item_Struct*)(row[1]))->Common.RecSkill = 252;
							
							if (!EMuShareMemDLL.Items.cbAddItem(atoi(row[0]), (Item_Struct*) row[1])) {
								mysql_free_result(result);
								cout << "Error: Database::DBLoadItems: !EMuShareMemDLL.Items.cbAddItem(" << atoi(row[0]) << ")" << endl;
								return false;
							}
						}
						else {
							// TODO: Invalid item length in database
							printf("Lengths: %ld - sizeof(): %d\n", lengths[1], sizeof(Item_Struct));
						}
						Sleep(0);
					}
					mysql_free_result(result);
				}
				else {
					cerr << "Error in DBLoadItems query '" << query << "' " << errbuf << endl;
					safe_delete_array(query);
					return false;
				}
			}
			else {
				mysql_free_result(result);
			}
		}
		else {
			cerr << "Error in DBLoadItems query '" << query << "' " << errbuf << endl;
		}
		
	#endif
	
	return ret;
}

const Item_Struct* Database::GetItem(uint32 id) {
	return EMuShareMemDLL.Items.GetItem(id);
}

const Item_Struct* Database::IterateItems(uint32* NextIndex) {
	return EMuShareMemDLL.Items.IterateItems(NextIndex);
}
#else
// @merth: This function is way out of date
bool Database::LoadItems()
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(id) FROM items");
	
	
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete_array(query);
		row = mysql_fetch_row(result);
		if (row && row[0]) { 
			max_item = atoi(row[0]);
			item_array = new Item_Struct*[max_item+1];
			for(unsigned int i=0; i<max_item; i++)
			{

				item_array[i] = 0;
			}
			mysql_free_result(result);
			
			MakeAnyLenString(&query, "SELECT id,raw_data FROM items");
			
			if (RunQuery(query, strlen(query), errbuf, &result))
			{
				safe_delete_array(query);
				while((row = mysql_fetch_row(result)))
				{
					unsigned long* lengths;
					lengths = mysql_fetch_lengths(result);
					if (lengths[1] == sizeof(Item_Struct))
					{
						item_array[atoi(row[0])] = new Item_Struct;
						memcpy(item_array[atoi(row[0])], row[1], sizeof(Item_Struct));
                        // neotokyo: dirty fix for recommended skill 255
                        if (item_array[atoi(row[0])]->Common.RecSkill > 252)
                            item_array[atoi(row[0])]->Common.RecSkill = 252;
					}
					else
					{
						// TODO: Invalid item length in database
					}
					Sleep(0);
				}
				mysql_free_result(result);
			}
			else {
				cerr << "Error in LoadItems query '" << query << "' " << errbuf << endl;
				safe_delete_array(query);
				return false;
			}
		}
		else {
			mysql_free_result(result);
		}
	}
	else {
		cerr << "Error in LoadItems query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	return true;
}

const Item_Struct* Database::GetItem(uint32 id) {
	if (item_array && id <= max_item)
		return item_array[id];
	else
		return 0;
}
#endif

#ifdef SHAREMEM
extern "C" bool extDBLoadDoors(sint32 iDoorCount, int32 iMaxDoorID) { return database.DBLoadDoors(iDoorCount, iMaxDoorID); }
const Door* Database::GetDoor(int8 door_id, const char* zone_name) {
	for(uint32 i=0; i!=max_door_type;i++)
	{
        const Door* door;
        door = GetDoorDBID(i);
        if (!door)
		continue;
	if(door->door_id == door_id && strcasecmp(door->zone_name, zone_name) == 0)
	return door;
	}
return 0;
}

const Door* Database::GetDoorDBID(uint32 db_id) {
	return EMuShareMemDLL.Doors.GetDoor(db_id);
}

bool Database::LoadDoors() {
	if (!EMuShareMemDLL.Load())
		return false;
	sint32 tmp = 0;
	tmp = GetDoorsCount(&max_door_type);
	if (tmp == -1) {
		cout << "Error: Database::LoadDoors-ShareMem: GetDoorsCount() returned < 0" << endl;
		return false;
	}
	bool ret = EMuShareMemDLL.Doors.DLLLoadDoors(&extDBLoadDoors, sizeof(Door), &tmp, &max_door_type);
	return ret;
}

bool Database::DBLoadDoors(sint32 iDoorCount, int32 iMaxDoorID) {
   LogFile->write(EQEMuLog::Status, "Loading Doors from database...");
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(id), Count(*) FROM doors");
	if (RunQuery(query, strlen(query), errbuf, &result))
	{
		safe_delete_array(query);
		row = mysql_fetch_row(result);
		if (row && row[0]) {
			if ((int32)atoi(row[0]) > iMaxDoorID) {
				cout << "Error: Insufficient shared memory to load doors." << endl;
				cout << "Max(id): " << atoi(row[0]) << ", iMaxDoorID: " << iMaxDoorID << endl;
				cout << "Fix this by increasing the MMF_MAX_Door_ID define statement" << endl;
				mysql_free_result(result);
				return false;
			}
			if (atoi(row[1]) != iDoorCount) {
				cout << "Error: Insufficient shared memory to load doors." << endl;
				cout << "Count(*): " << atoi(row[1]) << ", iDoorCount: " << iDoorCount << endl;
				mysql_free_result(result);
				return false;
			}
			max_door_type = atoi(row[0]);
			mysql_free_result(result);
			Door tmpDoor;
			MakeAnyLenString(&query, "SELECT id,doorid,zone,name,pos_x,pos_y,pos_z,heading,opentype,guild,lockpick,keyitem,triggerdoor,triggertype,dest_zone,dest_x,dest_y,dest_z,dest_heading,liftheight,invert_state,incline from doors");//WHERE zone='%s'", zone_name
			if (RunQuery(query, strlen(query), errbuf, &result)) {
				safe_delete_array(query);
				while((row = mysql_fetch_row(result))) {
					memset(&tmpDoor, 0, sizeof(Door));
					tmpDoor.db_id = atoi(row[0]);
					tmpDoor.door_id = atoi(row[1]);
					strncpy(tmpDoor.zone_name,row[2],16);
					strncpy(tmpDoor.door_name,row[3],16);
					tmpDoor.pos_x = (float)atof(row[4]);
					tmpDoor.pos_y = (float)atof(row[5]);
					tmpDoor.pos_z = (float)atof(row[6]);
					tmpDoor.heading = (float)atof(row[7]);
					tmpDoor.opentype = atoi(row[8]);
					tmpDoor.guildid = atoi(row[9]);
					tmpDoor.lockpick = atoi(row[10]);
					tmpDoor.keyitem = atoi(row[11]);
					tmpDoor.trigger_door = atoi(row[12]);
					tmpDoor.trigger_type = atoi(row[13]);
                    strncpy(tmpDoor.dest_zone,row[14],16);
                    tmpDoor.dest_x = (float) atof(row[15]);
                    tmpDoor.dest_y = (float) atof(row[16]);
                    tmpDoor.dest_z = (float) atof(row[17]);
                    tmpDoor.dest_heading = (float) atof(row[18]);
					tmpDoor.liftheight=atoi(row[19]);
					tmpDoor.invert_state=atoi(row[20]);
					tmpDoor.incline=atoi(row[21]);
					if (!EMuShareMemDLL.Doors.cbAddDoor(tmpDoor.db_id, &tmpDoor)) {
						mysql_free_result(result);
						return false;
					}
					Sleep(0);
				}
				mysql_free_result(result);
			}
			else
			{
				cerr << "Error in DBLoadDoors query '" << query << "' " << errbuf << endl;
				safe_delete_array(query);
				return false;
			}
		}
		else
			mysql_free_result(result);
	}
	else
		safe_delete_array(query);

	return true;
}
#else
const Door* Database::GetDoor(int8 door_id, const char* zone_name)
{
	for(uint32 i=0; i!=max_door_type;i++)
	{
        const Door* door;
        door = GetDoorDBID(i);
        if (!door)
            continue;
	    if(door->door_id == door_id && strcasecmp(door->zone_name, zone_name) == 0)
	        return door;
	}
    return 0;
}

const Door* Database::GetDoorDBID(uint32 db_id)
{
	return door_array[db_id];
}

bool Database::LoadDoors()
{
	cout << "(NOMEMSHARE) Loading Doors from database..." << endl;
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
    Door tmpDoor;

    if (door_array == 0)
    {
        int32 count;
        
        if (!GetDoorsCount(&count))
            return false;

        max_door_type = count;

        door_array = new Door*[count+1];
        memset(door_array, 0, sizeof(door_array)*(count+1));
        if (!door_array)
            return false;
    }
    else
    {
        int32 count;
        
        if (!GetDoorsCount(&count))
            return false;
        if (count != max_door_type) // what happened here? do we need to reallocate?
            return false;
    }

   MakeAnyLenString(&query, "SELECT id,doorid,zone,name,pos_x,pos_y,pos_z,heading,opentype,guild,lockpick,keyitem,triggerdoor,triggertype,liftheight from doors");//WHERE zone='%s'", zone_name
	if (RunQuery(query, strlen(query), errbuf, &result))
	{
	    safe_delete_array(query);
		while((row = mysql_fetch_row(result)))
        {
		    memset(&tmpDoor, 0, sizeof(Door));
			memset(&door_array[tmpDoor.db_id],0,sizeof(Door));
			tmpDoor.db_id = atoi(row[0]);
			tmpDoor.door_id = atoi(row[1]);
			strncpy(tmpDoor.zone_name,row[2],16);
            strncpy(tmpDoor.door_name,row[3],10);
			tmpDoor.pos_x = (float)atof(row[4]);
			tmpDoor.pos_y = (float)atof(row[5]);
			tmpDoor.pos_z = (float)atof(row[6]);
			tmpDoor.heading = atoi(row[7]);
			tmpDoor.opentype = atoi(row[8]);
			tmpDoor.guildid = atoi(row[9]);
			tmpDoor.lockpick = atoi(row[10]);
			tmpDoor.keyitem = atoi(row[11]);
			tmpDoor.trigger_door = atoi(row[12]);
			tmpDoor.trigger_type = atoi(row[13]);
			tmpDoor.liftheight= atoi(row[14]);
            if (door_array[tmpDoor.db_id] == NULL)
                door_array[tmpDoor.db_id] = new Door;

            else
            {
                cerr << "Double DB_Id " << tmpDoor.door_id << " while loading doors" << endl;
            }
            memcpy(door_array[tmpDoor.db_id], &tmpDoor, sizeof(Door));

            Sleep(0);
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in LoadDoors query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	return true;
}
#endif


#ifdef ZONE
/* Searches npctable for matchind id, and returns the item if found,
 * or NULL otherwise. If id passed is 0, loads all npc_types for
 * the current zone, returning the last item added.
 */
const NPCType* Database::GetNPCType (uint32 id) {
   const NPCType *npc=NULL;
   map<uint32,NPCType *>::iterator itr;

   // If NPC is already in tree, return it.
   if ((itr=zone->npctable.find(id))!=zone->npctable.end())
   	return itr->second;
   // Otherwise, get NPCs from database.
   else
   {
   	char errbuf[MYSQL_ERRMSG_SIZE];
	   char *query;
	   MYSQL_RES *result;
	   MYSQL_ROW row;

      query = new char[512];

      // If id is 0, load all npc_types for the current zone,
      // according to spawn2.
      if (id == 0)
         MakeAnyLenString(&query,
            "SELECT npc_types.id,npc_types.name,npc_types.level,"
            "npc_types.race,npc_types.class,npc_types.hp,"
            "npc_types.gender,npc_types.texture,npc_types.helmtexture,"
            "npc_types.size,npc_types.loottable_id,"
            "npc_types.merchant_id,npc_types.banish,npc_types.mindmg,"
            "npc_types.maxdmg,npc_types.npcspecialattks,"
            "npc_types.npc_spells_id,npc_types.d_meele_texture1,"
            "npc_types.d_meele_texture2,npc_types.walkspeed,"
            "npc_types.runspeed,npc_types.fixedz,"
            "npc_types.hp_regen_rate,npc_types.mana_regen_rate,"
            "npc_types.aggroradius,npc_types.bodytype,"
            "npc_types.npc_faction_id,npc_types.face,"
            "npc_types.see_invis,npc_types.see_invis_undead,"
            "npc_types.lastname,npc_types.qglobal,npc_types.AC"
            " FROM npc_types,spawn2 WHERE spawn2.zone='%s'"
            " AND npc_types.id=spawn2.id",
            zone->GetShortName());
      // Otherwise, just load this specific NPC.
      else
         MakeAnyLenString(&query,
            "SELECT id,name,level,race,class,hp,gender,"
            "texture,helmtexture,size,loottable_id,merchant_id,"
            "banish,mindmg,maxdmg,npcspecialattks,npc_spells_id,"
            "d_meele_texture1,d_meele_texture2,walkspeed,"
            "runspeed,fixedz,hp_regen_rate,mana_regen_rate,"
            "aggroradius,bodytype,npc_faction_id,face,see_invis,"
            "see_invis_undead,lastname,qglobal,AC"
            " FROM npc_types WHERE id=%d", id);

      if (RunQuery(query, strlen(query), errbuf, &result))
	   {
         // Process each row returned.
		   while((row = mysql_fetch_row(result)))
		   {
      	   NPCType *tmpNPCType;
            tmpNPCType = (NPCType *)malloc (sizeof *tmpNPCType);

            memset (tmpNPCType, 0, sizeof *tmpNPCType);
			   strncpy(tmpNPCType->name, row[1], 30);

            tmpNPCType->npc_id = atoi(row[0]); // rembrant, Dec. 2
			   tmpNPCType->level = atoi(row[2]);
			   tmpNPCType->race = atoi(row[3]);
			   tmpNPCType->class_ = atoi(row[4]);
			   tmpNPCType->cur_hp = atoi(row[5]);
			   tmpNPCType->max_hp = atoi(row[5]);
			   tmpNPCType->gender = atoi(row[6]);
			   tmpNPCType->texture = atoi(row[7]);
			   tmpNPCType->helmtexture = atoi(row[8]);
			   tmpNPCType->size = atof(row[9]);
			   tmpNPCType->loottable_id = atoi(row[10]);
			   tmpNPCType->merchanttype = atoi(row[11]);
			   tmpNPCType->STR = 75;
			   tmpNPCType->STA = 75;
			   tmpNPCType->DEX = 75;
			   tmpNPCType->AGI = 75;
			   tmpNPCType->WIS = 75;
			   tmpNPCType->INT = 75;
			   tmpNPCType->CHA = 75;
			   tmpNPCType->banish = atoi(row[12]);
			   tmpNPCType->min_dmg = atoi(row[13]);
			   tmpNPCType->max_dmg = atoi(row[14]);
			   strcpy(tmpNPCType->npc_attacks,row[15]);
			   tmpNPCType->npc_spells_id = atoi(row[16]);
			   tmpNPCType->d_meele_texture1= atoi(row[17]);
			   tmpNPCType->d_meele_texture2= atoi(row[18]);
			   tmpNPCType->walkspeed= atof(row[19]);
			   tmpNPCType->runspeed= atof(row[20]);
			   tmpNPCType->fixedZ = atof(row[21]);
			   tmpNPCType->hp_regen = atoi(row[22]);
			   tmpNPCType->mana_regen = atoi(row[23]);
            tmpNPCType->aggroradius = (sint32)atoi(row[24]);

            if (row[25] && strlen(row[25]))
               tmpNPCType->bodytype = (int8)atoi(row[25]);
            else
               tmpNPCType->bodytype = 0;
			   tmpNPCType->npc_faction_id = atoi(row[26]);
			   tmpNPCType->luclinface = atoi(row[27]);

            // set defaultvalue for aggroradius
            if (tmpNPCType->aggroradius <= 0)
               tmpNPCType->aggroradius = 70;

			   tmpNPCType->see_invis = atoi(row[28]);			// Mongrel: Set see_invis flag
			   tmpNPCType->see_invis_undead = atoi(row[29]);	// Mongrel: Set see_invis_undead flag
            if (row[30] != NULL)
			      strncpy(tmpNPCType->lastname, row[30], 32);
		      tmpNPCType->qglobal = atoi(row[31]);	// qglobal
		      tmpNPCType->AC = atoi(row[32]);

            // If NPC with duplicate NPC id already in table,
            // free item we attempted to add.
	    if (zone->npctable.find(tmpNPCType->npc_id) != zone->npctable.end())
            {
               cerr << "Error loading duplicate NPC "
                    << tmpNPCType->npc_id << endl;
               free (tmpNPCType);
               npc = NULL;
            }
            else
	    {
	       zone->npctable[tmpNPCType->npc_id]=tmpNPCType;
               npc = tmpNPCType;
	    }

            Sleep(0);
		   }

	 if (result) {
         	mysql_free_result(result);
	}
      }
      else
         cerr << "Error loading NPCs from database. Bad query.\n";

      safe_delete_array(query);
   }

   return npc;
}
#endif

bool Database::LoadNPCTypes() {
//cout << "Entered Database::LoadNPCTypes()\n";
/*
   if (!EMuShareMemDLL.Load())
		return false;
	sint32 tmp = -1;
	int32 tmp_max_npc_type;
	tmp = GetNPCTypesCount(&tmp_max_npc_type);
	if (tmp < 0) {
		cout << "Error: Database::LoadNPCTypes-ShareMem: GetNPCTypesCount() returned < 0" << endl;
		return false;
	}
	max_npc_type = tmp_max_npc_type;
	bool ret = EMuShareMemDLL.NPCTypes.DLLLoadNPCTypes(&extDBLoadNPCTypes, sizeof(NPCType), &tmp, &max_npc_type);
	return ret;
*/
//   loadZoneNPCs ();
   return true;
}

#ifndef SHAREMEM
void Database::LoadAnItem(uint16 item_id, unsigned int* texture, unsigned int* color)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	Item_Struct tempitem;
	MakeAnyLenString(&query, "SELECT raw_data FROM items where id=%d",item_id);
	if (RunQuery(query, strlen(query), errbuf, &result))
	{
		safe_delete_array(query);
		while((row = mysql_fetch_row(result)))
		{
			unsigned long* lengths;
			lengths = mysql_fetch_lengths(result);
			if (lengths[0] == sizeof(Item_Struct))
			{
				//tempitem=row[0];
				memcpy(&tempitem, row[0], sizeof(Item_Struct));
				*texture=tempitem.Common.Material;
				*color=tempitem.Common.Color;
			}
			else
			{
				// TODO: Invalid item length in database
			}
			Sleep(0);
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in LoadAnItem query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		//return false;
	}
	
	//return tempitemptr;
}
#endif

#ifndef SHAREMEM
bool Database::SaveItemToDatabase(Item_Struct* ItemToSave)
{
	// Insert the modified item back into the database
	char Query[sizeof(Item_Struct)*2+1 + 64];
	char EscapedText[sizeof(Item_Struct)*2+1];
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	//	long AffectedRows;
	
	if(DoEscapeString(EscapedText, (const char *)ItemToSave, sizeof(Item_Struct))) {
		sprintf(Query, "UPDATE items SET raw_data = '%s' WHERE id = %d\n", EscapedText, ItemToSave->ItemNumber);
		
		if(RunQuery(Query, strlen(Query), errbuf))
		{
			//			dpf("Affected rows = %d", AffectedRows);
			return(true);
		}
		else {
			cout<<"Save failed::"<<Query<<endl;
		}
	}
	
	//dpf("Update failed");
	return(false);
}
#endif

bool Database::LoadZoneNames() {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(zoneidnumber) FROM zone");
	
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete_array(query);
		row = mysql_fetch_row(result);
		if (row && row[0])
		{ 
			max_zonename = atoi(row[0]);
			zonename_array = new char*[max_zonename+1];
			for(unsigned int i=0; i<max_zonename; i++) {
				zonename_array[i] = 0;
			}
			mysql_free_result(result);
			
			MakeAnyLenString(&query, "SELECT zoneidnumber, short_name FROM zone");
			if (RunQuery(query, strlen(query), errbuf, &result)) {
				safe_delete_array(query);
				while((row = mysql_fetch_row(result))) {
					zonename_array[atoi(row[0])] = new char[strlen(row[1]) + 1];
					strcpy(zonename_array[atoi(row[0])], row[1]);
					Sleep(0);
				}
				mysql_free_result(result);
			}
			else {
				cerr << "Error in LoadZoneNames query '" << query << "' " << errbuf << endl;
				safe_delete_array(query);
				return false;
			}
		}
		else {
			mysql_free_result(result);
		}
	}
	else {
		cerr << "Error in LoadZoneNames query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	return true;
}

#ifndef SHAREMEM
bool Database::SetItemAtt(char* att, char * value, unsigned int ItemIndex) {
	if (att && value && ItemIndex) {
		//item info
		if (strstr(att,"name")) {
			strcpy(item_array[ItemIndex]->Name,value);
		}
		else if (strstr(att, "lore")) {
 			strcpy(item_array[ItemIndex]->LoreName,value);
  		}
  		else if (strstr(att,"idfile")) {
 			strcpy(item_array[ItemIndex]->IDFile,value);
  		}
 		//else if (strstr(att, "flag")) {
 		//	item_array[ItemIndex]-> = atoi(value);
 		//}
  		else if (strstr(att, "weight")) {
 			item_array[ItemIndex]->Weight = atoi(value);
  		}
  		else if (strstr(att, "nosave")) {
 			item_array[ItemIndex]->NoDrop = atoi(value);
  		}
  		else if (strstr(att, "nodrop")) {
 			item_array[ItemIndex]->NoRent = atoi(value);
  		}
		else if (strstr(att, "size")) {
 			item_array[ItemIndex]->Size = atoi(value);
  		}
  		else if (strstr(att, "type")) {
 			item_array[ItemIndex]->Type = atoi(value);
  		}
  		else if (strstr(att, "icon")) {
 			item_array[ItemIndex]->IconNumber = atoi(value);
  		}
  		else if (strstr(att, "equipableSlots")) {
 			item_array[ItemIndex]->EquipSlots = atoi(value);
  		}
  		//item stats
		else if (strstr(att, "str")) {
			item_array[ItemIndex]->Common.STR = atoi(value);
		}
		else if (strstr(att, "sta")) {
			item_array[ItemIndex]->Common.STA = atoi(value);
		}
		else if (strstr(att, "cha")) {
			item_array[ItemIndex]->Common.CHA = atoi(value);
		}
		else if (strstr(att, "dex")) {
			item_array[ItemIndex]->Common.DEX = atoi(value);
		}
		else if (strstr(att, "int")) {
			item_array[ItemIndex]->Common.INT = atoi(value);
		}
		else if (strstr(att, "agi")) {
			item_array[ItemIndex]->Common.AGI = atoi(value);
		}
		else if (strstr(att, "wis")) {
			item_array[ItemIndex]->Common.WIS = atoi(value);
		}
		else if (strstr(att, "mr")) {
			item_array[ItemIndex]->Common.SvMagic = atoi(value);
		}
		else if (strstr(att, "fr")) {
			item_array[ItemIndex]->Common.SvFire = atoi(value);
		}
		else if (strstr(att, "cr")) {
			item_array[ItemIndex]->Common.SvCold = atoi(value);
		}
		else if (strstr(att, "dr")) {
			item_array[ItemIndex]->Common.SvDisease = atoi(value);
		}
		else if (strstr(att, "pr")) {
			item_array[ItemIndex]->Common.SvPoison = atoi(value);
		}
		else if (strstr(att, "hp")) {
			item_array[ItemIndex]->Common.HP = atoi(value);
		}
		else if (strstr(att, "mana")) {
			item_array[ItemIndex]->Common.Mana = atoi(value);
		}
		else if (strstr(att, "ac")) {
			item_array[ItemIndex]->Common.AC = atoi(value);
		}
		else if (strstr(att, "material")) {
			item_array[ItemIndex]->Common.Material = atoi(value);
		}
		else if (strstr(att, "damage")) {
			item_array[ItemIndex]->Common.Damage = atoi(value);
		}
		else if (strstr(att, "delay")) {
			item_array[ItemIndex]->Common.Delay = atoi(value);
		}
		else if (strstr(att, "effect")) {
			item_array[ItemIndex]->Common.EffectType = atoi(value);
		}
		else if (strstr(att,"spellID")) {
			item_array[ItemIndex]->Common.SpellId = atoi(value);
		}
		else if (strstr(att,"color")) {
			item_array[ItemIndex]->Common.Color = atoi(value);
		}
		else if (strstr(att,"classes")) {
			uint16 tv = 0;
			if (strstr(value,"warrior"))
				tv += warrior_1;
			if (strstr(value,"paladin"))
				tv += paladin_1;
			if (strstr(value,"shadow"))
				tv += shadow_1;
			if (strstr(value,"ranger"))
				tv += ranger_1;
			if (strstr(value,"rogue"))
				tv += rogue_1;
			if (strstr(value,"monk"))
				tv += monk_1;
			if (strstr(value,"beastlord"))
				tv += beastlord_1;
			if (strstr(value,"wizard"))
				tv += wizard_1;
			if (strstr(value,"enchanter"))
				tv += enchanter_1;
			if (strstr(value,"mage"))
				tv += mage_1;
			if (strstr(value,"necromancer"))
				tv += necromancer_1;
			if (strstr(value,"shaman"))
				tv += shaman_1;
			if (strstr(value,"cleric"))
				tv += cleric_1;
			if (strstr(value,"bard"))
				tv += bard_1;
			if (strstr(value,"druid"))
				tv += druid_1;
			if (strstr(value,"all"))
				tv += call_1;
			item_array[ItemIndex]->Common.Classes = tv;
		}
		else if (strstr(att,"races")) {
			uint16 tv = 0;
			if (strstr(value,"human"))
				tv += human_1;
			if (strstr(value,"barbarian"))
				tv += barbarian_1;
			if (strstr(value,"erudite"))
				tv += erudite_1;
			if (strstr(value,"woodelf"))
				tv += woodelf_1;
			if (strstr(value,"highelf"))
				tv += highelf_1;
			if (strstr(value,"darkelf"))
				tv += darkelf_1;
			if (strstr(value,"halfelf"))
				tv += halfelf_1;
			if (strstr(value,"dwarf"))
				tv += dwarf_1;
			if (strstr(value,"troll"))
				tv += troll_1;
			if (strstr(value,"ogre"))
				tv += ogre_1;
			if (strstr(value,"halfling"))
				tv += halfling_1;
			if (strstr(value,"gnome"))
				tv += gnome_1;
			if (strstr(value,"iksar"))
				tv += iksar_1;
			if (strstr(value,"vahshir"))
				tv += vahshir_1;
			if (strstr(value,"all"))
				tv += rall_1;			
			item_array[ItemIndex]->Common.Races = tv;
		}
		SaveItemToDatabase(item_array[ItemIndex]);
		return true;
	}
	return false;
	return false;
}
#endif

int32 Database::GetZoneID(const char* zonename) {
	if (zonename_array == 0)
		return 0;
	if (zonename == 0)
		return 0;
	for (unsigned int i=0; i<=max_zonename; i++) {
		if (zonename_array[i] != 0 && strcasecmp(zonename_array[i], zonename) == 0) {
			return i;
		}
	}
	return 0;
}

const char* Database::GetZoneName(int32 zoneID, bool ErrorUnknown) {
	if (zonename_array == 0) {
		if (ErrorUnknown)
			return "UNKNOWN";
		else
			return 0;
	}
	if (zoneID <= max_zonename) {
		if (zonename_array[zoneID])
			return zonename_array[zoneID];
		else {
			if (ErrorUnknown)
				return "UNKNOWN";
			else
				return 0;
		}
	}
	if (ErrorUnknown)
		return "UNKNOWN";
	else
		return 0;
}

bool Database::CheckNameFilter(const char* name) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	int i;
	
	// the minimum 4 is enforced by the client too
	if(!name || strlen(name) < 4 || strlen(name) > 64)
		return false;

	for (i = 0; name[i]; i++)
	{
		if(!isalpha(name[i]))
			return false;
	}

	if (RunQuery(query, MakeAnyLenString(&query, "SELECT count(*) FROM name_filter WHERE '%s' like name", name), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if (row[0] != 0) {
				if (atoi(row[0]) == 0) {

					mysql_free_result(result);
					return false;
				}
			}
		}
		mysql_free_result(result);
		return true;
	}
	else
	{
		cerr << "Error in CheckNameFilter query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	
	return false;
}

bool Database::AddToNameFilter(const char* name) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO name_filter (name) values ('%s')", name), errbuf, 0, &affected_rows)) {
		cerr << "Error in AddToNameFilter query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	safe_delete_array(query);
	
	if (affected_rows == 0) {
		return false;
	}
	
	return true;
}

int32 Database::GetAccountIDFromLSID(int32 iLSID, char* oAccountName, sint16* oStatus) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, name, status FROM account WHERE lsaccount_id=%i", iLSID), errbuf, &result))
	{
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 account_id = atoi(row[0]);
			if (oAccountName)
				strcpy(oAccountName, row[1]);
			if (oStatus)
				*oStatus = atoi(row[2]);
			mysql_free_result(result);
			return account_id;
		}
		else
		{
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetAccountIDFromLSID query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return 0;
	}
	
	return 0;
}

int8 Database::GetGridType(int16 grid,int16 zoneid ) {
	char *query = 0;
	char errbuf[MYSQL_ERRMSG_SIZE];
	MYSQL_RES *result;
	MYSQL_ROW row;
	int type = 0;
	if (RunQuery(query, MakeAnyLenString(&query,"SELECT type from grid where id = %i and zoneid = %i",grid,zoneid),errbuf,&result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			type = atoi( row[0] );
		}
			mysql_free_result(result);
	} else {
		cerr << "Error in GetGridType query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	return type;
}


int8 Database::GetGridType2(int16 grid, int16 zoneid) {
	char *query = 0;
	char errbuff[MYSQL_ERRMSG_SIZE];
	MYSQL_RES *result;
	MYSQL_ROW row;
	int type2 = 0;
	if (RunQuery(query, MakeAnyLenString(&query,"SELECT type2 from grid where id = %i and zoneid = %i",grid,zoneid),errbuff,&result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			type2 = atoi( row[0] );
		}
		mysql_free_result(result);
	} else {
		cerr << "Error in GetGridType2 query '" << query << "' " << errbuff << endl;
		safe_delete_array(query);
	}

	return(type2);
}

bool Database::GetWaypoints(int16 grid,int16 zoneid, int16 num, wplist* wp) {
	char *query = 0;
	char errbuff[MYSQL_ERRMSG_SIZE];
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query,"SELECT x, y, z, pause from grid_entries where gridid = %i and number = %i and zoneid = %i",grid,num,zoneid),errbuff,&result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if ( wp ) {
				wp->x = atof( row[0] );
				wp->y = atof( row[1] );
				wp->z = atof( row[2] );
				wp->pause = atoi( row[3] );
			}
			mysql_free_result(result);
			return true;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetWaypoints query '" << query << "' " << errbuff << endl;
		safe_delete_array(query);
	}
	return false;
}

#ifdef ZONE
void Database::AssignGrid(Client *client, float x, float y, int32 grid)
{
	char *query = 0;
	char errbuf[MYSQL_ERRMSG_SIZE];
	MYSQL_RES *result;
	MYSQL_ROW row;
	int matches = 0, fuzzy = 0, spawn2id = 0;
	int32 affected_rows;
	float dbx = 0, dby = 0;

	// solar: looks like most of the stuff in spawn2 is straight integers
	// so let's try that first
	RunQuery(
		query,
		MakeAnyLenString(
			&query,
			"SELECT id,x,y FROM spawn2 WHERE zone='%s' AND x=%i AND y=%i",
			zone->GetShortName(), (int)x, (int)y
		),
		errbuf,
		&result
	);
	safe_delete_array(query);

// how much it's allowed to be off by
#define _GASSIGN_TOLERANCE	1.0
	if(!(matches = mysql_num_rows(result)))	// try a fuzzy match if that didn't find it
	{
		mysql_free_result(result);
		RunQuery(
			query,
			MakeAnyLenString(
				&query,
				"SELECT id,x,y FROM spawn2 WHERE zone='%s' AND "
				"ABS( ABS(x) - ABS(%f) ) < %f AND "
				"ABS( ABS(y) - ABS(%f) ) < %f",
				zone->GetShortName(), x, _GASSIGN_TOLERANCE, y, _GASSIGN_TOLERANCE
			),
			errbuf,
			&result
		);
		safe_delete_array(query);
		fuzzy = 1;
		if(!(matches = mysql_num_rows(result)))
			mysql_free_result(result);
	}
	if(matches)
	{
		if(matches > 1)
		{
			client->Message(0, "ERROR: Unable to assign grid - multiple spawn2 rows match");
			mysql_free_result(result);
		}
		else
		{
			row = mysql_fetch_row(result);
			spawn2id = atoi(row[0]);
			dbx = atof(row[1]);
			dby = atof(row[2]);
			RunQuery(
				query,
				MakeAnyLenString(
					&query,
					"UPDATE spawn2 SET pathgrid = %d WHERE id = %d", grid, spawn2id
				),
				errbuf,
				&result,
				&affected_rows
			);
			if(affected_rows == 1)
			{
				if(fuzzy)
				{
					float difference;
					difference = sqrt(pow(fabs(x-dbx),2) + pow(fabs(y-dby),2));
					client->Message(0, 
						"Grid assign: spawn2 id = %d updated - fuzzy match: deviation %f",
						spawn2id, difference
					);
				}
				else
				{
					client->Message(0, "Grid assign: spawn2 id = %d updated - exact match", spawn2id);
				}
			}
			else
			{
				client->Message(0, "ERROR: found spawn2 id %d but the update query failed", spawn2id);
			}
		}
	}
	else
	{
		client->Message(0, "ERROR: Unable to assign grid - can't find it in spawn2");
	}
}
#endif

/******************
* ModifyGrid - Either adds an empty grid, or removes a grid and all its waypoints, for a particular zone.
*	remove:		TRUE if we are deleting the specified grid, FALSE if we are adding it
*	id:		The ID# of the grid to add or delete
*	type,type2:	The type and type2 values for the grid being created (ignored if grid is being deleted)
*	zoneid:		The ID number of the zone the grid is being created/deleted in
*/

void Database::ModifyGrid(bool remove, int16 id, int8 type, int8 type2, int16 zoneid)
{    char *query = 0;
	if (!remove)
	{
		RunQuery(query, MakeAnyLenString(&query,"INSERT INTO grid(id,zoneid,type,type2) VALUES(%i,%i,%i,%i)",id,zoneid,type,type2));
		safe_delete_array(query);
	}
	else
	{
		RunQuery(query, MakeAnyLenString(&query,"DELETE FROM grid where id=%i",id));
		safe_delete_array(query);
    query = 0;
		RunQuery(query, MakeAnyLenString(&query,"DELETE FROM grid_entries WHERE zoneid=%i AND gridid=%i",zoneid,id));
	safe_delete_array(query);
	}
} /*** END Database::ModifyGrid() ***/

/**************************************
* AddWP - Adds a new waypoint to a specific grid for a specific zone.
*/

void Database::AddWP(int32 gridid, int8 wpnum, float xpos, float ypos, float zpos, int32 pause, int16 zoneid)
{   char *query = 0;

	RunQuery(query,MakeAnyLenString(&query,"INSERT INTO grid_entries (gridid,zoneid,`number`,x,y,z,pause) values (%i,%i,%i,%f,%f,%f,%i)",gridid,zoneid,wpnum,xpos,ypos,zpos,pause));
	safe_delete_array(query);
} /*** END Database::AddWP() ***/


/**********
* ModifyWP() has been obsoleted.  The #wp command either uses AddWP() or DeleteWaypoint()
***********/

/******************
* DeleteWaypoint - Removes a specific waypoint from the grid
*	grid_id:	The ID number of the grid whose wp is being deleted
*	wp_num:		The number of the waypoint being deleted
*	zoneid:		The ID number of the zone that contains the waypoint being deleted
*/

void Database::DeleteWaypoint(int16 grid_num, int32 wp_num, int16 zoneid)
{    char *query=0;
	RunQuery(query, MakeAnyLenString(&query,"DELETE FROM grid_entries where gridid=%i and zoneid=%i and `number`=%i",grid_num,zoneid,wp_num));
	safe_delete_array(query);
} /*** END Database::DeleteWaypoint() ***/


/******************
* AddWPForSpawn - Used by the #wpadd command - for a given spawn, this will add a new waypoint to whatever grid that spawn is assigned to.
* If there is currently no grid assigned to the spawn, a new grid will be created using the next available Grid ID number for the zone
* the spawn is in.
* Returns 0 if the function didn't have to create a new grid.  If the function had to create a new grid for the spawn, then the ID of
* the created grid is returned.
*/

int32 Database::AddWPForSpawn(int32 spawn2id, float xpos, float ypos, float zpos, int32 pause, int type1, int type2, int16 zoneid)
{   char	*query = 0;
    int32	grid_num,	// The grid number the spawn is assigned to (if spawn has no grid, will be the grid number we end up creating)
		next_wp_num;	// The waypoint number we should be assigning to the new waypoint
    bool	CreatedNewGrid;	// Did we create a new grid in this function?
    MYSQL_RES	*result;
    MYSQL_ROW	row;
	
	// See what grid number our spawn is assigned
	if(RunQuery(query, MakeAnyLenString(&query,"SELECT pathgrid FROM spawn2 WHERE id=%i",spawn2id),0,&result))
	{   safe_delete_array(query);
	    if(mysql_num_rows(result) > 0)
	    {
		row = mysql_fetch_row(result);
		grid_num = atoi(row[0]);
	    }
	    else	// This spawn ID was not found in the `spawn2` table
		return 0;

	    mysql_free_result(result);
	}
	else	// Query error
		return 0;

	if (grid_num == 0)	// Our spawn doesn't have a grid assigned to it -- we need to create a new grid and assign it to the spawn
	{   
	    CreatedNewGrid = true;
	    if((grid_num = GetFreeGrid(zoneid)) == 0)	// There are no grids for the current zone -- create Grid #1
		grid_num = 1;

	    RunQuery(query, MakeAnyLenString(&query,"insert into grid set id='%i',zoneid= %i, type='%i', type2='%i'",grid_num,zoneid,type1,type2));
	    safe_delete_array(query);

	    query = 0;
	    RunQuery(query, MakeAnyLenString(&query,"update spawn2 set pathgrid='%i' where id='%i'",grid_num,spawn2id));
		safe_delete_array(query);
	}
	else	// NPC had a grid assigned to it
		CreatedNewGrid = false;
	
	
	// Find out what the next waypoint is for this grid
	query = 0;
	if(RunQuery(query, MakeAnyLenString(&query,"SELECT max(`number`) FROM grid_entries WHERE zoneid='%i' AND gridid='%i'",zoneid,grid_num),0,&result))
	{
	    safe_delete_array(query);
	    row = mysql_fetch_row(result);
	    if(row[0] != 0)
		next_wp_num = atoi(row[0]) + 1;
	    else	// No waypoints in this grid yet
		next_wp_num = 1;

	    mysql_free_result(result);
	}
	else	// Query error
		return 0;

	query = 0;
	RunQuery(query, MakeAnyLenString(&query,"INSERT INTO grid_entries(gridid,zoneid,`number`,x,y,z,pause) VALUES (%i,%i,%i,%f,%f,%f,%i)",grid_num,zoneid,next_wp_num,xpos,ypos,zpos,pause));
		safe_delete_array(query);
	
	if(CreatedNewGrid)	return grid_num;
	else			return 0;
} /*** END Database::AddWPForSpawn() ***/

/*** (The Database::DeleteGrid() command has been obsoleted.  To remove a grid entirely, use ModifyGrid() and pass True as the
'remove' argument.
***/


int16 Database::GetFreeGrid(int16 zoneid) {
    char *query = 0;
	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query,"SELECT max(id) from grid where zoneid = %i",zoneid),errbuf,&result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int16 tmp=0;
			if (row[0]) 
				tmp = atoi(row[0]);
			mysql_free_result(result);
			tmp++;
			return tmp;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetWaypoints query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	return 0;
}

bool Database::CreateSpawn2(int32 spawngroup, const char* zone, float heading, float x, float y, float z, int32 respawn, int32 variance)
{
	char errbuf[MYSQL_ERRMSG_SIZE];

    char *query = 0;
	int32 affected_rows = 0;
	
	//	if(GetInverseXY()==1) {
	//		float temp=x;
	//		x=y;
	//		y=temp;
	//	}
	if (RunQuery(query, MakeAnyLenString(&query, "INSERT INTO spawn2 (spawngroupID,zone,x,y,z,heading,respawntime,variance) Values (%i, '%s', %f, %f, %f, %f, %i, %i)", spawngroup, zone, x, y, z, heading, respawn, variance), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		if (affected_rows == 1) {
			return true;
		}
		else {
			return false;
		}
	}
	else {
		cerr << "Error in CreateSpawn2 query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return false;
}
int	Database::GetMerchantSlot(int32 merchantid, int32 item)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT slot FROM merchantlist WHERE merchantid=%d and item=%d", merchantid, item), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) >= 1) {
			row = mysql_fetch_row(result);
			int tmp = atoi(row[0]);
			mysql_free_result(result);
			return tmp;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetMerchantSlot query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	
	return 0;
}
int32	Database::GetMerchantData(int32 merchantid, int32 slot)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT item FROM merchantlist WHERE merchantid=%d and slot=%d", merchantid, slot), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 tmp = atoi(row[0]);
			mysql_free_result(result);
			return tmp;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetMerchantData query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	
	return 0;
}
int32	Database::GetMerchantListNumb(int32 merchantid)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT item FROM merchantlist WHERE merchantid=%d", merchantid), errbuf, &result)) {
		safe_delete_array(query);
		int32 tmp = mysql_num_rows(result);
		mysql_free_result(result);
		return tmp;
	}
	else {
		cerr << "Error in GetMerchantListNumb query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	
	return 0;
}

bool Database::UpdateName(const char* oldname, const char* newname) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32	affected_rows = 0;
	
	cout << "Renaming " << oldname << " to " << newname << "..." << endl;
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE character_ SET name='%s' WHERE name='%s';", newname, oldname), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		return false;
	}
	safe_delete_array(query);
	
	if (affected_rows == 0)
	{
		return false;
	}
	
	return true;
}

// If the name is used or an error occurs, it returns false, otherwise it returns true
bool Database::CheckUsedName(const char* name)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
    MYSQL_RES *result;
	//if (strlen(name) > 15)
	//	return false;
	if (!RunQuery(query, MakeAnyLenString(&query, "SELECT id FROM character_ where name='%s'", name), errbuf, &result)) {
		cerr << "Error in CheckUsedName query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	else { // It was a valid Query, so lets do our counts!
		safe_delete_array(query);
		int32 tmp = mysql_num_rows(result);
		mysql_free_result(result);
		if (tmp > 0) // There is a Name!  No change (Return False)
			return false;
		else // Everything is okay, so we go and do this.
			return true;
	}
}

bool Database::GetTradeRecipe(const ItemContainerInst* container, uint8 c_type, uint8 tradeskill, 
	DBTradeskillRecipe_Struct *spec)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
    char *query = 0;
	char buf2[220];
	
	uint32 sum = 0;
	uint32 count = 0;
	uint32 qcount = 0;
	uint32 qlen = 0;
	
	//use the world item type as type if we have a world item
	//otherwise use the item's ID... this make the assumption that
	//no tradeskill containers will have an item ID which is
	//below the highest ID of objects, which is currently 0x30
	uint32 type = c_type;
	
	//dunno why I have to cast this up to call GetItem
	const Item_Struct *istruct = ((const ItemInst *) container)->GetItem();
	if(c_type == 0 && istruct) {
		type = istruct->ItemNumber;
	}
	
	//Could prolly watch for stacks in this loop and handle them properly...
	//just increment sum and count accordingly
	bool first = true;
	uint8 i;
	char *pos = buf2;
	for (i=0; i<10; i++) {
		const ItemInst* inst = container->GetItem(i);
		if (inst) {
			const Item_Struct* item = GetItem(inst->GetItem()->ItemNumber);
			if (item) {
				if(first) {
					pos += snprintf(pos, 19, "%d", item->ItemNumber);
					first = false;
				} else {
					pos += snprintf(pos, 19, ",%d", item->ItemNumber);
				}
				sum += item->ItemNumber;
				count++;
			}
		}
	}
	
	qlen = MakeAnyLenString(&query, "SELECT tre.recipe_id "
	" FROM tradeskill_recipe_entries AS tre"
	" WHERE ( tre.item_id IN(%s) AND tre.componentcount>0 )"
	"  OR ( tre.item_id=%u AND tre.iscontainer=1 )"
	" GROUP BY tre.recipe_id HAVING sum(tre.componentcount) = %u"
	"  AND sum(tre.item_id * tre.componentcount) = %u", buf2, type, count, sum);
	
	if (!RunQuery(query, qlen, errbuf, &result)) {
		LogFile->write(EQEMuLog::Error, "Error in GetTradeRecept search, query: %s", query);
		safe_delete_array(query);
		LogFile->write(EQEMuLog::Error, "Error in GetTradeRecept search, error: %s", errbuf);
		return(false);
	}
	safe_delete_array(query);
	
	qcount = mysql_num_rows(result);
	if(qcount > 1) {
		//multiple recipes, partial match... do an extra query to get it exact.
		//this happens when combining components for a smaller recipe
		//which is completely contained within another recipe
		
		first = true;
		pos = buf2;
		for (i = 0; i < qcount; i++) {
			row = mysql_fetch_row(result);
			uint32 recipeid = (uint32) atoi(row[0]);
			if(first) {
				pos += snprintf(pos, 19, "%u", recipeid);
				first = false;
			} else {
				pos += snprintf(pos, 19, ",%u", recipeid);
			}
		}
		
		qlen = MakeAnyLenString(&query, "SELECT tre.recipe_id"
		" FROM tradeskill_recipe_entries AS tre"
		" WHERE tre.recipe_id IN (%s)"
		" GROUP BY tre.recipe_id HAVING sum(tre.componentcount) = %u"
		"  AND sum(tre.item_id * tre.componentcount) = %u", buf2, count, sum);
		
		if (!RunQuery(query, qlen, errbuf, &result)) {
			LogFile->write(EQEMuLog::Error, "Error in GetTradeRecept, re-query: %s", query);
			safe_delete_array(query);
			LogFile->write(EQEMuLog::Error, "Error in GetTradeRecept, error: %s", errbuf);
			return(false);
		}
		safe_delete_array(query);
	
		qcount = mysql_num_rows(result);
	}
	if (qcount != 1) {
		if(qcount > 1) {
			LogFile->write(EQEMuLog::Error, "Combine error: Recipe is not unique!");
		}
		//else, just not found i guess..
		return(false);
	}
	
	row = mysql_fetch_row(result);
	uint32 recipe_id = (uint32)atoi(row[0]);
	mysql_free_result(result);
	
	return(GetTradeRecipe(recipe_id, c_type, tradeskill, spec));
}
	


bool Database::GetTradeRecipe(uint32 recipe_id, uint8 c_type, uint8 tradeskill, 
	DBTradeskillRecipe_Struct *spec)
{	
	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
    char *query = 0;
	
	uint32 qcount = 0;
	uint32 qlen;
	
	qlen = MakeAnyLenString(&query, "SELECT tr.skillneeded, tr.trivial, tr.nofail"
	" FROM tradeskill_recipe AS tr"
	" WHERE tr.id = %lu AND tr.tradeskill = %u", recipe_id, tradeskill);
		
	if (!RunQuery(query, qlen, errbuf, &result)) {
		LogFile->write(EQEMuLog::Error, "Error in GetTradeRecept, query: %s", query);
		safe_delete_array(query);
		LogFile->write(EQEMuLog::Error, "Error in GetTradeRecept, error: %s", errbuf);
		return(false);
	}
	safe_delete_array(query);
	
	qcount = mysql_num_rows(result);
	if(qcount != 1) {
		//just not found i guess..
		return(false);
	}
	
	row = mysql_fetch_row(result);
	spec->skill_needed	= (sint16)atoi(row[0]);
	spec->trivial		= (uint16)atoi(row[1]);
	spec->nofail		= atoi(row[2]) ? true : false;
	mysql_free_result(result);
	
	//Pull the on-success items...
	qlen = MakeAnyLenString(&query, "SELECT item_id,successcount FROM tradeskill_recipe_entries"
	 " WHERE successcount>0 AND componentcount=0 AND recipe_id=%u", recipe_id);
	 
	if (!RunQuery(query, qlen, errbuf, &result)) {
		LogFile->write(EQEMuLog::Error, "Error in GetTradeRecept success query '%s': %s", query, errbuf);
		safe_delete_array(query);
		return(false);
	}
	safe_delete_array(query);
	
	qcount = mysql_num_rows(result);
	if(qcount < 1) {
		LogFile->write(EQEMuLog::Error, "Error in GetTradeRecept success: no success items returned");
		return(false);
	}
	uint8 r;
	spec->onsuccess.clear();
	for(r = 0; r < qcount; r++) {
		row = mysql_fetch_row(result);
		/*if(r == 0) {
			*product1_id	= (uint32)atoi(row[0]);
			*productcount	= (uint32)atoi(row[1]);
		} else if(r == 1) {
			*product2_id	= (uint32)atoi(row[0]);
		} else {
			LogFile->write(EQEMuLog::Warning, "Warning: recipe returned more than 2 products, not yet supported.");
		}*/
		uint32 item = (uint32)atoi(row[0]);
		uint8 num = (uint8) atoi(row[1]);
		spec->onsuccess.push_back(pair<uint32,uint8>::pair(item, num));
	}
	mysql_free_result(result);
	
	
	//Pull the on-fail items...
	qlen = MakeAnyLenString(&query, "SELECT item_id,failcount FROM tradeskill_recipe_entries"
	 " WHERE failcount>0 AND componentcount=0 AND recipe_id=%u", recipe_id);

	spec->onfail.clear();
	if (RunQuery(query, qlen, errbuf, &result)) {
		
		qcount = mysql_num_rows(result);
		uint8 r;
		for(r = 0; r < qcount; r++) {
			row = mysql_fetch_row(result);
			/*if(r == 0) {
				*failproduct_id	= (uint32)atoi(row[0]);
			} else {
				LogFile->write(EQEMuLog::Warning, "Warning: recipe returned more than 1 fail product, not yet supported.");
			}*/
			uint32 item = (uint32)atoi(row[0]);
			uint8 num = (uint8) atoi(row[1]);
			spec->onfail.push_back(pair<uint32,uint8>::pair(item, num));
		}
		mysql_free_result(result);
	}
	safe_delete_array(query);
	
	return(true);
}

string Database::GetBook(char txtfile[20])
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	char txtfile2[20];
	string txtout;
    strcpy(txtfile2,txtfile);
	if (!RunQuery(query, MakeAnyLenString(&query, "SELECT txtfile FROM books where name='%s'", txtfile2), errbuf, &result)) {
		cerr << "Error in GetBook query '" << query << "' " << errbuf << endl;
		if (query != 0)
			safe_delete_array(query);
		txtout.assign(" ",1);
		return txtout;
	}
	else {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 0) {
			mysql_free_result(result);
			LogFile->write(EQEMuLog::Error, "No book to send, (%s)", txtfile);
			txtout.assign(" ",1);
			return txtout;
		}
		else {
			row = mysql_fetch_row(result);
			LogFile->write(EQEMuLog::Normal, "Sending book (%s) Text: %s", txtfile,row[0]);
			txtout.assign(row[0],strlen(row[0]));
			mysql_free_result(result);
			return txtout;
		}
	}
}

bool Database::UpdateZoneSafeCoords(const char* zonename, float x=0, float y=0, float z=0) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32	affected_rows = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE zone SET safe_x='%f', safe_y='%f', safe_z='%f' WHERE short_name='%s';", x, y, z, zonename), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		return false;
	}
	safe_delete_array(query);
	
	if (affected_rows == 0)
	{
		return false;
	}
	
	return true;
}
int8 Database::GetServerType()
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT value FROM variables WHERE varname='ServerType'"), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);
			int8 ServerType = atoi(row[0]);
			mysql_free_result(result);
			return ServerType;
		}
		else
		{
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
	}
	else

	{
		

		cerr << "Error in GetServerType query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return 0;
	
}

int8 Database::GetUseCFGSafeCoords()
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT value FROM variables WHERE varname='UseCFGSafeCoords'"), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1)
		{
			row = mysql_fetch_row(result);

			int8 usecoords = atoi(row[0]);
			mysql_free_result(result);
			return usecoords;
		}
		else
		{
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
	}
	else
	{
		
		cerr << "Error in GetUseCFGSafeCoords query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return 0;
	
}
bool Database::MoveCharacterToZone(char* charname, const char* zonename,int32 zoneid) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int32	affected_rows = 0;
	assert(strlen(zonename) != 0);
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE character_ SET zonename = '%s',zoneid=%i,x=-1, y=-1, z=-1 WHERE name='%s'", zonename,zoneid, charname), errbuf, 0,&affected_rows)) {
		cerr << "Error in MoveCharacterToZone(name) query '" << query << "' " << errbuf << endl;
		return false;
	}
	safe_delete_array(query);
	
	if (affected_rows == 0)
		return false;
	
	return true;
}
bool Database::MoveCharacterToZone(char* charname, const char* zonename) {
	return MoveCharacterToZone(charname, zonename, GetZoneID(zonename));
}

bool Database::MoveCharacterToZone(int32 iCharID, const char* iZonename) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int32	affected_rows = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE character_ SET zonename = '%s', zoneid=%i, x=-1, y=-1, z=-1 WHERE id=%i", iZonename, GetZoneID(iZonename), iCharID), errbuf, 0,&affected_rows)) {
		cerr << "Error in MoveCharacterToZone(id) query '" << query << "' " << errbuf << endl;
		return false;
	}
	safe_delete_array(query);
	
	if (affected_rows == 0)
		return false;
	
	return true;
}

int8 Database::CopyCharacter(const char* oldname, const char* newname, int32 acctid) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	PlayerProfile_Struct* pp;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT profile, guild, guildrank FROM character_ WHERE name='%s'", oldname), errbuf, &result)) {
		safe_delete_array(query);
		
		row = mysql_fetch_row(result);
		
		pp = (PlayerProfile_Struct*)row[0];
		strcpy(pp->name, newname);
		
		mysql_free_result(result);
	}

	else {	
		cerr << "Error in CopyCharacter read query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return 0;
	}
	
	int32 affected_rows = 0;
	char query2[256 + sizeof(PlayerProfile_Struct)*2 + 1];
	char* end=query2;
	
	end += sprintf(end, "INSERT INTO character_ SET zonename=\'%s\', x = %f, y = %f, z = %f, profile=\'", GetZoneName(pp->zone_id), pp->x, pp->y, pp->z);
    end += DoEscapeString(end, (char*) pp, sizeof(PlayerProfile_Struct));
    end += sprintf(end, "\', account_id=%d, name='%s'", acctid, newname);
	
	if (!RunQuery(query2, (int32) (end - query2), errbuf, 0, &affected_rows)) {
        cerr << "Error in CopyCharacter query '" << query << "' " << errbuf << endl;
		return 0;
    }
	
	// @merth: Need to copy inventory as well (and shared bank?)
	if (affected_rows == 0) {
		return 0;
	}
	
	return 1;
}

/*
Get the name of the alternate advancement skill with the given 'index'.
Return true if the name was found, otherwise false.
False will also be returned if there is a database error.
*/
int8 Database::GetTotalAALevels(int32 skill_id) {
char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	int total=0;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT count(ability) from aa_levels where aa_id=%i", skill_id), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			total=atoi(row[0]);
		}
		mysql_free_result(result);
	} else {
		cerr << "Error in GetTotalAALevels '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	return total;
}
int32 Database::CountAAs(){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	int count=0;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT count(skill_id) from altadv_vars"), errbuf, &result)) {
		if(row = mysql_fetch_row(result))
			count = atoi(row[0]);
	}
	safe_delete_array(query);
	mysql_free_result(result);
	return count;
}
int32 Database::CountAALevels(){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	int count=0;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT count(id) from aa_levels"), errbuf, &result)) {
		if(row = mysql_fetch_row(result))
			count = atoi(row[0]);
	}
	safe_delete_array(query);
	mysql_free_result(result);
	return count;
}
int32 Database::GetSizeAA(){
	return (CountAAs()*sizeof(SendAA_Struct))+(CountAALevels()*sizeof(AA_Ability));
}
void Database::LoadAAs(AA_List* load){
	if(!load)
		return;
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT skill_id from altadv_vars order by skill_id"), errbuf, &result)) {
		int skill=0,ndx=0;
		while(row = mysql_fetch_row(result)) {
			skill=atoi(row[0]);
			load->aa[ndx]=GetAASkillVars(skill);
			load->aa[ndx]->seq=ndx+1;
			ndx++;
		}
	}
	safe_delete_array(query);
	mysql_free_result(result);
}
void Database::RetrieveAALevels(SendAA_Struct* aa_struct){
	if(!aa_struct)
		return;
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT ability, increase_amt, level from aa_levels where aa_id=%i order by level asc", aa_struct->id), errbuf, &result)) {
		int ndx=0;
		while(row = mysql_fetch_row(result)) {
			aa_struct->abilities[ndx].skill_id=atoi(row[0]);
			aa_struct->abilities[ndx].increase_amt=atoi(row[1]);
			aa_struct->abilities[ndx].last_level=atoi(row[2]);
			ndx++;
		}
	}
	safe_delete_array(query);
	mysql_free_result(result);
}

SendAA_Struct* Database::GetAASkillVars(int32 skill_id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	SendAA_Struct* sendaa = NULL;
	uchar* buffer;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT cost, max_level, hotkey_sid, hotkey_sid2, title_sid, desc_sid, type, prereq_skill, prereq_minpoints, spell_type, spell_refresh, classes, berserker,spellid FROM altadv_vars WHERE skill_id=%i", skill_id), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			int total_abilities = GetTotalAALevels(skill_id);
			int totalsize = total_abilities * sizeof(AA_Ability) + sizeof(SendAA_Struct);
			buffer = new uchar[totalsize];
			memset(buffer,0,totalsize);
			row = mysql_fetch_row(result);
			sendaa = (SendAA_Struct*)buffer;
			sendaa->cost=atoi(row[0]);
			sendaa->cost2=sendaa->cost;
			sendaa->max_level=atoi(row[1]);
			sendaa->hotkey_sid=atoi(row[2]);
			sendaa->id=skill_id;
			sendaa->hotkey_sid2=atoi(row[3]);
			sendaa->title_sid=atoi(row[4]);
			sendaa->desc_sid=atoi(row[5]);
			sendaa->type=atoi(row[6]);
			sendaa->prereq_skill=atoi(row[7]);
			sendaa->prereq_minpoints=atoi(row[8]);
			sendaa->spell_type=atoi(row[9]);
			sendaa->spell_refresh=atoi(row[10]);
			sendaa->classes=atoi(row[11]);
			sendaa->berserker=atoi(row[12]);
			sendaa->last_id=0xFFFFFFFF;
			sendaa->current_level=1;
			sendaa->spellid=atoi(row[14]);
			switch(sendaa->type){
				case 1:
					sendaa->class_type=0x33;
					break;
				case 2:
					sendaa->class_type=0x37;
					break;
				case 3:
					sendaa->class_type=0x3B;
					break;
				case 4:
					sendaa->class_type=0x3D;
					break;
				case 5:
					sendaa->class_type=0x3E;
					break;
			}
			sendaa->total_abilities=total_abilities;
			if(sendaa->hotkey_sid==0xFFFFFFFF)
				sendaa->next_id=skill_id+1;
			else
				sendaa->next_id=0xFFFFFFFF;
			RetrieveAALevels(sendaa);
		}
		mysql_free_result(result);
	} else {
		cerr << "Error in GetAASkillVars '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	return sendaa;
}

/*
Update the player alternate advancement table for the given account "account_id" and character name "name"
Return true if the character was found, otherwise false.
False will also be returned if there is a database error.
*/
bool Database::SetPlayerAlternateAdv(int32 account_id, char* name, PlayerAA_Struct* aa)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[256+sizeof(PlayerAA_Struct)*2+1];
	char* end = query;
	
	end += sprintf(end, "UPDATE character_ SET alt_adv=\'");
	end += DoEscapeString(end, (char*)aa, sizeof(PlayerAA_Struct));
	*end++ = '\'';
	end += sprintf(end," WHERE account_id=%d AND name='%s'", account_id, name);
	
	int32 affected_rows = 0;
    if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows)) {
        cerr << "Error in SetPlayerAlternateAdv query " << errbuf << endl;
		return false;
    }
	
	if (affected_rows == 0) {
		return false;
	}
	
	return true;
}

/*
Get the player alternate advancement table for the given account "account_id" and character name "name"
Return true if the character was found, otherwise false.
False will also be returned if there is a database error.
*/
int32 Database::GetPlayerAlternateAdv(int32 account_id, char* name, PlayerAA_Struct* aa)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	unsigned long* lengths;
	unsigned long len = 0;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT alt_adv FROM character_ WHERE account_id=%i AND name='%s'", account_id, name), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {	
			row = mysql_fetch_row(result);
			lengths = mysql_fetch_lengths(result);
			len = result->lengths[0];
			//if (lengths[0] == sizeof(PlayerAA_Struct)) {
			if(row[0] && lengths[0] >= sizeof(PlayerAA_Struct)) {
				memcpy(aa, row[0], sizeof(PlayerAA_Struct));
			} else { // let's support ghetto-ALTERed databases that don't contain any data in the alt_adv column
				memset(aa, 0, sizeof(PlayerAA_Struct));
				len = sizeof(PlayerAA_Struct);
			}
			//}
			//else {
			//cerr << "Player alternate advancement table length mismatch in GetPlayerAlternateAdv" << endl;
			//mysql_free_result(result);
			//return false;
			//}
		}
		else {
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
		//		unsigned long len=result->lengths[0];
		return len;
	}
	else {
		cerr << "Error in GetPlayerAlternateAdv query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return 0;
	}
	
	//return true;
}

bool Database::SetHackerFlag(const char* accountname, const char* charactername, const char* hacked) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int32	affected_rows = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "INSERT INTO hackers(account,name,hacked) values('%s','%s','%s')", accountname, charactername, hacked), errbuf, 0,&affected_rows)) {
		cerr << "Error in SetHackerFlag query '" << query << "' " << errbuf << endl;
		return false;
	}
	safe_delete_array(query);
	
	if (affected_rows == 0)
	{
		return false;
	}
	
	return true;
}

int32 Database::GetServerFilters(char* name, ServerSideFilters_Struct *ssfs) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;

    MYSQL_ROW row;
	
	
	unsigned long* lengths;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT serverfilters FROM account WHERE name='%s'", name), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {	
			row = mysql_fetch_row(result);
			lengths = mysql_fetch_lengths(result);
			if (lengths[0] == sizeof(ServerSideFilters_Struct)) {
				memcpy(ssfs, row[0], sizeof(ServerSideFilters_Struct));
			}
			else {
				cerr << "Player profile length mismatch in ServerSideFilters" << endl;
				mysql_free_result(result);
				return 0;
			}
		}
		else {
			mysql_free_result(result);
			return 0;

		}
		int32 len = lengths[0];
		mysql_free_result(result);
		return len;
	}
	else {
		cerr << "Error in ServerSideFilters query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return 0;
	}
	
	return 0;
}

bool Database::SetServerFilters(char* name, ServerSideFilters_Struct *ssfs) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[256+sizeof(ServerSideFilters_Struct)*2+1];
	char* end = query;
	
	//if (strlen(name) > 15)
	//	return false;
	
	/*for (int i=0; i<strlen(name); i++)
	{
	if ((name[i] < 'a' || name[i] > 'z') && 
	(name[i] < 'A' || name[i] > 'Z') && 
	(name[i] < '0' || name[i] > '9'))
	return 0;
}*/
	
	
	end += sprintf(end, "UPDATE account SET serverfilters=");
	*end++ = '\'';
    end += DoEscapeString(end, (char*)ssfs, sizeof(ServerSideFilters_Struct));
    *end++ = '\'';
    end += sprintf(end," WHERE name='%s'", name);
	
	int32 affected_rows = 0;
    if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows)) {
        cerr << "Error in SetServerSideFilters query " << errbuf << endl;
		return false;
    }
	
	if (affected_rows == 0) {
		return false;
	}
	
	return true;
}

bool Database::GetFactionIdsForNPC(sint32 nfl_id, LinkedList<struct NPCFaction*> *faction_list, sint32* primary_faction) {
	if (nfl_id <= 0) {
		(*faction_list).Clear();
		if (primary_faction)
			*primary_faction = nfl_id;
		return true;
	}
	const NPCFactionList* nfl = GetNPCFactionList(nfl_id);
	if (!nfl)
		return false;
	if (primary_faction)
		*primary_faction = nfl->primaryfaction;
	(*faction_list).Clear();
	for (int i=0; i<MAX_NPC_FACTIONS; i++) {
		struct NPCFaction *pFac;
		if (nfl->factionid[i]) {
			pFac = new struct NPCFaction;
			pFac->factionID = nfl->factionid[i];
			pFac->value_mod = nfl->factionvalue[i];
			if (nfl->primaryfaction == pFac->factionID)
				pFac->primary = true;
			else
				pFac->primary = false;
			faction_list->Insert(pFac);
		}
	}
	return true;
/*	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result; 
	MYSQL_ROW row; 
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT faction_id, value, primary_faction FROM npc_faction WHERE npc_id=%d", npc_id), errbuf, &result)) 
	{ 
		delete [] query; 
		
		while ((row = mysql_fetch_row(result))) 
		{ 
			struct NPCFaction *pFac; 
			
			pFac = new struct NPCFaction; 
			pFac->factionID = atoi(row[0]); 
			pFac->value_mod = atoi(row[1]); 
			if (atoi(row[2]) == 1) 
				pFac->primary = true; 
			else 
				pFac->primary = false; 
			faction_list->Insert(pFac); 
		} 
		mysql_free_result(result);
		return true; 
	} 
	else 
	{ 
		cerr << "Error in Database::GetFactionIdsForNPC query '" << query << "' " << errbuf << endl; 
		safe_delete_array(query); 
	} 
	return false; */
}

float Database::GetSafePoint(const char* short_name, const char* which) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	//	int buf_len = 256;
	//    int chars = -1;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT safe_%s FROM zone WHERE short_name='%s'", which, short_name), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			float ret = atof(row[0]);
			mysql_free_result(result);
			return ret;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetSafePoint query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	return 0;
}

//New functions for timezone
int32 Database::GetZoneTZ(int32 zoneid) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT timezone FROM zone WHERE zoneidnumber=%i", zoneid), errbuf, &result))
	{
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int32 tmp = atoi(row[0]);
			mysql_free_result(result);
			return tmp;
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetZoneTZ query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	return 0;
}

bool Database::SetZoneTZ(int32 zoneid, int32 tz) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	if (RunQuery(query, MakeAnyLenString(&query, "UPDATE zone SET timezone=%i WHERE zoneidnumber=%i", tz, zoneid), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);

		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else {
		cerr << "Error in SetZoneTZ query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return false;
}
//End new timezone functions.


//New Class for bug tracking connection.  I felt it was safer to use a new class, with less methods available to connect
//to the shared database.  Also, this limits the possibilty of something accidently mixing up the connections by keeping
//it completely seperate.

#ifdef BUGTRACK
BugDatabase::BugDatabase()
{
#ifdef EQDEBUG
	cout << "Creating BugDatabase object!" << endl;
#endif
	mysql_init(&mysqlbug);
	if (!mysql_real_connect(&mysqlbug, buguploadhost, buguploaduser, buguploadpassword,buguploaddatabase,0,NULL,0))
	{
	cerr << "Failed to connect to database: Error: " << mysql_error(&mysqlbug) << endl;
	}
	else
	{
	cout << "Connected!" << endl;
 	}

}

bool BugDatabase::RunQuery(const char* query, int32 querylen, char* errbuf, MYSQL_RES** result, int32* affected_rows, int32* errnum, bool retry) {
	if (errnum)
		*errnum = 0;
	if (errbuf)
		errbuf[0] = 0;
	bool ret = false;
	LockMutex lock(&MBugDatabase);
	if (mysql_real_query(&mysqlbug, query, querylen)) {
		if (mysql_errno(&mysqlbug) == CR_SERVER_LOST) {
			if (retry) {
				cout << "Database Error: Lost connection, attempting to recover...." << endl;
				ret = RunQuery(query, querylen, errbuf, result, affected_rows, errnum, false);
			}
			else {
				if (errnum)
					*errnum = mysql_errno(&mysqlbug);
				if (errbuf)
					snprintf(errbuf, MYSQL_ERRMSG_SIZE, "#%i: %s", mysql_errno(&mysqlbug), mysql_error(&mysqlbug));
				cout << "DB Query Error #" << mysql_errno(&mysqlbug) << ": " << mysql_error(&mysqlbug) << endl;
				ret = false;
			}
		}
		else {
			if (errnum)
				*errnum = mysql_errno(&mysqlbug);
			if (errbuf)
				snprintf(errbuf, MYSQL_ERRMSG_SIZE, "#%i: %s", mysql_errno(&mysqlbug), mysql_error(&mysqlbug));
#ifdef EQDEBUG
			cout << "DB Query Error #" << mysql_errno(&mysqlbug) << ": " << mysql_error(&mysqlbug) << endl;
#endif
			ret = false;
		}
	}
	else {
		if (affected_rows) {
			*affected_rows = mysql_affected_rows(&mysqlbug);
		}
		if (result && mysql_field_count(&mysqlbug)) {
			*result = mysql_store_result(&mysqlbug);
			if (*result) {
				ret = true;
			}
			else {
#ifdef EQDEBUG
				cout << "DB Query Error: No Result" << endl;
#endif
				if (errnum)
					*errnum = UINT_MAX;
				if (errbuf)
					strcpy(errbuf, "Database::RunQuery: No Result");
				ret = false;
			}
		}
		else {
			ret = true;
		}
	}
	return ret;
}


bool BugDatabase::UploadBug(const char* bugdetails, const char* version,const char* loginname) {
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query=0;
	//char* end=0;

        MYSQL_RES *result;
	//MYSQL_ROW row;
	
#ifdef EQDEBUG
	cout << "Version is : " << version << endl;
	cout << "Loginname is : " << loginname << endl;
#endif

	if (RunQuery(query, MakeAnyLenString(&query, "insert into eqemubugs set loginname='%s', version='%s', bugtxt='%s'", loginname, version, bugdetails), errbuf, &result))
	{
	safe_delete_array(query);
	mysql_free_result(result);
	//mysql_close(conn);
	cout << "Successful bug report" << endl;
	return true;
	}
	else
	{
	safe_delete_array(query);
	//mysql_close(conn);
	cout << "Bug Report submission failed." << endl;
	return false;
	}
	return true;
}

int32 BugDatabase::DoEscapeString(char* tobuf, const char* frombuf, int32 fromlen) {
	LockMutex lock(&MBugDatabase);
	return mysql_real_escape_string(&mysqlbug, tobuf, frombuf, fromlen);
}

BugDatabase::~BugDatabase()
{
#ifdef EQDEBUG	
	cout << "Destroying BugDatabase object" << endl;
#endif
        mysql_close(&mysqlbug);
}

#endif

bool Database::InsertNewsPost(int8 type,char* logone,char* logtwo,int32 levelone,int32 leveltwo) {
	//Need to recreate this
	return true;
}

sint32 Database::DeleteStalePlayerCorpses() {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;

	// 604800 seconds = 1 week
	if (!RunQuery(query, MakeAnyLenString(&query, "Delete from player_corpses where (UNIX_TIMESTAMP() - UNIX_TIMESTAMP(timeofdeath)) > 604800 and not timeofdeath=0"), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		return -1;
	}
	safe_delete_array(query);
	
	return affected_rows;
}

sint32 Database::DeleteStalePlayerBackups() {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;

	// 1209600 seconds = 2 weeks
	if (!RunQuery(query, MakeAnyLenString(&query, "Delete from player_corpses where (UNIX_TIMESTAMP() - UNIX_TIMESTAMP(timeofdeath)) > 1209600"), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		return -1;
	}
	safe_delete_array(query);
	
	return affected_rows;
}

sint32 Database::GetNPCFactionListsCount(int32* oMaxID) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(id), count(*) FROM npc_faction");
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete_array(query);
		row = mysql_fetch_row(result);
		if (row != NULL && row[1] != 0) {
			sint32 ret = atoi(row[1]);
			if (oMaxID) {
				if (row[0])
					*oMaxID = atoi(row[0]);
				else
					*oMaxID = 0;
			}
			mysql_free_result(result);
			return ret;
		}
	}
	else {
		cerr << "Error in GetNPCFactionListsCount query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return -1;
	}
	
	return -1;
}

#ifdef SHAREMEM
extern "C" bool extDBLoadNPCFactionLists(sint32 iNPCFactionListCount, int32 iMaxNPCFactionListID) { return database.DBLoadNPCFactionLists(iNPCFactionListCount, iMaxNPCFactionListID); }
const NPCFactionList* Database::GetNPCFactionList(uint32 id) {
	return EMuShareMemDLL.NPCFactionList.GetNPCFactionList(id);
}

bool Database::LoadNPCFactionLists() {
	if (!EMuShareMemDLL.Load())
		return false;
	sint32 tmp = -1;
	int32 tmp_npcfactionlist_max;
	tmp = GetNPCFactionListsCount(&tmp_npcfactionlist_max);
	if (tmp < 0) {
		cout << "Error: Database::LoadNPCFactionLists-ShareMem: GetNPCFactionListsCount() returned < 0" << endl;
		return false;
	}
	npcfactionlist_max = tmp_npcfactionlist_max;
	bool ret = EMuShareMemDLL.NPCFactionList.DLLLoadNPCFactionLists(&extDBLoadNPCFactionLists, sizeof(NPCFactionList), &tmp, &npcfactionlist_max, MAX_NPC_FACTIONS);
	return ret;
}

bool Database::DBLoadNPCFactionLists(sint32 iNPCFactionListCount, int32 iMaxNPCFactionListID) {
	LogFile->write(EQEMuLog::Status, "Loading NPC Faction Lists from database...");
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(id), Count(*) FROM npc_faction");
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete_array(query);
		row = mysql_fetch_row(result);
		if (row && row[0]) {
			if ((int32)atoi(row[0]) > iMaxNPCFactionListID) {
				cout << "Error: Insufficient shared memory to load NPC Faction Lists." << endl;
				cout << "Max(id): " << atoi(row[0]) << ", iMaxNPCFactionListID: " << iMaxNPCFactionListID << endl;
				cout << "Fix this by increasing the MMF_MAX_NPCFactionList_ID define statement" << endl;
				mysql_free_result(result);
				return false;
			}
			if (atoi(row[1]) != iNPCFactionListCount) {
				cout << "Error: number of NPCFactionLists in memshare doesnt match database." << endl;
				cout << "Count(*): " << atoi(row[1]) << ", iNPCFactionListCount: " << iNPCFactionListCount << endl;
				mysql_free_result(result);
				return false;
			}
			npcfactionlist_max = atoi(row[0]);
			mysql_free_result(result);
			NPCFactionList tmpnfl;
			if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, primaryfaction from npc_faction"), errbuf, &result)) {
				safe_delete_array(query);
				while((row = mysql_fetch_row(result))) {
					memset(&tmpnfl, 0, sizeof(NPCFactionList));
					tmpnfl.id = atoi(row[0]);
					tmpnfl.primaryfaction = atoi(row[1]);
					if (!EMuShareMemDLL.NPCFactionList.cbAddNPCFactionList(tmpnfl.id, &tmpnfl)) {
						mysql_free_result(result);
						cout << "Error: Database::DBLoadNPCFactionLists: !EMuShareMemDLL.NPCFactionList.cbAddNPCFactionList" << endl;
						return false;
					}

					Sleep(0);
				}
				mysql_free_result(result);
			}
			else {
				cerr << "Error in DBLoadNPCFactionLists query2 '" << query << "' " << errbuf << endl;
				safe_delete_array(query);
				return false;
			}
			if (RunQuery(query, MakeAnyLenString(&query, "SELECT npc_faction_id, faction_id, value FROM npc_faction_entries order by npc_faction_id"), errbuf, &result)) {
				safe_delete_array(query);
				sint8 i = 0;
				int32 curflid = 0;
				int32 tmpflid = 0;
				uint32 tmpfactionid[MAX_NPC_FACTIONS];
				memset(tmpfactionid, 0, sizeof(tmpfactionid));
				sint32 tmpfactionvalue[MAX_NPC_FACTIONS];

				memset(tmpfactionvalue, 0, sizeof(tmpfactionvalue));
				while((row = mysql_fetch_row(result))) {
					tmpflid = atoi(row[0]);
					if (curflid != tmpflid && curflid != 0) {
						if (!EMuShareMemDLL.NPCFactionList.cbSetFaction(curflid, tmpfactionid, tmpfactionvalue)) {
							mysql_free_result(result);
							cout << "Error: Database::DBLoadNPCFactionLists: !EMuShareMemDLL.NPCFactionList.cbSetFaction" << endl;
							return false;
						}
						memset(tmpfactionid, 0, sizeof(tmpfactionid));
						memset(tmpfactionvalue, 0, sizeof(tmpfactionvalue));
						i = 0;
					}
					curflid = tmpflid;
					tmpfactionid[i] = atoi(row[1]);
					tmpfactionvalue[i] = atoi(row[2]);
					i++;
					if (i >= MAX_NPC_FACTIONS) {
						cerr << "Error in DBLoadNPCFactionLists: More than MAX_NPC_FACTIONS factions returned, flid=" << tmpflid << endl;
						i--;
					}
					Sleep(0);
				}
				if (tmpflid) {
					EMuShareMemDLL.NPCFactionList.cbSetFaction(curflid, tmpfactionid, tmpfactionvalue);
				}

				mysql_free_result(result);
			}
			else {
				cerr << "Error in DBLoadNPCFactionLists query3 '" << query << "' " << errbuf << endl;
				safe_delete_array(query);
				return false;
			}
		}
		else {
			mysql_free_result(result);
			//return false;
		}
	}
	else {
		cerr << "Error in DBLoadNPCFactionLists query1 '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	return true;
}
#else
const NPCFactionList* Database::GetNPCFactionList(uint32 id) {
	if (id <= npcfactionlist_max && npcfactionlist_array)
		return npcfactionlist_array[id];
	return 0;
}

bool Database::LoadNPCFactionLists() {
	LogFile->write(EQEMuLog::Status, "Loading NPC Faction Lists from database...");
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	int i;
	query = new char[256];
	strcpy(query, "SELECT MAX(id), Count(*) FROM npc_faction");
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete_array(query);
		row = mysql_fetch_row(result);
		if (row && row[0]) {
			npcfactionlist_max = atoi(row[0]);
			mysql_free_result(result);
			npcfactionlist_array = new NPCFactionList*[npcfactionlist_max+1];
			for (i=0; i<=npcfactionlist_max; i++)
				npcfactionlist_array[i] = 0;
			if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, primaryfaction from npc_faction"), errbuf, &result)) {
				safe_delete_array(query);
				while((row = mysql_fetch_row(result))) {
					i = atoi(row[0]);
					npcfactionlist_array[i] = new NPCFactionList;
					memset(npcfactionlist_array[i], 0, sizeof(NPCFactionList));
					npcfactionlist_array[i]->id = atoi(row[0]);
					npcfactionlist_array[i]->primaryfaction = atoi(row[1]);
					Sleep(0);
				}
				mysql_free_result(result);
			}
			else {
				cerr << "Error in LoadNPCFactionLists query2 '" << query << "' " << errbuf << endl;
				safe_delete_array(query);
				return false;
			}
			if (RunQuery(query, MakeAnyLenString(&query, "SELECT npc_faction_id, faction_id, value FROM npc_faction_entries order by npc_faction_id"), errbuf, &result)) {
				safe_delete_array(query);
				int32 curflid = 0;
				int32 tmpflid = 0;
				while((row = mysql_fetch_row(result))) {
					tmpflid = atoi(row[0]);
					if (curflid != tmpflid && curflid != 0) {
						i = 0;
					}
					curflid = tmpflid;
					if (npcfactionlist_array[tmpflid]) {
						npcfactionlist_array[tmpflid]->factionid[i] = atoi(row[1]);
						npcfactionlist_array[tmpflid]->factionvalue[i] = atoi(row[2]);
					}
					i++;
					if (i >= MAX_NPC_FACTIONS) {
						cerr << "Error in DBLoadNPCFactionLists: More than MAX_NPC_FACTIONS factions returned, flid=" << tmpflid << endl;
						i--;
					}
					Sleep(0);
				}
				mysql_free_result(result);
			}			else {
				cerr << "Error in LoadNPCFactionLists query3 '" << query << "' " << errbuf << endl;
				safe_delete_array(query);
				return false;
			}
		}
		else {
			mysql_free_result(result);
			return false;
		}
	}
	else {
		cerr << "Error in LoadNPCFactionLists query1 '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	return true;
}
#endif

//Functions for weather
int8 Database::GetZoneW(int32 zoneid) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT weather FROM zone WHERE zoneidnumber=%i", zoneid), errbuf, &result))
	{
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int8 tmp = atoi(row[0]);
			mysql_free_result(result);
			return tmp;

		}
		mysql_free_result(result);
	}

	else {
		cerr << "Error in GetZoneW query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	return 0;
}

bool Database::SetZoneW(int32 zoneid, int8 w) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 affected_rows = 0;
	
	if (RunQuery(query, MakeAnyLenString(&query, "UPDATE zone SET weather=%i WHERE zoneidnumber=%i", w, zoneid), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		if (affected_rows == 1)
			return true;
		else
			return false;
	}
	else {
		cerr << "Error in SetZoneW query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	return false;
}
//End weather functions.

const char* Database::GetItemLink(const Item_Struct* item) {
	static char ret[250];
	if (item) {
		if (item->ItemNumber >= 10000)
			snprintf(ret, sizeof(ret), "%c00%i%s%c", 0x12, item->ItemNumber, item->Name, 0x12);
		else
			snprintf(ret, sizeof(ret), "%c00%i %s%c", 0x12, item->ItemNumber, item->Name, 0x12);
	}
	else {
		ret[0] = 0;
	}
	return ret;
}

// FIXME this can go into shared mem
int16 Database::GetTrainlevel(int16 eqclass, int8 skill_id) {
	// Returns the level eqclass gets skill_id
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT skill_%i FROM class_skill WHERE class=%i", skill_id, eqclass), errbuf, &result))
	{
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int8 tmp = atoi(row[0]);
			mysql_free_result(result);
			if (tmp >= 67) {
				LogFile->write(EQEMuLog::Error, "Database Error invalid skill entry:%i for class:%i in class_skill Table", skill_id, eqclass);
				tmp = 66;
			}
			return tmp;
		}
		mysql_free_result(result);
	}
	else {
		LogFile->write(EQEMuLog::Error, "Database Warning could not find skill entry:%i for class:%i in class_skill Table", skill_id, eqclass);
		safe_delete_array(query);
	}
	return 66; // Aka never
}

bool Database::InjectToRaw(){
#ifndef SHAREMEM
	cout<<"Starting"<<endl;
//
    if (1){
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	query = new char[256];
	strcpy(query, "SELECT MAX(id) FROM items");
	
	
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete_array(query);
		row = mysql_fetch_row(result);
		if (row && row[0]) { 
			max_item = atoi(row[0]);
			item_array = new Item_Struct*[max_item+1];
			for(unsigned int i=0; i<max_item; i++)
			{

				item_array[i] = 0;
			}
			mysql_free_result(result);
			
			MakeAnyLenString(&query, "SELECT id,raw_data_last FROM items");
			
			if (RunQuery(query, strlen(query), errbuf, &result))
			{
				safe_delete_array(query);
				while((row = mysql_fetch_row(result)))
				{
					unsigned long* lengths;
					lengths = mysql_fetch_lengths(result);
					if (lengths[1] == sizeof(OLDItem_Struct))
					{
						//item_array[atoi(row[0])] = new OLDItem_Struct;
						//memcpy(item_array[atoi(row[0])], row[1], sizeof(OLDItem_Struct));
					}
					else
					{
					    //cout<<"Fuck:"<<lengths[1]<<":"<<sizeof(OLDItem_Struct)<<endl;
						// TODO: Invalid item length in database
						item_array[atoi(row[0])] = new Item_Struct;
						memset(item_array, 0x0, sizeof(Item_Struct));
						item_array[atoi(row[0])]->ItemNumber = atoi(row[0]);
				
					}
					Sleep(0);
				}
				mysql_free_result(result);
			}
			else {
				cerr << "Error in LoadItems query '" << query << "' " << errbuf << endl;
				safe_delete_array(query);
				return false;
			}
		}
		else {
			mysql_free_result(result);
		}
	}
	else {
		cerr << "Error in LoadItems query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
    }
	
	uint32 max_item = 0;
	GetItemsCount(&max_item);
	//Item_Struct* u_item_array[max_item+1];
	for (uint32 i = 0; i < max_item; i++){
		//(const Item_Struct*) u_item_array[i] = GetItem(i);
	}
	
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	query = new char[256];
// Inject Field data
	cout<<"Done filling memmory: x=0 max_item="<<max_item<<endl;
	int count = 0;
	for (unsigned int x = 0; x < max_item; x++){
			if(!item_array[x]) {
			    continue;
			}
			count++;
			// Retrieve field data for current item
#if 0
MakeAnyLenString(&query, "SELECT RecLevel,ReqLevel,SkillModId,SkillModPercent,ElemDmgType,ElemDmg,BaneDMGBody,BaneDMGRace,BaneDMG FROM items WHERE ID=%i", item_array[x]->ItemNumber);
			if (RunQuery(query, strlen(query), errbuf, &result))
			{
				safe_delete_array(query);
				while((row = mysql_fetch_row(result)))
				{
				    item_array[x]->common.RecLevel = atoi(row[0]);
				    item_array[x]->common.ReqLevel = atoi(row[1]);
				    item_array[x]->Common.SkillModId = atoi(row[2]);
				    item_array[x]->Common.SkillModPercent = atoi(row[3]);
				    item_array[x]->common.ElemDmgType = atoi(row[4]);
				    item_array[x]->common.ElemDmg = atoi(row[5]);
				    item_array[x]->common.BaneDMGBody = atoi(row[6]);
				    item_array[x]->common.BaneDMGRace = atoi(row[7]);
				    item_array[x]->common.BaneDMG = atoi(row[8]);
				    for (int cur = 0; cur < 10; cur++){
					item_array[x]->common.unknown0282[cur] = 0;
				    }
				    for (int cur = 0; cur < 16; cur++){
					item_array[x]->common.unknown0296[cur] = 0;
				    }
				    for (int cur = 0; cur < 3; cur++){
					item_array[x]->common.unknown0316[cur] = 0;
				    }
				    
				    for (int cur = 0; cur < 2; cur++){
					item_array[x]->common.unknown0325[cur] = 0;
				    }
				    for (int cur = 0; cur < 22; cur++){
					item_array[x]->common.unknown0330[cur] = 0;
				    }
				    for (int cur = 0; cur < 5; cur++){
					item_array[x]->common.unknown0352[cur] = 0;
				    }
#endif
			// Retrieve field data for current item
			MakeAnyLenString(&query, "SELECT * FROM items WHERE ID=%i", item_array[x]->ItemNumber);
			if (RunQuery(query, strlen(query), errbuf, &result))
			{
				safe_delete_array(query);
				while((row = mysql_fetch_row(result)))
  				{
  					// Normal
  					strncpy(item_array[x]->Name, row[2], sizeof(item_array)-1);
 					// if ((atoi(row[4])) != 0){
 					//item_array[x]->LoreName = "*";
  					//strncpy(item_array[x]->lore[1], row[?], sizeof(item_array)-1);
 					//}
  					//sprintf(item_array[x]->idfile, "IT%s", row[?]);
  					//item_array[x]->flag = atoi(row[?]);
 					item_array[x]->Weight = atoi(row[44]);
  					//item_array[x]->nosave = atoi(row[?]);
  					if ((atoi(row[8])) != 0){
 					item_array[x]->NoDrop = 0;
  					}
  					else
 					item_array[x]->NoDrop = -1;
 					item_array[x]->Size = atoi(row[46]);
 					//item_array[x]->type = atoi(row[?]);
  					item_array[x]->ItemNumber = atoi(row[0]);
 					item_array[x]->IconNumber = atoi(row[50]);
 					item_array[x]->EquipSlots = atoi(row[9]);
 					if (item_array[x]->Cost == 0)
 					item_array[x]->Cost = 100000000;
  					// .common
	  				
  					//
 					item_array[x]->Common.EffectType = atoi(row[40]);
 					item_array[x]->Common.SpellId = atoi(row[41]);
 					item_array[x]->Common.CastTime = atoi(row[43]);
 					item_array[x]->Common.ModSkill = atoi(row[16]);
 					item_array[x]->Common.SkillModValue = atoi(row[17]);
 					item_array[x]->Common.BaneDmgRace = atoi(row[21]);
 					//item_array[x]->Common.BaneDmgBody = atoi(row[20]);
 					item_array[x]->Common.BaneDmgAmt = atoi(row[22]);
 					item_array[x]->Common.RecommendedLevel = atoi(row[37]);
 					//item_array[x]->Common.RecSkill = atoi(row[38]);
 					//item_array[x]->Common.ElemDmgType = atoi(row[18]);
 					//item_array[x]->Common.ElemDmg = atoi(row[19]);
 					item_array[x]->Common.RequiredLevel = atoi(row[39]);
 					item_array[x]->Common.FocusId = atoi(row[42]);
	 				
  					// Zero unknowns in items we could just mem set ect
  					for (int cur = 0; cur < 10; cur++){
 					//item_array[x]->Common.unknown0282[cur] = 0;
  					}
  					for (int cur = 0; cur < 16; cur++){
 					//item_array[x]->Common.unknown0296[cur] = 0;
  					}
  					for (int cur = 0; cur < 3; cur++){
 					//item_array[x]->Common.unknown0316[cur] = 0;
					}
	  				
  					for (int cur = 0; cur < 2; cur++){
 					//item_array[x]->Common.unknown0325[cur] = 0;
  					}
  					for (int cur = 0; cur < 22; cur++){
 					//item_array[x]->Common.unknown0330[cur] = 0;
  					}
  					for (int cur = 0; cur < 5; cur++){
 					//item_array[x]->Common.unknown0352[cur] = 0;
  					}
				}
  				mysql_free_result(result);
				// Put Items back into the DB
				// Insert the modified item back into the database
				char tQuery[sizeof(Item_Struct)*2+1 + 64];
				char tEscapedText[sizeof(Item_Struct)*2+1];
				//	long AffectedRows;
				
				if(DoEscapeString(tEscapedText, (const char *)item_array[x], sizeof(Item_Struct))) {
					sprintf(tQuery, "UPDATE items SET raw_data = '%s' WHERE ID=%d\n", tEscapedText, item_array[x]->ItemNumber);
					if(RunQuery(tQuery, strlen(tQuery), errbuf))
					{
					}
					else {
						cout<<"Save failed::"<<tQuery<<endl;
					}
				}
				else{
					cout<<"Escapestrign failed"<<endl;
				}
			}
			else {
				cout<<"Query failed on saveing"<<endl;
			}
	}
	cout<<"Loaded "<<count<<endl;
#endif // Sharemem
return true;}

bool Database::GetStartZone(PlayerProfile_Struct* in_pp, CharCreate_Struct* in_cc)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row = 0;
	int rows;

	if(!in_pp || !in_cc)
		return false;

	in_pp->x = in_pp->y = in_pp->z = in_pp->zone_id = 0;
	in_pp->bind_x[0] = in_pp->bind_y[0] = in_pp->bind_z[0] = in_pp->bind_zone_id = 0;

	RunQuery
	(
		query,
		MakeAnyLenString
		(
			&query,
			"SELECT x,y,z,zone_id,bind_id FROM start_zones "
			"WHERE player_choice=%i AND player_class=%i "
			"AND player_deity=%i AND player_race=%i",
			in_cc->start_zone,
			in_cc->class_,
			in_cc->deity,
			in_cc->race
		),
		errbuf,
		&result
	);
	safe_delete_array(query); 

	if((rows = mysql_num_rows(result)) == 1)
		row = mysql_fetch_row(result);
	if(result) mysql_free_result(result);	

	if(row)
	{         
		LogFile->write(EQEMuLog::Status, "Found starting location in start_zones");
		in_pp->x = atoi(row[0]); 
		in_pp->y = atoi(row[1]); 
		in_pp->z = atoi(row[2]); 
		in_pp->zone_id = atoi(row[3]); 
		in_pp->bind_zone_id = atoi(row[4]); 
	} 
	else
	{
		printf("No start_zones entry in database, using defaults\n");
		switch(in_cc->start_zone)
		{
			case 0:
			{
				in_pp->zone_id = 24;	// erudnext
				in_pp->bind_zone_id = 38;	// tox
				break;
			}
			case 1:
			{
				in_pp->zone_id =2;	// qeynos2
				in_pp->bind_zone_id = 2;	// qeynos2
				break;
			}
			case 2:
			{
				in_pp->zone_id =29;	// halas
				in_pp->bind_zone_id = 30;	// everfrost
				break;
			}
			case 3:
			{
				in_pp->zone_id =19;	// rivervale
				in_pp->bind_zone_id = 20;	// kithicor
				break;
			}
			case 4:
			{
				in_pp->zone_id =9;	// freportw
				in_pp->bind_zone_id = 9;	// freportw
				break;
			}
			case 5:
			{
				in_pp->zone_id =40;	// neriaka
				in_pp->bind_zone_id = 25;	// nektulos
				break;
			}
			case 6:
			{
				in_pp->zone_id =52;	// gukta
				in_pp->bind_zone_id = 46;	// innothule
				break;
			}
			case 7:
			{
				in_pp->zone_id =49;	// oggok
				in_pp->bind_zone_id = 47;	// feerrott
				break;
			}
			case 8:
			{
				in_pp->zone_id =60;	// kaladima
				in_pp->bind_zone_id = 68;	// butcher
				break;
			}
			case 9:
			{
				in_pp->zone_id =54;	// gfaydark
				in_pp->bind_zone_id = 54;	// gfaydark
				break;
			}
			case 10:
			{	
				in_pp->zone_id =61;	// felwithea
				in_pp->bind_zone_id = 54;	// gfaydark
				break;
			}
			case 11:
			{	
				in_pp->zone_id =55;	// akanon
				in_pp->bind_zone_id = 56;	// steamfont
				break;
			}
			case 12:
			{	
				in_pp->zone_id =82;	// cabwest
				in_pp->bind_zone_id = 78;	// fieldofbone
				break;
			}
			case 13:
			{
				in_pp->zone_id =155;	// sharvahl
				in_pp->bind_zone_id = 155;	// sharvahl
				break;
			}
		}
	}

	if(in_pp->x == 0 && in_pp->y == 0 && in_pp->z == 0)
		database.GetSafePoints(in_pp->zone_id, &in_pp->x, &in_pp->y, &in_pp->z);

	if(in_pp->bind_x == 0 && in_pp->bind_y == 0 && in_pp->bind_z == 0)
		database.GetSafePoints(in_pp->bind_zone_id, &in_pp->bind_x[0], &in_pp->bind_y[0], &in_pp->bind_z[0]);

	return true;
}

// Quick and dirty TEMPORARY conversion from blob db structure to field based
// You should have new field based table as 'items' and old blob as 'items_blob'
void Database::ConvertItemBlob()
{
	char errbuff[20000] = {0};
	MYSQL_RES* result1;
	MYSQL_ROW row1;
	
	cout << "CONVERTING ITEMS\nRunning query on old items..." << endl;
	
	char query1[] = "select id, raw_data from items_"; // retrieve from blob format
	if (RunQuery(query1, sizeof(query1), errbuff, &result1))
	{
		while ((row1 = mysql_fetch_row(result1)))
		{
			int id = atoi(row1[0]);
			OLDItem_Struct* olditem = (OLDItem_Struct*)row1[1];
			Item_Struct newitem;
			memset(&newitem, 0, sizeof(Item_Struct));
			
			newitem.Cost = olditem->cost;
			newitem.EquipSlots = olditem->equipableSlots;
			newitem.IconNumber = olditem->icon_nr;
			memcpy(newitem.IDFile, olditem->idfile, sizeof(olditem->idfile));
			//newitem.ItemNumber = olditem->item_nr; // doesn't appear to be in sync with 'id'
			newitem.ItemNumber = id;
			memcpy(newitem.LoreName, olditem->lore, sizeof(olditem->lore));
			memcpy(newitem.Name, olditem->name, sizeof(olditem->name));
			newitem.NoDrop = olditem->nodrop;
			newitem.NoRent = olditem->nosave;
			newitem.Size = olditem->size;
			newitem.Weight = olditem->weight;
			
			if (olditem->type == (int)ItemTypeCommon)
			{
				cout << "Common: " << olditem->name << endl;
				newitem.Common.AC = olditem->common.AC;
				newitem.Common.AGI = olditem->common.AGI;
				newitem.Common.BaneDmg = olditem->common.BaneDMG;
				newitem.Common.BaneDmgRace = olditem->common.BaneDMGRace;
				newitem.Common.CastTime = olditem->common.casttime;
				newitem.Common.CHA = olditem->common.CHA;
				//newitem.Common.Charges = olditem->common.charges;
				//newitem.Common.CharmFile = ?
				newitem.Common.Classes = olditem->common.classes;
				newitem.Common.Color = olditem->common.color;
				newitem.Common.Damage = olditem->common.damage;
				//newitem.Common.Deity = olditem->common.?
				newitem.Common.Delay = olditem->common.delay;
				newitem.Common.DEX = olditem->common.DEX;
				newitem.Common.EffectType = olditem->common.effecttype;
				//newitem.Common.EffectTypeOther = olditem->common.effecttype0;
				//newitem.Common.FactionAmt1 = olditem->common.?
				//newitem.Common.FactionAmt2;
				//newitem.Common.FactionAmt3;
				//newitem.Common.FactionAmt4;
				//newitem.Common.FactionMod1;
				//newitem.Common.FactionMod2;
				//newitem.Common.FactionMod3;
				//newitem.Common.FactionMod4;
				newitem.Common.FocusId = olditem->common.focusspellId;
				newitem.Common.HP = olditem->common.HP;
				newitem.Common.INT = olditem->common.INT;
				newitem.hastepercent = olditem->common.level;
				//newitem.Common.LevelOther = olditem->common.level0;
				newitem.Common.Light = olditem->common.light;
				newitem.Common.Magic = olditem->common.magic;
				newitem.Common.Mana = olditem->common.MANA;
				newitem.Common.Material = olditem->common.material;
				newitem.Common.MaxCharges = olditem->common.MaxCharges;
				newitem.Common.SkillModType = olditem->common.skillModId;
				newitem.Common.Races = olditem->common.normal.races;
				newitem.Common.Range = olditem->common.range;
				newitem.Common.RecommendedLevel = olditem->common.RecLevel;
				newitem.Common.RequiredLevel = olditem->common.ReqLevel;
				//newitem.Common.SizeType = olditem->common.?
				newitem.Common.Skill = olditem->common.skill;
				newitem.Common.SkillModValue = olditem->common.skillModPercent;
				newitem.Common.SpellId = olditem->common.spellId;
				//newitem.Common.SpellIdOther = olditem->common.spellId0;
				newitem.Common.STA = olditem->common.STA;
				//newitem.Common.Stackable = olditem->common.normal.stackable;
				if ((olditem->common.normal.stackable) && (newitem.Common.Skill==0))
					newitem.Common.Skill = 17;
				//newitem.Common.StackCount = olditem->common.number;
				newitem.Common.STR = olditem->common.STR;
				newitem.Common.SvCold = olditem->common.CR;
				newitem.Common.SvDisease = olditem->common.DR;
				newitem.Common.SvFire = olditem->common.FR;
				newitem.Common.SvMagic = olditem->common.MR;
				newitem.Common.SvPoison = olditem->common.PR;
				newitem.Common.WIS = olditem->common.WIS;
			}
			else if (olditem->type == (int)ItemTypeBook)
			{
				cout << "Book: " << olditem->name << endl;
				memcpy(newitem.Book.File, olditem->book.file, sizeof(olditem->book.file));
				//newitem.Book.BookType = olditem->book.booktype;
			}
			else if (olditem->type == (int)ItemTypeContainer)
			{
				cout << "Container: " << olditem->name << endl;
				//newitem.Container.Combine = olditem->container.?
				//newitem.Container.Contents = olditem->container.?
				//newitem.Container.Open = olditem->container.?
				//newitem.Container.PackType = olditem->container.?
				newitem.Container.SizeCapacity = olditem->container.sizeCapacity;
				newitem.Container.Slots = olditem->container.numSlots;
				newitem.Container.WeightReduction = olditem->container.weightReduction;
				//newitem.Container.PackType = olditem->container.packType;
				newitem.Container.PackType = 1;
			}
			
			char idfile[20] = {0};
			DoEscapeString(idfile, newitem.IDFile, strlen(newitem.IDFile)+1);
			char lore[200] = {0};
			DoEscapeString(lore, newitem.LoreName, strlen(newitem.LoreName)+1);
			char name[200] = {0};
			DoEscapeString(name, newitem.Name, strlen(newitem.Name)+1);
			char bookfile[600] = {0};
			DoEscapeString(bookfile, newitem.Book.File, strlen(newitem.Book.File)+1);
			
			char* query2 = new char[20000];
			memset(query2, 0, 20000);
			int len = MakeAnyLenString(&query2,
				"insert into items (\n "
					"id,weight,norent,nodrop,size,itemclass,idfile,lore,slots,cost,name,icon,\n "				// common 0->11
					"filename,\n "																				// books 12->13
					"bagtype,bagslots,bagsize,bagwr,\n "														// bags 14->17
					"astr,asta,aagi,adex,awis,aint,acha,pr,mr,dr,fr,cr,skillmodvalue,skillmodtype,\n "			// common 18->31
					"banedmgamt,banedmgrace,magic,hasteproclvl,light,delay,effecttype,range,damage,material,\n "// common 31->41
					"maxcharges,reclevel,factionmod1,factionmod2,factionmod3,\n "								// common 42->49
					"factionmod4,factionamt1,factionamt2,factionamt3,factionamt4,reqlevel,hp,mana,ac,color,\n "	// common 50->59
					"classes,races,spellid,casttime,focusid,itemtype\n"										// common 60->66
				") values (\n "
					"%i, %i, %i, %i, %i, %i, '%s', '%s', %i, %i, '%s', %i,\n "
					"'%s',\n "
					"%i, %i, %i, %i,\n "
					"%i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i,\n "
					"%i, %i, %i, %i, %i, %i, %i, %i, %i, %i,\n "
					"%i, %i, 0, 0, 0,\n "
					"0, 0, 0, 0, 0, %i, %i, %i, %i, %i,\n "
					"%i, %i, %i, %i, %i, %i\n"
				")",
				newitem.ItemNumber, newitem.Weight, newitem.NoRent, newitem.NoDrop, newitem.Size, newitem.ItemClass, idfile, lore, newitem.EquipSlots, newitem.Cost, name, newitem.IconNumber,
				bookfile,
				newitem.Container.PackType, newitem.Container.Slots, newitem.Container.SizeCapacity, newitem.Container.WeightReduction,
				newitem.Common.STR, newitem.Common.STA, newitem.Common.AGI, newitem.Common.DEX, newitem.Common.WIS, newitem.Common.INT,
					newitem.Common.CHA, newitem.Common.SvPoison, newitem.Common.SvMagic, newitem.Common.SvDisease, newitem.Common.SvFire, newitem.Common.SvCold,
					newitem.Common.SkillModValue, newitem.Common.SkillModType,
				newitem.Common.BaneDmg, newitem.Common.BaneDmgRace, newitem.Common.Magic, newitem.hastepercent, newitem.Common.Light,
					newitem.Common.Delay, newitem.Common.EffectType, newitem.Common.Range, newitem.Common.Damage, newitem.Common.Material,
				newitem.Common.MaxCharges, newitem.Common.RecommendedLevel,
				newitem.Common.RequiredLevel, newitem.Common.HP, newitem.Common.Mana, newitem.Common.AC, newitem.Common.Color,
				newitem.Common.Classes, newitem.Common.Races, newitem.Common.SpellId, newitem.Common.CastTime, newitem.Common.FocusId, newitem.Common.Skill
				);
			
			if (!RunQuery(query2, len, errbuff))
			{
				cout << "ERROR:\r\n" << query2 << "\r\n\r\n" << errbuff << endl;
				DebugBreak();
			}
			safe_delete_array(query2);
		}
	}
	else
	{
		cout << "ERROR:\r\n" << query1 << "\r\n\r\n" << errbuff << endl;
		DebugBreak();
	}
}

#ifdef GUILDWARS
bool Database::UpdateZoneOnlineStatus(int32 zoneid,int8 online)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int32	affected_rows = 0;

	if (!RunQuery(query, MakeAnyLenString(&query, "update zsstatistics set status=%i where zoneid=%i",online,zoneid), errbuf, 0, &affected_rows)) {
		safe_delete_array(query);
		return false;
	}
	safe_delete_array(query);
return true;
}
#endif
