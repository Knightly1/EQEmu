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
#include <iostream>
using namespace std;
#include <string.h>
#include "../common/misc.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#ifdef WIN32
	#include <windows.h>
	#include <winsock.h>
#else
	#include <sys/socket.h>
#ifdef FREEBSD //Timothy Whitman - January 7, 2003
	#include <sys/types.h>
#endif
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <errno.h>
	#include <pthread.h>
	#include <stdarg.h>

	#include "../common/unix.h"

	#define SOCKET_ERROR -1
	#define INVALID_SOCKET -1

	extern int errno;
#endif

#include "../common/servertalk.h"
#include "../common/eq_packet_structs.h"
#include "../common/packet_dump.h"
#include "../common/database.h"
#include "../common/races.h"
#include "../common/classes.h"
#include "../common/seperator.h"
#include "../common/files.h"
#include "../common/md5.h"
#include "zoneserver.h"
#include "net.h"
#include "client.h"
#include "LoginServer.h"
#include <string>
#include <assert.h>

#ifdef WIN32
	#define snprintf	_snprintf
	#define vsnprintf	_vsnprintf
	#define strncasecmp	_strnicmp
	#define strcasecmp  _stricmp
#endif
int32 numplayers = 0;
extern Database			database;
extern uint32			numzones;
extern ZSList			zoneserver_list;
extern ClientList		client_list;
extern ConsoleList		console_list;
extern GuildRanks_Struct guilds[512];
extern bool GetZoneLongName(char* short_name, char** long_name);
extern NetConnection net;
extern TCPServer tcps;
extern LoginServer loginserver;
extern bool holdzones;
extern volatile bool RunLoops;
Timer* spawntimeleft=new Timer(300000);

ZoneServer::ZoneServer(TCPConnection* itcpc) : WorldTCPConnection() {
	tcpc = itcpc;
	ID = zoneserver_list.GetNextID();
	memset(zone_name, 0, sizeof(zone_name));
	zoneID = 0;

	memset(clientaddress, 0, sizeof(clientaddress));
	clientport = 0;
	BootingUp = false;
	authenticated = false;
	staticzone = false;
	pNumPlayers = 0;
}

ZoneServer::~ZoneServer() {
	if (RunLoops)
		zoneserver_list.CLERemoveZSRef(this);
	tcpc->Free();
}

bool ZoneServer::SetZone(int32 iZoneID, bool iStaticZone) {
	BootingUp = false;
	
	zoneID = iZoneID;
	if (zoneID == 0) {
		zoneserver_list.CLERemoveZSRef(this);
		pNumPlayers = 0;
	}

	const char* zn = database.GetZoneName(zoneID);
	if (zn)
		strcpy(zone_name, zn);
	else
		strcpy(zone_name, "");

	staticzone = iStaticZone;
	cout << "Zoneserver SetZone: " << GetCAddress() << ":" << GetCPort() << " " << zone_name << " (" << zoneID << ")";
	if (staticzone)
		cout << " Static";
	cout << endl;

	client_list.ZoneBootup(this);
	return true;
}

bool ZoneServer::SetConnectInfo(const char* in_address, int16 in_port) {
	if(!spawntimeleft->Enabled())
		spawntimeleft->Start();
	client_list.ZoneBootup(this);
	strcpy(clientaddress, in_address);
	clientport = in_port;
	struct in_addr  in;
	in.s_addr = GetIP();
	cout << "Zoneserver SetConnectInfo: " << inet_ntoa(in) << ":" << GetPort() << ": " << clientaddress << ":" << clientport << endl;

	return true;
}

