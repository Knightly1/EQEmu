#include "../common/debug.h"
#include <iostream>
using namespace std;
#include <iomanip>
using namespace std;
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

// Disgrace: for windows compile
#ifdef WIN32
	#include <windows.h>
	#include <winsock.h>
	#define snprintf	_snprintf
	#define vsnprintf	_vsnprintf
	#define strncasecmp	_strnicmp
	#define strcasecmp  _stricmp
#else
	#include <sys/socket.h>
#ifdef FREEBSD //Timothy Whitman - January 7, 2003
	#include <sys/types.h>
#endif
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <unistd.h>
#endif

#include "client.h"
#include "../common/eq_opcodes.h"
#include "../common/eq_packet_structs.h"
#include "../common/packet_dump.h"
#include "../common/database.h"
#include "../common/Item.h"
#include "../common/races.h"
#include "../common/classes.h"
#include "../common/languages.h"
#include "../common/skills.h"
#include "LoginServer.h"
#include "zoneserver.h"
#include "net.h"

extern Database database;
extern const char* ZONE_NAME;
extern GuildRanks_Struct guilds[512];
extern ZSList zoneserver_list;
extern LoginServer loginserver;
extern uint32 numclients;
extern NetConnection net;
extern volatile bool RunLoops;

Client::Client(EQNetworkConnection* ieqnc) {
	eqnc = ieqnc;
	eqnc->SetDataRate(7);
	ip = eqnc->GetrIP();
	port = ntohs(eqnc->GetrPort());

	autobootup_timeout = new Timer(10000);
	autobootup_timeout->Disable();
	
	CLE_keepalive_timer = new Timer(15000);
	connect = new Timer(1000);
	connect->Disable();
	firstlogin = true;
	realfirstlogin = false;
	seencharsel = false;
	cle = 0;
	zoneID = 0;
	char_name[0] = 0;
	charid = 0;
	pwaitingforbootup = 0;
	numclients++;
}

Client::~Client() {
	if (RunLoops && cle && zoneID == 0)
		cle->SetOnline(CLE_Status_Offline);
	eqnc->Free();
	safe_delete(autobootup_timeout);
	safe_delete(CLE_keepalive_timer);
	safe_delete(connect);
	numclients--;
}

void Client::SendCharInfo() {
	if (cle) {
		cle->SetOnline(CLE_Status_CharSelect);
	}
	
	seencharsel = true;
	
	// Send OP_EnterWorld
	APPLAYER* outapp = 0;
				char cname[64] = {0};
				if (!firstlogin && database.GetLiveChar(GetAccountID(), cname)) {
					if(database.GetAccountIDByChar(cname) != GetAccountID())
					eqnc->Close();

					cout << "Telling client to continue session with: " << cname << endl;
					outapp = new APPLAYER(OP_EnterWorld, strlen(cname)+1);
					memcpy(outapp->pBuffer, cname, strlen(cname));
					QueuePacket(outapp);
					safe_delete(outapp);
				}
				else {
					outapp = new APPLAYER(OP_EnterWorld, 1);
					QueuePacket(outapp);
					safe_delete(outapp);
				}

	if(firstlogin)
	{
	// Send OP_ExpansionInfo
	outapp = new APPLAYER(OP_ExpansionInfo, 4);
	char val[20] = {0};
	if (database.GetVariable("Expansions", val, 20)) {
		outapp->pBuffer[0] = atoi(val);
	}
	else {
		outapp->pBuffer[0] = 0xFF;
	}
	QueuePacket(outapp);
	safe_delete(outapp);
	}
	
	// Send OP_SendCharInfo
	outapp = new APPLAYER(OP_SendCharInfo, sizeof(CharacterSelect_Struct));
	CharacterSelect_Struct* cs = (CharacterSelect_Struct*)outapp->pBuffer;
	
	database.GetCharSelectInfo(GetAccountID(), cs);
	outapp->Deflate();
	
	QueuePacket(outapp);
	safe_delete(outapp);
}

