// Quest Parser written by Wes, Leave this here =P
//Variables causing crash with linked lists (Bad pointers being added to the lists) - Npcs causing crashes with linked lists


#include <iostream>
#include <ctype.h>
using namespace std;
#include <fstream>
using namespace std;
#include <string>
#include <list>
#include <algorithm>

using namespace std;
#include "../common/debug.h"
#include "entity.h"
#include "masterentity.h"

#include "worldserver.h"
#include "net.h"
#include "../common/skills.h"
#include "../common/classes.h"
#include "../common/races.h"
#include "../common/database.h"
#include "../common/files.h"
#include "spdat.h"
#include "../common/packet_functions.h"
#include "spawn2.h"
#include "zone.h"
#include "event_codes.h"
#include <time.h>
#include "parser.h"
#include "basic_functions.h"

extern Database database;
extern Zone* zone;
extern WorldServer worldserver;
extern EntityList entity_list;
std::list<timers*> TimerList;


#define Parser_DEBUG 1

int mindex1 = 0;

const char * charIn3 = "`~1234567890-=!@#$%^&*_+qwertyuiop[]asdfghjkl'zxcvbnm,./QWERTYUIOP|ASDFGHJKL:ZXCVBNM<>?\"\\";

// MYRA - restore missing commands for qst type files & add itemlink
string cmds("if 0|break 1|spawn 6|spawn2 7|settimer 2|stoptimer 1|rebind 4|echo 1|summonitem 1|selfcast 1|zone 1|castspell 2|say 1|emote 1|shout 1|shout2 1|depop 1|cumflag 1|flagnpc 1|exp 1|level 1|safemove 1|rain 1|snow 1|givecash 4|pvp 1|doanim 1|addskill 2|flagcheck 1|me 1|write 2|settarget 2|follow 1|sfollow 1|save 1|setallskill 1|faction 2|settime 2|setguild 2|setsky 1|setstat 2|movepc 4|gmmove 3|movegrp 4|signal 1|attack 1|itemlink 1|setglobal 4|delglobal 1|targlobal 6|setskill 2|setlanguage 2|stop 0|resume 0|start 1|moveto 3|pause 1|addldonpoints 2|");


//end Myra


int GetArgs(string command)
{
	string::iterator iterator = cmds.begin();
	string buffer;
	string cmd;
	string argnum;
	while (*iterator) {
		if (*iterator == ' ')
		{
			if (buffer.compare(command)==0) {
				cmd = buffer;
				buffer="";
			}
		}
		else {
			if (*iterator != '|')
			buffer += *iterator;
		}
		if (*iterator == '|')
		{
			if (!cmd.empty()) {
			int argnums = atoi(buffer.c_str());
			return argnums;
			}
			else {
				buffer="";
			}
		}
	iterator++;
	}
	return -1;
}

int calc(string calc)
{
	string::iterator iterator = calc.begin();
	string buffer;
	string integer1;
	char op = 0;
	int returnvalue = 0;
	
	while (*iterator) {
		char ch = *iterator;
	    
		if(ch >= '0' && ch <= '9') { //If this character is numeric add it to the buffer
			buffer += ch;
		}
		else if(ch == '+' || ch == '-' || ch == '/' || ch == '*') { //otherwise, are we an operator?
			int val = atoi(buffer.c_str());
			if (!op) { //if this is the first time through, set returnvalue to what we have so far
				returnvalue = val;
			}
			else { //otherwise we've got returnvalue initialized, perform operation on previous numbers
				if (buffer.length() == 0) { //Do we have a value?
					printf("Parser::calc() Error in syntax: '%s'.\n", calc.c_str());
					return 0;
				}
				
				//what was the previous op
				switch(op)
				{
				case '+': {
					returnvalue += val;
					break;
				}
				case '-': {
					returnvalue -= val;
					break;
				}
				case '/': {
					if(val == 0)//can't divide by zero
					{
						printf("Parser::calc() Error, Divide by zero '%s'.\n", calc.c_str());
						return 0;
					}
					returnvalue /= val;
					break;
				}
				case '*': {
					returnvalue *= val;
					break;
				}
				};
				
				op = ch; //save current operator and continue parsing
			}
			buffer=""; //clear buffer now that we're starting on a new number
			op = ch;
		}
		else {
			printf("Parser::calc() Error processing '%c'.\n", ch);
			return 0;
		}
	}
	return returnvalue;
}

int Parser::numtok(string text, const char * character) {
	string::iterator iterator = text.begin();
	int returnvalue=0;
	while(*iterator) {
		if (*iterator == *character) returnvalue++;
		iterator++;
	}
	return returnvalue;
}
#ifdef WIN32
char* itoa(int integer) {static char tmp[10];itoa(integer,tmp,10);return tmp;}
#endif
string strlwr(string tmp) {
	string res;
	transform(tmp.begin(), tmp.end(), res.begin(), (int(*)(int))tolower);
	return(res);
}

int strcmp(const string &com, const string &com2) {
	return strcmp(com.c_str(),com2.c_str());
}

string gettok(string text, string character, int index)
{
	string::iterator iterator = text.begin();
	string buffer;
	int find=0;
	while (*iterator)
	{
		if (*iterator != character[0])
			buffer+=*iterator;
		else {
			if (find == index)
				break;
			buffer="";
			find++;
		}
		iterator++;
	}
	return buffer;
}

void Parser::MakeVars(string text, int32 npcid) {
	string buffer;
	string temp;
	int pos = numtok(text," ")+1;
	for(int i=0;i<pos;i++)
	{
			buffer = gettok(text,string(" "),i).c_str();
			temp = (string)itoa(i+1); temp += "."; temp += (string)itoa(npcid);
#if Parser_DEBUG>10
				printf("Buffer: %s, temp: %s\n",buffer.c_str(),temp.c_str());
#endif
AddVar(temp,buffer);
			temp="";
			temp=(string)itoa(i+1) + "-." + (string)itoa(npcid);
			AddVar(temp,strstr(text.c_str(),buffer.c_str()));
			buffer="";
	}

}


int Parser::CheckAliases(const char * alias, int32 npcid, Mob* npcmob, Mob* mob)
{
/*	MyListItem <Alias> * Ptr = AliasList.First;

	while (Ptr) {
		if ( (int32)Ptr->Data->npcid == npcid) {
			for (int i=0; i <= Ptr->Data->index; i++) {
				if (!strcmp(strlwr(Ptr->Data->name[i]),alias)) {
					CommandEx(Ptr->Data->command[i], npcid, npcmob, mob);
					return 1;
				}
			}
		}
		Ptr = Ptr->Next;
	}
	return 0;*/
	return 0;
}


int Parser::pcalc(const char * string) {
	char temp[100];
	memset(temp, 0, sizeof(temp));
	char temp2[100];
	memset(temp2, 0, sizeof(temp2));
	int p =0;
	char temp3[100];
	memset(temp3, 0, sizeof(temp3));
	char temp4[100];
	memset(temp4, 0, sizeof(temp4));
	while (strrchr(string,'(')) {
		strn0cpy(temp,strrchr(string,'('), sizeof(temp));
		for ( unsigned int i=0;i < strlen(temp); i++ ) {
			if (temp[i] != '(' && temp[i] != ')') {
				temp2[p] = temp[i];
				p++;
			}
			else if (temp[i] == ')') {
				snprintf(temp3, sizeof(temp3), "(%s)", temp2);
//				Replace(string,temp3,itoa(calc(temp2),temp4,10),0);
				memset(temp, 0, sizeof(temp));
				memset(temp2, 0, sizeof(temp2));
				memset(temp3, 0, sizeof(temp3));
				memset(temp4, 0, sizeof(temp4));
				p=0;
			}
		}
	}
	return calc(string);
}

void Parser::MakeParms(const char * string, int32 npcid) {
	char temp[100];
	memset(temp, 0, sizeof(temp));
	char temp2[100];
	memset(temp2, 0, sizeof(temp2));
	char temp3[100];
	memset(temp3, 0, sizeof(temp3));

	int tmpfor = numtok(string, ",")+1;
	for ( int i=0; i < tmpfor; i++) {
		memset(temp2, 0, sizeof(temp2));
		strn0cpy(temp2, gettok(string, ',', i), sizeof(temp2));
		snprintf(temp, sizeof(temp), "param%s.%d", itoa(i+1 ,temp3, 10),npcid);
		AddVar(temp, temp2);
	}
}

int Parser::GetItemCount(string itemid, int32 npcid)
{
	string temp;
	int a=0;
	for (int i=1;i<5;i++)
	{
		temp = (string)"item" + (string)itoa(i);
		if (!GetVar(temp,npcid).compare(itemid))
			a++;
		temp="";
	}
	return a;
}

int Parser::HasQuestFile(int32 npcid)
{
	sint32 qstID = GetNPCqstID(npcid);
	int success=1;
	if (qstID==-1)
		success = LoadScript(npcid, zone->GetShortName());
	if (!success) 
		return false;

return true;
}