bool ZoneServer::Process() {
	if (!tcpc->Connected())
		return false;
	if(spawntimeleft->Check())
		database.UpdateTimeleftWorld();
	
	ServerPacket *pack = 0;
	while((pack = tcpc->PopPacket())) {
		if (!authenticated) {
			char tmp[100];
			if (database.GetVariable("ZSPassword", tmp, sizeof(tmp))) {
				if (pack->opcode == ServerOP_ZAAuth && pack->size == 16) {
					int8 tmppass[16];
					MD5::Generate((uchar*) tmp, strlen(tmp), tmppass);
					if (memcmp(pack->pBuffer, tmppass, 16) == 0)
						authenticated = true;
					else {
						struct in_addr  in;
						in.s_addr = GetIP();
						cout << "Zoneserver auth failed: " << inet_ntoa(in) << ":" << GetPort() << endl;
						ServerPacket* pack = new ServerPacket(ServerOP_ZAAuthFailed);
						SendPacket(pack);
						delete pack;
						Disconnect();
						return false;
					}
				}
				else {
					struct in_addr  in;
					in.s_addr = GetIP();
					cout << "Zoneserver auth failed: " << inet_ntoa(in) << ":" << GetPort() << endl;
					ServerPacket* pack = new ServerPacket(ServerOP_ZAAuthFailed);
					SendPacket(pack);
					delete pack;
					Disconnect();
					return false;
				}
			}
			else
			{
				printf("**WARNING** You have not setup a zone server password into the database, it is HIGHLY recommended that you add a variable into variables with the varname ZSPassword and the value with random characters to protect from people connecting to your world server.");
				authenticated = true;
			}
		}
		switch(pack->opcode) {
		case 0:
			break;
		case ServerOP_KeepAlive: {
			// ignore this
			break;
		}
		case ServerOP_ZAAuth: {
			break;
		}
		/*
		case ServerOP_SendGroup: {
			SendGroup_Struct* sgs=(SendGroup_Struct*)pack->pBuffer;
			ZoneServer* zs=zoneserver_list.FindByZoneID(sgs->zoneid);
			if(!zs)
				printf("Could not find zone id: %i running to transfer group to!\n",sgs->zoneid);
			else{
				zs->SendPacket(pack);
			}
			break;
		}*/
		case ServerOP_GroupIDReq: {
			SendGroupIDs();
			break;
		}
		case ServerOP_GroupLeave: {
			if(pack->size != sizeof(ServerGroupLeave_Struct))
				break;
			//bounce the group leave structure to the correct zone server
			ServerGroupLeave_Struct* sgl = (ServerGroupLeave_Struct*)pack->pBuffer;
			sgl->member_name[63] = '\0';
			ClientListEntry *cle = zoneserver_list.FindCharacter(sgl->member_name);
			if(cle) {
				ZoneServer *zs = cle->Server();
				if(zs) {
					zs->SendPacket(pack);
				}
			}
			break;
		}
		case ServerOP_ChannelMessage: {
			ServerChannelMessage_Struct* scm = (ServerChannelMessage_Struct*) pack->pBuffer;
			if (scm->chan_num == 7 || scm->chan_num == 14) {
				if (scm->deliverto[0] == '*') {
					Console* con = 0;
					con = console_list.FindByAccountName(&scm->deliverto[1]);
					if (((!con) || (!con->SendChannelMessage(scm))) && (!scm->noreply))
						zoneserver_list.SendEmoteMessage(scm->from, 0, 0, 0, "You told %s, '%s is not online at this time'", scm->to, scm->to);
					break;
				}
				ClientListEntry* cle = zoneserver_list.FindCharacter(scm->deliverto);
				if (cle == 0 || cle->Online() < CLE_Status_Zoning || (cle->TellsOff() && ((cle->Anon() == 1 && scm->fromadmin < cle->Admin()) || scm->fromadmin < 80))) {
					if (!scm->noreply)
						zoneserver_list.SendEmoteMessage(scm->from, 0, 0, 0, "You told %s, '%s is not online at this time'", scm->to, scm->to);
				}
				else if (cle->Online() == CLE_Status_Zoning) {
					if (!scm->noreply) {
						char errbuf[MYSQL_ERRMSG_SIZE];
						char *query = 0;
						MYSQL_RES *result;
						//MYSQL_ROW row;   Trumpcard - commenting.  Currently unused.
						time_t rawtime;
						struct tm * timeinfo;
						time ( &rawtime );
						timeinfo = localtime ( &rawtime );
						char *telldate=asctime(timeinfo);
						if (database.RunQuery(query, MakeAnyLenString(&query, "SELECT name from character_ where name='%s'",scm->deliverto), errbuf, &result)) {
							safe_delete(query);
							if (result!=0) {
								if (database.RunQuery(query, MakeAnyLenString(&query, "INSERT INTO tellque (Date,Receiver,Sender,Message) values('%s','%s','%s','%s')",telldate,scm->deliverto,scm->from,scm->message), errbuf, &result))
									zoneserver_list.SendEmoteMessage(scm->from, 0, 0, 0, "Your message has been added to the %s's que.", scm->to);
								else
									zoneserver_list.SendEmoteMessage(scm->from, 0, 0, 0, "You told %s, '%s is not online at this time'", scm->to, scm->to);
								safe_delete(query);
							}
							else
								zoneserver_list.SendEmoteMessage(scm->from, 0, 0, 0, "You told %s, '%s is not online at this time'", scm->to, scm->to);
							mysql_free_result(result);
						}
						else
							safe_delete(query);
					}
				//		zoneserver_list.SendEmoteMessage(scm->from, 0, 0, 0, "You told %s, '%s is not online at this time'", scm->to, scm->to);
				}
				else if (cle->Server() == 0) {
					if (!scm->noreply)
						zoneserver_list.SendEmoteMessage(scm->from, 0, 0, 0, "You told %s, '%s is not contactable at this time'", scm->to, scm->to);
				}
				else
					cle->Server()->SendPacket(pack);
			}
			else {
				if (scm->chan_num == 5 || scm->chan_num == 6 || scm->chan_num == 11) {
					console_list.SendChannelMessage(scm);
				}
				zoneserver_list.SendPacket(pack);
			}
			break;
		}
		case ServerOP_EmoteMessage: {
			ServerEmoteMessage_Struct* sem = (ServerEmoteMessage_Struct*) pack->pBuffer;
			zoneserver_list.SendEmoteMessageRaw(sem->to, sem->guilddbid, sem->minstatus, sem->type, sem->message);
			break;
		}
		case ServerOP_RezzPlayer: {

		    RezzPlayer_Struct* sRezz = (RezzPlayer_Struct*) pack->pBuffer;
			if (zoneserver_list.SendPacket(pack)){
               LogFile->write(EQEMuLog::Status, "Sent Rezz packet %s", sRezz->rez.your_name);
			}
			else {
               LogFile->write(EQEMuLog::Status, "NOT Sent Rezz packet %s", sRezz->rez.your_name);
			}
			break;
		}
		case ServerOP_MultiLineMsg: {
			ServerMultiLineMsg_Struct* mlm = (ServerMultiLineMsg_Struct*) pack->pBuffer;
			zoneserver_list.SendPacket(mlm->to, pack);
			break;
		}
		case ServerOP_SetZone: {
			if(pack->size != sizeof(SetZone_Struct))
				break;

			SetZone_Struct* szs = (SetZone_Struct*) pack->pBuffer;
			if (szs->zoneid != 0) {
				if(database.GetZoneName(szs->zoneid))
					SetZone(szs->zoneid, szs->staticzone);
				else
					SetZone(0);
			}
			else
				SetZone(0);
			break;
		}
		case ServerOP_SetConnectInfo: {
			if (pack->size != sizeof(ServerConnectInfo))
				break;

			ServerConnectInfo* sci = (ServerConnectInfo*) pack->pBuffer;
			SetConnectInfo(sci->address, sci->port);
			break;
		}
		case ServerOP_ShutdownAll: {
			if(pack->size==0){
				zoneserver_list.SendPacket(pack);
				zoneserver_list.Process();
				CatchSignal(0);
			}
			else{
				WorldShutDown_Struct* wsd=(WorldShutDown_Struct*)pack->pBuffer;
				if(wsd->time==0 && wsd->interval==0 && zoneserver_list.shutdowntimer->Enabled()){
					zoneserver_list.shutdowntimer->Disable();
					zoneserver_list.reminder->Disable();
				}
				else{
					zoneserver_list.shutdowntimer->SetTimer(wsd->time);
					zoneserver_list.reminder->SetTimer(wsd->interval-1000);
					zoneserver_list.reminder->SetAtTrigger(wsd->interval);
					zoneserver_list.shutdowntimer->Start();
					zoneserver_list.reminder->Start();
				}
			}
			break;
		}
		case ServerOP_ZoneShutdown: {
			ServerZoneStateChange_struct* s = (ServerZoneStateChange_struct *) pack->pBuffer;
			ZoneServer* zs = 0;
			if (s->ZoneServerID != 0)
				zs = zoneserver_list.FindByID(s->ZoneServerID);
			else if (s->zoneid != 0)
				zs = zoneserver_list.FindByName(database.GetZoneName(s->zoneid));
			else
				zoneserver_list.SendEmoteMessage(s->adminname, 0, 0, 0, "Error: SOP_ZoneShutdown: neither ID nor name specified");

			if (zs == 0)
				zoneserver_list.SendEmoteMessage(s->adminname, 0, 0, 0, "Error: SOP_ZoneShutdown: zoneserver not found");
			else
				zs->SendPacket(pack);
			break;
		}
		case ServerOP_ZoneBootup: {
			ServerZoneStateChange_struct* s = (ServerZoneStateChange_struct *) pack->pBuffer;
			ZSList::SOPZoneBootup(s->adminname, s->ZoneServerID, database.GetZoneName(s->zoneid), s->makestatic);
			break;
		}
		case ServerOP_ZoneStatus: {
			if (pack->size >= 1)
				zoneserver_list.SendZoneStatus((char *) &pack->pBuffer[1], (int8) pack->pBuffer[0], this);
			break;

		}
		case ServerOP_AcceptWorldEntrance: {
			if(pack->size != sizeof(WorldToZone_Struct))
				break;

			WorldToZone_Struct* wtz = (WorldToZone_Struct*) pack->pBuffer;
			Client* client = 0;
			client = client_list.FindByAccountID(wtz->account_id);
			if(client != 0)
				client->Clearance(wtz->response);
		}
		case ServerOP_ZoneToZoneRequest: {
		//
		// solar: ZoneChange is received by the zone the player is in, then the
		// zone sends a ZTZ which ends up here.  This code then find the target
		// (ingress point) and boots it if needed, then sends the ZTZ to it.
		// The ingress server will decide wether the player can enter, then will
		// send back the ZTZ to here.  This packet is passed back to the egress
		// server, which will send a ZoneChange response back to the client
		// which can be an error, or a success, in which case the client will
		// disconnect, and their zone location will be saved when ~Client is
		// called, so it will be available when they ask to zone.
		//
			if(pack->size != sizeof(ZoneToZone_Struct))
				break;

			ZoneToZone_Struct* ztz = (ZoneToZone_Struct*) pack->pBuffer;

#if DEBUG >= 6
			printf("World (from zone id %d) received ZTZ for %s current zone %d req zone %d\n",
				GetZoneID(), ztz->name, ztz->current_zone_id, ztz->requested_zone_id);
#endif

			if(GetZoneID() == ztz->current_zone_id)	// this is a request from the egress zone
			{
				printf("World: processing ZTZ from egress zone (%d) client: %s\n", GetZoneID(), ztz->name);

				if
				(
					ztz->admin < 80 &&
					ztz->ignorerestrictions < 2 &&
					zoneserver_list.IsZoneLocked(ztz->requested_zone_id)
				)
				{
					ztz->response = 0;
					SendPacket(pack);
					break;
				}


				ZoneServer *ingress_server = zoneserver_list.FindByZoneID(ztz->requested_zone_id);
				if(ingress_server)	// found a zone already running
				{
					printf("World (%d): Found a zone already booted for %s\n", GetZoneID(), ztz->name);
					ztz->response = 1;
				}
				else	// need to boot one
				{
					int server_id;
					if ((server_id = zoneserver_list.TriggerBootup(ztz->requested_zone_id))){
						printf("World (%d): Successfully booted a zone for %s\n", GetZoneID(), ztz->name);
						// bootup successful, ready to rock
						ztz->response = 1;
						ingress_server = zoneserver_list.FindByID(server_id);
					}
					else
					{
						printf("World (%d): FAILED to boot a zone for %s\n", GetZoneID(), ztz->name);
						// bootup failed, send back error code 0
						ztz->response = 0;
					}
				}
				SendPacket(pack);	// send back to egress server
				if(ingress_server)	// if we couldn't boot one, this is 0
				{
					ingress_server->SendPacket(pack);	// inform target server
				}
			}
			else	// this is response from the ingress server, route it back to the egress server
			{
				printf("World: processing ZTZ from ingress zone (%d) client: %s\n", GetZoneID(), ztz->name);
				ZoneServer *egress_server = zoneserver_list.FindByZoneID(ztz->current_zone_id);
				if(egress_server)
				{
					egress_server->SendPacket(pack);
				}
			}

			break;
		}
		case ServerOP_ClientList: {
			if (pack->size != sizeof(ServerClientList_Struct)) {
				cout << "Wrong size on ServerOP_ClientList. Got: " << pack->size << ", Expected: " << sizeof(ServerClientList_Struct) << endl;
				break;
			}
			zoneserver_list.ClientUpdate(this, (ServerClientList_Struct*) pack->pBuffer);
			break;
		}
		case ServerOP_ClientListKA: {
			ServerClientListKeepAlive_Struct* sclka = (ServerClientListKeepAlive_Struct*) pack->pBuffer;
			if (pack->size < 4 || pack->size != 4 + (4 * sclka->numupdates)) {
				cout << "Wrong size on ServerOP_ClientListKA. Got: " << pack->size << ", Expected: " << (4 + (4 * sclka->numupdates)) << endl;
				break;
			}
			zoneserver_list.CLEKeepAlive(sclka->numupdates, sclka->wid);
			break;
		}
		case ServerOP_Who: {
			ServerWhoAll_Struct* whoall = (ServerWhoAll_Struct*) pack->pBuffer;
			Who_All_Struct* whom = new Who_All_Struct;
			memset(whom,0,sizeof(Who_All_Struct));
			whom->gmlookup = whoall->gmlookup;
			whom->lvllow = whoall->lvllow;
			whom->lvlhigh = whoall->lvlhigh;
			whom->wclass = whoall->wclass;
			whom->wrace = whoall->wrace;
			strcpy(whom->whom,whoall->whom);
			zoneserver_list.SendWhoAll(whoall->fromid,whoall->from, whoall->admin, whom, this);
			delete whom;
			break;
		}
		case ServerOP_ZonePlayer: {
			//ServerZonePlayer_Struct* szp = (ServerZonePlayer_Struct*) pack->pBuffer;
			zoneserver_list.SendPacket(pack);
			break;
		}
		case ServerOP_KickPlayer: {
			zoneserver_list.SendPacket(pack);
			break;
		}
		case ServerOP_KillPlayer: {
			zoneserver_list.SendPacket(pack);
			break;
		}
		case ServerOP_RefreshGuild: {
			if (pack->size == 5) {
				int32 guildeqid = 0xFFFFFFFF;
				memcpy(&guildeqid, pack->pBuffer, 4);
				database.GetGuildRanks(guildeqid, &guilds[guildeqid]);
				zoneserver_list.SendPacket(pack);
			}
			else
				cout << "Wrong size: ServerOP_RefreshGuild. size=" << pack->size << endl;
			break;
		}
		case ServerOP_GuildLeader:
		case ServerOP_GuildInvite:
		case ServerOP_GuildRemove:
		case ServerOP_GuildPromote:
		case ServerOP_GuildDemote:
		case ServerOP_GuildGMSet:
		case ServerOP_GuildGMSetRank:
		{
			ServerGuildCommand_Struct* sgc = (ServerGuildCommand_Struct*) pack->pBuffer;
			ClientListEntry* cle = zoneserver_list.FindCharacter(sgc->target);
			if (cle == 0)
				zoneserver_list.SendEmoteMessage(sgc->from, 0, 0, 0, "%s is not online.", sgc->target);
			else if (cle->Server() == 0)
				zoneserver_list.SendEmoteMessage(sgc->from, 0, 0, 0, "%s is not online.", sgc->target);
			else if ((cle->Admin() >= 100 && sgc->admin < 100) && cle->GuildDBID() != sgc->guilddbid) // no peeping for GMs, make sure tell message stays the same
				zoneserver_list.SendEmoteMessage(sgc->from, 0, 0, 0, "%s is not online.", sgc->target);
			else
				cle->Server()->SendPacket(pack);
			break;
		}
		case ServerOP_GuildJoin:{
			zoneserver_list.SendPacket(pack);
			break;
		}
		case ServerOP_FlagUpdate: {
			ClientListEntry* cle = zoneserver_list.FindCLEByAccountID(*((int32*) pack->pBuffer));
			if (cle)
				cle->SetAdmin(*((sint16*) &pack->pBuffer[4]));
			zoneserver_list.SendPacket(pack);
			break;
		}
		case ServerOP_GMGoto: {
			if (pack->size != sizeof(ServerGMGoto_Struct)) {
				cout << "Wrong size on ServerOP_GMGoto. Got: " << pack->size << ", Expected: " << sizeof(ServerGMGoto_Struct) << endl;
				break;
			}
			ServerGMGoto_Struct* gmg = (ServerGMGoto_Struct*) pack->pBuffer;
			ClientListEntry* cle = zoneserver_list.FindCharacter(gmg->gotoname);
			if (cle != 0) {
				if (cle->Server() == 0)
					this->SendEmoteMessage(gmg->myname, 0, 0, 13, "Error: Cannot identify %s's zoneserver.", gmg->gotoname);
				else if (cle->Anon() == 1 && cle->Admin() > gmg->admin) // no snooping for anon GMs
					this->SendEmoteMessage(gmg->myname, 0, 0, 13, "Error: %s not found", gmg->gotoname);
				else
					cle->Server()->SendPacket(pack);
			}
			else {
				this->SendEmoteMessage(gmg->myname, 0, 0, 13, "Error: %s not found", gmg->gotoname);
			}
			break;
		}
		case ServerOP_Lock: {
			if (pack->size != sizeof(ServerLock_Struct)) {
				cout << "Wrong size on ServerOP_Lock. Got: " << pack->size << ", Expected: " << sizeof(ServerLock_Struct) << endl;
				break;
			}
		    ServerLock_Struct* slock = (ServerLock_Struct*) pack->pBuffer;
  			if (slock->mode >= 1) 
				net.world_locked = true;
			else
				net.world_locked = false;
			if (loginserver.Connected()) {
				loginserver.SendStatus();
				if (slock->mode >= 1)
					this->SendEmoteMessage(slock->myname, 0, 0, 13, "World locked");
				else
					this->SendEmoteMessage(slock->myname, 0, 0, 13, "World unlocked");
			}
			else {
				if (slock->mode >= 1)
					this->SendEmoteMessage(slock->myname, 0, 0, 13, "World locked, but login server not connected.");
				else 
					this->SendEmoteMessage(slock->myname, 0, 0, 13, "World unlocked, but login server not conencted.");
			}
			break;
							}
		case ServerOP_Motd: {
			if (pack->size != sizeof(ServerMotd_Struct)) {
				cout << "Wrong size on ServerOP_Motd. Got: " << pack->size << ", Expected: " << sizeof(ServerMotd_Struct) << endl;
				break;
			}
			ServerMotd_Struct* smotd = (ServerMotd_Struct*) pack->pBuffer;
			database.SetVariable("MOTD",smotd->motd);
			this->SendEmoteMessage(smotd->myname, 0, 0, 13, "Updated Motd.");
			break;
		}
		case ServerOP_Uptime: {
			if (pack->size != sizeof(ServerUptime_Struct)) {
				cout << "Wrong size on ServerOP_Uptime. Got: " << pack->size << ", Expected: " << sizeof(ServerUptime_Struct) << endl;
				break;
			}
			ServerUptime_Struct* sus = (ServerUptime_Struct*) pack->pBuffer;
			if (sus->zoneserverid == 0) {
				ZSList::ShowUpTime(this, sus->adminname);
			}
			else {
				ZoneServer* zs = zoneserver_list.FindByID(sus->zoneserverid);
				if (zs)
					zs->SendPacket(pack);
			}
			break;
							  }
		case ServerOP_Petition: {
			zoneserver_list.SendPacket(pack);
			break;
								}
		case ServerOP_GetWorldTime: {
			cout << "Broadcasting a world time update" << endl;
			ServerPacket* pack = new ServerPacket;
			
			pack->opcode = ServerOP_SyncWorldTime;
			pack->size = sizeof(eqTimeOfDay);
			pack->pBuffer = new uchar[pack->size];
			memset(pack->pBuffer, 0, pack->size);
			eqTimeOfDay* tod = (eqTimeOfDay*) pack->pBuffer;
			tod->start_eqtime=zoneserver_list.worldclock.getStartEQTime();
			tod->start_realtime=zoneserver_list.worldclock.getStartRealTime();
			SendPacket(pack);
			delete pack;
			break;
									}
		case ServerOP_SetWorldTime: {
			cout << "Received Message SetWorldTime" << endl;
			eqTimeOfDay* newtime = (eqTimeOfDay*) pack->pBuffer;
			zoneserver_list.worldclock.setEQTimeOfDay(newtime->start_eqtime, newtime->start_realtime);
			printf("New time = %d-%d-%d %d:%d (%d)\n", newtime->start_eqtime.year, newtime->start_eqtime.month, (int)newtime->start_eqtime.day, (int)newtime->start_eqtime.hour, (int)newtime->start_eqtime.minute, (int)newtime->start_realtime);
			zoneserver_list.worldclock.saveFile(EQTIME_INI);
			zoneserver_list.SendTimeSync();
			break;
		}
		case ServerOP_IPLookup: {
			if (pack->size < sizeof(ServerGenericWorldQuery_Struct)) {
				cout << "Wrong size on ServerOP_Uptime. Got: " << pack->size << ", Expected: " << sizeof(ServerUptime_Struct) << endl;
				break;
			}
			ServerGenericWorldQuery_Struct* sgwq = (ServerGenericWorldQuery_Struct*) pack->pBuffer;
			if (pack->size == sizeof(ServerGenericWorldQuery_Struct))
				zoneserver_list.SendCLEList(sgwq->admin, sgwq->from, this);
			else
				zoneserver_list.SendCLEList(sgwq->admin, sgwq->from, this, sgwq->query);
			break;
		}
		case ServerOP_LockZone: {
			if (pack->size < sizeof(ServerLockZone_Struct)) {
				cout << "Wrong size on ServerOP_Uptime. Got: " << pack->size << ", Expected: " << sizeof(ServerLockZone_Struct) << endl;
				break;
			}
			ServerLockZone_Struct* s = (ServerLockZone_Struct*) pack->pBuffer;
			switch (s->op) {
				case 0:
					zoneserver_list.ListLockedZones(s->adminname, this);
					break;
				case 1:
					if (zoneserver_list.SetLockedZone(s->zoneID, true))
						zoneserver_list.SendEmoteMessage(0, 0, 80, 15, "Zone locked: %s", database.GetZoneName(s->zoneID));
					else
						this->SendEmoteMessageRaw(s->adminname, 0, 0, 0, "Failed to change lock");
					break;
				case 2:
					if (zoneserver_list.SetLockedZone(s->zoneID, false))
						zoneserver_list.SendEmoteMessage(0, 0, 80, 15, "Zone unlocked: %s", database.GetZoneName(s->zoneID));
					else
						this->SendEmoteMessageRaw(s->adminname, 0, 0, 0, "Failed to change lock");
					break;
			}
			break;
		}
		case ServerOP_ItemStatus: {
			zoneserver_list.SendPacket(pack);
			break;
		}
		case ServerOP_GWLocation:
			{
			zoneserver_list.SendPacket(pack);
			break;
			}
		case ServerOP_OOCMute: {
			zoneserver_list.SendPacket(pack);
			break;
		}
		case ServerOP_Revoke: {
			RevokeStruct* rev = (RevokeStruct*)pack->pBuffer;
			ClientListEntry* cle = zoneserver_list.FindCharacter(rev->name);
			if (cle != 0 && cle->Server() != 0)
			{
#if DEBUG >= 6
				zoneserver_list.SendEmoteMessage(rev->adminname, 0, 0, 0, "World: Passing revoke to zone %s", cle->Server()->GetZoneName());
#endif
				cle->Server()->SendPacket(pack);
			}
#if DEBUG >= 6
			else
			{
				zoneserver_list.SendEmoteMessage(rev->adminname, 0, 0, 0, "World: Can't find %s", rev->name);
			}
#endif
			break;
		}
		default:
		{
			cout << "Unknown ServerOPcode:" << (int)pack->opcode;
			cout << " size:" << pack->size << endl;
			DumpPacket(pack->pBuffer, pack->size);
			break;
		}
		}

		delete pack;
	}    
	return true;
}

