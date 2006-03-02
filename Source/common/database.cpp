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
#include <map>

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
#include "EQStream.h"
#include "packet_functions.h"
#include "emu_opcodes.h"
#include "../common/classes.h"
#include "../common/races.h"
#include "../common/files.h"
#include "../common/EQEMuError.h"
#include "../common/Item.h"
#include "../common/packet_dump.h"
#include "../common/EMuShareMem.h"
extern LoadEMuShareMemDLL EMuShareMemDLL;
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
	char host[200], user[200], passwd[200], database[200];
	int32 port=0;
	bool compression = false;
	bool items[6] = {false, false, false, false, false, false};
	
	if(!ReadDBINI(host, user, passwd, database, port, compression, items)) {
		exit(1);
	}
	
	if (!items[0] || !items[1] || !items[2] || !items[3])
	{
		printf ("Incomplete DB.INI file.\n");
		printf ("Read README.TXT!\n");
		exit (1);
	}
	
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

bool Database::FetchRowMap(MYSQL_RES *result, map<string,string> &rowmap)
{
MYSQL_FIELD *fields;
MYSQL_ROW row;
unsigned long num_fields,i;
bool  retval=false;
	rowmap.clear();
	if (result && (num_fields=mysql_num_fields(result)) && (row = mysql_fetch_row(result))!=NULL && (fields = mysql_fetch_fields(result))!=NULL) {
		retval=true;
		for(i=0;i<num_fields;i++) {
			rowmap[fields[i].name]=(row[i] ? row[i] : "");
		}
	}

	return retval;
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
#ifdef LECONVERT
void Database::ConvertDoors(){
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	NewZone_Struct* zone_data = NULL;
	if (database.RunQuery(query, MakeAnyLenString(&query, "SELECT raw,zone from doors_raw"), errbuf, &result)) {
		safe_delete_array(query);
		while((row = mysql_fetch_row(result))) {
			char *query2 = 0;
			char errbuf2[MYSQL_ERRMSG_SIZE];
			Door_Struct* d = (Door_Struct*)row[0];
			char* zone = (char*)row[1];
			if (!database.RunQuery(query2, MakeAnyLenString(&query2, "replace into doors (doorid,zone,name,pos_x,pos_y,pos_z,heading,incline,size,opentype,doorisopen,invert_state,door_param) values(%i,'%s','%s',%f,%f,%f,%f,%i,%i,%i,%i,%i,%i)",d->doorId,zone,d->name,d->xPos,d->yPos,d->zPos,d->heading,d->incline,d->size,d->opentype,d->state_at_spawn,d->invert_state,d->door_param), errbuf)) {
				cerr << "Error in ConvertDoors: " << errbuf2 << endl;
				return;
			}
		}
		mysql_free_result(result);
	}
	else
		LogFile->write(EQEMuLog::Error, "Error in ConvertRawZones query %s: %s", query, errbuf);
	safe_delete_array(query);
}
void Database::ConvertRawZones(){
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	NewZone_Struct* zone_data = NULL;
	if (database.RunQuery(query, MakeAnyLenString(&query, "SELECT name,raw from zones_raw"), errbuf, &result)) {
		safe_delete_array(query);
		while((row = mysql_fetch_row(result))) {	
			int32 zoneid = GetZoneID(row[0]);
			zone_data = (NewZone_Struct*)row[1];
			SaveRawZone(zoneid,zone_data);
		}
		mysql_free_result(result);
	}
	else
		LogFile->write(EQEMuLog::Error, "Error in ConvertRawZones query %s: %s", query, errbuf);
	safe_delete_array(query);
}
void Database::SaveRawZone(int32 zoneid,NewZone_Struct* zd){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int i=0;
	char* unknown425 = new char[500];
	char* unknown425ptr = unknown425;
	char* unknown475 = new char[500];
	char* unknown475ptr = unknown475;
	char* unknown672 = new char[500];
	char* unknown672ptr = unknown672;
	char* unknown_end = new char[500];
	char* unknown_endptr = unknown_end;
	for(i=0;i<sizeof(zd->unknown360);i++){
		if(i==0)
			sprintf(unknown425ptr,"%i",zd->unknown360[i]);
		else
			sprintf(unknown425ptr,"-%i",zd->unknown360[i]);
		unknown425ptr += strlen(unknown425ptr);
	}
	for(i=0;i<sizeof(zd->unknown331);i++){
		if(i==0)
			sprintf(unknown475ptr,"%i",zd->unknown331[i]);
		else
			sprintf(unknown475ptr,"-%i",zd->unknown331[i]);
		unknown475ptr += strlen(unknown475ptr);
	}
	for(i=0;i<sizeof(zd->unknown_end);i++){
		if(i==0)
			sprintf(unknown_endptr,"%i",zd->unknown_end[i]);
		else
			sprintf(unknown_endptr,"-%i",zd->unknown_end[i]);
		unknown_endptr += strlen(unknown_endptr);
	}
	for(i=0;i<sizeof(zd->unknown672);i++){
		if(i==0)
			sprintf(unknown672ptr,"%i",zd->unknown672[i]);
		else
			sprintf(unknown672ptr,"-%i",zd->unknown672[i]);
		unknown672ptr += strlen(unknown672ptr);
	}
	char longname[300]={0};
	mysql_real_escape_string(getMySQL(), longname, zd->zone_long_name, strlen(zd->zone_long_name));
	if (!RunQuery(query, MakeAnyLenString(&query, "replace into converted_zones set time_type=%i,short_name='%s',long_name='%s',underworld=%f,minclip=%f,maxclip=%f,fog_minclip=%f,fog_maxclip=%f,fog_blue=%i,fog_red=%i,fog_green=%i,sky=%i,ztype=%i,zone_exp_multiplier=%f,walkspeed=%f,safe_x=%f,safe_y=%f,safe_z=%f,unknown425='%s',unknown475='%s',unknown672='%s',unknown504=%f, unknown_end='%s',unknown387=%i,zoneidnumber=%i",
		zd->time_type,zd->zone_short_name,longname,zd->underworld,zd->minclip,zd->maxclip,zd->fog_minclip[0],zd->fog_maxclip[0],zd->fog_blue[0],zd->fog_red[0],zd->fog_green[0],zd->sky,zd->ztype,zd->zone_exp_multiplier,zd->walkspeed,zd->safe_x,zd->safe_y,zd->safe_z,unknown425,unknown475,unknown672,zd->unknown440,unknown_end,zd->unknown323,zoneid),errbuf))	{
		LogFile->write(EQEMuLog::Error, "Error in SaveRawZone query %s: %s", query, errbuf);
	}
	safe_delete(unknown425);
	safe_delete(unknown475);
	safe_delete(unknown672);
	safe_delete(unknown_end);
	safe_delete_array(query);
}
#endif

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
	if (!RunQuery(query, MakeAnyLenString(&query, "update zone set underworld=%f,minclip=%f,"
		"maxclip=%f,fog_minclip=%f,fog_maxclip=%f,fog_blue=%i,fog_red=%i,fog_green=%i,sky=%i,"
		"ztype=%i,zone_exp_multiplier=%f,walkspeed=%f,safe_x=%f,safe_y=%f,safe_z=%f "
		"where zoneidnumber=%i",
		zd->underworld,zd->minclip,
		zd->maxclip,zd->fog_minclip[0],zd->fog_maxclip[0],zd->fog_blue[0],zd->fog_red[0],zd->fog_green[0],zd->sky,
		zd->ztype,zd->zone_exp_multiplier,
		0.666 /*zd->walkspeed - currently missing*/
		,zd->safe_x,zd->safe_y,zd->safe_z,
		zoneid),errbuf))	{
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
	if (database.RunQuery(query, MakeAnyLenString(&query, "SELECT ztype,"
		"fog_red,fog_green,fog_blue,fog_minclip,fog_maxclip,"
		"fog_red2,fog_green2,fog_blue2,fog_minclip2,fog_maxclip2,"
		"fog_red3,fog_green3,fog_blue3,fog_minclip3,fog_maxclip3,"
		"fog_red4,fog_green4,fog_blue4,fog_minclip4,fog_maxclip4,"
		"sky,zone_exp_multiplier,safe_x,safe_y,safe_z,underworld,"
		"minclip,maxclip,walkspeed,time_type from zone where zoneidnumber=%i",zoneid), errbuf, &result)) {
		safe_delete_array(query);
		while((row = mysql_fetch_row(result))) {	
			zone_data = new NewZone_Struct();
			int r = 0;
			memset(zone_data,0,sizeof(NewZone_Struct));
			zone_data->ztype=atoi(row[r++]);
			
			for(i=0;i<4;i++){
				zone_data->fog_red[i]=atoi(row[r++]);
				zone_data->fog_green[i]=atoi(row[r++]);
				zone_data->fog_blue[i]=atoi(row[r++]);
				zone_data->fog_minclip[i]=atof(row[r++]);
				zone_data->fog_maxclip[i]=atof(row[r++]);
			}
			
			zone_data->sky=atoi(row[r++]);
			zone_data->zone_exp_multiplier=atof(row[r++]);
			zone_data->safe_x=atof(row[r++]);
			zone_data->safe_y=atof(row[r++]);
			zone_data->safe_z=atof(row[r++]);
			zone_data->underworld=atof(row[r++]);
			zone_data->minclip=atof(row[r++]);
			zone_data->maxclip=atof(row[r++]);
//lost in the current struct
//			zone_data->walkspeed=atof(row[r++]);
			r++;
			
			zone_data->time_type=atoi(row[r++]);
//not in the DB yet:
			zone_data->gravity = 0.4;
		}
		mysql_free_result(result);
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
				if(type[0] != '\0' && maxcommandlevel < 200) {
					//I really dont understadn how this worked before with the
					//'!' in front, but i'll fix it now...
					if(type[0] == '!') {
						snprintf(commands[maxcommandlevel], 200, "%s", type+1);
					} else {
						snprintf(commands[maxcommandlevel], 200, "%s", type);
					}
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
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id, status FROM account WHERE name='%s' AND password is not null and length(password) > 0 and (password='%s' or password=MD5('%s'))", tmpUN, tmpPW, tmpPW), errbuf, &result)) {
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

bool Database::SetHideMe(int32 account_id, int8 hideme)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE account SET hideme = %i where id = %i", hideme, account_id), errbuf)) {
		cerr << "Error in SetGMSpeed query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}
	
	safe_delete_array(query);
	return true;
	
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
bool Database::GetAccountInfoForLogin(int32 account_id, sint16* admin, char* account_name, int32* lsaccountid, int8* gmspeed, bool* revoked,bool* gmhideme) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT status, name, lsaccount_id, gmspeed, revoked, hideme FROM account WHERE id=%i", account_id), errbuf, &result)) {
		safe_delete_array(query);
		bool ret = GetAccountInfoForLogin_result(result, admin, account_name, lsaccountid, gmspeed, revoked,gmhideme);
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

bool Database::GetAccountInfoForLogin_result(MYSQL_RES* result, sint16* admin, char* account_name, int32* lsaccountid, int8* gmspeed, bool* revoked,bool* gmhideme) {
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
		if(gmhideme)
			*gmhideme = atoi(row[5]);
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

/*
	Cofruben:Starting adventure database functions.
*/

AdventureInfo Database::GetAdventureInfo(int32 questid,int32 mobid,int8 advtype){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	AdventureInfo rvalue;
	if(questid>0){
		if (RunQuery(query, MakeAnyLenString(&query, "Select NPCID,Type,Objetive,ObjetiveValue,Text,Minutes,Points,x,y,in_use,ShowCompass,zonedungeonid,zoneid,status,QuestID from adventures where QuestID=%i", questid), errbuf, &result)) {
			if ((row = mysql_fetch_row(result))) {
				safe_delete_array(query);
				rvalue.NPCID=atoi(row[0]);
				rvalue.type=atoi(row[1]);
				rvalue.Objetive=atoi(row[2]);
				rvalue.ObjetiveValue=atoi(row[3]);
				strcpy(rvalue.text,row[4]);
				rvalue.minutes=atoi(row[5]);
				rvalue.points=atoi(row[6]);
				rvalue.x=atof(row[7]);
				rvalue.y=atof(row[8]);
				rvalue.in_use=atoi(row[9]);
				rvalue.ShowCompass=atoi(row[10]);
				rvalue.zonedungeonid=atoi(row[11]);
				rvalue.zoneid=atoi(row[12]);
				rvalue.status=atoi(row[13]);
				rvalue.QuestID=atoi(row[14]);
				mysql_free_result(result);
			}
		}
		else
			printf("Q.GetAdvInfo error: %s\n",errbuf);
	}
	else {
		if (RunQuery(query, MakeAnyLenString(&query, "Select NPCID,Type,Objetive,ObjetiveValue,Text,Minutes,Points,x,y,in_use,ShowCompass,zonedungeonid,zoneid,status,QuestID from adventures where NPCID=%i and type=%i", mobid,advtype), errbuf, &result)) {
			if ((row = mysql_fetch_row(result))) {
				mysql_free_result(result);
				rvalue.NPCID=atoi(row[0]);
				rvalue.type=atoi(row[1]);
				rvalue.Objetive=atoi(row[2]);
				rvalue.ObjetiveValue=atoi(row[3]);
				strcpy(rvalue.text,row[4]);
				rvalue.minutes=atoi(row[5]);
				rvalue.points=atoi(row[6]);
				rvalue.x=atof(row[7]);
				rvalue.y=atof(row[8]);
				rvalue.in_use=atoi(row[9]);
				rvalue.ShowCompass=atoi(row[10]);
				rvalue.zonedungeonid=atoi(row[11]);
				rvalue.zoneid=atoi(row[12]);
				rvalue.status=atoi(row[13]);
				rvalue.QuestID=atoi(row[14]);
				safe_delete_array(query);
			}
			else {
				memset(&rvalue,0,sizeof(rvalue));
				rvalue.in_use=true;
			}
		}
		else
			printf("etAdvInfo error: %s\n",errbuf);
	}
	safe_delete_array(query);
	return rvalue;
}
void Database::SetAdventureInfo(int32 questid,bool inuse,int32 status){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "update adventures set in_use=%i,status=%i where QuestID=%i", inuse,status,questid), errbuf)) {
		printf("Error updating adventures: %s\n",errbuf);
	}
	safe_delete_array(query);
	return;
}
void Database::SetAdventureChar(int32 n,int32 charid,int32 questid){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	if (!RunQuery(query, MakeAnyLenString(&query, "update adventures set char%i=%i where QuestID=%i", n+1,charid,questid), errbuf)) {
		printf("Error updating adventure characters: %s\n",errbuf);
	}
	safe_delete_array(query);
}
int32 Database::GetAdventureChar(int32 n,int32 questid){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "Select char%i from adventures where QuestID=%i",n+1, questid), errbuf, &result)) {
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			safe_delete_array(query);
			mysql_free_result(result);
			return atoi(row[0]);
		}
	}
	safe_delete_array(query);
	return 0;
}
char* Database::GetAdventureNPCText(uint32 NPCID){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "Select Text from adventures_maintext where NPCID=%i",NPCID), errbuf, &result)) {
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			safe_delete_array(query);
			mysql_free_result(result);
			return row[0];	
		}
	}
	safe_delete_array(query);
