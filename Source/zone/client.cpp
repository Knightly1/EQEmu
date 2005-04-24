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
#include <iostream>
using namespace std;
#include <iomanip>
using namespace std;
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <math.h>

// Disgrace: for windows compile
#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#include <process.h>

#define snprintf	_snprintf
#define vsnprintf	_vsnprintf
#define strncasecmp	_strnicmp
#define strcasecmp  _stricmp
#else
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "../common/unix.h"
#endif

extern volatile bool RunLoops;
extern bool spells_loaded;

#include "features.h"
#include "masterentity.h"
#include "worldserver.h"
#include "net.h"
#include "../common/misc.h"
#include "../common/database.h"
#include "spdat.h"
#include "../common/packet_dump.h"
#include "../common/packet_functions.h"
#include "petitions.h"
#include "../common/serverinfo.h"
#include "../common/ZoneNumbers.h"
#include "../common/moremath.h"
#include "../common/guilds.h"
#include "forage.h"
#include "command.h"
#include "StringIDs.h"
#include "NpcAI.h"
#include "client_logs.h"

extern Database database;
extern EntityList entity_list;
extern Zone* zone;
extern volatile bool ZoneLoaded;
extern WorldServer worldserver;
extern GuildRanks_Struct guilds[512];
#ifndef NEW_LoadSPDat
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif
extern int32 numclients;
extern PetitionList petition_list;
bool commandlogged;
char entirecommand[255];

#ifdef GUILDWARS
#include "../GuildWars/GuildWars.h"
extern GuildWars guildwars;
#endif

#ifdef RAIDADDICTS
#include "RaidAddicts.h"
extern RaidAddicts raidaddicts;
#endif

Client::Client(EQStream* ieqs)
: Mob("No name",	// name
	"",	// lastname
	0,	// cur_hp
	0,	// max_hp
	0,	// gender
	0,	// race
	0,	// class
	BT_Client,	// bodytype
	0,	// deity
	0,	// level
	0,	// npctypeid
	0,	// skills
	0,	// size
	0.46,	// walkspeed
	0.7,	// runspeed
	0,	// heading
	0,	// x
	0,	// y
	0,	// z
	0,	// light
	NULL,	// equip
	0xFF,	// texture
	0xFF,	// helmtexture
	0,	// ac
	0,	// atk
	0,	// str
	0,	// sta
	0,	// dex
	0,	// agi
	0,	// int
	0,	// wis
	0,	// cha
	0xff,	// Luclin Hair Colour
	0xff,	// Luclin Beard Color
	0xff,	// Luclin Eye1
	0xff,	// Luclin Eye2
	0xff,	// Luclin Hair Style
// vesuvias - appearence fix
	0xff,	// Luclin Beard

	0xff,	// Luclin Face
	0xff,	// AA Title
	1, // fixedz
	0, // standart text1
	0, // standart text2
	0,	// see_invis
	0, // see_invis_undead 
	0	// qglobal

	),
	//these must be listed in the order they appear in client.h
	position_timer(250),
	hpupdate_timer(1800),
	camp_timer(29000),
	process_timer(100),
	disc_timer(60000),
	disc_elapse(60000),
	stamina_timer(40000),
	zoneinpacket_timer(3000),
	linkdead_timer(30000),
	dead_timer(2000),
	ooc_timer(1000),
	shield_timer(500),
	fishing_timer(8000),
#ifdef REVERSE_AGGRO
	scanarea_timer(AIClientScanarea_delay),
#endif
	tribute_timer(Tribute_duration),
#ifdef PACKET_UPDATE_MANAGER
	update_manager(ieqs),
#endif
	proximity_timer(ClientProximity_interval)
{
	for(int cf=0;cf<21;cf++)
		ClientFilters[cf]=0;
	character_id = 0;
	conn_state = NoPacketsReceived;
	client_data_loaded = false;
	feigned = false;
	berserk = false;
	dead = false;
	eqs = ieqs;
	ip = eqs->GetrIP();
	port = ntohs(eqs->GetrPort());
	client_state = CLIENT_CONNECTING;
	Trader=false;
	withcustomer=false;
	IsTracking=false;
	WID = 0;
	account_id = 0;
	admin = 0;
	lsaccountid = 0;
	shield_target = NULL;
	SQL_log = NULL;
	guilddbid = 0;
	guildeqid = GUILD_NONE;
	guildrank = 0;
	memset(lskey, 0, sizeof(lskey));
	strcpy(account_name, "");
	tellsoff = false;
	gmhideme = false;
	AFK = false;
	LFG = false;
	cheater = false;
	cheatcount =0;
	cheat_x=0;
	cheat_y=0;
	gmspeed = 0;
	playeraction = 0;
	target = 0;
	auto_attack = false;
	PendingGuildInvite = 0;
	linkdead_timer.Disable();
	zonesummon_x = -2;
	zonesummon_y = -2;
	zonesummon_z = -2;
	zonesummon_id = 0;
	zonesummon_ignorerestrictions = 0;
	zoning = false;
	zone_mode = ZoneUnsolicited;
	proximity_x = FLT_MAX;	//arbitrary large number
	proximity_y = FLT_MAX;
	proximity_z = FLT_MAX;
	casting_spell_id = 0;
	npcflag = false;
	npclevel = 0;
	pQueuedSaveWorkID = 0;
	position_timer_counter = 0;
	fishing_timer.Disable();
	shield_timer.Disable();
	dead_timer.Disable();
	camp_timer.Disable();
	instalog = false;
	pLastUpdate = 0;
	pLastUpdateWZ = 0;
	auto_split = false;
	// Kaiyodo - initialise haste variable
	m_tradeskill_object = NULL;
	IsSettingGuildDoor = false;
	delaytimer = false;
	pendingrezzexp = 0;
	numclients++;
	// emuerror;
	UpdateWindowTitle();
	horseId = 0;
	tgb = false;
	AbilityTimer=false;
	tribute_master_id = 0xFFFFFFFF;
	tribute_timer.Disable();
	
	disc_timer.Disable();
	disc_elapse.Disable();
	disc_inuse = discNone;
	pr = new PRange_Struct;
	pr->p1set=false;
	pr->p2set=false;
	pr->p3set=false;
	pr->p4set=false;
}

Client::~Client() {
	entity_list.RemoveFromTargets(this);
	Mob* horse = entity_list.GetMob(this->CastToClient()->GetHorseId());
	if (horse)
		horse->Depop();
	if(Trader)
		database.DeleteTraderItem(this->CharacterID());
	Object* object=GetTradeskillObject();
	if(object)
		object->Close();
	
	if(conn_state != ClientConnectFinished) {
		LogFile->write(EQEMuLog::Debug, "Client '%s' was destroyed before reaching the connected state:", GetName());
		ReportConnectingState();
	}
	
#ifdef CLIENT_LOGS
	client_logs.unsubscribeAll(this);
#endif
	
//	if(AbilityTimer || GetLevel()>=51)
//		database.UpdateAndDeleteAATimers(CharacterID());
	
	ChangeSQLLog(NULL);
	if(IsDueling() && GetDuelTarget() != 0) {
		Entity* entity = entity_list.GetID(GetDuelTarget());
		if(entity != NULL && entity->IsClient()) {
			entity->CastToClient()->SetDueling(false);
			entity->CastToClient()->SetDuelTarget(0);
			entity_list.DuelMessage(entity->CastToClient(),this,true);
		}
	}
	
	if (shield_target) {
		for (int y = 0; y < 2; y++) {
			if (shield_target->shielder[y].shielder_id == GetID()) {
				shield_target->shielder[y].shielder_id = 0;
				shield_target->shielder[y].shielder_bonus = 0;
			}
		}
		shield_target = NULL;
	}
	
	//if we are in a group and we are not zoning, force leave the group
	if(isgrouped && !zoning)
		LeaveGroup();
	
	eqs->Free();
	UpdateWho(2);
	// we save right now, because the client might be zoning and the world
	// will need this data right away
	Save(2); // This fails when database destructor is called first on shutdown	
	safe_delete(pr);
	numclients--;
	UpdateWindowTitle();
#ifdef GUILDWARS
	guildwars.SetCurrentUsers(numclients);
#endif
	zone->RemoveAuth(GetName());
}

void Client::ReportConnectingState() {
	switch(conn_state) {
	case NoPacketsReceived:		//havent gotten anything
		LogFile->write(EQEMuLog::Debug, "Client has not sent us an initial zone entry packet.");
		break;
	case ReceivedZoneEntry:		//got the first packet, loading up PP
		LogFile->write(EQEMuLog::Debug, "Client sent initial zone packet, but we never got their player info from the database.");
		break;
	case PlayerProfileLoaded:	//our DB work is done, sending it
		LogFile->write(EQEMuLog::Debug, "We were sending the player profile, tributes, tasks, spawns, time and weather, but never finished.");
		break;
	case ZoneInfoSent:		//includes PP, tributes, tasks, spawns, time and weather
		LogFile->write(EQEMuLog::Debug, "We successfully sent player info and spawns, waiting for client to request new zone.");
		break;
	case NewZoneRequested:	//received and sent new zone request
		LogFile->write(EQEMuLog::Debug, "We received client's new zone request, waiting for client spawn request.");
		break;
	case ClientSpawnRequested:	//client sent ReqClientSpawn
		LogFile->write(EQEMuLog::Debug, "We received the client spawn request, and were sending objects, doors, zone points and some other stuff, but never finished.");
		break;
	case ZoneContentsSent:		//objects, doors, zone points
		LogFile->write(EQEMuLog::Debug, "The rest of the zone contents were successfully sent, waiting for client ready notification.");
		break;
	case ClientReadyReceived:	//client told us its ready, send them a bunch of crap like guild MOTD, etc
		LogFile->write(EQEMuLog::Debug, "We received client ready notification, but never finished Client::CompleteConnect");
		break;
	case ClientConnectFinished:	//client finally moved to finished state, were done here
		LogFile->write(EQEMuLog::Debug, "  Client is successfully connected.");
		break;
	};
}

bool Client::Save(int8 iCommitNow) {
#if 0
// Orig. Offset: 344 / 0x00000000
//       Length: 36 / 0x00000024
   unsigned char rawData[36] =
{
    0x0D, 0x30, 0xE1, 0x30, 0x1E, 0x10, 0x22, 0x10, 0x20, 0x10, 0x21, 0x10, 0x1C, 0x20, 0x1F, 0x10, 
    0x7C, 0x10, 0x68, 0x10, 0x51, 0x10, 0x78, 0x10, 0xBD, 0x10, 0xD2, 0x10, 0xCD, 0x10, 0xD1, 0x10, 
    0x01, 0x10, 0x6D, 0x10
} ;
	for (int tmp = 0;tmp <=35;tmp++){
		m_pp.unknown0256[89+tmp] = rawData[tmp];
	}
#endif

	if(!ClientDataLoaded())
		return false;
	_ZP(Client_Save);
	
	m_pp.x = x_pos;
	m_pp.y = y_pos;
	m_pp.z = z_pos;
	m_pp.guildrank=guildrank;
	m_pp.heading = heading;
	int spentpoints=0;
	for(int a=0;a < MAX_PP_AA_ARRAY;a++)
		if(aa[a])
			spentpoints += aa[a]->value;
	
	m_pp.aapoints_spent = spentpoints;
	if (GetHP() <= 0) {
		if (GetMaxHP() > 30000)
			m_pp.cur_hp = 30000;
		else
			m_pp.cur_hp = GetMaxHP();
	}
	else if (GetHP() > 30000)
		m_pp.cur_hp = 30000;
	else
		m_pp.cur_hp = GetHP();
	m_pp.mana = cur_mana;
		
	for (int i=0; i < BUFF_COUNT; i++) {
		if (buffs[i].spellid != SPELL_UNKNOWN) {
			m_pp.buffs[i].spellid = buffs[i].spellid;
// solar: fix this if buffs struct is fixed
			m_pp.buffs[i].slotid = i+1/*2*/;
			m_pp.buffs[i].duration = buffs[i].ticsremaining;
			m_pp.buffs[i].level = buffs[i].casterlevel;
			m_pp.buffs[i].effect = 10;
			m_pp.buffs[i].poisoncounters = buffs[i].poisoncounters;
			m_pp.buffs[i].diseasecounters = buffs[i].diseasecounters;
		}
		else {
			m_pp.buffs[i].spellid = SPELL_UNKNOWN;
			m_pp.buffs[i].duration = 0;
			m_pp.buffs[i].level = 0;
			m_pp.buffs[i].effect = 0;
			m_pp.buffs[i].poisoncounters = 0;
			m_pp.buffs[i].diseasecounters = 0;
		}
	}
	if (pQueuedSaveWorkID) {
		dbasync->CancelWork(pQueuedSaveWorkID);
		pQueuedSaveWorkID = 0;
	}

	if (GetPet() && !GetPet()->IsFamiliar() && GetPet()->CastToNPC()->GetPetSpellID() && !dead) {
		NPC *pet = GetPet()->CastToNPC();
		m_epp.pet_id = pet->CastToNPC()->GetPetSpellID();
		m_epp.pet_hp = pet->GetHP();
		m_epp.pet_mana = pet->GetMana();
		pet->GetPetState(m_epp.pet_buffs, m_epp.pet_items, m_epp.pet_name);
	} else {
		m_epp.pet_id = 0;
		m_epp.pet_hp = 0;
	}
	
	if(tribute_timer.Enabled()) {
		m_pp.tribute_time_remaining = tribute_timer.GetRemainingTime();
	} else {
		m_pp.tribute_time_remaining = 0xFFFFFFFF;
		m_pp.tribute_active = 0;
	}
	
	p_timers.Store();
	
//	printf("Dumping inventory on save:\n");
//	m_inv.dumpInventory();
	
	if (iCommitNow <= 1) {
		char* query = 0;
		uint32_breakdown workpt;
		workpt.b4() = DBA_b4_Entity;
		workpt.w2_3() = GetID();
		workpt.b1() = DBA_b1_Entity_Client_Save;
		DBAsyncWork* dbaw = new DBAsyncWork(MTdbafq, workpt, DBAsync::Write, 0xFFFFFFFF);
		dbaw->AddQuery(iCommitNow == 0 ? true : false, &query, database.SetPlayerProfile_MQ(&query, account_id, character_id, &m_pp, &m_inv, &m_epp), false);
		if (iCommitNow == 0){
			pQueuedSaveWorkID = dbasync->AddWork(&dbaw, 2500);
		}
		else {
			dbasync->AddWork(&dbaw, 0);
			SaveBackup();
		}
		safe_delete_array(query);
		return true;
	}
	else if (database.SetPlayerProfile(account_id, character_id, &m_pp, &m_inv, &m_epp)) {
		SaveBackup();
	}
	else {
		cerr << "Failed to update player profile" << endl;
		return false;
	}
	
	return true;
}