void ZSList::ShowUpTime(WorldTCPConnection* con, const char* adminname) {
	int32 ms = Timer::GetCurrentTime();
	int32 d = ms / 86400000;
	ms -= d * 86400000;
	int32 h = ms / 3600000;
	ms -= h * 3600000;
	int32 m = ms / 60000;
	ms -= m * 60000;
	int32 s = ms / 1000;
	if (d)
		con->SendEmoteMessage(adminname, 0, 0, 0, "Worldserver Uptime: %02id %02ih %02im %02is", d, h, m, s);
	else if (h)
		con->SendEmoteMessage(adminname, 0, 0, 0, "Worldserver Uptime: %02ih %02im %02is", h, m, s);
	else
		con->SendEmoteMessage(adminname, 0, 0, 0, "Worldserver Uptime: %02im %02is", m, s);
}

ZSList::ZSList() {
	NextID = 1;
	NextCLEID = 1;
	CLStale_timer = new Timer(45000);
	CurGroupID = 1;
}

ZSList::~ZSList() {
	safe_delete(CLStale_timer);
}

void ZSList::Add(ZoneServer* zoneserver) {
	list.Insert(zoneserver);
	zoneserver->SendGroupIDs();	//send its initial set of group ids
}

void ZSList::KillAll() {
	LinkedListIterator<ZoneServer*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		iterator.GetData()->Disconnect();
		iterator.RemoveCurrent();
		numzones--;
	}
}

void ZSList::Process()
{
	if (CLStale_timer->Check())
		CLCheckStale();
	if(shutdowntimer && shutdowntimer->Check()){
		ServerPacket* pack2 = new ServerPacket;
		pack2->opcode = ServerOP_ShutdownAll;
		pack2->size=0;
		zoneserver_list.SendPacket(pack2);
		safe_delete(pack2);
		zoneserver_list.Process();
		CatchSignal(0);
	}
	if(reminder && reminder->Check()){
		SendEmoteMessage(0,0,0,15,"<SYSTEMWIDE MESSAGE>:SYSTEM MSG:World coming down, everyone log out now.  World will shut down in %i seconds...",shutdowntimer->GetRemainingTime()/1000);
	}
	LinkedListIterator<ZoneServer*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (!iterator.GetData()->Process()) {
			struct in_addr  in;
			in.s_addr = iterator.GetData()->GetIP();
			cout << "Removing zoneserver from ip:" << inet_ntoa(in) << " port:" << (int16)(iterator.GetData()->GetPort()) << " (" << iterator.GetData()->GetCAddress() << ":" << iterator.GetData()->GetCPort() << ")" << endl;
			if (holdzones){
				cout << "Hold Zones mode is ON - rebooting lost zone" << endl;
				if(!iterator.GetData()->IsStaticZone())
					zoneserver_list.RebootZone(inet_ntoa(in),iterator.GetData()->GetCPort(),iterator.GetData()->GetCAddress(),iterator.GetData()->GetID());
				else
					zoneserver_list.RebootZone(inet_ntoa(in),iterator.GetData()->GetCPort(),iterator.GetData()->GetCAddress(),iterator.GetData()->GetID(),database.GetZoneID(iterator.GetData()->GetZoneName()));
			}

			iterator.RemoveCurrent();
			numzones--;
		}
		else {
			iterator.Advance();
		}
	}
}