bool Client::HandlePacket(const APPLAYER *app) {
	#if DEBUG == 9
		cout << "Received 0x" << hex << setfill('0') << setw(4) << app->opcode << dec << endl;
		DumpPacket(app);
	#endif
	
	bool ret = true;
	
	if (!eqnc->CheckActive()) {
		cout << "Client disconnected" << endl;
		return false;
	}
	
	if (GetAccountID() == 0 && app->opcode != OP_SendLoginInfo) {
		// Got a packet other than OP_SendLoginInfo when not logged in
		LogFile->write(EQEMuLog::Error, "Expecting OP_SendLoginInfo, got %x", app->opcode);
		return false;
	}
	else if (app->opcode == OP_AckPacket) {
		return true;
	}
	
	#ifdef MERTHALICIOUS
		//@merth: this just here temporarily for my debugging
		cout << "Received 0x" << hex << setw(4) << setfill('0') << app->opcode << ", size=" << dec << app->size << endl;
	#endif
	
	switch(app->opcode)
	{
		case OP_CrashDump:
			break;
		case OP_SendLoginInfo:
		{
			//DumpPacket(app);
			// Quagmire - max len for name is 18, pass 15
			char name[19] = {0};
			char password[16] = {0};
			strncpy(name, (char*)app->pBuffer,18);
			if (app->size < strlen(name)+2) {
				ret = false;
				break;
			}
			strncpy(password, (char*)&app->pBuffer[strlen(name)+1], 15);
			int32 id=0;
			if(strncasecmp(name, "LS#", 3) == 0)
				id=atoi(&name[3]);
			else
				id=atoi(name);
			if (id > 0 && id < 100000) {
				if (loginserver.Connected() == false) {
					cout << "Error: Login server login while not connected to login server." << endl;
					ret = false;
					break;
				}
				if ((cle = zoneserver_list.CheckAuth(id, password))) {
					if (cle->AccountID() == 0) {
						ret = false;
						break;
					}
					
					
					cle->SetOnline();
					
					cout << "Logged in: ";
					if (firstlogin)
						cout << "FirstLogin ";
					cout << "LS#" << cle->LSID() << ": " << cle->LSName() << endl;
					APPLAYER* outapp;
					if(firstlogin)
					{
						if(!connect->Enabled())
							connect->Start();
						// Send OPCode: OP_LogServer
						outapp = new APPLAYER(OP_LogServer, sizeof(LogServer_Struct));
						QueuePacket(outapp);
						safe_delete(outapp);
					}
					else{
						SendGuildList();// Send OPCode: OP_GuildsList
						SendApproveWorld();
					}
				}
				else {
					// TODO: Find out how to tell the client wrong username/password
					//cerr << "Bad/expired session key: " << name << ", k=" << password << endl;
					cerr << "Bad/expired session key: " << name << endl;
					ret = false;
					break;
				}
			}
			else if (strlen(password) <= 1) {
				// TODO: Find out how to tell the client wrong username/password
				cerr << "Login without a password" << endl;
				ret = false;
				break;
			}
			else {
				cle = zoneserver_list.CheckAuth(name, password);
				if (cle == 0)
				{
					// TODO: Find out how to tell the client wrong username/password
					struct in_addr	in;
					in.s_addr = ip;
					cerr << inet_ntoa(in) << ": Wrong name/pass: name='" << name << "'" << endl;
					ret = false;
					break;
				}
				cout << "Logged in: Local: " << name << endl;
				APPLAYER* outapp; 
				if (firstlogin) 
				{ 
				// Send OPCode: OP_LogServer 
				outapp = new APPLAYER(OP_LogServer, sizeof(LogServer_Struct)); 
				QueuePacket(outapp); 
				delete outapp; 
				} 
				// Send OPCode: OP_GuildsList 
				SendGuildList(); 
				SendApproveWorld();
			}
			
			if (!cle)
				break;
			cle->SetIP(GetIP());
		    break;
		}
		case OP_ApproveName: //Name approval
		{
			if (GetAccountID() == 0) {
				cerr << "Name approval with no logged in account" << endl;
				ret = false;
				break;
			}
			char name[64] = {0};
			snprintf(name, 64, "%s", (char*)app->pBuffer);
		    uchar race = app->pBuffer[64];
		    uchar clas = app->pBuffer[68];
			
		    cout << "Name approval request for:" << name; 
		    cout << " race:" << (int)race;
		    cout << " class:" << (int)clas << endl;

			APPLAYER *outapp;
			outapp = new APPLAYER;
			outapp->opcode = OP_ApproveName;
		   	outapp->pBuffer = new uchar[1];
		   	outapp->size = 1;
			if (database.CheckNameFilter(name)) {
				outapp->pBuffer[0] = 0;
			}
			else if (database.ReserveName(GetAccountID(), name)) {
				outapp->pBuffer[0] = 1;
			}
			else {
				outapp->pBuffer[0] = 0;
			}
			QueuePacket(outapp);
			safe_delete(outapp);
		    break;			
		}
		case OP_RandomNameGenerator:
		{
// SCORPIOUS2K - added random name generator
			// creates up to a 10 char name
			char vowels[17]="aeiouyaeiouaeioe";
			char cons[47]="bcdfghjklmnpqrstvwxzybcdgklmnprstvwbcdgkpstrkd";
			char rndname[17]="\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
			char paircons[31]="ngrkndstshthphsktrdrbrgrfrclcr";
			int rndnum=rand()%76,n=1;
			bool dlc=false;
			bool vwl=false;
			bool dbl=false;
			if (rndnum>63)
			{	// rndnum is 0 - 75 where 64-75 is cons pair, 17-63 is cons, 0-16 is vowel
				rndnum=(rndnum-61)*2;	// name can't start with "ng" "nd" or "rk"
				rndname[0]=paircons[rndnum];
				rndname[1]=paircons[rndnum+1];
				n=2;
			}
			else if (rndnum>16)
			{
				rndnum-=17;
				rndname[0]=cons[rndnum];
			}
			else
			{
				rndname[0]=vowels[rndnum];
				vwl=true;
			}
			int namlen=(rand()%6)+5;
			for (int i=n;i<namlen;i++)
			{
				dlc=false;
				if (vwl)	//last char was a vowel
				{			// so pick a cons or cons pair
					rndnum=rand()%63;
					if (rndnum>46)
					{	// pick a cons pair
						if (i>namlen-3)	// last 2 chars in name?
						{	// name can only end in cons pair "rk" "st" "sh" "th" "ph" "sk" "nd" or "ng"
							rndnum=(rand()%8)*2;
						}
						else
						{	// pick any from the set
							rndnum=(rndnum-47)*2;
						}
						rndname[i]=paircons[rndnum];
						rndname[i+1]=paircons[rndnum+1];
						dlc=true;	// flag keeps second letter from being doubled below
						i+=1;
					}
					else
					{	// select a single cons
						rndname[i]=cons[rndnum];
					}
				}
				else	
				{		// select a vowel
					rndname[i]=vowels[rand()%17];
				}
				vwl=!vwl;
				if (!dbl && !dlc)
				{	// one chance at double letters in name
					if (!(rand()%(i+10)))	// chances decrease towards end of name
					{
						rndname[i+1]=rndname[i];
						dbl=true;
						i+=1;
					}
				}
			}
			
			rndname[0]=toupper(rndname[0]);
			NameGeneration_Struct* ngs = (NameGeneration_Struct*)app->pBuffer;
			memset(ngs->name,0,64);
			strcpy(ngs->name,rndname);

//			char names[8][64] = { "How", "About", "You", "Think", "Of", "Your", "Own", "Name" };
//			//Could have parts of the random name in this struct and they compile together
//			NameGeneration_Struct* ngs = (NameGeneration_Struct*)app->pBuffer;
//			strncpy(ngs->name,"Notcreated",64);

			QueuePacket(app);
			break;

		}
		case OP_CharacterCreate: //Char create
		{
			if (GetAccountID() == 0)
			{
				cerr << "Account ID not set; unable to create character." << endl;
				ret = false;
				break;
			}
			else if (app->size != sizeof(CharCreate_Struct))
			{
				cout << "Wrong size on OP_CharacterCreate. Got: " << app->size << ", Expected: " << sizeof(CharCreate_Struct) << endl;
				DumpPacket(app);
				break;
			}

			CharCreate_Struct *cc = (CharCreate_Struct*)app->pBuffer;
			if(OPCharCreate(cc) == false)
			{
				database.DeleteCharacter(cc->name);
				APPLAYER *outapp = new APPLAYER(OP_ApproveName, 1);
				outapp->pBuffer[0] = 0;
				QueuePacket(outapp);
				safe_delete(outapp);
			}

			SendCharInfo();

			break;
		}
		case OP_EnterWorld: // Enter world
		{
		
			if (GetAccountID() == 0) {
				cerr << "Enter world with no logged in account" << endl;
				eqnc->Close();
				break;
			}
			if(GetAdmin() < 0)
			{
				cerr << "Banned or suspended." << endl;
				eqnc->Close();
				break;
			}
			strncpy(char_name, (char*)app->pBuffer, 64);
			
			APPLAYER *outapp;
			int32 tmpaccid = 0;
			charid = database.GetCharacterInfo(char_name, &tmpaccid, &zoneID);
			if (charid == 0 || tmpaccid != GetAccountID()) {
				cerr << "Could not get CharInfo for " << char_name << endl;
				eqnc->Close();
				break;
			}
			
			// Make sure this account owns this character
			if (tmpaccid != GetAccountID()) {
				cerr << "This account does not own this character" << endl;
				eqnc->Close();
				break;
			}
			
			if (zoneID == 0 || !database.GetZoneName(zoneID)) {
				// This is to save people in an invalid zone, once it's removed from the DB
				database.MoveCharacterToZone(charid, "arena");
				LogFile->write(EQEMuLog::Error, "Zone not found in database zone_id=%i, moveing char to arena character:%s", zoneID, char_name);
			}
			
			if (firstlogin) {
				outapp = new APPLAYER(OP_MOTD);
				char tmp[500] = {0};
				if (database.GetVariable("MOTD", tmp, 500)) {
					outapp->size = strlen(tmp)+1;
					outapp->pBuffer = new uchar[outapp->size];
					memset(outapp->pBuffer,0,outapp->size);
					strcpy((char*)outapp->pBuffer, tmp);
					
					if(realfirstlogin)
						database.SetGroupID(char_name,0);
					else{
						int32 groupid=database.GetGroupID(char_name);
						if(groupid>0){
							char* leader=0;
							char leaderbuf[64]={0};
							if((leader=database.GetGroupLeaderForLogin(char_name,leaderbuf)) && strlen(leader)>1){
								APPLAYER* outapp3 = new APPLAYER(OP_GroupUpdate,sizeof(GroupJoin_Struct));
								GroupJoin_Struct* gj=(GroupJoin_Struct*)outapp3->pBuffer;
								gj->action=8;
								strcpy(gj->yourname,char_name);
								strcpy(gj->membername,leader);
								QueuePacket(outapp3);
								safe_delete(outapp3);
							}
						}
					}
				} else {
					//char DefaultMOTD[] = "Welcome to EQ Emu(tm)!";
					//outapp->size = strlen(DefaultMOTD) + 1;
					//outapp->pBuffer = new uchar[outapp->size];
					//strcpy((char*)outapp->pBuffer, DefaultMOTD);
					// Null Message of the Day. :)
					outapp->size = 1;
					outapp->pBuffer = new uchar[outapp->size];
					outapp->pBuffer[0] = 0;
				}
				QueuePacket(outapp);
				safe_delete(outapp);
			}
			APPLAYER *outapp2 = new APPLAYER(OP_SetChatServer);
			char buffer[112];
			sprintf(buffer,"%s,%i,%s.%s,%s",net.GetChatAddress(),net.GetChatPort(),net.GetWorldShortName(),this->GetCharName(),"067a79d4");
			outapp2->size=strlen(buffer)+1;
			outapp2->pBuffer = new uchar[outapp2->size];
			memcpy(outapp2->pBuffer,buffer,outapp2->size);
			QueuePacket(outapp2);
			DumpPacket(outapp2);
			
			firstlogin = false;
			EnterWorld();
			break;
		}
		case OP_LoginComplete:{
			realfirstlogin=true;
			break;
		}
		case OP_DeleteCharacter: {
			cout << "Delete character: " << app->pBuffer << endl;
			database.DeleteCharacter((char *)app->pBuffer);
			SendCharInfo();
			break;
		}
		case OP_ApproveWorld:
		{
			if(seencharsel)
				firstlogin = false;
			else
				firstlogin = true;
			SendCharInfo();
			break;
		}
		case OP_World_Client_CRC1:
		case OP_World_Client_CRC2:
		case OP_WearChange: { // User has selected a different character
			break;
		}
		default: {
			cout << "Received unknown opcode: 0x" << hex << setfill('0') << setw(4) << app->opcode << dec;
			cout << " size:" << app->size << " bytes" << endl;
#if DEBUG >= 5
			DumpPacket(app);
#else
			DumpPacket(app->pBuffer, app->size > 32 ? 32 : app->size);
#endif
			break;
		}
	}
	return ret;
}