void Client::SaveBackup() {
	if (!RunLoops)
		return;
	char* query = 0;
	DBAsyncWork* dbaw = new DBAsyncWork(&DBAsyncCB_CharacterBackup, this->CharacterID(), DBAsync::Read);
	dbaw->AddQuery(0, &query, MakeAnyLenString(&query, "Select id, UNIX_TIMESTAMP()-UNIX_TIMESTAMP(ts) as age from character_backup where charid=%u and backupreason=0 order by ts asc", this->CharacterID()), true);
	dbasync->AddWork(&dbaw, 0);
}

CLIENTPACKET::CLIENTPACKET()
{
    app = NULL;
    ack_req = false;
}

CLIENTPACKET::~CLIENTPACKET()
{
    safe_delete(app);
}

//this assumes we do not own pApp, and clones it.
bool Client::AddPacket(const EQApplicationPacket *pApp, bool bAckreq) {
	if (!pApp)
		return false;
	if(!zoneinpacket_timer.Enabled()) {
		//drop the packet because it will never get sent.
		return(false);
	}
    CLIENTPACKET *c = new CLIENTPACKET;

    c->ack_req = bAckreq;
    c->app = pApp->Copy();
    
    clientpackets.Append(c);
    return true;
}

//this assumes that it owns the object pointed to by *pApp
bool Client::AddPacket(EQApplicationPacket** pApp, bool bAckreq) {
	if (!pApp || !(*pApp))
		return false;
	if(!zoneinpacket_timer.Enabled()) {
		//drop the packet because it will never get sent.
		return(false);
	}
    CLIENTPACKET *c = new CLIENTPACKET;
	
    c->ack_req = bAckreq;
    c->app = *pApp;
	*pApp = 0;
	
    clientpackets.Append(c);
    return true;
}

bool Client::SendAllPackets() {
	LinkedListIterator<CLIENTPACKET*> iterator(clientpackets);
	
	CLIENTPACKET* cp = 0;
	iterator.Reset();
	while(iterator.MoreElements()) {
		cp = iterator.GetData();
		if(eqs)
			eqs->FastQueuePacket(&cp->app, cp->ack_req);
		iterator.RemoveCurrent();
#if EQDEBUG >= 6
		LogFile->write(EQEMuLog::Normal, "Transmitting a packet");
#endif
	}
	return true;
}

void Client::QueuePacket(const EQApplicationPacket* app, bool ack_req, CLIENT_CONN_STATUS required_state,int8 filter) {
/*	if (app->opcode==0x9999) {
		cout << "Sending an unknown opcode from: " << endl;
		print_stacktrace();
	}
	if (app->opcode==OP_SkillUpdate) {
		cout << "Sending OP_SkillUpdate from: " << endl;
		print_stacktrace();
	}
*/
	_ZP(Client_QueuePacket);
	if(filter!=0){
		if(GetFilter(filter)==0)
			return; //Client has this filter on, no need to send packet
	}
	if(client_state != CLIENT_CONNECTED && required_state == CLIENT_CONNECTED){
		AddPacket(app, ack_req);
		return;
	}
	//#ifdef EQDEBUG >= 9
		// This just here while figuring out new opcodes/packets
		#ifdef MERTHALICIOUS
			//@merth: this just here temporarily for my debugging
			cout << "Sending: 0x" << hex << setw(4) << setfill('0') << app->GetOpcode() << dec << ", size=" << app->size << endl;
		#endif
	//#endif
	
	// if the program doesnt care about the status or if the status isnt what we requested
    if (required_state != CLIENT_CONNECTINGALL && client_state != required_state)
    {
        // todo: save packets for later use
        AddPacket(app, ack_req);
//        LogFile->write(EQEMuLog::Normal, "Adding Packet to list (%d) (%d)", app->GetOpcode(), (int)required_state);
    }
    else
	    if(eqs)
            eqs->QueuePacket(app, ack_req);
}

void Client::FastQueuePacket(EQApplicationPacket** app, bool ack_req, CLIENT_CONN_STATUS required_state) {
	
	//cout << "Sending: 0x" << hex << setw(4) << setfill('0') << (*app)->GetOpcode() << dec << ", size=" << (*app)->size << endl;
	
	// if the program doesnt care about the status or if the status isnt what we requested
    if (required_state != CLIENT_CONNECTINGALL && client_state != required_state) {
        // todo: save packets for later use
        AddPacket(app, ack_req);
//        LogFile->write(EQEMuLog::Normal, "Adding Packet to list (%d) (%d)", (*app)->GetOpcode(), (int)required_state);
		return;
    }
    else {
	    if(eqs)
            eqs->FastQueuePacket(app, ack_req);
		else if (app && (*app))
			delete *app;
		*app = 0;
	}
	return;
}

void Client::ChannelMessageReceived(int8 chan_num, int8 language, const char* message, const char* targetname) {
	#if EQDEBUG >= 11
		LogFile->write(EQEMuLog::Debug,"Client::ChannelMessageReceived() Channel:%i message:'%s'", chan_num, message);
	#endif
	
	if (targetname == NULL) {
		targetname = (target==NULL) ? NULL : target->GetName();
	}
	
	switch(chan_num)
	{
	case 0: { // GuildChat
		if (guilddbid == 0 || guildeqid >= 512)
			Message(0, "Error: You arent in a guild.");
		else if (!guilds[guildeqid].rank[guildrank].speakgu)
			Message(0, "Error: You dont have permission to speak to the guild.");
		else if (!worldserver.SendChannelMessage(this, targetname, chan_num, guilddbid, language, message))
			Message(0, "Error: World server disconnected");
		break;
	}
	case 2: { // GroupChat
		Group* group = entity_list.GetGroupByMob(this);
		if (this->isgrouped && group != NULL) {
			group->GroupMessage(this,(const char*) message);
		}
		break;
	}
	case 3: // Shout
	case 4: { // Auction
		
		Mob *sender = this;
		if (GetPet() && GetPet()->FindType(SE_VoiceGraft))
			sender = GetPet();
		
		entity_list.ChannelMessage(sender, chan_num, language, message);
		break;
	}
	case 5: { // OOC
		if(!ooc_timer.Check())
		{
			if(strlen(targetname)==0)
			ChannelMessageReceived(5, language, message,"discard"); //Fast typer or spammer??
			else
			return;
		}
		if(worldserver.oocmuted && admin < 100)
		{
			Message(0,"OOC has been muted.  Try again later.");
			return;
		}
		if(GetRevoked())
		{
			Message(0, "You have been revoked.  You may not talk on OOC.");
			return;
		}
		else if (!worldserver.SendChannelMessage(this, 0, 5, 0, language, message))
			Message(0, "Error: World server disconnected");
		break;
	}
	case 6: // Broadcast
	case 11: { // GMSay
		if (!(admin >= 80))
			Message(0, "Error: Only GMs can use this channel");
		else if (!worldserver.SendChannelMessage(this, targetname, chan_num, 0, language, message))
			Message(0, "Error: World server disconnected");
		break;
	}
	case 7: { // Tell
		if(!worldserver.SendChannelMessage(this, targetname, chan_num, 0, language, message))
			Message(0, "Error: World server disconnected");
		break;
	}
	case 8: { // /say
		if(message[0] == COMMAND_CHAR)  {
			command_dispatch(this, message);
			break;
		}
		Mob* sender = this;
		if (GetPet() && GetPet()->FindType(SE_VoiceGraft))
			sender = GetPet();
		
		printf("Message: %s\n",message);
		entity_list.ChannelMessage(sender, chan_num, language, message);
		
		if (sender != this)
			break;
		
		if (target != 0 && target->IsNPC() && !target->CastToNPC()->IsEngaged()) {
			if (DistNoRootNoZ(*target) <= 200) {
				if(target->CastToNPC()->IsMoving() && !target->CastToNPC()->IsOnHatelist(target))
					target->CastToNPC()->PauseWandering(5);
				parse->Event(EVENT_SAY, target->GetNPCTypeID(), message, target->CastToNPC(), this);
			#ifdef IPC
                if(target->CastToNPC()->IsInteractive()) {
					target->CastToNPC()->InteractiveChat(chan_num,language,message,targetname,this);
				}
			#endif
				//parse->Event(EVENT_SAY, target->GetNPCTypeID(), message, target->CastToNPC(), this);
			}
		}
		break;
	}
	default: {
		Message(0, "Channel (%i) not implemented",(int16)chan_num);
	}
	}
}

void Client::ChannelMessageSend(const char* from, const char* to, int8 chan_num, int8 language, const char* message, ...) {
	if ((chan_num==11 && !(this->GetGM())) || (chan_num==10 && this->Admin()<80)) // dont need to send /pr & /petition to everybody
		return;
	va_list argptr;
	char buffer[4096];

	va_start(argptr, message);
	vsnprintf(buffer, 4096, message, argptr);
	va_end(argptr);

	EQApplicationPacket app(OP_ChannelMessage, sizeof(ChannelMessage_Struct)+strlen(buffer)+1);
	ChannelMessage_Struct* cm = (ChannelMessage_Struct*)app.pBuffer;

	if (from == 0)
		strcpy(cm->sender, "ZServer");
	else if (from[0] == 0)
		strcpy(cm->sender, "ZServer");
	else
		strcpy(cm->sender, from);
	if (to != 0)
		strcpy((char *) cm->targetname, to);
	else if (chan_num == 7)
		strcpy(cm->targetname, m_pp.name);
	else
		cm->targetname[0] = 0;
	if (language < MAX_PP_LANGUAGE) {
		cm->skill_in_language = m_pp.languages[language];
		cm->language = language;
	}
	else {
		cm->skill_in_language = 100;
		cm->language = 0;
	}

	cm->chan_num = chan_num;
	strcpy(&cm->message[0], buffer);
	QueuePacket(&app);
}