bool ZSList::SendPacket(ServerPacket* pack) {
/*	LinkedListIterator<ZoneServer*> iterator(list);

	TCPConnection::TCPNetPacket_Struct* tnps = TCPConnection::MakePacket(pack);
	iterator.Reset();
	while(iterator.MoreElements()) {
		iterator.GetData()->SendPacket(tnps);
		iterator.Advance();
	}
	delete tnps;*/
	tcps.SendPacket(pack);
	return true;
}

bool ZSList::SendPacket(const char* to, ServerPacket* pack) {
	if (to == 0 || to[0] == 0) {
		zoneserver_list.SendPacket(pack);
		return true;
	}
	else if (to[0] == '*') {
		// Cant send a packet to a console....
		return false;
	}
	else {
		ClientListEntry* cle = zoneserver_list.FindCharacter(to);
		if (cle != 0) {
			if (cle->Server() != 0) {
				cle->Server()->SendPacket(pack);
				return true;
			}
			return false;
		}
		else {
			ZoneServer* zs = zoneserver_list.FindByName(to);
			if (zs != 0) {
				zs->SendPacket(pack);
				return true;
			}
			return false;
		}
	}
	return false;
}

ZoneServer* ZSList::FindByName(const char* zonename) {
	LinkedListIterator<ZoneServer*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (strcasecmp(iterator.GetData()->GetZoneName(), zonename) == 0) {
			ZoneServer* tmp = iterator.GetData();
			return tmp;
		}
		iterator.Advance();
	}
	return 0;
}

ZoneServer* ZSList::FindByID(int32 ZoneID) {
	LinkedListIterator<ZoneServer*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->GetID() == ZoneID) {
			ZoneServer* tmp = iterator.GetData();
			return tmp;
		}
		iterator.Advance();
	}
	return 0;
}

ZoneServer* ZSList::FindByZoneID(int32 ZoneID) {
	LinkedListIterator<ZoneServer*> iterator(list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GetZoneID()==ZoneID) {
			ZoneServer* tmp = iterator.GetData();
			return tmp;
		}
		iterator.Advance();
	}
	return 0;
}

ZoneServer* ZSList::FindByPort(int16 port) {
	LinkedListIterator<ZoneServer*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GetCPort() == port) {
			ZoneServer* tmp = iterator.GetData();
			return tmp;
		}
		iterator.Advance();
	}
	return 0;
}

bool ZSList::SetLockedZone(int16 iZoneID, bool iLock) {
	for (int i=0; i<MaxLockedZones; i++) {
		if (iLock) {
			if (pLockedZones[i] == 0) {
				pLockedZones[i] = iZoneID;
				return true;
			}
		}
		else {
			if (pLockedZones[i] == iZoneID) {
				pLockedZones[i] = 0;
				return true;
			}
		}
	}
	return false;
}

bool ZSList::IsZoneLocked(int16 iZoneID) {
	for (int i=0; i<MaxLockedZones; i++) {
		if (pLockedZones[i] == iZoneID)
			return true;
	}
	return false;
}

void ZSList::ListLockedZones(const char* to, WorldTCPConnection* connection) {
	int x = 0;
	for (int i=0; i<MaxLockedZones; i++) {
		if (pLockedZones[i]) {
			connection->SendEmoteMessageRaw(to, 0, 0, 0, database.GetZoneName(pLockedZones[i], true));
			x++;
		}
	}
	connection->SendEmoteMessage(to, 0, 0, 0, "%i zones locked.", x);
}

void ZSList::SendZoneStatus(const char* to, sint16 admin, WorldTCPConnection* connection) {
	LinkedListIterator<ZoneServer*> iterator(list);
	struct in_addr  in;
	
	iterator.Reset();
	char locked[4];
	if (net.world_locked == true)
		strcpy(locked, "Yes");
	else
		strcpy(locked, "No");

	char* output = 0;
	int32 outsize = 0, outlen = 0;
	if (connection->IsConsole())
		AppendAnyLenString(&output, &outsize, &outlen, "World Locked: %s\r\n", locked);
	else
		AppendAnyLenString(&output, &outsize, &outlen, "World Locked: %s^", locked);
	if (connection->IsConsole())
		AppendAnyLenString(&output, &outsize, &outlen, "Zoneservers online:\r\n");
	else
		AppendAnyLenString(&output, &outsize, &outlen, "Zoneservers online:^");
//	connection->SendEmoteMessage(to, 0, 0, 0, "World Locked: %s", locked);
//	connection->SendEmoteMessage(to, 0, 0, 0, "Zoneservers online:");
	int v=0, w=0, x=0, y=0, z=0;
	char tmpStatic[2] = { 0, 0 }, tmpZone[64];
	memset(tmpZone, 0, sizeof(tmpZone));
	ZoneServer* zs = 0;
	while(iterator.MoreElements()) {
		zs = iterator.GetData();
		in.s_addr = zs->GetIP();

		if(zs->IsStaticZone())
			z++;
		else if (zs->GetZoneID() != 0)
			w++;
		else if(zs->GetZoneID() == 0 && !zs->IsBootingUp())
			v++;

		if (zs->IsStaticZone())
			tmpStatic[0] = 'S';
		else
			tmpStatic[0] = ' ';

		if (admin >= 150) {
			if (zs->GetZoneID())
				snprintf(tmpZone, sizeof(tmpZone), "%s (%i)", zs->GetZoneName(), zs->GetZoneID());
			else if (zs->IsBootingUp())
				strcpy(tmpZone, "...");
			else
				tmpZone[0] = 0;

			AppendAnyLenString(&output, &outsize, &outlen, "  #%-3i %s %15s:%-5i %2i  %s:%i  %s", zs->GetID(), tmpStatic, inet_ntoa(in), zs->GetPort(), zs->NumPlayers(), zs->GetCAddress(), zs->GetCPort(), tmpZone);
			if (outlen >= 3584) {
				connection->SendEmoteMessageRaw(to, 0, 0, 10, output);
				safe_delete(output);
				outsize = 0;
				outlen = 0;
			}
			else {
				if (connection->IsConsole())
					AppendAnyLenString(&output, &outsize, &outlen, "\r\n");
				else
					AppendAnyLenString(&output, &outsize, &outlen, "^");
			}
			x++;
		}
		else if (zs->GetZoneID() != 0) {
			if (zs->GetZoneID())
				strcpy(tmpZone, zs->GetZoneName());
			else
				tmpZone[0] = 0;
			AppendAnyLenString(&output, &outsize, &outlen, "  #%i %s  %s", zs->GetID(), tmpStatic, tmpZone);
			if (outlen >= 3584) {
				connection->SendEmoteMessageRaw(to, 0, 0, 10, output);
				safe_delete(output);
				outsize = 0;
				outlen = 0;
			}
			else {
				if (connection->IsConsole())
					AppendAnyLenString(&output, &outsize, &outlen, "\r\n");
				else
					AppendAnyLenString(&output, &outsize, &outlen, "^");
			}
			x++;
		}
		y++;
		iterator.Advance();
	}
	if (connection->IsConsole())
		AppendAnyLenString(&output, &outsize, &outlen, "%i servers listed. %i servers online.\r\n", x, y);
	else
		AppendAnyLenString(&output, &outsize, &outlen, "%i servers listed. %i servers online.^", x, y);
	AppendAnyLenString(&output, &outsize, &outlen, "%i zones are static zones, %i zones are booted zones, %i zones available.",z,w,v);
//	connection->SendEmoteMessage(to, 0, 0, "%i servers listed. %i servers online.", x, y);
//	connection->SendEmoteMessage(to,0,0,"%i zones are static zones, %i zones are booted zones, %i zones available.",z,w,v);
	if (output)
		connection->SendEmoteMessageRaw(to, 0, 0, 10, output);
	safe_delete(output);
}

void ZSList::SendChannelMessage(const char* from, const char* to, int8 chan_num, int8 language, const char* message, ...) {
	if (!message)
		return;
	va_list argptr;
	char buffer[1024];

	va_start(argptr, message);
	vsnprintf(buffer, sizeof(buffer), message, argptr);
	va_end(argptr);

	SendChannelMessageRaw(from, to, chan_num, language, buffer);
}

void ZSList::SendChannelMessageRaw(const char* from, const char* to, int8 chan_num, int8 language, const char* message) {
	if (!message)
		return;
	ServerPacket* pack = new ServerPacket;

	pack->opcode = ServerOP_ChannelMessage;
	pack->size = sizeof(ServerChannelMessage_Struct)+strlen(message)+1;
	pack->pBuffer = new uchar[pack->size];
	memset(pack->pBuffer, 0, pack->size);
	ServerChannelMessage_Struct* scm = (ServerChannelMessage_Struct*) pack->pBuffer;
	if (from == 0) {
		strcpy(scm->from, "WServer");
		scm->noreply = true;
	}
	else if (from[0] == 0) {
		strcpy(scm->from, "WServer");
		scm->noreply = true;
	}
	else
		strcpy(scm->from, from);
	if (to != 0) {
		strcpy((char *) scm->to, to);
		strcpy((char *) scm->deliverto, to);
	}
	else {
		scm->to[0] = 0;
		scm->deliverto[0] = 0;
	}

	scm->language = language;
	scm->chan_num = chan_num;
	strcpy(&scm->message[0], message);
	if (scm->chan_num == 5 || scm->chan_num == 6 || scm->chan_num == 11) {
		console_list.SendChannelMessage(scm);
	}
	pack->Deflate();
	zoneserver_list.SendPacket(pack);
	delete pack;
}

void ZoneServer::SendEmoteMessage(const char* to, int32 to_guilddbid, sint16 to_minstatus, int32 type, const char* message, ...) {
	if (!message)
		return;
	va_list argptr;
	char buffer[1024];

	va_start(argptr, message);
	vsnprintf(buffer, sizeof(buffer), message, argptr);
	va_end(argptr);
	SendEmoteMessageRaw(to, to_guilddbid, to_minstatus, type, buffer);
}

void ZoneServer::SendEmoteMessageRaw(const char* to, int32 to_guilddbid, sint16 to_minstatus, int32 type, const char* message) {
	if (!message)
		return;
	ServerPacket* pack = new ServerPacket;

	pack->opcode = ServerOP_EmoteMessage;
	pack->size = sizeof(ServerEmoteMessage_Struct)+strlen(message)+1;
	pack->pBuffer = new uchar[pack->size];
	memset(pack->pBuffer, 0, pack->size);
	ServerEmoteMessage_Struct* sem = (ServerEmoteMessage_Struct*) pack->pBuffer;
	
	if (to != 0) {
		strcpy((char *) sem->to, to);
	}
	else {
		sem->to[0] = 0;
	}

	sem->guilddbid = to_guilddbid;
	sem->minstatus = to_minstatus;
	sem->type = type;
	strcpy(&sem->message[0], message);
	
	pack->Deflate();
	this->SendPacket(pack);
	delete pack;
}