bool Client::Process() {
	bool ret = true;
	//bool sendguilds = true;
	//bool firstlogin = true;
    sockaddr_in to;

	memset((char *) &to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_port = port;
    to.sin_addr.s_addr = ip;

	if (autobootup_timeout->Check()) {
		ZoneUnavail();
	}
	if(connect->Check()){
		SendGuildList();// Send OPCode: OP_GuildsList
		SendApproveWorld();
		connect->Disable();
	}
	if (CLE_keepalive_timer->Check()) {
		if (cle)
			cle->KeepAlive();
	}
    
	/************ Get all packets from packet manager out queue and process them ************/
	APPLAYER *app = 0;
	while(ret && (app = eqnc->PopPacket())) {
		ret = HandlePacket(app);

		delete app;
	}    

	if (!eqnc->CheckActive()) {
		cout << "Client disconnected" << endl;
		return false;
	}

	return ret;
}

void Client::EnterWorld(bool TryBootup) {
	//int16 zone_port;
	//char zone_address[255];
	if (zoneID == 0)
		return;
	
	ZoneServer* zs = zoneserver_list.FindByZoneID(zoneID);
	if (zs) {
		// warn the world we're comming, so it knows not to shutdown
		zs->IncommingClient(this);
	}
	else {
		if (TryBootup) {
			autobootup_timeout->Start();
			cout << "Attempting autobootup of " << database.GetZoneName(zoneID, true) << " (" << zoneID << ") for " << char_name << endl;
			if (!(pwaitingforbootup = zoneserver_list.TriggerBootup(zoneID))) {
				cout << "Error: No zoneserver to bootup " << database.GetZoneName(zoneID, true) << " (" << zoneID << ") for " << char_name << endl;
				ZoneUnavail();
			}
			return;
		}
		else {
			cout << "Error: Player '" << char_name << "' requested zone status for " << database.GetZoneName(zoneID, true) << " (" << zoneID << ") but it's not up." << endl;
			ZoneUnavail();
			return;
		}
	}
	pwaitingforbootup = 0;
	
	cle->SetChar(charid, char_name);
	database.UpdateLiveChar(char_name, GetAccountID());
	cout << "Enter world: " << char_name << ": " << database.GetZoneName(zoneID, true) << " (" << zoneID << ")" << endl;
//	database.SetAuthentication(account_id, char_name, zone_name, ip);
	
	if (seencharsel) {
		if (GetAdmin() < 80 && zoneserver_list.IsZoneLocked(zoneID)) {
			ZoneUnavail();
			return;
		}
		
		ServerPacket* pack = new ServerPacket;
		pack->opcode = ServerOP_AcceptWorldEntrance;
		pack->size = sizeof(WorldToZone_Struct);
		pack->pBuffer = new uchar[pack->size];
		memset(pack->pBuffer, 0, pack->size);
		WorldToZone_Struct* wtz = (WorldToZone_Struct*) pack->pBuffer;
		wtz->account_id = GetAccountID();
		wtz->response = 0;
		zs->SendPacket(pack);
		delete pack;
	}
	else {	// if they havent seen character select screen, we can assume this is a zone
			// to zone movement, which should be preauthorized before they leave the previous zone
		Clearance(1);
	}
}

void Client::Clearance(sint8 response)
{
	ZoneServer* zs = zoneserver_list.FindByZoneID(zoneID);
	
    if(zs == 0 || response == -1 || response == 0)
    {
        if (zs == 0)
        {
            cout << "Unable to find zoneserver in Client::Clearance!!" << endl;
        }
		
        ZoneUnavail();
        return;
    }
	
	APPLAYER* outapp;
	
    if (zs->GetCAddress() == NULL) {
        cout << "Unable to do zs->GetCAddress() in Client::Clearance!!" << endl;
        ZoneUnavail();
        return;    
    }
	
    if (zoneID == 0) {
        cout << "zoneID is NULL in Client::Clearance!!" << endl;
        ZoneUnavail();
        return;
    }
	
	const char* zonename = database.GetZoneName(zoneID);
    if (zonename == 0) {
        cout << "zonename is NULL in Client::Clearance!!" << endl;
        ZoneUnavail();
        return;
    }
	
	// @bp This is the chat server
	/*
	char packetData[] = "64.37.148.34.9876,MyServer,Testchar,23cd2c95";
	outapp = new APPLAYER(0x0282, sizeof(packetData));
	strcpy((char*)outapp->pBuffer, packetData);
	QueuePacket(outapp);
	delete outapp;
	*/
	
	// Send zone server IP data
	outapp = new APPLAYER(OP_ZoneServerInfo, sizeof(ZoneServerInfo_Struct));
	ZoneServerInfo_Struct* zsi = (ZoneServerInfo_Struct*)outapp->pBuffer;
    strcpy(zsi->ip, zs->GetCAddress());
	zsi->port = ntohs(zs->GetCPort());
	QueuePacket(outapp);
	safe_delete(outapp);
	
	if (cle)
		cle->SetOnline(CLE_Status_Zoning);
}

void Client::ZoneUnavail() {
	APPLAYER* outapp = new APPLAYER(OP_ZoneUnavail, sizeof(ZoneUnavail_Struct));
	ZoneUnavail_Struct* ua = (ZoneUnavail_Struct*)outapp->pBuffer;
	const char* zonename = database.GetZoneName(zoneID);
	if (zonename)
		strcpy(ua->zonename, zonename);
	QueuePacket(outapp);
	delete outapp;
	
	zoneID = 0;
	pwaitingforbootup = 0;
	autobootup_timeout->Disable();
}

bool Client::GenPassKey(char* key) {
	char* passKey=NULL;
	*passKey += ((char)('A'+((int)(rand()%26))));
	*passKey += ((char)('A'+((int)(rand()%26))));
	memcpy(key, passKey, strlen(passKey));
	return true;
}

void Client::QueuePacket(const APPLAYER* app, bool ack_req) {
	//#if DEBUG == 9
		#ifdef MERTHALICIOUS // just temporary
			cout << "Sending: 0x" << hex << setfill('0') << setw(4) << app->opcode << dec << endl;
			//DumpPacket(app);
		#endif
	//#endif
	
	ack_req = true;	// It's broke right now, dont delete this line till fix it. =P
	if (app != 0) {
		if (app->size > 49156) {
			cout << "WARNING: abnormal packet size. o=0x" << hex << app->opcode << dec << ", s=" << app->size << endl;
		}
	}
	eqnc->QueuePacket(app, ack_req);
}

void Client::SendGuildList() {
	APPLAYER *outapp;
	outapp = new APPLAYER(OP_GuildsList, sizeof(GuildsList_Struct));
	GuildsList_Struct* gl = (GuildsList_Struct*) outapp->pBuffer;
	
	for (int i=0; i < 512; i++) {
		if (guilds[i].databaseID != 0) {
			strcpy(gl->Guilds[i].name, guilds[i].name);
		}
	}
	outapp->Deflate();
	this->QueuePacket(outapp);
	delete outapp;
}

ClientList::ClientList() {
}

ClientList::~ClientList() {
}

void ClientList::Add(Client* client) {
	list.Insert(client);
}

Client* ClientList::FindByAccountID(int32 account_id) {
	LinkedListIterator<Client*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		LogFile->write(EQEMuLog::Debug, "World: ClientList[0x%08x]::FindByAccountID(0x%08x) iterator.GetData()[0x%08x]", (int32) this, account_id, iterator.GetData());
		if (iterator.GetData()->GetAccountID() == account_id) {
			Client* tmp = iterator.GetData();
			return tmp;
		}
		iterator.Advance();
	}
	return 0;
}

