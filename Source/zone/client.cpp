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
#include "command.h"
#include "StringIDs.h"

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

#define ITEM_MAX_STACK 20
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

	)
{
	for(int cf=0;cf<21;cf++)
		ClientFilters[cf]=0;
	character_id = 0;
	client_data_loaded = false;
	feigned = false;
	this->berserk = false;
	this->dead = false;
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
	LDTimer = new Timer(30000);
	LDTimer->Disable();
	zonesummon_x = -2;
	zonesummon_y = -2;
	zonesummon_z = -2;
	zonesummon_ignorerestrictions = 0;
	casting_spell_id = 0;
	npcflag = false;
	npclevel = 0;
	pQueuedSaveWorkID = 0;
	stamina_timer = new Timer(46000);
	position_timer = new Timer(250);
	position_timer->Disable();
	hpregen_timer = new Timer(1800);
	position_timer_counter = 0;
	camp_timer = new Timer(29000);
	ooc_timer = new Timer(1000);
	process_timer = new Timer(100);
	dead_timer = new Timer(2000);
	dead_timer->Disable();
	camp_timer->Disable();
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
	
	disc_timer = new Timer(60000);
	disc_timer->Disable();
	disc_elapse = new Timer(60000);
	disc_elapse->Disable();
	disc_inuse=0;
	pr = new PRange_Struct;
	pr->p1set=false;
	pr->p2set=false;
	pr->p3set=false;
	pr->p4set=false;
}

Client::~Client() {
	Mob* horse = entity_list.GetMob(this->CastToClient()->GetHorseId());
	if (horse)
		horse->Depop();
	if(Trader)
		database.DeleteTraderItem(this->CharacterID());
	Object* object=GetTradeskillObject();
	if(object)
		object->Close();

	if(AbilityTimer || GetLevel()>=51)
		database.UpdateAndDeleteAATimers(CharacterID());

	if(IsDueling() && GetDuelTarget() != 0) {
		Entity* entity = entity_list.GetID(GetDuelTarget());
		if(entity != NULL && entity->IsClient()) {
			entity->CastToClient()->SetDueling(false);
			entity->CastToClient()->SetDuelTarget(0);
		}
	}
	
	if (this->isgrouped && entity_list.GetGroupByClient(this) != NULL)
		entity_list.GetGroupByClient(this)->Remove(this->CastToMob());
	//	entity_list.GetGroupByClient(this)->DelMember(this->CastToMob(),true);
	
	eqnc->Free();
	UpdateWho(2);
	// we save right now, because the client might be zoning and the world
	// will need this data right away
	Save(2); // This fails when database destructor is called first on shutdown	
	safe_delete(position_timer);
	safe_delete(hpregen_timer);
	safe_delete(camp_timer);
	safe_delete(process_timer);
	safe_delete(disc_timer);
	safe_delete(disc_elapse);
	safe_delete(stamina_timer);
	safe_delete(LDTimer);
	safe_delete(ooc_timer);
	safe_delete(dead_timer);
	safe_delete(pr);
	numclients--;
	UpdateWindowTitle();
#ifdef GUILDWARS
	guildwars.SetCurrentUsers(numclients);
#endif
	zone->RemoveAuth(GetName());
}

// Return max stat value for level
sint16 Client::GetMaxStat() {
	int level = GetLevel();
	
	if (level < 61)
		return 255;
	else if (level < 71)
		return 255 + 5 * (level - 60);
	else
		return 280;	
}

sint16 Client::GetMaxSTR() {
	return GetMaxStat();
}
sint16 Client::GetMaxSTA() {
	return GetMaxStat();
}
sint16 Client::GetMaxDEX() {
	return GetMaxStat();
}
sint16 Client::GetMaxAGI() {
	return GetMaxStat();
}
sint16 Client::GetMaxINT() {
	return GetMaxStat();
}
sint16 Client::GetMaxWIS() {
	return GetMaxStat();
}
sint16 Client::GetMaxCHA() {
	return GetMaxStat();
}

bool Client::GetIncreaseSpellDurationItem(int16 &spell_id, char *itemname)
{
	for (int i=0; i<22; i++) {
		const ItemInst* inst = m_inv[i];
		if (!inst || !inst->IsType(ItemTypeCommon))
			continue;
	    
		const Item_Struct* item = inst->GetItem();
		if (item->Common.FocusId && (item->Common.FocusId != 0xFFFF)) {
			if (IsIncreaseDurationSpell(item->Common.FocusId)) {
				spell_id = item->Common.FocusId;
				if (itemname)
					strcpy(itemname, item->Name);
				return true;
			}
		}
	}
	return false;
}

bool Client::GetReduceManaCostItem(int16 &spell_id, char *itemname)
{
	for (int i=0; i<22; i++) {
		const ItemInst* inst = m_inv[i];
		if (!inst || !inst->IsType(ItemTypeCommon))
			continue;
	    
		const Item_Struct* item = inst->GetItem();
		if (item->Common.FocusId && (item->Common.FocusId != 0xFFFF)) {
			if (IsReduceManaSpell(item->Common.FocusId)) {
				spell_id = item->Common.FocusId;
				if (itemname)
					strcpy(itemname, item->Name);
				return true;
			}
		}
	}
	return false;
}

bool Client::GetReduceCastTimeItem(int16 &spell_id, char *itemname)
{
	for (int i=0; i<22; i++) {
		const ItemInst* inst = m_inv[i];
		if (!inst || !inst->IsType(ItemTypeCommon))
			continue;
	    
		const Item_Struct* item = inst->GetItem();
		if (item->Common.FocusId && (item->Common.FocusId != 0xFFFF)) {
			if (IsReduceCastTimeSpell(item->Common.FocusId)) {
				spell_id = item->Common.FocusId;
				if (itemname)
					strcpy(itemname, item->Name);
				return true;
			}
		}
	}
	return false;
}

bool Client::GetExtendedRangeItem(int16 &spell_id, char *itemname)
{
	for (int i=0; i<22; i++) {
		const ItemInst* inst = m_inv[i];
		if (!inst || !inst->IsType(ItemTypeCommon))
			continue;
	    
		const Item_Struct* item = inst->GetItem();
		if (item->Common.FocusId && (item->Common.FocusId != 0xFFFF)) {
			if (IsExtRangeSpell(item->Common.FocusId)) {
				spell_id = item->Common.FocusId;
				if (itemname)
					strcpy(itemname, item->Name);
				return true;
			}
		}
	}
	return false;
}

bool Client::GetImprovedHealingItem(int16 &spell_id, char *itemname)
{
	for (int i=0; i<22; i++) {
		const ItemInst* inst = m_inv[i];
		if (!inst || !inst->IsType(ItemTypeCommon))
			continue;
	    
		const Item_Struct* item = inst->GetItem();
		if (item->Common.FocusId && (item->Common.FocusId != 0xFFFF)) {
			if (IsImprovedHealingSpell(item->Common.FocusId)) {
				spell_id = item->Common.FocusId;
				if (itemname)
					strcpy(itemname, item->Name);
				return true;
			}
		}
	}
	return false;
}

bool Client::GetImprovedDamageItem(int16 &spell_id, char *itemname)
{
	for (int i=0; i<22; i++) {
		const ItemInst* inst = m_inv[i];
		if (!inst || !inst->IsType(ItemTypeCommon))
			continue;
	    
		const Item_Struct* item = inst->GetItem();
		if (item->Common.FocusId && (item->Common.FocusId != 0xFFFF)) {
			if (IsImprovedDamageSpell(item->Common.FocusId)) {
				spell_id = item->Common.FocusId;
				if (itemname)
					strcpy(itemname, item->Name);
				return true;
			}
		}
	}
	return false;
}

sint32 Client::GenericFocus(int16 spell_id, int16 modspellid)
{
	int modifier = 100, i;
	SPDat_Spell_Struct spell = spells[spell_id];
	SPDat_Spell_Struct modspell = spells[modspellid];
    
	for (i = 0; i < EFFECT_COUNT; i++)
	{
		if(IsBlankSpellEffect(modspellid, i))
			continue;
		switch( spells[modspellid].effectid[i] )
		{
			case SE_LimitMaxLevel:
				if (spell.classes[(GetClass()%16) - 1] > modspell.base[i])
					return 100;
				break;
			case SE_LimitMinLevel:
				if (spell.classes[(GetClass()%16) - 1] < modspell.base[i])
					return 100;
				break;
			case SE_IncreaseRange:
				modifier += modspell.base[i];
				break;
			case SE_IncreaseSpellHaste:
				modifier -= modspell.base[i];
				break;
			case SE_IncreaseSpellDuration:
				modifier += modspell.base[i];
				break;
			case SE_LimitSpell:
				// negative sign means exclude
				// positive sign means include
				if (modspell.base[i] < 0)
				{
					if (modspell.base[i] * (-1) == spell_id)
						return 100;
				}
				else
				{
					if (spells[modspellid].base[i] != spell_id)
						return 100;
				}
				break;
			case SE_LimitEffect:
				switch( spells[modspellid].base[i] )
				{
					case -147:
						if (IsPercentalHealSpell(spell_id))
							return 100;
						break;
					case -101:
						if (IsCHDurationSpell(spell_id))
							return 100;
						break;
					case -40:
						if (IsInvulnerabilitySpell(spell_id))
							return 100;
						break;
					case -32:
						if (IsSummonItemSpell(spell_id))
							return 100;
						break;
					case 0:
						if (!IsEffectHitpointsSpell(spell_id))
							return 100;
						break;
					case 33:
						if (!IsSummonPetSpell(spell_id))
							return 100;
						break;
					case 36:
						if (!IsPoisonCounterSpell(spell_id))
							return 100;
						break;
					case 71:
						if (!IsSummonSkeletonSpell(spell_id))
							return 100;
						break;
					default:
						LogFile->write(EQEMuLog::Normal, "GenericFocus:  unknown limit effect %d", spells[modspellid].base[i]);
				}
				break;
			case SE_LimitCastTime:
				if (modspell.base[i] > (sint16)spell.cast_time)
					return 100;
				break;
			case SE_LimitSpellType:
				switch( spells[modspellid].base[i] )
				{
					case 0:
						if (!IsDetrimentalSpell(spell_id))
							return 100;
						break;
					case 1:
						if (!IsBeneficialSpell(spell_id))
							return 100;
						break;
					default:
						LogFile->write(EQEMuLog::Normal, "GenericFocus:  unknown limit spelltype %d", spells[modspellid].base[i]);
				}
				break;
			case SE_LimitMinDur:
				if (modspell.base[i] > CalcBuffDuration_formula(GetLevel(), spell.buffdurationformula, spell.buffduration))
					return 100;
				break;
			case SE_ImprovedDamage:
			case SE_ImprovedHeal:
				modifier += modspell.base[i];
				break;
			case SE_ReduceManaCost:
				modifier -= modspell.base[i];
				break;
			default:
				LogFile->write(EQEMuLog::Normal, "GenericFocus:  unknown effectid %d", modspell.effectid[i]);
		}
	}

	return modifier;
}

float Client::GetActSpellRange(int16 spell_id, float range)
{
	int16 modspellid = 0;
	float extrange = 100;

	if (GetExtendedRangeItem(modspellid, NULL)) {
		extrange = GenericFocus(spell_id, modspellid);
	}
	return (range * extrange) / 100;
}

