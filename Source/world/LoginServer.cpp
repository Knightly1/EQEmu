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
#include <stdio.h>
#include <iomanip>
using namespace std;
#include <stdlib.h>
#include "../common/version.h"

#ifdef WIN32
	#include <process.h>
	#include <windows.h>
	#include <winsock.h>

	#define snprintf	_snprintf
	#define vsnprintf	_vsnprintf
	#define strncasecmp	_strnicmp
	#define strcasecmp	_stricmp
#else // Pyro: fix for linux
	#include <sys/socket.h>
#ifdef FREEBSD //Timothy Whitman - January 7, 2003
	#include <sys/types.h>
#endif
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <pthread.h>
	#include <unistd.h>
	#include <errno.h>

	#include "../common/unix.h"

	#define SOCKET_ERROR -1
	#define INVALID_SOCKET -1
	extern int errno;
#endif

#include "../common/servertalk.h"
#include "LoginServer.h"
#include "../common/eq_packet_structs.h"
#include "../common/packet_dump.h"
#include "net.h"
#include "zoneserver.h"
#include "../common/database.h"

extern NetConnection net;
extern ZSList zoneserver_list;
extern LoginServer loginserver;
extern Database	database;
extern uint32 numzones;
extern int32 numplayers;
extern volatile bool	RunLoops;
volatile bool LoginLoopRunning = false;

bool AttemptingConnect = false;

LoginServer::LoginServer(const char* iAddress, int16 iPort) {
	LoginServerIP = ResolveIP(iAddress);
	LoginServerPort = iPort;
	statusupdate_timer = new Timer(LoginServer_StatusUpdateInterval);
	tcpc = new TCPConnection(true);
}

LoginServer::~LoginServer() {
	delete statusupdate_timer;
	delete tcpc;
}

bool LoginServer::Process() {

	if (statusupdate_timer->Check()) {
		this->SendStatus();
	}
    
	/************ Get all packets from packet manager out queue and process them ************/
	ServerPacket *pack = 0;
	while((pack = tcpc->PopPacket()))
	{
//cout << "Processing Packet from LoginServer: OPcode=0x" << hex << setw(4) << setfill('0') << pack->opcode << dec << " size=" << pack->size << endl;
		switch(pack->opcode) {
		case 0:
			break;
		case ServerOP_KeepAlive: {
			// ignore this
			break;
		}
		case ServerOP_UsertoWorldReq: {
			UsertoWorldRequest_Struct* utwr = (UsertoWorldRequest_Struct*) pack->pBuffer;	
			int32 id = database.GetAccountIDFromLSID(utwr->lsaccountid);
			sint16 status = database.CheckStatus(id);

			ServerPacket* outpack = new ServerPacket;
			outpack->opcode = ServerOP_UsertoWorldResp;
			outpack->size = sizeof(UsertoWorldResponse_Struct);
			outpack->pBuffer = new uchar[outpack->size];
			memset(outpack->pBuffer, 0, outpack->size);
			UsertoWorldResponse_Struct* utwrs = (UsertoWorldResponse_Struct*) outpack->pBuffer;
			utwrs->lsaccountid = utwr->lsaccountid;
			utwrs->ToID = utwr->FromID;

			if(net.world_locked == true)
			{
#ifndef SHAWN319
				if((status == 0 || status < 100) && (status != -2 || status != -1))
					utwrs->response = 0;
				if(status >= 100)
					utwrs->response = 1;
#else
				if((status < 255) && (status != -2 || status != -1))
					utwrs->response = 0;
#endif
			}
			else {
				utwrs->response = 1;
			}

			sint32 x = database.CommandRequirement("$MAXCLIENTS");
			if( (sint32)numplayers >= x && x != -1 && x != 255 && status < 80)
				utwrs->response = -3;

			if(status == -1)
				utwrs->response = -1;
			if(status == -2)
				utwrs->response = -2;

			//printf("Response is %i for %i\n",utwrs->response,id);

			utwrs->worldid = utwr->worldid;
			SendPacket(outpack);
			delete outpack;
			break;
		}
		case ServerOP_LSClientAuth: {
			ServerLSClientAuth* slsca = (ServerLSClientAuth*) pack->pBuffer;
			zoneserver_list.CLEAdd(slsca->lsaccount_id, slsca->name, slsca->key, slsca->worldadmin, slsca->ip);
			break;
		}
		case ServerOP_LSFatalError: {
			net.LoginServerInfo = false;
			net.UpdateStats = false;
			cout << "Login server responded with FatalError. Disabling reconnect." << endl;
			if (pack->size > 1) {
				cout << "Error message: '" << (char*) pack->pBuffer << "'" << endl;
			}
			break;
		}
		case ServerOP_SystemwideMessage: {
			ServerSystemwideMessage* swm = (ServerSystemwideMessage*) pack->pBuffer;
			zoneserver_list.SendEmoteMessageRaw(0, 0, 0, swm->type, swm->message);
			break;
		}
		default:
		{
			cout << " Unknown LSOPcode:" << (int)pack->opcode;
			cout << " size:" << pack->size << endl;
DumpPacket(pack->pBuffer, pack->size);
			break;
		}
		}

		delete pack;
	}

	return true;
}

