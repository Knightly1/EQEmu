/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2004  EQEMu Development Team (http://eqemulator.net)

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

/*

Assuming you want to add a new perl quest function named joe
that takes 1 integer argument....

1. Add the prototype to the quest manager:
questmgr.h: add (~line 50)
	void joe(int arg);

2. Define the actual function in questmgr.cpp:
void QuestManager::joe(int arg) {
	//... do something
}

3. Copy one of the XS routines in perlparser.cpp, preferably
 one with the same number of arguments as your routine. Rename
 as needed.
 Finally, add your routine to the list at the bottom of perlparser.cpp


4.
If you want it to work in old mode perl and .qst, edit parser.cpp
Parser::ExCommands (~line 777)
	else if (!strcmp(command,"joe")) {
		quest_manager.joe(atoi(arglist[0]));
	}

And then at then end of embparser.cpp, add:
"sub joe{push(@cmd_queue,{func=>'joe',args=>join(',',@_)});}"



*/

#include "../common/debug.h"
#include "entity.h"
#include "masterentity.h"

#include <iostream>
#include <list>
using namespace std;

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
#include "parser.h"
#include "event_codes.h"

extern Database database;
extern Zone* zone;
extern WorldServer worldserver;
extern EntityList entity_list;



#include "questmgr.h"

//declare our global instance
QuestManager quest_manager;

QuestManager::QuestManager() {
	depop_npc = false;
}

QuestManager::~QuestManager() {
}

void QuestManager::Process() {
	list<QuestTimer>::iterator cur = QTimerList.begin(), end, tmp;
	
	end = QTimerList.end();
	while (cur != end) {
		if (cur->Timer_.Enabled() && cur->Timer_.Check()) {
			//make sure the mob is still in zone.
			if(entity_list.IsMobInZone(cur->mob)) {
				parse->Event(EVENT_TIMER, cur->mob->GetNPCTypeID(), cur->name.c_str(), cur->mob, NULL);
				//we MUST reset our iterator since the quest could have removed/added any
				//number of timers... worst case we have to check a bunch of timers twice
				cur = QTimerList.begin();
				end = QTimerList.end();	//dunno if this is needed, cant hurt...
			} else {
				tmp = cur;
				tmp++;
				QTimerList.erase(cur);
				cur = tmp;
			}
		} else
			cur++;
	}
	
	
	list<SignalTimer>::iterator curS = STimerList.begin(), endS, tmpS;
	
	endS = STimerList.end();
	while (curS != endS) {
		if (curS->Timer_.Enabled() && curS->Timer_.Check()) {
			
			//signal the event...
			entity_list.SignalMobsByNPCID(curS->npc_id, curS->signal_id);
			
			//remove the timer
			tmpS = curS;
			tmpS++;
			STimerList.erase(curS);
			curS = tmpS;
		} else
			curS++;
	}
}

void QuestManager::StartQuest(NPC *_npc, Client *_initiator) {
	quest_mutex.lock();
	npc = _npc;
	initiator = _initiator;
	depop_npc = false;
}

void QuestManager::EndQuest() {
	quest_mutex.unlock();
	if(depop_npc) {
		//clear out any timers for them...
		list<QuestTimer>::iterator cur = QTimerList.begin(), end, tmp;
		
		end = QTimerList.end();
		while (cur != end) {
			if(cur->mob == npc) {
				tmp = cur;
				tmp++;
				QTimerList.erase(cur);
				cur = tmp;
			} else {
				cur++;
			}
		}
		
		npc->Depop();
		npc = NULL;	//just to be safe
	}
}


//quest perl functions
void QuestManager::echo(const char *str) {
	printf("%s\n", str);
}

void QuestManager::say(const char *str) {
	npc->Say(str);
}

void QuestManager::me(const char *str) {
	if (!initiator)
		return;
	entity_list.MessageClose(initiator, false, 200, 10, str);
}

void QuestManager::summonitem(int32 itemid, uint8 charges) {
	if(!initiator)
		return;
	initiator->SummonItem(itemid, charges);
}

void QuestManager::write(const char *file, const char *str) {
	FILE * pFile;
	pFile = fopen (file, "a");
	if(!pFile)
		return;
	fprintf(pFile, "%s\n", str);
	fclose (pFile);
}

int16 QuestManager::spawn2(int npc_type, int grid, int unused, float x, float y, float z, float heading) {
	const NPCType* tmp = 0;
	//int8 guildwarset = atoi(arglist[2]);
	if ((tmp = database.GetNPCType(npc_type))) 
	{

		NPC* npc = new NPC(tmp, 0, x, y, z, heading);


		npc->AddLootTable();
		entity_list.AddNPC(npc,true,true);
		// Quag: Sleep in main thread? ICK!
		// Sleep(200);
		// Quag: check is irrelevent, it's impossible for npc to be 0 here
		// (we're in main thread, nothing else can possibly modify it)
//		if(npc != 0) {
			if(grid > 0)
			{
				npc->AssignWaypoints(grid);
			}
			npc->SendPosUpdate();
//		}
		return(npc->GetID());
	}
	return(0);
}

int16 QuestManager::unique_spawn(int npc_type, int grid, int unused, float x, float y, float z, float heading) {
	Mob *other = entity_list.GetMobByNpcTypeID(npc_type);
	if(other != NULL) {
		return(other->GetID());
	}
	
	
	const NPCType* tmp = 0;
	//int8 guildwarset = atoi(arglist[2]);
	if ((tmp = database.GetNPCType(npc_type))) 
	{

		NPC* npc = new NPC(tmp, 0, x, y, z, heading);


		npc->AddLootTable();
		entity_list.AddNPC(npc,true,true);
		// Quag: Sleep in main thread? ICK!
		// Sleep(200);
		// Quag: check is irrelevent, it's impossible for npc to be 0 here
		// (we're in main thread, nothing else can possibly modify it)
//		if(npc != 0) {
			if(grid > 0)
			{
				npc->AssignWaypoints(grid);
			}
			npc->SendPosUpdate();
//		}
		return(npc->GetID());
	}
	return(0);
}

void QuestManager::setstat(int stat, int value) {
	if (initiator) 
		initiator->SetStats(stat, value);
}

void QuestManager::castspell(int spell_id, int target_id) {
	if (npc)
		npc->SpellFinished(spell_id, target_id);	
}

void QuestManager::selfcast(int spell_id) {
	if (initiator)
		initiator->SpellFinished(spell_id, initiator->GetID(),10,0);
}

void QuestManager::addloot(int item_id, int charges) {
	if(item_id != 0)
		npc->AddItem(item_id, charges);
}

void QuestManager::Zone(const char *zone_name) {
	if (initiator && initiator->IsClient())
	{
		ServerPacket* pack = new ServerPacket(ServerOP_ZoneToZoneRequest, sizeof(ZoneToZone_Struct));
		ZoneToZone_Struct* ztz = (ZoneToZone_Struct*) pack->pBuffer;
		ztz->response = 0;
		ztz->current_zone_id = zone->GetZoneID();
		ztz->requested_zone_id = database.GetZoneID(zone_name);
		ztz->admin = initiator->Admin();
		strcpy(ztz->name, initiator->GetName());
		ztz->guild_id = initiator->GuildDBID();
		ztz->ignorerestrictions = 3;
		worldserver.SendPacket(pack);
		safe_delete(pack);				
	}
}

void QuestManager::settimer(const char *timer_name, int seconds) {
	list<QuestTimer>::iterator cur = QTimerList.begin(), end;
	
	end = QTimerList.end();
	while (cur != end)
	{
		if (cur->name == timer_name) {
			cur->mob = npc;
			cur->Timer_.Enable();
			cur->Timer_.Start(seconds * 1000, false);
			printf("Reseting: %s for %d seconds\n", cur->name.c_str(), seconds);
			return;
		}
		cur++;
	}
	
/*	timers * tmp = new timers;
	tmp->mob = npc;
	tmp->Timer_ = new Timer(seconds * 1000,0);
	tmp->Timer_->Start(seconds * 1000,false);
	tmp->name = timer_name;
	printf("Adding: %s for %d seconds\n", tmp->name.c_str(), seconds);
	QTimerList.push_back(tmp);*/
	QTimerList.push_back(QuestTimer(seconds * 1000, npc, timer_name));
}

void QuestManager::stoptimer(const char *timer_name) {
printf("Stop timer called on '%s'!\n", timer_name);
	list<QuestTimer>::iterator cur = QTimerList.begin(), end;
	
	end = QTimerList.end();
	while (cur != end)
	{
		if(cur->name == timer_name)
		{
			QTimerList.erase(cur);
			return;
		}
		cur++;
	}
}

void QuestManager::emote(const char *str) {
	npc->Emote(str);
}

void QuestManager::shout(const char *str) {
	npc->Shout(str);
}

void QuestManager::shout2(const char *str) {
	worldserver.SendEmoteMessage(0,0,0,13, "%s shouts, '%s'", npc->GetCleanName(), str);	
}

void QuestManager::depop(int npc_type) {
	if (npc_type != 0){
		Mob * tmp = entity_list.GetMobByNpcTypeID(npc_type);
		if (tmp) {
			if(tmp != npc)
				tmp->CastToNPC()->Depop();
			else
				depop_npc = true;
		}
	}
	else {	//depop self
		depop_npc = true;
	}
}

void QuestManager::settarget(const char *type, int target_id) {
	Mob* tmp = NULL;
	if (!strcasecmp(type,"npctype")) {
		tmp = entity_list.GetMobByNpcTypeID(target_id);
	}
	else if (!strcasecmp(type, "entity")) {
		tmp = entity_list.GetMob(target_id);
	}
	if(tmp != NULL) {
		npc->SetTarget(tmp);
	}
}

void QuestManager::follow(int entity_id) {
	npc->SetFollowID(entity_id);
}

void QuestManager::sfollow() {
	npc->SetFollowID(0);
}

void QuestManager::cumflag() {
	npc->flag[50] = npc->flag[50] + 1;
}

void QuestManager::flagnpc(int32 flag_num, int8 flag_value) {
	if (flag_num >= (sizeof(npc->flag) / sizeof(npc->flag[0])))
		npc->flag[flag_num] = flag_value;
	else {
		// Quag: TODO: Script error here, handle it somehow?
	}
}

void QuestManager::flagcheck(int32 flag_to_check, int32 flag_to_set) {
	if (flag_to_check >= (sizeof(npc->flag) / sizeof(npc->flag[0])) || flag_to_set >= (sizeof(npc->flag) / sizeof(npc->flag[0]))) {
	if (initiator && initiator->flag[flag_to_check] != 0)
		initiator->flag[flag_to_set] = 0;
	} else {
	// Quag: TODO: Script error here, handle it somehow?
	}
// Quag: Orignal code, not sure how this is supposed to work
//				if (initiator->flag[atoi(arglist[0])] != 0)
//					if (initiator) initiator->flag[atoi(arg1)] = 0;

}

void QuestManager::changedeity(int diety_id) {
	//Cofruben:-Changes the deity.
	if (initiator && initiator->IsClient())
	{
		initiator->SetDeity(diety_id);
		initiator->Message(15,"Your Deity has been changed/set to: %i", diety_id);
		initiator->Save(1);
		initiator->Kick();
	}
	else
		initiator->Message(15,"Error changing Deity");

}

void QuestManager::exp(int amt) {
	if (initiator && initiator->IsClient())
		initiator->AddEXP(amt);
}

void QuestManager::level(int newlevel) {
	if (initiator && initiator->IsClient())
		initiator->SetLevel(newlevel, true);
}

void QuestManager::traindisc(int discipline_tome_item_id) {
	if (initiator && initiator->IsClient())
		initiator->TrainDiscipline(discipline_tome_item_id);
}

bool QuestManager::isdisctome(int item_id) {
//get the item info
	const Item_Struct *item = database.GetItem(item_id);
	if(item == NULL) {
		return(false);
	}
	
	if(item->ItemClass != ItemTypeCommon || item->Common.ItemUse != ItemUseSpell) {
		return(false);
	}
	
	//Need a way to determine the difference between a spell and a tome
	//so they cant turn in a spell and get it as a discipline
	//this is kinda a hack:
	if(!(
		item->Name[0] == 'T' &&
		item->Name[1] == 'o' &&
		item->Name[2] == 'm' &&
		item->Name[3] == 'e' &&
		item->Name[4] == ' '
		)) {
		return(false);
	}
	
	//we know for sure none of the int casters get disciplines
	uint32 cbit = 0;
	cbit |= 1 << (WIZARD-1);
	cbit |= 1 << (ENCHANTER-1);
	cbit |= 1 << (MAGICIAN-1);
	cbit |= 1 << (NECROMANCER-1);
	if(item->Common.Classes & cbit) {
		return(false);
	}
	
	int32 spell_id = item->Common.SpellId;
	if(!IsValidSpell(spell_id)) {
		return(false);
	}
	
	//we know for sure none of the int casters get disciplines
	const SPDat_Spell_Struct &spell = spells[spell_id];
	if(
		spell.classes[WIZARD - 1] != 255 &&
		spell.classes[ENCHANTER - 1] != 255 &&
		spell.classes[MAGICIAN - 1] != 255 &&
		spell.classes[NECROMANCER - 1] != 255
	) {
		return(false);
	}
	
	return(true);
}

void QuestManager::safemove() {
	if (initiator && initiator->IsClient())
		initiator->MovePC(zone->GetShortName(),database.GetSafePoint(zone->GetShortName(),"x"),database.GetSafePoint(zone->GetShortName(),"y"),database.GetSafePoint(zone->GetShortName(),"z"));
}

void QuestManager::rain(int weather) {
	zone->zone_weather = weather;
	APPLAYER* outapp = new APPLAYER(OP_Weather, 8);
	*((int32*) &outapp->pBuffer[4]) = (int32) weather; // Why not just use 0x01/2/3?
	entity_list.QueueClients(npc, outapp);
	safe_delete(outapp);
}

void QuestManager::snow(int weather) {
	zone->zone_weather = weather + 1;
	APPLAYER* outapp = new APPLAYER(OP_Weather, 8);
	outapp->pBuffer[0] = 0x01;
	*((int32*) &outapp->pBuffer[4]) = (int32)weather;
	entity_list.QueueClients(initiator, outapp);
	safe_delete(outapp);
}

void QuestManager::surname(const char *name) {
	//Cofruben:-Changes the last name. 
	if (initiator && initiator->IsClient()) 
	{ 
			initiator->ChangeLastName(name); 
			initiator->Message(15,"Your surname has been changed/set to: %s", name); 
	} 
	else 
		initiator->Message(15,"Error changing/setting surname");
}

void QuestManager::permaclass(int class_id) {
 	//Cofruben:-Makes the client the class specified 
	initiator->SetBaseClass(class_id); 
	initiator->Save(2); 
	initiator->Kick();
}

void QuestManager::permarace(int race_id) {
 	//Cofruben:-Makes the client the race specified 
	initiator->SetBaseRace(race_id); 
	initiator->Save(2); 
	initiator->Kick();
}

void QuestManager::permagender(int gender_id) {
 	//Cofruben:-Makes the client the gender specified 
	initiator->SetBaseGender(gender_id); 
	initiator->Save(2); 
	initiator->Kick();
}

void QuestManager::scribespells() {
 	//Cofruben:-Scribe spells for user up to his actual level. 
	int book_slot;
	int16 curspell;
	for(curspell = 0, book_slot = 0; curspell < SPDAT_RECORDS && book_slot < MAX_PP_SPELLBOOK; curspell++)
	{
	   if
	   (
		  spells[curspell].classes[WARRIOR] != 0 &&
		  spells[curspell].classes[initiator->GetPP().class_-1] <= initiator->GetLevel() &&
		  spells[curspell].skill != 52
	   )
	   {
		  initiator->ScribeSpell(curspell, book_slot++);
	   }
	}
}

void QuestManager::givecash(int copper, int silver, int gold, int platinum) {
	APPLAYER* outapp = new APPLAYER(OP_MoneyOnCorpse, sizeof(moneyOnCorpseStruct)); 
	moneyOnCorpseStruct* d = (moneyOnCorpseStruct*) outapp->pBuffer; 
	d->response      = 1; 
	d->unknown1      = 0x5a; 
	d->unknown2      = 0x40; 
	d->unknown3      = 0; 
	if (initiator && initiator->IsClient())
	{
	d->copper      = copper; 
	d->silver      = silver; 
	d->gold         = gold; 
	d->platinum      = platinum; 
	initiator->AddMoneyToPP(d->copper, d->silver, d->gold, d->platinum,true); 
	initiator->QueuePacket(outapp);
	string tmp;
	if (d->platinum>0){
		tmp = "You receive ";
		tmp += d->platinum;
		tmp += " plat"; 
	}
	if (d->gold>0){
		if (tmp.length()==0){
			tmp = "You receive ";
			tmp += d->gold;
			tmp += " gold";
		}
		else{
			tmp += ",";
			tmp += d->gold;
			tmp += " gold";
		}
	}
	if(d->silver>0){
		if (tmp.length()==0){
			tmp = "You receive ";
			tmp += d->silver;
			tmp += " silver";
		}
		else{
			tmp += ",";
			tmp += d->silver;
			tmp += " silver";
		}
	}
	if(d->copper>0){
		if (tmp.length()==0){
			tmp = "You receive ";
			tmp += d->copper;
			tmp += " copper";
		}
		else{
			tmp += ",";
			tmp += d->copper;
			tmp += " copper";
		}
	}
	tmp += " pieces.";
	if (initiator) 
		initiator->Message(MT_OOC,tmp.c_str());
	}
	safe_delete(outapp);
}

void QuestManager::pvp(const char *mode) {
	if (!strcasecmp(mode,"on"))
		if (initiator)
			initiator->SetPVP(true);
	else
		if (initiator)
			initiator->SetPVP(false);
}

void QuestManager::movepc(int zone_id, float x, float y, float z) {
	if (initiator && initiator->IsClient()) 
		 initiator->MovePC(zone_id, x, y, z);	
}

void QuestManager::gmmove(float x, float y, float z) {
	if (initiator && initiator->IsClient()) 
		initiator->GMMove(x, y, z);
}

void QuestManager::movegrp(int zoneid, float x, float y, float z) {
#ifdef IPC
    if (initiator && initiator->IsClient()|| (initiator->IsNPC() && initiator->CastToNPC()->IsInteractive()) )
#else
    if (initiator && initiator->IsClient())
#endif
	{
		Group *g = entity_list.GetGroupByClient(initiator);
       	if (g != NULL){
			g->TeleportGroup(initiator, zoneid, x, y, z);
		}
		else {
			if (initiator) initiator->MovePC(zoneid, x, y, z);
		}
	}
}

void QuestManager::doanim(int anim_id) {
	npc->DoAnim(anim_id);
}

void QuestManager::addskill(int skill_id, int value) {
	if (initiator && initiator->IsClient())
		initiator->AddSkill(skill_id, value);
}

void QuestManager::setlanguage(int skill_id, int value) {
	if (initiator && initiator->IsClient())
		initiator->SetLanguageSkill(skill_id, value);
}

void QuestManager::setskill(int skill_id, int value) {
	if (initiator && initiator->IsClient())
		initiator->SetSkill(skill_id, value);
}

void QuestManager::setallskill(int value) {
	if (!initiator)
		return;
	if (initiator && initiator->IsClient()) { 
		for(int skill_num=0;skill_num<74;skill_num++)
			initiator->SetSkill(skill_num, value);
	}
}

void QuestManager::attack(const char *client_name) {
	Client* getclient = entity_list.GetClientByName(client_name);
	if(getclient && npc->IsAttackAllowed(getclient))
	{
		npc->AddToHateList(getclient,1);
	}
	else
	{
		npc->Say("I am unable to attack %s.", client_name);
	}
}

void QuestManager::save() {
	if (initiator && initiator->IsClient())
		initiator->Save();
}

void QuestManager::faction(int faction_id, int faction_value) {
	if (initiator && initiator->IsClient()) {
		if(faction_id != 0 && faction_value != 0) {
	// SCORPIOUS2K - fixed faction command
			//Client *p; 
			initiator->SetFactionLevel2(
				initiator->CharacterID(), 
				faction_id, 
				initiator->GetClass(), 
				initiator->GetRace(), 
				initiator->GetDeity(), 
				faction_value); 
			
		}
	}
}

void QuestManager::setsky(uint8 new_sky) {
	if (zone)
		zone->newzone_data.sky = new_sky;
	APPLAYER* outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
	memcpy(outapp->pBuffer, &zone->newzone_data, outapp->size);
	entity_list.QueueClients(initiator, outapp);
	safe_delete(outapp);
}

void QuestManager::setguild(int32 new_guild_id, int8 new_rank) {
	if (initiator && initiator->IsClient()){
		initiator->SetGuild(new_guild_id, new_rank);
	}
}

void QuestManager::settime(int8 new_hour, int8 new_min) {
	if (zone)
		zone->SetTime(new_hour, new_min);
}

void QuestManager::itemlink(int item_id) {
	//I dont think this is right anymore, need the hash
/*
uint32_t calc_hash (const char *string) 
{ 
    register hash = 0; 

    while (*string != '\0') 
    { 
        register c = toupper(*string); 

        asm volatile(" 
            imul $31, %1, %1; 
            movzx %%ax, %%edx; 
            addl %%edx, %1; 
            movl %1, %0; 
            " 
            :"=r"(hash) 
            :"D"(hash), "a"(c) 
            :"%edx" 
             ); 

        //This is what the inline asm is doing: 
        //hash *= 0x1f; 
        //hash += (int)c; 

        string++; 
    } 

    return hash; 
} 


Now the not so simple part, generating the string to feed into the hash function. 

The string for normal (unaugmented) items looks like this: 
Code: 
sprintf(hashstr, "%d%s%s%d %d %d %d %d %d %d %d", id, name, "-1-1-1-1-1", hp, mana, ac, light, icon, price, size, weight); 


The string for bags looks like this: 
Code: 
sprintf(hashstr, "%d%s%d%d%d%d", id, name, bagslots, bagwr, price, weight); 


The string for books looks like this: 
Code: 
sprintf(hashstr, "%d%s%d%d", id, name, weight, booktype); 
*/

// MYRA - added itemlink(ItemNumber) command
	const Item_Struct* item = 0; 
	int16 itemid = item_id; 
	item = database.GetItem(itemid); 
	initiator->Message(0, "%s tells you, '%c00%i %s%c",npc->GetName(),0x12, item->ItemNumber, item->Name, 0x12);
}

void QuestManager::signalwith(int npc_id, int signal_id, int wait_ms) {
// SCORPIOUS2K - signal command
	// signal(npcid) - generates EVENT_SIGNAL on specified npc
	if(wait_ms > 0) {
		STimerList.push_back(SignalTimer(wait_ms, npc_id, signal_id));
		return;
	}
	
	if (npc_id<1)
	{
		printf("signal() bad npcid=%i\n",npc_id);
	}
	else
	{
		//initiator* signalnpc=0;
		entity_list.SignalMobsByNPCID(npc_id, signal_id);
	}
}

void QuestManager::signal(int npc_id, int wait_ms) {
	signalwith(npc_id, 0, wait_ms);
}

void QuestManager::setglobal(const char *varname, const char *newvalue, int options, const char *duration) {
// SCORPIOUS2K - qglobal variable commands
	// setglobal(varname,value,options,duration)
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	//MYSQL_ROW row;
	int qgZoneid=zone->GetZoneID();
	int qgCharid=0;
	int qgNpcid = npc->GetID();	//this might be the wrong method...
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
	if (initiator && initiator->IsClient())  // some events like waypoint and spawn don't have a player involved
	{
		qgCharid=initiator->CharacterID();
	}

	else
	{
		qgCharid=-qgNpcid;		// make char id negative npc id as a fudge
	}
	if (options < 0 || options > 7)
	{
		cerr << "Invalid options for global var " << varname << " using defaults" << endl;
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
		,Timer::GetCurrentTime(),varname,qgNpcid,qgCharid,qgZoneid), errbuf, &result);
	if (query)
	{
		safe_delete_array(query);
		query=0;
	}
	mysql_free_result(result);
	//NOTE: this should be escaping the contents of arglist
	//npcwise a malicious script can arbitrarily alter the DB
	if (!database.RunQuery(query, MakeAnyLenString(&query, 
	  "INSERT INTO quest_globals (charid,npcid,zoneid,name,value,expdate) VALUES (%i,%i,%i,'%s','%s',unix_timestamp(now())+%i)",
	  qgCharid,qgNpcid,qgZoneid,varname,newvalue,
	  QGexpdate(varname,duration)
	  ), errbuf, &result)) 
	{
		cerr << "setglobal error inserting " << varname << " : " << errbuf << endl;
	}
	if (query)
	{
		safe_delete_array(query);
		query=0;
	}
	mysql_free_result(result);

}

