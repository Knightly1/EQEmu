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

//extends the parser to include perl
//Eglin

#ifndef EMBPARSER_CPP
#define EMBPARSER_CPP

#ifdef EMBPERL

#include "masterentity.h"
#include "../common/debug.h"
#include "embparser.h"

#include <algorithm>

//Uncomment this line, or put -DQUEST_SCRIPTS_BYNAME in your
//makefile's cflags to enable named quest files:
//#define QUEST_SCRIPTS_BYNAME

#ifdef QUEST_SCRIPTS_BYNAME

//extends byname system to look in templates directory
#define QUEST_TEMPLATES_BYNAME
#define QUEST_TEMPLATES_DIRECTORY "templates"

#endif

#ifdef WIN32
//borrow this from Wes... I like it
extern char* itoa(int integer);
#endif

PerlembParser::PerlembParser(void) : Parser()
{
	try { perl = new Embperl; map_funs(); }
	catch(const char * msg) { perl = NULL; LogFile->write(EQEMuLog::Status, "Error initializing perlembed: %s", msg); throw msg;}

	//let's load the default script
	try { LoadScript(0, NULL); }
	catch(const char * err)
	{
		LogFile->write(EQEMuLog::Status, "Error loading default script: %s", err);
	}
}

PerlembParser::~PerlembParser()
{
	if(perl)
		delete perl;
}

void PerlembParser::ExportVar(const char * pkgprefix, const char * varname, const char * value) const
{
	if(!perl)
		return;
	try
	{
		//todo: consider replacing ' w/ ", so that values can be expanded on the perl side
// MYRA - fixed eval in ExportVar per Eglin
		perl->eval(std::string("$").append(pkgprefix).append("::").append(varname).append("=q(").append(value).append(");").c_str());
//end Myra

	}
	catch(const char * err)
	{ //todo: consider rethrowing 
		LogFile->write(EQEMuLog::Status, "Error exporting var: %s", err);
	}
}