void Client::Message(uint32 type, const char* message, ...) {
	va_list argptr;
	char buffer[4096];
	
	if (GetFilter(16) == 0 && type == MT_NonMelee)
		return;
	if (GetFilter(15) == 0 && type == MT_CritMelee) //98 is self...
		return;
	if (GetFilter(14) == 0 && type == MT_SpellCrits)
		return;
	
	va_start(argptr, message);
	vsnprintf(buffer, sizeof(buffer), message, argptr);
	va_end(argptr);
	
	// @merth: haven't figured out the packet entirely.. sending 4096k packet
	// for now to ensure client clears buffer properly
	uint32 len_packet = sizeof(buffer);
	uint32 len_buf = strlen(buffer)+1;
	if (len_buf > len_packet) {
		LogFile->write(EQEMuLog::Debug, "Client::Message() - OP_SpecialMesg needs work (%s)", buffer);
		len_packet = sizeof(SpecialMesg_Struct)+strlen(buffer)+1;
	}
	
	//uint32 len_packet = sizeof(SpecialMesg_Struct)+strlen(buffer)+1;
	EQApplicationPacket* app = new EQApplicationPacket(OP_SpecialMesg, len_packet);
	SpecialMesg_Struct* sm=(SpecialMesg_Struct*)app->pBuffer;
	sm->header[0] = 0x04; // Header used for #emote style messages..
	sm->header[1] = 0x04; // Play around with these to see other types
	sm->header[2] = 0x00;
	sm->msg_type = type;
	sm->target_spawn_id = this->GetID();
	memcpy(sm->message, buffer, strlen(buffer));
	
	QueuePacket(app);
	safe_delete(app);
}

void Client::SetMaxHP() {
	if(dead)
		return;
	SetHP(CalcMaxHP());
	SendHPUpdate();
	Save();
}

#ifndef GUILDWARS

bool Client::UpdateLDoNPoints(sint32 points, int32 theme)
{
/*
// make sure total stays in sync with individual buckets
	m_pp.ldon_available_points = m_pp.ldon_guk_points
		+m_pp.ldon_mirugal_points
		+m_pp.ldon_mistmoore_points
		+m_pp.ldon_rujarkian_points
		+m_pp.ldon_takish_points;

	if(points < 0)
	{
		if(m_pp.ldon_available_points < ((uint32)points*-1))
			return false;
	}
	switch(theme)
	{
	// handle generic points (theme=0)
	case 0:
		{	// no theme, so distribute evenly across all
			int splitpts=points/5;
			int gukpts=splitpts+(points%5);
			int mirpts=splitpts;
			int mmcpts=splitpts;
			int rujpts=splitpts;
			int takpts=splitpts;

			splitpts=0;

			if(points < 0)
			{
				if(m_pp.ldon_available_points < (uint32)(0-points))
				{
					return false;
				}				
				if(m_pp.ldon_guk_points < (uint32)(0-gukpts))
				{
					mirpts+=gukpts+m_pp.ldon_guk_points;
					gukpts=0-m_pp.ldon_guk_points;
				}
				if(m_pp.ldon_mirugal_points < (uint32)(0-mirpts))
				{
					mmcpts+=mirpts+m_pp.ldon_mirugal_points;
					mirpts=0-m_pp.ldon_mirugal_points;
				}
				if(m_pp.ldon_mistmoore_points < (uint32)(0-mmcpts))
				{
					rujpts+=mmcpts+m_pp.ldon_mistmoore_points;
					mmcpts=0-m_pp.ldon_mistmoore_points;
				}
				if(m_pp.ldon_rujarkian_points < (uint32)(0-rujpts))
				{
					takpts+=rujpts+m_pp.ldon_rujarkian_points;
					rujpts=0-m_pp.ldon_rujarkian_points;
				}
				if(m_pp.ldon_takish_points < (uint32)(0-takpts))
				{
					splitpts=takpts+m_pp.ldon_takish_points;
					takpts=0-m_pp.ldon_takish_points;
				}
			}
			m_pp.ldon_guk_points += gukpts;
			m_pp.ldon_mirugal_points+=mirpts;
			m_pp.ldon_mistmoore_points += mmcpts;
			m_pp.ldon_rujarkian_points += rujpts;
			m_pp.ldon_takish_points += takpts;
			points-=splitpts;
		// if anything left, recursively loop thru again
			if (splitpts !=0)
				UpdateLDoNPoints(splitpts,0);
			break;
		}
	case 1:
		{
			if(points < 0)
			{
				if(m_pp.ldon_guk_points < (uint32)(0-points))
					return false;
			}
			m_pp.ldon_guk_points += points;
			break;
		}
	case 2:
		{
			if(points < 0)
			{
				if(m_pp.ldon_mirugal_points < (uint32)(0-points))
					return false;
			}
			m_pp.ldon_mirugal_points += points;
			break;
		}
	case 3:
		{
			if(points < 0)
			{
				if(m_pp.ldon_mistmoore_points < (uint32)(0-points))
					return false;
			}
			m_pp.ldon_mistmoore_points += points;
			break;
		}
	case 4:
		{
			if(points < 0)
			{
				if(m_pp.ldon_rujarkian_points < (uint32)(0-points))
					return false;
			}
			m_pp.ldon_rujarkian_points += points;
			break;
		}
	case 5:
		{
			if(points < 0)
			{
				if(m_pp.ldon_takish_points < (uint32)(0-points))
					return false;
			}
			m_pp.ldon_takish_points += points;
			break;
		}
	}
	m_pp.ldon_available_points += points;
#ifdef RAIDADDICTS
	raidaddicts.UpdateRAPoints(points, theme, this);
#endif
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_AdventurePointsUpdate, sizeof(AdventurePoints_Update_Struct));
	AdventurePoints_Update_Struct* apus = (AdventurePoints_Update_Struct*)outapp->pBuffer;
	apus->ldon_available_points = m_pp.ldon_available_points;
	apus->ldon_guk_points = m_pp.ldon_guk_points;
	apus->ldon_mirugal_points = m_pp.ldon_mirugal_points;
	apus->ldon_mistmoore_points = m_pp.ldon_mistmoore_points;
	apus->ldon_rujarkian_points = m_pp.ldon_rujarkian_points;
	apus->ldon_takish_points = m_pp.ldon_takish_points;
	outapp->priority = 6;
	QueuePacket(outapp);
	safe_delete(outapp);
	return true;
*/
	return(false);
}
#endif


void Client::SetSkill(int skillid, int8 value) {
	if (skillid > HIGHEST_SKILL)
		return;
	m_pp.skills[skillid] = value; // We need to be able to #setskill 254 and 255 to reset skills

	if(value <= 252) {
		EQApplicationPacket* outapp = new EQApplicationPacket(OP_SkillUpdate, sizeof(SkillUpdate_Struct));
		SkillUpdate_Struct* skill = (SkillUpdate_Struct*)outapp->pBuffer;
		skill->skillId=skillid;
		skill->value=value;
		QueuePacket(outapp);
		safe_delete(outapp);
	}
}

void Client::AddSkill(int skillid, int8 value) {
	if (skillid > HIGHEST_SKILL)
		return;
	value = GetRawSkill(skillid) + value;
	if (value > 252)
		value = 252;
	SetSkill(skillid, value);
}

void Client::SendSound(){//-Cofruben:Makes a sound.
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_Sound, 68);
	unsigned char x[68];
	memset(x, 0, 68);
	x[0]=0x22;
	memset(&x[4],0x8002,sizeof(int16));
	memset(&x[8],0x8624,sizeof(int16));
	memset(&x[12],0x4A01,sizeof(int16));
	x[16]=0x05;
	x[28]=0x00;//change this value to give gold to the client
    memset(&x[40],0xFFFFFFFF,sizeof(int32));
    memset(&x[44],0xFFFFFFFF,sizeof(int32));
    memset(&x[48],0xFFFFFFFF,sizeof(int32));
    memset(&x[52],0xFFFFFFFF,sizeof(int32));
    memset(&x[56],0xFFFFFFFF,sizeof(int32));
    memset(&x[60],0xFFFFFFFF,sizeof(int32));
    memset(&x[64],0xffffffff,sizeof(int32));
	memcpy(outapp->pBuffer,x,outapp->size);
	QueuePacket(outapp);
	DumpPacket(outapp);
	safe_delete(outapp);

}

void Client::UpdateWho(int8 remove) {
	if (account_id == 0)
		return;
	if (!worldserver.Connected())
		return;
	ServerPacket* pack = new ServerPacket(ServerOP_ClientList, sizeof(ServerClientList_Struct));
	ServerClientList_Struct* scl = (ServerClientList_Struct*) pack->pBuffer;
	scl->remove = remove;
	scl->wid = this->GetWID();
	scl->IP = this->GetIP();
	scl->charid = this->CharacterID();
	strcpy(scl->name, this->GetName());

	scl->gm = GetGM();
	scl->Admin = this->Admin();
	scl->AccountID = this->AccountID();
	strcpy(scl->AccountName, this->AccountName());
	scl->LSAccountID = this->LSAccountID();
	strn0cpy(scl->lskey, lskey, sizeof(scl->lskey));
	scl->zone = zone->GetZoneID();
	scl->race = this->GetRace();
	scl->class_ = GetClass();
	scl->level = GetLevel();
	if (m_pp.anon == 0)
		scl->anon = 0;
	else if (m_pp.anon == 1)
		scl->anon = 1;
	else if (m_pp.anon >= 2)
		scl->anon = 2;

	scl->tellsoff = tellsoff;
	scl->guilddbid = guilddbid;
	scl->guildeqid = guildeqid;
	scl->LFG = LFG;

	worldserver.SendPacket(pack);
	safe_delete(pack);
}

void Client::WhoAll(Who_All_Struct* whom) {
	if (!worldserver.Connected())
		Message(0, "Error: World server disconnected");
	else {
		ServerPacket* pack = new ServerPacket(ServerOP_Who, sizeof(ServerWhoAll_Struct));
		ServerWhoAll_Struct* whoall = (ServerWhoAll_Struct*) pack->pBuffer;
		whoall->admin = (int8) admin;
		whoall->fromid=this->GetID();
		strcpy(whoall->from, this->GetName());
		strcpy(whoall->whom, whom->whom);
		whoall->lvllow = whom->lvllow;
		whoall->lvlhigh = whom->lvlhigh;
		whoall->gmlookup = whom->gmlookup;
		whoall->wclass = whom->wclass;
		whoall->wrace = whom->wrace;
		worldserver.SendPacket(pack);
		safe_delete(pack);
	}
}

void Client::UpdateAdmin(bool iFromDB) {
	sint16 tmp = admin;
	if (iFromDB)
		admin = database.CheckStatus(account_id);
	if (tmp == admin && iFromDB)
		return;

	if(m_pp.gm)
	{
#if EQDEBUG >= 5
		printf("%s is a GM\n", GetName());
#endif
// solar: no need for this, having it set in pp you already start as gm
// and it's also set in your spawn packet so other people see it too
//		SendAppearancePacket(AT_GM, 1, false);
		petition_list.UpdateGMQueue();
	}

	UpdateWho();
}

void Client::SetStats(int8 type,sint16 increase_val){
	if(type>STAT_DISEASE){
		printf("Error in Client::SetStats, received invalid type of: %i\n",type); 
		return;
	}
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_IncreaseStats,sizeof(IncreaseStat_Struct));
	IncreaseStat_Struct* iss=(IncreaseStat_Struct*)outapp->pBuffer;
	switch(type){
		case STAT_STR:
			if(increase_val>0)
				iss->str=increase_val;
			if((m_pp.STR+increase_val*2)<0)
				m_pp.STR=0;
			else if((m_pp.STR+increase_val*2)>255)
				m_pp.STR=255;
			else
				m_pp.STR+=increase_val*2;
			break;
		case STAT_STA:
			if(increase_val>0)
				iss->sta=increase_val;
			if((m_pp.STA+increase_val*2)<0)
				m_pp.STA=0;
			else if((m_pp.STA+increase_val*2)>255)
				m_pp.STA=255;
			else
				m_pp.STA+=increase_val*2;
			break;
		case STAT_AGI:
			if(increase_val>0)
				iss->agi=increase_val;
			if((m_pp.AGI+increase_val*2)<0)
				m_pp.AGI=0;
			else if((m_pp.AGI+increase_val*2)>255)
				m_pp.AGI=255;
			else
				m_pp.AGI+=increase_val*2;
			break;
		case STAT_DEX:
			if(increase_val>0)
				iss->dex=increase_val;
			if((m_pp.DEX+increase_val*2)<0)
				m_pp.DEX=0;
			else if((m_pp.DEX+increase_val*2)>255)
				m_pp.DEX=255;
			else
				m_pp.DEX+=increase_val*2;
			break;
		case STAT_INT:
			if(increase_val>0)
				iss->int_=increase_val;
			if((m_pp.INT+increase_val*2)<0)
				m_pp.INT=0;
			else if((m_pp.INT+increase_val*2)>255)
				m_pp.INT=255;
			else
				m_pp.INT+=increase_val*2;
			break;
		case STAT_WIS:
			if(increase_val>0)
				iss->wis=increase_val;
			if((m_pp.WIS+increase_val*2)<0)
				m_pp.WIS=0;
			else if((m_pp.WIS+increase_val*2)>255)
				m_pp.WIS=255;
			else
				m_pp.WIS+=increase_val*2;
			break;
		case STAT_CHA:
			if(increase_val>0)
				iss->cha=increase_val;
			if((m_pp.CHA+increase_val*2)<0)
				m_pp.CHA=0;
			else if((m_pp.CHA+increase_val*2)>255)
				m_pp.CHA=255;
			else
				m_pp.CHA+=increase_val*2;
			break;
	}
	QueuePacket(outapp);
	safe_delete(outapp);
}

