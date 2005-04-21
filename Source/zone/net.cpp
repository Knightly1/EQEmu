#define DONT_SHARED_OPCODES
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
#include "features.h"
#include <iostream>
using namespace std;
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
	#ifdef _CRTDBG_MAP_ALLOC
		#undef new
	#endif
#include <fstream>
	#ifdef _CRTDBG_MAP_ALLOC
		#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
	#endif
using namespace std;
#ifdef WIN32
#include <conio.h>
#define snprintf	_snprintf
#define vsnprintf	_vsnprintf
#define strncasecmp	_strnicmp
#define strcasecmp  _stricmp
#endif

bool spells_loaded = false;
volatile bool RunLoops = true;
extern volatile bool ZoneLoaded;
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




#include "../common/queue.h"
#include "../common/timer.h"
#include "../common/EQStream.h"
#include "../common/EQStreamFactory.h"
#include "../common/eq_packet_structs.h"
#include "../common/Mutex.h"
#include "../common/version.h"
#include "../common/files.h"
#include "../common/EQEMuError.h"
#include "../common/packet_dump_file.h"
#include "../common/opcodemgr.h"

#include "masterentity.h"
#include "worldserver.h"
#include "net.h"
#include "spdat.h"
#include "zone.h"
#include "command.h"
#include "parser.h"
#include "embparser.h"
#include "perlparser.h"
#include "client_logs.h"
#include "questmgr.h"
#include "titles.h"

#ifdef GUILDWARS
#include "../GuildWars/GuildWars.h"
extern GuildLocationList location_list;
#endif

NetConnection		net;
EntityList			entity_list;
WorldServer			worldserver;
int32				numclients = 0;
#ifdef CATCH_CRASH
int8			error = 0;
#endif
GuildRanks_Struct	guilds[512];
char errorname[32];
int16 adverrornum = 0;
extern Zone* zone;
EQStreamFactory eqsf(ZoneStream);
npcDecayTimes_Struct npcCorpseDecayTimes[100];
TitleManager title_manager;


bool zoneprocess;

#ifdef NEW_LoadSPDat
	// For NewLoadSPDat function
	const SPDat_Spell_Struct* spells; 
	SPDat_Spell_Struct* spells_delete; 
	sint32 GetMaxSpellID();


	void LoadSPDat();
	bool FileLoadSPDat(SPDat_Spell_Struct* sp, sint32 iMaxSpellID);
	sint32 SPDAT_RECORDS = -1;
#else
	#define SPDat_Location	"spdat.eff"
	SPDat_Spell_Struct spells[SPDAT_RECORDS];
	void LoadSPDat(SPDat_Spell_Struct** SpellsPointer = 0);


#endif

Database database;

#ifdef WIN32
#include <process.h>
#else
#include <pthread.h>
#include "../common/unix.h"
#endif

void Shutdown();
extern void MapOpcodes();

//bool ZoneBootup(int32 iZoneID, bool iStaticZone = false);
char *strsep(char **stringp, const char *delim);

#ifdef ADDONCMD
#include "addoncmd.h"
extern  AddonCmd addonCmd;
#endif

int main(int argc, char** argv) {
#ifdef _EQDEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//	_crtBreakAlloc = 2025;
#endif
	srand(time(NULL));
	
#if EQDEBUG > 5 
		for (int logs = 0; logs < EQEMuLog::MaxLogID; logs++) 
			LogFile->write((EQEMuLog::LogIDs)logs, "CURRENT_ZONE_VERSION: %s", CURRENT_ZONE_VERSION);
#else
		LogFile->write(EQEMuLog::Status, "CURRENT_ZONE_VERSION: %s", CURRENT_ZONE_VERSION);
#endif
	
	if (argc != 5)
	{
		cerr << "Usage: zone zone_name address port worldaddress" << endl;
		exit(0);
	}
	char* filename = argv[0];
	char* zone_name = argv[1];
	char* address = argv[2];
	int32 port = atoi(argv[3]);
	char* worldaddress = argv[4];
	
	if (strlen(address) <= 0) {
		cerr << "Invalid address" << endl;

		exit(0);
	}
	if (port <= 0) {
		cerr << "Bad port specified" << endl;
		exit(0);
	}
	if (strlen(worldaddress) <= 0) {
		cerr << "Invalid worldaddress" << endl;
		exit(0);
	}
	if (signal(SIGINT, CatchSignal) == SIG_ERR) {
		cerr << "Could not set signal handler" << endl;
		return 0;
	}
#ifndef WIN32
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		cerr << "Could not set signal handler" << endl;
		return 0;
	}
