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
#include "../common/version.h"
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

void Client::SendGuildMembers(int32 guildid){
	if(guildid==0)
		return;
	uchar* blah=new uchar[(sizeof(GuildMember)*database.NumberInGuild(guildid)+sizeof(GuildMember_Struct))];
	GuildMember_Struct* gms=(GuildMember_Struct*)blah;
	database.GetGuildMembers(guildid,gms);
	if(!gms || gms->count==0){
		printf("Error in SendGuildMembers, no members!\n");
		if(gms)
			safe_delete(gms);
		return;
	}
	int16 namelen=strlen(GetName());
	APPLAYER* outapp = new APPLAYER(OP_GuildMemberList,gms->length+(34*gms->count)+namelen+5);
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
		memcpy(buffer,&gms->member[i].class_, sizeof(int32));
		buffer+=sizeof(int32);
		gms->member[i].rank=htonl(gms->member[i].rank);
		memcpy(buffer,&gms->member[i].rank, sizeof(int32));
		buffer+=sizeof(int32);
		memcpy(buffer,&gms->member[i].timelaston, sizeof(int32));
		buffer+=(sizeof(int32)*4);
		if(strlen(gms->member[i].publicnote)>1){
			memcpy(buffer,&gms->member[i].publicnote, strlen(gms->member[i].publicnote));
			buffer+=strlen(gms->member[i].publicnote);
		}
		buffer+=sizeof(int32);
		Client *member = entity_list.GetClientByName(gms->member[i].name);
		if(member)	//only add zone info if player is online :)
			memcpy(buffer,&gms->member[i].zoneid, sizeof(int8));	
		buffer+=sizeof(int8);
	}
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
	APPLAYER* outapp = new APPLAYER(OP_GuildManageAdd,sizeof(GuildJoin_Struct));
	GuildJoin_Struct* outgj=(GuildJoin_Struct*)outapp->pBuffer;
	outgj->class_=gj->class_;
	outgj->guildid=gj->guildid;
	outgj->level=gj->level;
	strcpy(outgj->name,gj->name);
	outgj->rank=gj->rank;
	outgj->zoneid=gj->zoneid;
	QueuePacket(outapp);
	safe_delete(outapp);
}

void Client::GuildChangeRank(int32 guildid,int32 oldrank,int32 newrank){
	GuildChangeRank(GetName(),guildid,oldrank,newrank);
}

void Client::GuildChangeRank(const char* name, int32 guildid,int32 oldrank,int32 newrank){
	APPLAYER* outapp = new APPLAYER(OP_GuildManageStatus,sizeof(GuildManageStatus_Struct));
	GuildManageStatus_Struct* gms=(GuildManageStatus_Struct*)outapp->pBuffer;
	gms->guildid=guildid;
	strcpy(gms->name,name);
	gms->newrank=newrank;
	gms->oldrank=oldrank;
	entity_list.QueueClientsGuild(this,outapp,false,guildid);
	safe_delete(outapp);
}