Client* ClientList::FindByName(char* charname) {
	LinkedListIterator<Client*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->GetCharName() == charname) {
			Client* tmp = iterator.GetData();
			return tmp;
		}
		iterator.Advance();
	}
	return 0;
}

Client* ClientList::Get(int32 ip, int16 port) {
	LinkedListIterator<Client*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GetIP() == ip && iterator.GetData()->GetPort() == port)
		{
			Client* tmp = iterator.GetData();
			return tmp;
		}
		iterator.Advance();
	}
	return 0;
}

void ClientList::Process() {
	LinkedListIterator<Client*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (!iterator.GetData()->Process()) {
			struct in_addr  in;
			in.s_addr = iterator.GetData()->GetIP();
			cout << "Removing client from ip:" << inet_ntoa(in) << " port:" << iterator.GetData()->GetPort() << endl;
			iterator.RemoveCurrent();
		}
		else
			iterator.Advance();
	}
}

void ClientList::ZoneBootup(ZoneServer* zs) {
	LinkedListIterator<Client*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->WaitingForBootup()) {
			if (iterator.GetData()->GetZoneID() == zs->GetZoneID()) {
				iterator.GetData()->EnterWorld(false);
			}
			else if (iterator.GetData()->WaitingForBootup() == zs->GetID()) {
				iterator.GetData()->ZoneUnavail();
			}
		}
		iterator.Advance();
	}
}