#endif
	net.SaveInfo(address, port, worldaddress,filename);
	
	LogFile->write(EQEMuLog::Status, "Loading opcodes..");
#ifdef DONT_SHARED_OPCODES
	EQOpcodeManager = new RegularOpcodeManager();
#else
	EQOpcodeManager = new SharedOpcodeManager();
#endif
	if(!EQOpcodeManager->LoadOpcodes(OPCODES_FILE)) {
		LogFile->write(EQEMuLog::Error, "Loading opcodes failed. I cant live like this!");
		return(1);
	}
	
	LogFile->write(EQEMuLog::Status, "Mapping Opcodes");
	MapOpcodes();
	LogFile->write(EQEMuLog::Status, "Loading Variables");
	database.LoadVariables();
	LogFile->write(EQEMuLog::Status, "Loading zone names");
	database.LoadZoneNames();
	LogFile->write(EQEMuLog::Status, "Loading items");
	if (!database.LoadItems()) {
		LogFile->write(EQEMuLog::Error, "Loading items FAILED!");
		cout << "Failed.  But ignoring error and going on..." << endl;
	}
	LogFile->write(EQEMuLog::Status, "Loading npcs");
/*
	if (!database.LoadNPCTypes()) {
		LogFile->write(EQEMuLog::Error, "Loading npcs FAILED!");
		CheckEQEMuErrorAndPause();
		return 0;
	}
*/
#ifdef SHAREMEM
	LogFile->write(EQEMuLog::Status, "Loading npc faction lists");
	if (!database.LoadNPCFactionLists()) {
		LogFile->write(EQEMuLog::Error, "Loading npcs faction lists FAILED!");
		CheckEQEMuErrorAndPause();
		return 0;
	}
#endif
	LogFile->write(EQEMuLog::Status, "Loading loot tables");
	if (!database.LoadLoot()) {
		LogFile->write(EQEMuLog::Error, "Loading loot FAILED!");
		CheckEQEMuErrorAndPause();
		return 0;
	}
	LogFile->write(EQEMuLog::Status, "Loading doors");
	database.LoadDoors();
	
	LoadSPDat();

	// New Load function.  keeping it commented till I figure out why its not working correctly in linux. Trump.
	// NewLoadSPDat();
	LogFile->write(EQEMuLog::Status, "Loading guilds");
	database.LoadGuilds(guilds);
	LogFile->write(EQEMuLog::Status, "Loading factions");
	database.LoadFactionData();
	LogFile->write(EQEMuLog::Status, "Loading titles");
	title_manager.LoadTitles();
	LogFile->write(EQEMuLog::Status, "Loading AA effects");
	database.LoadAAEffects();
	LogFile->write(EQEMuLog::Status, "Loading swarm spells");
	database.LoadSwarmSpells();
	LogFile->write(EQEMuLog::Status, "Loading tributes");
	database.LoadTributes();
	LogFile->write(EQEMuLog::Status, "Loading corpse timers");
	database.GetDecayTimes(npcCorpseDecayTimes);
	LogFile->write(EQEMuLog::Status, "Loading what ever is left");
	database.ExtraOptions();
	LogFile->write(EQEMuLog::Status, "Loading commands");
	int retval=command_init();
	if(retval<0)
		LogFile->write(EQEMuLog::Status, "Command loading FAILED");
	else
		LogFile->write(EQEMuLog::Status, "%d commands loaded", retval);


#ifdef EMBPERL
#ifdef EMBPERL_XS
       LogFile->write(EQEMuLog::Status, "Loading embedded perl XS");
       AutoDelete<PerlXSParser> ADparse;
       try {
		ADparse.init((PerlXSParser **)(&parse), new PerlXSParser);
	}
#else //old EMBPERL
       LogFile->write(EQEMuLog::Status, "Loading embedded perl");
       AutoDelete<PerlembParser> ADparse;
       try {
		ADparse.init((PerlembParser **)(&parse), new PerlembParser);
	}