void QuestManager::targlobal(const char *varname, const char *value, const char *duration, int qgNpcid, int qgCharid, int qgZoneid) {
	// targlobal(varname,value,duration,npcid,charid,zoneid)
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	//MYSQL_ROW row;
	// clean up expired vars and get rid of the one we're going to set if there
	database.RunQuery(query, MakeAnyLenString(&query, 
		"DELETE FROM quest_globals WHERE expdate < %i || (name='%s' && (charid=0 || (npcid=%i && charid=%i && zoneid=%i)))"
		,Timer::GetCurrentTime(),varname,qgNpcid,qgCharid,qgZoneid), errbuf, &result);
	if (query)
	{
		safe_delete_array(query);
		query=0;
	}
	mysql_free_result(result);
	if (!database.RunQuery(query, MakeAnyLenString(&query, 
	  "INSERT INTO quest_globals (charid,npcid,zoneid,name,value,expdate) VALUES (%i,%i,%i,'%s','%s',unix_timestamp(now())+%i)",
	  qgCharid,qgNpcid,qgZoneid,varname,value,
	  QGexpdate(varname,duration)
	  ), errbuf, &result)) 
	{
		cerr << "targlobal error inserting " << varname << " : " << errbuf << endl;
	}
	if (query)
	{
		safe_delete_array(query);
		query=0;
	}
	mysql_free_result(result);
}