void Parser::Event(int event, int32 npcid, const char * data, Mob* npcmob, Mob* mob) {
	if (npcid == 0)
		return;
	sint32 qstID = GetNPCqstID(npcid);
	int success=1;
	if (qstID==-1)
		success = LoadScript(npcid, zone->GetShortName(),mob);
	if (!success) 
		return;
	else
		qstID = GetNPCqstID(npcid);

// SCORPIOUS2K - load global variables

	if (npcmob->GetQglobal())
	{
		char errbuf[MYSQL_ERRMSG_SIZE];
		char *query = 0;
		MYSQL_RES *result;
		MYSQL_ROW row;
		char tmpname[65];
		int charid=0;
		
			if (mob && mob->IsClient())  // some events like waypoint and spawn don't have a player involved
			{
					charid=mob->CastToClient()->CharacterID();
			}

		else
		{
			charid=0-npcmob->GetNPCTypeID();		// make char id negative npc id as a fudge
		}

		AddVar("charid.g",itoa(charid));

		database.RunQuery(query, MakeAnyLenString(&query, 
		  "SELECT name,value FROM quest_globals WHERE (npcid=%i || npcid=0) && (charid=%i || charid=0) && (zoneid=%i || zoneid=0) && expdate >= unix_timestamp(now())", 
		     npcmob->GetNPCTypeID(),charid,zone->GetZoneID()), errbuf, &result);
		printf("%s\n",query);
		printf("%s\n",errbuf);
		if (result)
		{
			printf("Loading global variables for %s\n",npcmob->GetName());
			while ((row = mysql_fetch_row(result)))
			{
				sprintf(tmpname,"%s.g",row[0]);
				AddVar(tmpname, row[1]);
			}
			mysql_free_result(result);		
		}
		if (query)
		{
			safe_delete_array(query);
			query=0;
		}
	}

	if (event == EVENT_TIMER)
	{
		AddVar("timername.g",data);
	}
	int8 fac = 0;
	if (mob && mob->IsClient()) {		
		AddVar("uguildid.g", itoa(mob->CastToClient()->GuildDBID()));
		AddVar("uguildrank.g", itoa(mob->CastToClient()->GuildRank()));
	}

	if (mob && npcmob && mob->IsClient() && npcmob->IsNPC()) {
		Client* client = mob->CastToClient();
		NPC* npc = npcmob->CastToNPC();
		
		// Need to figure out why one of these casts would fail..
		if (client && npc) {
			fac = client->GetFactionLevel(client->GetID(), npcmob->GetID(), client->GetRace(), client->GetClass(), DEITY_AGNOSTIC, npc->GetPrimaryFaction(), npcmob);
		}
		else if (!client) {
			cerr << "WARNING: cast failure on mob->CastToClient()" << endl;
		}
		else if (!npc) {
			cerr << "WARNING: cast failure on npcmob->CastToNPC()" << endl;
		}
	}
	if (mob) {
		AddVar("name.g",   mob->GetName());
		AddVar("race.g",   GetRaceName(mob->GetRace()));
		AddVar("class.g",  GetEQClassName(mob->GetClass()));
		AddVar("ulevel.g", itoa(mob->GetLevel()));
		AddVar("userid.g", itoa(mob->GetID()));
	}
	if (npcmob)
	{
		AddVar("mname.g",npcmob->GetName());
	}

	if (fac) {
		AddVar("faction.g", itoa(fac));
	}

	if (zone) {
// SCORPIOUS2K- added variable zoneid
		AddVar("zoneid.g",itoa(zone->GetZoneID()));
		AddVar("zoneln.g",zone->GetLongName());
		AddVar("zonesn.g",zone->GetShortName());
	}

	string temp;
#if Parser_DEBUG>10
		printf("Data: %s,Id: %i\n",data,npcid);
#endif
	switch (event) {
		case EVENT_SAY: {
			MakeVars(data, npcid);
			npcmob->FaceTarget(mob);
			SendCommands("event_say", qstID, npcmob, mob);
			break;
		}
		case EVENT_TIMER: {
			SendCommands("event_timer", qstID, npcmob, mob);
			break;
		}
		case EVENT_DEATH: {
			SendCommands("event_death", qstID, npcmob, mob);
			break;
		}
		case EVENT_ITEM: {
			npcmob->FaceTarget(mob);
			SendCommands("event_item", qstID, npcmob, mob);
			break;
		}
		case EVENT_SPAWN: {
			SendCommands("event_spawn", qstID, npcmob, mob);
			break;
		}
		case EVENT_ATTACK: {
			SendCommands("event_attack", qstID, npcmob, mob);
			break;
		}
		case EVENT_SLAY: {
			SendCommands("event_slay", qstID, npcmob, mob);
			break;
		}
		case EVENT_WAYPOINT: {
			temp = "wp." + (string)itoa(npcid);
			AddVar(temp,data);
			SendCommands("event_waypoint", qstID, npcmob, mob);
			break;
		}
		case EVENT_SIGNAL: {
			SendCommands("event_signal", qstID, npcmob, mob);
			break;
		}
		default: {
			// should we do anything here?
			break;
		}
	}
	DelChatAndItemVars(npcid);

}
	
Parser::Parser() : DEFAULT_QUEST_PREFIX("default") {
	MainList.clear();
	pMaxNPCID = database.GetMaxNPCType();
	/*pNPCqstID = new sint32[pMaxNPCID+1];
	for (int32 i=0; i<pMaxNPCID+1; i++)
		pNPCqstID[i] = -1;*/
	pNPCqstID = new sint32[1];
	npcarrayindex=1;
}

Parser::~Parser() {
	MainList.clear();
	varlist.clear();
	AliasList.clear();
	safe_delete_array(pNPCqstID);
	TimerList.clear();
}

bool Parser::LoadAttempted(int32 iNPCID) {
	if (iNPCID > pMaxNPCID)
		return false;

	return (bool) (FindNPCQuestID(iNPCID) != 0);
}
int32 Parser::FindNPCQuestID(sint32 npcid){
	for(int32 i=0;i<npcarrayindex;i++)
		if(pNPCqstID[i]==npcid)
			return i;
	return 0;
}
int32 Parser::AddNPCQuestID(int32 npcid){
	sint32* newpNPCqstID= new sint32[npcarrayindex+2];
	for(int32 i=0;i<npcarrayindex;i++)
		newpNPCqstID[i]=pNPCqstID[i];
	newpNPCqstID[npcarrayindex]=npcid;
	npcarrayindex++;
	safe_delete_array(pNPCqstID);
/*	pNPCqstID = new sint32[npcarrayindex+1];
	for(int32 j=0;j<npcarrayindex;j++)
		pNPCqstID[j]=newpNPCqstID[j];
	safe_delete_array(newpNPCqstID);*/
	pNPCqstID = newpNPCqstID;
	return npcarrayindex-1;
}
bool Parser::SetNPCqstID(int32 iNPCID, sint32 iValue) {
	if (iNPCID > pMaxNPCID)
		return false;
	int32 idx = FindNPCQuestID(iNPCID);
	if(idx)
		pNPCqstID[idx] = iValue;
	else{
		idx=AddNPCQuestID(iNPCID);
		pNPCqstID[idx] = iValue;
	}
	return true;
}

sint32 Parser::GetNPCqstID(int32 iNPCID) {
	if (iNPCID > pMaxNPCID || iNPCID == 0)
		return -1;
	if(int32 idx=FindNPCQuestID(iNPCID))
		return pNPCqstID[idx];
	else
		return -1;
}

void Parser::ClearCache() {
#if Parser_DEBUG >= 2
	cout << "Parser::ClearCache" << endl;
#endif
	//for (int32 i=0; i<pMaxNPCID+1; i++)
	//	pNPCqstID[i] = -1;
	MainList.clear();
	safe_delete_array(pNPCqstID);
	pNPCqstID = new sint32[1];
	npcarrayindex=1;
}

void Parser::SendCommands(const char * event, int32 npcid, Mob* npcmob, Mob* mob) {
	iter_events listIt = MainList.begin();
	Events *p;
	EventList *pp;
	while (listIt != MainList.end())
	{
		p = *listIt;
		iter_eventlist listIt2 = p->Event.begin();
		if ( p->npcid == npcid ) {
			while (listIt2 != p->Event.end())
			{
				pp = *listIt2;
				if (pp && !strcmp(strlwr(pp->event.c_str()),strlwr(event))) {
#if Parser_DEBUG>10
						printf("PP command: %s\n",pp->command.c_str());
#endif
						ParseCommands(pp->command,0,0,npcid,npcmob,mob);
				}
				listIt2++;
			}
		}
		listIt++;
	}
}

