/*  EQEMu:  Everquest Server Emulator
Copyright (C) 2001-2004  EQEMu Development Team (http://eqemu.org)

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
#include "AA.h"
#include "mob.h"
#include "client.h"
#include "groups.h"
#include "spdat.h"
#include "object.h"
#include "doors.h"
#include "beacon.h"
#include "PlayerCorpse.h"
#include "../common/races.h"
#include "../common/classes.h"
#include "../common/eq_packet_structs.h"
#include "StringIDs.h"

#ifndef NEW_LoadSPDat
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif

//static data arrays, really not big enough to warrant shared mem.
AA_DBAction AA_Actions[aaHighestID][MAX_AA_ACTION_RANKS];	//[aaid][rank]
map<int16, AA_SwarmPet> AA_SwarmPets;	//key=spell_id

/*

Schema:

spell_id is spell to cast, SPELL_UNKNOWN == no spell
nonspell_action is action to preform on activation which is not a spell, 0=none
nonspell_mana is mana that the nonspell action consumes
nonspell_duration is a duration which may be used by the nonspell action
redux_aa is the aa which reduces the reuse timer of the skill
redux_rate is the multiplier of redux_aa, as a percentage of total rate (10 == 10% faster)

CREATE TABLE aa_actions (
	aaid mediumint unsigned not null,
	rank tinyint unsigned not null,
	reuse_time mediumint unsigned not null,
	spell_id mediumint unsigned not null,
	target tinyint unsigned not null,
	nonspell_action tinyint unsigned not null,
	nonspell_mana mediumint unsigned not null,
	nonspell_duration mediumint unsigned not null,
	redux_aa mediumint unsigned not null,
	redux_rate tinyint not null,
	
	PRIMARY KEY(aaid, rank)
);

CREATE TABLE aa_swarmpets (
	spell_id mediumint unsigned not null,
	count tinyint unsigned not null,
	npc_id int not null,
	duration mediumint unsigned not null,
	PRIMARY KEY(spell_id)
);
*/

/*

Credits for this function:
	-FatherNitwit: Structure and mechanism
	-Wiz: Initial set of AAs, original function contents
	-Branks: Much updated info and a bunch of higher-numbered AAs

*/
void Client::ActivateAA(aaID activate){
	if(activate < 0 || activate >= aaHighestID)
		return;
	
	uint8 activate_val = GetAA(activate);
	
	if (activate_val == 0 || IsStunned() || IsMezzed() || IsSitting())
		return;
	
	if(!p_timers.Expired(pTimerAAStart + activate)) {
		return;
	}
	
	if(activate_val > MAX_AA_ACTION_RANKS)
		activate_val = MAX_AA_ACTION_RANKS;
	activate_val--;		//to get array index.
	
	//get our current node, now that the indices are well bounded
	const AA_DBAction *caa = &AA_Actions[activate][activate_val];
	
	//everything should be configured out now
	
	int16 target_id = 0;
	
	//figure out our target
	switch(caa->target) {
		case aaTargetUser:
			target_id = GetID();
			break;
		case aaTargetCurrent:
			if(GetTarget() == NULL) {
				Message(0, "A target is required for this skill.");
				return;
			}
			target_id = GetTarget()->GetID();
			break;
		case aaTargetGroup:
			target_id = GetID();
			break;
		case aaTargetCurrentGroup:
			if(GetTarget() == NULL) {
				Message(0, "A target is required for this skill.");
				return;
			}
			target_id = GetTarget()->GetID();
			break;
		case aaTargetPet:
			if(GetPet() == NULL) {
				Message(0, "A pet is required for this skill.");
				return;
			}
			target_id = GetPetID();
			break;
	}
	
	//handle non-spell action
	if(caa->action != aaActionNone) {
		if(caa->mana_cost > 0) {
			if(GetMana() < caa->mana_cost) {
				Message(0, "Not enough mana to use this skill.");
				return;
			}
			SetMana(GetMana() - caa->mana_cost);
		}
		HandleAAAction(activate);
	}
	
	//cast the spell, if we have one
	if(caa->spell_id > 0 && caa->spell_id < SPDAT_RECORDS) {
		//I dont know when we need to mem and when we do not, if ever...
		//MemorizeSpell(8, spell_id, 3);
		CastSpell(caa->spell_id, target_id);
	}
	
	//set our re-use timer.
	if(caa->reuse_time > 0) {
		int32 redux = 0;
		if(caa->redux_aa > 0 && caa->redux_aa < aaHighestID) {
			redux = GetAA(caa->redux_aa) * caa->redux_rate;
		}
		
		int32 timer_base = caa->reuse_time * (100 - redux) / 100;
		
		//start the usage timer
		p_timers.Start(pTimerAAStart + activate, timer_base);
		
		//notify the client
		//I do not know why we do not put the proper end time in here:
		time_t timestamp = time(NULL);
		SendAATimer(activate, timestamp, timestamp);
	}
}