void QuestManager::delglobal(const char *varname) {
	// delglobal(varname)
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	MYSQL_RES *result;
	//MYSQL_ROW row;
	int qgZoneid=zone->GetZoneID();
	int qgCharid=0;
	int qgNpcid=npc->GetID();
	if (initiator && initiator->IsClient())  // some events like waypoint and spawn don't have a player involved
	{
		qgCharid=initiator->CharacterID();
	}

	else
	{
		qgCharid=-qgNpcid;		// make char id negative npc id as a fudge
	}
	if (!database.RunQuery(query, 
	  MakeAnyLenString(&query, "DELETE FROM quest_globals WHERE name='%s' && (npcid=0 || charid=0 || zoneid=0 ||(npcid=%i && charid=%i && zoneid=%i))",
	  varname,qgNpcid,qgCharid,qgZoneid),errbuf, &result)) 
	{
		cerr << "delglobal error deleting " << varname << " : " << errbuf << endl;
	}
	if (query)
	{
		safe_delete_array(query);
		query=0;
	}
	mysql_free_result(result);
}

// SCORPIOUS2K - convert duration value to expdate
int32 QuestManager::QGexpdate(const char * name, const char * options)
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



void QuestManager::ding() {
	//-Cofruben:makes a sound.
	if (initiator && initiator->IsClient())
		initiator->SendSound();
	
}