void ZSList::SendEmoteMessage(const char* to, int32 to_guilddbid, sint16 to_minstatus, int32 type, const char* message, ...) {
	if (!message)
		return;
	va_list argptr;
	char buffer[1024];

	va_start(argptr, message);
	vsnprintf(buffer, sizeof(buffer), message, argptr);
	va_end(argptr);

	SendEmoteMessageRaw(to, to_guilddbid, to_minstatus, type, buffer);
}

void ZSList::SendEmoteMessageRaw(const char* to, int32 to_guilddbid, sint16 to_minstatus, int32 type, const char* message) {
	if (!message)
		return;
	ServerPacket* pack = new ServerPacket;

	pack->opcode = ServerOP_EmoteMessage;
	pack->size = sizeof(ServerEmoteMessage_Struct)+strlen(message)+1;
	pack->pBuffer = new uchar[pack->size];
	memset(pack->pBuffer, 0, pack->size);
	ServerEmoteMessage_Struct* sem = (ServerEmoteMessage_Struct*) pack->pBuffer;
	
	if (to) {
		if (to[0] == '*') {
			Console* con = console_list.FindByAccountName(&to[1]);
			if (con)
				con->SendEmoteMessageRaw(to, to_guilddbid, to_minstatus, type, message);
			delete pack;
			return;
		}
		strcpy((char *) sem->to, to);
	}
	else {
		sem->to[0] = 0;
	}

	sem->guilddbid = to_guilddbid;
	sem->minstatus = to_minstatus;
	sem->type = type;
	strcpy(&sem->message[0], message);
	char tempto[64]={0};
	if(to)
		strncpy(tempto,to,64);
	pack->Deflate();
	if (tempto[0] == 0) {
		zoneserver_list.SendPacket(pack);
		if (sem->guilddbid == 0)
			console_list.SendEmoteMessageRaw(type, message);
	}
	else {
		ZoneServer* zs = zoneserver_list.FindByName(to);

		if (zs != 0)
			zs->SendPacket(pack);
		else
			zoneserver_list.SendPacket(pack);
	}
	delete pack;
}

void ZSList::SendTimeSync() {
	ServerPacket* pack = new ServerPacket(ServerOP_SyncWorldTime, sizeof(eqTimeOfDay));
	eqTimeOfDay* tod = (eqTimeOfDay*) pack->pBuffer;
	tod->start_eqtime=worldclock.getStartEQTime();
	tod->start_realtime=worldclock.getStartRealTime();
	zoneserver_list.SendPacket(pack);
	delete pack;
}

void ZSList::NextGroupIDs(int32 &start, int32 &end) {
	start = CurGroupID;
	CurGroupID += 1000;	//hand them out 1000 at a time...
	if(CurGroupID < start) {	//handle overflow
		start = 1;
		CurGroupID = 1001;
	}
	end = CurGroupID - 1;
}

void ZoneServer::SendGroupIDs() {
	ServerPacket* pack = new ServerPacket(ServerOP_GroupIDReply, sizeof(ServerGroupIDReply_Struct));
	ServerGroupIDReply_Struct* sgi = (ServerGroupIDReply_Struct*)pack->pBuffer;
	zoneserver_list.NextGroupIDs(sgi->start, sgi->end);
	SendPacket(pack);
}

void ZoneServer::ChangeWID(int32 iCharID, int32 iWID) {
	ServerPacket* pack = new ServerPacket(ServerOP_ChangeWID, sizeof(ServerChangeWID_Struct));
	ServerChangeWID_Struct* scw = (ServerChangeWID_Struct*) pack->pBuffer;
	scw->charid = iCharID;
	scw->newwid = iWID;
	zoneserver_list.SendPacket(pack);
	delete pack;
}

void ZSList::CLERemoveZSRef(ZoneServer* iZS) {
	LinkedListIterator<ClientListEntry*> iterator(clientlist);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->Server() == iZS) {
#ifdef GUILDWARS
			if(iterator.GetData()->Server()->GetZoneID() != 0)
			database.UpdateZoneOnlineStatus(iterator.GetData()->Server()->GetZoneID(),0);
#endif
			iterator.GetData()->ClearServer(); // calling this before LeavingZone() makes CLE not update the number of players in a zone
			iterator.GetData()->LeavingZone();
		}
		iterator.Advance();
	}
}

ClientListEntry* ZSList::GetCLE(int32 iID) {
	LinkedListIterator<ClientListEntry*> iterator(clientlist);


	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->GetID() == iID) {
			return iterator.GetData();
		}
		iterator.Advance();
	}
	return 0;
}

ClientListEntry* ZSList::FindCharacter(const char* name) {
	LinkedListIterator<ClientListEntry*> iterator(clientlist);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (strcasecmp(iterator.GetData()->name(), name) == 0) {
			return iterator.GetData();
		}
		iterator.Advance();
	}
	return 0;
}


ClientListEntry* ZSList::FindCLEByAccountID(int32 iAccID) {
	LinkedListIterator<ClientListEntry*> iterator(clientlist);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->AccountID() == iAccID) {
			return iterator.GetData();
		}
		iterator.Advance();
	}
	return 0;
}