void Client::HandleAAAction(aaID activate) {
	if(activate < 0 || activate >= aaHighestID)
		return;
	
	uint8 activate_val = GetAA(activate);
	
	if (activate_val == 0)
		return;
	
	if(activate_val > MAX_AA_ACTION_RANKS)
		activate_val = MAX_AA_ACTION_RANKS;
	activate_val--;		//to get array index.
	
	//get our current node, now that the indices are well bounded
	const AA_DBAction *caa = &AA_Actions[activate][activate_val];
	
	uint16 timer_id = 0;
	uint16 timer_duration = caa->duration;
	aaTargetType target = aaTargetUser;
	
	int16 spell_id = SPELL_UNKNOWN;	//gets cast at the end if not still unknown
	
	switch(caa->action) {
		case aaActionAETaunt:
			entity_list.AETaunt(this);
			break;
			
		case aaActionMassBuff:
			EnableAAEffect(aaEffectMassGroupBuff);
			break;
		
		case aaActionFlamingArrows:
			//toggle it
			if(CheckAAEffect(aaEffectFlamingArrows))
				EnableAAEffect(aaEffectFlamingArrows);
			else
				DisableAAEffect(aaEffectFlamingArrows);
			break;
		
		case aaActionFrostArrows:
			if(CheckAAEffect(aaEffectFrostArrows))
				EnableAAEffect(aaEffectFrostArrows);
			else
				DisableAAEffect(aaEffectFrostArrows);
			break;
		
		case aaActionRampage:
			EnableAAEffect(aaEffectRampage);
			timer_id = aaEffectRampage;
			break;
		
		case aaActionSharedHealth:
			if(CheckAAEffect(aaEffectSharedHealth))
				EnableAAEffect(aaEffectSharedHealth);
			else
				DisableAAEffect(aaEffectSharedHealth);
			break;
		
		case aaActionCelestialRegen: {
			//special because spell_id depends on a different AA
			switch (GetAA(aaCelestialRenewal)) {
				case 1:
					spell_id = 3250;
					break;
				case 2:
					spell_id = 3251;
					break;
				default:
					spell_id = 2740;
					break;
			}
			break;
		}
		
		case aaActionDireCharm: {
			//special because spell_id depends on class
			switch (GetClass())
			{
				case DRUID:
					spell_id = 2760; 	//2644?
					break;
				case NECROMANCER:
					spell_id = 2759;	//2643?
					break;
				case ENCHANTER:
					spell_id = 2761;	//2642?
					break;
			}
			break;
		}
		
		case aaActionImprovedFamiliar: {
			//Spell IDs might be wrong...
			if (GetAA(aaAllegiantFamiliar))
				spell_id = 3264;	//1994?
			else
				spell_id = 2758;	//2155?
			break;
		}
		
		case aaActionActOfValor:
			if(GetTarget() != NULL) {
				int heal = GetHP();
				int curhp = GetTarget()->GetHP();
				GetTarget()->SetHP(heal+curhp);
				Death(this,0,0,0);
			}
			break;
		
		case aaActionSuspendedMinion:
			if (GetPet()) {
				target = aaTargetPet;
				switch (GetAA(aaSuspendedMinion)) {
					case 1:
						spell_id = 3248;
						break;
					case 2:
						spell_id = 3249;
						break;
				}
				//do we really need to cast a spell?
				
				Message(0,"You call your pet to your side.");
				GetPet()->WhipeHateList();
				GetPet()->GMMove(GetX(),GetY(),GetZ());
				if (activate_val > 1)
					entity_list.ClearFeignAggro(GetPet());
			} else {
				Message(0,"You have no pet to call.");
			}
			break;
		
		case aaActionEscape:
			Escape();
			break;
			
		case aaBeastialAlignment:
			switch(GetBaseRace()) {
				case BARBARIAN:
					spell_id = AA_Choose3(activate_val, 4521, 4522, 4523);
					break;
				case TROLL:
					spell_id = AA_Choose3(activate_val, 4524, 4525, 4526);
					break;
				case OGRE:
					spell_id = AA_Choose3(activate_val, 4527, 4527, 4529);
					break;
				case IKSAR:
					spell_id = AA_Choose3(activate_val, 4530, 4531, 4532);
					break;
				case VAHSHIR:
					spell_id = AA_Choose3(activate_val, 4533, 4534, 4535);
					break;
			}
		
		default:
			LogFile->write(EQEMuLog::Error, "Unknown AA nonspell action type %d", caa->action);
			return;
	}
	
	
	int16 target_id = 0;
	
	//figure out our target
	switch(target) {
		case aaTargetUser:
			target_id = GetID();
			break;
		case aaTargetCurrent:
			if(GetTarget() == NULL) {
				Message(0, "A target is required for this skill.");
				return;
			}
			target_id = GetTarget()->GetID();
			break;
		case aaTargetGroup:
			target_id = GetID();
			break;
		case aaTargetCurrentGroup:
			if(GetTarget() == NULL) {
				Message(0, "A target is required for this skill.");
				return;
			}
			target_id = GetTarget()->GetID();
			break;
		case aaTargetPet:
			if(GetPet() == NULL) {
				Message(0, "A pet is required for this skill.");
				return;
			}
			target_id = GetPetID();
			break;
	}
	
	//cast the spell, if we have one
	if(spell_id > 0 && spell_id < SPDAT_RECORDS) {
		//I dont know when we need to mem and when we do not, if ever...
		//MemorizeSpell(8, spell_id, 3);
		CastSpell(spell_id, target_id);
	}
	
	//handle the duration timer if we have one.   
	if(timer_id > 0 && timer_duration > 0) {
		p_timers.Start(pTimerAAEffectStart + timer_id, timer_duration);
	}
}


//Originally written by Branks
//functionality rewritten by Father Nitwit
void Client::TemporaryPets(int16 spell_id) {
	
	//It might not be a bad idea to put these into the database, eventually..
	
	//Dook- swarms and wards 
	Mob* targ = GetTarget();	//if null, they dont get any hate
	
	if(AA_SwarmPets.count(spell_id) != 1) {
		//log write
		LogFile->write(EQEMuLog::Error, "Unknown swarm pet spell id: %d", spell_id);
		Message(0,"Unknown pet!");
		return;
	}
	
	const AA_SwarmPet &pet = AA_SwarmPets[spell_id];
	
	const NPCType *npc_type = database.GetNPCType(pet.npc_id);
	if(npc_type == NULL) {
		//log write
		LogFile->write(EQEMuLog::Error, "Unknown npc type for swarm pet spell id: %d", spell_id);
		Message(0,"Unable to find pet!");
		return;
	}
	
	int summon_count = 0;
	summon_count = pet.count;
	
	if(summon_count > MAX_SWARM_PETS)
		summon_count = MAX_SWARM_PETS;
	
	static const float swarm_pet_x[MAX_SWARM_PETS] = { 	5, -5, 5, -5, 
														10, -10, 10, -10,
														8, -8, 8, -8 };
	static const float swarm_pet_y[MAX_SWARM_PETS] = { 	5, 5, -5, -5, 
														10, 10, -10, -10,
														8, 8, -8, -8 };
	
	while(summon_count > 0) {
		NPC* npca = new NPC(npc_type, 0, 
				GetX()+swarm_pet_x[summon_count], GetY()+swarm_pet_y[summon_count], 
				GetZ(), GetHeading());
		npca->SetOwnerID(GetID());
		entity_list.AddNPC(npca);
		if(targ != NULL)
			npca->AddToHateList(targ, 1000, 1000);
		npca->StartSwarmTimer(pet.duration);
		
		summon_count--;
	}
	
	if(targ != NULL)
		targ->AddToHateList(this, 1, 0);
}

//turn on an AA effect
//duration == 0 means no time limit, used for one-shot deals, etc..
void Client::EnableAAEffect(aaEffectType type, int32 duration) {
	if(type > 32)
		return;	//for now, special logic needed.
	m_pp.aa_effects |= 1 << (type-1);
	
	if(duration > 0) {
		p_timers.Start(pTimerAAEffectStart + type, duration);
	} else {
		p_timers.Clear(pTimerAAEffectStart + type);
	}
}

