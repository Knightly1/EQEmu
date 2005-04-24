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
#include "masterentity.h"
#include "worldserver.h"
#include "net.h"
#include "../common/database.h"
#include "spdat.h"
#include "../common/packet_dump.h"
#include "../common/packet_functions.h"
#include "petitions.h"
#include "../common/serverinfo.h"
#include "../common/ZoneNumbers.h"
#include "../common/moremath.h"
#include "../common/guilds.h"
#include "StringIDs.h"
#include "NpcAI.h"

#ifdef GUILDWARS
#include "../GuildWars/GuildWars.h"
extern GuildWars guildwars;
#endif

#ifdef RAIDADDICTS
#include "RaidAddicts.h"
extern RaidAddicts raidaddicts;
#endif

extern GuildRanks_Struct guilds[512];

void Client::SendGuildMembers(int32 guildid, bool sendtoall){
	if(guildid==0)
		return;
	uint32 len = (sizeof(GuildMember)*database.NumberInGuild(guildid)+sizeof(GuildMember_Struct));
	uchar* blah=new uchar[len];
	memset(blah, 0, len);
	
	GuildMember_Struct* gms=(GuildMember_Struct*)blah;
	database.GetGuildMembers(guildid,gms);
	if(!gms || gms->count==0){
		printf("Error in SendGuildMembers, no members!\n");
		if(gms)
			safe_delete(gms);
		return;
	}
	int16 namelen=strlen(GetName());
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_GuildMemberList,gms->length+(42*gms->count)+namelen+5);
	memset(outapp->pBuffer,0,outapp->size);
	uchar* buffer=(uchar*)outapp->pBuffer;
	memcpy(buffer,GetName(), namelen);
	buffer+=namelen+1;
	int32 count=htonl(gms->count);
	memcpy(buffer,&count, sizeof(int32));
	buffer+=sizeof(int32);
	for(int32 i=0;i<gms->count;i++){	
		memcpy(buffer,&gms->member[i].name, strlen(gms->member[i].name));
		buffer+=(strlen(gms->member[i].name)+1);
		memcpy(buffer,&gms->member[i].level, sizeof(int32));
		buffer+=sizeof(int32);
		memcpy(buffer,&gms->member[i].banker_flag, sizeof(int32));
		buffer+=sizeof(int32);
		memcpy(buffer,&gms->member[i].class_, sizeof(int32));
		buffer+=sizeof(int32);
		gms->member[i].rank=htonl(gms->member[i].rank);
		memcpy(buffer,&gms->member[i].rank, sizeof(int32));
		buffer+=sizeof(int32);
		memcpy(buffer,&gms->member[i].timelaston, sizeof(int32));
		buffer+=sizeof(int32);
		memcpy(buffer,&gms->member[i].guild_tribute_flag, sizeof(int32));
		buffer+=sizeof(int32);
		memcpy(buffer,&gms->member[i].guild_tribute_donated, sizeof(int32));
		buffer+=sizeof(int32);
		memcpy(buffer,&gms->member[i].last_tribute_donation_time, sizeof(int32));
		buffer+=sizeof(int32);
		if(strlen(gms->member[i].publicnote)>1){
			memcpy(buffer,&gms->member[i].publicnote, strlen(gms->member[i].publicnote));
			buffer+=strlen(gms->member[i].publicnote);
		}
		buffer+=sizeof(int8);
		Client *member = entity_list.GetClientByName(gms->member[i].name);
		if(member)	//only add zone info if player is online :)
			memcpy(buffer,&gms->member[i].zoneinstance, sizeof(int16));	
		buffer+=sizeof(int16);
		if(member)	//only add zone info if player is online :)
			memcpy(buffer,&gms->member[i].zoneid, sizeof(int16));	
		buffer+=sizeof(int16);
	}
	if(sendtoall)
		entity_list.QueueClientsGuild(this, outapp, false, GuildEQID());
	else
		QueuePacket(outapp);
	safe_delete(outapp);
	safe_delete_array(blah);
}
bool Client::SetGuild(int32 in_guilddbid, int8 in_rank) {
	if (in_guilddbid == 0) {
		// update DB
		if (!database.SetGuild(character_id, 0, GUILD_MEMBER))
			return false;
		// clear guildtag
		guilddbid = in_guilddbid;
		guildeqid = GUILD_NONE;
		guildrank = GUILD_MEMBER;
		SendAppearancePacket(AT_GuildID, GUILD_NONE);
		SendAppearancePacket(AT_GuildRank, 3);
		UpdateWho();
		return true;
	}
	else {
		int32 tmp = database.GetGuildEQID(in_guilddbid);
		if (tmp != GUILD_NONE) {
			if (!database.SetGuild(character_id, in_guilddbid, in_rank))
				return false;
			guildeqid = tmp;
			guildrank = in_rank;
			if (guilddbid != in_guilddbid) {
				guilddbid = in_guilddbid;
				SendAppearancePacket(AT_GuildID, guildeqid);
			}
			SendAppearancePacket(AT_GuildRank, in_rank);
			UpdateWho();
			return true;
		}
	}
	UpdateWho();
	return false;
}