#endif
       catch(const char *err)
       {//this should never happen, so if it does, it is something really serious (like a bad perl install), so we'll shutdown.
               LogFile->write(EQEMuLog::Status, "Fatal error initializing perl: %s", err);
               return EXIT_FAILURE;
       }
#else	//old .qst
	AutoDelete<Parser> ADparse(&parse, new Parser);
#endif //EMBPERL

#ifdef ADDONCMD	
	LogFile->write(EQEMuLog::Status, "Looding addon commands from dll");
	if ( !addonCmd.openLib() ) {
		LogFile->write(EQEMuLog::Error, "Loading addons failed =(");
	}
#endif
#ifdef CLIENT_LOGS
	LogFile->SetAllCallbacks(ClientLogs::EQEmuIO_buf);
	LogFile->SetAllCallbacks(ClientLogs::EQEmuIO_fmt);
#endif
	if (!worldserver.Connect()) {
		LogFile->write(EQEMuLog::Error, "worldserver.Connect() FAILED!");
	}
	
	LogFile->write(EQEMuLog::Status, "Starting EQ Network server.");
	//start up the network server
	if (!eqsf.Open(net.GetZonePort())) {
		safe_delete(zone);
		cerr << "eqsf.Open failed" << endl;
		worldserver.SetZone(0);
		return false;
	}
	
	if (strcmp(zone_name, ".") == 0 || strcasecmp(zone_name, "sleep") == 0) {
		LogFile->write(EQEMuLog::Status, "Entering sleep mode");
	} else if (!Zone::Bootup(database.GetZoneID(zone_name), true)) {
		LogFile->write(EQEMuLog::Error, "Zone bootup FAILED!");
		zone = 0;
	}
	
	Timer InterserverTimer(INTERSERVER_TIMER); // does MySQL pings and auto-reconnect
#ifdef EQPROFILE
#ifdef PROFILE_DUMP_TIME
	Timer profile_dump_timer(PROFILE_DUMP_TIME*1000);
	profile_dump_timer.Start();