void ClientList::RemoveCLEReferances(ClientListEntry* cle) {
	LinkedListIterator<Client*> iterator(list);

	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->GetCLE() == cle) {
			iterator.GetData()->SetCLE(0);
		}
		iterator.Advance();
	}
}

// @merth: I have no idea what this struct is for, so it's hardcoded for now
void Client::SendApproveWorld()
{
	APPLAYER* outapp;
	
	// Send OPCode: OP_ApproveWorld, size: 544
	outapp = new APPLAYER(OP_ApproveWorld, sizeof(ApproveWorld_Struct));
	ApproveWorld_Struct* aw = (ApproveWorld_Struct*)outapp->pBuffer;
	uchar foo[] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x95,0x5E,0x30,0xA5,0xCA,0xD4,0xEA,0xF5,
0xCB,0x14,0xFC,0xF7,0x78,0xE2,0x73,0x15,0x90,0x17,0xCE,0x7A,0xEB,0xEC,0x3C,0x34,
0x5C,0x6D,0x10,0x05,0xFC,0xEA,0xED,0x19,0xC5,0x0D,0x7A,0x82,0x17,0xCC,0xCC,0x71,
0x56,0x38,0xDF,0x78,0x8D,0xE6,0x44,0xD3,0x6F,0xDB,0xE3,0xCF,0x21,0x30,0x75,0x2F,
0xCD,0xDC,0xE9,0xB4,0xA4,0x4E,0x58,0xDE,0xEE,0x54,0xDD,0x87,0xDA,0xE9,0xC6,0xC8,
0x02,0xDD,0xC4,0xFD,0x94,0x36,0x32,0xAD,0x1B,0x39,0x0F,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x15,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x53,0xC3,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00};
	memcpy(aw->unknown544, foo, sizeof(foo));
	QueuePacket(outapp);
	safe_delete(outapp);
}

bool Client::OPCharCreate(CharCreate_Struct *cc)
{
	PlayerProfile_Struct pp; 
	Inventory inv;
	time_t bday = time(NULL);
	char startzone[50]={0};
	int i;
	struct in_addr	in;

			
	int stats_sum = cc->STR + cc->STA + cc->AGI + cc->DEX + 
		cc->WIS + cc->INT + cc->CHA;

	in.s_addr = GetIP();
	printf("Character creation request from %s LS#%d (%s:%d) : \n", GetCLE()->LSName(), GetCLE()->LSID(), inet_ntoa(in), GetPort());
	printf("Name: %s\n", cc->name);
	printf("Race: %d  Class: %d  Gender: %d  Deity: %d  Start zone: %d\n",
		cc->race, cc->class_, cc->gender, cc->deity, cc->start_zone);
	printf("STR  STA  AGI  DEX  WIS  INT  CHA    Total\n");
	printf("%3d  %3d  %3d  %3d  %3d  %3d  %3d     %3d\n",
		cc->STR, cc->STA, cc->AGI, cc->DEX, cc->WIS, cc->INT, cc->CHA, 
		stats_sum);
	printf("Face: %d  Eye colors: %d %d\n", cc->face, cc->eyecolor1, cc->eyecolor2);
	printf("Hairstyle: %d  Haircolor: %d\n", cc->hairstyle, cc->haircolor);
	printf("Beard: %d  Beardcolor: %d\n", cc->beard, cc->beardcolor);

	// validate the char creation struct
	if(!CheckCharCreateInfo(cc))
	{
		printf("CheckCharCreateInfo did not validate the request (bad race/class/stats)\n");
		return false;
	}

	// Convert incoming cc_s to the new PlayerProfile_Struct
	memset(&pp, 0, sizeof(PlayerProfile_Struct));	// start building the profile

	strncpy(pp.name, cc->name, 63);
	// clean the capitalization of the name
#if 0	// on second thought, don't - this will just make the creation fail
// because the name won't match what was already reserved earlier
	for (i = 0; pp.name[i] && i < 63; i++)
	{
		if(!isalpha(pp.name[i]))
			return false;
		pp.name[i] = tolower(pp.name[i]);
	}
	pp.name[0] = toupper(pp.name[0]);	
#endif

	pp.race				= cc->race;
	pp.class_			= cc->class_;
	pp.gender			= cc->gender;
	pp.deity			= cc->deity;
	pp.STR				= cc->STR;
	pp.STA				= cc->STA;
	pp.AGI				= cc->AGI;
	pp.DEX				= cc->DEX;
	pp.WIS				= cc->WIS;
	pp.INT				= cc->INT;
	pp.CHA				= cc->CHA;
	pp.face				= cc->face;
	pp.eyecolor1	= cc->eyecolor1;
	pp.eyecolor2	= cc->eyecolor2;
	pp.hairstyle	= cc->hairstyle;
	pp.haircolor	= cc->haircolor;
	pp.beard		 	= cc->beard;
	pp.beardcolor	= cc->beardcolor;

	pp.birthday		= bday;
	pp.lastlogin	= bday;
	pp.level			= 1;
	pp.points			= 5;
	pp.cur_hp			= 1000; // 1k hp during dev only
	pp.expAA			= 0xFFFFFFFF;

	// FIXME: FV roleplay, database goodness...

	// Racial Languages
	SetRacialLanguages( &pp ); // bUsh
	SetRaceStartingSkills( &pp ); // bUsh
	SetClassStartingSkills( &pp ); // bUsh
	
	pp.unknown3596[28] = 15; // @bp: This is to enable disc usage
	strcpy(pp.servername, "eqemu");
			

	for(i = 0; i < MAX_PP_SPELLBOOK; i++)
		pp.spell_book[i] = 0xFFFFFFFF;

	for(i = 0; i < MAX_PP_MEMSPELL; i++)
		pp.mem_spells[i] = 0xFFFFFFFF;

	for(i = 0; i < BUFF_COUNT; i++)
		pp.buffs[i].spellid = 0xFFFF;


	memset(pp.unknown2384, 0xff, 448);
	memset(pp.unknown2864, 0xff, 32);
	
	//If server is PVP by default, make all character set to it.
	pp.pvp = database.GetServerType() == 1 ? 1 : 0;			
		
	// if there's a startzone variable put them in there
	if(database.GetVariable("startzone", startzone, 50))
	{
		printf("Found 'startzone' variable setting: %s\n", startzone);
		pp.zone_id = database.GetZoneID(startzone);
		if(pp.zone_id)
			database.GetSafePoints(pp.zone_id, &pp.x, &pp.y, &pp.z);
		else
			printf("Error getting zone id for '%s'\n", startzone);
	}
	else	// otherwise use normal starting zone logic
	{
		database.GetStartZone(&pp, cc);
	}

	if(!pp.zone_id)
	{
		pp.zone_id = 1;		// qeynos
		pp.x = pp.y = pp.z = -1;
	}

	if(!pp.bind_zone_id)
	{
		pp.bind_zone_id = pp.zone_id;
		pp.bind_x = pp.x;
		pp.bind_y = pp.y;
		pp.bind_z = pp.z;
 	}
		
	printf("Current location: %s  %0.2f, %0.2f, %0.2f\n",
		database.GetZoneName(pp.zone_id), pp.x, pp.y, pp.z);
	printf("Bind location: %s  %0.2f, %0.2f, %0.2f\n",
		database.GetZoneName(pp.bind_zone_id), pp.bind_x, pp.bind_y, pp.bind_z);


	// Starting Items inventory
	database.SetStartingItems(&pp, &inv, pp.race, pp.class_, pp.deity, pp.zone_id, pp.name, GetAdmin());
			
			
	// now we give the pp and the inv we made to StoreCharacter
	// to see if we can store it
	if (!database.StoreCharacter(GetAccountID(), &pp, &inv))
	{
		printf("Character creation failed: %s\n", pp.name);
		return false;
	}
	else
	{
		printf("Character creation successful: %s\n", pp.name);
		return true;
	}
}

