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
#include <stdlib.h>

#include <signal.h>

#include "../common/debug.h"
#include "../common/queue.h"
#include "../common/timer.h"
#include "../common/EQNetwork.h"
#include "net.h"
#include "client.h"
#include "../common/database.h"
#include "../common/seperator.h"
#include "../common/version.h"
#include "../common/files.h"
#include "../common/eqtime.h"
#include "../common/EQEMuError.h"
#ifdef WIN32
	#include <process.h>
	#define snprintf	_snprintf
	#define vsnprintf	_vsnprintf
	#define strncasecmp	_strnicmp
	#define strcasecmp  _stricmp
	#include <conio.h>
#else
	#include <pthread.h>
	#include "../common/unix.h"

#endif
#ifdef SHAREMEM
	#include "../common/EMuShareMem.h"
	extern LoadEMuShareMemDLL EMuShareMemDLL;
    #ifndef WIN32
	#include <sys/types.h>
	#include <sys/ipc.h>
	#include <sys/sem.h>
	#include <sys/shm.h>
#ifndef FREEBSD
	union semun {
	    int val;
	    struct semid_ds *buf;
	    ushort *array;
	    struct seminfo *__buf;
	    void *__pad;
	};	  
#endif
    #endif
#endif
#ifdef EQPROFILE
#ifdef COMMON_PROFILE
CommonProfiler _cp;
#endif
#endif

#include "zoneserver.h"
#include "console.h"
#include "LoginServer.h"
#include "../common/dbasync.h"

EQNetworkServer eqns(PORT);
TCPServer tcps(PORT);
NetConnection net;
ClientList client_list;
extern ConsoleList console_list;
ZSList zoneserver_list;
LoginServer loginserver;
volatile bool RunLoops = true;
uint32 numclients = 0;
uint32 numzones = 0;
bool holdzones = false;
GuildRanks_Struct guilds[512];
Database database;