void Client::DisableAAEffect(aaEffectType type) {
	if(type > 32)
		return;	//for now, special logic needed.
	uint32 bit = 1 << (type-1);
	if(m_pp.aa_effects & bit) {
		m_pp.aa_effects ^= bit;
	}
	p_timers.Clear(pTimerAAEffectStart + type);
}

/*
By default an AA effect is a one shot deal, unless
a duration timer is set.
*/
bool Client::CheckAAEffect(aaEffectType type) {
	if(type > 32)
		return(false);	//for now, special logic needed.
	if(m_pp.aa_effects & (1 << (type-1))) {	//is effect enabled?
		//has our timer expired?
		if(p_timers.Expired(pTimerAAEffectStart + type)) {
			DisableAAEffect(type);
			return(false);
		}
		return(true);
	}
	return(false);
}

void Client::SendAAStats() {
	APPLAYER* outapp = new APPLAYER(OP_SendAAStats, sizeof(AltAdvStats_Struct));
	AltAdvStats_Struct *aps = (AltAdvStats_Struct *)outapp->pBuffer;
	aps->experience = m_pp.expAA;
	aps->experience = (int32)(((float)330.0f * (float)m_pp.expAA) / (float)max_AAXP);
	aps->unspent = m_pp.aapoints;
	aps->percentage = m_pp.perAA;
	QueuePacket(outapp);
	safe_delete(outapp);
}

void Client::BuyAA(AA_Action* action){
	
	//find the AA information from the database
	SendAA_Struct* aa2 = zone->FindAA(action->ability);
	if(!aa2) {
		for(int i=1;i<5;i++){
			if((aa2 = zone->FindAA(action->ability-i)))
				break;
		}
	}
	if(!aa2)
		return;	//invalid ability...
	
	int32 cur_level = GetAA(aa2->id);
	
	if(m_pp.aapoints >= aa2->cost && cur_level < aa2->max_level) {
		SetAA(aa2->id, cur_level+1);
		
		m_pp.aapoints -= aa2->cost;
		database.SetPlayerAlternateAdv(account_id, m_pp.name, &aa);
		Save();
		
		SendAA(aa2->id);
		SendAATable();
		char val1[20]={0};
		char val2[20]={0};
		char val3[20]={0};
		char points[20]={0};
		char point[20]={0};
		const char* points2=ConvertArray(AA_POINTS,points);
		const char* point2=ConvertArray(AA_POINT,point);
		if(cur_level<1){
			if(aa2->cost>1)
				Message_StringID(15,AA_GAIN_ABILITY,ConvertArray(aa2->title_sid,val1),ConvertArray(aa2->cost,val2),points2);
			else
				Message_StringID(15,AA_GAIN_ABILITY,ConvertArray(aa2->title_sid,val1),ConvertArray(aa2->cost,val2),point2);
		}
		else{
			if(aa2->cost>1)
				Message_StringID(15,AA_IMPROVE,ConvertArray(aa2->title_sid,val1),ConvertArray(cur_level,val2),ConvertArray(aa2->cost,val3),points2);
			else
				Message_StringID(15,AA_IMPROVE,ConvertArray(aa2->title_sid,val1),ConvertArray(cur_level,val2),ConvertArray(aa2->cost,val3),point2);
		}
		SendAAStats();
		
		CalcBonuses();
	}
}

void Client::SendAATimer(int32 ability, int32 begin, int32 end) {
	APPLAYER* outapp = new APPLAYER(OP_AAAction,sizeof(UseAA_Struct));
	UseAA_Struct* uaaout = (UseAA_Struct*)outapp->pBuffer;
	uaaout->ability = ability;
	uaaout->begin = begin;
	uaaout->end = end;
	QueuePacket(outapp);
	safe_delete(outapp);
}

//sends all AA timers.
void Client::SendAATimers() {
	//we dont use SendAATimer because theres no reason to allocate the APPLAYER every time
	APPLAYER* outapp = new APPLAYER(OP_AAAction,sizeof(UseAA_Struct));
	UseAA_Struct* uaaout = (UseAA_Struct*)outapp->pBuffer;
	
	PTimerList::iterator c,e;
	c = p_timers.begin();
	e = p_timers.end();
	while(c != e) {
		PersistentTimer *cur = c->second;
		//send timer
		uaaout->begin = cur->GetStartTime();
		uaaout->end = uaaout->begin + cur->GetTimerTime();
		uaaout->ability = cur->GetType();
		QueuePacket(outapp);
		
		c++;
	}
	
	safe_delete(outapp);
}

void Client::SendAATable() {
    APPLAYER* outapp = new APPLAYER(OP_RespondAA, sizeof(AATable_Struct));
    
    AATable_Struct* aa2 = (AATable_Struct *)outapp->pBuffer;
	for(int i=0;i < MAX_PP_AA_ARRAY;i++) {
		if(aa.aa_list[i].aa_value>1)
			aa2->aa_list[i].aa_skill = aa.aa_list[i].aa_skill + aa.aa_list[i].aa_value - 1;
		else
			aa2->aa_list[i].aa_skill=aa.aa_list[i].aa_skill;
		aa2->aa_list[i].aa_value=aa.aa_list[i].aa_value;
	}
	outapp->Deflate();
    QueuePacket(outapp);
    safe_delete(outapp);
}

void Client::SendAA(int32 id, int seq, bool update) {
	uint32 value=0;
	SendAA_Struct* saa2 = NULL;
	if(id==0)
		saa2 = zone->GetAAList()->aa[seq];
	else
		saa2 = zone->FindAA(id);
	
	int16 classes = saa2->classes;
	if(!(classes & (1 << GetClass())) && (GetClass()!=BERSERKER || saa2->berserker==0)){
		return;
	}
	
	int size=sizeof(SendAA_Struct)+sizeof(AA_Ability)*saa2->total_abilities;
	uchar* buffer = new uchar[size];
	SendAA_Struct* saa=(SendAA_Struct*)buffer;
	memcpy(saa,saa2,size);
	
	if(saa->spellid==0)
		saa->spellid=0xFFFFFFFF;
	
	value=GetAA(saa->id);
	
	if(value){
		if(value < saa->max_level){
			saa->id+=value;
			saa->next_id=saa->id+1;
			value++;
		}
		else{
			saa->id+=value-1;
			saa->next_id=0xFFFFFFFF;
		}
		saa->last_id=saa->id-1;
		saa->current_level=value;
		saa->cost2=value;
		if(saa->type==1) //general ability
			saa->abilities[0].increase_amt*=value;
	}
//printf("Sending AA(%d): id=%d pr=%d prm=%d\n", id, saa->id, saa->prereq_skill, saa->prereq_minpoints);
	APPLAYER* outapp = new APPLAYER(OP_SendAATable);
	outapp->size=size;
	outapp->pBuffer=(uchar*)saa;
	QueuePacket(outapp);
	safe_delete(outapp);
	//will outapp delete the buffer for us even though it didnt make it?
}