// this should always be called in a new thread
#ifdef WIN32
	void AutoInitLoginServer(void *tmp) {
#else
	void *AutoInitLoginServer(void *tmp) {
#endif
	srand(time(NULL));
	if (loginserver.GetState() == TCPS_Ready) {
		InitLoginServer();
	}
#ifndef WIN32
	return 0;
#endif
}

bool InitLoginServer() {
	if (loginserver.GetState() != TCPS_Ready) {
		cout << "Error: InitLoginServer() while already attempting connect" << endl;
		return false;
	}
	if (!net.LoginServerInfo) {
		cout << "Error: Login server info not loaded" << endl;
		return false;
	}

	AttemptingConnect = true;
	int16 port;
	char* address = net.GetLoginInfo(&port);
	loginserver.Connect(address, port);
	return true;
}

bool LoginServer::Connect(const char* iAddress, int16 iPort) {
	char tmp[25];
	if(database.GetVariable("loginType",tmp,sizeof(tmp)) && strcasecmp(tmp,"MinILogin") == 0){
		minilogin = true;
		cout << "Setting World to MiniLogin Server type\n";
	}
	else
		minilogin = false;

	char errbuf[TCPConnection_ErrorBufferSize];
	if (iAddress == 0) {
		cout << "Error: LoginServer::Connect: address == 0" << endl;
		return false;
	}
	else {
		if ((LoginServerIP = ResolveIP(iAddress, errbuf)) == 0) {
			cout << "Error: LoginServer::Connect: Resolving IP address: '" << errbuf << "'" << endl;
			return false;
		}
	}
	if (iPort != 0)
		LoginServerPort = iPort;

	if (LoginServerIP == 0 || LoginServerPort == 0) {
		cout << "Error: LoginServer::Connect: Connect info incomplete, cannot connect" << endl;
		return false;
	}

	if (tcpc->Connect(LoginServerIP, LoginServerPort, errbuf)) {
		cout << "Connected to LoginServer: " << iAddress << ":" << LoginServerPort << endl;
		SendInfo();
		SendStatus();
		return true;
	}
	else {
		cout << "Error: LoginServer::Connect: '" << errbuf << "'" << endl;
		return false;
	}
}

void LoginServer::SendInfo() {
	ServerPacket* pack = new ServerPacket;
	pack->opcode = ServerOP_LSInfo;
	pack->size = sizeof(ServerLSInfo_Struct);
	pack->pBuffer = new uchar[pack->size];
	memset(pack->pBuffer, 0, pack->size);
	ServerLSInfo_Struct* lsi = (ServerLSInfo_Struct*) pack->pBuffer;
	strcpy(lsi->protocolversion, EQEMU_PROTOCOL_VERSION);
	strcpy(lsi->serverversion, CURRENT_VERSION);
	strcpy(lsi->name, net.GetWorldName());
	strcpy(lsi->account, net.GetWorldAccount());
	strcpy(lsi->password, net.GetWorldPassword());
	strcpy(lsi->address, net.GetWorldAddress());
	SendPacket(pack);
	delete pack;
}

void LoginServer::SendStatus() {
	statusupdate_timer->Start();
	ServerPacket* pack = new ServerPacket;
	pack->opcode = ServerOP_LSStatus;
	pack->size = sizeof(ServerLSStatus_Struct);
	pack->pBuffer = new uchar[pack->size];
	memset(pack->pBuffer, 0, pack->size);
	ServerLSStatus_Struct* lss = (ServerLSStatus_Struct*) pack->pBuffer;

	if (net.world_locked)
		lss->status = -2;
	else if (numzones <= 0)
		lss->status = -2;
	else
		lss->status = numplayers;

	lss->num_zones = numzones;
	lss->num_players = numplayers;
	SendPacket(pack);
	delete pack;
}