int main(int argc, char** argv) {
	#if DEBUG >= 9
		for (int logs = 0; logs < EQEMuLog::MaxLogID; logs++) {
			LogFile->write((EQEMuLog::LogIDs)logs, "CURRENT_WORLD_VERSION:%s", CURRENT_WORLD_VERSION);
		}
	#else
		LogFile->write(EQEMuLog::Status, "CURRENT_WORLD_VERSION:%s", CURRENT_WORLD_VERSION);
	#endif
	
	#ifdef _DEBUG
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	#endif
	
	if (signal(SIGINT, CatchSignal) == SIG_ERR)	{
		LogFile->write(EQEMuLog::Error, "Could not set signal handler");
		return 0;
	}
	#ifndef WIN32
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)	{
		LogFile->write(EQEMuLog::Error, "Could not set signal handler");
		return 0;
	}
	#endif
	
	if (argc >= 2) {
		char tmp[2];
		if (strcasecmp(argv[1], "help") == 0 || strcasecmp(argv[1], "?") == 0 || strcasecmp(argv[1], "/?") == 0 || strcasecmp(argv[1], "-?") == 0 || strcasecmp(argv[1], "-h") == 0 || strcasecmp(argv[1], "-help") == 0) {
			cout << "Worldserver command line commands:" << endl;
			cout << "adduser username password flag    - adds a user account" << endl;
			cout << "flag username flag    - sets GM flag on the account" << endl;
			cout << "startzone zoneshortname    - sets the starting zone" << endl;
			cout << "-holdzones    - reboots lost zones" << endl;
			return 0;
		}
		else if (strcasecmp(argv[1], "-holdzones") == 0) {
			cout << "Reboot Zones mode ON" << endl;
			holdzones = true;
		}
		else if (database.GetVariable("disablecommandline", tmp, 2)) {
			if (strlen(tmp) == 1) {
				if (tmp[0] == '1') {
					cout << "Command line disabled in database... exiting" << endl;
					return 0;
				}
			}
		}
		else if (strcasecmp(argv[1], "adduser") == 0) {
			if (argc == 5) {
				if (Seperator::IsNumber(argv[4])) {
					if (atoi(argv[4]) >= 0 && atoi(argv[4]) <= 255) {
						if (database.CreateAccount(argv[2], argv[3], atoi(argv[4])) == 0)
							cout << "database.CreateAccount failed." << endl;
						else
							cout << "Account created: Username='" << argv[2] << "', Password='" << argv[3] << "', status=" << argv[4] << endl;
						return 0;
					}
				}
			}
			cout << "Usage: world adduser username password flag" << endl;
			cout << "flag = 0, 1 or 2" << endl;
			return 0;
		}
		else if (strcasecmp(argv[1], "flag") == 0) {
			if (argc == 4) {
				if (Seperator::IsNumber(argv[3])) {

					if (atoi(argv[3]) >= 0 && atoi(argv[3]) <= 255) {
						if (database.SetGMFlag(argv[2], atoi(argv[3])))
							cout << "Account flagged: Username='" << argv[2] << "', status=" << argv[3] << endl;
						else
							cout << "database.SetGMFlag failed." << endl;
						return 0;
					}
				}
			}
			cout << "Usage: world flag username flag" << endl;
			cout << "flag = 0-200" << endl;
			return 0;
		}
		else if (strcasecmp(argv[1], "startzone") == 0) {
			if (argc == 3) {
				if (strlen(argv[2]) < 3) {
					cout << "Error: zone name too short" << endl;
				}
				else if (strlen(argv[2]) > 15) {
					cout << "Error: zone name too long" << endl;
				}
				else {
					if (database.SetVariable("startzone", argv[2]))
						cout << "Starting zone changed: '" << argv[2] << "'" << endl;
					else
						cout << "database.SetVariable failed." << endl;
				}
				return 0;
			}
			cout << "Usage: world startzone zoneshortname" << endl;
			return 0;
		}
		else {
			cout << "Error, unknown command line option" << endl;
			return 0;
		}
	}
	//cout << net.GetDefaultStatus() << ' ' << net.GetUnavailZone() << endl;
	srand(time(NULL));
	LogFile->write(EQEMuLog::Status, "Loading variables..");
	database.LoadVariables();
	LogFile->write(EQEMuLog::Status, "Loading zones..");
	database.LoadZoneNames();
	LogFile->write(EQEMuLog::Status, "Clearing groups..");
	database.ClearGroup();
	LogFile->write(EQEMuLog::Status, "Loading items..");
	if (!database.LoadItems()) {
		LogFile->write(EQEMuLog::Error, "Error: Could not load item data.  But ignoring");
	}
	net.ReadLoginINI();
	LogFile->write(EQEMuLog::Status, "Loading guild ranks..");
	database.LoadGuilds(guilds);
	LogFile->write(EQEMuLog::Status, "Loading %s..", ADDON_INI_FILE);
	database.ExtraOptions();
	LogFile->write(EQEMuLog::Status, "Loading EQ time of day..");
	if (!zoneserver_list.worldclock.loadFile(EQTIME_INI))
		LogFile->write(EQEMuLog::Error, "Unable to load %s", EQTIME_INI);
	
	char tmp[20];
	database.GetVariable("holdzones",tmp, 20);	
	if ((strcasecmp(tmp, "1") == 0)) {
		LogFile->write(EQEMuLog::Status, "Reboot zone modes ON");
		holdzones = true;
	}
	
	LogFile->write(EQEMuLog::Status, "Deleted %i stale player corpses from database", database.DeleteStalePlayerCorpses());
	LogFile->write(EQEMuLog::Status, "Deleted %i stale player backups from database", database.DeleteStalePlayerBackups());
	switch (rand()%12)
	{
	case 0:
		cout <<"Breaking bards...done"<<endl;
		break;
	case 1:
		cout <<"Assassinating people that whine about horses...done"<<endl;
		break;
	case 2:
		cout <<"HELP I WORK IN THE EQEMU SWEATSHOP, YOU'RE MY ONLY HOPE!"<<endl;
		break;
	case 3:
		cout <<"Please don't make me a levelme server. :("<<endl;
		break;
	case 4:
		cout <<"You may already be a winner... of the darwin award."<<endl;
		break;
	case 5:
		cout <<"Warning: Pants may change during online play."<<endl;
		break;
	case 6:
		cout <<"Abort/retry/cancel?"<<endl;
		break;
	case 7:
		cout <<"Formatting drive C:"<<endl;
		break;
	case 8:
		cout <<"Liberating a country that despises us..."<<endl;
		break;
	case 9:
		cout <<"Breaking linux compiles...."<<endl;
		break;
	case 10:
		cout <<"Screwing up windows compiles..."<<endl;
		break;
	case 11:
		cout <<"Warning: You need to get laid..."<<endl;
		break;
	}
	char errbuf[TCPConnection_ErrorBufferSize];
	if (tcps.Open(PORT, errbuf)) {
		if (strlen(net.GetWorldAddress()) == 0)
			cout << "TCP listening on: port " << PORT << endl;
		else
			cout << "TCP listening on: " << net.GetWorldAddress() << ":" << PORT << endl;
	}
	else {
		cout << "Failed to open TCP port "<<PORT<<"." << endl;
		cout << "errbuf=" << errbuf << endl;
		return 1;
	}
	if (eqns.Open()) {
		if (strlen(net.GetWorldAddress()) == 0)
			cout << "World server listening on: port " << PORT << endl;
		else
			cout << "World server listening on: " << net.GetWorldAddress() << ":" << PORT << endl;
	}
	else {
		cout << "Failed to open port 9000." << endl;
		return 1;
	}
	zoneserver_list.shutdowntimer=new Timer(60000);
	zoneserver_list.shutdowntimer->Disable();
	zoneserver_list.reminder = new Timer(20000);
	zoneserver_list.reminder->Disable();
	Timer InterserverTimer(INTERSERVER_TIMER); // does MySQL pings and auto-reconnect
	InterserverTimer.Trigger();
	EQNetworkConnection* eqnc;
	TCPConnection* tcpc;
	while(RunLoops) {
		Timer::SetCurrentTime();

		while ((eqnc = eqns.NewQueuePop())) {
			struct in_addr	in;
			in.s_addr = eqnc->GetrIP();
			cout << Timer::GetCurrentTime() << " New client from ip: " << inet_ntoa(in) << " port: " << ntohs(eqnc->GetrPort()) << endl;
			Client* client = new Client(eqnc);
			// @merth: client->zoneattempt=0;
			client_list.Add(client);
		}
		client_list.Process();
		while ((tcpc = tcps.NewQueuePop())) {
			struct in_addr in;
			in.s_addr = tcpc->GetrIP();
			cout << Timer::GetCurrentTime() << " New TCP connection: " << inet_ntoa(in) << ":" << tcpc->GetrPort() << endl;
			console_list.Add(new Console(tcpc));
		}
		loginserver.Process();
		console_list.Process();
		zoneserver_list.Process();
		if (InterserverTimer.Check()) {
			InterserverTimer.Start();
			database.ping();
			AsyncLoadVariables();
			if (net.LoginServerInfo && loginserver.Connected() == false) {
#ifdef WIN32
				_beginthread(AutoInitLoginServer, 0, NULL);
#else
				pthread_t thread;
				pthread_create(&thread, NULL, &AutoInitLoginServer, NULL);
#endif
			}
		}
		if (numclients == 0) {
			Sleep(10);
			continue;
		}
		Sleep(1);
	}
	console_list.KillAll();
	zoneserver_list.KillAll();
	tcps.Close();
	eqns.Close();