//	return "Error loading initial text";
	return(NULL);
}

bool Database::IsLDoNDungeon(uint32 zoneid){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "Select ldondungeon from zone where zoneidnumber=%i", zoneid), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int res = atoi(row[0]);
			mysql_free_result(result);
			if(res == 0)
				return false;
			else
				return true;
		}
	}
	return false;
}



//---------------------------------
//End of adventure database code.--
//---------------------------------


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
			////////////
			////////////	This is the current one, the other are for converting
			////////////
			if ((lengths[1] == sizeof(PlayerProfile_Struct))) {
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
							if(strlen(item->GetItem()->IDFile) > 2) {
								int32 idfile=atoi(&item->GetItem()->IDFile[2]);
								if (material==MATERIAL_PRIMARY)
									cs->primary[char_num]=idfile;
								else
									cs->secondary[char_num]=idfile;
							}
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
		"bagidx,itemid,charges,augslot1,augslot2,augslot3,augslot4,augslot5 from object_contents where parentid=%i", parentid);
	
	if (database.RunQuery(query, len_query, errbuf, &result)) {
		while ((row = mysql_fetch_row(result))) {
			uint8 index = (uint8)atoi(row[0]);
			uint32 item_id = (uint32)atoi(row[1]);
			sint8 charges = (sint8)atoi(row[2]);
			uint32 aug[5];
			aug[0]	= (uint32)atoi(row[3]);
			aug[1]	= (uint32)atoi(row[4]);
			aug[2]	= (uint32)atoi(row[5]);
			aug[3]	= (uint32)atoi(row[6]);
			aug[4]	= (uint32)atoi(row[7]);
			
			ItemInst* inst = ItemInst::Create(item_id, charges);
			if (inst) {
				if (inst->GetItem()->ItemClass == ItemClassCommon) {
					ItemCommonInst *ci=(ItemCommonInst *)inst;
					for(int i=0;i<5;i++) {
						if (aug[i]) {
							ItemCommonInst augment(aug[i]);
							if (augment.GetItem()) {
								ci->PutAugment(i,augment);
							} else
								cout << "NULL item!" << endl;
						}
					}
				}
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
		if (inst) {
			uint32 item_id = inst->GetItem()->ID;
			uint32 augslot[5] = { 0, 0, 0, 0, 0 };
			if (inst->IsType(ItemClassCommon)) {
				ItemCommonInst *ic=(ItemCommonInst *)inst;
				for(int i=0;i<5;i++) {
					ItemCommonInst *auginst=ic->GetAugment(i);
					augslot[i]=(auginst && auginst->GetItem()) ? auginst->GetItem()->ID : 0;
				}
			}
			uint32 len_query = MakeAnyLenString(&query,

				"replace into object_contents (zoneid,parentid,bagidx,itemid,charges,augslot1,augslot2,augslot3,augslot4,augslot5,droptime) values (%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,now())",
				zone_id, parent_id, index, item_id, inst->GetCharges(),augslot[0],augslot[1],augslot[2],augslot[3],augslot[4]);
			
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

bool Database::SaveCursor(uint32 char_id, list<ItemInst*>::const_iterator &start, list<ItemInst*>::const_iterator &end)
{
iter_queue it;
int i;
bool ret=true;
	char errbuf[MYSQL_ERRMSG_SIZE];
    	char* query = 0;
	// Delete cursor items
	if ((ret = RunQuery(query, MakeAnyLenString(&query, "DELETE FROM inventory WHERE charid=%i AND ( (slotid >=8000 and slotid<=8999) or slotid=30 or (slotid>=331 and slotid<=340))", char_id), errbuf))) {
		for(it=start,i=8000;it!=end;it++,i++) {
			ItemInst *inst=*it;
			if (!(ret=SaveInventory(char_id,inst,(i==8000) ? 30 : i)))
				break;
		}
	} else {
		cout << "Clearing cursor failed: " << errbuf << endl;
	}
	safe_delete_array(query);

	return ret;
}

bool Database::VerifyInventory(uint32 account_id, sint16 slot_id, const ItemInst* inst)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	// Delete cursor items
	if (!RunQuery(query, MakeAnyLenString(&query, 
		"SELECT itemid,charges FROM sharedbank "
		"WHERE acctid=%d AND slotid=%d",
		account_id, slot_id), errbuf, &result)) {
		LogFile->write(EQEMuLog::Error, "Error runing inventory verification query '%s': %s", query, errbuf);
		safe_delete_array(query);
		//returning true is less harmful in the face of a query error
		return(true);
	}
	safe_delete_array(query);
	
	row = mysql_fetch_row(result);
	bool found = false;
	if(row) {
		uint32 id = atoi(row[0]);
		int16 charges = atoi(row[1]);
		
		int16 expect_charges = 0;
		if(inst->GetCharges() >= 0)
			expect_charges = inst->GetCharges();
		else
			expect_charges = 255;
		
		if(id == inst->GetItem()->ID && charges == expect_charges)
			found = true;
	}
	mysql_free_result(result);
	return(found);
}

bool Database::SaveInventory(uint32 char_id, const ItemInst* inst, sint16 slot_id) {
	_CP(Database_SaveInventory);
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
	bool ret = false;
	uint32 augslot[5] = { 0, 0, 0, 0, 0 };

	//never save tribute slots:
	if(slot_id >= 400 && slot_id <= 404)
		return(true);
	
	if (inst && inst->IsType(ItemClassCommon)) {
		const ItemCommonInst *ic=(const ItemCommonInst *)inst;
		for(int i=0;i<5;i++) {
			ItemCommonInst *auginst=ic->GetAugment(i);
			augslot[i]=(auginst && auginst->GetItem()) ? auginst->GetItem()->ID : 0;
		}
	}
	
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
			int16 charges = 0;
			if(inst->GetCharges() >= 0)
				charges = inst->GetCharges();
			else
				charges = 255;

			uint32 len_query =  MakeAnyLenString(&query, "REPLACE INTO sharedbank (acctid,slotid,itemid,charges,augslot1,augslot2,augslot3,augslot4,augslot5) VALUES(%i,%i,%i,%i,%i,%i,%i,%i,%i)",
				account_id, slot_id, inst->GetItem()->ID, charges ,augslot[0],augslot[1],augslot[2],augslot[3],augslot[4]);

			
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
			int16 charges = 0;
			if(inst->GetCharges() >= 0)
				charges = inst->GetCharges();
			else
				charges = 255;
			// Update/Insert item
			uint32 len_query = MakeAnyLenString(&query, "REPLACE INTO inventory (charid,slotid,itemid,charges,instnodrop,color,augslot1,augslot2,augslot3,augslot4,augslot5) VALUES(%i,%i,%i,%i,%i,%i,%i,%i,%i,%i,%i)",
				char_id, slot_id, inst->GetItem()->ID, charges, inst->IsInstNoDrop() ? 1:0,inst->GetColor(),augslot[0],augslot[1],augslot[2],augslot[3],augslot[4],augslot[5] );
			
			ret = RunQuery(query, len_query, errbuf);
		}
	}
	
	if (!ret)
		LogFile->write(EQEMuLog::Error, "SaveInventory query '%s': %s", query, errbuf);
	safe_delete_array(query);
	
	// Save bag contents, if slot supports bag contents
	if (inst && inst->IsType(ItemClassContainer) && Inventory::SupportsContainers(slot_id)) {
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
	printf(" ptimers");
#endif
	RunQuery(query, MakeAnyLenString(&query, "DELETE from timers WHERE char_id='%d'", charid), errbuf, NULL, &affected_rows);
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
bool Database::StoreCharacter(uint32 account_id, PlayerProfile_Struct* pp, Inventory* inv, ExtendedProfile_Struct *ext)
{
	_CP(Database_StoreCharacter);
	char errbuf[MYSQL_ERRMSG_SIZE];
	char query[256+sizeof(PlayerProfile_Struct)*2+sizeof(ExtendedProfile_Struct)*2+5];
	char* end = query;
	int32 affected_rows = 0;
	int i;
	int32 charid = 0;
	char* charidquery = 0;
	char* invquery = 0;
	MYSQL_RES *result;
	MYSQL_ROW row = 0;
	char zone[50];
	float x, y, z;

//	memset(&playeraa, 0, sizeof(playeraa));

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

	const char *zname = GetZoneName(pp->zone_id);
	if(zname == NULL) {
		//zone not in the DB, something to prevent crash...
		strncpy(zone, "qeynos", 49);
		pp->zone_id = 1;
	} else
		strncpy(zone, zname, 49);
	x=pp->x;
	y=pp->y;
	z=pp->z;

	// construct the character_ query
	end += sprintf(end,
		"UPDATE character_ SET timelaston=0, guild=0, "
		"zonename=\'%s\', x=%f, y=%f, z=%f, profile=\'",
		zone, x, y, z
	);
	end += DoEscapeString(end, (char*)pp, sizeof(PlayerProfile_Struct));
	end += sprintf(end, "\', extprofile=\'");
	end += DoEscapeString(end, (char*)ext, sizeof(ExtendedProfile_Struct));
	end += sprintf(end, "\' WHERE account_id=%d AND name='%s'",account_id, pp->name);
	
	RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows);
	
	if(!affected_rows)
	{
		LogFile->write(EQEMuLog::Error, "StoreCharacter query '%s' %s", query, errbuf);
		return false;
	}

	affected_rows = 0;


	// Doodman: Is this even used?
	// now the inventory
	for (i=22; i<=29; i++)
	{
		const ItemInst* newinv = inv->GetItem((sint16)i);
		if (newinv)
		{
			MakeAnyLenString
			(
				&invquery,
				"INSERT INTO inventory SET "
				"charid=%d, slotid=%d, itemid=%d, charges=%d, color=%d",
				charid, i, newinv->GetItem()->ID, 
				newinv->GetCharges(), newinv->GetColor()
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
//why would you enforce the items to be common... no books or bags? 
//				if(myitem && myitem->ItemClass == ItemClassCommon)
				if(myitem)
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
#ifdef _EQDEBUG
	printf("Set group id on '%s' to %d\n", name, id);
#endif
	safe_delete_array(query);
}


void Database::ClearGroup(int32 gid) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	if(gid == 0) {  //clear all groups
		if (!RunQuery(query, MakeAnyLenString(&query, "update character_ set groupid=0 where groupid!=0"), errbuf))
			printf("Unable to clear groups: %s\n",errbuf);
	} else {	//clear a specific group
		if (!RunQuery(query, MakeAnyLenString(&query, "update character_ set groupid=0 where groupid = %lu", gid), errbuf))
			printf("Unable to clear groups: %s\n",errbuf);
	}
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

bool Database::GetCharacterInfoForLogin(const char* name, uint32* character_id, char* current_zone, PlayerProfile_Struct* pp, Inventory* inv, ExtendedProfile_Struct *ext, uint32* pplen, uint32* guilddbid, int8* guildrank) {
	_CP(Database_GetCharacterInfoForLogin);
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32 querylen;
    MYSQL_RES *result;
	
	bool ret = false;
	
	if (character_id && *character_id) {
		// searching by ID should be a lil bit faster
		querylen = MakeAnyLenString(&query, "SELECT id, profile, zonename, x, y, z, guild, guildrank, extprofile FROM character_ WHERE id=%i", *character_id);
	}
	else {
		querylen = MakeAnyLenString(&query, "SELECT id, profile, zonename, x, y, z, guild, guildrank, extprofile FROM character_ WHERE name='%s'", name);
	}
	
	if (RunQuery(query, querylen, errbuf, &result)) {
		ret = GetCharacterInfoForLogin_result(result, character_id, current_zone, pp, inv, ext, pplen, guilddbid, guildrank);
		mysql_free_result(result);
	}
	else {
		LogFile->write(EQEMuLog::Error, "GetCharacterInfoForLogin query '%s' %s", query, errbuf);
	}
	
	safe_delete_array(query);
	return ret;
}

#define StructDist(in, f1, f2) (uint32(&in->f2)-uint32(&in->f1))

// Process results of GetCharacterInfoForLogin()
// Query this processes: SELECT id,profile,zonename,x,y,z,alt_adv,guild,guildrank FROM character_ WHERE id=%i
bool Database::GetCharacterInfoForLogin_result(MYSQL_RES* result, int32* character_id, char* current_zone, PlayerProfile_Struct* pp, Inventory* inv, ExtendedProfile_Struct *ext, uint32* pplen, uint32* guilddbid, int8* guildrank, bool* gmhideme) {
	_CP(Database_GetCharacterInfoForLogin_result);
	
    MYSQL_ROW row;
	unsigned long* lengths;
	
	if (mysql_num_rows(result) == 1) {
		row = mysql_fetch_row(result);
		lengths = mysql_fetch_lengths(result);
		if (pp && pplen) {
			if (lengths[1] == sizeof(PlayerProfile_Struct)) {
				memcpy(pp, row[1], sizeof(PlayerProfile_Struct));
			} else {
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

		if (guilddbid)
			*guilddbid = atoi(row[6]);
		if (guildrank)
			*guildrank = atoi(row[7]);
		
		if(ext) {
			//SetExtendedProfile handles any conversion
			SetExtendedProfile(ext, row[8], lengths[8]);
		}
		
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
			"SELECT sb.slotid,sb.itemid,sb.charges,sb.augslot1,sb.augslot2,sb.augslot3,sb.augslot4,sb.augslot5 from sharedbank sb "
			"INNER JOIN character_ ch ON ch.account_id=sb.acctid "
			"WHERE ch.id=%i", id);
	}
	else {
		len_query = MakeAnyLenString(&query,
			"SELECT slotid,itemid,charges,augslot1,augslot2,augslot3,augslot4,augslot5 from sharedbank WHERE acctid=%i", id);
	}
	
	if (RunQuery(query, len_query, errbuf, &result)) {
		while ((row = mysql_fetch_row(result))) {
			sint16 slot_id	= (sint16)atoi(row[0]);
			uint32 item_id	= (uint32)atoi(row[1]);
			sint8 charges	= (sint8)atoi(row[2]);
			uint32 aug[5];
			aug[0]	= (uint32)atoi(row[3]);
			aug[1]	= (uint32)atoi(row[4]);
			aug[2]	= (uint32)atoi(row[5]);
			aug[3]	= (uint32)atoi(row[6]);
			aug[4]	= (uint32)atoi(row[7]);
			const Item_Struct* item = GetItem(item_id);
			
			if (item) {
				sint16 put_slot_id = SLOT_INVALID;
				
				if (item->ItemClass == ItemClassCommon) {
					ItemCommonInst common(item, charges);
					for(int i=0;i<5;i++) {
						if (aug[i]) {
							ItemCommonInst augment(aug[i]);
							if (augment.GetItem()) {
								common.PutAugment(i,augment);
							} else
								cout << "NULL item!" << endl;
						}
					}
					put_slot_id = inv->PutItem(slot_id, (ItemInst&)common);
				}
				else if (item->ItemClass == ItemClassContainer) {
					ItemContainerInst bag(item, charges);
					put_slot_id = inv->PutItem(slot_id, (ItemInst&)bag);
				}
				else if (item->ItemClass == ItemClassBook) {
					ItemBookInst book(item, charges);
					put_slot_id = inv->PutItem(slot_id, (ItemInst&)book);
				}
				
				// Save ptr to item in inventory
				if (put_slot_id == SLOT_INVALID) {
					LogFile->write(EQEMuLog::Error,
						"Warning: Invalid slot_id for item in shared bank inventory: %s=%i, item_id=%i, slot_id=%i",
						((is_charid==true) ? "charid" : "acctid"), id, item_id, slot_id);

					if(is_charid)
						SaveInventory(id,NULL,slot_id);
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
	else {
		LogFile->write(EQEMuLog::Error, "Database::GetSharedBank(int32 account_id): %s", errbuf);
	}
	
	safe_delete_array(query);
	return ret;
}

// Get the player profile and inventory for the given account "account_id" and
// character name "name".  Return true if the character was found, otherwise false.
// False will also be returned if there is a database error.
bool Database::GetPlayerProfile(int32 account_id, char* name, PlayerProfile_Struct* pp, Inventory* inv, ExtendedProfile_Struct *ext, char* current_zone) {
	_CP(Database_GetPlayerProfile);
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
    MYSQL_RES* result;
    MYSQL_ROW row;
	bool ret = false;
	
	unsigned long* lengths;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT profile,zonename,x,y,z,extprofile FROM character_ WHERE account_id=%i AND name='%s'", account_id, name), errbuf, &result)) {
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
				
				if(ext) {
					//SetExtendedProfile handles any conversion
					SetExtendedProfile(ext, row[5], lengths[5]);
				}
				
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
	_CP(Database_GetInventory);
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
    MYSQL_RES* result;
    MYSQL_ROW row;
	bool ret = false;
	
	// Retrieve character inventory
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT slotid,itemid,charges,color,augslot1,augslot2,augslot3,augslot4,augslot5,instnodrop FROM inventory WHERE charid=%i ORDER BY slotid", char_id), errbuf, &result)) {

		while ((row = mysql_fetch_row(result))) {	
			sint16 slot_id	= (sint16)atoi(row[0]);
			uint32 item_id	= (uint32)atoi(row[1]);
			int16 charges	= (int16)atoi(row[2]);
			uint32 color		= (uint32)atoi(row[3]);
			uint32 aug[5];
			aug[0]	= (uint32)atoul(row[4]);
			aug[1]	= (uint32)atoul(row[5]);
			aug[2]	= (uint32)atoul(row[6]);
			aug[3]	= (uint32)atoul(row[7]);
			aug[4]	= (uint32)atoul(row[8]);
			bool instnodrop	= (row[9] && (int16)atoi(row[9])) ? true : false;

			const Item_Struct* item = GetItem(item_id);
			
			if (item) {
				sint16 put_slot_id = SLOT_INVALID;
				
				if (item->ItemClass == ItemClassCommon) {
					ItemCommonInst common(item, charges);
					if (instnodrop)
							common.SetInstNoDrop(true);
					if (color > 0)
						common.SetColor(color);
					if(charges==255)
						common.SetCharges(-1);
					else
						common.SetCharges(charges);

					for(int i=0;i<5;i++) {
						if (aug[i]) {
							ItemCommonInst augment(aug[i]);
							if (augment.GetItem()) {
								common.PutAugment(i,augment);
							} else
								cout << "NULL item!" << endl;
						}
					}

					if (slot_id>=8000 && slot_id <= 8999)
						put_slot_id = inv->PushCursor((ItemInst&)common);
					else 
						put_slot_id = inv->PutItem(slot_id, (ItemInst&)common);
				}
				else if (item->ItemClass == ItemClassContainer) {
					ItemContainerInst bag(item, charges);
					if (slot_id>=8000 && slot_id <= 8999)
						put_slot_id = inv->PushCursor((ItemInst&)bag);
					else 
						put_slot_id = inv->PutItem(slot_id, (ItemInst&)bag);
				}
				else if (item->ItemClass == ItemClassBook) {
					ItemBookInst book(item, charges);
					if (slot_id>=8000 && slot_id <= 8999)
						put_slot_id = inv->PushCursor((ItemInst&)book);
					else 
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
	else {
		LogFile->write(EQEMuLog::Error, "GetInventory query '%s' %s", query, errbuf);
		LogFile->write(EQEMuLog::Error, "If you got an error related to the 'instnodrop' field, run the following SQL Queries:\nalter table inventory add instnodrop tinyint(1) unsigned default 0 not null;\n");
	}
	
	safe_delete_array(query);
	return ret;
}

// Overloaded: Retrieve character inventory based on account_id and character name
bool Database::GetInventory(uint32 account_id, char* name, Inventory* inv) {
	_CP(Database_GetInventory_name);
	char errbuf[MYSQL_ERRMSG_SIZE];
    char* query = 0;
    MYSQL_RES* result;
    MYSQL_ROW row;
	bool ret = false;
	
	// Retrieve character inventory
#ifdef WORLD
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT slotid,itemid,charges,color FROM inventory INNER JOIN character_ ch ON ch.id=charid WHERE ch.name='%s' AND ch.account_id=%i AND slotid<22 ORDER BY slotid", name, account_id), errbuf, &result))
#else
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT slotid,itemid,charges,color,augslot1,augslot2,augslot3,augslot4,augslot5,instnodrop FROM inventory INNER JOIN character_ ch ON ch.id=charid WHERE ch.name='%s' AND ch.account_id=%i ORDER BY slotid", name, account_id), errbuf, &result))
#endif
	{
		while ((row = mysql_fetch_row(result))) {
			sint16 slot_id	= (sint16)atoi(row[0]);
			uint32 item_id	= (uint32)atoi(row[1]);
			sint8 charges	= (sint8)atoi(row[2]);
			uint32 color		= (uint32)atoi(row[3]);
#ifndef WORLD
			uint32 aug[5];
			aug[0]	= (uint32)atoi(row[4]);
			aug[1]	= (uint32)atoi(row[5]);
			aug[2]	= (uint32)atoi(row[6]);
			aug[3]	= (uint32)atoi(row[7]);
			aug[4]	= (uint32)atoi(row[8]);
			bool instnodrop	= (row[9] && (int16)atoi(row[9])) ? true : false;
#endif
			const Item_Struct* item = GetItem(item_id);
			sint16 put_slot_id = SLOT_INVALID;
			if(!item)
				continue;
			if (item->ItemClass == ItemClassCommon) {
				ItemCommonInst common(item, charges);
				if (color > 0)
					common.SetColor(color);
				common.SetCharges(charges);
#ifndef WORLD
				for(int i=0;i<5;i++) {
					if (aug[i]) {
						ItemCommonInst augi(aug[i]);
						common.PutAugment(augi,i);
					}
				}
				if (instnodrop)
						common.SetInstNoDrop(true);
#endif
				if (slot_id!=SLOT_CURSOR)
					put_slot_id = inv->PutItem(slot_id, (ItemInst&)common);
				else 
					put_slot_id = inv->PushCursor((ItemInst&)common);
			}
			else if (item->ItemClass == ItemClassContainer) {
				ItemContainerInst bag(item, charges);
				if (slot_id!=SLOT_CURSOR)
					put_slot_id = inv->PutItem(slot_id, (ItemInst&)bag);
				else 
					put_slot_id = inv->PushCursor((ItemInst&)bag);
			}
			else if (item->ItemClass == ItemClassBook) {
				ItemBookInst book(item, charges);
				if (slot_id!=SLOT_CURSOR)
					put_slot_id = inv->PutItem(slot_id, (ItemInst&)book);
				else 
					put_slot_id = inv->PushCursor((ItemInst&)book);
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
	else {
		LogFile->write(EQEMuLog::Error, "GetInventory query '%s' %s", query, errbuf);
		LogFile->write(EQEMuLog::Error, "If you got an error related to the 'instnodrop' field, run the following SQL Queries:\nalter table inventory add instnodrop tinyint(1) unsigned default 0 not null;\n");
	}
	
	safe_delete_array(query);
	return ret;
}

bool Database::SetPlayerProfile(uint32 account_id, uint32 charid, PlayerProfile_Struct* pp, Inventory* inv, ExtendedProfile_Struct *ext, uint32 current_zone) {
	_CP(Database_SetPlayerProfile);
	char errbuf[MYSQL_ERRMSG_SIZE];
	char* query = 0;
	int32 affected_rows = 0;
	bool ret = false;
    
	if (RunQuery(query, SetPlayerProfile_MQ(&query, account_id, charid, pp, inv, ext, current_zone), errbuf, 0, &affected_rows)) {
		ret = (affected_rows != 0);
	}
	
	if (!ret) {
		LogFile->write(EQEMuLog::Error, "SetPlayerProfile query '%s' %s", query, errbuf);
    }
	
	safe_delete_array(query);
	return ret;
}

// Generate SQL for updating player profile/inventory
int32 Database::SetPlayerProfile_MQ(char** query, uint32 account_id, uint32 charid, PlayerProfile_Struct* pp, Inventory* inv, ExtendedProfile_Struct *ext, uint32 current_zone) {
    *query = new char[276 + sizeof(PlayerProfile_Struct)*2 + sizeof(ExtendedProfile_Struct)*2 + 1];
	char* end = *query;
	if (!current_zone)
		current_zone = pp->zone_id;

	if(strlen(pp->name) == 0) // Sanity check in case pp never loaded
		return false;
	
	end += sprintf(end, "UPDATE character_ SET timelaston=unix_timestamp(now()),name=\'%s\', zonename=\'%s\', zoneid=%u, x = %f, y = %f, z = %f, profile=\'", pp->name, GetZoneName(current_zone), current_zone, pp->x, pp->y, pp->z);
	end += DoEscapeString(end, (char*)pp, sizeof(PlayerProfile_Struct));
	end += sprintf(end,"\', extprofile=\'");
	end += DoEscapeString(end, (char*)ext, sizeof(ExtendedProfile_Struct));
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
int32 Database::GetMiniLoginAccount(char* ip){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
	MYSQL_ROW row;
	int32 retid = 0;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT id FROM account WHERE minilogin_ip='%s'", ip), errbuf, &result)) {
		safe_delete_array(query);
		if ((row = mysql_fetch_row(result)))
			retid = atoi(row[0]);
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in GetMiniLoginAccount query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	return retid;
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

// Pyro: Get zone starting points from DB
bool Database::GetSafePoints(const char* short_name, float* safe_x, float* safe_y, float* safe_z, sint16* minstatus, int8* minlevel) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	//	int buf_len = 256;
	//    int chars = -1;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT safe_x, safe_y, safe_z, min_status, min_level FROM zone WHERE short_name='%s'", short_name), errbuf, &result)) {
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
		cerr << "If it errors on minium_level, run the following querys:\n";
		cerr << "ALTER TABLE `zone` CHANGE `minium_level` `min_level` TINYINT(3)  UNSIGNED DEFAULT \"0\" NOT NULL;\n";
		cerr << "ALTER TABLE `zone` CHANGE `minium_status` `min_status` TINYINT(3)  UNSIGNED DEFAULT \"0\" NOT NULL;\n";
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

sint32 Database::GetItemsCount(int32* oMaxID, int32 *oSerializationSize) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
	sint32 ret = -1;
	
	char query[] = "SELECT MAX(id),count(*),sum(length(serialization)+1) FROM items";
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
			if (oSerializationSize) {
				if (row[0])
					*oSerializationSize = atoi(row[2]);
				else
					*oSerializationSize = 0;
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

extern "C" bool extDBLoadItems(sint32 iItemCount, int32 iMaxItemID) { return database.DBLoadItems(iItemCount, iMaxItemID); }
bool Database::LoadItems() {
	if (!EMuShareMemDLL.Load())
		return false;
	sint32 tmp = 0;
	int32 serialization_size=0;
	tmp = GetItemsCount(&max_item,&serialization_size);
	if (tmp == -1) {
		cout << "Error: Database::LoadItems() (sharemem): GetItemsCount() returned -1" << endl;
		return false;
	}
	bool ret = EMuShareMemDLL.Items.DLLLoadItems(&extDBLoadItems, sizeof(Item_Struct), &tmp, &max_item, &serialization_size);
#if defined(GOTFRAGS) || defined(_EQDEBUG)
	if (ret)
		LoadItemStatus();
#endif
	return ret;
}

// Load all database items into cache
bool Database::DBLoadItems(sint32 iItemCount, uint32 iMaxItemID) {
	_CP(Database_DBLoadItems);
	char errbuf[MYSQL_ERRMSG_SIZE];
	MYSQL_RES *result;
	MYSQL_ROW row;
	bool ret = false;
	
	LogFile->write(EQEMuLog::Status, "Loading items from database: count=%i", iItemCount);
	
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
	
	bool disableNoDrop = false;
	char ndbuffer[4];
	if(GetVariable("disablenodrop", ndbuffer, 4)) {
		if(ndbuffer[0] == '1' && ndbuffer[1] == '\0') {
			disableNoDrop = true;
		}
	}
	
	// Retrieve all items from database
	char query[] = "select "
		"serialization,"
#define F(x) #x ","
#include "item_fieldlist.h"
#undef F
		"updated"
		" from items order by id";
	
	Item_Struct item;
	unsigned char  *item_s;
	if (RunQuery(query, sizeof(query), errbuf, &result)) {
                while((row = mysql_fetch_row(result))) {
#if EQDEBUG >= 6
				LogFile->write(EQEMuLog::Status, "Loading %s:%i", row[ItemField::name], row[ItemField::id]);
#endif				
			memset(&item, 0, sizeof(Item_Struct));
			item_s=NULL;
			
			item.ItemClass = (uint8)atoi(row[ItemField::itemclass]);
			strcpy(item.Name,row[ItemField::name]);
			strcpy(item.Lore,row[ItemField::lore]);
			strcpy(item.IDFile,row[ItemField::idfile]);
			item.ID = (uint32)atoul(row[ItemField::id]);
			item.Weight = (uint8)atoi(row[ItemField::weight]);
			item.NoRent = (uint8)atoi(row[ItemField::norent]);
			item.NoDrop = (uint8)atoi(row[ItemField::nodrop]);
			item.Size = (uint8)atoi(row[ItemField::size]);
			item.Slots = (uint32)atoul(row[ItemField::slots]);
			item.Price = (uint32)atoul(row[ItemField::price]);
			item.Icon = (uint32)atoul(row[ItemField::icon]);
			//item.Unk012 = (sint32)atoul(row[ItemField::unk012]);
			//item.Unk013 = (uint32)atoul(row[ItemField::unk013]);
			//item.Unk054 = (uint32)atoul(row[ItemField::unk054]);
			//item.Unk059 = (uint32)atoul(row[ItemField::unk059]);
			//item.Unk061 = (uint32)atoul(row[ItemField::unk061]);
			item.LoreFlag = (atoi(row[ItemField::loreflag])==0) ? false : true;
			item.PendingLoreFlag = (atoi(row[ItemField::pendingloreflag])==0) ? false : true;
			item.ArtifactFlag = (atoi(row[ItemField::artifactflag])==0) ? false : true;
			item.SummonedFlag = (atoi(row[ItemField::summonedflag])==0) ? false : true;
			item.Favor = (uint32)atoul(row[ItemField::favor]);
			item.FVNoDrop = (atoi(row[ItemField::fvnodrop])==0) ? false : true;
			item.GuildFavor = (uint32)atoul(row[ItemField::guildfavor]);
			item.PointType = (uint32)atoul(row[ItemField::pointtype]);
			//item.Unk117 = (uint32)atoul(row[ItemField::unk117]);
			//item.Unk118 = (uint32)atoul(row[ItemField::unk118]);
			//item.Unk121 = (uint32)atoul(row[ItemField::unk121]);
			//item.Unk124 = (uint32)atoul(row[ItemField::unk124]);
			if (item.ItemClass == ItemClassCommon) { // Common
				item.Common.BenefitFlag = (uint32)atoul(row[ItemField::benefitflag]);
				item.Common.Tradeskills = (atoi(row[ItemField::tradeskills])==0) ? false : true;
				item.Common.CR = (sint8)atoi(row[ItemField::cr]);
				item.Common.DR = (sint8)atoi(row[ItemField::dr]);
				item.Common.PR = (sint8)atoi(row[ItemField::pr]);
				item.Common.MR = (sint8)atoi(row[ItemField::mr]);
				item.Common.FR = (sint8)atoi(row[ItemField::fr]);
				item.Common.AStr = (sint8)atoi(row[ItemField::astr]);
				item.Common.ASta = (sint8)atoi(row[ItemField::asta]);
				item.Common.AAgi = (sint8)atoi(row[ItemField::aagi]);
				item.Common.ADex = (sint8)atoi(row[ItemField::adex]);
				item.Common.ACha = (sint8)atoi(row[ItemField::acha]);
				item.Common.AInt = (sint8)atoi(row[ItemField::aint]);
				item.Common.AWis = (sint8)atoi(row[ItemField::awis]);
				item.Common.HP = (sint32)atoul(row[ItemField::hp]);
				item.Common.Mana = (sint32)atoul(row[ItemField::mana]);
				item.Common.AC = (sint32)atoul(row[ItemField::ac]);
				item.Common.Deity = (uint32)atoul(row[ItemField::deity]);
				item.Common.SkillModValue = (sint32)atoul(row[ItemField::skillmodvalue]);
				item.Common.SkillModType = (uint32)atoul(row[ItemField::skillmodtype]);
				item.Common.BaneDmgRace = (uint32)atoul(row[ItemField::banedmgrace]);
				item.Common.BaneDmgAmt = (sint8)atoi(row[ItemField::banedmgbody]);
				item.Common.BaneDmgBody = (uint32)atoul(row[ItemField::banedmgbody]);
				item.Common.Magic = (atoi(row[ItemField::magic])==0) ? false : true;
				item.Common.CastTime_ = (sint32)atoul(row[ItemField::casttime_]);
				item.Common.ReqLevel = (uint8)atoi(row[ItemField::reqlevel]);
				item.Common.BardType = (uint32)atoul(row[ItemField::bardtype]);
				item.Common.BardValue = (sint32)atoul(row[ItemField::bardvalue]);
				item.Common.Light = (sint8)atoi(row[ItemField::light]);
				item.Common.Delay = (uint8)atoi(row[ItemField::delay]);
				item.Common.RecLevel = (uint8)atoi(row[ItemField::reclevel]);
				item.Common.RecSkill = (uint8)atoi(row[ItemField::recskill]);
				item.Common.ElemDmgType = (uint8)atoi(row[ItemField::elemdmgtype]);
				item.Common.ElemDmgAmt = (uint8)atoi(row[ItemField::elemdmgamt]);
				item.Common.Range = (uint8)atoi(row[ItemField::range]);
				item.Common.Damage = (uint8)atoi(row[ItemField::damage]);
				item.Common.Color = (uint32)atoul(row[ItemField::color]);
				item.Common.Classes = (uint32)atoul(row[ItemField::classes]);
				item.Common.Races = (uint32)atoul(row[ItemField::races]);
				item.Common.MaxCharges = (sint16)atoi(row[ItemField::maxcharges]);
				item.Common.ItemType = (uint8)atoi(row[ItemField::itemtype]);
				item.Common.Material = (uint8)atoi(row[ItemField::material]);
				item.Common.SellRate = (float)atof(row[ItemField::sellrate]);
				item.Common.CastTime = (uint32)atoul(row[ItemField::casttime]);
				item.Common.ProcRate = (uint32)atoul(row[ItemField::procrate]);
				item.Common.CombatEffects = (sint8)atoi(row[ItemField::combateffects]);
				item.Common.Shielding = (sint8)atoi(row[ItemField::shielding]);
				item.Common.StunResist = (sint8)atoi(row[ItemField::stunresist]);
				item.Common.StrikeThrough = (sint8)atoi(row[ItemField::strikethrough]);
				item.Common.ExtraDmgSkill = (uint32)atoul(row[ItemField::extradmgskill]);
				item.Common.ExtraDmgAmt = (uint32)atoul(row[ItemField::extradmgamt]);
				item.Common.SpellShield = (sint8)atoi(row[ItemField::spellshield]);
				item.Common.Avoidance = (sint8)atoi(row[ItemField::avoidance]);
				item.Common.Accuracy = (sint8)atoi(row[ItemField::accuracy]);
				item.Common.CharmFileID = (uint32)atoul(row[ItemField::charmfileid]);
				item.Common.FactionMod1 = (sint32)atoul(row[ItemField::factionmod1]);
				item.Common.FactionMod2 = (sint32)atoul(row[ItemField::factionmod2]);
				item.Common.FactionMod3 = (sint32)atoul(row[ItemField::factionmod3]);
				item.Common.FactionMod4 = (sint32)atoul(row[ItemField::factionmod4]);
				item.Common.FactionAmt1 = (sint32)atoul(row[ItemField::factionamt1]);
				item.Common.FactionAmt2 = (sint32)atoul(row[ItemField::factionamt2]);
				item.Common.FactionAmt3 = (sint32)atoul(row[ItemField::factionamt3]);
				item.Common.FactionAmt4 = (sint32)atoul(row[ItemField::factionamt4]);
				strcpy(item.Common.CharmFile,row[ItemField::charmfile]);
				item.Common.AugType = (uint32)atoul(row[ItemField::augtype]);
				item.Common.AugSlotType[0] = (uint8)atoi(row[ItemField::augslot1type]);
				item.Common.AugSlotType[1] = (uint8)atoi(row[ItemField::augslot2type]);
				item.Common.AugSlotType[2] = (uint8)atoi(row[ItemField::augslot3type]);
				item.Common.AugSlotType[3] = (uint8)atoi(row[ItemField::augslot4type]);
				item.Common.AugSlotType[4] = (uint8)atoi(row[ItemField::augslot5type]);
				item.Common.LDoNTheme = (uint32)atoul(row[ItemField::ldontheme]);
				item.Common.LDoNPrice = (uint32)atoul(row[ItemField::ldonprice]);
				item.Common.LDoNSold = (uint32)atoul(row[ItemField::ldonsold]);
				item.Common.BaneDmgRaceAmt = (uint32)atoul(row[ItemField::banedmgraceamt]);
				item.Common.AugRestrict = (uint32)atoul(row[ItemField::augrestrict]);
				item.Common.Endur = (uint32)atoul(row[ItemField::endur]);
				item.Common.DotShielding = (uint32)atoul(row[ItemField::dotshielding]);
				item.Common.Attack = (uint32)atoul(row[ItemField::attack]);
				item.Common.Regen = (uint32)atoul(row[ItemField::regen]);
				item.Common.ManaRegen = (uint32)atoul(row[ItemField::manaregen]);
				item.Common.Haste = (uint32)atoul(row[ItemField::haste]);
				item.Common.DamageShield = (uint32)atoul(row[ItemField::damageshield]);
				item.Common.RecastDelay = (uint32)atoul(row[ItemField::recastdelay]);
				item.Common.RecastType = (uint32)atoul(row[ItemField::recasttype]);
				item.Common.AugDistiller = (uint32)atoul(row[ItemField::augdistiller]);
				item.Common.Attuneable = (atoi(row[ItemField::attuneable])==0) ? false : true;
				item.Common.NoPet = (atoi(row[ItemField::nopet])==0) ? false : true;
				item.Common.PotionBelt = (atoi(row[ItemField::potionbelt])==0) ? false : true;
				item.Common.StackSize = (uint8)atoi(row[ItemField::stacksize]);
				item.Common.Click.Effect = (uint32)atoul(row[ItemField::clickeffect]);
				item.Common.Click.Type = (uint8)atoul(row[ItemField::clicktype]);
				item.Common.Click.Level = (uint8)atoul(row[ItemField::clicklevel]);
				//item.Common.Click.Unk129 = (uint8)atoul(row[ItemField::unk129]);
				item.Common.Proc.Effect = (uint16)atoul(row[ItemField::proceffect]);
				item.Common.Proc.Type = (uint8)atoul(row[ItemField::proctype]);
				item.Common.Proc.Level = (uint8)atoul(row[ItemField::proclevel]);
				//item.Common.Proc.Unk133 = (uint8)atoul(row[ItemField::unk133]);
				item.Common.Worn.Effect = (uint16)atoul(row[ItemField::worneffect]);
				item.Common.Worn.Type = (uint8)atoul(row[ItemField::worntype]);
				item.Common.Worn.Level = (uint8)atoul(row[ItemField::wornlevel]);
				//item.Common.Worn.Unk137 = (uint8)atoul(row[ItemField::unk137]);
				item.Common.Focus.Effect = (uint16)atoul(row[ItemField::focuseffect]);
				item.Common.Focus.Type = (uint8)atoul(row[ItemField::focustype]);
				item.Common.Focus.Level = (uint8)atoul(row[ItemField::focuslevel]);
				//item.Common.Focus.Unk141 = (uint8)atoul(row[ItemField::unk141]);
				item.Common.Scroll.Effect = (uint16)atoul(row[ItemField::scrolleffect]);
				item.Common.Scroll.Type = (uint8)atoul(row[ItemField::scrolltype]);
				item.Common.Scroll.Level = (uint8)atoul(row[ItemField::scrolllevel]);
				//item.Common.Scroll.Unk145 = (uint8)atoul(row[ItemField::unk145]);
			} else if (item.ItemClass == ItemClassContainer) { // Containers
				item.Container.BagType = (uint8)atoi(row[ItemField::bagtype]);
				item.Container.BagSlots = (uint8)atoi(row[ItemField::bagslots]);
				item.Container.BagSize = (uint8)atoi(row[ItemField::bagsize]);
				item.Container.BagWR = (uint8)atoi(row[ItemField::bagwr]);
			} else if (item.ItemClass == ItemClassBook) { // Books
				item.Book.Book = (uint8)atoi(row[ItemField::book]);
				item.Book.BookType = (uint32)atoul(row[ItemField::booktype]);
				strcpy(item.Book.Filename,row[ItemField::filename]);
			}
			
			//handle the serialization
			if(row[ItemField::serialization] && row[ItemField::serialization][0] != '\0') {
				item_s=(unsigned char *)row[ItemField::serialization];
			} else {
				item_s=NULL;
				//TODO: serialize the item
			}

			if (!EMuShareMemDLL.Items.cbAddItem(item.ID, &item, item_s)) {
				LogFile->write(EQEMuLog::Error, "Database::DBLoadItems: Failure reported from EMuShareMemDLL.Items.cbAddItem(%i)", item.ID);
				break;
			}
		}
		
		mysql_free_result(result);
		ret = true;
	}
	else {
		LogFile->write(EQEMuLog::Error, "DBLoadItems query '%s', %s", query, errbuf);
		LogFile->write(EQEMuLog::Error, "If you got boat loads of errors, make sure you sourced: utils/items-0.6.0-DR2-0.6.1-DR1-convert.sql\n");
	}
	return ret;
}

const Item_Struct* Database::GetItem(uint32 id) {
	return EMuShareMemDLL.Items.GetItem(id);
}

const unsigned char * Database::GetItemSerialization(uint32 id) {
	return EMuShareMemDLL.Items.GetItemSerialization(id);
}

const Item_Struct* Database::IterateItems(uint32* NextIndex) {
	return EMuShareMemDLL.Items.IterateItems(NextIndex);
}

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
			MakeAnyLenString(&query, "SELECT id,doorid,zone,name,pos_x,pos_y,pos_z,heading,opentype,guild,lockpick,keyitem,triggerdoor,triggertype,dest_zone,dest_x,dest_y,dest_z,dest_heading,door_param,invert_state,incline,size from doors order by doorid asc");//WHERE zone='%s'", zone_name
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
					tmpDoor.door_param=atoi(row[19]);
					tmpDoor.invert_state=atoi(row[20]);
					tmpDoor.incline=atoi(row[21]);
					tmpDoor.size=atoi(row[22]);
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
				cerr << "If you get an error about door_param and size, run the following queries:\n";
				cerr << "ALTER TABLE `doors` CHANGE `liftheight` `door_param` INT(4)  DEFAULT \"0\" NOT NULL;\n";
				cerr << "ALTER TABLE `doors` ADD `size` SMALLINT(5)  UNSIGNED DEFAULT \"100\" NOT NULL;\n";
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
		char errbuf[MYSQL_ERRMSG_SIZE];
		char *query = 0;
		MYSQL_RES *result;
		MYSQL_ROW row;


      // If id is 0, load all npc_types for the current zone,
      // according to spawn2.
      if (id == 0)
         MakeAnyLenString(&query,
            "SELECT npc_types.id,npc_types.name,npc_types.level,"
            "npc_types.race,npc_types.class,npc_types.hp,"
            "npc_types.gender,npc_types.texture,npc_types.helmtexture,"
            "npc_types.size,npc_types.loottable_id,"
            "npc_types.merchant_id,"
            "npc_types.STR,npc_types.STA,npc_types.DEX,npc_types.AGI,"
            "npc_types._INT,npc_types.WIS,npc_types.CHA,"
            "npc_types.banish,npc_types.mindmg,"
            "npc_types.maxdmg,npc_types.npcspecialattks,"
            "npc_types.npc_spells_id,npc_types.d_meele_texture1,"
            "npc_types.d_meele_texture2,npc_types.walkspeed,"
            "npc_types.runspeed,npc_types.fixedz,"
            "npc_types.hp_regen_rate,npc_types.mana_regen_rate,"
            "npc_types.aggroradius,npc_types.bodytype,"
            "npc_types.npc_faction_id,npc_types.face,"
            "npc_types.luclin_hairstyle,npc_types.luclin_haircolor,"
            "npc_types.luclin_eyecolor,npc_types.luclin_eyecolor2,"
            "npc_types.luclin_beardcolor,npc_types.luclin_beard,"
            "npc_types.see_invis,npc_types.see_invis_undead,"
            "npc_types.lastname,npc_types.qglobal,npc_types.AC,"
            "npc_types.npc_aggro,npc_types.spawn_limit"
            " FROM npc_types,spawn2 WHERE spawn2.zone='%s'"
            " AND npc_types.id=spawn2.id",
            zone->GetShortName());
      // Otherwise, just load this specific NPC.
      else
         MakeAnyLenString(&query,
            "SELECT id,name,level,race,class,hp,gender,"
            "texture,helmtexture,size,loottable_id,merchant_id,"
            "STR,STA,DEX,AGI,_INT,WIS,CHA,"
            "banish,mindmg,maxdmg,npcspecialattks,npc_spells_id,"
            "d_meele_texture1,d_meele_texture2,walkspeed,"
            "runspeed,fixedz,hp_regen_rate,mana_regen_rate,"
            "aggroradius,bodytype,npc_faction_id,face,"
            "luclin_hairstyle,luclin_haircolor,luclin_eyecolor,"
            "luclin_eyecolor2,luclin_beardcolor,luclin_beard,"
            "see_invis,see_invis_undead,lastname,qglobal,AC,npc_aggro,spawn_limit"
            " FROM npc_types WHERE id=%d", id);

		if (RunQuery(query, strlen(query), errbuf, &result)) {
         // Process each row returned.
			while((row = mysql_fetch_row(result))) {
				NPCType *tmpNPCType;
				tmpNPCType = new NPCType;
				memset (tmpNPCType, 0, sizeof *tmpNPCType);
				
				int r = 0;
				tmpNPCType->npc_id = atoi(row[r++]);
				
				strncpy(tmpNPCType->name, row[r++], 30);

				tmpNPCType->level = atoi(row[r++]);
				tmpNPCType->race = atoi(row[r++]);
				tmpNPCType->class_ = atoi(row[r++]);
				tmpNPCType->max_hp = atoi(row[r++]);
				tmpNPCType->cur_hp = tmpNPCType->cur_hp;
				tmpNPCType->gender = atoi(row[r++]);
				tmpNPCType->texture = atoi(row[r++]);
				tmpNPCType->helmtexture = atoi(row[r++]);
				tmpNPCType->size = atof(row[r++]);
				tmpNPCType->loottable_id = atoi(row[r++]);
				tmpNPCType->merchanttype = atoi(row[r++]);
				tmpNPCType->STR = atoi(row[r++]);
				tmpNPCType->STA = atoi(row[r++]);
				tmpNPCType->DEX = atoi(row[r++]);
				tmpNPCType->AGI = atoi(row[r++]);
				tmpNPCType->INT = atoi(row[r++]);
				tmpNPCType->WIS = atoi(row[r++]);
				tmpNPCType->CHA = atoi(row[r++]);
				tmpNPCType->banish = atoi(row[r++]);
				tmpNPCType->min_dmg = atoi(row[r++]);
				tmpNPCType->max_dmg = atoi(row[r++]);
				strcpy(tmpNPCType->npc_attacks,row[r++]);
				tmpNPCType->npc_spells_id = atoi(row[r++]);
				tmpNPCType->d_meele_texture1= atoi(row[r++]);
				tmpNPCType->d_meele_texture2= atoi(row[r++]);
				tmpNPCType->walkspeed= atof(row[r++]);
				tmpNPCType->runspeed= atof(row[r++]);
				tmpNPCType->fixedZ = atof(row[r++]);
				tmpNPCType->hp_regen = atoi(row[r++]);
				tmpNPCType->mana_regen = atoi(row[r++]);
				
				tmpNPCType->aggroradius = (sint32)atoi(row[r++]);
				// set defaultvalue for aggroradius
				if (tmpNPCType->aggroradius <= 0)
					tmpNPCType->aggroradius = 70;

				if (row[r] && strlen(row[r]))
					tmpNPCType->bodytype = (int8)atoi(row[r]);
				else
					tmpNPCType->bodytype = 0;
				r++;
				
				tmpNPCType->npc_faction_id = atoi(row[r++]);
				
				tmpNPCType->luclinface = atoi(row[r++]);
				tmpNPCType->hairstyle = atoi(row[r++]);
				tmpNPCType->haircolor = atoi(row[r++]);
				tmpNPCType->eyecolor1 = atoi(row[r++]);
				tmpNPCType->eyecolor2 = atoi(row[r++]);
				tmpNPCType->beardcolor = atoi(row[r++]);
				tmpNPCType->beard = atoi(row[r++]);

				tmpNPCType->see_invis = atoi(row[r++]);			// Mongrel: Set see_invis flag
				tmpNPCType->see_invis_undead = atoi(row[r++]);	// Mongrel: Set see_invis_undead flag
				if (row[r] != NULL)
					strncpy(tmpNPCType->lastname, row[r], 32);
				r++;
				
				tmpNPCType->qglobal = atoi(row[r++]);	// qglobal
				tmpNPCType->AC = atoi(row[r++]);
				tmpNPCType->npc_aggro = atoi(row[r++])==0?false:true;
				tmpNPCType->spawn_limit = atoi(row[r++]);

				// If NPC with duplicate NPC id already in table,
				// free item we attempted to add.
				if (zone->npctable.find(tmpNPCType->npc_id) != zone->npctable.end())
				{
					cerr << "Error loading duplicate NPC " << tmpNPCType->npc_id << endl;
					delete tmpNPCType;
					npc = NULL;
				} else {
					zone->npctable[tmpNPCType->npc_id]=tmpNPCType;
					npc = tmpNPCType;
				}

//				Sleep(0);
			}

			if (result) {
				mysql_free_result(result);
			}
		} else
			cerr << "Error loading NPCs from database. Bad query: " << errbuf << endl;
      safe_delete_array(query);

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
void Database::GetAccountFromID(int32 id, char* oAccountName, sint16* oStatus) {
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT name, status FROM account WHERE id=%i", id), errbuf, &result))
	{
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if (oAccountName)
				strcpy(oAccountName, row[0]);
			if (oStatus)
				*oStatus = atoi(row[1]);
		}
		mysql_free_result(result);
	}
	else
		cerr << "Error in GetAccountFromID query '" << query << "' " << errbuf << endl;
	safe_delete_array(query);
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



void Database::SaveMerchantTemp(int32 npcid, int32 slot, int32 item, int32 charges){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;

	if (!RunQuery(query, MakeAnyLenString(&query, "replace into merchantlist_temp (npcid,slot,itemid,charges) values(%d,%d,%d,%d)", npcid, slot, item, charges), errbuf)) {
		cerr << "Error in SaveMerchantTemp query '" << query << "' " << errbuf << endl;
	}
	safe_delete_array(query);	
}
void Database::DeleteMerchantTemp(int32 npcid, int32 slot){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;

	if (!RunQuery(query, MakeAnyLenString(&query, "delete from merchantlist_temp where npcid=%d and slot=%d", npcid, slot), errbuf)) {
		cerr << "Error in DeleteMerchantTemp query '" << query << "' " << errbuf << endl;
	}
	safe_delete_array(query);	
}
void Database::ClearMerchantTemp(){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;

	if (!RunQuery(query, MakeAnyLenString(&query, "delete from merchantlist_temp"), errbuf)) {
		cerr << "Error in ClearMerchantTemp query '" << query << "' " << errbuf << endl;
	}
	safe_delete_array(query);	
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
bool Database::MoveCharacterToZone(const char* charname, const char* zonename,int32 zoneid) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int32	affected_rows = 0;
	
	if(zonename == NULL || strlen(zonename) == 0)
		return(false);
	
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE character_ SET zonename = '%s',zoneid=%i,x=-1, y=-1, z=-1 WHERE name='%s'", zonename,zoneid, charname), errbuf, 0,&affected_rows)) {
		cerr << "Error in MoveCharacterToZone(name) query '" << query << "' " << errbuf << endl;
		return false;
	}
	safe_delete_array(query);
	
	if (affected_rows == 0)
		return false;
	
	return true;
}
bool Database::MoveCharacterToZone(const char* charname, const char* zonename) {
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
	ExtendedProfile_Struct* ext;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT profile, guild, guildrank, extprofile FROM character_ WHERE name='%s'", oldname), errbuf, &result)) {
		safe_delete_array(query);
		
		row = mysql_fetch_row(result);
		
		pp = (PlayerProfile_Struct*)row[0];
		strcpy(pp->name, newname);
		
		ext = (ExtendedProfile_Struct*)row[3];
		
		mysql_free_result(result);
	}

	else {	
		cerr << "Error in CopyCharacter read query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return 0;
	}
	
	int32 affected_rows = 0;
	char query2[276 + sizeof(PlayerProfile_Struct)*2 + sizeof(ExtendedProfile_Struct)*2 + 1];
	char* end=query2;
	
	end += sprintf(end, "INSERT INTO character_ SET zonename=\'%s\', x = %f, y = %f, z = %f, profile=\'", GetZoneName(pp->zone_id), pp->x, pp->y, pp->z);
    end += DoEscapeString(end, (char*) pp, sizeof(PlayerProfile_Struct));
	end += sprintf(end,"\', extprofile=\'");
	end += DoEscapeString(end, (char*) ext, sizeof(ExtendedProfile_Struct));
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
	_CP(Database_DBLoadNPCFactionLists);
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
			if (RunQuery(query, MakeAnyLenString(&query, "SELECT npc_faction_id, faction_id, value, npc_value FROM npc_faction_entries order by npc_faction_id"), errbuf, &result)) {
				safe_delete_array(query);
				sint8 i = 0;
				int32 curflid = 0;
				int32 tmpflid = 0;
				uint32 tmpfactionid[MAX_NPC_FACTIONS];
				sint32 tmpfactionvalue[MAX_NPC_FACTIONS];
				sint8 tmpfactionnpcvalue[MAX_NPC_FACTIONS];

				memset(tmpfactionid, 0, sizeof(tmpfactionid));
				memset(tmpfactionvalue, 0, sizeof(tmpfactionvalue));
				memset(tmpfactionnpcvalue, 0, sizeof(tmpfactionnpcvalue));
				
				while((row = mysql_fetch_row(result))) {
					tmpflid = atoi(row[0]);
					if (curflid != tmpflid && curflid != 0) {
						if (!EMuShareMemDLL.NPCFactionList.cbSetFaction(curflid, tmpfactionid, tmpfactionvalue, tmpfactionnpcvalue)) {
							mysql_free_result(result);
							cout << "Error: Database::DBLoadNPCFactionLists: !EMuShareMemDLL.NPCFactionList.cbSetFaction" << endl;
							return false;
						}
						memset(tmpfactionid, 0, sizeof(tmpfactionid));
						memset(tmpfactionvalue, 0, sizeof(tmpfactionvalue));
						memset(tmpfactionnpcvalue, 0, sizeof(tmpfactionnpcvalue));
						i = 0;
					}
					curflid = tmpflid;
					tmpfactionid[i] = atoi(row[1]);
					tmpfactionvalue[i] = atoi(row[2]);
					tmpfactionnpcvalue[i] = atoi(row[3]);
					i++;
					if (i >= MAX_NPC_FACTIONS) {
						cerr << "Error in DBLoadNPCFactionLists: More than MAX_NPC_FACTIONS factions returned, flid=" << tmpflid << endl;
						break;
					}
					Sleep(0);
				}
				if (tmpflid) {
					EMuShareMemDLL.NPCFactionList.cbSetFaction(curflid, tmpfactionid, tmpfactionvalue, tmpfactionnpcvalue);
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
		if (item->ID >= 10000)
			snprintf(ret, sizeof(ret), "%c00%i%s%c", 0x12, item->ID, item->Name, 0x12);
		else
			snprintf(ret, sizeof(ret), "%c00%i %s%c", 0x12, item->ID, item->Name, 0x12);
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
	LogFile->write(EQEMuLog::Status, "Start zone query: %s\n", query);
	safe_delete_array(query); 
	
	if((rows = mysql_num_rows(result)) > 0)
		row = mysql_fetch_row(result);
	
	if(row)
	{         
		LogFile->write(EQEMuLog::Status, "Found starting location in start_zones");
		in_pp->x = atof(row[0]); 
		in_pp->y = atof(row[1]); 
		in_pp->z = atof(row[2]); 
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
	if(result) 
		mysql_free_result(result);	
	return true;
}

int8 Database::GetRaceSkill(int8 skillid, int8 in_race)
{
	int16 race_cap = 0;
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int32	affected_rows = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;

	//Check for a racial cap!
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT skillcap from race_skillcaps where skill = %i && race = %i", skillid, in_race), errbuf, &result, &affected_rows))
	{
		if (affected_rows != 0)
		{
			row = mysql_fetch_row(result);
			race_cap = atoi(row[0]);
		}
		delete[] query;
		mysql_free_result(result);
	}

	return race_cap;
}

int8 Database::GetSkillCap(int8 skillid, int8 in_race, int8 in_class, int16 in_level)
{
	int8 skill_level = 0, skill_formula = 0;
	int16 base_cap = 0, skill_cap = 0, skill_cap2 = 0, skill_cap3 = 0;
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int32	affected_rows = 0;
	MYSQL_RES *result;
	MYSQL_ROW row;
	//Fetch the data from DB.
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT level, formula, pre50cap, post50cap, post60cap from skillcaps where skill = %i && class = %i", skillid, in_class), errbuf, &result, &affected_rows))
	{
		if (affected_rows != 0)
		{
			row = mysql_fetch_row(result);
			skill_level = atoi(row[0]);
			skill_formula = atoi(row[1]);
			skill_cap = atoi(row[2]);
			if (atoi(row[3]) > skill_cap)
				skill_cap2 = (atoi(row[3])-skill_cap)/10; //Split the post-50 skill cap into difference between pre-50 cap and post-50 cap / 10 to determine amount of points per level.
			skill_cap3 = atoi(row[4]);
		}
		delete[] query;
		mysql_free_result(result);
	}

	int race_skill = GetRaceSkill(skillid,in_race);

	if (race_skill > 0 && (race_skill > skill_cap || skill_cap == 0 || in_level < skill_level))
		return race_skill;

	if (skill_cap == 0) //Can't train this skill at all.
		return 255; //Untrainable

	if (in_level < skill_level)
		return 254; //Untrained

	//Determine pre-51 level-based cap
	if (skill_formula > 0)
		base_cap = in_level*skill_formula+skill_formula;
	if (base_cap > skill_cap || skill_formula == 0)
		base_cap = skill_cap;
	//If post 50, add post 50 cap to base cap.
	if (in_level > 50 && skill_cap2 > 0)
		base_cap += skill_cap2*(in_level-50);
	//No cap should ever go above its post50cap
	if (skill_cap3 > 0 && base_cap > skill_cap3)
		base_cap = skill_cap3;
	//Base cap is now the max value at the person's level, return it!
	return base_cap;
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

void Database::UpdateDoorGuildID(int doorid, int guildid)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
	int32	affected_rows = 0;

	RunQuery(query, MakeAnyLenString(&query, 
		"UPDATE doors SET guild=%i WHERE id=%i;",guildid,doorid),
		errbuf, 0, &affected_rows);

	safe_delete_array(query);

	return;
}

extern "C" bool extDBLoadGuildList() { return database.DBLoadGuildList(); }

const char* Database::GetGuild(uint32 db_id) {
	return EMuShareMemDLL.GuildList.GetGuild(db_id);
}

uint32 Database::GetMaxGuildID() {
	return EMuShareMemDLL.GuildList.GetMaxGuildID();
}

bool Database::LoadGuildList() {
	if (!EMuShareMemDLL.Load())
		return false;
	bool ret = EMuShareMemDLL.GuildList.DLLLoadGuildList(&extDBLoadGuildList, sizeof(GuildsListEntry_Struct));
	return ret;
}

bool Database::DBLoadGuildList() {
   LogFile->write(EQEMuLog::Status, "Loading GuildList from database...");
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	query = new char[256];
	MakeAnyLenString(&query, "SELECT eqid,name from guilds order by eqid asc");
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		safe_delete_array(query);
		while((row = mysql_fetch_row(result))) {
			uint32 db_id=atoi(row[0]);
			if (!EMuShareMemDLL.GuildList.cbAddGuild(db_id, (char *)row[1])) {
				mysql_free_result(result);
				return false;
			}
			Sleep(0);
		}
		mysql_free_result(result);
	}
	else
	{
		cerr << "Error in DBLoadGuildList query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
		return false;
	}

	return true;
}