sint32 Client::GetActSpellValue(int16 spell_id, sint32 value)
{
	int16 modspellid = 0;

	int modifier = 100;

    if (spells[spell_id].goodEffect == 0) {
		if (GetImprovedDamageItem(modspellid, NULL))
			modifier = GenericFocus(spell_id, modspellid);
	}
	else {
		if (GetImprovedHealingItem(modspellid, NULL))
			modifier = GenericFocus(spell_id, modspellid);
	}
	return (value * modifier) / 100;
}
sint32 Client::GetDotFocus(int16 spell_id, sint32 value)
{
	int16 modspellid = 0;

	int modifier = 100;
	

	if (GetImprovedDamageItem(modspellid, NULL)) {
			modifier = GenericFocus(spell_id, modspellid);
	}
	int randamount = MakeRandomInt(1, (100-modifier));
	return (value * (100-randamount)) / 100;
}

sint32 Client::GetActSpellCost(int16 spell_id, sint32 cost)
{
	int16 modspellid = 0;

	int reduce = 100;
	if (GetReduceManaCostItem(modspellid, NULL)) {
		reduce = GenericFocus(spell_id, modspellid);
	}
	
	int spec_skill = GetSpecializeSkill(spell_id);
	uint32 spec_value = spec_skill < HIGHEST_SKILL ? GetSkill(spec_skill) : 0;
		//VERY rough success formula, needs research
	if(spec_value > 0 && (((spec_value+98)/6) < (uint32)MakeRandomInt(0, 100))) {
		reduce -= spec_value * SPECIALIZE_MANA_REDUCE / 200;
	}
	//arbitrary rule: cannot reduce it below 10%
	if(reduce < 10)
		reduce = 10;
	return (cost * reduce) / 100;
}

sint32 Client::GetActSpellDuration(int16 spell_id, sint32 duration)
{
	int16 modspellid = 0;

	int increase = 100;
	if (GetIncreaseSpellDurationItem(modspellid, NULL)) {
		increase = GenericFocus(spell_id, modspellid);
	}
	return (duration * increase) / 100;
}

sint32 Client::GetActSpellCasttime(int16 spell_id, sint32 casttime)
{
	int16 modspellid = 0;

	int reduce = 100;
	if (GetReduceCastTimeItem(modspellid, NULL)) {
		reduce = GenericFocus(spell_id, modspellid);
	}
	return (casttime * reduce) / 100;
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
	for(int a=0;a<MAX_AAS;a++){
		if(aa.aa_list[a].aa_value>1)
			m_pp.aa_array[a].AA=aa.aa_list[a].aa_skill+aa.aa_list[a].aa_value-1;
		else
			m_pp.aa_array[a].AA=aa.aa_list[a].aa_skill;
		m_pp.aa_array[a].value=aa.aa_list[a].aa_value;
		spentpoints+=aa.aa_list[a].aa_value;
	}
	m_pp.aapoints_spent=spentpoints;
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
		if (buffs[i].spellid != 0xFFFF) {
			m_pp.buffs[i].spellid = buffs[i].spellid;
// solar: fix this if buffs struct is fixed
			m_pp.buffs[i].slotid = 2;
			m_pp.buffs[i].duration = buffs[i].ticsremaining;
			m_pp.buffs[i].level = buffs[i].casterlevel;
			m_pp.buffs[i].effect = 10;
		}
		else {
			m_pp.buffs[i].spellid = 0;
			m_pp.buffs[i].duration = 0;
			m_pp.buffs[i].level = 0;
			m_pp.buffs[i].effect = 0;
		}
	}
	if (pQueuedSaveWorkID) {
		dbasync->CancelWork(pQueuedSaveWorkID);
		pQueuedSaveWorkID = 0;
	}
	
	//FatherNitwit: I dont know if there is a better place for this:
	p_timers.Store();
	
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


sint32 Client::LevelRegen()
{
	sint32 hp = 0;
	if (GetLevel() <= 19) {
		if(IsSitting())
			hp+=2;
		else
			hp+=1;
	}
	else if(GetLevel() <= 49) {
		if(IsSitting())
			hp+=3;
		else
			hp+=1;
	}
	else if(GetLevel() == 50) {
		if(IsSitting())
			hp+=4;

		else
			hp+=1;
	}
	else if(GetLevel() >= 51) {
		if(IsSitting())
			hp+=5;
		else
			hp+=2;
	}
	if(GetRace() == IKSAR || GetRace() == TROLL) {
		if (GetLevel() <= 19) { // 1 4
			if(IsSitting())
				hp+=2;
		}
		else if(GetLevel() <= 49) { // 2 6
			if(IsSitting())
				hp+=3;
			else
				hp+=1;
		}
		else if(GetLevel() >= 50) { // 4 12
			if(IsSitting())
				hp+=7;
			else
				hp+=2;
		}
	}
	if (GetAA(225) >= 1){
		hp += GetAA(225);
	}
	if (GetAA(29) >= 1){
		hp += GetAA(29);
	}
	
	return hp;
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
        LogFile->write(EQEMuLog::Normal, "Adding Packet to list (%d) (%d)", app->opcode, (int)required_state);
    }
    else
	    if(eqnc)
            eqnc->QueuePacket(app, ack_req);
}