void Client::SendAAList(){
	int total = zone->GetTotalAAs();
	for(int i=0;i < total;i++){
		SendAA(0,i);
	}
}

int32 Client::GetAA(int32 aa_id) {
	/*
	old version:
	uint8 *aa_ = &(((uint8 *)&aa)[aa_id]);
	return (uint16)*aa_;
	
	I think this function should be converted back to the old
	array based system if possible because the GetAA() method is 
	used hundreds of times, all over the place, and looping
	through a possible 120 elements is pretty crazy to do
	*/
	
	//new version
	if(!aa.aa_list)
		return 0;
	for(int i=0;i < MAX_PP_AA_ARRAY;i++){
		if(aa.aa_list[i].aa_skill==aa_id)
			return aa.aa_list[i].aa_value;
		else if(aa.aa_list[i].aa_skill==0)
			break;
	}
	return 0;
}

bool Client::SetAA(int32 aa_id, int32 new_value) {
	if(!aa.aa_list)
		return false;
		
	for(int cur=0;cur < MAX_PP_AA_ARRAY;cur++){
		if(aa.aa_list[cur].aa_skill==aa_id){
			aa.aa_list[cur].aa_value=new_value;
			return true;
		}
		else if(aa.aa_list[cur].aa_skill==0){ //end of list
			aa.aa_list[cur].aa_skill=aa_id;
			aa.aa_list[cur].aa_value=new_value;
			return true;
		}
	}
	
	return false;
}

SendAA_Struct* Zone::FindAA(int32 id) {
	SendAA_Struct* ret = NULL;
	for(int i=0; i < totalAAs; i++) {
		if(aas->aa[i]->id == id) {
			ret = aas->aa[i];
			break;
		}
	}
	return ret;
}

void Zone::LoadAAs() {
	int32 size=database.GetSizeAA();
	if(size>=sizeof(SendAA_Struct)){
		aa_buffer = new uchar[size];
		aas=(AA_List*)aa_buffer;
		database.LoadAAs(aas);
		totalAAs=database.CountAAs();
	}
	else{
		LogFile->write(EQEMuLog::Error, "Failed to load AAs!");
		aas=NULL;
	}
}

bool Database::LoadAAEffects() {
	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	memset(AA_Actions, 0, sizeof(AA_Actions));	//I hope the compiler is smart about this size...
	
	const char *query = "SELECT aaid,rank,reuse_time,spell_id,target,nonspell_action,nonspell_mana,nonspell_duration,redux_aa,redux_rate FROM aa_actions";
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		//safe_delete_array(query);
		int r;
		while ((row = mysql_fetch_row(result))) {
			r = 0;
			int aaid = atoi(row[r++]);
			int rank = atoi(row[r++]);
			if(aaid < 0 || aaid >= aaHighestID || rank < 0 || rank >= MAX_AA_ACTION_RANKS)
				continue;
			AA_DBAction *caction = &AA_Actions[aaid][rank];
			
			caction->reuse_time = atoi(row[r++]);
			caction->spell_id = atoi(row[r++]);
			caction->target = (aaTargetType) atoi(row[r++]);
			caction->action = (aaNonspellAction) atoi(row[r++]);
			caction->mana_cost = atoi(row[r++]);
			caction->duration = atoi(row[r++]);
			caction->redux_aa = (aaID) atoi(row[r++]);
			caction->redux_rate = atoi(row[r++]);
				
		}
		mysql_free_result(result);
	}
	else {
		LogFile->write(EQEMuLog::Error, "Error in LoadAAEffects query '%s': %s", query, errbuf);;
		//safe_delete_array(query);
		return false;
	}

	return true;
}

bool Database::LoadSwarmSpells() {
	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;

	AA_SwarmPets.clear();
	
	const char *query = "SELECT spell_id,count,npc_id,duration FROM aa_swarmpets";
	if (RunQuery(query, strlen(query), errbuf, &result)) {
		//safe_delete_array(query);
		int r;
		while ((row = mysql_fetch_row(result))) {
			r = 0;
			int16 spell_id = atoi(row[r++]);
			if(spell_id > SPDAT_RECORDS)
				continue;
			AA_SwarmPet pet;
			
			pet.count = atoi(row[r++]);
			pet.npc_id = strtoul(row[r++], NULL, 10);
			pet.duration = atoi(row[r++]);
			
			AA_SwarmPets[spell_id] = pet;
		}
		mysql_free_result(result);
	}
	else {
		LogFile->write(EQEMuLog::Error, "Error in LoadSwarmSpells query '%s': %s", query, errbuf);;
		//safe_delete_array(query);
		return false;
	}

	return true;
}

/*
Get the name of the alternate advancement skill with the given 'index'.
Return true if the name was found, otherwise false.
False will also be returned if there is a database error.
*/
int8 Database::GetTotalAALevels(int32 skill_id) {
char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	int total=0;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT count(ability) from aa_levels where aa_id=%i", skill_id), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			total=atoi(row[0]);
		}
		mysql_free_result(result);
	} else {
		LogFile->write(EQEMuLog::Error, "Error in GetTotalAALevels '%s: %s", query, errbuf);
		safe_delete_array(query);
	}
	return total;
}
int32 Database::CountAAs(){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	int count=0;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT count(title_sid) from altadv_vars"), errbuf, &result)) {
		if((row = mysql_fetch_row(result))!=NULL)
			count = atoi(row[0]);
	}
	safe_delete_array(query);
	mysql_free_result(result);
	return count;
}
int32 Database::CountAALevels(){
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	int count=0;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT count(id) from aa_levels"), errbuf, &result)) {
		if((row = mysql_fetch_row(result))!=NULL){
			count = atoi(row[0]);
			mysql_free_result(result);
		}
	}
	safe_delete_array(query);
	return count;
}
int32 Database::GetSizeAA(){
	int size=CountAAs()*sizeof(SendAA_Struct);
	if(size>0)
		size+=CountAALevels()*sizeof(AA_Ability);
	return size;
}
void Database::LoadAAs(AA_List* load){
	if(!load)
		return;
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT skill_id from altadv_vars order by skill_id"), errbuf, &result)) {
		int skill=0,ndx=0;
		while((row = mysql_fetch_row(result))!=NULL) {
			skill=atoi(row[0]);
			load->aa[ndx]=GetAASkillVars(skill);
			load->aa[ndx]->seq=ndx+1;
			ndx++;
		}
	}
	safe_delete_array(query);
	mysql_free_result(result);
}
void Database::RetrieveAALevels(SendAA_Struct* aa_struct){
	if(!aa_struct)
		return;
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT ability, increase_amt, level from aa_levels where aa_id=%i order by level asc", aa_struct->id), errbuf, &result)) {
		int ndx=0;
		while((row = mysql_fetch_row(result))!=NULL) {
			aa_struct->abilities[ndx].skill_id=atoi(row[0]);
			aa_struct->abilities[ndx].increase_amt=atoi(row[1]);
			aa_struct->abilities[ndx].last_level=atoi(row[2]);
			ndx++;
		}
	}
	safe_delete_array(query);
	mysql_free_result(result);
}