#if 0
#if defined(SHAREMEM) && !defined(WIN32)
		for (int ipc_files = 0; ipc_files <= 4; ipc_files++) {
			key_t share_key;
			switch (ipc_files) {
				// Item
				case 0: share_key = ftok(".", 'I'); break;
				// Npctype
				case 1: share_key = ftok(".", 'N'); break;
				// Door
				case 2: share_key = ftok(".", 'D'); break;
				// Spell
				case 3: share_key = ftok(".", 'S'); break;
				// Faction
				case 4: share_key = ftok(".", 'F'); break;
				// ERROR Fatal
				default: cerr<<"Opps!"<<endl; share_key = 0xFF; break;
			}
			
			int share_id = shmget(share_key, 0, IPC_NOWAIT|0400);
			if (share_id <= 0) {
				cerr<<"exiting could not check user count on shared memory ipcs mem leak!!!!!!!! id="<<share_id<<" key:"<<share_key<<endl;
				exit(1);
			}
			struct shmid_ds mem_users;
			if ((shmctl(share_id, IPC_STAT, &mem_users)) != 0) {
				cerr<<"exiting error checking user count on shared memory, marking for deletion!!!!!id="<<share_id<<" key:"<<share_key<<endl;
				shmctl(share_id, IPC_RMID, 0);
				exit(1);
			}
			if (mem_users.shm_nattch == 0) {
				//cerr<<"exiting stale share marked for deletion!id="<<share_id<<" key:"<<share_key<<endl;
				shmctl(share_id, IPC_RMID, 0);
			}
			else if (mem_users.shm_nattch == 1) {
				//cerr<<"mem_users = 1"<<endl;
				// Detatch and delete shared mem here
				EMuShareMemDLL.Unload();
				shmctl(share_id, IPC_RMID, 0);
			}

		}