void Parser::scanformat(char *string, const char *format, char arg[10][1024])
{
	int increment_arglist = 0;
	int argnum = 0;
	int i = 0;
	char lookfor = 0;

	// someone forgot to set string or format
	if(!format)
		return;
	if(!string)
		return;

	for(;;)
	{
		// increment while they're the same (and not NULL)
		while(*format && *string && *format == *string) {
			format++;
			string++;
		}

		// format string is gone
		if(!format)
			break;
		// string is gone while the format string is still there (ERRor)
		if(!string)
			return;

		// the format string HAS to be equal to ÿ or else things are messed up
		if(*format != 'ÿ')
			return;

		format++;
		lookfor = *format;  // copy until we find char after 'y'
		format++;

		if(!lookfor)
			break;

		// start on a new arg
		if(increment_arglist) {
			arg[argnum][i] = 0;
			argnum++;
		}

		increment_arglist = 1; // we found the first crazy y
		i = 0;  // reset index

		while(*string && *string != lookfor)
			arg[argnum][i++] = *string++;
		string++;
	}

	// final part of the string
	if(increment_arglist) {
		arg[argnum][i] = 0;
		argnum++;
		i = 0;
	}

	while(*string)
		arg[argnum][i++] = *string++;

	arg[argnum][i] = 0;
}

void Parser::ClearEventsByNPCID(int32 iNPCID) {
	list<Events*>::iterator iterator = MainList.begin();
	Events* p;
	while (iterator != MainList.end())
	{
		p = *iterator;
		if (p->npcid == iNPCID) {
			p->Event.clear();
			return;
		}
		iterator++;
	}
}

void Parser::ClearAliasesByNPCID(int32 iNPCID) {
/*	MyListItem<Alias>* Ptr = AliasList.First;
	MyListItem<Alias>* next = 0;
	while (Ptr) {
		next = Ptr->Next;
		if ( (int32)Ptr->Data->npcid == iNPCID) {
			AliasList.DeleteItemAndData(Ptr);
		}
		Ptr = next;
	}*/
}