void Client::FastQueuePacket(APPLAYER** app, bool ack_req, CLIENT_CONN_STATUS required_state) {
	if (app != 0 && (*app) != 0) {
		if ((*app)->size >= 31500) {
			cout << "WARNING: abnormal packet size. n='" << this->GetName() << "', o=0x" << hex << (*app)->opcode << dec << ", s=" << (*app)->size << endl;
		}
	}
	
	//cout << "Sending: 0x" << hex << setw(4) << setfill('0') << (*app)->opcode << dec << ", size=" << (*app)->size << endl;
	
	// if the program doesnt care about the status or if the status isnt what we requested
    if (required_state != CLIENT_CONNECTINGALL && client_state != required_state) {
        // todo: save packets for later use
        AddPacket(app, ack_req);
        LogFile->write(EQEMuLog::Normal, "Adding Packet to list (%d) (%d)", (*app)->opcode, (int)required_state);
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
		if (this->isgrouped && group != 0) {
			group->GroupMessage(this,(const char*) message);
		}
		break;
	}
	case 3: // Shout
	case 4: { // Auction
		entity_list.ChannelMessage(this, chan_num, language, message);
		break;
	}
	case 5: { // OOC
		if(!ooc_timer->Check())
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
		if(message[0] == COMMAND_CHAR) 
			command_dispatch(this, message);
		else
		{
			printf("Message: %s\n",message);
//			if ((target != 0) && (DistNoRootNoZ(target) <= 200)) {
//				parse->Event(EVENT_SAY, target->GetNPCTypeID(), message, target, this->CastToMob());
//			}
			entity_list.ChannelMessage(this, chan_num, language, message);
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

void Client::AddEXP(int32 add_exp) {
	if (m_pp.perAA<0 || m_pp.perAA>100) m_pp.perAA=0;	// stop exploit with sanity check
	int32 add_aaxp = (int32)((float)add_exp * ((float)(m_pp.perAA) / 100.0f));

	// Old function
	//int32 exp = GetEXP() + (add_exp - add_aaxp);

	// TC - Uses modifier now from variables table.
	int32 exp = GetEXP() + (int32)((zone->GetEXPMod()) * (add_exp - add_aaxp));

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

	int8 maxlevel = 71;

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

void Client::MovePC(int32 zoneID, float x, float y, float z, int8 ignorerestrictions, bool summoned)
{
	MovePC(database.GetZoneName(zoneID), x, y, z, ignorerestrictions, summoned);
}

void Client::MovePC(const char* zonename, float x, float y, float z, int8 ignorerestrictions, bool summoned)
{
	//if (this->isgrouped && entity_list.GetGroupByClient(this) != 0)
	//	entity_list.GetGroupByClient(this)->DelMember(this->CastToMob());
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

sint32 Client::CalcMaxHP() {
	max_hp = (CalcBaseHP() + itembonuses->HP + spellbonuses->HP);
	if (GetAA(28) != 0) {
		if (GetAA(28) == 1)
			max_hp += (sint32)(max_hp * ((GetAA(120)!=0) ? 4:2))/100;
		if (GetAA(28) == 2)
			max_hp += (sint32)(max_hp * ((GetAA(120)!=0) ? 7:5))/100;
		if (GetAA(28) == 3)
			max_hp += (sint32)(max_hp * ((GetAA(120)!=0) ? 12:10))/100;
	}

	if (cur_hp > max_hp)
		cur_hp = max_hp;
	return max_hp;
}

// Note: The client calculates max hp separatly, we cant change this function
sint32 Client::CalcBaseHP()
{
	int8 multiplier=GetClassLevelFactor();

	if (multiplier == 0) {
		cerr << "Multiplier == 0 in Client::CalcBaseHP, Using Generic..." << endl;
		multiplier=12;
	}

	#if EQDEBUG >= 11
		LogFile->write(EQEMuLog::Debug,"Client::CalcBaseHP() multiplier:%i level:%i sta:%i", multiplier, GetLevel(), GetSTA());
	#endif
int16 sta = GetSTA();
if(sta > 305)
sta = 305;
	base_hp = 5+multiplier*GetLevel()+multiplier*GetLevel()*sta/300;
	return base_hp;
}

void Client::SetSkill(int skillid, int value) {
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

void Client::AddSkill(int skillid, int value) {
	if (skillid > HIGHEST_SKILL)
		return;
	value = GetSkill(skillid) + value;
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

// This should return the combined AC of all the items the player is wearing.
sint16 Client::GetRawItemAC() {
	sint16 Total = 0;
	
	for (sint16 slot_id=0; slot_id<21; slot_id++) {
		const ItemInst* inst = m_inv[slot_id];
		if (inst && inst->IsType(ItemTypeCommon)) {
			Total += inst->GetItem()->Common.AC;
		}
	}
	
	return Total;
}

sint16 Client::acmod() {
	int agility = GetAGI();
	int level = GetLevel();
	if (agility >=1 && agility <=74){
		if (agility == 1)
			return -24;
		else if (agility >=2 && agility <=3)
			return -23;
		else if (agility == 4)
			return -22;
		else if (agility >=5 && agility <=6)
			return -21;
		else if (agility >=7 && agility <=8)
			return -20;
		else if (agility == 9)
			return -19;
		else if (agility >=10 && agility <=11)
			return -18;
		else if (agility == 12)
			return -17;
		else if (agility >=13 && agility <=14)
			return -16;
		else if (agility >=15 && agility <=16)
			return -15;
		else if (agility == 17)
			return -14;
		else if (agility >=18 && agility <=19)
			return -13;
		else if (agility == 20)
			return -12;
		else if (agility >=21 && agility <=22)
			return -11;
		else if (agility >=23 && agility <=24)
			return -10;
		else if (agility == 25)
			return -9;
		else if (agility >=26 && agility <=27)
			return -8;
		else if (agility == 28)
			return -7;
		else if (agility >=29 && agility <=30)
			return -6;
		else if (agility >=31 && agility <=32)
			return -5;
		else if (agility == 33)
			return -4;
		else if (agility >=34 && agility <=35)
			return -3;
		else if (agility == 36)
			return -2;
		else if (agility >=37 && agility <=38)
			return -1;
		else if (agility >=39 && agility <=65)
			return 0;
		else if (agility >=66 && agility <=70)
			return 1;
		else if (agility >=71 && agility <=74)
			return 5;
	}
	else {
		if (agility == 75){
			if (level >= 1 && level <= 6)
				return 9;
			else if (level >= 7 && level <= 19)
				return 23;
			else if (level >= 20 && level <= 39)
				return 33;
			else if (level >= 40)
				return 39;
		}
		else if (agility >= 76 && agility <= 79){
			if (level >= 1 && level <= 6)
				return 10;
			else if (level >= 7 && level <= 19)
				return 23;
			else if (level >= 20 && level <= 39)
				return 33;
			else if (level >= 40)
				return 40;
		}
		else if (agility == 80){
			if (level >= 1 && level <= 6)
				return 11;
			else if (level >= 7 && level <= 19)
				return 24;
			else if (level >= 20 && level <= 39)
				return 34;
			else if (level >= 40)
				return 41;
		}
		else if (agility >= 81 && agility <= 85){
			if (level >= 1 && level <= 6)
				return 12;
			else if (level >= 7 && level <= 19)
				return 25;
			else if (level >= 20 && level <= 39)
				return 35;
			else if (level >= 40)
				return 42;
		}
		else if (agility >= 86 && agility <= 90){
			if (level >= 1 && level <= 6)
				return 12;
			else if (level >= 7 && level <= 19)
				return 26;
			else if (level >= 20 && level <= 39)
				return 36;
			else if (level >= 40)
				return 42;
		}
		else if (agility >= 91 && agility <= 95){
			if (level >= 1 && level <= 6)
				return 13;
			else if (level >= 7 && level <= 19)
				return 26;
			else if (level >= 20 && level <= 39)
				return 36;
			else if (level >= 40)
				return 43;
		}
		else if (agility >= 96 && agility <= 99){
			if (level >= 1 && level <= 6)
				return 14;
			else if (level >= 7 && level <= 19)
				return 27;
			else if (level >= 20 && level <= 39)
				return 37;
			else if (level >= 40)
				return 44;
		}
		else if (agility == 100 && level >= 7){
			if (level >= 7 && level <= 19)
				return 28;
			else if (level >= 20 && level <= 39)
				return 38;
			else if (level >= 40)
				return 45;
		}
		else if (level >= 1 && level <= 6) {
			return 15;
		}
		else if (agility >= 101 && agility <= 105){
			if (level >= 7 && level <= 19)
				return 29;
			else if (level >= 20 && level <= 39)
				return 39;// not verified
			else if (level >= 40)
				return 45;
		}
		else if (agility >= 106 && agility <= 110){
			if (level >= 7 && level <= 19)
				return 29;
			else if (level >= 20 && level <= 39)
				return 39;// not verified
			else if (level >= 40)
				return 46;
		}
		else if (agility >= 111 && agility <= 115){
			if (level >= 7 && level <= 19)
				return 30;
			else if (level >= 20 && level <= 39)
				return 40;// not verified
			else if (level >= 40)
				return 47;
		}
		else if (agility >= 116 && agility <= 119){
			if (level >= 7 && level <= 19)
				return 31;
			else if (level >= 20 && level <= 39)
				return 41;
			else if (level >= 40)
				return 47;
		}
		else if (agility == 120 && level >= 20){
			if (level >= 20 && level <= 39)
				return 42;
			else if (level >= 40)
				return 48;
		}
		else if (level >= 7 && level <= 19) {
				return 32;
		}
		else if (agility >= 121 && agility <= 125){
			if (level >= 20 && level <= 39)
				return 42;
			else if (level >= 40)
				return 49;
		}
		else if (agility >= 126 && agility <= 135){
			if (level >= 20 && level <= 39)
				return 42;
			else if (level >= 40)
				return 50;
		}
		else if (agility >= 136){
			if (level >= 20 && level <= 39)
				return 42;
			else if (level >= 40)
				return 51;
		}
	}
	LogFile->write(EQEMuLog::Error, "Error in Client::acmod()");
	return 0;
};

// This is a testing formula for AC, the value this returns should be the same value as the one the client shows...
// ac1 and ac2 are probably the damage migitation and damage avoidance numbers, not sure which is which.
// I forgot to include the iksar defense bonus and i cant find my notes now...
// AC from spells are not included (cant even cast spells yet..)
int16 Client::GetCombinedAC_TEST() {
#if 0
	int ac1;

	ac1 = GetRawItemAC();
	if (m_pp.class_ != WIZARD && m_pp.class_ != MAGICIAN && m_pp.class_ != NECROMANCER && m_pp.class_ != ENCHANTER) {
		ac1 = ac1*4/3;
	}
	ac1 += GetSkill(DEFENSE)/3;
	if (GetAGI() > 70) {
		ac1 += GetAGI()/20;
	}

	int ac2;

	ac2 = GetRawItemAC();
	if (m_pp.class_ != WIZARD && m_pp.class_ != MAGICIAN && m_pp.class_ != NECROMANCER && m_pp.class_ != ENCHANTER) {
		ac2 = ac2*4/3;
	}
	ac2 += GetSkill(DEFENSE)*400/255;

	int combined_ac = (ac1+ac2)*1000/847;
	return combined_ac;
	float combined_ac = ((float)ac1+(float)ac2)*1000.0f/847.0f;
	return (int16) combined_ac;//*10.0f)-10;
#else
	// new formula
	int avoidance = 0;
	avoidance = (acmod() + ((GetSkill(DEFENSE)*16)/9));
	if (avoidance < 0)
		avoidance = 0;
	int mitigation = 0;
	if (m_pp.class_ != WIZARD && m_pp.class_ != MAGICIAN && m_pp.class_ != NECROMANCER && m_pp.class_ != ENCHANTER) {
		mitigation = (spellbonuses->AC/4) + (GetSkill(DEFENSE)/3) + ((itembonuses->AC*4)/3);
	}
	else {
		mitigation = (spellbonuses->AC/3) + (GetSkill(DEFENSE)/2) + (itembonuses->AC+1);
	}
	int displayed = 0;
	displayed = ((avoidance+mitigation)*1000)/847;
	
	return displayed;
#endif
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
void Client::SendGuildMembers(int32 guildid){
	if(guildid==0)
		return;
	uchar* blah=new uchar[(sizeof(GuildMember)*database.NumberInGuild(guildid)+sizeof(GuildMember_Struct))];
	GuildMember_Struct* gms=(GuildMember_Struct*)blah;
	database.GetGuildMembers(guildid,gms);
	if(!gms || gms->count==0){
		printf("Error in SendGuildMembers, no members!\n");
		if(gms)
			safe_delete(gms);
		return;
	}
	int16 namelen=strlen(GetName());
	APPLAYER* outapp = new APPLAYER(OP_GuildMemberList,gms->length+(34*gms->count)+namelen+5);
	memset(outapp->pBuffer,0,outapp->size);
	uchar* buffer=(uchar*)outapp->pBuffer;
	memcpy(buffer,GetName(), namelen);
	buffer+=namelen+1;
	int32 count=htonl(gms->count);
	memcpy(buffer,&count, sizeof(int32));
	buffer+=sizeof(int32);
	for(int32 i=0;i<gms->count;i++){	
		memcpy(buffer,&gms->member[i].name, strlen(gms->member[i].name));
		buffer+=(strlen(gms->member[i].name)+1);
		memcpy(buffer,&gms->member[i].level, sizeof(int32));
		buffer+=sizeof(int32);
		memcpy(buffer,&gms->member[i].class_, sizeof(int32));
		buffer+=sizeof(int32);
		gms->member[i].rank=htonl(gms->member[i].rank);
		memcpy(buffer,&gms->member[i].rank, sizeof(int32));
		buffer+=sizeof(int32);
		memcpy(buffer,&gms->member[i].timelaston, sizeof(int32));
		buffer+=(sizeof(int32)*4);
		if(strlen(gms->member[i].publicnote)>1){
			memcpy(buffer,&gms->member[i].publicnote, strlen(gms->member[i].publicnote));
			buffer+=strlen(gms->member[i].publicnote);
		}
		buffer+=sizeof(int32);
		memcpy(buffer,&gms->member[i].zoneid, sizeof(int8));	
		buffer+=sizeof(int8);
	}
	QueuePacket(outapp);
	safe_delete(outapp);
	safe_delete_array(blah);
}
bool Client::SetGuild(int32 in_guilddbid, int8 in_rank) {
	if (in_guilddbid == 0) {
		// update DB
		if (!database.SetGuild(character_id, 0, GUILD_MEMBER))
			return false;
		// clear guildtag
		guilddbid = in_guilddbid;
		guildeqid = GUILD_NONE;
		guildrank = GUILD_MEMBER;
		SendAppearancePacket(AT_GuildID, GUILD_NONE);
		SendAppearancePacket(AT_GuildRank, 3);
		UpdateWho();
		return true;
	}
	else {
		int32 tmp = database.GetGuildEQID(in_guilddbid);
		if (tmp != GUILD_NONE) {
			if (!database.SetGuild(character_id, in_guilddbid, in_rank))
				return false;
			guildeqid = tmp;
			guildrank = in_rank;
			if (guilddbid != in_guilddbid) {
				guilddbid = in_guilddbid;
				SendAppearancePacket(AT_GuildID, guildeqid);
			}
			SendAppearancePacket(AT_GuildRank, in_rank);
			UpdateWho();
			return true;
		}
	}
	UpdateWho();
	return false;
}

sint32 Client::CalcMaxMana()
{
	switch(GetCasterClass())
	{
		case 'I': {
			max_mana = (int32)(((GetINT()/5)+2) * GetLevel()) +spellbonuses->Mana + itembonuses->Mana;
			break;
				  }
		case 'W': {
			max_mana = (((GetWIS()/5)+2) * GetLevel()) + spellbonuses->Mana + itembonuses->Mana;
			break;
				  }
		case 'N': {
			max_mana = 0;
			break;
		}
		default: {
			cerr << "Invalid Class in CalcMaxMana" << endl;
			max_mana = 0;
			break;
		}
	}
	if (cur_mana > max_mana) {
		cur_mana = max_mana;
		//SendManaUpdatePacket();
	}
#if EQDEBUG >= 11
	LogFile->write(EQEMuLog::Debug, "Client::CalcMaxMana() called for %s - returning %d", GetName(), max_mana);
#endif
	return max_mana;
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

// @merth: this needs to be touched up
uint32 Client::NukeItem(uint32 itemnum) {
	if (itemnum == 0)
		return 0;
	uint32 x = 0;
	/*
	for (i=0; i<=29; i++) { // Equipped and personal inventory
		if (GetItemIDAt(i) == itemnum || (itemnum == 0xFFFE && GetItemIDAt(i) != INVALID_ID)) {
			DeleteItemInInventory(i, 0, true);
			x++;
		}
	}
	for (i=251; i<=339; i++) { // Main inventory's and cursor's containers
		if (GetItemIDAt(i) == itemnum || (itemnum == 0xFFFE && GetItemIDAt(i) != INVALID_ID)) {
			DeleteItemInInventory(i, 0, true);
			x++;
		}
	}
	for (i=2000; i<=2015; i++) { // Bank slots
		if (GetItemIDAt(i) == itemnum || (itemnum == 0xFFFE && GetItemIDAt(i) != INVALID_ID)) {
			DeleteItemInInventory(i, 0, true);
			x++;
		}
	}
	for (i=2030; i<=2109; i++) { // Bank's containers
		if (GetItemIDAt(i) == itemnum || (itemnum == 0xFFFE && GetItemIDAt(i) != INVALID_ID)) {
			DeleteItemInInventory(i, 0, true);
			x++;
		}
	}
	for (i=2500; i<=2501; i++) { // Shared bank
		if (GetItemIDAt(i) == itemnum || (itemnum == 0xFFFE && GetItemIDAt(i) != INVALID_ID)) {
			DeleteItemInInventory(i, 0, true);
			x++;
		}
	}
	for (i=2531; i<=2550; i++) { // Shared bank's containers
		if (GetItemIDAt(i) == itemnum || (itemnum == 0xFFFE && GetItemIDAt(i) != INVALID_ID)) {
			DeleteItemInInventory(i, 0, true);
			x++;
		}
	}
	*/
	return x;
}


bool Client::CheckLoreConflict(const Item_Struct* item) {
	if (!item)
		return false;
	if (!(item->attribs & ItemAttribLore))
		return false;
	
	return (m_inv.HasItem(item->ItemNumber) != SLOT_INVALID);
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
void Client::SummonItem(uint32 item_id, sint8 charges) {
	// For now, we're not allowing summon when an item is already on cursor
	int16 slot=SLOT_CURSOR;
 	if (m_inv[SLOT_CURSOR]) {
		for(int i=0;i<10;i++){
			if(!m_inv[8000+i]){
				slot=(8000+i);
				break;
			}
		}
 		//Message(13, "Error: Item already on cursor! (%s)", m_inv[SLOT_CURSOR]->GetItem()->Name);
 		//return;
 	}
	const Item_Struct* item = database.GetItem(item_id);
	
	if (item == NULL) {
		Message(0, "No such item: %i", item_id);
		return;
	}
	
	// Checking to see if the Item is lore or not.
	bool foundlore = CheckLoreConflict(item);
	
	// Checking to see if it is a GM only Item or not.
	bool foundgm = (item->gm && (this->Admin() < 100));
	
	if (!foundlore && !foundgm) { // Okay, It isn't LORE, or if it is, it is not in player's inventory.
		ItemInst* inst = ItemInst::Create(item, charges);
		if (inst) {
			// Custom logic for SummonItem
			if ((inst->GetCharges()==0))// && inst->IsStackable())
				inst->SetCharges(1);
			//inst->SetCharges(
			PutItemInInventory(slot, *inst);
			// Send item packet to user
			SendItemPacket(SLOT_CURSOR, inst, ItemPacketSummonItem);
			safe_delete(inst);
		}
	}
	else { // Item was already in inventory & is a LORE item or was a GM only item.  Give them a message about it.
		if (foundlore){
			Message_StringID(0,PICK_LORE);
			//Message(0, "You already have a %s (%i) in your inventory!", item->Name, item_id);
		}
		else if (foundgm)
			Message(0, "You are not a GM to summon this item");
	}
}

// Drop item from inventory to ground (generally only dropped from SLOT_CURSOR)
void Client::DropItem(sint16 slot_id)
{
	// Take control of item in client inventory
	ItemInst* inst = m_inv.PopItem(slot_id);
	
	if (!inst) {
		// Item doesn't exist in inventory!
		Message(13, "Error: Item not found in slot %i", slot_id);
		return;
	}
	
	// Save client inventory change to database
	database.SaveInventory(CharacterID(), NULL, slot_id);
	if (inst->GetItem()->NoDrop == 0)
	{
		Message(0, "You can't drop a no drop item.");
		return;
	}
	// Package as zone object
	Object* object = new Object(this, inst);
	entity_list.AddObject(object, true);
	object->Save();
	
	safe_delete(inst);
}

const sint32& Client::SetMana(sint32 amount) {
	Mob::SetMana(amount);
	SendManaUpdatePacket();
	return cur_mana;
}

void Client::SendManaUpdatePacket() {
	if (!Connected() || IsCasting())
		return;
	APPLAYER* outapp = new APPLAYER(OP_ManaChange, sizeof(ManaChange_Struct));
	ManaChange_Struct* manachange = (ManaChange_Struct*)outapp->pBuffer;
//	manachange->new_mana = cur_mana*1.1+2;
	manachange->new_mana = cur_mana;
	manachange->stamina = 6000;
	manachange->spell_id = casting_spell_id;
	outapp->priority = 6;
//	Message(0, "Queueing a manachange with %d new mana (%d max)", manachange->new_mana, GetMaxMana());
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

	// @merth: these two may be related to ns->spawn.equip_chest2
	/*
	ns->spawn.npc_armor_graphic = texture;
	ns->spawn.npc_helm_graphic = helmtexture;
	*/
}

// Returns a slot's item ID (returns INVALID_ID if not found)
uint32 Client::GetItemIDAt(sint16 slot_id) {
	const ItemInst* inst = m_inv[slot_id];
	if (inst)
		return inst->GetItem()->ItemNumber;
	
	// None found
	return INVALID_ID;
}

// Remove item from inventory
void Client::DeleteItemInInventory(sint16 slot_id, sint8 quantity, bool client_update) {
	#if (EQDEBUG >= 5)
		LogFile->write(EQEMuLog::Debug, "DeleteItemInInventory(%i, %i, %s)", slot_id, quantity, (client_update) ? "true":"false");
	#endif
	
	// Nuke from inventory
	m_inv.DeleteItem(slot_id, quantity);
	
	// Save change to database
	const ItemInst* inst = m_inv[slot_id];
	if(inst)//quantity 0 is delete
		database.SaveInventory(character_id, inst, slot_id);
	else
		database.SaveInventory(character_id, 0, slot_id);
	if(client_update)
	{
/*
		APPLAYER *outapp = new APPLAYER(OP_MoveItem, sizeof(MoveItem_Struct));
		MoveItem_Struct *mi = (MoveItem_Struct *)outapp->pBuffer;
		mi->from_slot = slot_id;
		mi->to_slot = 256;
		mi->number_in_stack = quantity;
		QueuePacket(outapp);
		safe_delete(outapp);
*/
		if (inst && inst->GetCharges()) {
			APPLAYER* outapp = new APPLAYER(OP_TraderDelItem, sizeof(TraderDelItem_Struct));
			TraderDelItem_Struct* delitem	= (TraderDelItem_Struct*)outapp->pBuffer;
			delitem->slotid			= slot_id;
			delitem->unknown			= 0xFFFFFFFF;
			//if(inst->GetCharges()<=quantity && (inst->GetItem()->Common.SpellId>=0xFFFF ||inst->GetItem()->Common.SpellId<=0))
			//	delitem->quantity=0xFFFFFFFF; //fully delete item
			//else
//			delitem->quantity	= quantity; // @merth: check that this packet is constructed correctly..
			delitem->quantity = 0xffffffff;
			QueuePacket(outapp);
			safe_delete(outapp);
		}
		else {
			APPLAYER* outapp = new APPLAYER(OP_TraderDelItem, sizeof(TraderDelItem_Struct));
			TraderDelItem_Struct* delitem	= (TraderDelItem_Struct*)outapp->pBuffer;
			delitem->slotid			= slot_id;
			delitem->unknown			= 0xFFFFFFFF;
			delitem->quantity	= 0xFFFFFFFF;
			QueuePacket(outapp);
			safe_delete(outapp);
		}
	}
}

// Puts an item into the person's inventory
// Any items already there will be removed from user's inventory
// (Also saves changes back to the database: this may be optimized in the future)
// client_update: Sends packet to client
bool Client::PutItemInInventory(sint16 slot_id, const ItemInst& inst, bool client_update)
{
	m_inv.PutItem(slot_id, inst);
	
	if (client_update && inst) {
		SendItemPacket(slot_id, &inst, ItemPacketSummonItem);
	}
	
	return database.SaveInventory(this->CharacterID(), &inst, slot_id);
}

void Client::PutLootInInventory(sint16 slot_id, const ItemInst &inst, ServerLootItem_Struct** bag_item_data)
{
	m_inv.PutItem(slot_id, inst);
	SendLootItemInPacket(&inst, slot_id);
	database.SaveInventory(this->CharacterID(), &inst, slot_id);

	if(bag_item_data)	// bag contents
	{
		sint16 interior_slot;
		// solar: our bag went into slot_id, now let's pack the contents in
		for(int i = 0; i < 10; i++)
		{
			if(bag_item_data[i])
			{
				const ItemInst *bagitem = ItemInst::Create(bag_item_data[i]->item_id, bag_item_data[i]->charges);
				interior_slot = Inventory::CalcSlotId(slot_id, i);
				PutLootInInventory(interior_slot, *bagitem);
			}
		}
	}
	CalcBonuses();
}

// Locate an available space in inventory to place an item
// and then put the item there
// The change will be saved to the database
bool Client::AutoPutLootInInventory(ItemInst& inst, bool try_worn, bool try_cursor, ServerLootItem_Struct** bag_item_data)
{
	// #1: Try to auto equip
	if (try_worn && inst.IsEquipable(GetRace(), GetClass()) && inst.GetItem()->Common.RequiredLevel<=level)

	{
		for (sint16 i = 0; i < 22; i++)
		{
			if (!m_inv[i])
			{
				if( i == SLOT_PRIMARY && inst.IsWeapon() ) // If item is primary slot weapon
				{
					if( (inst.GetItem()->Common.Skill == 1) || (inst.GetItem()->Common.Skill == 4) || (inst.GetItem()->Common.Skill == 35) ) // and uses 2hs \ 2hb \ 2hp
					{
						if( m_inv[SLOT_SECONDARY] ) // and if secondary slot is not empty
						{
							continue; // Can't auto-equip
						}
					}
				}

				if
				(
					i == SLOT_SECONDARY &&
					inst.IsWeapon() &&
					!CanThisClassDualWield()
				)
				{
					continue;
				}

				if (inst.IsEquipable(i))	// Equippable at this slot?
				{
					PutLootInInventory(i, inst);
					return true;
				}
			}
		}
	}
		
	// #2: Stackable item?
	if (inst.IsStackable())
	{
		// Pass 1: (Inventory) Attempt to fill stacks that aren't yet full
		sint16 i;
		for (i = 22; i <= 29; i++)
		{
			ItemInst* tmp_inst = m_inv.GetItem(i);
				
			if
			(
				tmp_inst &&
				tmp_inst->GetItem() == inst.GetItem() &&
				tmp_inst->GetCharges() < ITEM_MAX_STACK
			)
			{
				MoveLootCharges(inst, i);
				if(inst.GetCharges())	// we didn't get them all
				{
					return AutoPutLootInInventory(inst, try_worn, try_cursor, 0);
				}
				return true;
			}
		}
			
		// Pass 2: (Inventory Bags) Attempt to fill stacks that aren't yet full
		for (i = 22; i <= 29; i++)
		{
			for (uint8 j = 0; j < 10; j++)
			{
				int16 slotid = Inventory::CalcSlotId(i, j);
				ItemInst* tmp_inst = m_inv.GetItem(slotid);

				if
				(
					tmp_inst &&
					tmp_inst->GetItem() == inst.GetItem() &&
					tmp_inst->GetCharges() < ITEM_MAX_STACK
				)
				{
					MoveLootCharges(inst, slotid);
					CalcBonuses();
					if(inst.GetCharges())	// we didn't get them all
						return AutoPutLootInInventory(inst, try_worn, try_cursor, 0);
					return true;
				}
			}
		}
	}

	// #3: put it in inventory
	sint16 slot_id = m_inv.FindFreeSlot(inst.IsType(ItemTypeContainer), try_cursor);
	if (slot_id != SLOT_INVALID)
	{
		PutLootInInventory(slot_id, inst, bag_item_data);
		return true;
	}
	
	return false;
}

// solar: helper function for AutoPutLootInInventory
void Client::MoveLootCharges(ItemInst &from, sint16 to_slot)
{
	ItemInst *tmp_inst = m_inv.GetItem(to_slot);

	if(tmp_inst && tmp_inst->GetCharges() < ITEM_MAX_STACK)
	{
		// this is how much room is left on the item we're stacking onto
		int charge_slots_left = ITEM_MAX_STACK - tmp_inst->GetCharges();
		// this is how many charges we can move from the looted item to
		// the item in the inventory
		int charges_to_move =
			from.GetCharges() < charge_slots_left ?
				from.GetCharges() :
				charge_slots_left;

		tmp_inst->SetCharges(tmp_inst->GetCharges() + charges_to_move);
		from.SetCharges(from.GetCharges() - charges_to_move);
		SendLootItemInPacket(tmp_inst, to_slot);
		database.SaveInventory(this->CharacterID(), tmp_inst, to_slot);
	}
}

void Client::SendItemLink(const ItemInst* inst, bool send_to_all)
{
	if (!inst)
		return;
	
	const Item_Struct* item = inst->GetItem();
	const char* name2 = &item->Name[0];
	APPLAYER* outapp = new APPLAYER(OP_ItemLinkText,strlen(name2)+68);
	char buffer2[135] = {0};
	char itemlink[135] = {0};
	sprintf(itemlink,"%c%07u%s%s%c",0x12,item->ItemNumber,"-00001-00001-00001-00001-0000000000000",name2,0x12);
	sprintf(buffer2,"%c%c%c%c%c%c%c%c%c%c%c%c%s",0x00,0x00,0x00,0x00,0xD3,0x01,0x00,0x00,0x1E,0x01,0x00,0x00,itemlink);
	memcpy(outapp->pBuffer,buffer2,outapp->size);
	QueuePacket(outapp);
	safe_delete(outapp);
	if (send_to_all==false)
		return;
	const char* charname = this->GetName();
	outapp = new APPLAYER(OP_ItemLinkText,strlen(itemlink)+14+strlen(charname));
	char buffer3[150] = {0};
	sprintf(buffer3,"%c%c%c%c%c%c%c%c%c%c%c%c%6s%c%s",0x00,0x00,0x00,0x00,0xD2,0x01,0x00,0x00,0x00,0x00,0x00,0x00,charname,0x00,itemlink);
	memcpy(outapp->pBuffer,buffer3,outapp->size);
	outapp->Deflate();
	entity_list.QueueCloseClients(this->CastToMob(),outapp,true,200,0,false);
	safe_delete(outapp);
}

void Client::SendLootItemInPacket(const ItemInst* inst, sint16 slot_id)
{
	SendItemPacket(slot_id,inst, ItemPacketTrade);
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
	string booktxt2=database.GetBook(txtfile);
	int length=strlen(booktxt2.c_str())+3;
	char booktxt[5000]={0};//booktxt2.c_str();
	strcpy(booktxt,booktxt2.c_str());
	if (booktxt != 0) {
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
	if (GetSkill(skillid) < MaxSkill(skillid))
	{
		// the higher your current skill level, the harder it is
		sint16 Chance = 10 + chancemodi + ((252 - GetSkill(skillid)) / 20);
		if (Chance < 1)
			Chance = 1; // Make it always possible
		if(((float)rand()/RAND_MAX)*100 < Chance)
		{
			SetSkill(skillid, GetSkill(skillid) + 1);
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
			SetSkill(skillid,GetSkill(skillid)+1);
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

// Place items into inventory of client specified
void Client::FinishTrade(NPC* with){
	int32 items[4]={0};
	int8 charges[4]={0};
	for (sint16 i=3000; i<=3003; i++){
		const ItemInst* inst = m_inv[i];
		if (inst) {
			items[i-3000]=inst->GetItem()->ItemNumber;
			charges[i-3000]=inst->GetCharges();
			DeleteItemInInventory(i);
		}
	}
	char temp1[100];
	memset(temp1,0x0,100);
	char temp2[100];
	memset(temp2,0x0,100);
	for ( int z=0; z < 4; z++ ) {
		sprintf(temp1,"item%d.%d", z+1,with->GetNPCTypeID());
		sprintf(temp2,"%d",items[z]);
		parse->AddVar(temp1,temp2);
		memset(temp1,0x0,100);
		memset(temp2,0x0,100);
	}
	sprintf(temp1,"copper.%d",with->GetNPCTypeID());
	sprintf(temp2,"%i",trade->cp);
	parse->AddVar(temp1,temp2);
	memset(temp1,0x0,100);
	memset(temp2,0x0,100);
	sprintf(temp1,"silver.%d",with->GetNPCTypeID());
	sprintf(temp2,"%i",trade->sp);
	parse->AddVar(temp1,temp2);
	memset(temp1,0x0,100);
	memset(temp2,0x0,100);
	sprintf(temp1,"gold.%d",with->GetNPCTypeID());
	sprintf(temp2,"%i",trade->gp);
	parse->AddVar(temp1,temp2);
	memset(temp1,0x0,100);
	memset(temp2,0x0,100);
	sprintf(temp1,"platinum.%d",with->GetNPCTypeID());
	sprintf(temp2,"%i",trade->pp);
	parse->AddVar(temp1,temp2);
	memset(temp1,0x0,100);
	memset(temp2,0x0,100);
	parse->Event(EVENT_ITEM, with->GetNPCTypeID(), 0, with, this->CastToMob());
	LinkedListIterator<ServerLootItem_Struct*> iterator(*with->CastToNPC()->itemlist);
	iterator.Reset();
	int xy = 0;
	while(iterator.MoreElements()) {
		xy++;
		iterator.Advance();
	}
	
	for(int y=0;y<4;y++){
		if (xy <20){
			xy++;
			NPC* npc=with->CastToNPC();
			const Item_Struct* item2 = database.GetItem(items[y]);
			if (item2) { //no "no drop" items for j00!
				ServerLootItem_Struct* item = new ServerLootItem_Struct;
				item->item_id = item2->ItemNumber;
				item->charges = charges[y];
				char newid[20];
				memset(newid, 0, sizeof(newid));
				for(int i=0;i<7;i++){
					if (!isalpha(item2->IDFile[i])){
						strncpy(newid, &item2->IDFile[i],5);
						i=8;
					}
				}
				APPLAYER* outapp = new APPLAYER(OP_WearChange, sizeof(WearChange_Struct));
	 			WearChange_Struct* wc = (WearChange_Struct*)outapp->pBuffer;
	 			wc->spawn_id = npc->GetID();
				wc->material=0;
				if (((item2->EquipSlots==24576) || (item2->EquipSlots==8192)) && (npc->d_meele_texture1==0)) {
					wc->wear_slot_id=7;
					if (item2->Common.SpellId!=0)
						npc->CastToMob()->AddProcToWeapon(item2->Common.SpellId,true);
					npc->equipment[7]=item2->ItemNumber;
					npc->d_meele_texture1=atoi(newid);
					if (item2->Common.Material >0)
						wc->material=item2->Common.Material;
					else
						wc->material=atoi(newid);
					npc->AC+=item2->Common.AC;
					npc->STR+=item2->Common.STR;
					npc->INT+=item2->Common.INT;
				}
				else if (((item2->EquipSlots==24576) || (item2->EquipSlots==16384)) && (npc->d_meele_texture2 ==0) && ((npc->GetLevel()>=13) || (item2->Common.Damage==0))) 
				{
					if (item2->Common.SpellId!=0)
						npc->CastToMob()->AddProcToWeapon(item2->Common.SpellId,true);
					npc->d_meele_texture2=atoi(newid);
					npc->equipment[8]=item2->ItemNumber;
					wc->wear_slot_id=8;
					if (item2->Common.Material >0)
						wc->material=item2->Common.Material;
					else
						wc->material=atoi(newid);
					npc->AC+=item2->Common.AC;
					npc->STR+=item2->Common.STR;
					npc->INT+=item2->Common.INT;
				}
				else if ((item2->EquipSlots==4) && (npc->equipment[0]==0)){

					npc->equipment[0]=atoi(newid);
					if (item2->Common.Material >0)
						wc->material=item2->Common.Material;
					else
						wc->material=atoi(newid);
					wc->wear_slot_id=0;
					npc->AC+=item2->Common.AC;
					npc->STR+=item2->Common.STR;
					npc->INT+=item2->Common.INT;
				}
				else if ((item2->EquipSlots==131072) && (npc->equipment[1]==0)){
					npc->equipment[1]=atoi(newid);
					if (item2->Common.Material >0)
						wc->material=item2->Common.Material;
					else
						wc->material=atoi(newid);
					wc->wear_slot_id=1;
					npc->AC+=item2->Common.AC;
					npc->STR+=item2->Common.STR;
					npc->INT+=item2->Common.INT;
				}
				else if ((item2->EquipSlots==128) && (npc->equipment[2]==0)){
					npc->equipment[2]=atoi(newid);
					if (item2->Common.Material >0)
						wc->material=item2->Common.Material;
					else
						wc->material=atoi(newid);
					wc->wear_slot_id=2;
					npc->AC+=item2->Common.AC;
					npc->STR+=item2->Common.STR;

					npc->INT+=item2->Common.INT;
				}
				else if ((item2->EquipSlots==1536) && (npc->equipment[3]==0)){
					npc->equipment[3]=atoi(newid);
					if (item2->Common.Material >0)
						wc->material=item2->Common.Material;
					else
						wc->material=atoi(newid);
					wc->wear_slot_id=3;
					npc->AC+=item2->Common.AC;
					npc->STR+=item2->Common.STR;
					npc->INT+=item2->Common.INT;
				}

				else if ((item2->EquipSlots==4096) && (npc->equipment[4]==0)){
					npc->equipment[4]=atoi(newid);
					if (item2->Common.Material >0)
						wc->material=item2->Common.Material;
					else
						wc->material=atoi(newid);
					wc->wear_slot_id=4;
					npc->AC+=item2->Common.AC;
					npc->STR+=item2->Common.STR;
					npc->INT+=item2->Common.INT;
				}
				else if ((item2->EquipSlots==262144) && (npc->equipment[5]==0)){
					npc->equipment[5]=atoi(newid);
					if (item2->Common.Material >0)

						wc->material=item2->Common.Material;
					else
						wc->material=atoi(newid);
					wc->wear_slot_id=5;
					npc->AC+=item2->Common.AC;
					npc->STR+=item2->Common.STR;
					npc->INT+=item2->Common.INT;
				}
				else if ((item2->EquipSlots==524288) && (npc->equipment[6]==0)){
					npc->equipment[6]=atoi(newid);
					if (item2->Common.Material >0)
						wc->material=item2->Common.Material;
					else
						wc->material=atoi(newid);
					wc->wear_slot_id=6;
					npc->AC+=item2->Common.AC;
					npc->STR+=item2->Common.STR;


					npc->INT+=item2->Common.INT;
				}
				if (((npc->GetRace()==127) && (npc->CastToMob()->GetOwnerID()!=0)) && (item2->EquipSlots==24576) || (item2->EquipSlots==8192) || (item2->EquipSlots==16384)){
					npc->d_meele_texture2=atoi(newid);
					wc->wear_slot_id=8;
					if (item2->Common.Material >0)
						wc->material=item2->Common.Material;
					else
						wc->material=atoi(newid);
					npc->AC+=item2->Common.AC;
					npc->STR+=item2->Common.STR;
					npc->INT+=item2->Common.INT;
				}
				item->equipSlot = item2->EquipSlots;
				if((item2->NoDrop != 0 && !parse->HasQuestFile(with->GetNPCTypeID())) || this->GetGM())
					(*npc->itemlist).Append(item);
	 			entity_list.QueueClients(this, outapp);
	 			safe_delete(outapp);
			}
		}
	}

}
void Client::FinishTrade(Client* other)
{
	sint16 slot_id;
	if (!other)
		return;
	// Move each trade slot into free inventory slot
	for (sint16 i=3000; i<=3007; i++){
		const ItemInst* inst = m_inv[i];
		
		if (inst && inst->GetItem()->NoDrop) {
			slot_id = other->GetInv().FindFreeSlot(inst->IsType(ItemTypeContainer), true);
			
			if (other->PutItemInInventory(slot_id, *inst, true))
				this->DeleteItemInInventory(i);
		}
	}
	
	// Money @merth: look into how NPC's receive cash
	this->AddMoneyToPP(other->trade->cp, other->trade->sp, other->trade->gp, other->trade->pp, true);
	
	// Clear trade inventory
	trade->Reset();
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

bool Database::CheckGuildDoor(int8 doorid,int16 guildid,const char* zone) {
	MYSQL_ROW row;
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
    MYSQL_RES *result;
	if (!RunQuery(query, MakeAnyLenString(&query, "SELECT guild FROM doors where doorid=%i AND zone='%s'",doorid-128, zone), errbuf, &result)) {
		cerr << "Error in CheckUsedName query '" << query << "' " << errbuf << endl;
		if (query != 0)
			safe_delete_array(query);
		return false;
	}
	else { 
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if (atoi(row[0]) == guildid)
			{
				mysql_free_result(result);
				return true;
			}
			else
			{
				mysql_free_result(result);
				return false;
			}
			
			// code below will never be reached
			mysql_free_result(result);
			return false;
		}
	}
	return false;
}

bool Database::SetGuildDoor(int8 doorid,int16 guildid, const char* zone) {
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	int32	affected_rows = 0;
	if (doorid > 127)
		doorid = doorid - 128;
	if (!RunQuery(query, MakeAnyLenString(&query, "UPDATE doors SET guild = %i WHERE (doorid=%i) AND (zone='%s')",guildid,doorid, zone), errbuf, 0,&affected_rows)) {
		cerr << "Error in SetGuildDoor query '" << query << "' " << errbuf << endl;
		return false;
	}
	
	safe_delete_array(query);
	
	if (affected_rows == 0)
	{
		return false;
	}
	
	return true;
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
	QueuePacket(outapp);
	safe_delete(outapp);
}
bool Client::LootToStack(int32 itemid) {  //Loots stackable items to existing stacks - Wiz
	// @merth: Need to do loot code with new inventory struct
	/*
	const Item_Struct* item;
	int i;
	for (i=22; i<=29; i++) {
		item = GetItemAt(i);
		if (item) {
			if (m_pp.invitemproperties[i].charges < 20 && item->ItemNumber == itemid)
			{
				m_pp.invitemproperties[i].charges += 1;
				APPLAYER* outapp = new APPLAYER(OP_PlaceItem, sizeof(Item_Struct));
				memcpy(outapp->pBuffer, item, outapp->size);
				Item_Struct* outitem = (Item_Struct*) outapp->pBuffer;
				outitem->equipSlot = i;
				outitem->common.charges = m_pp.invitemproperties[i].charges;
				QueuePacket(outapp);
				safe_delete(outapp);
				return true;
			}
		}
	}
	for (i=0; i<=pp_containerinv_size; i++) {
		if (m_pp.containerinv[i] != 0xFFFF) {
			item = database.GetItem(m_pp.containerinv[i]);
			if (m_pp.bagitemproperties[i].charges < 20 && item->ItemNumber == itemid)
			{
				m_pp.bagitemproperties[i].charges += 1;

				APPLAYER* outapp = new APPLAYER(OP_PlaceItem, sizeof(Item_Struct));
				memcpy(outapp->pBuffer, item, outapp->size);
				Item_Struct* outitem = (Item_Struct*) outapp->pBuffer;
				outitem->equipSlot = 250+i;
				outitem->common.charges = m_pp.bagitemproperties[i].charges;
				QueuePacket(outapp);
				safe_delete(outapp);
				return true;
			}
		}
	}
	*/
	return false;
}

void Client::SetFeigned(bool in_feigned) {
	if (in_feigned)
	{
		SetPet(0);
		entity_list.ClearFeignAggro(this);
	}
	feigned=in_feigned;
 }

sint16	Client::GetMR()
{
    return 20 + itembonuses->MR + spellbonuses->MR + GetAA(Innate_Magic_Protection) * 5;
}

sint16	Client::GetFR()
{
    return 20 + itembonuses->FR + spellbonuses->FR + GetAA(Innate_Fire_Protection) * 5;
}

sint16	Client::GetDR()
{
    return 20 + itembonuses->DR + spellbonuses->DR + GetAA(Innate_Disease_Protection) * 5;
}

sint16	Client::GetPR()
{
    return 20 + itembonuses->PR + spellbonuses->PR + GetAA(Innate_Poison_Protection) * 5;
}

sint16	Client::GetCR()
{
    return 20 + itembonuses->CR + spellbonuses->CR + GetAA(Innate_Cold_Protection) * 5;
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
		if(!bindwound_timer->Enabled()) {
			// start complete timer
			bindwound_timer->Start(10000);
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
				bindwound_timer->Disable();
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
		}
		else if (bindwound_timer->Enabled()){
		// finish bind
			// disable complete timer
			bindwound_timer->Disable();
			bindwound_target = 0;
			if(!bindmob){
					// send "bindmob gone" to client
					bind_out->type = 5; // not in zone
					QueuePacket(outapp);
					bind_out->type = 0;
			}

			else {
				if (bindmob->Dist(*this) <= 20) {
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
					DeleteItemInInventory(m_inv.HasItem(13009, 1), 1, true);
					bind_out->type = 1; // Done
					QueuePacket(outapp);
					bind_out->type = 0;
					CheckIncreaseSkill(BIND_WOUND);
					
					float max_percent = 0.5f;
					uint8 *aa_item = &(((uint8 *)&aa)[14]);
					if (*aa_item){
						max_percent += 0.1f * (float) *aa_item;
					}
					
					// send bindmob new hp's
					if (bindmob->GetHP() < bindmob->GetMaxHP() && bindmob->GetHP() <= (bindmob->GetMaxHP()*max_percent)-1){
						// 0.120 per skill point, 0.60 per skill level, minimum 3 max 30
						int bindhps = 3;

						if (GetSkill(BIND_WOUND) >= 10) {
							bindhps += (int)(GetSkill(BIND_WOUND)*0.120);
						}
						if (bindhps > 30){
							bindhps = 30;
						}
						bindmob->SetHP( bindmob->GetHP() + bindhps);
						bindmob->SendHPUpdate();
					}
					else {
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
	else if (fail && bindwound_timer->Enabled()) {
		// You moved
		outapp = new APPLAYER(OP_Bind_Wound, sizeof(BindWound_Struct));
		BindWound_Struct* bind_out = (BindWound_Struct*) outapp->pBuffer;
		bindwound_timer->Disable();
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
void Client::Message_StringID(int32 type, int32 string_id)
{
	APPLAYER* outapp = new APPLAYER(OP_SimpleMessage,12);
	SimpleMessage_Struct* sms = (SimpleMessage_Struct*)outapp->pBuffer;
	sms->color=type;
	sms->string_id=string_id;

	sms->unknown8=0;

	QueuePacket(outapp);
	safe_delete(outapp);
}

//
// solar: this list of 9 args isn't how I want to do it, but to use va_arg
// you have to know how many args you're expecting, and to do that we have
// to load the eqstr file and count them in the string.
// This hack sucks but it's gonna work for now.
//
void Client::Message_StringID(int32 type, int32 string_id,  const char* message1,const char* message2,const char* message3,const char* message4,const char* message5,const char* message6,const char* message7,const char* message8,const char* message9)
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

	QueuePacket(outapp);
	safe_delete(outapp);
}

// Moves items around both internally and in the database
// In the future, this can be optimized by pushing all changes through one database REPLACE call
bool Client::SwapItem(MoveItem_Struct* move_in) {
	if (move_in->from_slot == move_in->to_slot)
		return true; // Item summon, no further proccessing needed
	
	if (move_in->to_slot == (uint32)SLOT_INVALID) {
		DeleteItemInInventory(move_in->from_slot);
		if(move_in->from_slot==SLOT_CURSOR){
			for(int ndx=0;ndx<10;ndx++){
				if(m_inv[8000+ndx]){
					if(!m_inv[SLOT_CURSOR]){//no item has been put on the cursor from the que
						m_inv.SwapItem(8000+ndx,SLOT_CURSOR);//put the next item in line onto the cursor
						DeleteItemInInventory(8000+ndx);//delete the source item
					}
					else{//item is on cursor now
						DeleteItemInInventory(8000+ndx-1);//delete from destination
						m_inv.SwapItem(8000+ndx,8000+ndx-1);//move items ahead in the que
					}
				}
			}
		}
		database.SaveInventory(character_id, m_inv[move_in->to_slot], move_in->to_slot);
		database.SaveInventory(character_id, m_inv[move_in->from_slot], move_in->from_slot);
		return true; // Item deletetion
	}
	if(auto_attack && (move_in->from_slot == SLOT_PRIMARY || move_in->from_slot == SLOT_SECONDARY))
		SetAttackTimer();
	else if(auto_attack && (move_in->to_slot == SLOT_PRIMARY || move_in->to_slot == SLOT_SECONDARY))
		SetAttackTimer();
	// Step 1: Variables
	sint16 src_slot_id = (sint16)move_in->from_slot;
	sint16 dst_slot_id = (sint16)move_in->to_slot;
	
	//Setup world containers
	uint32 srcitemid = 0;
	uint32 dstitemid = 0;
	ItemInst* src_inst = m_inv.GetItem(src_slot_id);
	ItemInst* dst_inst = m_inv.GetItem(dst_slot_id);
	if (src_inst){
		srcitemid = src_inst->GetItem()->ItemNumber;
	}
	if (dst_inst)
		dstitemid = dst_inst->GetItem()->ItemNumber;
	if (Trader && srcitemid>0){
		ItemInst* srcbag;
		uint32 srcbagid =0;
		if (src_slot_id>=250 && src_slot_id<330){
			srcbag=m_inv.GetItem(((int)(src_slot_id/10))-3);
			if(srcbag)
				srcbagid=srcbag->GetItem()->ItemNumber;
		}
		
		if (srcitemid==17899 || srcbagid==17899){
			this->Trader_EndTrader();
			this->Message(15,"You cannot move items while trading!");
		}
	}
	
	// Step 2: Validate item in from_slot
	// After this, we can assume src_inst is a valid ptr
	if (!src_inst && (src_slot_id<4000 || src_slot_id>4009) ) {
		Message(13, "Error: Server found no item in slot %i, Deleting Item!", src_slot_id);
		this->DeleteItemInInventory(dst_slot_id,0,true);
		return false;
	}
	
	// Step 3: Check for interaction with World Container (tradeskills)
	if (src_slot_id>=4000 && src_slot_id<=4009 && m_tradeskill_object!=NULL) {
		// Picking up item from world container
		ItemInst* inst = m_tradeskill_object->PopItem(Inventory::CalcBagIdx(src_slot_id));
		if (inst) {
			PutItemInInventory(dst_slot_id, *inst, false);
			safe_delete(inst);
		}
		
		return true;
	}
	else if (dst_slot_id>=4000 && dst_slot_id<=4009 && m_tradeskill_object!=NULL) {
		// Putting item into world container, which may swap (or pile onto) with existing item
		uint8 world_idx = Inventory::CalcBagIdx(dst_slot_id);
		ItemInst* world_inst = m_tradeskill_object->PopItem(world_idx);
		
		// Case 1: No item in container, unidirectional "Put"
		if (world_inst == NULL) {
			m_tradeskill_object->PutItem(world_idx, src_inst);
			m_inv.DeleteItem(src_slot_id);
		}
		else {
			const Item_Struct* world_item = world_inst->GetItem();
			const Item_Struct* src_item = src_inst->GetItem();
			if (world_item && (int32)world_item!=0xFEEEFEEE && src_item) {
				// Case 2: Same item on cursor, stacks, transfer of charges needed
				if ((world_item->ItemNumber == src_item->ItemNumber) && src_inst->IsStackable()) {
					sint8 world_charges = world_inst->GetCharges();
					sint8 src_charges = src_inst->GetCharges();
					
					// Fill up destination stack as much as possible
					world_charges += src_charges;
					if (world_charges > ITEM_MAX_STACK) {
						src_charges = world_charges - ITEM_MAX_STACK;
						world_charges = ITEM_MAX_STACK;
					}
					else {
						src_charges = 0;
					}
					
					world_inst->SetCharges(world_charges);
					m_tradeskill_object->Save();
					
					if (src_charges == 0) {
						m_inv.DeleteItem(src_slot_id); // DB remove will occur below
					}
					else {
						src_inst->SetCharges(src_charges);
					}
				}
				else {
					// Case 3: Swap the item on user with item in world container
					// World containers don't follow normal rules for swapping
					ItemInst* inv_inst = m_inv.PopItem(src_slot_id);
					m_tradeskill_object->PutItem(world_idx, inv_inst);
					m_inv.PutItem(src_slot_id, *world_inst);
					safe_delete(inv_inst);
				}
			}
		}
		
		safe_delete(world_inst);
		database.SaveInventory(character_id, m_inv[src_slot_id], src_slot_id);
		return true;
	}
	
	// Step 4: Check for entity trade
	Mob* with = trade->With();
	if (with && dst_slot_id>=3000 && dst_slot_id<=3007) {

#if EQDEBUG>=5
			LogFile->write(EQEMuLog::Debug, "Trade: %s adding item(s) to trade session with %s", GetName(), with->GetName());
#endif		
		// Fill Trade list with items from cursor
		if (!m_inv[SLOT_CURSOR]) {
			Message(13, "Error: Cursor item not located on server!");
			return false;
		}
		
		// Add cursor item to trade bucket
		// Also sends trade information to other client of trade session
		trade->AddEntity(src_slot_id, dst_slot_id);
		return true;
	}
	
	// Step 5: Swap (or stack) items
	if (move_in->number_in_stack > 0) {
		// Determine if charged items can stack
		if ((dst_inst) && (src_inst->GetItem()==dst_inst->GetItem()) && (dst_inst->GetCharges() < 20)) {
			// Charges can be emptied into dst
			uint8 usedcharges = 20 - dst_inst->GetCharges();
			if (usedcharges > move_in->number_in_stack)
				usedcharges = move_in->number_in_stack;
			
			dst_inst->SetCharges(dst_inst->GetCharges() + usedcharges);
			src_inst->SetCharges(src_inst->GetCharges() - usedcharges);
			
			// Depleted all charges?
			if (src_inst->GetCharges() < 1)
				m_inv.DeleteItem(src_slot_id);
		}
		else {
			// Nothing in destination slot: split stack into two
			if ((sint16)move_in->number_in_stack >= src_inst->GetCharges()) {
				// Move entire stack
				m_inv.SwapItem(src_slot_id, dst_slot_id);
			}
			else {
				// Split into two
				src_inst->SetCharges(src_inst->GetCharges() - move_in->number_in_stack);
				ItemInst* inst = ItemInst::Create(src_inst->GetItem(), move_in->number_in_stack);
				m_inv.PutItem(dst_slot_id, *inst);
				safe_delete(inst);
			}
		}
	}
	else {
		// Not dealing with charges - just do direct swap
		if(src_inst && dst_slot_id<22 && dst_slot_id>0)
			SetMaterial(dst_slot_id,src_inst->GetItem()->ItemNumber);
		m_inv.SwapItem(src_slot_id, dst_slot_id);
	}
	if(move_in->from_slot ==SLOT_CURSOR){
		if(!m_inv[SLOT_CURSOR]){//item on cursor is deleted, see if there is something in the cursor que
			for(int ndx=0;ndx<10;ndx++){
				if(m_inv[8000+ndx]){
					if(!m_inv[SLOT_CURSOR])//no item has been put on the cursor from the que
						m_inv.SwapItem(8000+ndx,SLOT_CURSOR);//put the next item in line onto the cursor
					else//item is on cursor now
						m_inv.SwapItem(8000+ndx,8000+ndx-1);//move items ahead in the que
					DeleteItemInInventory(8000+ndx);//delete the source item
				}
			}
		}
	}
	// Step 7: Save change to the database
	database.SaveInventory(character_id, m_inv[src_slot_id], src_slot_id);
	database.SaveInventory(character_id, m_inv[dst_slot_id], dst_slot_id);
	
	// Step 8: Re-calc stats
	CalcBonuses();
	return true;
}
void Client::Discipline(ClientDiscipline_Struct* disc_in, Mob* tar) {
	if (disc_timer->Enabled()) {
		char val1[20]={0};
		char val2[20]={0};
		Message_StringID(0,DISCIPLINE_CANUSEIN,ConvertArray((disc_timer->GetRemainingTime()/1000)/60,val1),ConvertArray(disc_timer->GetRemainingTime()/1000%60,val2));
		//Message(0,"You can use a new discipline in %i minutes %i seconds.", (disc_timer->GetRemainingTime()/1000)/60,	disc_timer->GetRemainingTime()/1000%60);
		return;
	}
    switch(disc_in->disc_id){
	// Shared?
	case 30: { // Resistant
		// 1 minute duration
		// 1 hour reuse
		// +3 to +10 to resists 
		if (GetLevel()<=29)
			return;
		disc_timer->Start(1000*60*60);
		disc_elapse->Start(1000*60);
		entity_list.MessageClose(this, false, 100, 0, "%s has become more resistant!", GetName());
	    break;
	}
	case 31: { // Fearless
		// 11 second duration
		// 1 hour reuse
		// 100% fear immunity
		if (GetLevel()<=39)
			return;
		disc_timer->Start(1000*60*60);
		disc_elapse->Start(1000*11);
		entity_list.MessageClose_StringID(this, false, 100, 0, DISCIPLINE_FEARLESS, GetName());
		//entity_list.MessageClose(this, false, 100, 0, "%s becomes fearless!", GetName());
	    break;
	}
	case 6: { // Counterattack/Whirlwind/Furious
		// warrior level 56
		// rogue/monk level 53
		// 9 second duration
		// 1 hour reuse
		if (      (GetClass() == WARRIOR && GetLevel() <= 56)
			||(GetLevel() <= 53)
			) return;
		disc_timer->Start(1000*60*60);
		disc_elapse->Start(1000*9);
		entity_list.MessageClose(this, false, 100, 0, "%s\'s face becomes twisted with fury!", GetName());
	    break;
	}
	case 14: { // Duelist/Innerflame/Fellstrike
		// monk level 56
		// rogue level 59
		// warrior level 58
		// 12 second duration
		// 30 minute reuse
		// min 4*base hand/weapon damage
		if (      (GetClass() == MONK && GetLevel() <= 55)
			||(GetClass() == WARRIOR && GetLevel() <= 58)
			||(GetClass() == ROGUE && GetLevel() <= 59)
			) return;
		disc_timer->Start(1000*60*30);
		disc_elapse->Start(1000*12);
		entity_list.MessageClose(this, false, 100, 0, "%s\'s muscles bulge with force of will!", GetName());
	    break;
	}
	case 15: { // Blindingspeed/Hundredfist
		// rogue level 58
		// monk level 57
		// 15 second duration
		// 30 minute reuse
		if (      (GetClass() == MONK && GetLevel() <= 58)
			||(GetClass() == ROGUE && GetLevel() <= 57)
			) return;
		//disc_timer->Start(1000*60*30);
		//disc_elapse->Start(1000*15);
		Message(0, "This discipline not implemented..");
	    break;
	}
	case 16: { // Deadeye/Charge
		// warrior level 53
		// rogue level 54
		// 14 second duration
		// 30 minute reuse
		if (      (GetClass() == WARRIOR && GetLevel() <= 53)
			||(GetClass() == ROGUE && GetLevel() <= 54)
			) return;
		disc_timer->Start(1000*60*30);
		disc_elapse->Start(1000*14);
		entity_list.MessageClose(this, false, 100, 0, "%s feels unstopable!", GetName());
	    break;
	}
	// Warrior
	case 4: { // Evasive
		// level 52
		// 3 minute duration
		// 15 minute reuse
		// +35% avoidance
		// -15% out
	    break;
	}
	case 17: { // Mightystrike
		// level 54
		// 10 second duration
		// 1 hour reuse
		// Auto crit
	    break;
	}
	case 3: { // Defensive
		// level 55
		// 3 minute duration
		// 15 minute reuse
		// +35% mitigation
		// -15% out
	    break;
	}
	case 2: { // Precise
		// level 57
		// 3 minute duration
		// 30 minute reuse
		// -15% avoidance
		// +35% out
	    break;
	}
	case 1: { // Aggressive
		// level 60
		// 3 minute duration
		// 27 minute reuse
		// -15% mitigation
		// +35% out
	    break;
	}
	// Monk
	case 11: { // Stonestance
	    break;
	}
	case 12: { // Thunderkick
	    break;
	}
	case 13: { // Voidance
	    break;
	}
	case 20: { // Silentfist
		// level 59
		// 9 minute reuse
		// Dragon punch damage bonus
		// Chance to stun
	    break;
	}
	case 5: { // Ashenhand
		// level 60
		// 72 minute reuse
		// Eagle Strike damage bonus
		// Chance to slay
	    break;
	}
	// Rogue
	case 19: { // Nimble
		// level 55
		// 12 second duration
		// 30 minute reuse
		// Auto dodge
	    break;
	}
	case 21: { // Kinesthetics
		// level 57
		// 18 second duration
		// 30 minute reuse
		// Auto dualwield
		// Auto double attack
	    break;
	}
	// Paladin
	case 22: { // Holyforge
		// level 55
		// 2 minute duration
		// 72 minute reuse
		// Crit/Crip undead
		// +15% to crit chance
	    break;
	}
	case 23: { // Sanctification
		// level 60
		// 10 second duration
		// 72 minute reuse
		// Spell immunity
	    break;
	}
	// Ranger
	case 24: { // Trueshot
		// level 55
		// 2 minute duration
		// 72 minute reuse
		// Max to two times max bow damage
		// +15% to hit
	    break;
	}
	case 25: { // Weaponshield
		// level 60
		// 15 second duration
		// 72 minute reuse
		// auto parry
	    break;
	}
	// Bard
	case 28: { // Deftdance
		// level 55
		// 10 second duration
		// 72 minute reuse
		// auto dodge
		// auto dualwield
	    break;
	}
	case 29: { // Puretone
		// level 60
		// 2 minute duration
		// 72 minute reuse
		// Auto instrument
	    break;
	}
	// Shadow knight
	case 26: { // Unholy
		// level 55
		// 72 minute reuse
		// +25% to harmtouch
		// -300 to resist
	    break;
	}
	case 27: { // Leech curse
		// level 60
		// 15 second duration
		// 72 minute reuse
		// Heal self for each point of melee damage done
	    break;
	}
	// Default
	case 0:{ // Timer request
		break;
	}
	default: 
	    LogFile->write(EQEMuLog::Error, "Unknown Discipline requested by client: %s class: %i Disciline:%i", GetName(), class_,disc_in->disc_id);
	    return;
    }
	disc_inuse = disc_in->disc_id;
}
void Client::DyeArmor(DyeStruct* dye)
{
	sint16 item_slot;
	int i;

	// solar: there's actually 9 elements, but even the interface in client
	// doesn't let you tint your weapon slots so we don't bother with those

	for(i = 0; i < 7; i++)
	{
		if(m_pp.item_tint[i].color != dye->dye[i].color)
		{
			// look for 'A Vial of Prismatic Dye'
			item_slot = GetInv().HasItem(32557, 1);
			if(item_slot != -1)
			{
				DeleteItemInInventory(item_slot, 1, true);
				m_pp.item_tint[i].color = dye->dye[i].color;
				SendWearChange(i);
			}
			else
			{
				Message(13, "Could not locate A Vial of Prismatic Dye.");
				return;
			}
		}
	}

	APPLAYER* outapp = new APPLAYER(OP_Dye, 0);
	QueuePacket(outapp);
	safe_delete(outapp);
	Save();
}
bool Client::CheckCheat(){
	float dx=cheat_x-x_pos;
	float dy=cheat_y-y_pos;
	float result=sqrt((dx*dx)+(dy*dy));
	return result>70;
}
void Client::SendGuildJoin(GuildJoin_Struct* gj){
	APPLAYER* outapp = new APPLAYER(OP_GuildManageAdd,sizeof(GuildJoin_Struct));
	GuildJoin_Struct* outgj=(GuildJoin_Struct*)outapp->pBuffer;
	outgj->class_=gj->class_;
	outgj->guildid=gj->guildid;
	outgj->level=gj->level;
	strcpy(outgj->name,gj->name);
	outgj->rank=gj->rank;
	outgj->zoneid=gj->zoneid;
	QueuePacket(outapp);
	safe_delete(outapp);
}
void Client::GuildChangeRank(int32 guildid,int32 oldrank,int32 newrank){
	GuildChangeRank(GetName(),guildid,oldrank,newrank);
}
void Client::GuildChangeRank(const char* name, int32 guildid,int32 oldrank,int32 newrank){
	APPLAYER* outapp = new APPLAYER(OP_GuildManageStatus,sizeof(GuildManageStatus_Struct));
	GuildManageStatus_Struct* gms=(GuildManageStatus_Struct*)outapp->pBuffer;
	gms->guildid=guildid;
	strcpy(gms->name,name);
	gms->newrank=newrank;
	gms->oldrank=oldrank;
	entity_list.QueueClientsGuild(this,outapp,false,guildid);
	safe_delete(outapp);
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

// these functions operate with a material slot, which is from 0 to 8
sint32 Client::GetEquipment(int8 material_slot)
{
	int invslot;
	const ItemInst *item;

	if(material_slot > 8)
	{
		return -1;
	}

	invslot = Inventory::CalcSlotFromMaterial(material_slot);
	if(invslot == -1)
	{
		return -1;
	}
	
	item = m_inv.GetItem(invslot);

	if(item != 0)
	{
		return item->GetItem()->ItemNumber;
	}

	return -1;
}

/*
sint32 Client::GetEquipmentMaterial(int8 material_slot)
{
	const Item_Struct *item;
	
	item = database.GetItem(GetEquipment(material_slot));
	if(item != 0)
	{
		return item->Common.Material;
	}

	return 0;
}
*/

sint32 Client::GetEquipmentColor(int8 material_slot)
{
	const Item_Struct *item;

	if(material_slot > 8)
	{
		return -1;
	}

	item = database.GetItem(GetEquipment(material_slot));
	if(item != 0)
	{
		return m_pp.item_tint[material_slot].rgb.use_tint ?
			m_pp.item_tint[material_slot].color :
			item->Common.Color;
	}

	return 0;
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


void Client::TradeskillSearchResults(const char *query, unsigned long qlen, 
  unsigned long objtype, unsigned long someid) {
	
	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
    
//printf("TradeskillSearchResults query ' %s '\n", query);
	if (!database.RunQuery(query, qlen, errbuf, &result)) {
		LogFile->write(EQEMuLog::Error, "Error in TradeskillSearchResults query '%s': %s", query, errbuf);
		return;
	}
	
	uint8 qcount = 0;
	
	qcount = mysql_num_rows(result);
	if(qcount < 1) {
		//search gave no results... not an error
		return;
	}
	if(mysql_num_fields(result) != 4) {
		LogFile->write(EQEMuLog::Error, "Error in TradeskillSearchResults query '%s': Invalid column count in result", query);
		return;		
	}
	
	uint8 r;
	//I could prolly get away with allocating a single APPLAYER, and
	//just re-using it, but this is safe, and im not sure.
	for(r = 0; r < qcount; r++) {
		row = mysql_fetch_row(result);
		uint32 recipe = (uint32)atoi(row[0]);
		const char *name = row[1];
		uint32 trivial = (uint32) atoi(row[2]);
		uint32 comp_count = (uint32) atoi(row[3]);
		
		APPLAYER* outapp = new APPLAYER(OP_RecipeReply, sizeof(RecipeReply_Struct));
		RecipeReply_Struct *reply = (RecipeReply_Struct *) outapp->pBuffer;
		
		reply->object_type = objtype;
		reply->some_id = someid;
		reply->component_count = comp_count;
		reply->recipe_id = recipe;
		reply->trivial = trivial;
		strncpy(reply->recipe_name, name, 63);
		
		QueuePacket(outapp);
		//DumpPacket(outapp);
		safe_delete(outapp);
	}
	mysql_free_result(result);
}

void Client::SendTradeskillDetails(unsigned long recipe_id) {

//from server in response to a 4 byte OP_RecipeDetails, just the item id
/*struct RecipeDetails_Struct {
	unsigned long recipe_id;	//backwards byte order from the Reply
	//dynamic part...
	// there are as many as 10 0xFFFFFFFF here in a row..
	// there are 10 - component count of them...
	
	//then one of these for each component:
	// unsigned long item_id;	//in backwards byte order
	// unsigned long icon_id;	//in backwards byte order
	// NULL terminated name...
	
};*/

	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
    char *query = 0;
	
	uint32 qlen = 0;
	uint8 qcount = 0;

	//pull the list of components
	qlen = MakeAnyLenString(&query, "SELECT tre.item_id,tre.componentcount,i.icon,i.Name "
	 " FROM tradeskill_recipe_entries AS tre "
	 " LEFT JOIN items AS i ON tre.item_id = i.id "
	 " WHERE tre.componentcount > 0 AND tre.recipe_id=%u", recipe_id);

	if (!database.RunQuery(query, qlen, errbuf, &result)) {
		LogFile->write(EQEMuLog::Error, "Error in SendTradeskillDetails query '%s': %s", query, errbuf);
		safe_delete_array(query);
		return;
	}
	safe_delete_array(query);
	
	qcount = mysql_num_rows(result);
	if(qcount < 1) {
		LogFile->write(EQEMuLog::Error, "Error in SendTradeskillDetails: no components returned");
		return;
	}
	if(qcount > 10) {
		LogFile->write(EQEMuLog::Error, "Error in SendTradeskillDetails: too many components returned (%u)", qcount);
		return;
	}
	
	//biggest this packet can ever be:
	// 64 * 10 + 8 * 10 + 4 + 4 * 10 = 764
	char *buf = new char[775];	//dynamic so we can just give it to APPLAYER
	uint8 r,k;
	
	unsigned long *header = (unsigned long *) buf;
	//Hell if I know why this is in the wrong byte order....
	*header = htonl(recipe_id);
	
	char *startblock = buf;
	startblock += sizeof(unsigned long);
	
	unsigned long *ffff_start = (unsigned long *) startblock;
	//fill in the FFFF's as if there were 0 items
	for(r = 0; r < 10; r++) {
		*ffff_start = 0xFFFFFFFF;
		ffff_start++;
	}
	char * datastart = (char *) ffff_start;
	char * cblock = (char *) ffff_start;
	
	unsigned long *itemptr;
	unsigned long *iconptr;
	uint32 len;
	uint32 datalen = 0;
	uint8 count = 0;
	for(r = 0; r < qcount; r++) {
		row = mysql_fetch_row(result);
		
		//watch for references to items which are not in the
		//items table, which the left join will make NULL...
		if(row[2] == NULL || row[3] == NULL) {
			continue;
		}
		
		uint32 item = (uint32)atoi(row[0]);
		uint8 num = (uint8) atoi(row[1]);
		
		
		uint32 icon = (uint32) atoi(row[2]);
		const char *name = row[3];
		len = strlen(name);
		if(len > 63)
			len = 63;
		
		//Hell if I know why these are in the wrong byte order....
		item = htonl(item);
		icon = htonl(icon);
		
		//if we get more than 10 items, just start skipping them...
		for(k = 0; k < num && count < 10; k++) {
			itemptr = (unsigned long *) cblock;
			cblock += sizeof(unsigned long);
			datalen += sizeof(unsigned long);
			iconptr = (unsigned long *) cblock;
			cblock += sizeof(unsigned long);
			datalen += sizeof(unsigned long);
			
			*itemptr = item;
			*iconptr = icon;
			strncpy(cblock, name, len);
			
			cblock[len] = '\0';	//just making sure.
			cblock += len + 1;	//get the null
			datalen += len + 1;	//gte the null
			count++;
		}
		
	}
	mysql_free_result(result);
	
	//now move the item data over top of the FFFFs
	uint8 dist = sizeof(unsigned long) * (10 - count);
	startblock += dist;
	memmove(startblock, datastart, datalen);
	
	uint32 total = sizeof(unsigned long) + dist + datalen;
	
	APPLAYER* outapp = new APPLAYER(OP_RecipeDetails);
	outapp->size = total;
	outapp->pBuffer = (uchar*) buf;
	QueuePacket(outapp);
	DumpPacket(outapp);
	safe_delete(outapp);
}

void Client::TradeskillExecute(DBTradeskillRecipe_Struct *spec, uint16 tradeskill) {
	if(spec == NULL || tradeskill == 0)
		return;
	
	sint16 user_skill = (sint16) GetSkill(tradeskill);
	float chance = 0;
	
	// statbonus 20%/10% with 200 + 0.05% / 0.025% per point above 200
	float wisebonus =  (m_pp.WIS > 200) ? 20 + ((m_pp.WIS - 200) * 0.05) : m_pp.WIS * 0.1;
	float intbonus =  (m_pp.INT > 200) ? 10 + ((m_pp.INT - 200) * 0.025) : m_pp.INT * 0.05;
	
	vector< pair<uint32,uint8> >::iterator itr;
	
	//Reworked this because it seemed to use spec->skill_needed as spec->trivial...
	if(spec->nofail) {
		chance = 100;	//cannot fail.
	} else if(((sint16)user_skill - (sint16)spec->skill_needed) < 0) {
		chance = 0;
		//impossible... is there a message for this???
	} else if (((sint16)user_skill - (sint16)spec->trivial) > 0) {
		chance = 80+wisebonus-10; // 80% basechance + max 20% stats
		Message_StringID(4,TRADESKILL_TRIVIAL);
	} else {
		if ((spec->trivial - user_skill) < 20) {
			// 40 base chance success + max 40% skill + 20% max stats
			chance = 40 + wisebonus + 40 - ((spec->trivial - user_skill)*2);
		}
		else {
			// 0 base chance success + max 30% skill + 10% max stats
			chance = 0 + (wisebonus/2) + 30 - (((spec->trivial - user_skill) * (spec->trivial - user_skill))*0.01875);
		}
		
//Is there a reason we dont use CheckIncreaseSkill()?
		// skillincrease?
		if ((55-(user_skill*0.236))+intbonus > (float)rand()/RAND_MAX*100) {
			SetSkill(tradeskill, user_skill + 1);
			//Message(4, "You have become better at (skillid=%i)", tradeskill);
		}
	}
	
	float res = ((float)rand()/RAND_MAX*100);
	if ((tradeskill==75) || GetGM() || (chance > res)){
		Message_StringID(4,TRADESKILL_SUCCEED);
		
		itr = spec->onsuccess.begin();
		while(itr != spec->onsuccess.end()) {
			//should we check this crap?
			SummonItem(itr->first, itr->second);
			itr++;
		}
	} else {
		Message_StringID(4,TRADESKILL_FAILED);
		
		itr = spec->onfail.begin();
		while(itr != spec->onfail.end()) {
			//should we check these arguments?
			SummonItem(itr->first, itr->second);
			itr++;
		}
	}
}