void ZSList::SendWhoAll(int32 fromid,const char* to, sint16 admin, Who_All_Struct* whom, WorldTCPConnection* connection) {
	try{
	LinkedListIterator<ClientListEntry*> iterator(clientlist);
	LinkedListIterator<ClientListEntry*> countclients(clientlist);
	ClientListEntry* cle = 0;
	ClientListEntry* countcle = 0;
	//char tmpgm[25] = "";
	//char accinfo[150] = "";
	char line[300] = "";
	//char tmpguild[50] = "";
	//char LFG[10] = "";
	//int32 x = 0;
	int whomlen = 0;
	if (whom)
		whomlen = strlen(whom->whom);

	char* output = 0;
	int32 outsize = 0, outlen = 0;
	int32 totalusers=0;
	int32 totallength=0;
	AppendAnyLenString(&output, &outsize, &outlen, "Players on server:");
	if (connection->IsConsole())
		AppendAnyLenString(&output, &outsize, &outlen, "\r\n");
	else
		AppendAnyLenString(&output, &outsize, &outlen, "\n");
	countclients.Reset();
	while(countclients.MoreElements()){
		countcle = countclients.GetData();
		const char* tmpZone = database.GetZoneName(countcle->zone());
		if (
	(countcle->Online() >= CLE_Status_Zoning)
	&& (whom == 0 || (
		((countcle->Admin() >= 80 && countcle->GetGM()) || whom->gmlookup == 0xFFFF) && 
		(whom->lvllow == 0xFFFF || (countcle->level() >= whom->lvllow && countcle->level() <= whom->lvlhigh && (countcle->Anon()==0 || admin > countcle->Admin()))) && 
		(whom->wclass == 0xFFFF || (countcle->class_() == whom->wclass && (countcle->Anon()==0 || admin > countcle->Admin()))) && 
		(whom->wrace == 0xFFFF || (countcle->race() == whom->wrace && (countcle->Anon()==0 || admin > countcle->Admin()))) &&
		(whomlen == 0 || (
			(tmpZone != 0 && strncasecmp(tmpZone, whom->whom, whomlen) == 0) || 
			strncasecmp(countcle->name(),whom->whom, whomlen) == 0 || 
			(countcle->GuildEQID() != 0xFFFFFFFF && strncasecmp(guilds[countcle->GuildEQID()].name, whom->whom, whomlen) == 0) || 
			(admin >= 100 && strncasecmp(countcle->AccountName(), whom->whom, whomlen) == 0)
		))
	))
) {
			if((countcle->Anon()>0 && admin>=countcle->Admin() && admin>0) || countcle->Anon()==0 ){
				totalusers++;
				if(totalusers<=20 || admin>=100)
					totallength=totallength+strlen(countcle->name())+strlen(countcle->AccountName())+strlen(guilds[countcle->GuildEQID()].name)+5;
			}
			else if((countcle->Anon()>0 && admin<=countcle->Admin()) || countcle->Anon()==0 && !countcle->GetGM()){
				totalusers++;
				if(totalusers<=20 || admin>=100)
					totallength=totallength+strlen(countcle->name())+strlen(guilds[countcle->GuildEQID()].name)+5;
			}
		}
		countclients.Advance();
	}
	int32 plid=fromid;
	int32 playerineqstring=5001;
	const char line2[]="---------------------------";
	int8 unknown35=0x0A;
	int32 unknown36=0;
	int32 playersinzonestring=5028;
	if(totalusers>20 && admin<100){
		totalusers=20;
		playersinzonestring=5033;
	}
	else if(totalusers>1)
		playersinzonestring=5036;
	int32 unknown44[2];
	unknown44[0]=0;
	unknown44[1]=0;
	int32 unknown52=totalusers;
	int32 unknown56=1;
	ServerPacket* pack2 = new ServerPacket(0x2010,64+totallength+(49*totalusers));
	memset(pack2->pBuffer,0,pack2->size);
	uchar *buffer=pack2->pBuffer;
	uchar *bufptr=buffer;
	//memset(buffer,0,pack2->size);
	memcpy(bufptr,&plid, sizeof(int32));
	bufptr+=sizeof(int32);
	memcpy(bufptr,&playerineqstring, sizeof(int32));
	bufptr+=sizeof(int32);
	memcpy(bufptr,&line2, strlen(line2));
	bufptr+=strlen(line2);
	memcpy(bufptr,&unknown35, sizeof(int8));
	bufptr+=sizeof(int8);
	memcpy(bufptr,&unknown36, sizeof(int32));
	bufptr+=sizeof(int32);
	memcpy(bufptr,&playersinzonestring, sizeof(int32));
	bufptr+=sizeof(int32);
	memcpy(bufptr,&unknown44[0], sizeof(int32));
	bufptr+=sizeof(int32);
	memcpy(bufptr,&unknown44[1], sizeof(int32));
	bufptr+=sizeof(int32);
	memcpy(bufptr,&unknown52, sizeof(int32));
	bufptr+=sizeof(int32);
	memcpy(bufptr,&unknown56, sizeof(int32));
	bufptr+=sizeof(int32);
	memcpy(bufptr,&totalusers, sizeof(int32));
	bufptr+=sizeof(int32);

	iterator.Reset();
	int idx=-1;
	while(iterator.MoreElements()) {
		cle = iterator.GetData();
		const char* tmpZone = database.GetZoneName(cle->zone());
		if (
	(cle->Online() >= CLE_Status_Zoning)
	&& (whom == 0 || (
		((cle->Admin() >= 80 && cle->GetGM()) || whom->gmlookup == 0xFFFF) && 
		(whom->lvllow == 0xFFFF || (cle->level() >= whom->lvllow && cle->level() <= whom->lvlhigh && (cle->Anon()==0 || admin>cle->Admin()))) && 
		(whom->wclass == 0xFFFF || (cle->class_() == whom->wclass && (cle->Anon()==0 || admin>cle->Admin()))) && 
		(whom->wrace == 0xFFFF || (cle->race() == whom->wrace && (cle->Anon()==0 || admin>cle->Admin()))) &&
		(whomlen == 0 || (
			(tmpZone != 0 && strncasecmp(tmpZone, whom->whom, whomlen) == 0) || 
			strncasecmp(cle->name(),whom->whom, whomlen) == 0 || 
			(cle->GuildEQID() != 0xFFFFFFFF && strncasecmp(guilds[cle->GuildEQID()].name, whom->whom, whomlen) == 0) || 
			(admin >= 100 && strncasecmp(cle->AccountName(), whom->whom, whomlen) == 0)
		))
	))
) {
			line[0] = 0;
			int32 rankstring=0xFFFFFFFF;
				if(cle->Anon()==1 && cle->Admin()>admin && !cle->GetGM() || (idx>=20 && admin<100)){ //hide gms that are anon from lesser gms and normal players, cut off at 20
					rankstring=0;
					iterator.Advance();
					continue;
				}
				else if (cle->Admin() >=250)
					rankstring=5021;
				else if (cle->Admin() >= 200)
					rankstring=5020;
				else if (cle->Admin() >= 180)
					rankstring=5019;
				else if (cle->Admin() >= 170)
					rankstring=5018;
				else if (cle->Admin() >= 160)
					rankstring=5017;
				else if (cle->Admin() >= 150)
					rankstring=5016;
				else if (cle->Admin() >= 100)
					rankstring=5015;
				else if (cle->Admin() >= 95)
					rankstring=5014;
				else if (cle->Admin() >= 90)
					rankstring=5013;
				else if (cle->Admin() >= 85)
					rankstring=5012;
				else if (cle->Admin() >= 81)
					rankstring=5011;
				else if (cle->Admin() >= 80)
					rankstring=5010;
				else if (cle->Admin() >= 50)
					rankstring=5009;
				else if (cle->Admin() >= 20)
					rankstring=5008;
				else if (cle->Admin() >= 10)
					rankstring=5007;
			idx++;
			char guildbuffer[67]={0};
			if (cle->GuildEQID() < 0xFFFFFFFF && cle->GuildEQID()>0)
				sprintf(guildbuffer,"<%s>",guilds[cle->GuildEQID()].name);
			int32 formatstring=5025;
			if(cle->Anon()==1 && (admin<cle->Admin() || admin==0))
				formatstring=5024;
			else if(cle->Anon()==1 && admin>=cle->Admin() && admin>0)
				formatstring=5022;
			else if(cle->Anon()==2 && (admin<cle->Admin() || admin==0))
				formatstring=5023;//display guild
			else if(cle->Anon()==2 && admin>=cle->Admin() && admin>0)
				formatstring=5022;//display everything
	
	//war* wars2 = (war*)pack2->pBuffer;

	int32 plclass_=0;
	int32 pllevel=0;
	int32 pidstring=0xFFFFFFFF;//5003;
	int32 plrace=0;
	int32 zonestring=0xFFFFFFFF;
	int32 plzone=0;
	int32 unknown80[2];
	if(cle->Anon()==0 || (admin>=cle->Admin() && admin>0)){
		plclass_=cle->class_();
		pllevel=cle->level();
		if(admin>=100)
			pidstring=5003;
		plrace=cle->race();
		zonestring=5006;
		plzone=cle->zone();
	}
	
	
	if(admin>=cle->Admin() && admin>0)
		unknown80[0]=cle->Admin();
	else
		unknown80[0]=0xFFFFFFFF;
	unknown80[1]=0xFFFFFFFF;//1035
	

	//char plstatus[20]={0};
	//sprintf(plstatus, "Status %i",cle->Admin());
	char plname[64]={0};
	strcpy(plname,cle->name());
	
	char placcount[30]={0};
	if(admin>=cle->Admin() && admin>0)
		strcpy(placcount,cle->AccountName());

	memcpy(bufptr,&formatstring, sizeof(int32));
	bufptr+=sizeof(int32);
	memcpy(bufptr,&pidstring, sizeof(int32));
	bufptr+=sizeof(int32);
	memcpy(bufptr,&plname, strlen(plname)+1);
	bufptr+=strlen(plname)+1;
	memcpy(bufptr,&rankstring, sizeof(int32));
	bufptr+=sizeof(int32);
	memcpy(bufptr,&guildbuffer, strlen(guildbuffer)+1);
	bufptr+=strlen(guildbuffer)+1;
	memcpy(bufptr,&unknown80[0], sizeof(int32));
	bufptr+=sizeof(int32);
	memcpy(bufptr,&unknown80[1], sizeof(int32));
	bufptr+=sizeof(int32);
	memcpy(bufptr,&zonestring, sizeof(int32));
	bufptr+=sizeof(int32);
	memcpy(bufptr,&plzone, sizeof(int32));
	bufptr+=sizeof(int32);
	memcpy(bufptr,&plclass_, sizeof(int32));
	bufptr+=sizeof(int32);
	memcpy(bufptr,&pllevel, sizeof(int32));
	bufptr+=sizeof(int32);
	memcpy(bufptr,&plrace, sizeof(int32));
	bufptr+=sizeof(int32);
	int32 ending=0;
	memcpy(bufptr,&placcount, strlen(placcount)+1);
	bufptr+=strlen(placcount)+1;
	ending=207;
	memcpy(bufptr,&ending, sizeof(int32));
	bufptr+=sizeof(int32);
		}
		iterator.Advance();
	}
	pack2->Deflate();
	//zoneserver_list.SendPacket(pack2); // NO NO NO WHY WOULD YOU SEND IT TO EVERY ZONE SERVER?!?
	zoneserver_list.SendPacket(to,pack2);
	safe_delete(pack2);
	safe_delete(output);
	}
	catch(...){
		printf("Unknown error in world's SendWhoAll (probably mem error), ignoring...\n");
		return;
	}
}

void ZSList::ConsoleSendWhoAll(const char* to, sint16 admin, Who_All_Struct* whom, WorldTCPConnection* connection) {
	LinkedListIterator<ClientListEntry*> iterator(clientlist);
	ClientListEntry* cle = 0;
	char tmpgm[25] = "";
	char accinfo[150] = "";
	char line[300] = "";
	char tmpguild[50] = "";
	char LFG[10] = "";
	int32 x = 0;
	int whomlen = 0;
	if (whom)
		whomlen = strlen(whom->whom);

	char* output = 0;
	int32 outsize = 0, outlen = 0;
	AppendAnyLenString(&output, &outsize, &outlen, "Players on server:");
	if (connection->IsConsole())
		AppendAnyLenString(&output, &outsize, &outlen, "\r\n");
	else
		AppendAnyLenString(&output, &outsize, &outlen, "\n");
	iterator.Reset();
	while(iterator.MoreElements()) {
		cle = iterator.GetData();
		const char* tmpZone = database.GetZoneName(cle->zone());
		if (
	(cle->Online() >= CLE_Status_Zoning)
	&& (whom == 0 || (
		((cle->Admin() >= 80 && cle->GetGM()) || whom->gmlookup == 0xFFFF) && 
		(whom->lvllow == 0xFFFF || (cle->level() >= whom->lvllow && cle->level() <= whom->lvlhigh)) && 
		(whom->wclass == 0xFFFF || cle->class_() == whom->wclass) && 
		(whom->wrace == 0xFFFF || cle->race() == whom->wrace) && 
		(whomlen == 0 || (
			(tmpZone != 0 && strncasecmp(tmpZone, whom->whom, whomlen) == 0) || 
			strncasecmp(cle->name(),whom->whom, whomlen) == 0 || 
			(cle->GuildEQID() != 0xFFFFFFFF && strncasecmp(guilds[cle->GuildEQID()].name, whom->whom, whomlen) == 0) || 
			(admin >= 100 && strncasecmp(cle->AccountName(), whom->whom, whomlen) == 0)
		))
	))
) {
			line[0] = 0;
// MYRA - use new (5.x) Status labels in who for telnet connection			
			if (cle->Admin() >=250)
				strcpy(tmpgm, "* GM-Impossible * ");
			else if (cle->Admin() >= 200)
				strcpy(tmpgm, "* GM-Mgmt * ");
			else if (cle->Admin() >= 180)
				strcpy(tmpgm, "* GM-Coder * ");
			else if (cle->Admin() >= 170)
				strcpy(tmpgm, "* GM-Areas * ");
			else if (cle->Admin() >= 160)
				strcpy(tmpgm, "* QuestMaster * ");
			else if (cle->Admin() >= 150)
				strcpy(tmpgm, "* GM-Lead Admin * ");
			else if (cle->Admin() >= 100)
				strcpy(tmpgm, "* GM-Admin * ");
			else if (cle->Admin() >= 95)
				strcpy(tmpgm, "* GM-Staff * ");
			else if (cle->Admin() >= 90)
				strcpy(tmpgm, "* EQ Support * ");
			else if (cle->Admin() >= 85)
				strcpy(tmpgm, "* GM-Tester * ");
			else if (cle->Admin() >= 81)
				strcpy(tmpgm, "* Senior Guide * ");
			else if (cle->Admin() >= 80)
				strcpy(tmpgm, "* QuestTroupe * ");
			else if (cle->Admin() >= 50)
				strcpy(tmpgm, "* Guide * ");
			else if (cle->Admin() >= 20)
				strcpy(tmpgm, "* Apprentice Guide * ");
			else if (cle->Admin() >= 10)
				strcpy(tmpgm, "* Steward * ");
			else
				tmpgm[0] = 0;
// end Myra		

			if (cle->GuildEQID() != 0xFFFFFFFF) {
				if (guilds[cle->GuildEQID()].databaseID == 0)
					tmpguild[0] = 0;
				else
					snprintf(tmpguild, 36, " <%s>", guilds[cle->GuildEQID()].name);
			}
			else
				tmpguild[0] = 0;

			if (cle->LFG())
				strcpy(LFG, " LFG");
			else
				LFG[0] = 0;

			if (admin >= 150 && admin >= cle->Admin()) {
				sprintf(accinfo, " AccID: %i AccName: %s LSID: %i Status: %i", cle->AccountID(), cle->AccountName(), cle->LSAccountID(), cle->Admin());
			}
			else
				accinfo[0] = 0;

			if (cle->Anon() == 2) { // Roleplay
				if (admin >= 100 && admin >= cle->Admin())
					sprintf(line, "  %s[RolePlay %i %s] %s (%s)%s zone: %s%s%s", tmpgm, cle->level(), GetEQClassName(cle->class_(),cle->level()), cle->name(), GetRaceName(cle->race()), tmpguild, tmpZone, LFG, accinfo);
				else if (cle->Admin() >= 80 && admin < 80 && cle->GetGM()) {
					iterator.Advance();
					continue;
				}
				else
					sprintf(line, "  %s[ANONYMOUS] %s%s%s%s", tmpgm, cle->name(), tmpguild, LFG, accinfo);
			}
			else if (cle->Anon() == 1) { // Anon
				if (admin >= 100 && admin >= cle->Admin())
					sprintf(line, "  %s[ANON %i %s] %s (%s)%s zone: %s%s%s", tmpgm, cle->level(), GetEQClassName(cle->class_(),cle->level()), cle->name(), GetRaceName(cle->race()), tmpguild, tmpZone, LFG, accinfo);
				else if (cle->Admin() >= 80 && cle->GetGM()) {
					iterator.Advance();
					continue;
				}
				else
					sprintf(line, "  %s[ANONYMOUS] %s%s%s", tmpgm, cle->name(), LFG, accinfo);
			}
			else
				sprintf(line, "  %s[%i %s] %s (%s)%s zone: %s%s%s", tmpgm, cle->level(), GetEQClassName(cle->class_(),cle->level()), cle->name(), GetRaceName(cle->race()), tmpguild, tmpZone, LFG, accinfo);

			AppendAnyLenString(&output, &outsize, &outlen, line);
			if (outlen >= 3584) {
				connection->SendEmoteMessageRaw(to, 0, 0, 10, output);
				safe_delete(output);
				outsize = 0;
				outlen = 0;
			}
			else {
				if (connection->IsConsole())
					AppendAnyLenString(&output, &outsize, &outlen, "\r\n");
				else
					AppendAnyLenString(&output, &outsize, &outlen, "\n");
			}
			x++;
			if (x >= 20 && admin < 80)
				break;
		}
		iterator.Advance();
	}

	if (x >= 20 && admin < 80)
		AppendAnyLenString(&output, &outsize, &outlen, "too many results...20 players shown");
	else
		AppendAnyLenString(&output, &outsize, &outlen, "%i players online", x);
	if (admin >= 150 && (whom == 0 || whom->gmlookup != 0xFFFF)) {
		if (connection->IsConsole())
			AppendAnyLenString(&output, &outsize, &outlen, "\r\n");
		else
			AppendAnyLenString(&output, &outsize, &outlen, "\n");
		console_list.SendConsoleWho(connection, to, admin, &output, &outsize, &outlen);
	}
	if (output)
		connection->SendEmoteMessageRaw(to, 0, 0, 10, output);
	safe_delete(output);
}