SendAA_Struct* Database::GetAASkillVars(int32 skill_id)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	SendAA_Struct* sendaa = NULL;
	uchar* buffer;
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT cost, max_level, hotkey_sid, hotkey_sid2, title_sid, desc_sid, type, prereq_skill, prereq_minpoints, spell_type, spell_refresh, classes, berserker,spellid FROM altadv_vars WHERE skill_id=%i", skill_id), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			int total_abilities = GetTotalAALevels(skill_id);
			int totalsize = total_abilities * sizeof(AA_Ability) + sizeof(SendAA_Struct);
			buffer = new uchar[totalsize];
			memset(buffer,0,totalsize);
			row = mysql_fetch_row(result);
			sendaa = (SendAA_Struct*)buffer;
			
			//ATOI IS NOT UNISGNED LONG-SAFE!!!
			
			sendaa->cost = atoul(row[0]);
			sendaa->cost2 = sendaa->cost;
			sendaa->max_level = atoul(row[1]);
			sendaa->hotkey_sid = atoul(row[2]);
			sendaa->id = skill_id;
			sendaa->hotkey_sid2 = atoul(row[3]);
			sendaa->title_sid = atoul(row[4]);
			sendaa->desc_sid = atoul(row[5]);
			sendaa->type = atoul(row[6]);
			sendaa->prereq_skill = atoul(row[7]);
			sendaa->prereq_minpoints = atoul(row[8]);
			sendaa->spell_type = atoul(row[9]);
			sendaa->spell_refresh = atoul(row[10]);
			sendaa->classes = atoul(row[11]);
			sendaa->berserker = atoul(row[12]);
			sendaa->last_id = 0xFFFFFFFF;
			sendaa->current_level=1;
			sendaa->spellid = atoul(row[14]);
			switch(sendaa->type){
				case 1:
					sendaa->class_type=0x33;
					break;
				case 2:
					sendaa->class_type=0x37;
					break;
				case 3:
					sendaa->class_type=0x3B;
					break;
				case 4:
					sendaa->class_type=0x3D;
					break;
				case 5:
					sendaa->class_type=0x3E;
					break;
			}
			sendaa->total_abilities=total_abilities;
			if(sendaa->hotkey_sid==0xFFFFFFFF)
				sendaa->next_id=skill_id+1;
			else
				sendaa->next_id=0xFFFFFFFF;
			RetrieveAALevels(sendaa);
		}
		mysql_free_result(result);
	} else {
		LogFile->write(EQEMuLog::Error, "Error in GetAASkillVars '%s': %s", query, errbuf);
		safe_delete_array(query);
	}
	return sendaa;
}

/*
Update the player alternate advancement table for the given account "account_id" and character name "name"
Return true if the character was found, otherwise false.
False will also be returned if there is a database error.
*/
bool Database::SetPlayerAlternateAdv(int32 account_id, char* name, PlayerAA_Struct* aa)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[256+sizeof(PlayerAA_Struct)*2+1];
	char* end = query;
	
	//if (strlen(name) > 15)
	//	return false;
	
	/*for (int i=0; i< 'a' || name[i] > 'z') && 
	(name[i] < 'A' || name[i] > 'Z') && 
	(name[i] < '0' || name[i] > '9'))
	return 0;
}*/
	
	
	end += sprintf(end, "UPDATE character_ SET alt_adv=\'");
	end += DoEscapeString(end, (char*)aa, sizeof(PlayerAA_Struct));
	*end++ = '\'';
	end += sprintf(end," WHERE account_id=%d AND name='%s'", account_id, name);
	
	int32 affected_rows = 0;
    if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows)) {
        LogFile->write(EQEMuLog::Error, "Error in SetPlayerAlternateAdv query '%s': %s", query, errbuf);
		return false;
    }
	
	if (affected_rows == 0) {
		return false;
	}
	
	return true;
}

/*
Update the player alternate advancement table for the given account "account_id" and character name "name"
Return true if the character was found, otherwise false.
False will also be returned if there is a database error.
*/
/*bool Database::SetPlayerAlternateAdv(int32 account_id, char* name, PlayerAA_Struct* aa)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char query[256+sizeof(PlayerAA_Struct)*2+1];
	char* end = query;
	
	end += sprintf(end, "UPDATE character_ SET alt_adv=\'");
	end += DoEscapeString(end, (char*)aa, sizeof(PlayerAA_Struct));
	*end++ = '\'';
	end += sprintf(end," WHERE account_id=%d AND name='%s'", account_id, name);
	
	int32 affected_rows = 0;
    if (!RunQuery(query, (int32) (end - query), errbuf, 0, &affected_rows)) {
        LogFile->write(EQEMuLog::Error, "Error in SetPlayerAlternateAdv query: %s", errbuf);
		return false;
    }
	
	if (affected_rows == 0) {
		return false;
	}
	
	return true;
}*/