const sint32& Client::SetMana(sint32 amount) {
	bool update = false;
	if (amount < 0)
		amount = 0;
	if (amount > GetMaxMana())
		amount = GetMaxMana();
	if (amount != cur_mana)
		update = true;
	cur_mana = amount;
	if (update)
		Mob::SetMana(amount);
	SendManaUpdatePacket();
	return cur_mana;
}

void Client::SendManaUpdatePacket() {
	if (!Connected() || IsCasting())
		return;
	//cout << "Sending mana update: " << (cur_mana - last_reported_mana) << endl;
	if (last_reported_mana != cur_mana) {
		
		EQApplicationPacket* outapp = new EQApplicationPacket(OP_ManaChange, sizeof(ManaChange_Struct));
		ManaChange_Struct* manachange = (ManaChange_Struct*)outapp->pBuffer;
		manachange->new_mana = cur_mana;
		manachange->stamina = 6000;
		manachange->spell_id = casting_spell_id;
		outapp->priority = 6;
		QueuePacket(outapp);
		safe_delete(outapp);

		last_reported_mana=cur_mana;
	}
}

void Client::FillSpawnStruct(NewSpawn_Struct* ns, Mob* ForWho)
{
	Mob::FillSpawnStruct(ns, ForWho);
	
	// Populate client-specific spawn information
	ns->spawn.afk		= AFK;
	ns->spawn.lfg		= LFG; // @bp: afk and lfg are cleared on zoneing on live
	ns->spawn.anon		= m_pp.anon;
	ns->spawn.gm		= GetGM() ? 1 : 0;
	ns->spawn.guild_id	= GuildEQID();
	ns->spawn.linkdead	= IsLD() ? 1 : 0;
	ns->spawn.aa_title	= aa_title;
	ns->spawn.pvp		= GetPVP() ? 1 : 0;
	
	strncpy(ns->spawn.title, m_pp.title, 64);
	
	if (IsBecomeNPC() == true)
		ns->spawn.npc = true;
	else if (ForWho == this)
		ns->spawn.npc = 10;
	else
		ns->spawn.npc = 0;
	
	if (guildeqid == GUILD_NONE) {
		ns->spawn.guild_rank = 0xFF;
	}
	else {
		if (guilds[guildeqid].rank[guildrank].warpeace || guilds[guildeqid].leader == account_id)
			ns->spawn.guild_rank = 2;
		else if (guilds[guildeqid].rank[guildrank].invite || guilds[guildeqid].rank[guildrank].remove || guilds[guildeqid].rank[guildrank].motd)
			ns->spawn.guild_rank = 1;
		else
			ns->spawn.guild_rank = 0;
	}
	ns->spawn.size			= 0; // Changing size works, but then movement stops! (wth?)
	ns->spawn.runspeed		= (gmspeed == 0) ? runspeed : 3.125f;
	ns->spawn.walkspeed		= 0.46000001f;

	// @merth: pp also hold this info; should we pull from there or inventory?
	// (update: i think pp should do it, as this holds LoY dye - plus, this is ugly code with Inventory!)
	const Item_Struct* item = NULL;
	const ItemInst* inst = NULL;
	if ((inst = m_inv[SLOT_HANDS]) && inst->IsType(ItemClassCommon)) {
		item = inst->GetItem();
		ns->spawn.equipment[MATERIAL_HANDS]	= item->Common.Material;
		ns->spawn.dye_rgb[MATERIAL_HANDS].color	= item->Common.Color;
	}
	if ((inst = m_inv[SLOT_HEAD]) && inst->IsType(ItemClassCommon)) {
		item = inst->GetItem();
		ns->spawn.equipment[MATERIAL_HEAD]	= item->Common.Material;
		ns->spawn.dye_rgb[MATERIAL_HEAD].color	= item->Common.Color;
	}
	if ((inst = m_inv[SLOT_ARMS]) && inst->IsType(ItemClassCommon)) {
		item = inst->GetItem();
		ns->spawn.equipment[MATERIAL_ARMS]	= item->Common.Material;
		ns->spawn.dye_rgb[MATERIAL_ARMS].color	= item->Common.Color;
	}
	if ((inst = m_inv[SLOT_BRACER01]) && inst->IsType(ItemClassCommon)) {
		item = inst->GetItem();
		ns->spawn.equipment[MATERIAL_BRACER]= item->Common.Material;
		ns->spawn.dye_rgb[MATERIAL_BRACER].color	= item->Common.Color;
	}
	if ((inst = m_inv[SLOT_BRACER02]) && inst->IsType(ItemClassCommon)) {
		item = inst->GetItem();
		ns->spawn.equipment[MATERIAL_BRACER]= item->Common.Material;
		ns->spawn.dye_rgb[MATERIAL_BRACER].color	= item->Common.Color;
	}
	if ((inst = m_inv[SLOT_CHEST]) && inst->IsType(ItemClassCommon)) {
		item = inst->GetItem();
		ns->spawn.equipment[MATERIAL_CHEST]	= item->Common.Material;
		ns->spawn.dye_rgb[MATERIAL_CHEST].color	= item->Common.Color;
	}
	if ((inst = m_inv[SLOT_LEGS]) && inst->IsType(ItemClassCommon)) {
		item = inst->GetItem();
		ns->spawn.equipment[MATERIAL_LEGS]	= item->Common.Material;
		ns->spawn.dye_rgb[MATERIAL_LEGS].color	= item->Common.Color;
	}
	if ((inst = m_inv[SLOT_FEET]) && inst->IsType(ItemClassCommon)) {
		item = inst->GetItem();
		ns->spawn.equipment[MATERIAL_FEET]	= item->Common.Material;
		ns->spawn.dye_rgb[MATERIAL_FEET].color	= item->Common.Color;
	}
	if ((inst = m_inv[SLOT_PRIMARY]) && inst->IsType(ItemClassCommon)) {
		item = inst->GetItem();
		if (strlen(item->IDFile) > 2)
			ns->spawn.equipment[MATERIAL_PRIMARY] = atoi(&item->IDFile[2]);
	}
	if ((inst = m_inv[SLOT_SECONDARY]) && inst->IsType(ItemClassCommon)) {
		item = inst->GetItem();
		if (strlen(item->IDFile) > 2)
			ns->spawn.equipment[MATERIAL_SECONDARY] = atoi(&item->IDFile[2]);
	}
	
	// @merth: these two may be related to ns->spawn.equip_chest2
	/*
	ns->spawn.npc_armor_graphic = texture;
	ns->spawn.npc_helm_graphic = helmtexture;
	*/
}

bool Client::GMHideMe(Client* client) {
	if (gmhideme) {
		if (client == 0)
			return true;
		else if (admin > client->Admin())
			return true;
		else
			return false;
	}
	else
		return false;
}

void Client::Duck() {
	SetAppearance(2, false);
}

void Client::Stand() {
	SetAppearance(0, false);
}

void Client::ChangeLastName(const char* in_lastname) {
	memset(m_pp.last_name, 0, sizeof(m_pp.last_name));
	if (strlen(in_lastname) >= sizeof(m_pp.last_name))
		strncpy(m_pp.last_name, in_lastname, sizeof(m_pp.last_name) - 1);
	else
		strcpy(m_pp.last_name, in_lastname);
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_GMLastName, sizeof(GMLastName_Struct));
	GMLastName_Struct* gmn = (GMLastName_Struct*)outapp->pBuffer;
	strcpy(gmn->name, name);
	strcpy(gmn->gmname, name);
	strcpy(gmn->lastname, in_lastname);
	gmn->unknown[0]=1;
	gmn->unknown[1]=1;
	gmn->unknown[2]=1;
	gmn->unknown[3]=1;
	entity_list.QueueClients(this, outapp, false);
	// Send name update packet here... once know what it is
	safe_delete(outapp);
}

bool Client::ChangeFirstName(const char* in_firstname, const char* gmname)
{
	// check duplicate name
	bool usedname = database.CheckUsedName((const char*) in_firstname);
	if (!usedname) {
		return false;
	}
	
	// update character_
	if(!database.UpdateName(GetName(), in_firstname))
		return false;
	
	// update pp
	memset(m_pp.name, 0, sizeof(m_pp.name));
	snprintf(m_pp.name, sizeof(m_pp.name), "%s", in_firstname);
	strcpy(name, m_pp.name);
	Save();
	
	// send name update packet
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_GMNameChange, sizeof(GMName_Struct));
	GMName_Struct* gmn=(GMName_Struct*)outapp->pBuffer;
	strncpy(gmn->gmname,gmname,64);
	strncpy(gmn->oldname,GetName(),64);
	strncpy(gmn->newname,in_firstname,64);
	gmn->unknown[0] = 1;
	gmn->unknown[1] = 1;
	gmn->unknown[2] = 1;
	entity_list.QueueClients(this, outapp, false);
	safe_delete(outapp);
	
	// finally, update the /who list
	UpdateWho();

	// success
	return true;
}

void Client::SetGM(bool toggle) {
	m_pp.gm = toggle ? 1 : 0;
	Message(13, "You are %s a GM.", m_pp.gm ? "now" : "no longer");
	SendAppearancePacket(AT_GM, m_pp.gm);
	Save();
	UpdateWho();
}

void Client::ReadBook(BookRequest_Struct *book) {
	char *txtfile = book->txtfile;
	
	if(txtfile[0] == '0' && txtfile[1] == '\0') {
		//invalid book... coming up on non-book items.
		return;
	}
	
	string booktxt2 = database.GetBook(txtfile);
	int length = booktxt2.length();
	
	if (booktxt2[0] != '\0') {
#if EQDEBUG >= 6
		LogFile->write(EQEMuLog::Normal,"Client::ReadBook() textfile:%s Text:%s", txtfile, booktxt2.c_str());
#endif
		EQApplicationPacket* outapp = new EQApplicationPacket(OP_ReadBook, length + 3);
		
		BookText_Struct *out = (BookText_Struct *) outapp->pBuffer;
		out->unknown0 = book->unknown0;
		out->type = book->type;
		memcpy(out->booktext, booktxt2.c_str(), length);
		
		QueuePacket(outapp);
		safe_delete(outapp);
	}
}

void Client::SendClientMoneyUpdate(int8 type,int32 amount){
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_TradeMoneyUpdate,sizeof(TradeMoneyUpdate_Struct));
	TradeMoneyUpdate_Struct* mus= (TradeMoneyUpdate_Struct*)outapp->pBuffer;
	mus->amount=amount;
	mus->trader=0;
	mus->type=type;
	QueuePacket(outapp);
	safe_delete(outapp);
}