// returns true if the request is ok, false if there's an error
bool CheckCharCreateInfo(CharCreate_Struct *cc)
{
	int bSTR, bSTA, bAGI, bDEX, bWIS, bINT, bCHA, bTOTAL, cTOTAL, stat_points;
	int classtemp, racetemp;
	int Charerrors = 0;


// solar: if this is increased you'll have to add a column to the classrace
// table below
#define _TABLE_RACES	15

	int BaseRace[_TABLE_RACES][7] =
	{          /* STR  STA  AGI  DEX  WIS  INT  CHR */
	/*Human*/      75,  75,  75,  75,  75,  75,  75,
	/*Barbarian*/ 103,  95,  82,  70,  70,  60,  55,
	/*Erudite*/    60,  70,  70,  70,  83, 107,  70,
	/*Wood Elf*/   65,  65,  95,  80,  80,  75,  75,
	/*High Elf*/   55,  65,  85,  70,  95,  92,  80,
	/*Dark Elf*/   60,  65,  90,  75,  83,  99,  60,                
	/*Half Elf*/   70,  70,  90,  85,  60,  75,  75,
	/*Dwarf*/      90,  90,  70,  90,  83,  60,  45,
	/*Troll*/     108, 109,  83,  75,  60,  52,  40,
	/*Ogre*/      130, 122,  70,  70,  67,  60,  37,
	/*Halfling*/   70,  75,  95,  90,  80,  67,  50,
	/*Gnome*/      60,  70,  85,  85,  67,  98,  60,
	/*Iksar*/      70,  70,  90,  85,  80,  75,  55,
	/*Vah Shir*/   90,  75,  90,  70,  70,  65,  65,
	/*Froglok*/    70,  80, 100, 100,  75,  75,  50 
	};

	int BaseClass[PLAYER_CLASS_COUNT][8] =
	{            /* STR  STA  AGI  DEX  WIS  INT  CHR  ADD*/
	/*Warrior*/      10,  10,   5,   0,   0,   0,   0,  25,
	/*Cleric*/        5,   5,   0,   0,  10,   0,   0,  30,
	/*Paladin*/      10,   5,   0,   0,   5,   0,  10,  20,
	/*Ranger*/        5,  10,  10,   0,   5,   0,   0,  20,
	/*ShadowKnight*/ 10,   5,   0,   0,   0,   10,  5,  20,
	/*Druid*/         0,  10,   0,   0,  10,   0,   0,  30,
	/*Monk*/          5,   5,  10,  10,   0,   0,   0,  20,                
	/*Bard*/          5,   0,   0,  10,   0,   0,  10,  25,
	/*Rouge*/         0,   0,  10,  10,   0,   0,   0,  30,
	/*Shaman*/        0,   5,   0,   0,  10,   0,   5,  30,
	/*Necromancer*/   0,   0,   0,  10,   0,  10,   0,  30,
	/*Wizard*/        0,  10,   0,   0,   0,  10,   0,  30,
	/*Magician*/      0,  10,   0,   0,   0,  10,   0,  30,
	/*Enchanter*/     0,   0,   0,   0,   0,  10,  10,  30,
	/*Beastlord*/     0,  10,   5,   0,  10,   0,   5,  20,
	/*Berserker*/    10,   5,   0,  10,   0,   0,   0,  25
	};

	bool ClassRaceLookupTable[PLAYER_CLASS_COUNT][_TABLE_RACES]= 
	{                 /*Human  Barbarian Erudite Woodelf Highelf Darkelf Halfelf Dwarf  Troll  Ogre   Halfling Gnome  Iksar  Vahshir Froglok*/
	/*Warrior*/         true,  true,     false,  true,   false,  true,   true,   true,  true,  true,  true,    true,  true,  true,   true,
	/*Cleric*/          true,  false,    true,   false,  true,   true,   true,   true,  false, false, true,    true,  false, false,  true,  
	/*Paladin*/         true,  false,    true,   false,  true,   false,  true,   true,  false, false, true,    true,  false, false,  true,
	/*Ranger*/          true,  false,    false,  true,   false,  false,  true,   false, false, false, true,    false, false, false,  false,
	/*ShadowKnight*/    true,  false,    true,   false,  false,  true,   false,  false, true,  true,  false,   true,  true,  false,  false,
	/*Druid*/           true,  false,    false,  true,   false,  false,  true,   false, false, false, true,    false, false, false,  false,    
	/*Monk*/            true,  false,    false,  false,  false,  false,  false,  false, false, false, false,   false, true,  false,  false,
	/*Bard*/            true,  false,    false,  true,   false,  false,  true,   false, false, false, false,   false, false, true,   false,
	/*Rogue*/           true,  true,     false,  true,   false,  true,   true,   true,  false, false, true,    true,  false, true,   false,
	/*Shaman*/          false, true,     false,  false,  false,  false,  false,  false, true,  true,  false,   false, true,  true,   true,
	/*Necromancer*/     true,  false,    true,   false,  false,  true,   false,  false, false, false, false,   true,  true,  false,  false,
	/*Wizard*/          true,  false,    true,   false,  true,   true,   false,  false, false, false, false,   true,  false, false,  true,
	/*Magician*/        true,  false,    true,   false,  true,   true,   false,  false, false, false, false,   true,  false, false,  false,
	/*Enchanter*/       true,  false,    true,   false,  true,   true,   false,  false, false, false, false,   true,  false, false,  false,  
	/*Beastlord*/       false, true,     false,  false,  false,  false,  false,  false, true,  true,  false,   false, true,  true,   false,
	/*Berserker*/       false, true,     false,  false,  false,  false,  false,  true,  true,  true,  false,   false, false, true,   false
	};//Initial table by kathgar, editted by Wiz for accuracy, solar too

	if(!cc) return false;

	printf("Validating char creation info...\n");

	classtemp = cc->class_ - 1;
	racetemp = cc->race - 1;
	// these have non sequential race numbers so they need to be mapped
	if (cc->race == FROGLOK) racetemp = 14;
	if (cc->race == VAHSHIR) racetemp = 13;
	if (cc->race == IKSAR) racetemp = 12;

	// if out of range looking it up in the table would crash stuff
	// so we return from these
	if(classtemp >= PLAYER_CLASS_COUNT)
	{
		printf("  class is out of range\n");
		return false;
	}
	if(racetemp >= _TABLE_RACES)
	{
		printf("  race is out of range\n");
		return false;
	}

	if(!ClassRaceLookupTable[classtemp][racetemp]) //Lookup table better than a bunch of ifs?
	{
		printf("  invalid race/class combination\n");
		// we return from this one, since if it's an invalid combination our table
		// doesn't have meaningful values for the stats
		return false;
	}

#ifdef GUILDWARS
	if
	(
		cc->race == FROGLOK ||
		cc->race == VAHSHIR ||
		cc->race == IKSAR ||
		cc->class_ == BEASTLORD ||
		cc->class_ == BERSERKER
	)
	{
		printf("  GuildWars rules disallow this character\n");
		Charerrors++;
	}
#endif

	// solar: add up the base values for this class/race
	// this is what they start with, and they have stat_points more
	// that can distributed
	bSTR = BaseClass[classtemp][0] + BaseRace[racetemp][0];
	bSTA = BaseClass[classtemp][1] + BaseRace[racetemp][1];
	bAGI = BaseClass[classtemp][2] + BaseRace[racetemp][2];
	bDEX = BaseClass[classtemp][3] + BaseRace[racetemp][3];
	bWIS = BaseClass[classtemp][4] + BaseRace[racetemp][4];
	bINT = BaseClass[classtemp][5] + BaseRace[racetemp][5];
	bCHA = BaseClass[classtemp][6] + BaseRace[racetemp][6];
	stat_points = BaseClass[classtemp][7];
	bTOTAL = bSTR + bSTA + bAGI + bDEX + bWIS + bINT + bCHA;
	cTOTAL = cc->STR + cc->STA + cc->AGI + cc->DEX + cc->WIS + cc->INT + cc->CHA;

	// solar: the first check makes sure the total is exactly what was expected.
	// this will catch all the stat cheating, but there's still the issue
	// of reducing CHA or INT or something, to use for STR, so we check
	// that none are lower than the base or higher than base + stat_points
	// NOTE: these could just be else if, but i want to see all the stats
	// that are messed up not just the first hit

	if(bTOTAL + stat_points != cTOTAL)
	{
		printf("  stat points total doesn't match expected value: expecting %d got %d\n", bTOTAL + stat_points, cTOTAL);
		Charerrors++;
	}

	if(cc->STR > bSTR + stat_points || cc->STR < bSTR)
	{
		printf("  stat STR is out of range\n");
		Charerrors++;
	}
	if(cc->STA > bSTA + stat_points || cc->STA < bSTA)
	{
		printf("  stat STA is out of range\n");
		Charerrors++;
	}
	if(cc->AGI > bAGI + stat_points || cc->AGI < bAGI)
	{
		printf("  stat AGI is out of range\n");
		Charerrors++;
	}
	if(cc->DEX > bDEX + stat_points || cc->DEX < bDEX)
	{
		printf("  stat DEX is out of range\n");
		Charerrors++;
	}
	if(cc->WIS > bWIS + stat_points || cc->WIS < bWIS)
	{
		printf("  stat WIS is out of range\n");
		Charerrors++;
	}
	if(cc->INT > bINT + stat_points || cc->INT < bINT)
	{
		printf("  stat INT is out of range\n");
		Charerrors++;
	}
	if(cc->CHA > bCHA + stat_points || cc->CHA < bCHA)
	{
		printf("  stat CHA is out of range\n");
		Charerrors++;
	}

	/*TODO: Check for deity/class/race.. it'd be nice, but probably of any real use to hack(faction, deity based items are all I can think of)
	I am NOT writing those tables - kathgar*/

	printf("Found %d errors in character creation request\n", Charerrors);

	return Charerrors == 0;
}