void ZSList::SOPZoneBootup(const char* adminname, int32 ZoneServerID, const char* zonename, bool iMakeStatic) {
	ZoneServer* zs = 0;
	ZoneServer* zs2 = 0;
	int32 zoneid;
	if (!(zoneid = database.GetZoneID(zonename)))
		zoneserver_list.SendEmoteMessage(adminname, 0, 0, 0, "Error: SOP_ZoneBootup: zone '%s' not found in 'zone' table. Typo protection=ON.", zonename);
	else {
		if (ZoneServerID != 0)
			zs = zoneserver_list.FindByID(ZoneServerID);
		else
			zoneserver_list.SendEmoteMessage(adminname, 0, 0, 0, "Error: SOP_ZoneBootup: ServerID must be specified");

		if (zs == 0)
			zoneserver_list.SendEmoteMessage(adminname, 0, 0, 0, "Error: SOP_ZoneBootup: zoneserver not found");
		else {
			zs2 = zoneserver_list.FindByName(zonename);
			if (zs2 != 0)
				zoneserver_list.SendEmoteMessage(adminname, 0, 0, 0, "Error: SOP_ZoneBootup: zone '%s' already being hosted by ZoneServer #%i", zonename, zs2->GetID());
			else {
				zs->TriggerBootup(zoneid, adminname, iMakeStatic);
			}
		}
	}
}

void ZSList::RebootZone(const char* ip1,int16 port,const char* ip2, int32 skipid, int32 zoneid){
// get random zone
	LinkedListIterator<ZoneServer*> iterator(list);
	srand(time(NULL));
	int32 x = 0;
	iterator.Reset();
	while(iterator.MoreElements()) {
		x++;
		iterator.Advance();
	}
	if (x == 0)
		return;
	ZoneServer** tmp = new ZoneServer*[x];
	int32 y = 0;
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (!strcmp(iterator.GetData()->GetCAddress(),ip2) && !iterator.GetData()->IsBootingUp() && iterator.GetData()->GetID() != skipid) {
			tmp[y++] = iterator.GetData();
		}
		iterator.Advance();
	}
	if (y == 0) {
		safe_delete(tmp);
		return;
	}
	int32 z = rand() % y;
	
	ServerPacket* pack = new ServerPacket(ServerOP_ZoneReboot, sizeof(ServerZoneReboot_Struct));
	ServerZoneReboot_Struct* s = (ServerZoneReboot_Struct*) pack->pBuffer;
//	strcpy(s->ip1,ip1);
	strcpy(s->ip2,ip2);
	s->port = port;
	s->zoneid = zoneid;
	if(zoneid != 0)
		printf("Rebooting static zone with the ID of: %i\n",zoneid);
	tmp[z]->SendPacket(pack);
	delete pack;
	safe_delete(tmp);
}

int32 ZSList::TriggerBootup(int32 iZoneID) {
	LinkedListIterator<ZoneServer*> iterator(list);
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->GetZoneID() == 0 && !iterator.GetData()->IsBootingUp()) {
			ZoneServer* zone=iterator.GetData();
			zone->TriggerBootup(iZoneID);
			return zone->GetID();
		}
		iterator.Advance();
	}
	return 0;
	/*Old Random boot zones use this if your server is distributed across computers.
	LinkedListIterator<ZoneServer*> iterator(list);

	srand(time(NULL));
	int32 x = 0;
	iterator.Reset();
	while(iterator.MoreElements()) {
		x++;
		iterator.Advance();
	}
	if (x == 0) {
		return 0;
	}

	ZoneServer** tmp = new ZoneServer*[x];
	int32 y = 0;

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->GetZoneID() == 0 && !iterator.GetData()->IsBootingUp()) {
			tmp[y++] = iterator.GetData();
		}
		iterator.Advance();
	}
	if (y == 0) {
		safe_delete(tmp);
		return 0;
	}

	int32 z = rand() % y;
	
	tmp[z]->TriggerBootup(iZoneID);
	int32 ret = tmp[z]->GetID();
	safe_delete(tmp);
	return ret;
	*/
}

void ZoneServer::TriggerBootup(int32 iZoneID, const char* adminname, bool iMakeStatic) {
	BootingUp = true;
	ServerPacket* pack = new ServerPacket(ServerOP_ZoneBootup, sizeof(ServerZoneStateChange_struct));
	ServerZoneStateChange_struct* s = (ServerZoneStateChange_struct *) pack->pBuffer;
	s->ZoneServerID = ID;
	if (adminname != 0)
		strcpy(s->adminname, adminname);
	
	int32 zoneid = iZoneID;

	if (zoneid == 0)
		zoneid = this->GetZoneID();

	s->zoneid = zoneid;
	s->makestatic = iMakeStatic;
	SendPacket(pack);
	delete pack;
}

void ZoneServer::IncommingClient(Client* client) {
	BootingUp = true;
	ServerPacket* pack = new ServerPacket(ServerOP_ZoneIncClient, sizeof(ServerZoneIncommingClient_Struct));
	ServerZoneIncommingClient_Struct* s = (ServerZoneIncommingClient_Struct*) pack->pBuffer;
	s->zoneid = GetZoneID();
	s->wid = client->GetWID();
	s->ip = client->GetIP();
	s->accid = client->GetAccountID();
	s->admin = client->GetAdmin();
	s->charid = client->GetCharID();
	if (client->GetCLE())
		s->tellsoff = client->GetCLE()->TellsOff();
	strn0cpy(s->charname, client->GetCharName(), sizeof(s->charname));
	strn0cpy(s->lskey, client->GetLSKey(), sizeof(s->lskey));
	SendPacket(pack);
	delete pack;
}

void ZSList::SendCLEList(const sint16& admin, const char* to, WorldTCPConnection* connection, const char* iName) {
	LinkedListIterator<ClientListEntry*> iterator(clientlist);
	char* output = 0;
	int32 outsize = 0, outlen = 0;
	int x = 0, y = 0;
	int namestrlen = iName == 0 ? 0 : strlen(iName);
	bool addnewline = false;
	char newline[3];
	if (connection->IsConsole())
		strcpy(newline, "\r\n");
	else
		strcpy(newline, "^");

	iterator.Reset();
	while(iterator.MoreElements()) {
		ClientListEntry* cle = iterator.GetData();
		if (admin >= cle->Admin() && (iName == 0 || namestrlen == 0 || strncasecmp(cle->name(), iName, namestrlen) == 0 || strncasecmp(cle->AccountName(), iName, namestrlen) == 0 || strncasecmp(cle->LSName(), iName, namestrlen) == 0)) {
			struct in_addr  in;
			in.s_addr = cle->GetIP();
			if (addnewline) {
				AppendAnyLenString(&output, &outsize, &outlen, newline);
			}
			AppendAnyLenString(&output, &outsize, &outlen, "ID: %i  Acc# %i  AccName: %s  IP: %s", cle->GetID(), cle->AccountID(), cle->AccountName(), inet_ntoa(in));
			AppendAnyLenString(&output, &outsize, &outlen, "%s  Stale: %i  Online: %i  Admin: %i", newline, cle->GetStaleCounter(), cle->Online(), cle->Admin());
			if (cle->LSID())
				AppendAnyLenString(&output, &outsize, &outlen, "%s  LSID: %i  LSName: %s  WorldAdmin: %i", newline, cle->LSID(), cle->LSName(), cle->WorldAdmin());
			if (cle->CharID())
				AppendAnyLenString(&output, &outsize, &outlen, "%s  CharID: %i  CharName: %s  Zone: %s (%i)", newline, cle->CharID(), cle->name(), database.GetZoneName(cle->zone()), cle->zone());
			if (outlen >= 3072) {
				connection->SendEmoteMessageRaw(to, 0, 0, 10, output);
				safe_delete(output);
				outsize = 0;
				outlen = 0;
				addnewline = false;
			}
			else
				addnewline = true;
			y++;
		}
		iterator.Advance();
		x++;
	}
	AppendAnyLenString(&output, &outsize, &outlen, "%s%i CLEs in memory. %i CLEs listed. numplayers = %i.", newline, x, y, numplayers);
	connection->SendEmoteMessageRaw(to, 0, 0, 10, output);
	safe_delete(output);
}

