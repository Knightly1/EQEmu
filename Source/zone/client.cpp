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
#include "forage.h"
#include "command.h"
#include "StringIDs.h"
#include "NpcAI.h"

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

Client::Client(EQNetworkConnection* ieqnc)
: Mob("No name",	// name
	"",	// lastname
	0,	// cur_hp
	0,	// max_hp
	0,	// gender
	0,	// race
	0,	// class
	0,	// bodytype
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
	0,	// equip
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
	position_timer(250),
	hpregen_timer(1800),
	camp_timer(29000),
	process_timer(100),
	disc_timer(60000),
	disc_elapse(60000),
	stamina_timer(46000),
	linkdead_timer(30000),
	dead_timer(2000),
	ooc_timer(1000),
	shield_timer(500),
	fishing_timer(8000)
{
	for(int cf=0;cf<21;cf++)
		ClientFilters[cf]=0;
	character_id = 0;
	client_data_loaded = false;
	feigned = false;
	berserk = false;
	dead = false;
	eqnc = ieqnc;
	ip = eqnc->GetrIP();
	port = ntohs(eqnc->GetrPort());
	client_state = CLIENT_CONNECTING;
	Trader=false;
	withcustomer=false;
	IsTracking=false;
	WID = 0;
	account_id = 0;
	admin = 0;
	lsaccountid = 0;
	shield_target = NULL;
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
	zonesummon_ignorerestrictions = 0;
	casting_spell_id = 0;
	npcflag = false;
	npclevel = 0;
	pQueuedSaveWorkID = 0;
	position_timer_counter = 0;
	fishing_timer.Disable();
	shield_timer.Disable();
	dead_timer.Disable();
	camp_timer.Disable();
	zoning = false;
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
	hasmount = false;
	horseId = 0;
	tgb = false;
	AbilityTimer=false;
	memset(zonesummon_name, 0, sizeof(zonesummon_name));
	
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

//	if(AbilityTimer || GetLevel()>=51)
//		database.UpdateAndDeleteAATimers(CharacterID());

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
	
	eqnc->Free();
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

	m_pp.x = x_pos;
	m_pp.y = y_pos;
	m_pp.z = z_pos;
	m_pp.guildrank=guildrank;
	m_pp.heading = heading;
	int spentpoints=0;
	for(int a=0;a < MAX_PP_AA_ARRAY;a++){
		if(aa.aa_list[a].aa_value>1)
			m_pp.aa_array[a].AA=aa.aa_list[a].aa_skill+aa.aa_list[a].aa_value-1;
		else
			m_pp.aa_array[a].AA=aa.aa_list[a].aa_skill;
		m_pp.aa_array[a].value=aa.aa_list[a].aa_value;
		spentpoints+=aa.aa_list[a].aa_value;
	}
	
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
			m_pp.buffs[i].spellid = 0;	//should this be SPELL_UNKNOWN?
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
		m_pp.pet_id = GetPet()->CastToNPC()->GetPetSpellID();
		m_pp.pet_hp = GetPet()->GetHP();
	} else {
		m_pp.pet_id = 0;
		m_pp.pet_hp = 0;
	}
	
	//FatherNitwit: I dont know if there is a better place for this:
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
		dbaw->AddQuery(iCommitNow == 0 ? true : false, &query, database.SetPlayerProfile_MQ(&query, account_id, character_id, &m_pp, &m_inv), false);
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
	else if (database.SetPlayerProfile(account_id, character_id, &m_pp, &m_inv)) {
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

bool Client::AddPacket(const APPLAYER *pApp, bool bAckreq) {
	if (!pApp)
		return false;
    CLIENTPACKET *c = new CLIENTPACKET;

    c->ack_req = bAckreq;
    c->app = pApp->Copy();
    clientpackets.Append(c);
    return true;
}

bool Client::AddPacket(APPLAYER** pApp, bool bAckreq) {
	if (!pApp || !(*pApp))
		return false;
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
		if(eqnc)
			eqnc->FastQueuePacket(&cp->app, iterator.GetData()->ack_req);
		iterator.RemoveCurrent();
		LogFile->write(EQEMuLog::Normal, "Transmitting a packet");
	}
	return true;
}

void Client::QueuePacket(const APPLAYER* app, bool ack_req, CLIENT_CONN_STATUS required_state,int8 filter) {
	if(filter!=0){
		if(GetFilter(filter)==0)
			return; //Client has this filter on, no need to send packet
	}
	if (app != 0) {
		if (app->size >= 31500) {
			cout << "WARNING: abnormal packet size. n='" << this->GetName() << "', o=0x" << hex << app->opcode << dec << ", s=" << app->size << endl;
			return;
		}
	}
	
	//#ifdef EQDEBUG >= 9
		// This just here while figuring out new opcodes/packets
		#ifdef MERTHALICIOUS
			//@merth: this just here temporarily for my debugging
			cout << "Sending: 0x" << hex << setw(4) << setfill('0') << app->opcode << dec << ", size=" << app->size << endl;
		#endif
	//#endif
	
	// if the program doesnt care about the status or if the status isnt what we requested
    if (required_state != CLIENT_CONNECTINGALL && client_state != required_state)
    {
        // todo: save packets for later use
        AddPacket(app, ack_req);
//        LogFile->write(EQEMuLog::Normal, "Adding Packet to list (%d) (%d)", app->opcode, (int)required_state);
    }
    else
	    if(eqnc)
            eqnc->QueuePacket(app, ack_req);
}

void Client::FastQueuePacket(APPLAYER** app, bool ack_req, CLIENT_CONN_STATUS required_state) {
	if (app != 0 && (*app) != 0) {
		if ((*app)->size >= 31500) {
			cout << "WARNING: abnormal packet size. n='" << this->GetName() << "', o=0x" << hex << (*app)->opcode << dec << ", s=" << (*app)->size << endl;
			return;
		}
	}
	
	//cout << "Sending: 0x" << hex << setw(4) << setfill('0') << (*app)->opcode << dec << ", size=" << (*app)->size << endl;
	
	// if the program doesnt care about the status or if the status isnt what we requested
    if (required_state != CLIENT_CONNECTINGALL && client_state != required_state) {
        // todo: save packets for later use
        AddPacket(app, ack_req);
//        LogFile->write(EQEMuLog::Normal, "Adding Packet to list (%d) (%d)", (*app)->opcode, (int)required_state);
    }
    else {
	    if(eqnc)
            eqnc->FastQueuePacket(app, ack_req);
		else if (app && (*app))
			delete *app;
		*app = 0;
	}
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
//			if ((target != 0) && (DistNoRootNoZ(target) <= 200)) {
//				parse->Event(EVENT_SAY, target->GetNPCTypeID(), message, target, this->CastToMob());
//			}
		entity_list.ChannelMessage(sender, chan_num, language, message);
		
		if (sender != this)
			break;
		
		if (target != 0 && target->IsNPC() && !target->CastToNPC()->IsEngaged()) {
			if (DistNoRootNoZ(*target) <= 200) {
				parse->Event(EVENT_SAY, target->GetNPCTypeID(), message, target, this->CastToMob());
			#ifdef IPC
                if(target->CastToNPC()->IsInteractive()) {
					target->CastToNPC()->InteractiveChat(chan_num,language,message,targetname,this);
				}
			#endif
				//parse->Event(EVENT_SAY, target->GetNPCTypeID(), message, target, this->CastToMob());
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

	APPLAYER app(OP_ChannelMessage, sizeof(ChannelMessage_Struct)+strlen(buffer)+1);
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
	app.Deflate();
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
	APPLAYER* app = new APPLAYER(OP_SpecialMesg, len_packet);
	SpecialMesg_Struct* sm=(SpecialMesg_Struct*)app->pBuffer;
	sm->header[0] = 0x04; // Header used for #emote style messages..
	sm->header[1] = 0x04; // Play around with these to see other types
	sm->header[2] = 0x00;
	sm->msg_type = type;
	sm->target_spawn_id = this->GetID();
	memcpy(sm->message, buffer, strlen(buffer));
	app->Deflate();
	
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

void Client::AddEXP(int32 add_exp, int8 conlevel, bool resexp) {
#ifdef GUILDWARS
	m_pp.perAA = 0;
#endif
	if (m_pp.perAA<0 || m_pp.perAA>100) m_pp.perAA=0;	// stop exploit with sanity check
	int32 add_aaxp = add_exp * m_pp.perAA / 100;
	add_exp -= add_aaxp;
	
	//int lvldiff = my_level - otherlevel;
	
	if (!resexp && zone->GetEXPMod() > 0) {
		int32 factor = 100 * (int32) zone->GetEXPMod();
		add_exp += (add_exp * factor / 10000);
	}
#ifdef CON_XP_SCALING
	if (!resexp && conlevel != 0xFF) {
		switch (conlevel)
		{
		case CON_GREEN:
			//Message(15,"This creature is trivial to you and offers no experience.");
			return;
		case CON_LIGHTBLUE:
				add_exp = add_exp * 2/10;
			break;
		case CON_BLUE:
			//if (lvldiff >= 12)
			//	add_exp = add_exp * 6/10;
			//else if (lvldiff > 5)
				add_exp = add_exp * 8/10;
			//else if (lvldiff > 3)
			//	add_exp = add_exp * 9/10;
			break;
		case CON_WHITE:
				add_exp = add_exp * 125/100;
			break;
		case CON_YELLOW:
				add_exp = add_exp * 150/100;
			break;
		case CON_RED:
				add_exp = add_exp * 200/100;
			break;
		}
		/*
		if (otherlevel >= 65)
		{
			int add = add_exp*((otherlevel-49)*20/100);
			add_exp += add_exp*((otherlevel-64))*2;
			add_exp += add;
		}
		else if (otherlevel >= 50)
		{
			add_exp += add_exp*((otherlevel-49)*20/100);
		}*/
	}
#endif
	
#ifdef FREEBSD
	//Father Nitwit Debug:
	Message(15, "Adding %i experience to your character.", add_exp);
#endif

	if (m_pp.perAA<0 || m_pp.perAA>100)
		m_pp.perAA=0;	// stop exploit with sanity check

	// Old function
	//int32 exp = GetEXP() + (add_exp - add_aaxp);

	// TC - Uses modifier now from variables table.
	int32 exp = GetEXP() + add_exp;

	//int32 aaexp = GetAAXP() + add_aaxp;
	// TC - New function

	int32 aaexp = (int32)((zone->GetAAXPMod()) * add_aaxp);
	if(GetAAXP()<0xFFFFFFFF)
		aaexp+=GetAAXP();
	SetEXP(exp, aaexp, false);
}

void Client::SetEXP(int32 set_exp, int32 set_aaxp, bool isrezzexp) {
	max_AAXP = GetEXPForLevel(52) - GetEXPForLevel(51);
	if (max_AAXP == 0 || GetEXPForLevel(GetLevel()) == 0xFFFFFFFF) {
		Message(13, "Error in Client::SetEXP. EXP not set.");
		return; // Must be invalid class/race
	}
	if ((set_exp + set_aaxp) > m_pp.exp) {
		if (isrezzexp)
			this->Message_StringID(15,REZ_REGAIN);
		else{
			if(this->IsGrouped())
				this->Message_StringID(15,GAIN_GROUPXP);
			else
				this->Message_StringID(15,GAIN_XP);
		}
	}
	else
		Message(15, "You have lost experience.");

#ifdef FREEBSD
//Father Nitwit Debug:
Message(15, "You now have %i experience points.", (set_exp + set_aaxp));
#endif
	
	int16 check_level = GetLevel()+1;
	while (set_exp >= GetEXPForLevel(check_level)) {
		check_level++;
		if (check_level > 100) { // Quagmire - this was happening because GetEXPForLevel returned 0 on unknown race/class combo, Changed it to return 0xFFFFFFFF on error
			check_level = GetLevel()+1;
			break;
		}
	}
	while (set_exp < GetEXPForLevel(check_level-1)) {
		check_level--;
		if (check_level < 2) {
			check_level = 2;
			break;
		}
	}
	
	if (set_aaxp >= max_AAXP) {
		int last_unspentAA = m_pp.aapoints;
		m_pp.aapoints = set_aaxp / max_AAXP;
		set_aaxp = set_aaxp - (max_AAXP * m_pp.aapoints);
		if(set_aaxp <=0) {
			set_aaxp = 0;
		}
		m_pp.expAA = set_aaxp;
		m_pp.aapoints += last_unspentAA;
		set_aaxp = m_pp.expAA % max_AAXP;
		
		//Message(15, "You have gained %d skill points!!", m_pp.aapoints - last_unspentAA);
		char val1[20]={0};
		Message_StringID(15,GAIN_ABILITY_POINT,ConvertArray(m_pp.aapoints,val1),"(s)");
		//Message(15, "You now have %d skill points available to spend.", m_pp.aapoints);
	}
	
	m_pp.expAA = set_aaxp;

	int8 maxlevel = 66;

#ifdef RAIDADDICTS
	maxlevel = raidaddicts.GetZoneLevel();
#endif

	#ifdef GUILDWARS
		if(GuildDBID() == 0)
			maxlevel = NOGUILDCAPLEVEL;
		else
			maxlevel = GAINLEVEL;
	#endif
	if ((GetLevel() != check_level-1) && !(check_level-1 >= maxlevel)) {
		char val1[20]={0};
		if (GetLevel() == check_level-2){
			Message_StringID(15,GAIN_LEVEL,ConvertArray(check_level-1,val1));
			//Message(15, "You have gained a level! Welcome to level %i!", check_level-1);
		}
		if (GetLevel() == check_level){
			Message_StringID(15,LOSE_LEVEL,ConvertArray(check_level-1,val1));
			//Message(15, "You lost a level! You are now level %i!", check_level-1);
		}
		else
			Message(15, "Welcome to level %i!", check_level-1);
		m_pp.exp = set_exp;
		SetLevel(check_level-1);
	}

	//send the expdata in any case so the xp bar isnt stuck after leveling
	APPLAYER* outapp = new APPLAYER(OP_ExpUpdate, sizeof(ExpUpdate_Struct));
	ExpUpdate_Struct* eu = (ExpUpdate_Struct*)outapp->pBuffer;
	int32 tmpxp1 = GetEXPForLevel(GetLevel()+1);
	int32 tmpxp2 = GetEXPForLevel(GetLevel());
	// Quag: crash bug fix... Divide by zero when tmpxp1 and 2 equalled each other, most likely the error case from GetEXPForLevel() (invalid class, etc)
	if (tmpxp1 != tmpxp2 && tmpxp1 != 0xFFFFFFFF && tmpxp2 != 0xFFFFFFFF) {
		double tmpxp = (double) ( (double) set_exp-tmpxp2 ) / ( (double) tmpxp1-tmpxp2 );
		eu->exp = (uint32)(330.0f * tmpxp);
		QueuePacket(outapp);
	}
	safe_delete(outapp);
	m_pp.exp = set_exp;

	if (level<51) m_pp.perAA=0;	// turn off aa exp if they drop below 51

	SendAAStats();
	if (admin>=100 && GetGM()) {
		char val1[20]={0};
		char val2[20]={0};
		char val3[20]={0};
		Message_StringID(15,GM_GAINXP,ConvertArray(set_aaxp,val1),ConvertArray(set_exp,val2),ConvertArray(GetEXPForLevel(GetLevel()+1),val3));
		//Message(15, "[GM] You now have %d / %d EXP and %d / %d AA exp.", set_exp, GetEXPForLevel(GetLevel()+1), set_aaxp, max_AAXP);

	}
}

#ifndef GUILDWARS
bool Client::UpdateLDoNPoints(sint32 points, int32 theme)
{
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
	APPLAYER* outapp = new APPLAYER(OP_AdventurePointsUpdate, sizeof(AdventurePoints_Update_Struct));
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
}
#endif

void Client::MovePC(int32 zoneID, float x, float y, float z, int8 ignorerestrictions, bool summoned)
{
	MovePC(database.GetZoneName(zoneID), x, y, z, ignorerestrictions, summoned);
}

void Client::MovePC(const char* zonename, float x, float y, float z, int8 ignorerestrictions, bool summoned)
{
	//if (this->isgrouped && GetGroup() != 0)
	//	GetGroup()->DelMember(this->CastToMob());
#ifdef GUILDWARS
if(admin == 0)
{
if(database.FindZoneKillTimeStamp(GetName(),zonename))
{
Message(0,"You died too recently there, you may not enter.");
return;
}
}
#endif
	if (IsAIControlled() && zonename == 0) {
		GMMove(x, y, z);
		return;
	}

	zonesummon_ignorerestrictions = ignorerestrictions;
	APPLAYER* outapp = new APPLAYER;

	if (summoned == true) {
		outapp->size = sizeof(GMSummon_Struct);
		outapp->pBuffer = new uchar[outapp->size];
		memset(outapp->pBuffer, 0, outapp->size);
		GMSummon_Struct* gms = (GMSummon_Struct*) outapp->pBuffer;

		strcpy(gms->charname, this->GetName());
		strcpy(gms->gmname, this->GetName());

		outapp->opcode = OP_GMSummon;
		gms->x = (sint32) x;
		gms->y = (sint32) y;
		gms->z = (sint32) z;

		if (zonename == 0) {
			gms->zoneID = zone->GetZoneID();
		}
		else {
			gms->zoneID = database.GetZoneID(zonename);
			strcpy(zonesummon_name, zonename);
			zonesummon_x = x;
			zonesummon_y = y;
			zonesummon_z = z;
		}
		if (gms->zoneID != zone->GetZoneID()) {
			zoning = true;
		}
	}
	else {
		outapp->size = sizeof(GMGoto_Struct);
		outapp->pBuffer = new uchar[outapp->size];
		memset(outapp->pBuffer, 0, outapp->size);
		GMGoto_Struct* gmg = (GMGoto_Struct*) outapp->pBuffer;

		strcpy(gmg->charname, this->GetName());
		strcpy(gmg->gmname, this->GetName());

		outapp->opcode = OP_GMGoto;
		gmg->x = (sint32) x;
		gmg->y = (sint32) y;
		gmg->z = (sint32) z;

		if (zonename == 0) {
            gmg->zoneID = zone->GetZoneID();
        }
        else {
            gmg->zoneID = database.GetZoneID(zonename);
            if (gmg->zoneID == 0)
			{
				Message(0, "Invalid zone name");
				safe_delete(outapp);
				return;
			}
			strcpy(zonesummon_name, zonename);
            zonesummon_x = x;
            zonesummon_y = y;
            zonesummon_z = z;
			if (gmg->zoneID != zone->GetZoneID()) {
				zoning = true;
	        }
	    }
	}

	QueuePacket(outapp);
	safe_delete(outapp);
}

void Client::SetLevel(int8 set_level, bool command)
{
	#ifdef GUILDWARS
		if(set_level > SETLEVEL) {
			Message(0,"You cannot exceed level %i on a GuildWars Server.",SETLEVEL);
			return;
		}
	#endif

	if (GetEXPForLevel(set_level) == 0xFFFFFFFF) {
		LogFile->write(EQEMuLog::Error,"Client::SetLevel() GetEXPForLevel(%i) = 0xFFFFFFFF", set_level);
		return;
	}

	APPLAYER* outapp = new APPLAYER(OP_LevelUpdate, sizeof(LevelUpdate_Struct));
	LevelUpdate_Struct* lu = (LevelUpdate_Struct*)outapp->pBuffer;
	lu->level = set_level;
	lu->level_old = level;
	level = set_level;

	if(set_level > m_pp.level) // Yes I am aware that you could delevel yourself and relevel this is just to test!
		m_pp.points += 5;

	m_pp.level = set_level;
	if (command){
		m_pp.exp = GetEXPForLevel(set_level);
		Message(15, "Welcome to level %i!", set_level);
		lu->exp = 0;
	}
	else {
		double tmpxp = (double) ( (double) m_pp.exp - GetEXPForLevel( GetLevel() )) /
						( (double) GetEXPForLevel(GetLevel()+1) - GetEXPForLevel(GetLevel()));
		lu->exp =  (int32)(330.0f * tmpxp);
    }
	QueuePacket(outapp);
	safe_delete(outapp);
	this->SendAppearancePacket(AT_WhoLevel, set_level); // who level change

    LogFile->write(EQEMuLog::Normal,"Setting Level for %s to %i", GetName(), set_level);

	CalcBonuses();
	SetHP(CalcMaxHP());		// Why not, lets give them a free heal
	SendHPUpdate();
	SetMana(CalcMaxMana());
	UpdateWho();
	Save();
}

//                            hum     bar     eru     elf     hie     def     hef     dwa     tro     ogr     hal    gno     iks,    vah     frog
float  race_modifiers[15] = { 100.0f, 105.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 120.0f, 115.0f, 95.0f, 100.0f, 120.0f, 100.0f, 100.0f}; // Quagmire - Guessed on iks and vah
//                            war   cle    pal    ran    shd    dru    mnk    brd    rog    shm    nec    wiz    mag    enc    bst    bes

float class_modifiers[16] = { 9.0f, 10.0f, 14.0f, 14.0f, 14.0f, 10.0f, 12.0f, 14.0f, 9.05f, 10.0f, 11.0f, 11.0f, 11.0f, 11.0f, 10.0f, 10.0f};


// Note: The client calculates exp separately, we cant change this function
// Add: You can set the values you want now, client will be always sync :) - Merkur
uint32 Client::GetEXPForLevel(int16 check_level)
{
	int16 tmprace = GetBaseRace();
	if (tmprace == IKSAR) // Quagmire, set these up so they read from array right
		tmprace = 12;
	else if (tmprace == VAHSHIR)
		tmprace = 13;
	else if ((tmprace == FROGLOK) || (tmprace == FROGLOK2))
		tmprace = 14;
	else
		tmprace--;

	if (tmprace >= sizeof(race_modifiers) || GetClass() < 1 || GetClass() - 1 >= PLAYER_CLASS_COUNT)
		return 0xFFFFFFFF;

	int16 check_levelm1 = check_level-1;
	if (check_level < 31)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]);
	else if (check_level < 36)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*1.1);
	else if (check_level < 41)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*1.2);
	else if (check_level < 46)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*1.3);
	else if (check_level < 52)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*1.4);
	else if (check_level < 53)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*1.5);
	else if (check_level < 54)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*1.6);
	else if (check_level < 55)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*1.7);
	else if (check_level < 56)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*1.9);
	else if (check_level < 57)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*2.1);
	else if (check_level < 58)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*2.3);
	else if (check_level < 59)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*2.5);
	else if (check_level < 60)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*2.7);
	else if (check_level < 61)
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*3.0);
	else
		return (uint32)((check_levelm1)*(check_levelm1)*(check_levelm1)*class_modifiers[GetClass()-1]*race_modifiers[tmprace]*3.1);
}