void PerlembParser::Event(int event, int32 npcid, const char * data, Mob* npcmob, Mob* mob)
{
	if(!perl)
		return;

	string packagename = GetPkgPrefix(npcid);

	if(!isloaded(packagename.c_str()))
	{
		LoadScript(npcid, zone->GetShortName());
	}

	packagename = GetPkgPrefix(npcid);

// SCORPIOUS2K - load global variables

	if (npcmob->GetQglobal())
	{
		char errbuf[MYSQL_ERRMSG_SIZE];
		char *query = 0;
		MYSQL_RES *result;
		MYSQL_ROW row;
//		char tmpname[64];
		int charid=0;
		
			if (mob && mob->IsClient())  // some events like waypoint and spawn don't have a player involved
			{
					charid=mob->CastToClient()->CharacterID();
			}

		else
		{
			charid=-npcmob->GetNPCTypeID();		// make char id negative npc id as a fudge
		}

		ExportVar(packagename.c_str(),"charid",itoa(charid));

		database.RunQuery(query, MakeAnyLenString(&query, 
		  "SELECT name,value FROM quest_globals WHERE (npcid=%i || npcid=0) && (charid=%i || charid=0) && (zoneid=%i || zoneid=0) && expdate >= unix_timestamp(now())", 
		     npcmob->GetNPCTypeID(),charid,zone->GetZoneID()), errbuf, &result);
//		printf("%s\n",query);
//		printf("%s\n",errbuf);
		if (result)
		{
//			printf("Loading global variables for %s\n",npcmob->GetName());
			while ((row = mysql_fetch_row(result)))
			{
//				printf("$%s = %s\n",row[0],row[1]);
				ExportVar(packagename.c_str(), row[0], row[1]);
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
		ExportVar(packagename.c_str(), "timer", data);
	}
	int8 fac = 0;
	if (mob && mob->IsClient()) {
		ExportVar(packagename.c_str(), "uguildid", itoa(mob->CastToClient()->GuildDBID()));
// MYRA - corrected spelling for var $uguildrank for event_timer (was $uguildrang)
		ExportVar(packagename.c_str(), "uguildrank", itoa(mob->CastToClient()->GuildRank()));
//end Myra
// MYRA - added vars $status & $cumflag per Eglin
		ExportVar(packagename.c_str(), "status", itoa(mob->CastToClient()->Admin())); 
		ExportVar(packagename.c_str(), "cumflag", itoa(mob->CastToClient()->flag[50])); 
//end Myra
	}

	if (mob && npcmob && mob->IsClient() && npcmob->IsNPC()) {
		Client* client = mob->CastToClient();
		NPC* npc = npcmob->CastToNPC();
		
		// Need to figure out why one of these casts would fail..
		if (client && npc) {
			fac = client->GetFactionLevel(client->GetID(), npcmob->GetID(), client->GetRace(), client->GetClass(), DEITY_AGNOSTIC, npc->GetPrimaryFaction(), npcmob);
		}
		else if (!client) {
			//avoid cerr, since the zone servers may eventually not be running on the same machine/interface
//			cerr << "WARNING: cast failure on mob->CastToClient()" << endl;
			LogFile->write(EQEMuLog::Status, "WARNING: cast failure on mob->CastToClient()");
		}
		else if (!npc) {
//			cerr << "WARNING: cast failure on npcmob->CastToNPC()" << endl;
			LogFile->write(EQEMuLog::Status, "WARNING: cast failure on npcmob->CastToNPC()");
		}
	}
	if (mob) {
		ExportVar(packagename.c_str(), "name", mob->GetName());
		ExportVar(packagename.c_str(), "race", GetRaceName(mob->GetRace()));
		ExportVar(packagename.c_str(), "class", GetEQClassName(mob->GetClass()));
		ExportVar(packagename.c_str(), "ulevel", itoa(mob->GetLevel()));
		ExportVar(packagename.c_str(), "userid", itoa(mob->GetID()));
	}
	if (npcmob)
	{
		ExportVar(packagename.c_str(), "mname", npcmob->GetName());
// MYRA - added vars $mobid & $mlevel per Eglin
		ExportVar(packagename.c_str(), "mobid", itoa(npcmob->GetID())); 
		ExportVar(packagename.c_str(), "mlevel", itoa(npcmob->GetLevel())); 
//end Myra
// hp event
		ExportVar(packagename.c_str(), "hpevent", itoa((int)npcmob->GetNextHPEvent())); 
		ExportVar(packagename.c_str(), "hpratio", itoa((int)npcmob->GetHPRatio()));
// sandy bug fix
		ExportVar(packagename.c_str(), "x", itoa( (int)npcmob->GetX() )); 
		ExportVar(packagename.c_str(), "y", itoa( (int)npcmob->GetY() )); 
		ExportVar(packagename.c_str(), "z", itoa( (int)npcmob->GetZ() )); 
		ExportVar(packagename.c_str(), "h", itoa( (int)npcmob->GetHeading() ));
		if ( npcmob->GetTarget() ) {
			ExportVar(packagename.c_str(), "targetid", itoa(npcmob->GetTarget()->GetID()));
			ExportVar(packagename.c_str(), "targetname", npcmob->GetTarget()->GetName());
		}
	}

	if (fac) {
		ExportVar(packagename.c_str(), "faction", itoa(fac));
	}

	if (zone) {
// SCORPIOUS2K- added variable zoneid
		ExportVar(packagename.c_str(), "zoneid", itoa(zone->GetZoneID()));
		ExportVar(packagename.c_str(), "zoneln", zone->GetLongName());
		ExportVar(packagename.c_str(), "zonesn", zone->GetShortName());
	}

// $hasitem - compliments of smogo
#define HASITEM_FIRST 0 
#define HASITEM_LAST 29 // this includes worn plus 8 base slots 
#define HASITEM_ISNULLITEM(item) ((item==-1) || (item==0)) 

	if(mob && mob->IsClient())
	{ 
		string hashname = packagename + std::string("::hasitem"); 
		LogFile->write(EQEMuLog::Debug, "starting hasitem, on : %s",hashname.c_str() ); 

		//start with an empty hash 
		perl->eval(std::string("%").append(hashname).append(" = ();").c_str()); 

		for(int slot=HASITEM_FIRST; slot<=HASITEM_LAST;slot++)
		{ 
			char *hi_decl=NULL; 
			int itemid=mob->CastToClient()->GetItemIDAt(slot); 
			if(!HASITEM_ISNULLITEM(itemid))
			{ 
				MakeAnyLenString(&hi_decl, "push (@{$%s{%d}},%d);",hashname.c_str(),itemid,slot); 
//Father Nitwit: this is annoying
#if EQDEBUG >= 7
				LogFile->write(EQEMuLog::Debug, "declare hasitem : %s",hi_decl); 
#endif
				perl->eval(hi_decl); 
			} 
		}
	}

	switch (event) {
		case EVENT_SAY: {
			npcmob->FaceTarget(mob);

			ExportVar(packagename.c_str(), "data", itoa(npcid));
			ExportVar(packagename.c_str(), "text", data);
			SendCommands(packagename.c_str(), "EVENT_SAY", npcid, npcmob, mob);
			break;
		}
		case EVENT_TIMER: {
			SendCommands(packagename.c_str(), "EVENT_TIMER", npcid, npcmob, mob);
			break;
		}
		case EVENT_DEATH: {
			SendCommands(packagename.c_str(), "EVENT_DEATH", npcid, npcmob, mob);
			break;
		}
		case EVENT_ITEM: {
			npcmob->FaceTarget(mob);

			ExportVar(packagename.c_str(), "item1", GetVar("item1", npcid).c_str());
			ExportVar(packagename.c_str(), "item2", GetVar("item2", npcid).c_str()); 
			ExportVar(packagename.c_str(), "item3", GetVar("item3", npcid).c_str()); 
			ExportVar(packagename.c_str(), "item4", GetVar("item4", npcid).c_str()); 
			ExportVar(packagename.c_str(), "copper", GetVar("copper", npcid).c_str()); 
			ExportVar(packagename.c_str(), "silver", GetVar("silver", npcid).c_str()); 
			ExportVar(packagename.c_str(), "gold", GetVar("gold", npcid).c_str()); 
			ExportVar(packagename.c_str(), "platinum", GetVar("platinum", npcid).c_str());  
			string hashname = packagename + std::string("::itemcount"); 
			perl->eval(std::string("%").append(hashname).append(" = ();").c_str()); 
			perl->eval(std::string("++$").append(hashname).append("{$").append(packagename).append("::item1};").c_str()); 
			perl->eval(std::string("++$").append(hashname).append("{$").append(packagename).append("::item2};").c_str()); 
			perl->eval(std::string("++$").append(hashname).append("{$").append(packagename).append("::item3};").c_str()); 
			perl->eval(std::string("++$").append(hashname).append("{$").append(packagename).append("::item4};").c_str()); 
			SendCommands(packagename.c_str(), "EVENT_ITEM", npcid, npcmob, mob);
			
			break;
		}
		case EVENT_SPAWN: {
			SendCommands(packagename.c_str(), "EVENT_SPAWN", npcid, npcmob, mob);
			break;
		}
		case EVENT_ATTACK: {
			SendCommands(packagename.c_str(), "EVENT_ATTACK", npcid, npcmob, mob);
			break;
		}
		case EVENT_SLAY: {
			SendCommands(packagename.c_str(), "EVENT_SLAY", npcid, npcmob, mob);
			break;
		}
		case EVENT_WAYPOINT: {
			std::string temp = "wp";
				temp += itoa(npcid);
			ExportVar(packagename.c_str(), temp.c_str(), data);
			SendCommands(packagename.c_str(), "EVENT_WAYPOINT", npcid, npcmob, mob);
			break;
		}
		case EVENT_SIGNAL: {
			SendCommands(packagename.c_str(), "EVENT_SIGNAL", npcid, npcmob, mob);
			break;
		}
// event hp
		case EVENT_HP: { 
			if (npcmob) { 
				SendCommands(packagename.c_str(), "EVENT_HP", npcid, npcmob, mob); 
			} 
			break; 
		}
		case EVENT_AGGRO: { 
			SendCommands(packagename.c_str(), "EVENT_AGGRO", npcid, npcmob, mob); 
			break; 
		}

		default: {
			// should we do anything here?
			break;
		}
	}
}

void PerlembParser::ReloadQuests()
{
delete perl;
	try { perl = new Embperl; map_funs(); }
	catch(const char * msg) { perl = NULL; LogFile->write(EQEMuLog::Status, "Error initializing perlembed: %s", msg); throw msg;}
		try { LoadScript(0, NULL); }
	catch(const char * err)
	{
		LogFile->write(EQEMuLog::Status, "Error loading default script: %s", err);
	}
	hasQuests.clear();
}

int PerlembParser::LoadScript(int npcid, const char * zone, Mob* activater)
{
	if(!perl)
		return(0);
	
	//we have allready tried to load this quest...
	if(hasQuests.count(npcid) == 1) {
		return(1);
	}
	
	string filename= "quests/", packagename = GetPkgPrefix(npcid);
	//each package name is of the form qstxxxx where xxxx = npcid (since numbers alone are not valid package names)
	questMode curmode = questDefault;
//LogFile->write(EQEMuLog::Debug, "LoadScript(%d, %s):\n", npcid, zone);
	if(!npcid || !zone)
	{
//LogFile->write(EQEMuLog::Debug, "	default 1");
		filename += DEFAULT_QUEST_PREFIX;
		filename += ".pl";
		curmode = questDefault;
	}
	else
	{
		filename += zone;
		filename += "/";
#ifdef QUEST_SCRIPTS_BYNAME
		string bnfilename = filename;
#endif
		filename += itoa(npcid);
		filename += ".pl";
		curmode = questByID;
		
#ifdef QUEST_SCRIPTS_BYNAME
		//Father Nitwit's naming hack.
		//untested on windows...
		
		//assuming name limit stays 64 chars.
		char tmpname[64];
		int count0 = 0;
		bool filefound = false;
		FILE *tmpf;
		tmpf = fopen(filename.c_str(), "r");
		if(tmpf != NULL) {
			fclose(tmpf);
			filefound = true;
		}
//LogFile->write(EQEMuLog::Debug, "	tried '%s': %d", filename.c_str(), filefound);
		
		tmpname[0] = 0;
		//if there is no file for the NPC's ID, try for the NPC's name
		if(! filefound) {
			//revert to just path
			filename = bnfilename;
			const NPCType *npct = database.GetNPCType(npcid);
			if(npct == NULL) {
//LogFile->write(EQEMuLog::Debug, "	no npc type");
				//revert and go on with life
				filename += itoa(npcid);
				filename += ".pl";
				curmode = questByID;
			} else {
				//trace out the ` characters, turn into -
				
				int nlen = strlen(npct->name);
				if(nlen < 64) {	//just to make sure
					int r;
					//this should get our NULL as well..
					for(r = 0; r <= nlen; r++) {
						tmpname[r] = npct->name[r];
						
						//watch for 00 delimiter
						if(tmpname[r] == '0') {
							count0++;
							if(count0 > 1) {	//second '0'
								//stop before previous 0
								tmpname[r-1] = '\0';
								break;
							}
						} else {
							count0 = 0;
						}
						
						//rewrite ` to be more file name friendly
						if(tmpname[r] == '`')
							tmpname[r] = '-';
						
					}
					filename += tmpname;
					filename += ".pl";
					curmode = questByName;
				} else {
//LogFile->write(EQEMuLog::Debug, "	namelen too long");
					//revert and go on with life, again
					filename += itoa(npcid);
					filename += ".pl";
					curmode = questByID;
				}
			}
		}
		
	#ifdef QUEST_TEMPLATES_BYNAME
		bool filefound2 = false;
		tmpf = fopen(filename.c_str(), "r");
		if(tmpf != NULL) {
			fclose(tmpf);
			filefound2 = true;
		}
//LogFile->write(EQEMuLog::Debug, "	tried '%s': %d", filename.c_str(), filefound2);
		
		//if there is no file for the NPC's ID or name, 
		//try for the NPC's name in the templates directory
		//only works if we have gotten the NPC's name above
		if(! filefound && ! filefound2) {
			if(tmpname[0] != 0) {
				//revert to just path
				filename = "quests/";
				filename += QUEST_TEMPLATES_DIRECTORY;
				filename += "/";
				filename += tmpname;
				filename += ".pl";
				curmode = questTemplate;
//LogFile->write(EQEMuLog::Debug, "	template '%s'", filename.c_str(), filefound2);
			} else {
//LogFile->write(EQEMuLog::Debug, "	no template name");
				filename += itoa(npcid);
				filename += ".pl";
				curmode = questDefault;
			}
		}
	#endif	//QUEST_TEMPLATES_BYNAME
		
#endif //QUEST_SCRIPTS_BYNAME

	}

//LogFile->write(EQEMuLog::Debug, "	finally settling on '%s'", filename.c_str());
//	LogFile->write(EQEMuLog::Status, "Looking for quest file: '%s'", filename.c_str());

//  todo: decide whether or not to delete the package to allow for script refreshes w/o restarting the server
//  remember to guard against deleting the default package, on a similar note... consider deleting packages upon zone change
//	try { perl->eval(std::string("delete_package(\"").append(packagename).append("\");").c_str()); }
//	catch(...) {/*perl balked at us trynig to delete a non-existant package... no big deal.*/}

	try { perl->eval_file(packagename.c_str(), filename.c_str()); }
	catch(const char * err)
	{
		//try to reduce some of the console spam... 
		//todo: tweak this to be more accurate at deciding what to filter (we don't want to gag legit errors)
		if(!strstr(err,"No such file or directory"))
			LogFile->write(EQEMuLog::Status, "WARNING: error compiling quest file %s: %s (reverting to default questfile)", filename.c_str(), err);
	}
	//todo: change this to just read eval_file's %cache - duh!
	if(!isloaded(packagename.c_str()))
	{
		//the npc has no qst file, attach the defaults
		std::string setdefcmd = "$";
			setdefcmd += packagename;
			setdefcmd += "::isdefault = 1;";
		perl->eval(setdefcmd.c_str());
		setdefcmd = "$";
			setdefcmd += packagename;
			setdefcmd += "::isloaded = 1;";
		perl->eval(setdefcmd.c_str());
		curmode = questDefault;
	}
	
//LogFile->write(EQEMuLog::Debug, "	final mode = %d", curmode);
	hasQuests[npcid] = curmode;
	return(1);
}

//this function does NOT consider the default to be a quest 
bool PerlembParser::HasQuestFile(int32 npcid) {
	sint32 qstID = GetNPCqstID(npcid);
	int success=1;
	
	if(hasQuests.count(npcid) == 1) {
		questMode mode = hasQuests[npcid];
		if(mode == questDefault)
			return(false);
		return(true);
	}
	
	if (qstID==-1)
		success = LoadScript(npcid, zone->GetShortName());
	if (!success) 
		return(false);
	
	if(hasQuests.count(npcid) != 1)
		return(false);
	
	questMode mode = hasQuests[npcid];
	if(mode == questDefault)
		return(false);
	
	return(true);
}

//utility - return something of the form "qst1234"... 
//will return "qst[DEFAULT_QUEST_PREFIX]" if the npc in question has no script of its own or failed to compile and defaultOK is set to true
std::string PerlembParser::GetPkgPrefix(int32 npcid, bool defaultOK)
{
	char buf[32];
	snprintf(buf, 32, "qst%lu", (unsigned long) npcid);
//	std::string prefix = "qst";
//	std::string temp = prefix + (std::string)(itoa(npcid));
//	if(!npcid || (defaultOK && isdefault(temp.c_str())))
	if(!npcid || (defaultOK && (hasQuests.count(npcid) == 1 && hasQuests[npcid] == questDefault)))
	{
		snprintf(buf, 32, "qst%s", DEFAULT_QUEST_PREFIX.c_str());
	}
	
	return(std::string(buf));
}

void PerlembParser::SendCommands(const char * pkgprefix, const char *event, int32 npcid, Mob* other, Mob* mob)
{
	if(!perl)
		return;
	try
	{
		std::string cmd = "@quest::cmd_queue = (); package " + (std::string)(pkgprefix) + (std::string)(";");
		perl->eval(cmd.c_str());
		perl->dosub(std::string(pkgprefix).append("::").append(event).c_str());
	}
	catch(const char * err)
	{
		//try to reduce some of the console spam... 
		//todo: tweak this to be more accurate at deciding what to filter (we don't want to gag legit errors)
		if(!strstr(err,"Undefined subroutine"))
			LogFile->write(EQEMuLog::Status, "Script error: %s::%s - %s", pkgprefix, event, err);
		return;
	}
	
	int numcoms = perl->geti("quest::qsize()");
	for(int c = 0; c < numcoms; ++c)
	{
		char var[1024] = {0};
		sprintf(var,"$quest::cmd_queue[%d]{func}",c);
		std::string cmd = perl->getstr(var);
		sprintf(var,"$quest::cmd_queue[%d]{args}",c);
		std::string args = perl->getstr(var);
		size_t num_args = std::count(args.begin(), args.end(), ',') + 1;
		ExCommands(cmd, args, num_args, npcid, other, mob);
	}
}

void PerlembParser::map_funs(void) const
{
	//map each "exported" function to a variable list that we can access from c
	//todo:
	//	break 1|settimer 2|stoptimer 1|dbspawnadd 2|flagcheck 1|write 2|
	//	settarget 2|follow 1|sfollow 1|save 1|setallskill 1
	//update/ensure that the api matches that of the native script engine
	perl->eval(
"{"
"package quest;"
"&boot_qc;"
"@cmd_queue = ();"
"sub qsize{return scalar(@cmd_queue)};"
"sub say{push(@cmd_queue,{func=>'say',args=>join(',',@_)});}"
"sub emote{push(@cmd_queue,{func=>'emote',args=>join(',',@_)});}"
"sub shout{push(@cmd_queue,{func=>'shout',args=>join(',',@_)});}"
"sub spawn{push(@cmd_queue,{func=>'spawn',args=>join(',',@_)});}"
"sub spawn2{push(@cmd_queue,{func=>'spawn2',args=>join(',',@_)});}"
"sub echo{push(@cmd_queue,{func=>'echo',args=>join(',',@_)});}"
"sub summonitem{push(@cmd_queue,{func=>'summonitem',args=>join(',',@_)});}"
"sub castspell{push(@cmd_queue,{func=>'castspell',args=>join(',',@_)});}"
"sub selfcast{push(@cmd_queue,{func=>'selfcast',args=>join(',',@_)});}"
"sub depop{push(@cmd_queue,{func=>'depop'});}"
"sub cumflag{push(@cmd_queue,{func=>'cumflag'});}"
"sub flagnpc{push(@cmd_queue,{func=>'flagnpc',args=>join(',',@_)});}"
"sub flagclient{push(@cmd_queue,{func=>'flagclient',args=>join(',',@_)});}"
"sub exp{push(@cmd_queue,{func=>'exp',args=>join(',',@_)});}"
"sub level{push(@cmd_queue,{func=>'level',args=>join(',',@_)});}"
"sub safemove{push(@cmd_queue,{func=>'safemove'});}"
"sub rain{push(@cmd_queue,{func=>'rain',args=>join(',',@_)});}"
"sub snow{push(@cmd_queue,{func=>'snow',args=>join(',',@_)});}"
"sub givecash{push(@cmd_queue,{func=>'givecash',args=>join(',',@_)});}"
"sub pvp{push(@cmd_queue,{func=>'pvp',args=>join(',',@_)});}"
"sub doanim{push(@cmd_queue,{func=>'doanim',args=>join(',',@_)});}"
"sub addskill{push(@cmd_queue,{func=>'addskill',args=>join(',',@_)});}"
"sub me{push(@cmd_queue,{func=>'me',args=>join(',',@_)});}"

"sub permagender{push(@cmd_queue,{func=>'permagender',args=>join(',',@_)});}"//Cofruben:-New perma functions 
"sub permarace{push(@cmd_queue,{func=>'permarace',args=>join(',',@_)});}" 
"sub scribespells{push(@cmd_queue,{func=>'scribespells',args=>join(',',@_)});}" 
"sub permaclass{push(@cmd_queue,{func=>'permaclass',args=>join(',',@_)});}" 
"sub surname{push(@cmd_queue,{func=>'surname',args=>join(',',@_)});}" 
"sub addldonpoint{push(@cmd_queue,{func=>'addldonpoint',args=>join(',',@_)});}" 
"sub ding{push(@cmd_queue,{func=>'ding',args=>join(',',@_)});}" 

// MYRA - added missing commands + itemlink to perl
"sub faction{push(@cmd_queue,{func=>'faction',args=>join(',',@_)});}" 
"sub setguild{push(@cmd_queue,{func=>'setguild',args=>join(',',@_)});}"
"sub rebind{push(@cmd_queue,{func=>'rebind',args=>join(',',@_)});}" 
"sub flagcheck{push(@cmd_queue,{func=>'flagcheck',args=>join(',',@_)});}"
"sub write{push(@cmd_queue,{func=>'write',args=>join(',',@_)});}" 
"sub settime{push(@cmd_queue,{func=>'settime',args=>join(',',@_)});}"
"sub setsky{push(@cmd_queue,{func=>'setsky',args=>join(',',@_)});}" 
"sub settimer{push(@cmd_queue,{func=>'settimer',args=>join(',',@_)});}"
"sub stoptimer{push(@cmd_queue,{func=>'stoptimer',args=>join(',',@_)});}"
"sub settarget{push(@cmd_queue,{func=>'settarget',args=>join(',',@_)});}"
"sub follow{push(@cmd_queue,{func=>'follow',args=>join(',',@_)});}" 
"sub sfollow{push(@cmd_queue,{func=>'sfollow',args=>join(',',@_)});}"
"sub movepc{push(@cmd_queue,{func=>'movepc',args=>join(',',@_)});}"
"sub gmmove{push(@cmd_queue,{func=>'gmmove',args=>join(',',@_)});}"
"sub movegrp{push(@cmd_queue,{func=>'movegrp',args=>join(',',@_)});}"
"sub setlanguage{push(@cmd_queue,{func=>'setlanguage',args=>join(',',@_)});}" // bUsh
"sub setskill{push(@cmd_queue,{func=>'setskill',args=>join(',',@_)});}" // bUsh
"sub setallskill{push(@cmd_queue,{func=>'setallskill',args=>join(',',@_)});}"
"sub attack{push(@cmd_queue,{func=>'attack',args=>join(',',@_)});}"
"sub save{push(@cmd_queue,{func=>'save',args=>join(',',@_)});}"
"sub linkitem{push(@cmd_queue,{func=>'linkitem',args=>join(',',@_)});}"
//end Myra
"sub signal{push(@cmd_queue,{func=>'signal',args=>join(',',@_)});}"
// SCORPIOUS2K - add perl versions qglobal commands
"sub setglobal{push(@cmd_queue,{func=>'setglobal',args=>join(',',@_)});}"
"sub targlobal{push(@cmd_queue,{func=>'targlobal',args=>join(',',@_)});}"
"sub delglobal{push(@cmd_queue,{func=>'delglobal',args=>join(',',@_)});}"
// event hp
"sub setnexthpevent{push(@cmd_queue,{func=>'setnexthpevent',args=>join(',',@_)});}"
"sub respawn{push(@cmd_queue,{func=>'respawn',args=>join(',',@_)});}"
// new wandering commands
"sub stop{push(@cmd_queue,{func=>'stop',args=>join(',',@_)});}"
"sub pause{push(@cmd_queue,{func=>'pause',args=>join(',',@_)});}"
"sub resume{push(@cmd_queue,{func=>'resume',args=>join(',',@_)});}"
"sub start{push(@cmd_queue,{func=>'start',args=>join(',',@_)});}"
"sub moveto{push(@cmd_queue,{func=>'moveto',args=>join(',',@_)});}"
// add ldon points command
"sub warp{push(@cmd_queue,{func=>'warp',args=>join(',',@_)});}"
"sub changedeity{push(@cmd_queue,{func=>'changedeity',args=>join(',',@_)});}"
"sub addldonpoints{push(@cmd_queue,{func=>'addldonpoints',args=>join(',',@_)});}"
"sub addloot{push(@cmd_queue,{func=>'addloot',args=>join(',',@_)});}"
"sub traindisc{push(@cmd_queue,{func=>'traindisc',args=>join(',',@_)});}"
"package main;"
"}"
);//eval
}

#endif //EMBPERL

#endif //EMBPARSER_CPP