void Parser::ExCommands(string o_command, string parms, int argnums, int32 npcid, Mob* other, Mob* mob )
{
	char arglist[10][1024];
	//Work out the argument list, if there needs to be one
#if Parser_DEBUG>10
		printf("Parms: %s\n", parms.c_str());
#endif
	if (argnums > 1) {
		string buffer;
		string::iterator iterator = parms.begin();
		int quote=0,alist=0,ignore=0;
		while (1) {
			if (*iterator == '"') {
				if (quote) quote--;
				else	  { quote++; if (buffer.empty()) ignore=1; }
			}
			if (((*iterator != '"' && *iterator != ',' && *iterator != ' ' && *iterator != ')') || quote) && !ignore) {
				buffer+=*iterator;
			}
			if (ignore && *iterator == '"')ignore--;
			if ((*iterator == ',' && !quote)) {
				strcpy(arglist[alist],buffer.c_str());
				alist++;
				buffer="";
			}
			if (iterator == parms.end())
			{
				strcpy(arglist[alist],buffer.c_str());
				alist++;
				break;
			}
			iterator++;
		}
	}
	else {
		string::iterator iterator = parms.begin();
		if (*iterator == '"')
		{
			int quote=0,ignore=0;	//once=0
			string tmp;
			while (iterator != parms.end())
			{
				if (*iterator == '"')
				{
					if (quote)quote--;
					else quote++;
					ignore++;
				}

				if (!ignore  && (quote || (*iterator != '"')))
					tmp+=*iterator;

				else if (*iterator == '"' && ignore)
					ignore--;
				iterator++;
			}
			strcpy(arglist[0],tmp.c_str());
		}
		else
			strcpy(arglist[0],parms.c_str());
	}
#if Parser_DEBUG>10
		printf("After: %s\n", arglist[0]);
#endif
	
	char command[256];
	strncpy(command, o_command.c_str(), 255);
	command[255] = 0;
	char *cptr = command;
	while(*cptr != '\0') {
		if(*cptr >= 'A' && *cptr <= 'Z')
			*cptr = *cptr + ('a' - 'A');
		 *cptr++;
	}
	
	if (!strcmp(command,"write")) {
				char temp[1024];
				memset(temp,0x0,1024);
				snprintf(temp, sizeof(temp), "%s\n", arglist[1]);
					FILE * pFile;
					pFile = fopen (arglist[0], "a");
					fwrite (temp , 1 , strlen(temp) , pFile);
					fclose (pFile);
			}
			else if (!strcmp(command,"me")) {
// MYRA - fixed comma bug for me command
				if (mob && mob->IsClient()) entity_list.MessageClose(mob->CastToClient(), false, 200, 10, "%s", parms.c_str());
//end Myra
			}
			else if (!strcmp(command,"spawn") || !strcmp(command,"spawn2")) 
			{

				//char tempa[100];
				const NPCType* tmp = 0;
				int16 grid = atoi(arglist[1]);
				//int8 guildwarset = atoi(arglist[2]);
				float hdng;
				if ((tmp = database.GetNPCType(atoi(arglist[0])))) 
				{

					if (!strcmp(command,"spawn")) 
					{
						hdng=mob->CastToClient()->GetHeading();
					}
					else
					{
						hdng=atof(arglist[6]);
					}
					NPC* npc = new NPC(tmp, 0, atof(arglist[3]), atof(arglist[4]), atof(arglist[5]), hdng);


					npc->AddLootTable();
					entity_list.AddNPC(npc,true,true);
					// Quag: Sleep in main thread? ICK!
					// Sleep(200);
					// Quag: check is irrelevent, it's impossible for npc to be 0 here
					// (we're in main thread, nothing else can possibly modify it)
//					if(npc != 0) {
						if(grid > 0)
						{
							npc->AssignWaypoints(grid);
						}
						npc->SendPosUpdate();
//					}
				}
			}
			else if (!strcmp(command,"echo")) {
// MYRA - fixed comma bug for echo command
				printf("%s\n", parms.c_str());
//end Myra
			}
			else if (!strcmp(command,"summonitem")) {
				if (mob) mob->CastToClient()->SummonItem(atoi(arglist[0]));
			}
			else if (!strcmp(command,"setstat")) {
				if (mob) 
					mob->CastToClient()->SetStats(atoi(arglist[0]),atoi(arglist[1]));
			}
			else if (!strcmp(command,"castspell")) {
				if (other) other->SpellFinished(atoi(arglist[1]), atoi(arglist[0]));
			}
			else if (!strcmp(command,"selfcast")) {
				if (mob) mob->SpellFinished(atoi(arglist[0]), mob->GetID(),10,0);
			}
			else if (!strcmp(command,"addloot")) {//Cofruben: add an item to the mob.
				if (other && atoi(arglist[0])>0 && other->IsNPC())
					other->CastToNPC()->AddItem(atoi(arglist[0]),atoi(arglist[1]));
			}
			else if (!strcmp(command,"zone")) {
				if (mob && mob->IsClient())
				{
							ServerPacket* pack = new ServerPacket(ServerOP_ZoneToZoneRequest, sizeof(ZoneToZone_Struct));
							ZoneToZone_Struct* ztz = (ZoneToZone_Struct*) pack->pBuffer;
							ztz->response = 0;
							ztz->current_zone_id = zone->GetZoneID();
							ztz->requested_zone_id = database.GetZoneID(arglist[0]);
							ztz->admin = mob->CastToClient()->Admin();
							strcpy(ztz->name, mob->GetName());
							ztz->guild_id = mob->CastToClient()->GuildDBID();
							ztz->ignorerestrictions = 3;
							worldserver.SendPacket(pack);
							safe_delete(pack);				
				}
			}
			else if (!strcmp(command,"settimer"))
			{
				list<timers*>::iterator iterator = TimerList.begin();
				timers*p=0;
				while (iterator != TimerList.end())
				{
					p=*iterator;
//					if (p) printf("%s - %s\n",p->name.c_str(), arglist[0]);
					if (p && !strcmp(arglist[0],p->name.c_str()))
					{
						p->mob = other;
						p->Timer_->Enable();
						p->Timer_->Start(atoi(arglist[1]) * 1000,false);
						printf("Reseting: %s for %d seconds\n", p->name.c_str(), atoi(arglist[1]));
						break;
					}
					iterator++;
					p=0;
				}
				if (!p)
				{
				timers * tmp = new timers;
				tmp->mob = other;
				tmp->Timer_ = new Timer(atoi(arglist[1]) * 1000,0);
				tmp->Timer_->Start(atoi(arglist[1]) * 1000,false);
				tmp->name = arglist[0];
				printf("Adding: %s for %d seconds\n", tmp->name.c_str(), atoi(arglist[1]));
				TimerList.push_back(tmp);
			}
			}

			else if (!strcmp(command,"say")) {
				if(other)
				{
					other->Say(parms.c_str());
				}
			}
			else if (!strcmp(command,"stoptimer")) {
				list<timers*>::iterator iterator = TimerList.begin();
				timers*p=0;
				while (iterator != TimerList.end())
				{
					p=*iterator;
					if (p) printf("%s - %s\n",p->name.c_str(), arglist[0]);
					if (p && !strcmp(arglist[0],p->name.c_str()))
					{
						p->Timer_->Disable();
					}
					iterator++;
				}
			}
			else if (!strcmp(command,"emote")) {
				if(other)
				{
					other->Emote(parms.c_str());
				}
			}
			else if (!strcmp(command,"shout2")) {
				if (other){
					worldserver.SendEmoteMessage(0,0,0,13, "%s shouts, '%s'", other->GetCleanName(), parms.c_str());
				}
			}
			else if (!strcmp(command,"shout")) {
				if(other)
				{
					other->Shout(parms.c_str());
				}
			}
			else if (!strcmp(command,"depop")) {
				if (atoi(arglist[0]) != 0){
					Mob* tmp = entity_list.GetMobByNpcTypeID(atoi(arglist[0]));
					if (tmp)
						tmp->CastToNPC()->Depop();
				}
				else
					if (other) other->CastToNPC()->Depop();
			}
			else if (!strcmp(command,"settarget")) {
				Mob* tmp=0;
				if (!strcmp(strlwr(arglist[0]),"npctype")) {
					tmp = entity_list.GetMobByNpcTypeID(atoi(arglist[1]));
				}
				else if (!strcmp(strlwr(arglist[0]),"entity")) {
					tmp = entity_list.GetMob(atoi(arglist[1]));
				}
				if (tmp) {
					if (other) other->SetTarget(tmp);
				}
			}
			else if (!strcmp(command,"follow"))  {
				if (other) other->SetFollowID(atoi(arglist[0]));
			}
			else if (!strcmp(command,"sfollow"))  {
				if (other) other->SetFollowID(0);
			}
			else if (!strcmp(command,"cumflag")) {
				if (other) other->flag[50] = other->flag[50] + 1;
			}
			else if (!strcmp(command,"flagnpc")) {
				int32 tmpFlagNum = atoi(arglist[0]);
				if (tmpFlagNum >= (sizeof(other->flag) / sizeof(other->flag[0])))
					if (other) other->flag[tmpFlagNum] = atoi(arglist[1]);
				else {
					// Quag: TODO: Script error here, handle it somehow?
				}
			}
        		else if (!strcmp(command,"changedeity")) { //Cofruben:-Changes the deity.
          			  if (mob && mob->IsClient())
					  {
              			 		  mob->CastToClient()->SetDeity(atoi(arglist[0]));
			            		  mob->CastToClient()->Message(15,"Your Deity has been changed/set to: %i",arglist[0]);
						  mob->CastToClient()->Save(1);
						  mob->CastToClient()->Kick();
					  }
					  else
						mob->CastToClient()->Message(15,"Error changing Deity");
         		}
			else if (!strcmp(command,"flagmob->CastToClient()")) {
				if (mob && mob->IsClient())
					mob->CastToClient()->flag[atoi(arglist[0])] = atoi(arglist[1]);
			}
			else if (!strcmp(command,"exp")) {
				if (mob && mob->IsClient())
					mob->CastToClient()->AddEXP(atoi(arglist[0]));
			}
			else if (!strcmp(command,"level")) {
				if (mob && mob->IsClient())
					mob->CastToClient()->SetLevel(atoi(arglist[0]), true);
			}
			else if (!strcmp(command,"traindisc")) {
				if (mob && mob->IsClient())
					mob->CastToClient()->TrainDiscipline(atoi(arglist[0]));
			}
			else if (!strcmp(command,"safemove")) {
				if (mob && mob->IsClient())
					mob->CastToClient()->MovePC(zone->GetShortName(),database.GetSafePoint(zone->GetShortName(),"x"),database.GetSafePoint(zone->GetShortName(),"y"),database.GetSafePoint(zone->GetShortName(),"z"));
			}
			else if (!strcmp(command,"rain")) {
				zone->zone_weather = atoi(arglist[0]);
				APPLAYER* outapp = new APPLAYER(OP_Weather, 8);
				*((int32*) &outapp->pBuffer[4]) = (int32) atoi(arglist[0]); // Why not just use 0x01/2/3?
				entity_list.QueueClients(other, outapp);
				safe_delete(outapp);
			}
			else if (!strcmp(command,"snow")) {
				zone->zone_weather = atoi(arglist[0]) + 1;
				APPLAYER* outapp = new APPLAYER(OP_Weather, 8);
				outapp->pBuffer[0] = 0x01;
				*((int32*) &outapp->pBuffer[4]) = (int32)atoi(arglist[0]);
				entity_list.QueueClients(mob->CastToClient(), outapp);
				safe_delete(outapp);
			}
			else if (!strcmp(command,"surname")) { //Cofruben:-Changes the last name. 
			if (mob && mob->IsClient()) 
			{ 
					mob->CastToClient()->ChangeLastName(arglist[0]); 
					mob->CastToClient()->Message(15,"Your surname has been changed/set to: %s",arglist[0]); 
			} 
			 else 
				mob->CastToClient()->Message(15,"Error changing/setting surname"); 
			} 
			 else if (!strcmp(command,"permaclass")) {//Cofruben:-Makes the client the class specified 
				mob->CastToClient()->SetBaseClass(atoi(arglist[0])); 
				mob->CastToClient()->Save(2); 
				mob->CastToClient()->Kick(); 
			} 
			 else if (!strcmp(command,"permarace")) {//Cofruben:-Makes the client the race specified 
				mob->CastToClient()->SetBaseRace(atoi(arglist[0])); 
				mob->CastToClient()->Save(2); 
				mob->CastToClient()->Kick(); 
			 } 
			 else if (!strcmp(command,"permagender")) {//Cofruben:-Makes the client the gender specified 
				mob->CastToClient()->SetBaseGender(atoi(arglist[0])); 
				mob->CastToClient()->Save(2); 
				mob->CastToClient()->Kick(); 
			 } 
			 else if (!strcmp(command,"scribespells")) {//Cofruben:-Scribe spells for user up to his actual level. 
				int book_slot; 
				int16 curspell; 
				for(curspell = 0, book_slot = 0; curspell < SPDAT_RECORDS && book_slot < MAX_PP_SPELLBOOK; curspell++) 
				{ 
				   if 
				   ( 
					  spells[curspell].classes[WARRIOR] != 0 && 
					  spells[curspell].classes[mob->CastToClient()->GetPP().class_-1] <= mob->CastToClient()->GetLevel() && 
					  spells[curspell].skill != 52 
				   ) 
				   { 
					  mob->CastToClient()->ScribeSpell(curspell, book_slot++); 
				   } 
				} 
			 } 

			else if (!strcmp(command,"givecash")) { 
				APPLAYER* outapp = new APPLAYER(OP_MoneyOnCorpse, sizeof(moneyOnCorpseStruct)); 
				moneyOnCorpseStruct* d = (moneyOnCorpseStruct*) outapp->pBuffer; 
				d->response      = 1; 
				d->unknown1      = 0x5a; 
				d->unknown2      = 0x40; 
				d->unknown3      = 0; 
				if (mob && mob->IsClient())
				{
				d->copper      = atoi(arglist[0]); 
				d->silver      = atoi(arglist[1]); 
				d->gold         = atoi(arglist[2]); 
				d->platinum      = atoi(arglist[3]); 
				mob->CastToClient()->AddMoneyToPP(d->copper, d->silver, d->gold, d->platinum,true); 
				mob->CastToClient()->QueuePacket(outapp);
				string tmp;
				if (d->platinum>0) tmp = "You receive "+(string)itoa(d->platinum)+" plat"; 
				if (d->gold>0)
					if (tmp.length()==0)
						tmp = "You receive "+(string)itoa(d->gold)+" gold";
					else
						tmp = tmp + ","+(string)itoa(d->gold)+" gold";
				if(d->silver>0) 
					if (tmp.length()==0)
						tmp = "You receive "+(string)itoa(d->silver)+" silver";
					else
						tmp = tmp + ","+(string)itoa(d->silver)+" silver";
				if(d->copper>0)
					if (tmp.length()==0)
						tmp = "You receive "+(string)itoa(d->copper)+" copper";
					else
						tmp = tmp + ","+(string)itoa(d->copper)+" copper";
				tmp = tmp + " pieces.";
				if (mob) 
					mob->CastToClient()->Message(MT_OOC,tmp.c_str()); 
				safe_delete(outapp); 
				}
			}
			else if (!strcmp(command,"pvp")) {
				if (!strcmp(strlwr(arglist[0]),"on"))
					if (mob) mob->CastToClient()->SetPVP(true);
				else

					if (mob) mob->CastToClient()->SetPVP(false);
			}
			else if (!strcmp(command,"movepc")) { 
				if (mob && mob->IsClient()) 
					 mob->CastToClient()->MovePC((atoi(arglist[0])),(atof(arglist[1])),(atof(arglist[2])),(atof(arglist[3]))); 
			}
			else if (!strcmp(command,"gmmove")) { 
				if (mob && mob->IsClient()) 
					 mob->CastToClient()->GMMove( atof(arglist[0]), atof(arglist[1]), atof(arglist[2])); 
			}
			else if (!strcmp(command,"movegrp")) {
			#ifdef IPC
                if (mob && mob->IsClient()|| (mob->IsNPC() && mob->CastToNPC()->IsInteractive()) ){
			#else
			    if (mob && mob->IsClient()) {
            #endif
                   	if (entity_list.GetGroupByMob(mob) != 0){
						entity_list.GetGroupByMob(mob)->TeleportGroup(mob, atoi(arglist[0]),atof(arglist[1]),atof(arglist[2]),atof(arglist[3]));
					}
					else {
						if (mob) mob->CastToClient()->MovePC((atoi(arglist[0])),(atof(arglist[1])),(atof(arglist[2])),(atof(arglist[3])));
					}
				}
			}
			else if (!strcmp(command,"doanim")) {
				if (other) other->DoAnim(atoi(arglist[0]));
			}
			else if (!strcmp(command,"addskill")) {
				if (mob && mob->IsClient()) mob->CastToClient()->AddSkill(atoi(arglist[0]), atoi(arglist[1]));
			}
			else if (!strcmp(command,"setlanguage")) { // bUsh
				if (mob && mob->IsClient()) mob->CastToClient()->SetLanguageSkill(atoi(arglist[0]), atoi(arglist[1]));
			}
			else if (!strcmp(command,"setskill")) { // bUsh
				if (mob && mob->IsClient()) mob->CastToClient()->SetSkill(atoi(arglist[0]), atoi(arglist[1]));
			}
			else if (!strcmp(command,"setallskill")) { // khuong
				int8 skill_id = atoi(arglist[0]);
				if (mob && mob->IsClient()) { 
				for(int skill_num=0;skill_num<74;skill_num++){
				if (mob) mob->CastToClient()->SetSkill(skill_num, skill_id);
					}
				}
			else if (!strcmp(command,"attack")) { // not khuong
				Client* getclient = entity_list.GetClientByName(arglist[0]);
				if(getclient && other && other->IsNPC() && other->IsAttackAllowed(getclient))
				{
					if (other) other->CastToNPC()->AddToHateList(getclient,1);
				}
				else
				{
					if(other) other->Say("I am unable to attack %s.", arglist[0]);
				}
			}
			}
			else if (!strcmp(command,"save")) { // khuong
				if (mob && mob->IsClient()) mob->CastToClient()->Save();
			}
			else if (!strcmp(command,"flagcheck")) {
				int32 tmpFlagNum1 = atoi(arglist[0]);
				int32 tmpFlagNum2 = atoi(arglist[1]);
				if (tmpFlagNum1 >= (sizeof(other->flag) / sizeof(other->flag[0])) || tmpFlagNum2 >= (sizeof(other->flag) / sizeof(other->flag[0]))) {
					if (mob && mob->CastToClient()->flag[tmpFlagNum1] != 0)
						mob->CastToClient()->flag[tmpFlagNum2] = 0;
				}
				else {
					// Quag: TODO: Script error here, handle it somehow?
				}
				// Quag: Orignal code, not sure how this is supposed to work
//				if (mob->CastToClient()->flag[atoi(arglist[0])] != 0)
//					if (mob) mob->CastToClient()->flag[atoi(arg1)] = 0;
			}
			else if (!strcmp(command,"faction")) {
			    if (mob && mob->IsClient()) {
					sint32 faction_id = atoi(arglist[0]);
					sint32 faction_value = atoi(arglist[1]);
					if(faction_id!=0&&faction_value!=0){
// SCORPIOUS2K - fixed faction command
						if (mob) 
						{
							//Client *p; 
							mob->CastToClient()->SetFactionLevel2(
								mob->CastToClient()->CharacterID(), 
								faction_id, 
								mob->GetClass(), 
								mob->GetRace(), 
								mob->GetDeity(), 
								faction_value); 
						}

						
//						if (mob) mob->CastToClient()->SetFactionLevel2(mob->GetID(), faction_id, mob->GetClass(), mob->GetRace(), mob->GetDeity(), faction_value);
//						// FIXME add a faction message?
					}
			    }
			}
			else if (!strcmp(command,"setsky")) {
				uint8 new_sky = atoi(arglist[0]);
				if (zone) zone->newzone_data.sky = new_sky;
                        	APPLAYER* outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
				memcpy(outapp->pBuffer, &zone->newzone_data, outapp->size);
				entity_list.QueueClients(mob, outapp);
				safe_delete(outapp);
			}
			else if (!strcmp(command,"setguild")) {
				if (mob && mob->IsClient()){
					int32 new_gid = atoi(arglist[0]);
					int8  new_rank = atoi(arglist[1]);
					if (mob) mob->CastToClient()->SetGuild(new_gid,new_rank);
				}
			}
			else if (!strcmp(command,"settime")) {
				int8 new_hour = atoi(arglist[0]);
				int8 new_min  = atoi(arglist[1]);
				if (zone) zone->SetTime(new_hour,new_min);
			}
// MYRA - added itemlink(ItemNumber) command

			else if (!strcmp(command,"itemlink")) { 

				const Item_Struct* item = 0; 
					int16 itemid = atoi(arglist[0]); 
				item = database.GetItem(itemid); 
				mob->CastToClient()->Message(0, "%s tells you, '%c00%i %s%c",other->GetName(),0x12, item->ItemNumber, item->Name, 0x12); 
			}
//end Myra
// SCORPIOUS2K - signal command
			else if (!strcmp(command,"signal")) 
			{	// signal(npcid) - generates EVENT_SIGNAL on specified npc
				int snpc=atoi(arglist[0]);
				if (snpc<1)
				{
					printf("signal() bad npcid=%i\n",snpc);
				}
				else
				{
					//Mob* signalnpc=0;
					entity_list.SignalMobsByNPCID(snpc);
					}
			}
// SCORPIOUS2K - qglobal variable commands
			else if (!strcmp(command,"setglobal")) 
			{	// setglobal(varname,value,options,duration)
				char errbuf[MYSQL_ERRMSG_SIZE];
				char *query = 0;
				MYSQL_RES *result;
				//MYSQL_ROW row;
				int qgZoneid=zone->GetZoneID();
				int qgCharid=0;
				int qgNpcid=npcid;
				int options=atoi(arglist[2]);
				/*	options value determines the availability of global variables to NPCs when a quest begins
				------------------------------------------------------------------
				  value		   npcid	  player		zone
				------------------------------------------------------------------
					0			this		this		this
					1			all			this		this
					2			this		all			this
					3			all			all			this
					4			this		this		all
					5			all			this		all
					6			this		all			all
					7			all			all			all
				*/		
				if (mob && mob->IsClient())  // some events like waypoint and spawn don't have a player involved
				{
					qgCharid=mob->CastToClient()->CharacterID();
				}

				else
				{
					qgCharid=-qgNpcid;		// make char id negative npc id as a fudge
				}
				if (options < 0 || options > 7)
				{
					cerr << "Invalid options for global var " << arglist[1] << " using defaults" << endl;
				}	// default = 0 (only this npcid,player and zone)
				else
				{
					if (options & 1)
						qgNpcid=0;
					if (options & 2)
						qgCharid=0;
					if (options & 4)
						qgZoneid=0;
				}

				// clean up expired vars and get rid of the one we're going to set if there
				database.RunQuery(query, MakeAnyLenString(&query, 
					"DELETE FROM quest_globals WHERE expdate < %i || (name='%s' && (charid=0 || (npcid=%i && charid=%i && zoneid=%i)))"
					,Timer::GetCurrentTime(),arglist[0],qgNpcid,qgCharid,qgZoneid), errbuf, &result);
				if (query)
				{
					safe_delete_array(query);
					query=0;
				}
				mysql_free_result(result);
				if (!database.RunQuery(query, MakeAnyLenString(&query, 
				  "INSERT INTO quest_globals (charid,npcid,zoneid,name,value,expdate) VALUES (%i,%i,%i,'%s','%s',unix_timestamp(now())+%i)",
				  qgCharid,qgNpcid,qgZoneid,arglist[0],arglist[1],QGexpdate(arglist[0],arglist[3])
				  ), errbuf, &result)) 
				{
					cerr << "setglobal error inserting " << arglist[1] << " : " << errbuf << endl;
				}
				if (query)
				{
					safe_delete_array(query);
					query=0;
				}
				mysql_free_result(result);
			}
			else if (!strcmp(command,"targlobal")) 
			{	// targlobal(varname,value,duration,npcid,charid,zoneid)
				char errbuf[MYSQL_ERRMSG_SIZE];
				char *query = 0;
				MYSQL_RES *result;
				//MYSQL_ROW row;
				int qgZoneid=atoi(arglist[5]);
				int qgCharid=atoi(arglist[4]);
				int qgNpcid=atoi(arglist[3]);
				// clean up expired vars and get rid of the one we're going to set if there
				database.RunQuery(query, MakeAnyLenString(&query, 
					"DELETE FROM quest_globals WHERE expdate < %i || (name='%s' && (charid=0 || (npcid=%i && charid=%i && zoneid=%i)))"
					,Timer::GetCurrentTime(),arglist[0],qgNpcid,qgCharid,qgZoneid), errbuf, &result);
				if (query)
				{
					safe_delete_array(query);
					query=0;
				}
				mysql_free_result(result);
				if (!database.RunQuery(query, MakeAnyLenString(&query, 
				  "INSERT INTO quest_globals (charid,npcid,zoneid,name,value,expdate) VALUES (%i,%i,%i,'%s','%s',unix_timestamp(now())+%i)",
				  qgCharid,qgNpcid,qgZoneid,arglist[0],arglist[1],QGexpdate(arglist[0],arglist[2])
				  ), errbuf, &result)) 
				{
					cerr << "targlobal error inserting " << arglist[0] << " : " << errbuf << endl;
				}
				if (query)
				{
					safe_delete_array(query);
					query=0;
				}
				mysql_free_result(result);
			}
			else if (!strcmp(command,"ding")) {//-Cofruben:makes a sound.
				if (mob && mob->IsClient())
					mob->CastToClient()->SendSound();
			}
			else if (!strcmp(command,"delglobal")) 
			{	// delglobal(varname)
				char errbuf[MYSQL_ERRMSG_SIZE];
				char *query = 0;
				MYSQL_RES *result;
				//MYSQL_ROW row;
				int qgZoneid=zone->GetZoneID();
				int qgCharid=0;
				int qgNpcid=npcid;
				if (mob && mob->IsClient())  // some events like waypoint and spawn don't have a player involved
				{
					qgCharid=mob->CastToClient()->CharacterID();
				}

				else
				{
					qgCharid=-qgNpcid;		// make char id negative npc id as a fudge
				}
				if (!database.RunQuery(query, 
				  MakeAnyLenString(&query, "DELETE FROM quest_globals WHERE name='%s' && (npcid=0 || charid=0 || zoneid=0 ||(npcid=%i && charid=%i && zoneid=%i))",
				  arglist[0],qgNpcid,qgCharid,qgZoneid),errbuf, &result)) 
				{
					cerr << "delglobal error deleting " << arglist[0] << " : " << errbuf << endl;
				}
				if (query)
				{
					safe_delete_array(query);
					query=0;
				}
				mysql_free_result(result);
			}
// SCORPIOUS2K - end of qglobal commands
			else if (!strcmp(command,"rebind")) {
				if(mob->IsClient()){
					if (mob) mob->CastToClient()->SetBindPoint( atoi(arglist[0]), atof(arglist[1]), atof(arglist[2]), atof(arglist[3]));
				}
			}
// new quest wandering commands
			else if (!strcmp(command,"stop")) 
			{
				other->StopWandering();
			}
			else if (!strcmp(command,"pause")) 
			{
				other->PauseWandering(atoi(arglist[0]));
			}
			else if (!strcmp(command,"moveto")) 
			{
				other->MoveTo(atof(arglist[0]), atof(arglist[1]), atof(arglist[2]));
			}
			else if (!strcmp(command,"resume")) 
			{
				other->ResumeWandering();
			}
			else if (!strcmp(command,"start")) 
			{
				other->AssignWaypoints(atoi(arglist[0]));
			}
// add ldon points
			else if (!strcmp(command,"addldonpoints")) 
			{
				mob->CastToClient()->UpdateLDoNPoints(atoi(arglist[0]), atoi(arglist[1]));
			}
// hp event 
			else if (!strcmp(command,"setnexthpevent")) 
			{ 
				if (other) other->SetNextHPEvent( atoi(arglist[0]) ); 
			} 
			else if (!strcmp(command,"respawn")) 
			{
				//char tempa[100];
				float x,y,z,h;
				if ( other ) 
				{
					x = other->GetX();
					y = other->GetY();
					z = other->GetZ();
					h = other->GetHeading();
					other->CastToNPC()->Depop();
				}
				else 
				{
					return;
				}
				const NPCType* tmp = 0;
				int16 grid = atoi(arglist[1]);
				//int8 guildwarset = atoi(arglist[2]);
				if ((tmp = database.GetNPCType(atoi(arglist[0])))) 
				{
					NPC* npc = new NPC(tmp, 0, x, y, z, h);
					npc->AddLootTable();
					entity_list.AddNPC(npc,true,true);
						if(grid > 0)
							npc->AssignWaypoints(grid);

						npc->SendPosUpdate();
				}
			}
			else
				printf("\nUnknown perl function used:%s",command);


}


// SCORPIOUS2K - convert duration value to expdate
int32 Parser::QGexpdate(char * name, char * options)
{
	// format:	Y#### or D## or H## or M## or S## or T###### or C#######

	int32 tval=strlen(options);

	if (tval < 2 || (tval>1 && !isdigit(options[1])))
	{
		cerr << "Invalid duration for " << name << " using default" << endl;
		tval=1000000;		// default=1 day
	}
	else
	{
		tval=atoi(&options[1]);
/*
		if (toupper(options[0])=='Y')
		{	// years
			if (tval>50)
			{
				tval=50;
			}
			tval=tval*10000000000;
		}
		else */if (toupper(options[0])=='D')
		{	// days

			if (tval>30)
			{
				tval=30;
			}
			tval=tval*1000000;
		}
		else if (toupper(options[0])=='H')
		{	// hours
			if (tval>23)
			{
				tval=23;
			}
			tval=tval*10000;
		}
		else if (toupper(options[0])=='M')
		{	// minutes
			if (tval>59)
			{
				tval=59;
			}
			tval=tval*100;
		}
		else if (toupper(options[0])=='S')
		{	// seconds
			if (tval>59)
			{
				tval=59;
			}
		}		
		else if (toupper(options[0])=='T')
		{	// time as hhmmss
			if (tval>235959)
			{
				tval=235959;
			}
		}		
		else if (toupper(options[0])!='C')
		{	// calender time as YYYMMDD
			cerr << "Invalid duration for " << name << " using default" << endl;
			tval=1000000;		// default=1 day
		}
	}

	return tval;
}

int Parser::LoadScript(int npcid, const char * zone, Mob* activater)
{
	SetNPCqstID(npcid, npcid);
	ClearEventsByNPCID(npcid);
	ClearAliasesByNPCID(npcid);

	//string strnpcid = ("default");
	string strnpcid = (DEFAULT_QUEST_PREFIX);

	if (npcid)
		strnpcid = itoa(npcid);
	string filename;
	filename = QUEST_DIR "/" + (string)zone + "/" + (string)strnpcid + ".qst";
	string line,buffer,temp;
	ifstream file( filename.c_str() );
	if (!file)
	{
		if (npcid) {
			SetNPCqstID(npcid, 0);
			LoadScript(0, zone);
		}
		else
			SetNPCqstID(0, -1);
	}

	int quote=0,ignore=0,bracket=0,line_num=0,paren=0;
	EventList* event1 = new EventList;
	Events * NewEventList = new Events;
	while (file && !file.eof())
	{
		getline(file,line);
		string::iterator iterator = line.begin();
		while (*iterator)
		{
			if (iterator[0] == '/' && iterator[1] == '/') break;
			if (!ignore && *iterator == '/' && iterator[1] == '*') { ignore++; iterator++; iterator++; }
			if (*iterator == '*' && iterator[1] == '/') { ignore--; iterator++; iterator++; }
			if (!ignore && (strchr(charIn,*iterator) || quote || paren))
				buffer+=*iterator;
			if (!ignore)
			{
			if (*iterator == '{')
			{
				bracket++;
				if (bracket == 1)
				{
					event1 = new EventList;
					NewEventList->npcid = npcid;
					buffer.replace(buffer.length()-1,buffer.length(),"");
					event1->event = buffer;
					buffer="";
				}
			}
			if (*iterator == '}')
			{
				bracket--;
				if (bracket == 0)
				{
				buffer.replace(buffer.length()-1,buffer.length(),"");
				int heh = ParseCommands(buffer,line_num,0,0,0,0,filename);
				if (!heh) return 0;
				event1->command = buffer;
				buffer="";
				NewEventList->Event.push_back(event1);
				}
				if (bracket==-1)
				{	
					if(activater && activater->IsClient())
					activater->CastToClient()->Message(10,"Line: %d,File: %s | error C0006: syntax error : too many ')'s",line_num,filename.c_str());
					return 0;
				}
			}
			if (*iterator == '"')if(quote)quote--;else quote++;
			if (*iterator == '(')paren++;
			if (*iterator == ')')paren--;
			}
			iterator++;
		}
		line_num++;

	}
	MainList.push_back(NewEventList);
	return 1;
}


void Parser::Replace(string& string1, string repstr, string rep, int all) {
	while (string1.find(repstr.c_str()) != string::npos) {
		string1.replace(string1.find(repstr.c_str()),repstr.length(),rep.c_str());
		if (!all)
			break;
	}
}

string Parser::GetVar(string varname, int32 npcid)
{
	list<vars*>::iterator iterator = varlist.begin();
	vars * p;
	string checkfirst;
	string checksecond;
	checkfirst = varname + (string)"." + (string)itoa(npcid);
	checksecond = varname + (string)".g";

	while(iterator != varlist.end())
	{
		p = *iterator;
		if (!strcasecmp(p->name.c_str(), checkfirst.c_str()) || !strcasecmp(p->name.c_str(),checksecond.c_str()))
		{
printf("GetVar(%s) = '%s'\n", varname.c_str(), p->value.c_str());
			return p->value;
		}
		iterator++;
	}
	checkfirst="";
	checkfirst = "NULL";
	return checkfirst;
}

void Parser::DeleteVar(string name)
{
	list<vars*>::iterator iterator = varlist.begin();
	vars* p;
	while(iterator != varlist.end())
	{
		p = *iterator;
		if (!p->name.compare(name))
		{
			varlist.erase(iterator);
			return;
		}
		iterator++;
	}
}

void Parser::DelChatAndItemVars(int32 npcid)
{
//	MyListItem <vars> * Ptr;
	string temp;
	int i=0;
	for (i=0;i<10;i++)
	{
		temp = (string)itoa(i) + "." + (string)itoa(npcid);
		DeleteVar(temp);
		temp = (string)itoa(i) + "-." + (string)itoa(npcid);
		DeleteVar(temp);
	}
	for (i=1;i<5;i++)
	{
		temp = "item"+(string)itoa(i) + "." + (string)itoa(npcid);
		DeleteVar(temp);
		temp = "item"+(string)itoa(i) + ".stack." + (string)itoa(npcid);
		DeleteVar(temp);
	}
}

void Parser::AddVar(string varname, string varval)
{
	list<vars*>::iterator iterator = varlist.begin();
	vars* p;
	while(iterator != varlist.end())
	{
		p  = *iterator;
		if (!p->name.compare(varname))
		{
			p->value="";
			p->value = varval;
			return;
		}
		iterator++;
	}
	vars * newvar = new vars;
	newvar->name = varname;
	newvar->value = varval;
	varlist.push_back(newvar);
}

void Parser::HandleVars(string varname, string varparms, string& origstring, string format, int32 npcid, Mob* mob)
{
	string tempvar;
	tempvar = GetVar(varname,npcid);
	char arglist[10][1024];
	string::iterator iterator = varparms.begin();
	string buffer;
	int quote=0;
	int alist=0;
	while (*iterator)
	{
		if (*iterator != '"' && *iterator != ',' && *iterator != ' ' || (quote && *iterator != '"'))
			buffer+=*iterator;
		if (*iterator == '"')
		{
			if (quote)quote--;
			else quote++;
		}
		if (*iterator == ',' && !quote)
		{
			strcpy(arglist[alist],buffer.c_str());
			alist++;
			buffer="";
		}
		iterator++;
	}
	strcpy(arglist[alist],buffer.c_str());
	if (!strcmp(strlwr((const char*)varname.c_str()),"mid")) {
		int pos=0;
		int one = atoi(arglist[1]);
		int two = atoi(arglist[2]);
		string buffer2;
		string find = arglist[0];
		string::iterator iterator = find.begin();
		while (*iterator)
		{
			pos++;
			if (pos>=one)
				buffer2+=*iterator;
			if (pos==two)
				break;
			iterator++;
		}
		Replace(origstring,format,buffer2.c_str());
	}
	else if (!strcmp(strlwr((const char*)varname.c_str()),"+")) {
		string temp;
		temp = (string)" "+format+(string)" ";
		Replace(origstring, temp, " REPLACETHISSHIT ",1);
	}
	else if (!strcmp(strlwr((const char*)varname.c_str()),"replace")) {
		string temp;
		temp = (string)arglist[0];
		Replace(temp, arglist[1], arglist[2],1);
		Replace(origstring, format, temp);
	}
	else if (!strcmp(strlwr(varname.c_str()),"itemcount")) {
		string temp;
		int o=0;
		o = GetItemCount(varparms,npcid);
		temp = (string)itoa(o);
		Replace(origstring,format,temp,1);
	}
	else if (!strcmp(strlwr(varname.c_str()),"calc")) {
		Replace(origstring,format,itoa(pcalc(varparms.c_str())));
	}
	else if (!strcmp(strlwr(varname.c_str()),"status") && mob &&  mob->IsClient()) {
		Replace(origstring,format,itoa(mob->CastToClient()->Admin()));
	}
	else if (!strcmp(strlwr(varname.c_str()),"hasitem") && mob && mob->IsClient()) {
		int has=0;
		for (int i=0; i<=30;i++) {
			if (mob->CastToClient()->GetItemIDAt(i) == (uint32) atoi(varparms.c_str())) {
				Replace(origstring,format,"true");
				has = 1;
				break;
			}
		}
		if (!has)
			Replace(origstring,format,"false");
	}
	else if (!strcmp(strlwr((const char*)varname.c_str()),"read")) {
		ifstream file(arglist[0]);
		if (file)
		{
			string line;
			int index=0,stop=atoi(arglist[1]);
			while (!file.eof())
			{
				getline(file,line);
				if (index == stop)break;
				index++;
			}
			Replace(origstring,format,line);
		}
	}
	else if (!strcmp(strlwr((const char*)varname.c_str()),"npc_status")) {
		Mob * tmp;
		if (!atoi(varparms.c_str())) {
			tmp = entity_list.GetMob(varparms.c_str());
		}
		else {
			tmp = entity_list.GetMobByNpcTypeID(atoi(varparms.c_str()));
		}
		if (tmp && tmp->GetHP() > 0) Replace(origstring,format,"up");
		else Replace(origstring,format,"down");
	}
	else if (!strcmp(strlwr((const char*)varname.c_str()),"strlen")) {
		Replace(origstring,format,itoa(varparms.length()-1));
	}
	else if (!strcmp(strlwr((const char*)varname.c_str()),"chr")) {
		char temp[4];
		memset(temp, 0x0, 4);
		temp[0] = atoi(varparms.c_str());
		Replace(origstring,format,temp);
	}
	//used_pawn - random implementation. 
	else if (!strcmp(strlwr((const char*)varname.c_str()),"random")) { 
		Replace(origstring,format,itoa(rand()%varparms[0])); 
	}  
   else if (!strcmp(strlwr((const char*)varname.c_str()),"asc")) { 
      Replace(origstring,format,itoa(varparms[0])); 
   } 
	else if (!strcmp(strlwr((const char*)varname.c_str()),"gettok")) {
		Replace(origstring,format,gettok(arglist[0],arglist[1],atoi(arglist[2])));
	}
	else {
		Replace(origstring,format,tempvar);
	}
	gClient = 0;
}

void Parser::ParseVars(string& text, int32 npcid, Mob* mob)
{
	if (text.find("$") == string::npos && text.find("%") == string::npos)
		return;
	string buffer2;
	string fname;
	string parms;
	while (text.find("%") != string::npos)
	{
		string temp;
		temp = (string)text.substr(text.find("%")).c_str();
		string::iterator iterator = temp.begin();
		string buffer;
		while (*iterator)
		{
			if (!strrchr(notin,*iterator))
				buffer+=*iterator;
			else
			{
				HandleVars(buffer,0,text,buffer,npcid,mob);
				Replace(text,buffer,"testing",0);
				break;
			}
			iterator++;
		}
	}
	while (text.find("$") != string::npos)
	{
		string temp;
		temp = (string)text.substr(text.rfind("$")).c_str();
		string::iterator iterator = temp.begin();
		int paren=0;
		int fin=0;
		string buffer;
		while (iterator != temp.end())
		{
			if (!strrchr(notin,*iterator) || paren)
			{
				if (*iterator != '(' && *iterator != ')')
					buffer+=*iterator;
			}
			else 
			{
					buffer.replace(0,1,"",0);
					HandleVars(buffer,"",text,buffer2,npcid,mob);
					buffer="";
					buffer2="";
					break;
			}
			buffer2+=*iterator;
			if (*iterator == '(')
			{
				paren++;
				if (paren == 1)
				{
					fname = buffer;
					buffer="";
				}
			}
			if (*iterator == ')')
			{
				paren--;
				if (paren == 0)
				{
					parms = buffer;
					fname.replace(0,1,"",0);
					HandleVars(fname,parms,text,buffer2,npcid,mob);
					buffer="";
					buffer2="";
					parms="";
					fname="";
					fin=1;
					break;
				}
			}
			iterator++;
		}
	}
}
	

char * fixstring(char * string)
{
	char tmp[255];
	memset(tmp,0x0,255);

	static char tmp2[255];
	memset(tmp2,0x0,255);
	int quote=0;
	int	o=0;
	strcpy(tmp,string);
	uint32 len = strlen(tmp);
	for (uint32 i=0;i<len;i++)
	{
		if (quote || tmp[i] != ' ') {
			tmp2[o] = tmp[i];
			o++;
		}
		if (tmp[i] == '"')
			if (quote) quote--;
			else	   quote++;
	}
	return tmp2;
}

int DoCompare(string compare1, string sign, string compare2)
{
#if Parser_DEBUG>10
		printf("compare1: %s,sign: %s, compare2: %s\n",compare1.c_str(),sign.c_str(),compare2.c_str());
#endif
	if (!strcmp(sign.c_str(),"==")) {
		if (strcmp(strlwr((const char*)compare1.c_str()),strlwr((const char*)compare2.c_str())))
			return 0;
	}
	else if (!strcmp(sign.c_str(),"!=")) {
		if (!strcmp(strlwr((const char*)compare1.c_str()),strlwr((const char*)compare2.c_str())))
			return 0;
	}
	else if (!strcmp(sign.c_str(),"=~")) {
		if (!strstr(strlwr(compare1.c_str()).c_str(),strlwr(compare2.c_str()).c_str()))
			return 0;
	}
	else if (!strcmp(sign.c_str(),"!~")) {
		if (strstr(strlwr(compare1.c_str()).c_str(),strlwr(compare2.c_str()).c_str()))
			return 0;
	}
	else if (!strcmp(sign.c_str(),"<")) {
		if (atoi(compare1.c_str()) > atoi(compare2.c_str()) || atoi(compare1.c_str()) == atoi(compare2.c_str()))
			return 0;
	}
	else if (!strcmp(sign.c_str(),">")) {
		if (atoi(compare1.c_str()) < atoi(compare2.c_str()) || atoi(compare1.c_str()) == atoi(compare2.c_str()))
			return 0;
	}
	else if (!strcmp(sign.c_str(),"<=")) {
		if (atoi(compare1.c_str()) > atoi(compare2.c_str()) || atoi(compare1.c_str()) != atoi(compare2.c_str()))
			return 0;
	}
	else if (!strcmp(sign.c_str(),">=")) {
		if (atoi(compare1.c_str()) < atoi(compare2.c_str()) || atoi(compare1.c_str()) != atoi(compare2.c_str()))
			return 0;
	}
	return 1;
}

int Parser::ParseIf(string text)
{
	string::iterator iterator = text.begin();
	string com1,com2,sign,next,buffer;
	while (*iterator)
	{
		if (!strchr(notin,*iterator))
			buffer+=*iterator;
		if (*iterator == '=' || *iterator == '!' || *iterator == '~')
		{
			sign+=*iterator;
			if (sign.length() == 1)
			{
				com1 = buffer;
				buffer="";
				next="";
			}
		}
		if ((*iterator == '<' || *iterator == '>') && iterator[1] != '=')
		{
			sign+=*iterator;
			if (sign.length() == 1)
			{
				com1 = buffer;
				buffer="";
				next="";
			}
			iterator++;
		}
		if (*iterator == '&' || *iterator == '|')
		{
			next+=*iterator;
			if (next.length() == 1)
			{
				com2 = buffer;
				buffer="";
			}
			if (next.length() == 2)
			{
				if (!DoCompare(com1,sign,com2) && strcmp(next.c_str(),"||"))
					return 0;
				com1="";
				sign="";
				com2="";
			}
		}
		iterator++;
		if (iterator == text.end()) {
			com2 = buffer;
			//com2.replace(0,1,"");
			buffer="";
			if (!DoCompare(com1,sign,com2))
				return 0;
		}
	}
	return 1;
}

int Parser::ParseCommands(string text, int line, int justcheck, int32 npcid, Mob* other, Mob* mob,  std::string filename)
{
	string buffer,command,parms,temp,temp2;
	temp2 = text;
	ParseVars(temp2,npcid,mob);
	int bracket=0,paren=0,lastif=0,last_finished=0,quote=0,escape=0,ignore=0,argnums=0,argit=1;
	string::iterator iterator = temp2.begin();
	while (iterator != temp2.end())
	{
		if (*iterator == '\\' && !escape) {
			escape++;
		}
		//"`~1234567890-=!@#$%^&*_+qwertyuiop[]asdfghjkl'zxcvbnm,./QWERTYUIOP|ASDFGHJKL:ZXCVBNM<>?\"\\"
		if (!ignore && *iterator != ')' && (strchr(charIn3,*iterator) || quote || escape || paren))
		{
			buffer+=*iterator;
		}
		if (*iterator == '"' && !escape && !ignore)
			if (quote)quote--;
			else quote++;
		
		if (*iterator == ',' && !ignore && !quote && !escape && paren)
			argit++;

		if (*iterator == '(' && !ignore && !quote && !escape)
		{
			paren++;
			if (paren == 1)	
			{
				if (last_finished)
				{
					if(mob && mob->IsClient())
					mob->CastToClient()->Message(10,"Line: %d,File: %s | error C0008: syntax error : missing ';' before function '%s'", line, filename.c_str(), command.c_str());
					return 0;
				}
				command = buffer;
				if(!strcmp(strlwr(command.c_str()),"break") && other)
					return 1;
				buffer="";
				argnums = GetArgs(command);
#if Parser_DEBUG>10
					if(mob && mob->IsClient())
					mob->CastToClient()->Message(10,"Command: %s, Num Args: %i\n",command.c_str(),argnums);
#endif
				if (argnums == -1)
				{
					if(mob && mob->IsClient())
					mob->CastToClient()->Message(10,"Line: %d,File: %s | error C0007: '%s' : Unknown function", line, filename.c_str(), command.c_str());
					return 0;
				}
			}
		}	
		else if (*iterator == ')' && !ignore && !quote && !escape)
		{
			paren--;
			if (paren == 0)
			{
				lastif=0,quote=0,escape=0,ignore=0;
				parms = buffer;
				buffer="";
				if (!strcmp(strlwr((const char*)command.c_str()),"if")) { last_finished=0; }
				else {
					last_finished=1;
				}
			}
			if (paren<0)
			{
					if(mob && mob->IsClient())
					mob->CastToClient()->Message(10,"Line: %d,File: %s | error C0006: syntax error : too many ')'s",line,filename.c_str() );
				return 0;
			}
		}			
		else if (*iterator == '{' && !escape)
		{
			bracket++;
			if (!ignore) {
				if (!strcmp(strlwr((const char*)command.c_str()),"if")) {
					lastif = ParseIf(parms);
#if Parser_DEBUG>10
					if(mob && mob->IsClient())
					mob->CastToClient()->Message(10,"Parms: %s\n",parms.c_str());
#endif
					if (!lastif) ignore=1;
					else ignore=0;
				}
			}
		}
		else if (*iterator == '}' && !escape)
		{
			bracket--;
			if (last_finished)
			{
					if(mob && mob->IsClient())
					mob->CastToClient()->Message(10,"Line: %d,File: %s | error C0008: syntax error : missing ';' before '}'", line,filename.c_str() );
				return 0;
			}
			if (bracket<0)
			{
					if(mob && mob->IsClient())
					mob->CastToClient()->Message(10,"Line: %d,File: %s | error C0006: syntax error : too many '}'s",line, filename.c_str() );
				return 0;
			}
			if (bracket == 0)
			{
				if (!ignore) lastif=1;
				else		 lastif=0;
				lastif=0,last_finished=0,quote=0,escape=0,ignore=0,argnums=0,argit=1;
			}
		}		
		else if (*iterator == ';' && !escape && !ignore && !quote)
		{
			if (last_finished)
			{
				last_finished=0;
				if (argnums != 1 && argnums!=argit)
				{
					if(mob && mob->IsClient())
					mob->CastToClient()->Message(10,"Line: %d, File: %s | error C0001: '%s' : function does not take %d parameter(s)", line, filename.c_str(), command.c_str(), argit);
					return 0;
				}
				if (!justcheck)
					ExCommands((const char*)command.c_str(),(const char*)parms.c_str(),argnums, npcid, other, mob);
				argit=1;
			}
			else {
					if(mob && mob->IsClient())
					mob->CastToClient()->Message(10,"Line: %d,File: %s | error C0002: '%s' :syntax error : '(' %d '('s still not closed.", line, filename.c_str(), command.c_str(), paren);
				}
		}
		if (escape) escape--;
			iterator++;
	}
	if (last_finished)
	{
					if(mob && mob->IsClient())
					mob->CastToClient()->Message(10,"Line: %d,File: %s | error C0008: syntax error : missing ';' before '}'", line, filename.c_str() );
		return 0;
	}
	return 1;
}