void QuestManager::rebind(int zoneid, float x, float y, float z) {
	if(initiator && initiator->IsClient()) {
		initiator->SetBindPoint(zoneid, x, y, z);
	}
}

void QuestManager::start(int wp) {
	npc->AssignWaypoints(wp);
}

void QuestManager::stop() {
	npc->StopWandering();
}

void QuestManager::pause(int duration) {
	npc->PauseWandering(duration);
}

void QuestManager::moveto(float x, float y, float z) {
	npc->MoveTo(x, y, z);
}

void QuestManager::resume() {
	npc->ResumeWandering();
}

void QuestManager::addldonpoints(sint32 points, int32 theme) {
	if(initiator)
		initiator->UpdateLDoNPoints(points, theme);
}

void QuestManager::setnexthpevent(int at) {
	npc->SetNextHPEvent( at ); 
}

void QuestManager::respawn(int npc_type, int grid) {
	//char tempa[100];
	float x,y,z,h;
	if ( !npc ) 
		return;
	
	x = npc->GetX();
	y = npc->GetY();
	z = npc->GetZ();
	h = npc->GetHeading();
	depop_npc = true;
		
	const NPCType* tmp = 0;
	//int8 guildwarset = atoi(arglist[2]);
	if ((tmp = database.GetNPCType(npc_type))) 
	{
		npc = new NPC(tmp, 0, x, y, z, h);
		npc->AddLootTable();
		entity_list.AddNPC(npc,true,true);
		if(grid > 0)
			npc->AssignWaypoints(grid);

		npc->SendPosUpdate();
	}
}

void QuestManager::set_proximity(float minx, float maxx, float miny, float maxy, float minz, float maxz) {
	entity_list.AddProximity(npc);
	
	npc->proximity->min_x = minx;
	npc->proximity->max_x = maxx;
	npc->proximity->min_y = miny;
	npc->proximity->max_y = maxy;
	npc->proximity->min_z = minz;
	npc->proximity->max_z = maxz;
}

void QuestManager::clear_proximity() {
	safe_delete(npc->proximity);
	entity_list.RemoveProximity(npc->GetID());
}