void Client::SetClassStartingSkills( PlayerProfile_Struct *pp )
{
   switch( pp->class_ )
   {
   case BARD:
      {
         pp->skills[_1H_SLASHING + 1] = 5;
         pp->skills[SINGING + 1] = 5;
         break;
      }
   case BEASTLORD:
      {
         pp->skills[HAND_TO_HAND + 1] = 5;
         break;
      }
   case BERSERKER: // A Guess
      {
         pp->skills[_2H_SLASHING + 1] = 5;
         break;
      }
   case CLERIC:
      {
         pp->skills[_1H_BLUNT + 1] = 5;
         break;
      }
   case DRUID:
      {
         pp->skills[_1H_BLUNT + 1] = 5;
         break;
      }
   case ENCHANTER:
      {
         pp->skills[PIERCING + 1] = 5;
         break;
      }
   case MAGICIAN:
      {
         pp->skills[PIERCING + 1] = 5;
         break;
      }
   case MONK:
      {
         pp->skills[DODGE + 1] = 5;
         pp->skills[DUAL_WIELD + 1] = 5;
         pp->skills[HAND_TO_HAND + 1] = 5;
         break;
      }
   case NECROMANCER:
      {
         pp->skills[PIERCING + 1] = 5;
         break;
      }
   case PALADIN:
      {
         pp->skills[_1H_SLASHING + 1] = 5;
         break;
      }
   case RANGER:
      {
         pp->skills[_1H_SLASHING + 1] = 5;
         break;
      }
   case ROGUE:
      {
         pp->skills[PIERCING + 1] = 5;
         pp->languages[LANG_THIEVES_CANT] = 100; // Thieves Cant
         break;
      }
   case SHADOWKNIGHT:
      {
         pp->skills[_1H_SLASHING + 1] = 5;
         break;
      }
   case SHAMAN:
      {
         pp->skills[_1H_BLUNT + 1] = 5;
         break;
      }
   case WARRIOR:
      {
         pp->skills[_1H_SLASHING + 1] = 5;
         break;
      }
   case WIZARD:
      {
         pp->skills[PIERCING + 1] = 5;
         break;
      }
   }
}