#endif
#endif
	Timer quest_timers(1000);	//highest resolution quest timer is 1 second
	UpdateWindowTitle();
	bool worldwasconnected = worldserver.Connected();
	EQStream* eqs;
	Timer temp_timer(10);
	temp_timer.Start();
	while(RunLoops) {
		{	//profiler block to omit the sleep from times
		_ZP(net_main);
		
		//Advance the timer to our current point in time
		Timer::SetCurrentTime();
		
		//process stuff from world
#ifdef CATCH_CRASH
		try{
#endif
			worldserver.Process();
#ifdef CATCH_CRASH
		}
		catch(...){
			error = 1;
			adverrornum = worldserver.GetErrorNumber();
			worldserver.Disconnect();
			worldwasconnected = false;
		}
#endif
		
		//look for new connections
		while ((eqs = eqsf.Pop())) {
			struct in_addr	in;
			in.s_addr = eqs->GetrIP();
			LogFile->write(EQEMuLog::Status, "%i New client from ip:%s port:%i", Timer::GetCurrentTime(), inet_ntoa(in), ntohs(eqs->GetrPort()));
			Client* client = new Client(eqs);
			entity_list.AddClient(client);
		}
		
		//check for timeouts in other threads
		timeout_manager.CheckTimeouts();
		
		if (worldserver.Connected()) {
			worldwasconnected = true;
		}
		else {
			if (worldwasconnected && ZoneLoaded)
				entity_list.ChannelMessageFromWorld(0, 0, 6, 0, 0, "WARNING: World server connection lost");
			worldwasconnected = false;
		}
		if (ZoneLoaded && temp_timer.Check()) {
			{
				int8 error2 = 4;
#ifdef CATCH_CRASH
				try{
#endif
					if(net.group_timer.Enabled() && net.group_timer.Check())
						entity_list.GroupProcess();
					error2 = 99;
					if(net.door_timer.Enabled() && net.door_timer.Check())
						entity_list.DoorProcess();
					error2 = 98;
					if(net.object_timer.Enabled() && net.object_timer.Check())
						entity_list.ObjectProcess();
					error2 = 97;
					if(net.corpse_timer.Enabled() && net.corpse_timer.Check())
						entity_list.CorpseProcess();
					if(net.trap_timer.Enabled() && net.trap_timer.Check())
						entity_list.TrapProcess();
					error2 = 98;
					error2 = 96;
					entity_list.Process();
					error2 = 95;
#ifdef CATCH_CRASH
					try{
					entity_list.MobProcess();
					}
					catch(...){
						printf("Catching Mob Crash...\n");
					}
#else
					entity_list.MobProcess();
#endif
					error2 = 94;
#ifdef GUILDWARS
					try{
					location_list.ProcessLocations();
					}
					catch(...){
						printf("GuildWars Location Crash.\n");
					}
#endif
					entity_list.BeaconProcess();
#ifdef CATCH_CRASH
				}
				catch(...){
					error=error2;
				}
				try{
#endif
					zoneprocess= zone->Process();
					if (!zoneprocess) {
						Zone::Shutdown();
					}
#ifdef CATCH_CRASH
				}
				catch(...){
					error = 2;
				}
				try{
#endif
					if(quest_timers.Check())
						quest_manager.Process();
#ifdef CATCH_CRASH
				}
				catch(...){
					error = 77777;
				}
#endif
			}
		}
		DBAsyncWork* dbaw = 0;
		while ((dbaw = MTdbafq->Pop())) {
			DispatchFinishedDBAsync(dbaw);
		}
		if (InterserverTimer.Check()
#ifdef CATCH_CRASH
			&& !error
#endif
			) {
#ifdef CATCH_CRASH
			try{
#endif
				InterserverTimer.Start();
				database.ping();
				AsyncLoadVariables();
//				NPC::GetAILevel(true);
				entity_list.UpdateWho();
				if (worldserver.TryReconnect() && (!worldserver.Connected()))
					worldserver.AsyncConnect();
#ifdef CATCH_CRASH
			}
			catch(...)
			{
				error = 16;
				RunLoops = false;
			}
#endif
		}
#ifdef CATCH_CRASH
		if (error){
			RunLoops = false;
		}
#endif
#if defined(_EQDEBUG) && defined(DEBUG_PC)
		QueryPerformanceCounter(&tmp3);
		mainloop_time += tmp3.QuadPart - tmp2.QuadPart;
		if (!--tmp0) {
			tmp0 = 200;
			printf("Elapsed Tics  : %9.0f (%1.4f sec)\n", (double)mainloop_time, ((double)mainloop_time/tmp.QuadPart));
			printf("NPCAI Tics    : %9.0f (%1.2f%%)\n", (double)npcai_time, ((double)npcai_time/mainloop_time)*100);
			printf("FindSpell Tics: %9.0f (%1.2f%%)\n", (double)findspell_time, ((double)findspell_time/mainloop_time)*100);
			printf("AtkAllowd Tics: %9.0f (%1.2f%%)\n", (double)IsAttackAllowed_time, ((double)IsAttackAllowed_time/mainloop_time)*100);
			printf("ClientPro Tics: %9.0f (%1.2f%%)\n", (double)clientprocess_time, ((double)clientprocess_time/mainloop_time)*100);
			printf("ClientAtk Tics: %9.0f (%1.2f%%)\n", (double)clientattack_time, ((double)clientattack_time/mainloop_time)*100);
			mainloop_time = 0;
			npcai_time = 0;
			findspell_time = 0;
			IsAttackAllowed_time = 0;
			clientprocess_time = 0;
			clientattack_time = 0;
		}
#endif
#ifdef EQPROFILE
#ifdef PROFILE_DUMP_TIME
		if(profile_dump_timer.Check()) {
			DumpZoneProfile();
		}
#endif
#endif
		}	//end extra profiler block
		Sleep(ZoneTimerResolution);
	}
	
#ifdef CATCH_CRASH
	if (error)
		FilePrint("eqemudebug.log",true,true,"Zone %i crashed. Errorcode: %i/%i. Current zone loaded:%s. Current clients:%i. Caused by: %s",net.GetZonePort(), error,adverrornum, zone->GetShortName(), numclients,errorname);
	try{
		entity_list.Message(0, 15, "ZONEWIDE_MESSAGE: This zone caused a fatal error and will shut down now. Your character will be restored to the last saved status. We are sorry for any inconvenience!");
	}
	catch(...){}
	if (error){
#ifdef WIN32		
		ExitProcess(error);
#else	
		entity_list.Clear();
		safe_delete(zone);
#endif
	}