bool Client::TakeMoneyFromPP(uint32 copper){
	sint32 copperpp,silver,gold,platinum;
	copperpp = m_pp.copper;
	silver = m_pp.silver*10;
	gold = m_pp.gold*100;
	platinum = m_pp.platinum*1000;
	
	sint32 clienttotal = m_pp.copper+m_pp.silver*10+m_pp.gold*100+m_pp.platinum*1000;
	clienttotal -= copper;
	if(clienttotal < 0)
	{
		return false; // Not enough money!
	}
	else
	{
		copperpp -= copper;
		if(copperpp <= 0)
		{
			copper = abs(copperpp);
			m_pp.copper = 0;
		}
		else
		{
			m_pp.copper = copperpp;
			Save();
			return true;
		}
		silver -= copper;
		if(silver <= 0)
		{
			copper = abs(silver);
			m_pp.silver = 0;
		}
		else
		{
			m_pp.silver = silver/10;
			m_pp.copper += (silver-(m_pp.silver*10));
			Save();
			return true;
		}
		
		gold -=copper;
		
		if(gold <= 0)
		{
			copper = abs(gold);
			m_pp.gold = 0;
		}
		else
		{
			m_pp.gold = gold/100;
			int32 silvertest = (gold-(m_pp.gold*100))/10;
			m_pp.silver += silvertest;
			int32 coppertest = (gold-(m_pp.gold*100+silvertest*10));
			m_pp.copper += coppertest;
			Save();
			return true;
		}
		
		platinum -= copper;
		
		//Impossible for plat to be negative, already checked above
		
		m_pp.platinum = platinum/1000;
		int32 goldtest = (platinum-(m_pp.platinum*1000))/100;
		m_pp.gold += goldtest;
		int32 silvertest = (platinum-(m_pp.platinum*1000+goldtest*100))/10;
		m_pp.silver += silvertest;
		int32 coppertest = (platinum-(m_pp.platinum*1000+goldtest*100+silvertest*10));
		m_pp.copper = coppertest;
		RecalcWeight();
		Save();
		return true;
	}
}

void Client::AddMoneyToPP(uint32 copper,bool updateclient){
	uint32 tmp;
	uint32 tmp2;
	tmp = copper;
	
	// Add Amount of Platinum
	tmp2 = tmp/1000;
	m_pp.platinum = m_pp.platinum + tmp2;
	tmp-=tmp2*1000;
	
	if (updateclient)
		SendClientMoneyUpdate(3,tmp2);
	
	// Add Amount of Gold
	tmp2 = tmp/100;
	m_pp.gold = m_pp.gold + tmp2;
	tmp-=tmp2*100;
	if (updateclient)
		SendClientMoneyUpdate(2,tmp2);
	
	// Add Amount of Silver
	tmp2 = tmp/10;
	tmp-=tmp2*10;
	m_pp.silver = m_pp.silver + tmp2;
	if (updateclient)
		SendClientMoneyUpdate(1,tmp2);
	
	// Add Copper
	//tmp	= tmp - (tmp2* 10);
	if (updateclient)
		SendClientMoneyUpdate(0,tmp);
	m_pp.copper = m_pp.copper + tmp;
	RecalcWeight();
	Save();
	LogFile->write(EQEMuLog::Debug, "Client::AddMoneyToPP() %s should have:  plat:%i gold:%i silver:%i copper:%i", GetName(), m_pp.platinum, m_pp.gold, m_pp.silver, m_pp.copper);
}

void Client::AddMoneyToPP(uint32 copper, uint32 silver, uint32 gold, uint32 platinum, bool updateclient){
	if ((updateclient) && platinum!=0)
		SendClientMoneyUpdate(3, platinum);
	if ((updateclient) && gold!=0)
		SendClientMoneyUpdate(2, gold);
	if ((updateclient) && silver!=0)
		SendClientMoneyUpdate(1, silver);
	if ((updateclient) && copper!=0)
		SendClientMoneyUpdate(0, copper);
	
	m_pp.platinum += platinum;
	m_pp.gold += gold;
	m_pp.silver += silver;
	m_pp.copper += + copper;
	
	RecalcWeight();
	Save();
	
#if (EQDEBUG>=5)
		LogFile->write(EQEMuLog::Debug, "Client::AddMoneyToPP() %s should have:  plat:%i gold:%i silver:%i copper:%i",
			GetName(), m_pp.platinum, m_pp.gold, m_pp.silver, m_pp.copper);
#endif
}

bool Client::CheckIncreaseSkill(int skillid, int chancemodi) {
	if (IsAIControlled()) // no skillups while chamred =p
		return false;
	if (skillid > HIGHEST_SKILL)
		return false;
	// Make sure we're not already at skill cap
	if (GetRawSkill(skillid) < MaxSkill(skillid))
	{
		// the higher your current skill level, the harder it is
		sint16 Chance = 10 + chancemodi + ((252 - GetRawSkill(skillid)) / 20);
		if (Chance < 1)
			Chance = 1; // Make it always possible
		if(((float)rand()/RAND_MAX)*100 < Chance)
		{
			SetSkill(skillid, GetRawSkill(skillid) + 1);
			return true;
		}
	}
	return false;
}

// Made this function to check for skill increases in skills where a linear algorithm for
// skill progression is acceptable.  For the time being using (10+level*5)
// solar: use CheckIncreaseSkill instead
/*
bool Client::SimpleCheckIncreaseSkill(int16 skillid,sint16 chancemodi){
	if (IsAIControlled()) // no skillups while chamred =p
		return false;
	if (skillid > HIGHEST_SKILL)
		return false;
	// Make sure we're not already at skill cap
	if (GetSkill(skillid) < (10+level*5) ){
		// the higher your current skill level, the harder it is
		sint16 Chance = 10 + chancemodi + ((252 - GetSkill(skillid)) / 20);
		if (Chance < 0)
			Chance = 0; // Make it always possible
		if (MakeRandomInt(0,100) < Chance){
			SetSkill(skillid,GetRawSkill(skillid)+1);
			return true;
		}
	}
	return false;
}
*/

#include "maxskill.h"
/*
int8 Mob::MaxSkill(int16 skillid, int16 class_, int16 level) {
	switch (skillid) {
		case OFFENSE:
		case DEFENSE: {
			int16 tmp = level * 5;
			if (tmp > 200)
				tmp = 200;
			return tmp;
		}
		default: {
			return level*4;
		}
	}
}
*/
void Client::SendLevelAppearance(){
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_LevelAppearance, sizeof(LevelAppearance_Struct));
	LevelAppearance_Struct* la = (LevelAppearance_Struct*)outapp->pBuffer;
	la->parm1 = 0x4D;
	la->parm2 = la->parm1 + 1;
	la->parm3 = la->parm2 + 1;
	la->parm4 = la->parm3 + 1;
	la->parm5 = la->parm4 + 1;
	la->spawn_id = GetID();
	la->value1a = 1;
	la->value2a = 2;
	la->value3a = 1;
	la->value3b = 1;
	la->value4a = 1;
	la->value4b = 1;
	la->value5a = 2;
	QueuePacket(outapp);
	safe_delete(outapp);
}

void Client::SetPVP(bool toggle) {
	m_pp.pvp = toggle ? 1 : 0;

	if(GetPVP())
		this->Message_StringID(13,PVP_ON);
	else
		Message(13, "You no longer follow the ways of discord.");

	SendAppearancePacket(AT_PVP, GetPVP());
	Save();
}

void Client::WorldKick() {
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_GMKick, sizeof(GMKick_Struct));
	GMKick_Struct* gmk = (GMKick_Struct *)outapp->pBuffer;
	strcpy(gmk->name,GetName());
	QueuePacket(outapp);
	safe_delete(outapp);
	Kick();
}

void Client::GMKill() {
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_GMKill, sizeof(GMKill_Struct));
	GMKill_Struct* gmk = (GMKill_Struct *)outapp->pBuffer;
	strcpy(gmk->name,GetName());
	QueuePacket(outapp);
	safe_delete(outapp);
}

bool Client::CheckAccess(sint16 iDBLevel, sint16 iDefaultLevel) {
	if ((admin >= iDBLevel) || (iDBLevel == 255 && admin >= iDefaultLevel))
		return true;
	else
		return false;
}

void Client::MemorizeSpell(int32 slot,int32 spellid,int32 scribing){
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_MemorizeSpell,sizeof(MemorizeSpell_Struct));
	MemorizeSpell_Struct* mss=(MemorizeSpell_Struct*)outapp->pBuffer;
	mss->scribing=scribing;
	mss->slot=slot;
	mss->spell_id=spellid;
	outapp->priority = 5;
	QueuePacket(outapp);
	safe_delete(outapp);
}

void Client::SetFeigned(bool in_feigned) {
	if (in_feigned)
	{
		SetPet(0);
		SetHorseId(0);
		entity_list.ClearFeignAggro(this);
	}
	feigned=in_feigned;
 }

void Client::LogMerchant(Client* player, Mob* merchant, Merchant_Sell_Struct* mp, const Item_Struct* item, bool buying)
{
	Merchant_Purchase_Struct* mps = new Merchant_Purchase_Struct;
	mps->itemslot=mp->itemslot;
	mps->npcid=mp->npcid;
	mps->price=mp->price;
	mps->quantity=mp->quantity;
	LogMerchant(player,merchant,mps,item,buying);
	safe_delete(mps);
}
void Client::LogMerchant(Client* player, Mob* merchant, Merchant_Purchase_Struct* mp, const Item_Struct* item, bool buying)
{
	char* logtext;
	char itemid[100];
	char itemcost[100];
	char itemname[100];
	char itemquantity[100];
	if (buying==true) {
		memset(itemid,0,sizeof(itemid));
		memset(itemcost,0,sizeof(itemid));
		memset(itemname,0,sizeof(itemid));
		memset(itemquantity,0,sizeof(itemid));
		itoa(mp->quantity,itemquantity,10);
		itoa(item->ID,itemid,10);
		itoa(mp->price,itemcost,20);
		sprintf(itemname,"%s",item->Name);
//		itoa(mp->price,itemcost,10);
		logtext=itemname;
		strcat(logtext,"(");
		strcat(logtext,itemid);
		strcat(logtext,"), Quantity: ");
		strcat(logtext,itemquantity);
		strcat(logtext,", Cost: ");
		strcat(logtext,itemcost);
		database.logevents(player->AccountName(),player->AccountID(),player->admin,player->GetName(),merchant->GetName(),"Buying from Merchant",logtext,2);
	}
	else {
		memset(itemid,0,sizeof(itemid));
		memset(itemcost,0,sizeof(itemid));
		memset(itemname,0,sizeof(itemid));
		memset(itemquantity,0,sizeof(itemid));
		itoa(mp->quantity,itemquantity,10);
		itoa(item->ID,itemid,10);
		sprintf(itemname,"%s",item->Name);
		// @merth: struct change broke this
		/*
		itoa((int)(item->cost*((mp->quantity == 0) ? 1:mp->quantity))-(item->cost * ((mp->quantity == 0) ? 1:mp->quantity) *0.08),itemcost,10);
		*/
		logtext=itemname;
		strcat(logtext,"(");
		strcat(logtext,itemid);
		strcat(logtext,"), Quantity: ");
		strcat(logtext,itemquantity);
		strcat(logtext,", Sell Total: ");
		strcat(logtext,itemcost);
		database.logevents(player->AccountName(),player->AccountID(),player->admin,player->GetName(),merchant->GetName(),"Selling to Merchant",logtext,3);
	}
}

void Client::LogLoot(Client* player, Corpse* corpse, const Item_Struct* item){
	char* logtext;
	char itemid[100];
	char itemname[100];
	char coinloot[100];
	if (item!=0){
		memset(itemid,0,sizeof(itemid));
		memset(itemname,0,sizeof(itemid));
		itoa(item->ID,itemid,10);
		sprintf(itemname,"%s",item->Name);
		logtext=itemname;
		
		strcat(logtext,"(");
		strcat(logtext,itemid);
		strcat(logtext,") Looted");
		database.logevents(player->AccountName(),player->AccountID(),player->admin,player->GetName(),corpse->orgname,"Looting Item",logtext,4);
	}
	else{
		if ((corpse->GetPlatinum() + corpse->GetGold() + corpse->GetSilver() + corpse->GetCopper())>0) {
			memset(coinloot,0,sizeof(coinloot));
			sprintf(coinloot,"%i PP %i GP %i SP %i CP",corpse->GetPlatinum(),corpse->GetGold(),corpse->GetSilver(),corpse->GetCopper());
			logtext=coinloot;
			strcat(logtext," Looted");
			if (corpse->GetPlatinum()>10000)
				database.logevents(player->AccountName(),player->AccountID(),player->admin,player->GetName(),corpse->orgname,"Excessive Loot!",logtext,9);
			else
				database.logevents(player->AccountName(),player->AccountID(),player->admin,player->GetName(),corpse->orgname,"Looting Money",logtext,5);
		}
	}
}