bool Database::CheckGuildDoor(int8 doorid,int16 guildid,const char* zone) {
	MYSQL_ROW row;
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
    MYSQL_RES *result;
	if (!RunQuery(query, MakeAnyLenString(&query, "SELECT guild FROM doors where doorid=%i AND zone='%s'",doorid-128, zone), errbuf, &result)) {
		cerr << "Error in CheckUsedName query '" << query << "' " << errbuf << endl;
		if (query != 0)
			safe_delete_array(query);
		return false;
	}
	else { 
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if (atoi(row[0]) == guildid)
			{
				mysql_free_result(result);
				return true;
			}
			else
			{
				mysql_free_result(result);
				return false;
			}
			
			// code below will never be reached
			mysql_free_result(result);
			return false;
		}
	}
	return false;
}

bool Database::SetGuildDoor(int8 doorid,int16 guildid, const char* zone) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int32	affected_rows = 0;
	if (doorid > 127)
		doorid = doorid - 128;
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE doors SET guild = %i WHERE (doorid=%i) AND (zone='%s')",guildid,doorid, zone), errbuf, 0,&affected_rows)) {
		cerr << "Error in SetGuildDoor query '" << query << "' " << errbuf << endl;
		return false;
	}
	
	safe_delete_array(query);
	
	if (affected_rows == 0)
	{
		return false;
	}
	
	return true;
}

void Client::SendGuildJoin(GuildJoin_Struct* gj){
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_GuildManageAdd,sizeof(GuildJoin_Struct));
	GuildJoin_Struct* outgj=(GuildJoin_Struct*)outapp->pBuffer;
	outgj->class_=gj->class_;
	outgj->guildid=gj->guildid;
	outgj->level=gj->level;
	strcpy(outgj->name,gj->name);
	outgj->rank=gj->rank;
	outgj->zoneid=gj->zoneid;
	QueuePacket(outapp);
	safe_delete(outapp);
	SendGuildMembers(guilds[gj->guildid].databaseID, true);
}

void Client::GuildChangeRank(int32 guildid,int32 oldrank,int32 newrank){
	GuildChangeRank(GetName(),guildid,oldrank,newrank);
}

void Client::GuildChangeRank(const char* name, int32 guildid,int32 oldrank,int32 newrank){
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_GuildManageStatus,sizeof(GuildManageStatus_Struct));
	GuildManageStatus_Struct* gms=(GuildManageStatus_Struct*)outapp->pBuffer;
	gms->guildid=guildid;
	strcpy(gms->name,name);
	gms->newrank=newrank;
	gms->oldrank=oldrank;
	entity_list.QueueClientsGuild(this,outapp,false,guildid);
	safe_delete(outapp);
	SendGuildMembers(guilds[guildid].databaseID, true);
}

// Rogean: Moved this function from common/guilds.cpp, VS 6.0 Doesn't like same filenames ^_^
int32 Database::GetGuildEQID(int32 guilddbid) {
	if (guilddbid == 0)
		return 0xFFFFFFFF;
	for (int32 i=0; i<512; i++) {
		if (guilds[i].databaseID == guilddbid)
			return i;
	}
	return 0xFFFFFFFF;
/*
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT eqid FROM guilds WHERE id=%i", guilddbid), errbuf, &result)) {
		delete[] query;
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
		cerr << "Error in GetGuildEQID query '" << query << "' " << errbuf << endl;
		delete[] query;
	}
	
	return 0xFFFFFFFF;
*/
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
			gms->member[count].zoneid=(pps->zone_id*256);
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

string Database::GetGuildMOTD(int32 guilddbid)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	string motd_str;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT motd FROM guilds WHERE id=%i", guilddbid), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if (row[0])
				motd_str = row[0];
		}
		mysql_free_result(result);
	}
	else {
		cerr << "Error in GetGuildMOTD query '" << query << "' " << errbuf << endl;
		safe_delete_array(query);
	}
	return motd_str;
}