#endif

	entity_list.Clear();
	if (zone != 0
#ifdef CATCH_CRASH
		& !error
#endif
		)
		Zone::Shutdown(true);
	//Fix for Linux world server problem.
	eqsf.Close();
	worldserver.Disconnect();
	dbasync->CommitWrites();
	dbasync->StopThread();
#ifdef NEW_LoadSPDat
	safe_delete(spells_delete);
#endif
	command_deinit();
//	Needed if REUSE_ZLIB is defined in packet_functions.h
//	DeflatePacket(NULL, 0, NULL, 0);
//	InflatePacket(NULL, 0, NULL, 0, false);
	
	CheckEQEMuErrorAndPause();
	LogFile->write(EQEMuLog::Status, "Proper zone shutdown complete.");
	return 0;
}

void CatchSignal(int sig_num) {
	LogFile->write(EQEMuLog::Status, "Recieved signal:%i", sig_num);
	RunLoops = false;
}

void Shutdown()
{
	Zone::Shutdown(true);
	RunLoops = false;
	worldserver.Disconnect();
	//	safe_delete(worldserver);
	LogFile->write(EQEMuLog::Status, "Shutting down...");
}

int32 NetConnection::GetIP()
{
	char     name[255+1];
	size_t   len = 0;
	hostent* host = 0;
	
	if (gethostname(name, len) < 0 || len <= 0)
	{
		return 0;
	}
	
	host = (hostent*)gethostbyname(name);
	if (host == 0)
	{
		return 0;
	}
	
	return inet_addr(host->h_addr);
}

int32 NetConnection::GetIP(char* name)
{
	hostent* host = 0;
	
	host = (hostent*)gethostbyname(name);
	if (host == 0)
	{
		return 0;
	}
	
	return inet_addr(host->h_addr);
	
}

void NetConnection::SaveInfo(char* address, int32 port, char* waddress, char* filename) {

	ZoneAddress = new char[strlen(address)+1];
	strcpy(ZoneAddress, address);
	ZonePort = port;
	WorldAddress = new char[strlen(waddress)+1];
	strcpy(WorldAddress, waddress);
	strn0cpy(ZoneFileName, filename, sizeof(ZoneFileName));
}

NetConnection::NetConnection() 
: 
	object_timer(5000),
	door_timer(5000),
	corpse_timer(2000),
	group_timer(1000),
	trap_timer(1000)
{
	ZonePort = 0;
	ZoneAddress = 0;
	WorldAddress = 0;
	group_timer.Disable();
	corpse_timer.Disable();
	door_timer.Disable();
	object_timer.Disable();
	trap_timer.Disable();
}

NetConnection::~NetConnection() {
	if (ZoneAddress != 0)
		safe_delete_array(ZoneAddress);
	if (WorldAddress != 0)
		safe_delete_array(WorldAddress);
}
bool chrcmpI(const char* a, const char* b) {
#if EQDEBUG >= 11
    LogFile->write(EQEMuLog::Debug, "crhcmpl() a:%i b:%i", (int*) a, (int*) b);
#endif
	if(((int)* a)==((int)* b))
		return false;
	else
		return true;
}