void ZSList::CLEAdd(int32 iLSID, const char* iLoginName, const char* iLoginKey, sint16 iWorldAdmin, int32 ip) {
	ClientListEntry* tmp = new ClientListEntry(iLSID, iLoginName, iLoginKey, iWorldAdmin, ip);

	clientlist.Append(tmp);
}

void ZSList::CLCheckStale() {
	LinkedListIterator<ClientListEntry*> iterator(clientlist);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->CheckStale()) {
			iterator.RemoveCurrent();
		}
		else
			iterator.Advance();
	}
}

void ZSList::ClientUpdate(ZoneServer* zoneserver, ServerClientList_Struct* scl) {
	LinkedListIterator<ClientListEntry*> iterator(clientlist);
	ClientListEntry* cle;
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->GetID() == scl->wid) {
			cle = iterator.GetData();
			if (scl->remove == 2)
				cle->LeavingZone(zoneserver, CLE_Status_Offline);
			else if (scl->remove == 1)
				cle->LeavingZone(zoneserver, CLE_Status_Zoning);
			else
				cle->Update(zoneserver, scl);
			return;
		}
		iterator.Advance();
	}
	if (scl->remove == 2)
		cle = new ClientListEntry(zoneserver, scl, CLE_Status_Online);
	else if (scl->remove == 1)
		cle = new ClientListEntry(zoneserver, scl, CLE_Status_Zoning);
	else
		cle = new ClientListEntry(zoneserver, scl, CLE_Status_InZone);
	clientlist.Insert(cle);
	zoneserver->ChangeWID(scl->charid, cle->GetID());
}

void ZSList::CLEKeepAlive(int32 numupdates, int32* wid) {
	LinkedListIterator<ClientListEntry*> iterator(clientlist);
	int32 i;

	iterator.Reset();
	while(iterator.MoreElements()) {
		for (i=0; i<numupdates; i++) {
			if (wid[i] == iterator.GetData()->GetID())
				iterator.GetData()->KeepAlive();
		}
		iterator.Advance();
	}
}
bool ClientListEntry::CheckAuth(int32 id, const char* iKey, int32 ip) {
	if (pIP==ip && strncmp(plskey, iKey,10) == 0){
		paccountid = id;
		database.GetAccountFromID(id,paccountname,&padmin);
		return true;
	}
	return false;
}
ClientListEntry* ZSList::CheckAuth(int32 id, const char* iKey, int32 ip ) {
  LinkedListIterator<ClientListEntry*> iterator(clientlist);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->CheckAuth(id, iKey, ip))
			return iterator.GetData();
		iterator.Advance();
	}
	return 0;
}
ClientListEntry* ZSList::CheckAuth(int32 iLSID, const char* iKey) {
  LinkedListIterator<ClientListEntry*> iterator(clientlist);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->CheckAuth(iLSID, iKey))
			return iterator.GetData();
		iterator.Advance();
	}
	return 0;
}

ClientListEntry* ZSList::CheckAuth(const char* iName, const char* iPassword) {
	LinkedListIterator<ClientListEntry*> iterator(clientlist);
	MD5 tmpMD5(iPassword);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->CheckAuth(iName, tmpMD5))
			return iterator.GetData();
		iterator.Advance();
	}
	sint16 tmpadmin;
	
	printf("Login with '%s' and '%s'\n", iName, iPassword);
	
	int32 accid = database.CheckLogin(iName, iPassword, &tmpadmin);
	if (accid) {
		ClientListEntry* tmp = new ClientListEntry(accid, iName, tmpMD5, tmpadmin);
		clientlist.Append(tmp);
		return tmp;
	}
	return 0;
}

ClientListEntry::ClientListEntry(int32 iLSID, const char* iLoginName, const char* iLoginKey, sint16 iWorldAdmin, int32 ip) {
	ClearVars(true);

	id = zoneserver_list.GetNextCLEID();
	pIP = ip;
	pLSID = iLSID;
	if(iLSID > 0)
		paccountid = database.GetAccountIDFromLSID(iLSID, paccountname, &padmin);
	strn0cpy(plsname, iLoginName, sizeof(plsname));
	strn0cpy(plskey, iLoginKey, sizeof(plskey));
	pworldadmin = iWorldAdmin;
}

ClientListEntry::ClientListEntry(int32 iAccID, const char* iAccName, MD5& iMD5Pass, sint16 iAdmin) {
	ClearVars(true);

	id = zoneserver_list.GetNextCLEID();
	pIP = 0;
	pLSID = 0;
	pworldadmin = 0;

	paccountid = iAccID;
	strn0cpy(paccountname, iAccName, sizeof(paccountname));
	pMD5Pass = iMD5Pass;
	padmin = iAdmin;
}

ClientListEntry::ClientListEntry(ZoneServer* iZS, ServerClientList_Struct* scl, sint8 iOnline) {
	ClearVars(true);

	id = zoneserver_list.GetNextCLEID();
	pIP = 0;
	pLSID = scl->LSAccountID;
	strn0cpy(plsname, scl->name, sizeof(plsname));
	strn0cpy(plskey, scl->lskey, sizeof(plskey));
	pworldadmin = 0;

	paccountid = scl->AccountID;
	strn0cpy(paccountname, scl->AccountName, sizeof(paccountname));
	padmin = scl->Admin;

	if (iOnline >= CLE_Status_Zoning)
		Update(iZS, scl, iOnline);
	else
		SetOnline(iOnline);
}

ClientListEntry::~ClientListEntry() {
	if (RunLoops) {
		Camp(); // updates zoneserver's numplayers
		client_list.RemoveCLEReferances(this);
	}
}

void ClientListEntry::SetChar(int32 iCharID, const char* iCharName) {
	pcharid = iCharID;
	strn0cpy(pname, iCharName, sizeof(pname));
}

void ClientListEntry::SetOnline(ZoneServer* iZS, sint8 iOnline) {
	if (iZS == this->Server())
		SetOnline(iOnline);
}

void ClientListEntry::SetOnline(sint8 iOnline) {
	if (iOnline >= CLE_Status_Online && pOnline < CLE_Status_Online)
		numplayers++;
	else if (iOnline < CLE_Status_Online && pOnline >= CLE_Status_Online) {
		numplayers--;
	}
	if (iOnline != CLE_Status_Online || pOnline < CLE_Status_Online)
		pOnline = iOnline;
	if (iOnline < CLE_Status_Zoning)
		Camp();
	if (pOnline >= CLE_Status_Online)
		stale = 0;
}

void ClientListEntry::Update(ZoneServer* iZS, ServerClientList_Struct* scl, sint8 iOnline) {
	if (pzoneserver != iZS) {
		if (pzoneserver)
			pzoneserver->NumPlayers()--;
		if (iZS)
			iZS->NumPlayers()++;
	}
	pzoneserver = iZS;
	pzone = scl->zone;
	pcharid = scl->charid;

	strcpy(pname, scl->name);
	if (paccountid == 0) {
		paccountid = scl->AccountID;
		strcpy(paccountname, scl->AccountName);
		strcpy(plsname, scl->AccountName);
		pIP = scl->IP;
		pLSID = scl->LSAccountID;
		strn0cpy(plskey, scl->lskey, sizeof(plskey));
	}
	padmin = scl->Admin;
	plevel = scl->level;
	pclass_ = scl->class_;
	prace = scl->race;
	panon = scl->anon;
	ptellsoff = scl->tellsoff;
	pguilddbid = scl->guilddbid;
	pguildeqid = scl->guildeqid;
	pLFG = scl->LFG;
	gm = scl->gm;

	SetOnline(iOnline);
}

void ClientListEntry::LeavingZone(ZoneServer* iZS, sint8 iOnline) {
	if (iZS != 0 && iZS != pzoneserver)
		return;
	SetOnline(iOnline);

	if (pzoneserver)
		pzoneserver->NumPlayers()--;
	pzoneserver = 0;
	pzone = 0;
}

void ClientListEntry::ClearVars(bool iAll) {
	if (iAll) {
		id = 0;
		pOnline = CLE_Status_Never;
		stale = 0;

		pLSID = 0;
		memset(plsname, 0, sizeof(plsname));
		memset(plskey, 0, sizeof(plskey));
		pworldadmin = 0;

		paccountid = 0;
		memset(paccountname, 0, sizeof(paccountname));
		padmin = 0;
	}
	pzoneserver = 0;
	pzone = 0;
	pcharid = 0;
	memset(pname, 0, sizeof(pname));
	plevel = 0;
	pclass_ = 0;
	prace = 0;
	panon = 0;
	ptellsoff = 0;
	pguilddbid = 0;
	pguildeqid = 0xFFFFFFFF;
	pLFG = 0;
	gm = 0;
}

void ClientListEntry::Camp(ZoneServer* iZS) {
	if (iZS != 0 && iZS != pzoneserver)
		return;
	if (pzoneserver)
		pzoneserver->NumPlayers()--;

	ClearVars();

	stale = 0;
}

bool ClientListEntry::CheckStale() {
	stale++;
	if (stale >= 3) {
		if (pOnline > CLE_Status_Offline)
			SetOnline(CLE_Status_Offline);
		else
			return true;
	}
	return false;
}

bool ClientListEntry::CheckAuth(int32 iLSID, const char* iKey) {
//	if (LSID() == iLSID && strncmp(plskey, iKey,10) == 0) {
	if (strncmp(plskey, iKey,10) == 0) {
		if (paccountid == 0 && LSID()>0) {
			sint16 tmpStatus = net.GetDefaultStatus();
			paccountid = database.CreateAccount(plsname, 0, tmpStatus, LSID());
			if (!paccountid) {
				cerr << "Error adding local account for LS login: '" << plsname << "', duplicate name?" << endl;
				return false;
			}
			strn0cpy(paccountname, plsname, sizeof(paccountname));
			padmin = tmpStatus;
		}
		char lsworldadmin[15] = "0";
		database.GetVariable("honorlsworldadmin", lsworldadmin, sizeof(lsworldadmin));
		if (atoi(lsworldadmin) == 1 && pworldadmin != 0 && (padmin < pworldadmin || padmin == 0))
			padmin = pworldadmin;
		return true;
	}
	return false;
}

bool ClientListEntry::CheckAuth(const char* iName, MD5& iMD5Password) {
	if (LSAccountID() == 0 && strcmp(paccountname, iName) == 0 && pMD5Pass == iMD5Password)
		return true;
	return false;
}