bool Client::BindWound(Mob* bindmob, bool start, bool fail){
	EQApplicationPacket* outapp = 0;
	if(!fail) {
		outapp = new EQApplicationPacket(OP_Bind_Wound, sizeof(BindWound_Struct));
		BindWound_Struct* bind_out = (BindWound_Struct*) outapp->pBuffer;
		// Start bind
		if(!bindwound_timer.Enabled()) {
			//make sure we actually have a bandage... and consume it.
			sint16 bslot = m_inv.HasItemByUse(ItemTypeBandage, 1, invWhereWorn|invWherePersonal);
			if(bslot == SLOT_INVALID) {
				bind_out->type = 3;
				QueuePacket(outapp);
				bind_out->type = 7;	//this is the wrong message, dont know the right one.
				QueuePacket(outapp);
				return(true);
			}
			DeleteItemInInventory(bslot, 1, true);	//do we need client update?
			
			// start complete timer
			bindwound_timer.Start(10000);
			bindwound_target = bindmob;

			// Send client unlock
			bind_out->type = 3;
			QueuePacket(outapp);
			bind_out->type = 0;
			// Client Unlocked
			if(!bindmob) {
				// send "bindmob dead" to client
				bind_out->type = 4;
				QueuePacket(outapp);
				bind_out->type = 0;
				bindwound_timer.Disable();
				bindwound_target = 0;
			}
			else {
				// send bindmob "stand still"
				if(!bindmob->IsAIControlled() && bindmob != this ) {
					bind_out->type = 2; // ?
					//bind_out->type = 3; // ?
					bind_out->to = GetID(); // ?
					bindmob->CastToClient()->QueuePacket(outapp);
					bind_out->type = 0;
					bind_out->to = 0;
				}
				else if (bindmob->IsAIControlled() && bindmob != this ){
					; // Tell IPC to stand still?
				}
				else {
					; // Binding self
				}
			}
		} else {
		// finish bind
			// disable complete timer
			bindwound_timer.Disable();
			bindwound_target = 0;
			if(!bindmob){
					// send "bindmob gone" to client
					bind_out->type = 5; // not in zone
					QueuePacket(outapp);
					bind_out->type = 0;
			}

			else {
				if (bindmob->DistNoRoot(*this) <= 400) {
					// send bindmob bind done 
					if(!bindmob->IsAIControlled() && bindmob != this ) {
		            
					}
					else if(bindmob->IsAIControlled() && bindmob != this ) {
					// Tell IPC to resume??
					}
					else {
					// Binding self
					}
					// Send client bind done
					
					//this is taken care of on start of bind, not finish now, and is improved
					//DeleteItemInInventory(m_inv.HasItem(13009, 1), 1, true);
					
					bind_out->type = 1; // Done
					QueuePacket(outapp);
					bind_out->type = 0;
					CheckIncreaseSkill(BIND_WOUND);
					
					int max_percent = 50 + 10 * GetAA(aaFirstAid);
					
					if(GetClass() == MONK && GetSkill(BIND_WOUND) > 200) {
						max_percent = 70 + 10 * GetAA(aaFirstAid);
					}
					
					int max_hp = bindmob->GetMaxHP()*max_percent/100;
					
					// send bindmob new hp's
					if (bindmob->GetHP() < bindmob->GetMaxHP() && bindmob->GetHP() <= (max_hp)-1){
						// 0.120 per skill point, 0.60 per skill level, minimum 3 max 30
						int bindhps = 3;


						if (GetSkill(BIND_WOUND) > 200) {
							bindhps += GetSkill(BIND_WOUND)*4/10;
						} else if (GetSkill(BIND_WOUND) >= 10) {
							bindhps += GetSkill(BIND_WOUND)/4;
						}
						
						//Implementation of aaMithanielsBinding is a guess (the multiplier)
						switch (GetAA(aaBandageWound))
						{
							case 1:
								bindhps = bindhps * (110 + 20*GetAA(aaMithanielsBinding)) / 100;
								break;
							case 2:
								bindhps = bindhps * (125 + 20*GetAA(aaMithanielsBinding)) / 100;
								break;
							case 3:
								bindhps = bindhps * (150 + 20*GetAA(aaMithanielsBinding)) / 100;
								break;
						}
						
						//if the bind takes them above the max bindable
						//cap it at that value. Dont know if live does it this way
						//but it makes sense to me.
						int chp = bindmob->GetHP() + bindhps;
						if(chp > max_hp)
							chp = max_hp;
						
						bindmob->SetHP(chp);
						bindmob->SendHPUpdate();
					}
					else {
						//I dont have the real, live 
						Message(15, "You cannot bind wounds above %d%% hitpoints.", max_percent);
						if(bindmob->IsClient())
							bindmob->CastToClient()->Message(15, "You cannot have your wounds bound above %d%% hitpoints.", max_percent);
						// Too many hp message goes here.
					}
				}
				else {
					// Send client bind failed
					bind_out->type = 6; // They moved
					QueuePacket(outapp);
					bind_out->type = 0;
				}
			}
		}
	}
	else if (bindwound_timer.Enabled()) {
		// You moved
		outapp = new EQApplicationPacket(OP_Bind_Wound, sizeof(BindWound_Struct));
		BindWound_Struct* bind_out = (BindWound_Struct*) outapp->pBuffer;
		bindwound_timer.Disable();
		bindwound_target = 0;
		bind_out->type = 7;
		QueuePacket(outapp);
		bind_out->type = 3;
		QueuePacket(outapp);
	}
	safe_delete(outapp);
	return true;
}

void Client::SetMaterial(sint16 in_slot, uint32 item_id){
	const Item_Struct* item = database.GetItem(item_id);
	if (item && (item->ItemClass==ItemClassCommon)) {
		if (in_slot==SLOT_HEAD)
			m_pp.item_material[MATERIAL_HEAD]		= item->Common.Material;
		else if (in_slot==SLOT_CHEST)
			m_pp.item_material[MATERIAL_CHEST]		= item->Common.Material;
		else if (in_slot==SLOT_ARMS)
			m_pp.item_material[MATERIAL_ARMS]		= item->Common.Material;
		else if (in_slot==SLOT_BRACER01)
			m_pp.item_material[MATERIAL_BRACER]		= item->Common.Material;
		else if (in_slot==SLOT_BRACER02)
			m_pp.item_material[MATERIAL_BRACER]		= item->Common.Material;
		else if (in_slot==SLOT_HANDS)
			m_pp.item_material[MATERIAL_HANDS]		= item->Common.Material;
		else if (in_slot==SLOT_LEGS)
			m_pp.item_material[MATERIAL_LEGS]		= item->Common.Material;
		else if (in_slot==SLOT_FEET)
			m_pp.item_material[MATERIAL_FEET]		= item->Common.Material;
		else if (in_slot==SLOT_PRIMARY)
			m_pp.item_material[MATERIAL_PRIMARY]	= atoi(item->IDFile+2);
		else if (in_slot==SLOT_SECONDARY)
			m_pp.item_material[MATERIAL_SECONDARY]	= atoi(item->IDFile+2);
	}
}

void Client::ServerFilter(SetServerFilter_Struct* filter){
	ClientFilters[FILTER_DAMAGESHIELD]=filter->damageshield;
	// solar: this one is reversed - 1 == off
	ClientFilters[FILTER_NPCSPELLS]=!filter->npcspells;
	if(filter->pcspells==0)
		ClientFilters[FILTER_PCSPELLS]=1; //all pc spells on
	else if(filter->pcspells==1)
		ClientFilters[FILTER_PCSPELLS]=0; //pc spells off
	else
		ClientFilters[FILTER_PCSPELLS]=99;//group pc spells on
	if(filter->bardsongs==0 || filter->bardsongs==1)
		ClientFilters[FILTER_BARDSONGS]=1;
	else if(filter->bardsongs==2)//group
		ClientFilters[FILTER_BARDSONGS]=99;
	else	
		ClientFilters[FILTER_BARDSONGS]=0;//turn off all pc bard songs
	ClientFilters[FILTER_GUILDSAY]=filter->guildsay;
	ClientFilters[FILTER_SOCIALS]=filter->socials;
	ClientFilters[FILTER_GROUP]=filter->group;
	ClientFilters[FILTER_SHOUT]=filter->shout;
	ClientFilters[FILTER_AUCTION]=filter->auction;
	ClientFilters[FILTER_OOC]=filter->ooc;
	ClientFilters[FILTER_MYMISSES]=filter->mymisses;
	ClientFilters[FILTER_OTHERMISSES]=filter->othermisses;
	ClientFilters[FILTER_OTHERHITS]=filter->otherhits;
	ClientFilters[FILTER_ATKMISSESME]=filter->atkmissesme;
	if(filter->critspells==0)
		ClientFilters[FILTER_CRITSPELLS]=1;//all
	else if(filter->critspells==1)
		ClientFilters[FILTER_CRITSPELLS]=98; //me only
	else
		ClientFilters[FILTER_CRITSPELLS]=0;//off
	if(filter->critmelee==0)
		ClientFilters[FILTER_CRITMELEE]=1;//all
	else if(filter->critmelee==1)
		ClientFilters[FILTER_CRITMELEE]=98;//me only
	else
		ClientFilters[FILTER_CRITMELEE]=0;//off
	if(filter->spelldamage==0)
		ClientFilters[FILTER_SPELLDAMAGE]=1;//all
	else if(filter->spelldamage==1)
		ClientFilters[FILTER_SPELLDAMAGE]=98;//me only
	else
		ClientFilters[FILTER_SPELLDAMAGE]=0;//off
	ClientFilters[FILTER_DOTDAMAGE]=filter->dotdamage;
	// solar: on these 0 means on and 1 means off
	ClientFilters[FILTER_MYPETHITS]=!filter->mypethits;
	ClientFilters[FILTER_MYPETMISSES]=!filter->mypetmisses;
}

// this version is for messages with no parameters
void Client::Message_StringID(int32 type, int32 string_id, int32 distance)
{
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_SimpleMessage,12);
	SimpleMessage_Struct* sms = (SimpleMessage_Struct*)outapp->pBuffer;
	sms->color=type;
	sms->string_id=string_id;

	sms->unknown8=0;

	if(distance>0)
		entity_list.QueueCloseClients(this,outapp,false,distance);
	else
		QueuePacket(outapp);
	safe_delete(outapp);
}

//
// solar: this list of 9 args isn't how I want to do it, but to use va_arg
// you have to know how many args you're expecting, and to do that we have
// to load the eqstr file and count them in the string.
// This hack sucks but it's gonna work for now.
//
void Client::Message_StringID(int32 type, int32 string_id,  const char* message1,
	const char* message2,const char* message3,const char* message4,
	const char* message5,const char* message6,const char* message7,
	const char* message8,const char* message9, int32 distance)
{
	int i, argcount, length;
	char *bufptr;
	const char *message_arg[9] = {0};

	if(type==MT_Emote)
		type=4;

	if(!message1)
	{
		Message_StringID(type, string_id);	// use the simple message instead
		return;
	}

	i = 0;
	message_arg[i++] = message1;
	message_arg[i++] = message2;
	message_arg[i++] = message3;
	message_arg[i++] = message4;
	message_arg[i++] = message5;
	message_arg[i++] = message6;
	message_arg[i++] = message7;
	message_arg[i++] = message8;
	message_arg[i++] = message9;

	for(argcount = length = 0; message_arg[argcount]; argcount++)
		length += strlen(message_arg[argcount]) + 1;
	
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_FormattedMessage, length+13);
	FormattedMessage_Struct *fm = (FormattedMessage_Struct *)outapp->pBuffer;
	fm->string_id = string_id;
	fm->type = type;
	bufptr = fm->message;
	for(i = 0; i < argcount; i++)
	{
		strcpy(bufptr, message_arg[i]);
		bufptr += strlen(message_arg[i]) + 1;
	}

	if(distance>0)
		entity_list.QueueCloseClients(this,outapp,false,distance);
	else
		QueuePacket(outapp);
	safe_delete(outapp);
}


void Client::SetTint(sint16 in_slot, uint32 color) {
	Color_Struct new_color;
	new_color.color = color;
	SetTint(in_slot, new_color);
}