#ifdef NEW_LoadSPDat
sint32 GetMaxSpellID() {
	int tempid=0, oldid=-1;
	char spell_line_start[2048];
	char* spell_line = spell_line_start;
	char token[64]="";
	char seps[] = "^";
	//ifstream in(SPELLS_FILE);
	
	/*struct stat s;
	if(stat(SPELLS_FILE, &s) != 0) {
		LogFile->write(EQEMuLog::Error, "File '%s' not found (stat failed) in same directory as zone.exe, spell loading FAILED!", SPELLS_FILE);
		return(-1);
	}
	*/

	FILE *sf = fopen(SPELLS_FILE, "r");
	
	if(sf == NULL) {
		LogFile->write(EQEMuLog::Error, "File '%s' not found in same directory as zone.exe, spell loading FAILED!", SPELLS_FILE);
		return -1;
	}
	
	fgets(spell_line, sizeof(spell_line_start), sf);
	while(!feof(sf)) {
		strcpy(token,strtok(spell_line, seps));
		if(token!=NULL);
		{
			tempid = atoi(token);
			if(tempid>oldid)
				oldid = tempid;
			else
				break;
		}
		fgets(spell_line, sizeof(spell_line_start), sf);
	}
	
	fclose(sf);
	
	/*ifstream in(SPELLS_FILE);
	
	if(!in) {
		LogFile->write(EQEMuLog::Error, "File '%s' not found in same directory as zone.exe, spell loading FAILED!", SPELLS_FILE);
		return -1;
	}
	
	in.getline(spell_line, sizeof(spell_line_start));
	while(strlen(spell_line)>1)
	{
		strcpy(token,strtok(spell_line, seps));
		if(token!=NULL);
		{
			tempid = atoi(token);
			if(tempid>oldid)
				oldid = tempid;
			else
				break;
		}
		in.getline(spell_line, sizeof(spell_line_start));
	}*/
	
		
	return oldid;
}
#ifdef SHAREMEM
extern "C" bool extFileLoadSPDat(void* sp, sint32 iMaxSpellID) { return FileLoadSPDat((SPDat_Spell_Struct*) sp, iMaxSpellID); }
#endif

void LoadSPDat() {
	if (SPDAT_RECORDS != -1) {
		LogFile->write(EQEMuLog::Debug, "LoadSPDat() SPDAT_RECORDS:%i != -1, spells already loaded?", SPDAT_RECORDS);
	}
	sint32 MaxSpellID = GetMaxSpellID();
	if (MaxSpellID == -1) {
		LogFile->write(EQEMuLog::Debug, "LoadSPDat() MaxSpellID == -1, %s missing?", SPELLS_FILE);
		return;
	}
#ifdef SHAREMEM
	if (!EMuShareMemDLL.Load())
		return;
	SPDAT_RECORDS = MaxSpellID+1;
	if (EMuShareMemDLL.Spells.DLLLoadSPDat((const CALLBACK_FileLoadSPDat)&extFileLoadSPDat, (const void**) &spells, &SPDAT_RECORDS, sizeof(SPDat_Spell_Struct))) {
		spells_loaded = true;
	}
	else {
		SPDAT_RECORDS = -1;
		LogFile->write(EQEMuLog::Error, "LoadSPDat() EMuShareMemDLL.Spells.DLLLoadSPDat() returned false");
		return;
	}
#else
	spells_delete = new SPDat_Spell_Struct[MaxSpellID+1];
	if (FileLoadSPDat(spells_delete, MaxSpellID)) {
		spells = spells_delete;
		SPDAT_RECORDS = MaxSpellID+1;
		spells_loaded = true;
	}
	else {
		safe_delete(spells_delete);
		LogFile->write(EQEMuLog::Error, "LoadSPDat() FileLoadSPDat() returned false");
		return;
	}
#endif
}