/*
Get the player alternate advancement table for the given account "account_id" and character name "name"
Return true if the character was found, otherwise false.
False will also be returned if there is a database error.
*/
/*int32 Database::GetPlayerAlternateAdv(int32 account_id, char* name, PlayerAA_Struct* aa)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    char *query = 0;
    MYSQL_RES *result;
    MYSQL_ROW row;
	
	unsigned long* lengths;
	unsigned long len = 0;
	
	if (RunQuery(query, MakeAnyLenString(&query, "SELECT alt_adv FROM character_ WHERE account_id=%i AND name='%s'", account_id, name), errbuf, &result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {	
			row = mysql_fetch_row(result);
			lengths = mysql_fetch_lengths(result);
			len = result->lengths[0];
			//if (lengths[0] == sizeof(PlayerAA_Struct)) {
			if(row[0] && lengths[0] >= sizeof(PlayerAA_Struct)) {
				memcpy(aa, row[0], sizeof(PlayerAA_Struct));
			} else { // let's support ghetto-ALTERed databases that don't contain any data in the alt_adv column
				memset(aa, 0, sizeof(PlayerAA_Struct));
				len = sizeof(PlayerAA_Struct);
			}
			//}
			//else {
			//cerr << "Player alternate advancement table length mismatch in GetPlayerAlternateAdv" << endl;
			//mysql_free_result(result);
			//return false;
			//}
		}
		else {
			mysql_free_result(result);
			return 0;
		}
		mysql_free_result(result);
		//		unsigned long len=result->lengths[0];
		return len;
	}
	else {
		LogFile->write(EQEMuLog::Error, "Error in GetPlayerAlternateAdv '%s': %s", query, errbuf);
		safe_delete_array(query);
		return 0;
	}
	
	//return true;
}*/