#endif
#endif

	CheckEQEMuErrorAndPause();
	return 0;
}

void CatchSignal(int sig_num) {
	cout << "Got signal " << sig_num << endl;
	if(zoneserver_list.worldclock.saveFile(EQTIME_INI)==false)
		cout << "Failed to save time file." << endl;
	RunLoops = false;
}

bool NetConnection::ReadLoginINI() {
	char buf[201], type[201];
	int items[3] = {0, 0};
	FILE *f;
	
	if (!(f = fopen (MAIN_INI_FILE, "r"))) {
		LogFile->write(EQEMuLog::Error, "File '%s' could not be opened", MAIN_INI_FILE);
		return false;
	}
	do {
		fgets (buf, 200, f);
		if (feof (f))
		{
			printf ("[LoginServer] block not found in '%s'.\n", MAIN_INI_FILE);
			fclose (f);
			return false;
		}
	}
	while (strncasecmp (buf, "[LoginServer]\n", 14) != 0 && strncasecmp (buf, "[LoginServer]\r\n", 15) != 0);

	while (!feof (f))
	{
#ifdef WIN32
		if (fscanf (f, "%[^=]=%[^\n]\r\n", type, buf) == 2)
#else
		if (fscanf (f, "%[^=]=%[^\r\n]\n", type, buf) == 2)
#endif
		{
			if (!strncasecmp (type, "worldname", 9)) {
				snprintf(worldname, sizeof(worldname), "%s", buf);
				items[1] = 1;
				if(strlen(worldname)<10)
					cout << "Invalid worldname, please edit LoginServer.ini.  Server name must be at least 10 characters." << endl;
			}
			if (!strncasecmp (type, "account", 7)) {
				strncpy(worldaccount, buf, 30);
			}
			if (!strncasecmp (type, "logstats", 8)) {
				if (strcasecmp(buf, "true") == 0 || (buf[0] == '1' && buf[1] == 0))
					net.UpdateStats = true;
			}
			if (!strncasecmp (type, "password", 8)) {
				strncpy (worldpassword, buf, 30);
			}
			if (!strncasecmp (type, "locked", 6)) {
				if (strcasecmp(buf, "true") == 0 || (buf[0] == '1' && buf[1] == 0))
					world_locked = true;
			}
			if (!strncasecmp (type, "worldaddress", 12)) {
				if (strlen(buf) >= 3) {
					strncpy (worldaddress, buf, 250);
				}
			}
			if ((!strcasecmp (type, "loginserver")) || (!strcasecmp (type, "loginserver1"))) {
				strncpy (loginaddress[0], buf, 100);
				items[0] = 1;
			}
			if ((!strcasecmp(type, "loginport")) || (!strcasecmp(type, "loginport1"))) {
				if (Seperator::IsNumber(buf) && atoi(buf) > 0 && atoi(buf) < 0xFFFF) {
					loginport[0] = atoi(buf);
				}
			}
			if (!strcasecmp (type, "loginserver2")) {
				strncpy (loginaddress[1], buf, 250);
			}
			if (!strcasecmp(type, "loginport2")) {
				if (Seperator::IsNumber(buf) && atoi(buf) > 0 && atoi(buf) < 0xFFFF) {
					loginport[1] = atoi(buf);
				}
			}
			if (!strcasecmp (type, "loginserver3")) {
				strncpy (loginaddress[2], buf, 250);
			}
			if (!strcasecmp(type, "loginport3")) {
				if (Seperator::IsNumber(buf) && atoi(buf) > 0 && atoi(buf) < 0xFFFF) {
					loginport[2] = atoi(buf);
				}
			}

			if (!strcasecmp (type, "loginserver4")) {
				strncpy (loginaddress[3], buf, 250);
			}
			if (!strcasecmp(type, "loginport4")) {
				if (Seperator::IsNumber(buf) && atoi(buf) > 0 && atoi(buf) < 0xFFFF) {
					loginport[3] = atoi(buf);
				}
			}
			if (!strcasecmp (type, "loginserver5")) {
				strncpy (loginaddress[4], buf, 250);
			}
			if (!strcasecmp(type, "loginport5")) {
				if (Seperator::IsNumber(buf) && atoi(buf) > 0 && atoi(buf) < 0xFFFF) {
					loginport[4] = atoi(buf);
				}
			}
		}
	}

	if (!items[0] || !items[1])
	{
		cout << "Incomplete LoginServer.INI file." << endl;
		fclose (f);
		return false;
	}
	
	/*
	if (strcasecmp(worldname, "Unnamed server") == 0) {
		cout << "LoginServer.ini: server unnamed, disabling uplink" << endl;
		fclose (f);
		return false;
	}
	*/
	
	fclose(f);
	f=fopen (MAIN_INI_FILE, "r");
	do {
		fgets (buf, 200, f);
		if (feof (f))
		{
			LogFile->write(EQEMuLog::Error, "[WorldServer] block not found in %s", MAIN_INI_FILE);
			fclose (f);
			return true;
		}

	}
	while (strncasecmp (buf, "[WorldServer]\n", 14) != 0 && strncasecmp (buf, "[WorldServer]\r\n", 15) != 0);

	while (!feof (f))
	{
#ifdef WIN32
		if (fscanf (f, "%[^=]=%[^\n]\r\n", type, buf) == 2)
#else
		if (fscanf (f, "%[^=]=%[^\r\n]\n", type, buf) == 2)
#endif
		{
			
/*			if (!strcasecmp (type, "Unavailzone")) {
				strncpy (UnavailZone, buf, 25);
				//cout << database.GetZoneID(UnavailZone);
				if(!database.GetZoneID(UnavailZone))
				{
					memset(UnavailZone,0,25);
					if(strlen(UnavailZone)!=0) cout << "Invalid zonename in UnavailZone" << endl;
				}
			}*/
			if (!strcasecmp(type, "Defaultstatus")) {
				if (Seperator::IsNumber(buf) && atoi(buf) > 0 && atoi(buf) < 0xFFFF) {
					DEFAULTSTATUS = atoi(buf);
				}
			}
		}
	}
	fclose (f);
#ifdef IRC
	f=fopen (MAIN_INI_FILE, "r");
	do {
		fgets (buf, 200, f);
		if (feof (f))
		{
			LogFile->write(EQEMuLog::Error, "[ChatChannelServer] block not found in %s", MAIN_INI_FILE);
			fclose (f);
			return true;
		}

	}
	while (strncasecmp (buf, "[ChatChannelServer]\n", 14) != 0 && strncasecmp (buf, "[ChatChannelServer]\r\n", 15) != 0);

	while (!feof (f))
	{
#ifdef WIN32
		if (fscanf (f, "%[^=]=%[^\n]\r\n", type, buf) == 2)
#else
		if (fscanf (f, "%[^=]=%[^\r\n]\n", type, buf) == 2)
#endif
		{
			
			if (!strncasecmp (type, "worldshortname", 14)) {
				snprintf(worldshortname, sizeof(worldshortname), "%s", buf);
				items[2] = 1;
				if(strlen(worldshortname)<3)
					cout << "Invalid worldshortname, please edit LoginServer.ini.  Short server name must be at least 3 characters." << endl;
			}
			if (!strcasecmp(type, "chataddress")) {
				strncpy (chataddress, buf, 250);
			}
			if (!strcasecmp(type, "chatport")) {
				chatport=atoi(buf);
			}
		}
	}
	fclose (f);
#else
	strcpy(worldshortname,"test");
	strcpy(chataddress,"127.0.0.1");
	chatport=9876;
#endif
	
	LogFile->write(EQEMuLog::Status, "%s read.", MAIN_INI_FILE);
	LoginServerInfo=1;
	return true;
}

char* NetConnection::GetLoginInfo(int16* oPort) {
	if (oPort == 0)
		return 0;
	if (loginaddress[0][0] == 0)
		return 0;

	int8 tmp[5] = { 0, 0, 0, 0, 0 };
	int8 count = 0;

	for (int i=0; i<5; i++) {
		if (loginaddress[i][0])
			tmp[count++] = i;
	}

	int x = rand() % count;

	*oPort = loginport[tmp[x]];
	return loginaddress[tmp[x]];
}

void UpdateWindowTitle(char* iNewTitle) {
#ifdef WIN32
	char tmp[500];
	if (iNewTitle) {
		snprintf(tmp, sizeof(tmp), "World: %s", iNewTitle);
	}
	else {
		snprintf(tmp, sizeof(tmp), "World");
	}
	SetConsoleTitle(tmp);
#endif
}