bool FileLoadSPDat(SPDat_Spell_Struct* sp, sint32 iMaxSpellID) {
	int tempid=0;
	int16 counter=0;
	char spell_line[2048];
	LogFile->write(EQEMuLog::Status,"FileLoadSPDat() Loading spells from %s", SPELLS_FILE);
	
	FILE *sf = fopen(SPELLS_FILE, "r");
	
	if(sf == NULL) {
		LogFile->write(EQEMuLog::Error, "File '%s' not found in same directory as zone.exe, spell loading FAILED!", SPELLS_FILE);
		return false;
	}
/*	ifstream in(SPELLS_FILE);
	if(!in) {
		LogFile->write(EQEMuLog::Error, "File '%s' not found in same directory as zone.exe, spell loading FAILED!", SPELLS_FILE);
		return false;
	}
	*/
	if (iMaxSpellID < 0) {
		LogFile->write(EQEMuLog::Error,"FileLoadSPDat() Loading spells FAILED! iMaxSpellID:%i < 0", iMaxSpellID);
		return false;
	}
/*

This is hanging on freebsd for me, not sure why...

//#if EQDEBUG >= 1
	else {
		LogFile->write(EQEMuLog::Debug,"FileLoadSPDat() Highest spell ID:%i", iMaxSpellID);
	}
//#endif
*/
/*	in.close();
	in.open(SPELLS_FILE);
	if(!in) {
		LogFile->write(EQEMuLog::Error, "File '%s' not found in same directory as zone.exe, spell loading FAILED!", SPELLS_FILE);
		return false;
	}
	while(!in.eof()) {
		in.getline(spell_line, sizeof(spell_line));
		Seperator sep(spell_line, '^', 200, 100, false, 0, 0, false);
		
		if(spell_line[0]=='\0')
			break;
		
		tempid = atoi(sep.arg[0]);
		if (tempid > iMaxSpellID) {
			LogFile->write(EQEMuLog::Error, "FATAL FileLoadSPDat() tempid:%i >= iMaxSpellID:%i", tempid, iMaxSpellID);
			return false;
		}
		*/
		
	while(!feof(sf)) {
		fgets(spell_line, sizeof(spell_line), sf);
		Seperator sep(spell_line, '^', 200, 100, false, 0, 0, false);
		
		if(spell_line[0]=='\0')
			break;
		
		tempid = atoi(sep.arg[0]);
		if (tempid > iMaxSpellID) {
			LogFile->write(EQEMuLog::Error, "FATAL FileLoadSPDat() tempid:%i >= iMaxSpellID:%i", tempid, iMaxSpellID);
			return false;
		}
		
		counter++;
		strcpy(sp[tempid].name, sep.arg[1]);
		strcpy(sp[tempid].player_1, sep.arg[2]);
		strcpy(sp[tempid].teleport_zone, sep.arg[3]);
		strcpy(sp[tempid].you_cast,  sep.arg[4]);
		strcpy(sp[tempid].other_casts, sep.arg[5]);
		strcpy(sp[tempid].cast_on_you, sep.arg[6]);
		strcpy(sp[tempid].cast_on_other, sep.arg[7]);
		strcpy(sp[tempid].spell_fades, sep.arg[8]);

		sp[tempid].range=atof(sep.arg[9]);
		sp[tempid].aoerange=atof(sep.arg[10]);
		sp[tempid].pushback=atof(sep.arg[11]);
		sp[tempid].pushup=atof(sep.arg[12]);
		sp[tempid].cast_time=atoi(sep.arg[13]);
		sp[tempid].recovery_time=atoi(sep.arg[14]);
		sp[tempid].recast_time=atoi(sep.arg[15]);
		sp[tempid].buffdurationformula=atoi(sep.arg[16]);
		sp[tempid].buffduration=atoi(sep.arg[17]);
		sp[tempid].AEDuration=atoi(sep.arg[18]);
		sp[tempid].mana=atoi(sep.arg[19]);
		
		int y=0;
		for(y=0; y< 12;y++)
			sp[tempid].base[y]=atoi(sep.arg[20+y]);
		for(y=0; y < 11; y++)
			sp[tempid].base2[y]=atoi(sep.arg[33+y]);
		for(y=0; y< 12;y++)
			sp[tempid].max[y]=atoi(sep.arg[44+y]);
		
		sp[tempid].icon=atoi(sep.arg[56]);
		sp[tempid].memicon=atoi(sep.arg[57]);
		
		for(y=0; y< 4;y++)
			sp[tempid].components[y]=atoi(sep.arg[58+y]);
		
		for(y=0; y< 4;y++)
			sp[tempid].component_counts[y]=atoi(sep.arg[62+y]);
		
		for(y=0; y< 4;y++)
			sp[tempid].NoexpendReagent[y]=atoi(sep.arg[66+y]);
		
		for(y=0; y< 12;y++)
			sp[tempid].formula[y]=atoi(sep.arg[70+y]);
		
		sp[tempid].LightType=atoi(sep.arg[82]);
		sp[tempid].goodEffect=atoi(sep.arg[83]);
		sp[tempid].Activated=atoi(sep.arg[84]);
		sp[tempid].resisttype=atoi(sep.arg[85]);
		
		for(y=0; y< 12;y++)
			sp[tempid].effectid[y]=atoi(sep.arg[86+y]);
		
		sp[tempid].targettype=atoi(sep.arg[98]);
		sp[tempid].basediff=atoi(sep.arg[99]);
		sp[tempid].skill=atoi(sep.arg[100]);
		sp[tempid].zonetype=atoi(sep.arg[101]);
		sp[tempid].EnvironmentType=atoi(sep.arg[102]);
		sp[tempid].TimeOfDay=atoi(sep.arg[103]);
		
		for(y=0; y< 16;y++)
			sp[tempid].classes[y]=atoi(sep.arg[104+y]);
		
		sp[tempid].CastingAnim=atoi(sep.arg[120]);
		sp[tempid].TargetAnim=atoi(sep.arg[121]);
		sp[tempid].TravelType=atoi(sep.arg[122]);
		sp[tempid].SpellAffectIndex=atoi(sep.arg[123]);
		
		for(y=0; y< 23;y++) {
			sp[tempid].Spacing2[y]=atoi(sep.arg[124+y]);
		}
		
		sp[tempid].ResistDiff=atoi(sep.arg[147]);
		
		for(y=0; y< 2;y++) {
			sp[tempid].Spacing3[y]=atoi(sep.arg[148+y]);
		}
		
		sp[tempid].RecourseLink = atoi(sep.arg[150]);
		sp[tempid].descnum = atoi(sep.arg[155]);
		sp[tempid].typedescnum = atoi(sep.arg[156]);
		sp[tempid].effectdescnum = atoi(sep.arg[157]);

		for(y=0; y< 17;y++)
			sp[tempid].Spacing4[y] = atoi(sep.arg[158+y]);
	} 
	LogFile->write(EQEMuLog::Status, "FileLoadSPDat() spells loaded: %i", counter);
	//in.close();
	fclose(sf);

	return true;
}