/*

this is here because theres a ton of information init,
and I am not sure that it is all in the DB yet.

	switch(activate){
		case aaMassGroupBuff:
			EnableAAEffect(aaEffectMassGroupBuff);
			timer_base = 4320;
			target = aaTargetCurrentGroup;
			break;
			
		case aaDivineResurrection: //Divine Res
			spell_id = 2738;
			timer_base = 64800;
			target = aaTargetCurrent;
			break;
			
		case aaInnateInvisToUndead: //Innate Undead Invis
			spell_id = 1047;
			timer_base = 7;
			break;
			
		case aaCelestialRegeneration: //Celestial Regen
			switch (GetAA(aaCelestialRenewal))
			{
				case 1:
					spell_id = 3250;
					break;
				case 2:
					spell_id = 3251;
					break;
				default:
					spell_id = 2740;
					break;
			}
			target = aaTargetCurrent;
			timer_base = 900;
			break;
			
		case aaBestowDivineAura: //Bestow DA
			dedux = 10*GetAA(aaHastenedDivinity);
			spell_id = 2690;
//MISSING timer value, this is wrong:
			timer_base = 8640;
			target = aaTargetCurrent;
			break;
			
		case aaTurnUndead: //Turn Undead
			dedux = 10*GetAA(aaHastenedTurning);
			spell_id = AA_Choose3(activate_val, 2776, 2777, 2778);
			timer_base = 4320;
			target = aaTargetCurrent;
			break;
			
		case aaPurifySoul: //Purify Soul
			dedux = 10*GetAA(aaHastenedPurificationofSoul);
			spell_id = 2742;	//1473?
			timer_base = 1800;
			target = aaTargetCurrent;
			break;
			
		case aaExodus: //Exodus
			spell_id = 2771;
			dedux = 10*GetAA(aaHastenedExodus);
			timer_base = 4320;
			break;
			
		case aaDireCharm: //Dire Charm
			switch (GetClass())
			{
				case DRUID:
					spell_id = 2760; 	//2644?
					break;
				case NECROMANCER:
					spell_id = 2759;	//2643?
					break;
				case ENCHANTER:
					spell_id = 2761;	//2642?
					break;
			}
			timer_base = 4320;
			target = aaTargetCurrent;
			break;
			
		case aaCannibalization: //Canni V
			spell_id = 2749;
			timer_base = 180;
			break;
			
		case aaRabidBear: 
			spell_id = 2750;
			timer_base = 7200;
			dedux = 10*GetAA(aaHastenedRabidity);
			target = aaTargetCurrent;
			break;
			
		case aaManaBurn:
			spell_id = 2751;
			timer_base = 112;
			target = aaTargetCurrent;	//?
			break;
			
		case aaImprovedFamiliar:
			//Spell IDs might be wrong...
			if (GetAA(aaAllegiantFamiliar))
				spell_id = 3264;	//1994?
			else
				spell_id = 2758;	//2155?
			timer_base = 420;
			break;
			
		case aaNexusGate: //Nexus Gate
			spell_id = 2734;	//864?
			timer_base = 4320;	//2160?
			break;
//aaPermanentIllusion
			
		case aaGatherMana: //Gather Mana
			//not sure which is better:
			//SetMana(GetMaxMana());
			//or:
			spell_id = 2753;
			
			dedux = 10*GetAA(aaHastenedGathering);
			timer_base = 8640;	//4320?
			break;
			
		case aaMendCompanion: //Mend Companion
			dedux = 10*GetAA(aaHastenedMending);
			if (GetPet()) {
				spell_id = 2752;	//2654?
				timer_base = 2160;
			}
			break;
			
		case aaFrenziedBurnout: //Frenzied Burnout
			spell_id = 2754;
			timer_base = 4320;	//2160?
			break;
			
		case aaElementalFormFire: //Ele Form Fire
			spell_id = AA_Choose3(activate_val, 2795, 2796, 2797);
			timer_base = 900;
			break;
			
		case aaElementalFormWater: //Ele Form Water
			spell_id = AA_Choose3(activate_val, 2798, 2799, 2800);
			//MemorizeSpell(8,spell_id,3);
			timer_base = 900;
			break;
			
		case aaElementalFormEarth: //Ele Form Earth
			spell_id = AA_Choose3(activate_val, 2792, 2793, 2794);
			timer_base = 900;
			break;
			
		case aaElementalFormAir: //Ele Form Air
			spell_id = AA_Choose3(activate_val, 2789, 2790, 2791);
			timer_base = 900;
			break;
//aaImprovedReclaimEnergy
			
		case aaTurnSummoned: //Turn Summoned
			spell_id = AA_Choose3(activate_val, 2779, 2780, 2781);
			//On WR was: spell_id = AA_Choose3(activate_val, 2692, 2693, 2694);
			timer_base = 4320;	//2160?
			target = aaTargetCurrent;
			break;
			
		case aaLifeBurn: //Lifeburn
			spell_id = 2755;
			timer_base = 8640;	//4320?
			target = aaTargetCurrent;
			break;
			
		case aaDeadMesmerization: //Dead Mez
			spell_id = 2756;	//2706?
			timer_base = 4320;	//2160?
			target = aaTargetCurrent;
			break;
			
		case aaFearstorm: //Fearstorm
			spell_id = 2757;	//2707?
			timer_base = 4320;	//2160?
			target = aaTargetCurrent;	//?
			break;
			
		case aaFleshToBone:
			spell_id = 2772;	//3452?
			timer_base = 7;
			break;
			
		case aaCallToCorpse:
			spell_id = 2764;
			timer_base = 4320;	//2160?
			target = aaTargetCurrent;
			break;
			
		case aaDivineStun: //Divine Stun
			//is this really supposed to be spell based?
			spell_id = 2190;	//3284?
			timer_base = 30;
			target = aaTargetCurrent;
			dedux = GetAA(aaRushtoJudgement)*23;
			break;
//aaSlayUndead
			
		case aaActOfValor: //Act of Valor
			spell_id = 2775;
			timer_base = 4320;
			
			target = aaTargetCurrent;
			
			//does the spell do this for us?
			if(GetTarget() != NULL) {
				int heal = GetHP();
				int curhp = GetTarget()->GetHP();
				GetTarget()->SetHP(heal+curhp);
				Death(this,0,0,0);
			}
			
			break;
			
		case aaHolySteed: 
			spell_id = 2871;
			timer_base = 0;
			break;
			
		case aaInnateCamouflage: //Innate Camo
			spell_id = 2765;
			timer_base = 7;
			break;
			
		case aaUnholySteed: 
			spell_id = 2918;
			timer_base = 0;
			break;
			
		case aaImprovedHarmTouch:
			Message(0,"Sorry Improved harmtouch not working YET");
			timer_base = 4320;
			target = aaTargetCurrent;
			break;
			
		case aaLeechTouch: //Leech Touch
			spell_id = 2766;	//610?
			timer_base = 4320;
			target = aaTargetCurrent;
			break;
//aaDeathPeace
//aaSoulAbrasion
//aaJamFest
//aaSonicCall
//aaCriticalMend
			
		case aaPurifyBody: //Purify Body
			dedux = 10*GetAA(aaHastenedPurificationoftheBody);
			spell_id = 2190;	//1470?
			timer_base = 4320;
			break;
			
		case aaEscape: //Escape
			dedux = 10*(GetAA(aaHastyExit) + GetAA(aaImprovedHastyExit));
			timer_base = 4320;
			//I dont know which is better:
			//Escape();
			//or:
			spell_id = 5244;
			break;
			
		case aaPurgePoison:
			dedux = 10*GetAA(aaHastenedPurification);
			//m_pp.aa_active[0] = 1;	//WR method...
			spell_id = 5232;
			timer_base = 4320;
			break;
			
		case aaRampage:
			EnableAAEffect(aaEffectRampage, 1000);
			act_timer = 10000;
			timer_base = 600;
			dedux = 10*GetAA(aaFuriousRampage);
			//I dont think this is complete... attack needs to check aaEffectRampage
			//or something else needs to be done.
			if(GetTarget() == NULL) {
				Message(0, "A target is required for this skill.");
				return;
			}
			Attack(GetTarget(), 13, false);
			break;
			
		case aaAreaTaunt: //AE Taunt
//			entity_list.AETaunt(this);
			timer_base = 900;
			dedux = 10*GetAA(aaHastenedInstigation);
			break;
			
		case aaWarcry: //Warcry
			spell_id = AA_Choose3(activate_val, 5229, 5230, 5231);
			//On WR was: spell_id = AA_Choose3(activate_val, 1930, 1931, 1932);
			timer_base = 2160;
			target = aaTargetGroup;
			break;
			
		case aaStrongRoot:
			dedux = 10*GetAA(aaHastenedRoot);
			spell_id = 2748;
			timer_base = 2160;
			target = aaTargetCurrent;
			break;
			
		case aaHobbleofSpirits: //Hobble of Spirits
			spell_id = 3290;
			timer_base = 300;
			target = aaTargetPet;
			break;
			
		case aaFrenzyofSpirit: //Frenzy of Spirit
			spell_id = 3289;
			timer_base = 720;
			target = aaTargetPet;
			break;
			
		case aaParagonofSpirit: //Paragon of Spirit
			spell_id = 3291;
			timer_base = 900;
			target = aaTargetPet;
			break;
			
		case aaRadiantCure:
			spell_id = AA_Choose3(activate_val, 3297, 3298, 3299);
			//On WR was: spell_id = AA_Choose3(activate_val, 1982, 1983, 1984);
			dedux = 10*GetAA(aaQuickenedCuring);
			timer_base = 180;
			target = aaTargetCurrent;
			break;
			
		case aaDivineArbitration: //Divine Arbitration
			spell_id = AA_Choose3(activate_val, 3252, 3253, 3254);
			//On WR was: spell_id = AA_Choose3(activate_val, 2014, 2015, 2017);
			timer_base = 180;
			target = aaTargetCurrent;
			break;
			
		case aaWrathoftheWild:
			spell_id = AA_Choose3(activate_val, 3255, 3256, 3257);
			//On WR was: spell_id = AA_Choose3(activate_val, 2264, 2265, 2267);
			timer_base = 240;
			break;
			
		case aaVirulentParalysis:
			spell_id = AA_Choose3(activate_val, 3274, 3275, 3276);
			timer_base = 120;
			target = aaTargetCurrent;
			break;
			
		case aaHarvestofDruzzil:
			spell_id = 3338;	//2747?
			timer_base = 480;
			break;
			
		case aaEldritchRune: //Guarding Rune
			spell_id = AA_Choose3(activate_val, 3258, 3259, 3260);
			target = aaTargetCurrent;
			timer_base = 600;
			break;
			
		case aaDeathPeace:
			spell_id = 611;		//wrong
			timer_base = 4320;
			target = aaTargetCurrent;
			break;
		
		case aaServantofRo:
			spell_id = AA_Choose3(activate_val, 3265, 3266, 3267);
			timer_base = 540;
			break;
		
		case aaWaketheDead:
			spell_id = AA_Choose3(activate_val, 3268, 3269, 3270);
			timer_base = 540;
			break;
			
//I dunno about this one...
		case aaSuspendedMinion:	
			if (GetPet()) {
				
				target = aaTargetPet;
				//dunno if we need to cast a spell..
				switch (activate_val) {
					case 1:
						spell_id = 3248;
						break;
					case 2:
						spell_id = 3249;
						break;
				}
				
				Message(0,"You call your pet to your side.");
				GetPet()->WhipeHateList();
				GetPet()->GMMove(GetX(),GetY(),GetZ());
				if (activate_val > 1)
					entity_list.ClearFeignAggro(GetPet());
			} else {
				Message(0,"You have no pet to call.");
			}
			timer_base = 1;
			break;
		
		case aaSpiritCall:
			spell_id = AA_Choose3(activate_val, 3283, 3284, 3285);
			timer_base = 720;
			break;
			
		case aaHandofPiety: //Hand of Piety
			spell_id = AA_Choose3(activate_val, 3261, 3262, 3263);
			timer_base = 2160;
			dedux = 10*GetAA(aaHastenedPiety);
			target = aaTargetCurrentGroup;
			break;
			
		case aaGuardianoftheForest: //Guardian of the Forest
			spell_id = AA_Choose3(activate_val, 3271, 3272, 3273);
			timer_base = 900;	//2160?
			break;
			
		case aaSpiritoftheWood:
			spell_id = AA_Choose3(activate_val, 3277, 3278, 3279);
			timer_base = 1320;
			target = aaTargetCurrent;
			break;
			
		case aaBoastfulBellow:
			spell_id = 3282;
			timer_base = 18;
			target = aaTargetCurrent;
			break;
			
		case aaHostoftheElements:
			spell_id = AA_Choose3(activate_val, 3286, 3287, 3288);
			timer_base = 1320;
			break;
			
		case aaCallofXuzl:
			spell_id = AA_Choose3(activate_val, 3292, 3293, 3294);
			timer_base = 900;
			break;
		
		case aaFadingMemories:
			Message(0,"Sorry Fading Memories not working YET");
			use_mana = 900;
			spell_id = 5243;	//?
			//Escape();
			timer_base = 1;
			break;
			
		case aaProjectIllusion:
			Message(0,"Sorry, Project Illusion not working YET");
			timer_base = 1;
			break;
			
		case aaEntrap:
			spell_id = 3614;
			timer_base = 5;
			target = aaTargetCurrent;
			break;
			
		case aaManaBurn2:	//another mana burn?
			spell_id = 2751;
			timer_base = 8640;
			break;
			
		case aaBeastialAlignment:
			switch(GetBaseRace()) {
				case BARBARIAN:
					spell_id = AA_Choose3(activate_val, 4521, 4522, 4523);
					break;
				case TROLL:
					spell_id = AA_Choose3(activate_val, 4524, 4525, 4526);
					break;
				case OGRE:
					spell_id = AA_Choose3(activate_val, 4527, 4527, 4529);
					break;
				case IKSAR:
					spell_id = AA_Choose3(activate_val, 4530, 4531, 4532);
					break;
				case VAHSHIR:
					spell_id = AA_Choose3(activate_val, 4533, 4534, 4535);
					break;
			}
			timer_base = 4320;
			Message(0,"Sorry Bestial Alignment not working YET");
			break;
			
		case aaFeralSwipe:
			Message(0,"Sorry, Feral Swipe not working YET");
			spell_id = 4788;
			timer_base = 60;
			break;
			
		case aaDivineAvatar:
			spell_id = AA_Choose3(activate_val, 4549, 4550, 4551);
			Message(0,"Sorry, Divine Avatar not working YET");
			timer_base = 7200;
			break;
			
		case aaExquisiteBenediction:
			spell_id = AA_Choose5(activate_val, 4790, 4791, 4792, 4793, 4794);
			Message(0,"Sorry Equisite Benediction not working YET");
			timer_base = 1800;
			break;
			
		case aaNaturesBoon:
			spell_id = AA_Choose5(activate_val, 4796, 4797, 4798, 4799, 4800);
			Message(0,"Sorry Natures Boon not working YET");
			timer_base = 1800;
			break;
			
		case aaDoppelganger:
			spell_id = AA_Choose3(activate_val, 4552, 4553, 4554);
			Message(0,"Sorry Doppelganer not working YET");
			timer_base = 4320;
			break;
			
		case aaSharedHealth:
			Message(0,"Sorry Shared Health not working YET");
			timer_base = 900;
			target = aaTargetPet;
			break;
			
		case aaDestructiveForce:
			Message(0,"Sorry Destructive Force not working YET");  
			timer_base = 3600;
			target = aaTargetCurrent;
			break;
			
		case aaSwarmofDecay:
			spell_id = AA_Choose3(activate_val, 4564, 4565, 4566);
			Message(0,"Sorry Swarm of Decay not working YET");
			timer_base = 1800;
			break;
			
		case aaRadiantCure2:	//dont know whats different about this one...
			spell_id = AA_Choose3(activate_val, 3297, 3298, 3299);
			//was spell_id = AA_Choose3(activate_val, 1982, 1983, 1984);
			dedux = 10*GetAA(aaQuickenedCuring);
			timer_base = 180;
			target = aaTargetCurrent;
			break;
			
		case aaPurification:
			spell_id = 2742;	//wrong
			timer_base = 4320;
			Message(0,"Sorry, Purification not working, enjoy your self only purify soul");
			break;
			
		case aaFlamingArrows:
			timer_base = 4320;
			Message(0,"Sorry Flaming Arrows not working YET");
			break;
			
		case aaFrostArrows:
			timer_base = 4320;
			Message(0,"Sorry Frost Arrows not working YET");
			break;
			
		case aaCalloftheAncients:
			spell_id = AA_Choose5(activate_val, 4828, 4829, 4830, 4831, 4832);
			Message(0,"Sorry Call of the Ancients not working YET");
			timer_base = 1800;
			break;
			
		case aaWarlordsTenacity:
			spell_id = AA_Choose3(activate_val, 4925, 4926, 4927);
			timer_base = 3600;
			break;
			
		case aaRosFlamingFamiliar:
			spell_id = 4833;
			timer_base = 60;
			Message(0,"Sorry Ro's Flaming Familiar not working YET");
			break;
			
		case aaEcisIcyFamiliar:
			spell_id = 4834;
			timer_base = 60;
			Message(0,"Sorry Eci's Icy Familiar not working YET");
			break;
			
		case aaDruzzilsMysticalFamiliar:
			spell_id = 4835;
			timer_base = 60;
			Message(0,"Sorry Druzzil's Mystical Familiar not working YET");
			break;
			
		case aaWardofDestruction:
			spell_id = AA_Choose5(activate_val, 4836, 4837, 4838, 4839, 4840);
			timer_base = 1800;
			Message(0,"Sorry Ward of Destruction not working YET");
			break;
			
		case aaFrenziedDevistation:
			spell_id = AA_Choose3(activate_val, 5245, 5246, 5247);
			timer_base = 4320;
			Message(0,"Sorry Frenzied Devastation not working YET");
			break;
*/