// @merth: Still need to reconcile bracer01 versus bracer02
void Client::SetTint(sint16 in_slot, Color_Struct& color) {
	if (in_slot==SLOT_HEAD)
		m_pp.item_tint[MATERIAL_HEAD].color=color.color;
	else if (in_slot==SLOT_ARMS)
		m_pp.item_tint[MATERIAL_ARMS].color=color.color;
	else if (in_slot==SLOT_BRACER01)
		m_pp.item_tint[MATERIAL_BRACER].color=color.color;
	else if (in_slot==SLOT_BRACER02)
		m_pp.item_tint[MATERIAL_BRACER].color=color.color;
	else if (in_slot==SLOT_HANDS)
		m_pp.item_tint[MATERIAL_HANDS].color=color.color;
	else if (in_slot==SLOT_PRIMARY)
		m_pp.item_tint[MATERIAL_PRIMARY].color=color.color;
	else if (in_slot==SLOT_SECONDARY)
		m_pp.item_tint[MATERIAL_SECONDARY].color=color.color;
	else if (in_slot==SLOT_CHEST)
		m_pp.item_tint[MATERIAL_CHEST].color=color.color;
	else if (in_slot==SLOT_LEGS)
		m_pp.item_tint[MATERIAL_LEGS].color=color.color;
	else if (in_slot==SLOT_FEET)
		m_pp.item_tint[MATERIAL_FEET].color=color.color;
}

bool Client::CheckCheat(){
	float dx=cheat_x-x_pos;
	float dy=cheat_y-y_pos;
	float result=sqrt((dx*dx)+(dy*dy));
	return result>70;
}

void Client::SetHideMe(bool flag)
{
	EQApplicationPacket app;

	gmhideme = flag;

	if(gmhideme)
	{
		database.SetHideMe(AccountID(),false);
		CreateDespawnPacket(&app);
		entity_list.RemoveFromTargets(this);
	}
	else
	{
		database.SetHideMe(AccountID(),true);
		CreateSpawnPacket(&app);
	}

	entity_list.QueueClientsStatus(this, &app, true, 0, Admin()-1);
}

void Client::SetLanguageSkill(int langid, int value)
{
	if (langid > 26)
		return;
	if( value <= 100 )
	{
		m_pp.languages[langid] = value;

		Message_StringID( 270, 449 );
	}
}

void Client::LinkDead()
{
	if (GetGroup())
	{
		entity_list.MessageGroup(this,true,15,"%s has gone linkdead.",GetName());
		GetGroup()->DelMember(this);
	}
//	save_timer.Start(2500);
	linkdead_timer.Start(180000);
	SendAppearancePacket(AT_Linkdead, 1);
}

int8 Client::SlotConvert(int8 slot,bool bracer){
	int8 slot2=0;
	if(bracer)
		return SLOT_BRACER02;
	switch(slot){
		case MATERIAL_HEAD:
			slot2=SLOT_HEAD;
			break;
		case MATERIAL_CHEST:
			slot2=SLOT_CHEST;
			break;
		case MATERIAL_ARMS:
			slot2=SLOT_ARMS;
			break;
		case MATERIAL_BRACER:
			slot2=SLOT_BRACER01;
			break;
		case MATERIAL_HANDS:
			slot2=SLOT_HANDS;
			break;
		case MATERIAL_LEGS:
			slot2=SLOT_LEGS;
			break;
		case MATERIAL_FEET:
			slot2=SLOT_FEET;
			break;
		}
	return slot2;
}

int8 Client::SlotConvert2(int8 slot){
	int8 slot2=0;
	switch(slot){
		case SLOT_HEAD:
			slot2=MATERIAL_HEAD;
			break;
		case SLOT_CHEST:
			slot2=MATERIAL_CHEST;
			break;
		case SLOT_ARMS:
			slot2=MATERIAL_ARMS;
			break;
		case SLOT_BRACER01:
			slot2=MATERIAL_BRACER;
			break;
		case SLOT_HANDS:
			slot2=MATERIAL_HANDS;
			break;
		case SLOT_LEGS:
			slot2=MATERIAL_LEGS;
			break;
		case SLOT_FEET:
			slot2=MATERIAL_FEET;
			break;
		}
	return slot2;
}

void Client::Escape()
{
	invisible = true;
	entity_list.ClearFeignAggro(this);

	EQApplicationPacket *outapp = new EQApplicationPacket(OP_SimpleMessage,12);
	SimpleMessage_Struct *msg=(SimpleMessage_Struct *)outapp->pBuffer;
	msg->color=0x010E;
	msg->string_id=114;
	FastQueuePacket(&outapp);

	outapp = new EQApplicationPacket(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
	SpawnAppearance_Struct* sa_out = (SpawnAppearance_Struct*)outapp->pBuffer;
	sa_out->spawn_id = GetID();
	sa_out->type = 0x03;
	sa_out->parameter = 1;
	entity_list.QueueClients(this, outapp);
	safe_delete(outapp);
}

float Client::CalcPriceMod(Mob* other, bool reverse)
{
	float chaformula = 0;

	if (GetCHA() > 100)
	{
		chaformula = (GetCHA() - 100)*-0.1;
		if (chaformula < -5)
			chaformula = -5;
	}
	else if (GetCHA() < 75)
	{
		chaformula = (75 - GetCHA())*0.1;
		if (chaformula > 5)
			chaformula = 5;
	}
	if (other)
	{
		int factionlvl = GetFactionLevel(CharacterID(), other->CastToNPC()->GetNPCTypeID(), GetRace(), GetClass(), GetDeity(), other->CastToNPC()->GetPrimaryFaction(), other);
		switch (factionlvl)
		{
			case 9: //Apprehensive
				chaformula += 10;
				break;
			case 4: //Amiable
				chaformula -= 2;
				break;
			case 3: //Warmly
				chaformula -= 5;
				break;
			case 2: //Kindly
				chaformula -= 8;
				break;
			case 1: //Ally
				chaformula -= 10;
				break;
		}
	}
	if (reverse)
		chaformula *= -1; //For selling
	//Now we have, for example, 10
	chaformula /= 100; //Convert to 0.10
	chaformula += 1; //Convert to 1.10;
	return chaformula; //Returns 1.10, expensive stuff!
}

//neat idea from winter's roar, not implemented
void Client::Insight(int32 t_id)
{
	Mob* who = entity_list.GetMob(t_id);
	if (!who)
		return;
	if (!who->IsNPC())
	{
		Message(0,"This ability can only be used on NPCs.");
		return;
	}
	if (Dist(*who) > 200)
	{
		Message(0,"You must get closer to your target!");
		return;
	}
	if (!CheckLosFN(who))
	{
		Message(0,"You must be able to see your target!");
		return;
	}
	char hitpoints[64];
	char resists[320];
	char dmg[64];
	memset(hitpoints,64,0);
	memset(resists,320,0);
	memset(dmg,64,0);
	//Start with HP blah
	int avg_hp = GetLevelHP(who->GetLevel());
	int cur_hp = who->GetHP();
	if (cur_hp == avg_hp)
	{
		strncpy(hitpoints,"averagely tough",32);
	}
	else if (cur_hp >= avg_hp*5)
	{
		strncpy(hitpoints,"extremely tough",32);
	}
	else if (cur_hp >= avg_hp*4)
	{
		strncpy(hitpoints,"exceptionally tough",32);
	}
	else if (cur_hp >= avg_hp*3)
	{
		strncpy(hitpoints,"very tough",32);
	}
	else if (cur_hp >= avg_hp*2)
	{
		strncpy(hitpoints,"quite tough",32);
	}
	else if (cur_hp >= avg_hp*1.25)
	{
		strncpy(hitpoints,"rather tough",32);
	}
	else if (cur_hp > avg_hp)
	{
		strncpy(hitpoints,"slightly tough",32);
	}
	else if (cur_hp <= avg_hp*0.20)
	{
		strncpy(hitpoints,"extremely frail",32);
	}
	else if (cur_hp <= avg_hp*0.25)
	{
		strncpy(hitpoints,"exceptionally frail",32);
	}
	else if (cur_hp <= avg_hp*0.33)
	{
		strncpy(hitpoints,"very frail",32);
	}
	else if (cur_hp <= avg_hp*0.50)
	{
		strncpy(hitpoints,"quite frail",32);
	}
	else if (cur_hp <= avg_hp*0.75)
	{
		strncpy(hitpoints,"rather frail",32);
	}
	else if (cur_hp < avg_hp)
	{
		strncpy(hitpoints,"slightly frail",32);
	}

	int avg_dmg = who->CastToNPC()->GetMaxDamage(who->GetLevel());
	int cur_dmg = who->CastToNPC()->GetMaxDMG();
	if (cur_dmg == avg_dmg)
	{
		strncpy(dmg,"averagely strong",32);
	}
	else if (cur_dmg >= avg_dmg*4)
	{
		strncpy(dmg,"extremely strong",32);
	}
	else if (cur_dmg >= avg_dmg*3)
	{
		strncpy(dmg,"exceptionally strong",32);
	}
	else if (cur_dmg >= avg_dmg*2)
	{
		strncpy(dmg,"very strong",32);
	}
	else if (cur_dmg >= avg_dmg*1.25)
	{
		strncpy(dmg,"quite strong",32);
	}
	else if (cur_dmg >= avg_dmg*1.10)
	{
		strncpy(dmg,"rather strong",32);
	}
	else if (cur_dmg > avg_dmg)
	{
		strncpy(dmg,"slightly strong",32);
	}
	else if (cur_dmg <= avg_dmg*0.20)
	{
		strncpy(dmg,"extremely weak",32);
	}
	else if (cur_dmg <= avg_dmg*0.25)
	{
		strncpy(dmg,"exceptionally weak",32);
	}
	else if (cur_dmg <= avg_dmg*0.33)
	{
		strncpy(dmg,"very weak",32);
	}
	else if (cur_dmg <= avg_dmg*0.50)
	{
		strncpy(dmg,"quite weak",32);
	}
	else if (cur_dmg <= avg_dmg*0.75)
	{
		strncpy(dmg,"rather weak",32);
	}
	else if (cur_dmg < avg_dmg)
	{
		strncpy(dmg,"slightly weak",32);
	}

	//Resists
	int res;
	int i = 1;

	//MR
	res = who->GetResist(i);
	i++;
	if (res >= 1000)
	{
		strcat(resists,"immune");
	}
	else if (res >= 500)
	{
		strcat(resists,"practically immune");
	}
	else if (res >= 250)
	{
		strcat(resists,"exceptionally resistant");
	}
	else if (res >= 150)
	{
		strcat(resists,"very resistant");
	}
	else if (res >= 100)
	{
		strcat(resists,"fairly resistant");
	}
	else if (res >= 50)
	{
		strcat(resists,"averagely resistant");
	}
	else if (res >= 25)
	{
		strcat(resists,"weakly resistant");
	}
	else
	{
		strcat(resists,"barely resistant");
	}
	strcat(resists," to magic, ");

	//FR
	res = who->GetResist(i);
	i++;
	if (res >= 1000)
	{
		strcat(resists,"immune");
	}
	else if (res >= 500)
	{
		strcat(resists,"practically immune");
	}
	else if (res >= 250)
	{
		strcat(resists,"exceptionally resistant");
	}
	else if (res >= 150)
	{
		strcat(resists,"very resistant");
	}
	else if (res >= 100)
	{
		strcat(resists,"fairly resistant");
	}
	else if (res >= 50)
	{
		strcat(resists,"averagely resistant");
	}
	else if (res >= 25)
	{
		strcat(resists,"weakly resistant");
	}
	else
	{
		strcat(resists,"barely resistant");
	}
	strcat(resists," to fire, ");

	//CR
	res = who->GetResist(i);
	i++;
	if (res >= 1000)
	{
		strcat(resists,"immune");
	}
	else if (res >= 500)
	{
		strcat(resists,"practically immune");
	}
	else if (res >= 250)
	{
		strcat(resists,"exceptionally resistant");
	}
	else if (res >= 150)
	{
		strcat(resists,"very resistant");
	}
	else if (res >= 100)
	{
		strcat(resists,"fairly resistant");
	}
	else if (res >= 50)
	{
		strcat(resists,"averagely resistant");
	}
	else if (res >= 25)
	{
		strcat(resists,"weakly resistant");
	}
	else
	{
		strcat(resists,"barely resistant");
	}
	strcat(resists," to cold, ");

	//PR
	res = who->GetResist(i);
	i++;
	if (res >= 1000)
	{
		strcat(resists,"immune");
	}
	else if (res >= 500)
	{
		strcat(resists,"practically immune");
	}
	else if (res >= 250)
	{
		strcat(resists,"exceptionally resistant");
	}
	else if (res >= 150)
	{
		strcat(resists,"very resistant");
	}
	else if (res >= 100)
	{
		strcat(resists,"fairly resistant");
	}
	else if (res >= 50)
	{
		strcat(resists,"averagely resistant");
	}
	else if (res >= 25)
	{
		strcat(resists,"weakly resistant");
	}
	else
	{
		strcat(resists,"barely resistant");
	}
	strcat(resists," to poison, and ");

	//MR
	res = who->GetResist(i);
	i++;
	if (res >= 1000)
	{
		strcat(resists,"immune");
	}
	else if (res >= 500)
	{
		strcat(resists,"practically immune");
	}
	else if (res >= 250)
	{
		strcat(resists,"exceptionally resistant");
	}
	else if (res >= 150)
	{
		strcat(resists,"very resistant");
	}
	else if (res >= 100)
	{
		strcat(resists,"fairly resistant");
	}
	else if (res >= 50)
	{
		strcat(resists,"averagely resistant");
	}
	else if (res >= 25)
	{
		strcat(resists,"weakly resistant");
	}
	else
	{
		strcat(resists,"barely resistant");
	}
	strcat(resists," to disease.");

	Message(0,"Your target is a level %i %s. It appears %s and %s for its level. It seems %s",who->GetLevel(),GetEQClassName(who->GetClass(),1),dmg,hitpoints,resists);
}

void Client::ChangeSQLLog(const char *file) {
	if(SQL_log != NULL) {
		fclose(SQL_log);
		SQL_log = NULL;
	}
	if(file != NULL) {
		if(strstr(file, "..") != NULL) {
			Message(13, ".. is forbibben in SQL log file names.");
			return;
		}
		char buf[512];
		snprintf(buf, 511, "%s%s", SQL_LOG_PATH, file);
		buf[511] = '\0';
		SQL_log = fopen(buf, "a");
		if(SQL_log == NULL) {
			Message(13, "Unable to open SQL log file: %s\n", strerror(errno));
		}
	}
}

void Client::LogSQL(const char *fmt, ...) {
	if(SQL_log == NULL)
		return;
	
	va_list argptr;
	va_start(argptr, fmt);
	vfprintf(SQL_log, fmt, argptr );
	fputc('\n', SQL_log);
	va_end(argptr);
}







/*
	Cofruben:Starting LDoN adventure system
*/
bool IsInAdventure(int32 id,int32 qid) {
	for(int zo=0;zo<6;zo++){
		if(database.GetAdventureChar(zo,qid) == id){
			return true;
		}
		else
			continue;
	}
	return false;
}
void Client::DeleteCharInAdventure(int32 id,int32 qid) {
	int zo;
	for(zo=0;zo<6;zo++){
		if(database.GetAdventureChar(zo,qid) == id){
			database.SetAdventureChar(zo,0,qid);
			break;
		}
	}
	int32 count=0;
	for(zo=0;zo<6;zo++) {
		int32 ac=database.GetAdventureChar(zo,GetAdventureID());
		if(ac>0){
			Client* z=NULL;
			z=entity_list.GetClientByCharID(ac);
			if(z!=NULL && z->IsClient())
				count++;
		}
	}
	if(count==0) {
		database.SetAdventureInfo(GetAdventureID(),false,0);
	}
}
void Client::SendAdventureFinish(uint32 state,uint32 points,bool grouptoo)
{
	if(GetAdventureID()==0)return;
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_AdventureFinish,sizeof(AdventureFinish_Struct));
	memset(outapp->pBuffer,0,outapp->size);
	AdventureFinish_Struct* af=(AdventureFinish_Struct*)outapp->pBuffer;
	AdventureInfo AI=database.GetAdventureInfo(GetAdventureID());
	int32 advid=GetAdventureID();
	if(grouptoo==true){//cambiar el grupo por los id's de char's de db
		int z;
		for(z=0;z<6;z++){
			int32 id=database.GetAdventureChar(z,advid);
			if( id > 0) {
				Client* c=entity_list.GetClientByCharID(id);
				if(c!=NULL && c->IsClient() && c->CastToClient()->GetAdventureID() == AI.QuestID ) {
					c->CastToClient()->SendAdventureFinish(1,points,false);
				}
			}

		}
		for(z=0;z<6;z++)
			database.SetAdventureChar(z,0,GetAdventureID());
	}
	else {
		if(state==0){ //lose
			af->points=0;
			af->win_lose=0;
		}
		else { //win
			af->points=points;
			if(af->points==0)af->points=1;
			af->win_lose=1;
			UpdateLDoNPoints(points,0);
		}
		DumpPacket(outapp);
		DeleteCharInAdventure(CharacterID(),GetAdventureID());

		if(p_timers.Get(pTimerAdventureTimer))p_timers.Disable(pTimerAdventureTimer);
		if(p_timers.Get(pTimerStartAdventureTimer))p_timers.Disable(pTimerStartAdventureTimer);
		SetAdventureID(0);
		QueuePacket(outapp);
		safe_delete(outapp);
	}
}
void Client::SendAdventureInfoRequest(const EQApplicationPacket* app){
	EntityId_Struct* eid = (EntityId_Struct*)app->pBuffer;
	Mob* tmp = entity_list.GetMob(eid->entity_id);
	char* buffer1;
	SetAdventureID(tmp->GetNPCTypeID());
	char* p=database.GetAdventureNPCText(tmp->GetNPCTypeID());
	if(p == NULL)
		return;
	buffer1=new char[strlen(p)+1];
	strcpy(buffer1,p);
	buffer1[strlen(p)]=0x00;
	buffer1[strlen(p)+1]='\0';
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_AdventureInfo,strlen(buffer1)+1);
	memset(outapp->pBuffer,0,outapp->size);
	char* buffer=(char*)outapp->pBuffer;
	memcpy(buffer,buffer1, strlen(buffer1)+1);
	QueuePacket(outapp);
	safe_delete(outapp);
	safe_delete(buffer1);
}
void Client::SendAdventureUpdate(){
	AdventureInfo AF=database.GetAdventureInfo(GetAdventureID());
	database.SetAdventureInfo(GetAdventureID(),true,AF.status+1);
	AF.status+=1;
	EQApplicationPacket* outapp=new EQApplicationPacket(OP_AdventureUpdate,8);
	uchar* p=(uchar*)outapp->pBuffer;
	memcpy(p,&AF.status,4);
	p+=4;
	memcpy(p,&AF.ObjetiveValue,4);
	for(int o=0;o<6;o++) {
		Client* id=NULL;
		int32 cid=database.GetAdventureChar(o,GetAdventureID());
		id=entity_list.GetClientByCharID(cid);
		if(id!=NULL && id->CastToClient()->GetAdventureID()>0) {
			id->CastToClient()->QueuePacket(outapp);
		}
	}
	if(AF.ObjetiveValue == AF.status)SendAdventureFinish(1,AF.points,true);
	safe_delete(outapp);
}