void Client::SetSkill(int skillid, int8 value) {
	if (skillid > HIGHEST_SKILL)
		return;
	m_pp.skills[skillid + 1] = value; // We need to be able to #setskill 254 and 255 to reset skills

	if(value <= 252) {
		APPLAYER* outapp = new APPLAYER(OP_SkillUpdate, sizeof(SkillUpdate_Struct));
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
	APPLAYER* outapp = new APPLAYER(0x01a6, 68);
	unsigned char x[68];
	memset(x, 0, 68);
	x[0]=0x22;
	x[4]=0x8002;
	x[8]=0x8624;
	x[12]=0x4A01;
	x[16]=0x05;
	x[28]=0x00;//change this value to give gold to the client
        x[40]=0xFFFFFFFF;
        x[44]=0xFFFFFFFF;
        x[48]=0xFFFFFFFF;
        x[52]=0xFFFFFFFF;
        x[56]=0xFFFFFFFF;
        x[60]=0xFFFFFFFF;
        x[64]=0xffffffff;
	outapp->pBuffer=x;
	outapp->priority = 2;
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
	APPLAYER* outapp = new APPLAYER(OP_IncreaseStats,sizeof(IncreaseStat_Struct));
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
	APPLAYER* outapp = new APPLAYER(OP_ManaChange, sizeof(ManaChange_Struct));
	ManaChange_Struct* manachange = (ManaChange_Struct*)outapp->pBuffer;
	manachange->new_mana = cur_mana;
	manachange->stamina = 6000;
	manachange->spell_id = casting_spell_id;
	outapp->priority = 6;
	QueuePacket(outapp);
	safe_delete(outapp);
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
	if ((inst = m_inv[SLOT_HANDS]) && inst->IsType(ItemTypeCommon)) {
		item = inst->GetItem();
		ns->spawn.equipment[MATERIAL_HANDS]	= item->Common.Material;
		ns->spawn.dye_rgb[MATERIAL_HANDS].color	= item->Common.Color;
	}
	if ((inst = m_inv[SLOT_HEAD]) && inst->IsType(ItemTypeCommon)) {
		item = inst->GetItem();
		ns->spawn.equipment[MATERIAL_HEAD]	= item->Common.Material;
		ns->spawn.dye_rgb[MATERIAL_HEAD].color	= item->Common.Color;
	}
	if ((inst = m_inv[SLOT_ARMS]) && inst->IsType(ItemTypeCommon)) {
		item = inst->GetItem();
		ns->spawn.equipment[MATERIAL_ARMS]	= item->Common.Material;
		ns->spawn.dye_rgb[MATERIAL_ARMS].color	= item->Common.Color;
	}
	if ((inst = m_inv[SLOT_BRACER01]) && inst->IsType(ItemTypeCommon)) {
		item = inst->GetItem();
		ns->spawn.equipment[MATERIAL_BRACER]= item->Common.Material;
		ns->spawn.dye_rgb[MATERIAL_BRACER].color	= item->Common.Color;
	}
	if ((inst = m_inv[SLOT_BRACER02]) && inst->IsType(ItemTypeCommon)) {
		item = inst->GetItem();
		ns->spawn.equipment[MATERIAL_BRACER]= item->Common.Material;
		ns->spawn.dye_rgb[MATERIAL_BRACER].color	= item->Common.Color;
	}
	if ((inst = m_inv[SLOT_CHEST]) && inst->IsType(ItemTypeCommon)) {
		item = inst->GetItem();
		ns->spawn.equipment[MATERIAL_CHEST]	= item->Common.Material;
		ns->spawn.dye_rgb[MATERIAL_CHEST].color	= item->Common.Color;
	}
	if ((inst = m_inv[SLOT_LEGS]) && inst->IsType(ItemTypeCommon)) {
		item = inst->GetItem();
		ns->spawn.equipment[MATERIAL_LEGS]	= item->Common.Material;
		ns->spawn.dye_rgb[MATERIAL_LEGS].color	= item->Common.Color;
	}
	if ((inst = m_inv[SLOT_FEET]) && inst->IsType(ItemTypeCommon)) {
		item = inst->GetItem();
		ns->spawn.equipment[MATERIAL_FEET]	= item->Common.Material;
		ns->spawn.dye_rgb[MATERIAL_FEET].color	= item->Common.Color;
	}
	if ((inst = m_inv[SLOT_PRIMARY]) && inst->IsType(ItemTypeCommon)) {
		item = inst->GetItem();
		if (strlen(item->IDFile) > 2)
			ns->spawn.equipment[MATERIAL_PRIMARY] = atoi(&item->IDFile[2]);
	}
	if ((inst = m_inv[SLOT_SECONDARY]) && inst->IsType(ItemTypeCommon)) {
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
	APPLAYER* outapp = new APPLAYER(OP_GMLastName, sizeof(GMLastName_Struct));
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
	APPLAYER* outapp = new APPLAYER(OP_GMNameChange, sizeof(GMName_Struct));
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

void Client::ReadBook(char txtfile[20]) {
	if(txtfile[0] == '0' && txtfile[1] == '\0') {
		//invalid book... coming up on non-book items.
		return;
	}
	
	string booktxt2=database.GetBook(txtfile);
	int length=strlen(booktxt2.c_str())+3;
	char booktxt[5000]={0};//booktxt2.c_str();
	strcpy(booktxt,booktxt2.c_str());
	if (booktxt[0] != '\0') {
		//char *buffer=(char*)malloc(length);
		//char *bufptr=buffer;
		uchar *buffer=new uchar[length];
		uchar *bufptr=buffer;
		LogFile->write(EQEMuLog::Normal,"Client::ReadBook() textfile:%s Text:%s", txtfile, booktxt);
		APPLAYER* outapp = new APPLAYER(OP_ReadBook,length);
		int16 unknown0=0x00FF;
		outapp->pBuffer=new uchar[(outapp->size)];
		memset(buffer,0,length);
		memcpy(bufptr,&unknown0, sizeof(int16));
		bufptr+=sizeof(int16);
		memcpy(bufptr,&booktxt,strlen(booktxt));
		bufptr+=strlen(booktxt);
		memcpy(outapp->pBuffer, buffer, outapp->size);
		QueuePacket(outapp);
		safe_delete(outapp);
		//free(buffer);
		safe_delete_array(outapp);
	}
}

void Client::SendClientMoneyUpdate(int8 type,int32 amount){
	APPLAYER* outapp = new APPLAYER(OP_TradeMoneyUpdate,sizeof(TradeMoneyUpdate_Struct));
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
	APPLAYER* outapp = new APPLAYER(OP_GMKick, sizeof(GMKick_Struct));
	GMKick_Struct* gmk = (GMKick_Struct *)outapp->pBuffer;
	strcpy(gmk->name,GetName());
	QueuePacket(outapp);
	safe_delete(outapp);
	Kick();
}

void Client::GMKill() {
	APPLAYER* outapp = new APPLAYER(OP_GMKill, sizeof(GMKill_Struct));
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
	APPLAYER* outapp = new APPLAYER(OP_MemorizeSpell,sizeof(MemorizeSpell_Struct));
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
		itoa(item->ItemNumber,itemid,10);
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
		itoa(item->ItemNumber,itemid,10);
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
		itoa(item->ItemNumber,itemid,10);
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
	APPLAYER* outapp = 0;
	if(!fail) {
		outapp = new APPLAYER(OP_Bind_Wound, sizeof(BindWound_Struct));
		BindWound_Struct* bind_out = (BindWound_Struct*) outapp->pBuffer;
		// Start bind
		if(!bindwound_timer.Enabled()) {
			//make sure we actually have a bandage... and consume it.
			sint16 bslot = m_inv.HasItemByUse(ItemUseBandage, 1, invWhereWorn|invWherePersonal);
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
					
					int max_hp = bindmob->GetMaxHP()*max_percent/100;
					
					// send bindmob new hp's
					if (bindmob->GetHP() < bindmob->GetMaxHP() && bindmob->GetHP() <= (max_hp)-1){
						// 0.120 per skill point, 0.60 per skill level, minimum 3 max 30
						int bindhps = 3;

						if (GetSkill(BIND_WOUND) >= 10) {
							bindhps += GetSkill(BIND_WOUND)*12/100;
						}
						
						if (bindhps > 30){
							bindhps = 30;
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
		outapp = new APPLAYER(OP_Bind_Wound, sizeof(BindWound_Struct));
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
	if (item && (item->ItemClass==ItemTypeCommon)) {
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
	APPLAYER* outapp = new APPLAYER(OP_SimpleMessage,12);
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
	
	APPLAYER* outapp = new APPLAYER(OP_FormattedMessage, length+13);
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

void Client::SendTribute(){
	//TODO: Setup table and pull all tributes from it
	const char* name="Antidote";
	APPLAYER* outapp = new APPLAYER(OP_Tribute,sizeof(TributeAbility_Struct)+strlen(name)+1);
	TributeAbility_Struct* tas = (TributeAbility_Struct*)outapp->pBuffer;
	tas->list_id=htonl(0);
	tas->tribute[0].cost=htonl(5);
	tas->tribute[0].level=htonl(20);
	tas->tribute[0].tribute_id=htonl(56300);
	tas->tribute[1].cost=htonl(7);
	tas->tribute[1].level=htonl(30);
	tas->tribute[1].tribute_id=htonl(56301);
	tas->tribute[2].cost=htonl(10);
	tas->tribute[2].level=htonl(40);
	tas->tribute[2].tribute_id=htonl(56302);
	tas->tribute[3].cost=htonl(14);
	tas->tribute[3].level=htonl(50);
	tas->tribute[3].tribute_id=htonl(56303);
	tas->tribute[4].cost=htonl(18);
	tas->tribute[4].level=htonl(60);
	tas->tribute[4].tribute_id=htonl(56304);
	tas->tribute[5].cost=htonl(23);
	strcpy(tas->name,name);
	QueuePacket(outapp);
	//DumpPacket(outapp);
	safe_delete(outapp);
}

void Client::SetHideMe(bool flag)
{
	APPLAYER app;

	gmhideme = flag;

	if(gmhideme)
	{
		CreateDespawnPacket(&app);
		entity_list.RemoveFromTargets(this);
	}
	else
	{
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
	APPLAYER* outapp = new APPLAYER(0x0202,12);
	uint8 rawData0[12] = { 0x5A, 0x01, 0x00, 0x00, 0x0E, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	memcpy(outapp->pBuffer,rawData0,12);
	QueuePacket(outapp);
	safe_delete(outapp);
	outapp = new APPLAYER(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
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
	if (!CheckLos(who))
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