void Client::SetRaceStartingSkills( PlayerProfile_Struct *pp )
{ 
   switch( pp->race )
   {
   case BARBARIAN:
   case DWARF:
   case ERUDITE:
   case HALF_ELF:
   case HIGH_ELF:
   case HUMAN:
   case OGRE:
   case TROLL:
      {
         // No Race Specific Skills
         break;
      }
   case DARK_ELF:
      {
         pp->skills[HIDE + 1] = 50;
         break;
      }
   case FROGLOK:
      {
         pp->skills[SWIMMING + 1] = 125;
         break;
      }
   case GNOME:
      {
         pp->skills[TINKERING + 1] = 50;
         break;
      }
   case HALFLING:
      {
         pp->skills[HIDE + 1] = 50;
         pp->skills[SNEAK + 1] = 50;
         break;
      }
   case IKSAR:
      {
         pp->skills[FORAGE + 1] = 50;
         pp->skills[SWIMMING + 1] = 100;
         break;
      }
   case WOOD_ELF:
      {
         pp->skills[FORAGE + 1] = 50;
         pp->skills[HIDE + 1] = 50;
         break;
      }
   case VAHSHIR:
      {
         pp->skills[SAFE_FALL + 1] = 50;
         pp->skills[SNEAK + 1] = 50;
         break;
      }
   }
}

void Client::SetRacialLanguages( PlayerProfile_Struct *pp )
{
   switch( pp->race )
   {
   case BARBARIAN:
      {
         pp->languages[LANG_COMMON_TONGUE] = 100;
         pp->languages[LANG_BARBARIAN] = 100;
         break;
      }
   case DARK_ELF:
      {
         pp->languages[LANG_COMMON_TONGUE] = 100;
         pp->languages[LANG_DARK_ELVISH] = 100;
         pp->languages[LANG_DARK_SPEECH] = 100;
         pp->languages[LANG_ELDER_ELVISH] = 100;
         pp->languages[LANG_ELVISH] = 25;
         break;
      }
   case DWARF:
      {
         pp->languages[LANG_COMMON_TONGUE] = 100;
         pp->languages[LANG_DWARVISH] = 100;
         pp->languages[LANG_GNOMISH] = 25;
         break;
      }
   case ERUDITE:
      {
         pp->languages[LANG_COMMON_TONGUE] = 100;
         pp->languages[LANG_ERUDIAN] = 100;
         break;
      }
   case FROGLOK:
      {
         pp->languages[LANG_COMMON_TONGUE] = 100;
         pp->languages[LANG_FROGLOK] = 100;
         pp->languages[LANG_TROLL] = 25;
         break;
      }
   case GNOME:
      {
         pp->languages[LANG_COMMON_TONGUE] = 100;
         pp->languages[LANG_DWARVISH] = 25;
         pp->languages[LANG_GNOMISH] = 100;
         break;
      }
   case HALF_ELF:
      {
         pp->languages[LANG_COMMON_TONGUE] = 100;
         pp->languages[LANG_ELVISH] = 100;
         break;
      }
   case HALFLING:
      {
         pp->languages[LANG_COMMON_TONGUE] = 100;
         pp->languages[LANG_HALFLING] = 100;
         break;
      }
   case HIGH_ELF:
      {
         pp->languages[LANG_COMMON_TONGUE] = 100;
         pp->languages[LANG_DARK_ELVISH] = 25;
         pp->languages[LANG_ELDER_ELVISH] = 25;
         pp->languages[LANG_ELVISH] = 100;
         break;
      }
   case HUMAN:
      {
         pp->languages[LANG_COMMON_TONGUE] = 100;
         break;
      }
   case IKSAR:
      {
         pp->languages[LANG_COMMON_TONGUE] = 95;
         pp->languages[LANG_DARK_SPEECH] = 100;
         pp->languages[LANG_LIZARDMAN] = 100;
         break;
      }
   case OGRE:
      {
         pp->languages[LANG_COMMON_TONGUE] = 95;
         pp->languages[LANG_DARK_SPEECH] = 100;
         pp->languages[LANG_OGRE] = 100;
         break;
      }
   case TROLL:
      {
         pp->languages[LANG_COMMON_TONGUE] = 95;
         pp->languages[LANG_DARK_SPEECH] = 100;
         pp->languages[LANG_TROLL] = 100;
         break;
      }
   case WOOD_ELF:
      {
         pp->languages[LANG_COMMON_TONGUE] = 100;
         pp->languages[LANG_ELVISH] = 100;
         break;
      }
   case VAHSHIR:
      {
         pp->languages[LANG_COMMON_TONGUE] = 100;
         pp->languages[LANG_COMBINE_TONGUE] = 100;
         pp->languages[LANG_ERUDIAN] = 25;
         pp->languages[LANG_VAH_SHIR] = 100;
         break;
      }
   }
}