void Client::SendAdventureRequest(){
	printf("Sending adventure info to %s...\n",GetName());
	int rd=MakeRandomInt(0,3);
	int count=0;
	char* buffer1;
	AdventureInfo AF=database.GetAdventureInfo(0,GetAdventureID(),rd);
	EQApplicationPacket* outapp;
	bool flag=AF.in_use;
	while(flag==true && count!=20){
		if(flag==0)break;
		Sleep(2);
		rd=MakeRandomInt(0,3);
		AF=database.GetAdventureInfo(0,GetAdventureID(),rd);
		flag=AF.in_use;
		count++;
		if(count>=20){
			break;
		}
	}
	if(count==20){
		const char* p="There is no adventure available";
		printf("%s\n", p);
		outapp=new EQApplicationPacket(OP_AdventureInfo,strlen(p)+1);
		buffer1=new char[strlen(p)+1];
		strcpy(buffer1,p);
		buffer1[strlen(p)]=0x00;
	}
	else {
		outapp = new EQApplicationPacket(OP_AdventureDetails,strlen(AF.text)+1);
		buffer1=new char[strlen(AF.text)+1];
		strcpy(buffer1,AF.text);
		buffer1[strlen(AF.text)]=0x00;
	}
	char* buffer=(char*)outapp->pBuffer;
	memset(outapp->pBuffer,0,outapp->size);
	memcpy(buffer,buffer1, strlen(buffer1)+1);
	SetAdventureID(AF.QuestID);
	QueuePacket(outapp);
	safe_delete(outapp);
	safe_delete(buffer1);
	
}
void Client::SendAdventureRequestData(Group* group,bool EnteredDungeon,bool EnteredZone,bool Zoned){
	bool send=false;
	AdventureInfo AF=database.GetAdventureInfo(GetAdventureID());
	if(strlen(AF.text)<2){
		printf("adventure with id %i not found!\n",GetAdventureID());
		return;
	}
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_AdventureData,sizeof(AdventureRequestResponse_Struct));
	memset(outapp->pBuffer,0,outapp->size);
	AdventureRequestResponse_Struct* adrr=(AdventureRequestResponse_Struct*)outapp->pBuffer;
	adrr->risk=1;
	adrr->unknown000=0xBFC40100;
	strcpy(adrr->text,AF.text);
	int32 ID=GetAdventureID();
	adrr->x=AF.x;
	adrr->y=AF.y;
	adrr->z=0;
	adrr->unknown2080=0x0A;
	database.SetAdventureInfo(ID,true,0);
	if(EnteredDungeon==false && EnteredZone==false && !p_timers.Enabled(pTimerStartAdventureTimer))
	{
		for(int z=0;z<6;z++)
			database.SetAdventureChar(z,0,ID);
		adrr->timeleft=30*60;
	}
	else if(EnteredDungeon==false && EnteredZone==false && p_timers.Enabled(pTimerStartAdventureTimer))
	{
		adrr->timeleft=p_timers.GetRemainingTime(pTimerStartAdventureTimer);
		send=true;
	}
	else if(EnteredDungeon==true && !p_timers.Enabled(pTimerAdventureTimer)){
		adrr->timeleft=AF.minutes*60;
	}
	else if(EnteredDungeon==true && p_timers.Enabled(pTimerAdventureTimer)){
		adrr->timeleft=p_timers.GetRemainingTime(pTimerAdventureTimer);
		send=true;
	}
	if(EnteredZone==true) {
		adrr->showcompass=AF.ShowCompass;
		printf("Client %s entered compass zone.\n", GetName());
		Zoned=true;
		send=true;
	}
	else
		adrr->showcompass=0;
	if(Zoned==true) {
		send=true;
		if(p_timers.Enabled(pTimerAdventureTimer)
			&& p_timers.GetRemainingTime(pTimerAdventureTimer)>0)
			adrr->timeleft=p_timers.GetRemainingTime(pTimerAdventureTimer);
		else if(p_timers.Enabled(pTimerStartAdventureTimer) 
			&& p_timers.GetRemainingTime(pTimerStartAdventureTimer)>0)
			adrr->timeleft=p_timers.GetRemainingTime(pTimerStartAdventureTimer);
		else {
			printf("zoned sin timer %i\n",ID);
			EnteredZone=true;
			SetAdventureID(0);
			send=false;
			return;
		}
	}
	if(send==true){
		QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}
	for(int xx=0;xx<6;xx++){
		if(EnteredDungeon==false && EnteredZone==false){
			if(group && group->members[xx]!=NULL && group->members[xx]->IsClient() && !group->members[xx]->CastToClient()->p_timers.Enabled(pTimerStartAdventureTimer)){
					group->members[xx]->CastToClient()->QueuePacket(outapp);
					group->members[xx]->CastToClient()->SetAdventureID(ID);
					group->members[xx]->CastToClient()->p_timers.Start(pTimerStartAdventureTimer,30*60);//* 60
					database.SetAdventureChar(xx,group->members[xx]->CastToClient()->CharacterID(),ID);
			}
		}
		else if(EnteredDungeon==true && EnteredZone==false){
				int32 id=database.GetAdventureChar(xx,ID);
				if(id==0)continue;
				Client* c=entity_list.GetClientByCharID(id);
				if(c==NULL || !c->IsClient())continue;
				if(c->CastToClient()->p_timers.Enabled(pTimerAdventureTimer) == true)continue;
				c->CastToClient()->QueuePacket(outapp);
				printf("Client %s entered a dungeon\n",c->GetName());
				c->CastToClient()->p_timers.Disable(pTimerStartAdventureTimer);
				c->CastToClient()->p_timers.Start(pTimerAdventureTimer,AF.minutes*60);//* 60
		}	
		if(group && group->members[xx]!=NULL && group->members[xx]->IsClient() && 
			group->members[xx]->CastToClient()->GetAdventureID()==0){
			group->members[xx]->CastToClient()->Message(0,"You should not be in this group,adventure error!");
			group->members[xx]->CastToClient()->LeaveGroup();
		}
	}
	safe_delete(outapp);
}