#endif

void UpdateWindowTitle(char* iNewTitle) {
#ifdef WIN32
	char tmp[500];
	if (iNewTitle) {
		snprintf(tmp, sizeof(tmp), "%i: %s", net.GetZonePort(), iNewTitle);
	}
	else {
		if (zone) {
			#if defined(GOTFRAGS) || defined(_EQDEBUG)
				snprintf(tmp, sizeof(tmp), "%i: %s, %i clients, %i", net.GetZonePort(), zone->GetShortName(), numclients, getpid());
			#else
				snprintf(tmp, sizeof(tmp), "%i: %s, %i clients", net.GetZonePort(), zone->GetShortName(), numclients);
			#endif
		}
		else {
			#if defined(GOTFRAGS) || defined(_EQDEBUG)
				snprintf(tmp, sizeof(tmp), "%i: sleeping, %i", net.GetZonePort(), getpid());
			#else
				snprintf(tmp, sizeof(tmp), "%i: sleeping", net.GetZonePort());
			#endif
		}
	}
	SetConsoleTitle(tmp);
#endif
}

/*bool ZoneBootup(int32 iZoneID, bool iStaticZone) {
	const char* zonename = database.GetZoneName(iStaticZone);
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
	if (!zone->Init(iStaticZone)) {
		safe_delete(zone);
		cerr << "Zone->Init failed" << endl;
		worldserver.SetZone(0);
		return false;
	}
	if (!eqns.Open(net.GetZonePort())) {
		safe_delete(zone);
		cerr << "NetConnection::Init failed" << endl;
		worldserver.SetZone(0);
		return false;
	}
	if (!zone->LoadZoneCFG(zone->GetShortName(), true)) // try loading the zone name...
		zone->LoadZoneCFG(zone->GetFileName()); // if that fails, try the file name, then load defaults
	
	//petition_list.ClearPetitions();
	//petition_list.ReadDatabase();
	ZoneLoaded = true;
	worldserver.SetZone(iZoneID);
	zone->GetTimeSync();
	cout << "-----------" << endl << "Zone server '" << zonename << "' listening on port:" << net.GetZonePort() << endl << "-----------" << endl;
	//entity_list.WriteEntityIDs();
	UpdateWindowTitle();
	return true;
}*/

// Original source found at http://www.castaglia.org/proftpd/doc/devel-guide/src/lib/strsep.c.html
char *strsep(char **stringp, const char *delim)
{
  char *res;

  if(!stringp || !*stringp || !**stringp)
    return (char*)0;

  res = *stringp;
  while(**stringp && !strchr(delim,**stringp))
    (*stringp)++;
  
  if(**stringp) {
    **stringp = '\0';
    (*stringp)++;
  }

  return res;
}
