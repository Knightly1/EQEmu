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
	
	client_process.cpp:
	Handles client login sequence and packets sent from client to zone
*/
#include "../common/debug.h"
#include <iostream>
#include <iomanip>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <zlib.h>
#include <assert.h>

#ifdef WIN32
	#include <windows.h>
	#include <winsock.h>
	#define snprintf	_snprintf
	#define vsnprintf	_vsnprintf
	#define strncasecmp	_strnicmp
	#define strcasecmp  _stricmp
#else
	#include <pthread.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <unistd.h>
#endif

#include "masterentity.h"
#include "../common/database.h"
#include "../common/packet_functions.h"
#include "../common/packet_dump.h"
#include "worldserver.h"
#include "../common/packet_dump_file.h"
#include "../common/MiscFunctions.h"
#include "spdat.h"
#include "petitions.h"
#include "NpcAI.h"
#include "../common/skills.h"
#include "forage.h"
#include "zone.h"
#include "event_codes.h"
#include "faction.h"
#include "../common/crc32.h"
#include "StringIDs.h"
#include "map.h"
using namespace std;

#ifdef GUILDWARS
#include "../GuildWars/GuildWars.h"
extern GuildWars guildwars;
extern GuildLocationList location_list;
extern int32 numclients;
#endif

#ifdef RAIDADDICTS
#include "RaidAddicts.h"
extern RaidAddicts raidaddicts;
#endif

extern Database database;
extern Zone* zone;
extern volatile bool ZoneLoaded;
extern WorldServer worldserver;
extern GuildRanks_Struct guilds[512];
#ifndef NEW_LoadSPDat
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif
extern bool spells_loaded;
extern PetitionList petition_list;
extern EntityList entity_list;

bool Client::Process() {
	_ZP(Client_Process);
	adverrorinfo = 1;
	bool ret = true;
	//bool throughpacket = true;
	if (Connected() || IsLD())
	{
        // try to send all packets that weren't sent before
		if(!IsLD() && zoneinpacket_timer.Check()){
//			zoneinpacket_timer.Start(1000);		//to decrease latency for these packets... no idea on a good value
			SendAllPackets();
		}
		
		if(dead)
			SetHP(-100);
		if(dead && this->client_state == CLIENT_LINKDEAD) {
			LeaveGroup();
			return false;
		}
		if(hpupdate_timer.Check())
			SendHPUpdate();	
		if(mana_timer.Check())
			SendManaUpdatePacket();
		if(dead && dead_timer.Check()) {
			database.MoveCharacterToZone(GetName(),database.GetZoneName(m_pp.bind_zone_id));
			m_pp.zone_id = m_pp.bind_zone_id;
			m_pp.x = m_pp.bind_x[0];
			m_pp.y = m_pp.bind_y[0];
			m_pp.z = m_pp.bind_z[0];
			Save();
			
			Group *mygroup = GetGroup();
			if (mygroup)	// && zone.GetZoneID() != m_pp.bind_zone_id
			{
				entity_list.MessageGroup(this,true,15,"%s died.", GetName());
				mygroup->MemberZoned(this);
			}
			return(false);
		}
		if((p_timers.Get(pTimerAdventureTimer) && p_timers.Expired(pTimerAdventureTimer,false))){
			p_timers.Disable(pTimerAdventureTimer);
			SendAdventureFinish(0,0);
		}
		else if(p_timers.Get(pTimerStartAdventureTimer) && p_timers.Expired(pTimerStartAdventureTimer,false)){
			p_timers.Disable(pTimerStartAdventureTimer);
			SendAdventureFinish(0,0);
		}		
		if(linkdead_timer.Check()){
			Save();
			LeaveGroup();
			return false; //delete client
		}

		if (camp_timer.Check()) {
			instalog = true;
		}
		
		if (IsStunned() && stunned_timer.Check()) {
			this->stunned = false;
			this->stunned_timer.Disable();
		}
		
		if (fishing_timer.Check()) {
			GoFish();
		}
		
		if (bardsong_timer.Check() && bardsong != 0) {
			//WR: need to figure out how to tell if they are dead...
			if (!bardsong_target /*|| bardsong_target->dead*/) {
				StopSong();
			} else {
				SpellFinished(bardsong, bardsong_target->GetID(), bardsong_slot, spells[bardsong].mana);
			}
		}
		
		if(this->client_state == CLIENT_LINKDEAD)
			this->CastToMob()->AI_Process();
		/*if(opcodetimer->Check()){
			time_t rawtime;
			struct tm* gmt_t;
			time(&rawtime);
			gmt_t = gmtime(&rawtime);
			cout << "Opcode: " << opcode2 << " " << (gmt_t->tm_year + 1900) << "/" << setw(2) << setfill('0') << (gmt_t->tm_mon + 1) << "/" << setw(2) << setfill('0') << gmt_t->tm_mday << " " << setw(2) << setfill('0') << gmt_t->tm_hour << ":" << setw(2) << setfill('0') << gmt_t->tm_min << ":" << setw(2) << setfill('0') << gmt_t->tm_sec << " GMT\n";
            
            Client* client = entity_list.GetClientByName("Lethal");
            if(client){
                    //client->Message(0,"Trying Opcode: %i",opcode2);
					char blah2[20];
					sprintf(blah2,"%i",opcode2);
					char* blah=blah2;
					APPLAYER* app = new APPLAYER(opcode2);
					app->size = 4+strlen(blah)+1;
					app->pBuffer = new uchar[app->size];
					SpecialMesg_Struct* sm=(SpecialMesg_Struct*)app->pBuffer;
					sm->msg_type = 0;
					client->Message(0,"%i (dec) sent now!",opcode2);
					strcpy(sm->message, blah);
					//if(opcode2<1000){
						app->Deflate();
						app->opcode |= FLAG_COMPRESSED;
					//}
					QueuePacket(app);
					delete app;
                    opcode2++;
            }
            else{
                    opcodetimer->Disable();
                    cout << "Stopped at: " << opcode2 << endl;
            }
            if(opcode2>=0xFFFF)
                    opcodetimer->Disable();
        }*/
		if (bindwound_timer.Check() && bindwound_target != 0) {
		    BindWound(bindwound_target, false);
		}
		if (auto_attack && !IsAIControlled() && !(spellend_timer.Enabled() && (spells[casting_spell_id].classes[7] < 1 && spells[casting_spell_id].classes[7] > 65)) && target != 0 && attack_timer.Check() && !IsStunned() && !IsMezzed() && dead == 0) {
			if (!CombatRange(target)) {
				//Message(0,"Target's Name: %s",target->GetName());
				//Message(0,"Target's X: %f, Your X: %f",target->CastToMob()->GetX(),GetX());
				//Message(0,"Target's Y: %f, Your Y: %f",target->CastToMob()->GetY(),GetY());
				//Message(0,"Target's Z: %f, Your Z: %f",target->CastToMob()->GetZ(),GetZ());
				Message_StringID(13,TARGET_TOO_FAR);
				//Message(13,"Your target is too far away, get closer!");
			}
			else if (target == this) {
				Message_StringID(13,TRY_ATTACKING_SOMEONE);
				//Message(13,"Try attacking someone else then yourself!");
			}
			/*
			else if (CantSee(target)) {
				Message(13,"You can't see your target from here.");
			}*/
			else if (!IsNPC() && appearance == 3) {
			}
			else if (target->GetHP() > -10) { // -10 so we can watch people bleed in PvP
				if(CheckAAEffect(aaEffectRampage)){	//Dook- AA Destructive Force- AE attacks for duration
					entity_list.AEAttack(this, 30);
				} else {
					Attack(target, 13); 	// Kaiyodo - added attacking hand to arguments
				}
				// Kaiyodo - support for double attack. Chance based on formula from Monkly business
				if( target && CanThisClassDoubleAttack() ) {
					
					if(CheckDoubleAttack(true)) {
						//should we allow rampage on double attack?
						if(CheckAAEffect(aaEffectRampage)) {
							entity_list.AEAttack(this, 30);
						} else {
							Attack(target, 13, true);
						}
					}
					
					//triple attack: warriors and monks over level 60
					if((((GetClass() == WARRIOR || GetClass() == MONK) && GetLevel() >= 60) 
						|| SpecAttacks[SPECATK_TRIPLE])
					   && CheckDoubleAttack(false,true))
					{
						Attack(target, 13, true);
					}
					
					//quad attack, does this belong here??
					if(SpecAttacks[SPECATK_QUAD] && CheckDoubleAttack(false,true))
					{
						Attack(target, 13, true);
					}
				}
				if (target && GetAA(aaFlurry) > 0) {
					int flurrychance = 0;
					switch (GetAA(aaFlurry)) {
						case 1:
							flurrychance += 15;
							break;
						case 2:
							flurrychance += 30;
							break;
						case 3:
							flurrychance += 50;
							break;
					}
					switch (GetAA(183)) {
						case 1:
							flurrychance += 10;
							break;
						case 2:
							flurrychance += 20;
							break;
						case 3:
							flurrychance += 30;
							break;
					}
					if (rand()%1000 < flurrychance) {
						Message_StringID(MT_CritMelee, 128);
						Attack(target, 13, true);
						
						//50% chance for yet another attack?
						if(MakeRandomFloat(0, 1) < 0.5)
							Attack(target, 13, true);
					}
				}
			}
		}
		if (GetClass() == WARRIOR && dead == 0 && !this->berserk && this->GetHPRatio() < 30) {
//			char temp[100];
//			snprintf(temp, 100, "%s goes into a berserker frenzy!", this->GetName());
//			entity_list.MessageClose(this, 0, 200, 10, temp);
			entity_list.MessageClose_StringID(this, false, 200, 0, BERSERK_START, GetName());
			this->berserk = true;
		}
		if (GetClass() == WARRIOR && this->berserk && this->GetHPRatio() > 30) {
//			char temp[100];
//			snprintf(temp, 100, "%s is no longer berserk.", this->GetName());
//			entity_list.MessageClose(this, 0, 200, 10, temp);
			entity_list.MessageClose_StringID(this, false, 200, 0, BERSERK_END, GetName());
			this->berserk = false;
		}
		// Kaiyodo - Check offhand attack timer
		if(auto_attack && !IsAIControlled() && CanThisClassDualWield() && target != 0 && attack_dw_timer.Check()&& !IsStunned() && !IsMezzed() && dead == 0) {
		
			attack_dw_timer.Start(0);
			// Range check
			if(!CombatRange(target)) {
				//Message(13,"Your target is too far away, get closer! (dual)");
				Message_StringID(13,TARGET_TOO_FAR);
			}
			// Don't attack yourself
			else if(target == this) {
				//Message(13,"Try attacking someone else then yourself! (dual)");
				Message_StringID(13,TRY_ATTACKING_SOMEONE);
			}
			else if (!IsNPC() && appearance == 3) {// Mezzed? Stunned?
        	}
			else if(target->GetHP() > -10) {
				float DualWieldProbability = (GetSkill(DUAL_WIELD) + GetLevel()) / 400.0f; // 78.0 max
				if(GetAA(aaAmbidexterity))
					DualWieldProbability += 0.1f;
				//discipline effects:
				DualWieldProbability += (spellbonuses.DualWeildChance + itembonuses.DualWeildChance) / 100.0f;
				
				float random = MakeRandomFloat(0, 1);
				//if (random > 0.9)	//this dosent make sense...
					CheckIncreaseSkill(DUAL_WIELD);
				if (random < DualWieldProbability  || GetAA(aaAmbidexterity)) { // Max 78% of DW
					if(CheckAAEffect(aaEffectRampage)) {
						entity_list.AEAttack(this, 30, 14);
					} else {
						Attack(target, 14);	// Single attack with offhand
					}
					CheckIncreaseSkill(DUAL_WIELD);
					
					if( CanThisClassDoubleAttack() && CheckDoubleAttack()) {
						if(CheckAAEffect(aaEffectRampage)) {
							entity_list.AEAttack(this, 30, 14);
						} else {
							if(target && target->GetHP() > -10)
								Attack(target, 14);	// Single attack with offhand
						}
					}
				}
				if (target && GetAA(aaFlurry) > 0) {
					int flurrychance = 0;
					switch (GetAA(aaFlurry)) {
						case 1:
							flurrychance += 15;
							break;
						case 2:
							flurrychance += 30;
							break;
						case 3:
							flurrychance += 50;
							break;
					}
					switch (GetAA(183)) {
						case 1:
							flurrychance += 10;
							break;
						case 2:
							flurrychance += 20;
							break;
						case 3:
							flurrychance += 30;
							break;
					}
					if (rand()%1000 < flurrychance) {
						Message_StringID(MT_CritMelee, 128);
						Attack(target, 13, true);
						
						//50% chance for yet another attack?
						if(MakeRandomFloat(0, 1) < 0.5)
							Attack(target, 13, true);
					}
				}
			}
		}
		if (disc_timer.Check()) {
			disc_timer.Disable();
			//Message(0, "Your disciplines are available for use!");
			Message_StringID(0,DISCIPLINE_RDY);
		}
		else if (disc_elapse.Check()) {
			disc_elapse.Disable();
			disc_inuse = discNone;
			//Message(0, "You lose your concentration!");
			Message_StringID(0,DISCIPLINE_CONLOST);
		}
		
		adverrorinfo = 2;
		if (position_timer.Check()) {
			if (IsAIControlled())
				SendPosUpdate(2);
			
			// Send a position packet every 8 seconds - if not done, other clients
			// see this char disappear after 10-12 seconds of inactivity
			if (position_timer_counter >= 36) { // Approx. 4 ticks per second
				entity_list.SendPositionUpdates(this, pLastUpdateWZ, 500, target, true);
				pLastUpdate = Timer::GetCurrentTime();
				pLastUpdateWZ = pLastUpdate;
				position_timer_counter = 0;
			}
			else {
				pLastUpdate = Timer::GetCurrentTime();
				position_timer_counter++;
			}
		}
		
		if (shield_timer.Check())
		{
			if (shield_target)
			{
				if (!CombatRange(shield_target))
				{
					entity_list.MessageClose(this,false,100,0,"%s ceases shielding %s.",GetName(),shield_target->GetName());
					for (int y = 0; y < 2; y++)
					{
						if (shield_target->shielder[y].shielder_id == GetID())
						{
							shield_target->shielder[y].shielder_id = 0;
							shield_target->shielder[y].shielder_bonus = 0;
						}
					}
					shield_target = 0;
					shield_timer.Disable();
				}
			}
			else
			{
				shield_target = 0;
				shield_timer.Disable();
			}
		}
		
		adverrorinfo = 3;
		SpellProcess();
		adverrorinfo = 4;
		if (tic_timer.Check() && !dead) {
			CalcMaxHP();
			CalcMaxMana();
			DoHPRegen();
			DoManaRegen();
			TicProcess();
			
			if(stamina_timer.Check()){
				APPLAYER* outapp = new APPLAYER(OP_Stamina, sizeof(Stamina_Struct));
				Stamina_Struct* sta = (Stamina_Struct*)outapp->pBuffer;
				if (m_pp.hunger_level > 0)
					m_pp.hunger_level-=32;
				if (m_pp.thirst_level > 0)
					m_pp.thirst_level-=32;
				sta->food = m_pp.hunger_level;
				sta->water = m_pp.thirst_level;
				QueuePacket(outapp);
				safe_delete(outapp);
			}
		}
	}
    
	
	
	if (client_state == CLIENT_KICKED) {
		if(GetAdventureID()>0)DeleteCharInAdventure(CharacterID(),GetAdventureID());
		LeaveGroup();
		Save();
		eqnc->Close();
		cout << "Client disconnected (cs=k): " << GetName() << endl;
		return false;
	}
	
	if (client_state == DISCONNECTED) {
		if(GetAdventureID()>0)DeleteCharInAdventure(CharacterID(),GetAdventureID());
		LeaveGroup();
		eqnc->Close();
		cout << "Client disconnected (cs=d): " << GetName() << endl;
		return false;
	}
	
	if (client_state == CLIENT_ERROR) {
		LeaveGroup();
		eqnc->Close();
		cout << "Client disconnected (cs=e): " << GetName() << endl;
		return false;
	}
	
	if (client_state != CLIENT_LINKDEAD && !eqnc->CheckActive()) {
		LeaveGroup();
		cout << "Client linkdead: " << name << endl;
		eqnc->Close();

		if (GetGM()) {
			return false;
		}
		else if(!linkdead_timer.Enabled()){
			linkdead_timer.Start(30000);
			client_state = CLIENT_LINKDEAD;
			AI_Start(CLIENT_LD_TIMEOUT);
			SendAppearancePacket(AT_Linkdead, 1);
		}
	}
	/************ Get all packets from packet manager out queue and process them ************/
	adverrorinfo = 5;
	if((int32)eqnc == 0xFEEEFEEE){
		if(GetAdventureID()>0)DeleteCharInAdventure(CharacterID(),GetAdventureID());
		LeaveGroup();
		eqnc->Close();
		safe_delete(eqnc);
		return false;
	}
	
	APPLAYER *app = 0;
	if(eqnc->GetState()>=EQNC_Closing && eqnc->CheckActive()){
		//eqnc->Close();
		//return false;
		//handled below 
	} else {
		while(ret && (app = eqnc->PopPacket())) {
			if(app)
				ret = HandlePacket(app);
			safe_delete(app);
		}
	}
	
#ifdef REVERSE_AGGRO
	//At this point, we are still connected, everything important has taken
	//place, now check to see if anybody wants to aggro us.
	if(scanarea_timer.Check()) {
		entity_list.CheckClientAggro(this);
	}
#endif	
	
	if (client_state != CLIENT_LINKDEAD && (client_state == CLIENT_ERROR || client_state == DISCONNECTED || client_state == CLIENT_KICKED || !eqnc->CheckActive())) {
		if (!zoning) {
			RemoveNoRent(); //Get rid of ze no rent stuff if logging out
		}
		ResetTrade();
		if (client_state != CLIENT_KICKED) {
			Save();
		}
		adverrorinfo = 811;
		client_state = CLIENT_LINKDEAD;
		if (/*!loggedin || */zoning || instalog || GetGM())
		{
			adverrorinfo = 811;
			Group *mygroup = GetGroup();
			if (mygroup)
			{
				adverrorinfo = 812;
				if (!zoning) {
					entity_list.MessageGroup(this,true,15,"%s logged out.",GetName());
					mygroup->DelMember(this);
				} else {
					entity_list.MessageGroup(this,true,15,"%s left the zone.",GetName());
					mygroup->MemberZoned(this);
				}
				
				adverrorinfo = 813;
			}
			eqnc->Close();
			return false;
		}
		else
		{
			adverrorinfo = 814;
			LinkDead();
			LeaveGroup();
		}
		eqnc->Close();
	}
	
	
	return ret;
}

// Sends the client complete inventory used in character login
//#ifdef ITEMCOMBINED
void Client::BulkSendInventoryItems()
{
	// Search all inventory buckets for items
	bool deletenorent=database.NoRentExpired(GetName());
	// Worn items and Inventory items
	sint16 slot_id = 0;
	if(deletenorent){//client was offline for more than 30 minutes, delete no rent items
		RemoveNoRent();
	}
	
	//TODO: this function is just retarded... it re-allocates the buffer for every
	//new item. It should be changed to loop through once, gather the
	//lengths, and item packet pointers into an array (fixed length), and
	//then loop again to build the packet.
	//APPLAYER *packets[50];
	//unsigned long buflen = 0;
	//unsigned long pos = 0;
	//memset(packets, 0, sizeof(packets));
	//foreach item in the invendor sections
	//	packets[pos++] = ReturnItemPacket(...)
	//	buflen += temp->size
	//...
	//allocat the buffer
	//for r from 0 to pos
	//	put pos[r]->pBuffer into the buffer
	//for r from 0 to pos
	//	safe_delete(pos[r]);
	int32 size=0;
	int16 i = 0;
	map<int16,string> ser_items;
	map<int16,string>::iterator itr;
	//Inventory items
	for (slot_id=0; slot_id<=30; slot_id++) {
		const ItemInst* inst = m_inv[slot_id];
		if (inst){
			string packet = inst->Serialize(slot_id);
			ser_items[i++] = packet;
			size+=packet.length() + 1;
		}
	}
	// Bank items
	for (slot_id=2000; slot_id<=2015; slot_id++) {
		const ItemInst* inst = m_inv[slot_id];
		if (inst){
			string packet = inst->Serialize(slot_id);
			ser_items[i++] = packet;
			size+=packet.length() + 1;
		}
	}
	// Shared Bank items
	for (slot_id=2500; slot_id<=2501; slot_id++) {
		const ItemInst* inst = m_inv[slot_id];
		if (inst){
			string packet = inst->Serialize(slot_id);
			ser_items[i++] = packet;
			size+=packet.length() + 1;
		}
	}
	APPLAYER* outapp = new APPLAYER(OP_CharInventory,size);
	uchar* ptr = outapp->pBuffer;
	for(itr=ser_items.begin();itr!=ser_items.end();itr++){
		int length = itr->second.length();
		if(length>5){
			memcpy(ptr,itr->second.c_str(),length);
			ptr+=length+1;
		}
	}
	//DumpPacket(outapp);
	outapp->Deflate();
	QueuePacket(outapp);
	safe_delete(outapp);
	// LINKDEAD TRADE ITEMS
	// If player went LD during a trade, they have items in the trade inventory
	// slots.  These items are now being put into their inventory (then queue up on cursor)
	for (sint16 trade_slot_id=3000; trade_slot_id<=3007; trade_slot_id++) {
		const ItemInst* inst = m_inv[slot_id];
		if (inst) {
			sint16 free_slot_id = m_inv.FindFreeSlot(inst->IsType(ItemTypeContainer), true, inst->GetItem()->Size);
			DeleteItemInInventory(trade_slot_id, 0, false);
			PutItemInInventory(free_slot_id, *inst, true);
		}
	}
}
/*#else
void Client::BulkSendInventoryItems()
{
	// Search all inventory buckets for items
	bool deletenorent=database.NoRentExpired(GetName());
	// Worn items and Inventory items
	sint16 slot_id = 0;
	if(deletenorent){//client was offline for more than 30 minutes, delete no rent items
		RemoveNoRent();
	}
	for (slot_id=0; slot_id<=30; slot_id++) {
		const ItemInst* inst = m_inv[slot_id];
		if (inst){
			SendItemPacket(slot_id, inst, ItemPacketCharInventory);
		}
	}
	// Bank items
	for (slot_id=2000; slot_id<=2015; slot_id++) {
		const ItemInst* inst = m_inv[slot_id];
		if (inst){
			SendItemPacket(slot_id, inst, ItemPacketCharInventory);
		}
	}
	
	// Shared Bank items
	for (slot_id=2500; slot_id<=2501; slot_id++) {
		const ItemInst* inst = m_inv[slot_id];
		if (inst){
			SendItemPacket(slot_id, inst, ItemPacketCharInventory);
		}
	}
	
	// LINKDEAD TRADE ITEMS
	// If player went LD during a trade, they have items in the trade inventory
	// slots.  These items are now being put into their inventory (then queue up on cursor)
	for (sint16 trade_slot_id=3000; trade_slot_id<=3007; trade_slot_id++) {
		const ItemInst* inst = m_inv[slot_id];
		if (inst) {
			sint16 free_slot_id = m_inv.FindFreeSlot(inst->IsType(ItemTypeContainer), true, inst->GetItem()->Size);
			DeleteItemInInventory(trade_slot_id, 0, false);
			PutItemInInventory(free_slot_id, *inst, true);
		}
	}
}
#endif*/
// Send an item packet (including all subitems of the item)
void Client::SendItemPacket(sint16 slot_id, const ItemInst* inst, ItemPacketType packet_type)
{
	if (!inst)
		return;
	
	// Serialize item into |-delimited string
	string packet = inst->Serialize(slot_id);
	
	uint16 opcode = 0;
	APPLAYER* outapp = NULL;
	ItemPacket_Struct* itempacket = NULL;
	
	// Construct packet
	opcode = (packet_type==ItemPacketViewLink) ? OP_ItemLinkResponse : OP_ItemPacket;
	outapp = new APPLAYER(opcode, packet.length()+5);
	itempacket = (ItemPacket_Struct*)outapp->pBuffer;
	memcpy(itempacket->SerializedItem, packet.c_str(), packet.length());
	itempacket->PacketType = packet_type;
	
#if EQDEBUG >= 9
		DumpPacket(outapp);
#endif
	//outapp->Deflate();
		if(slot_id >= 22)
			outapp->priority = 6;
	//DumpPacket(outapp);
	QueuePacket(outapp);
	safe_delete(outapp);
}

APPLAYER* Client::ReturnItemPacket(sint16 slot_id, const ItemInst* inst, ItemPacketType packet_type)
{
	if (!inst)
		return 0;
	
	// Serialize item into |-delimited string
	string packet = inst->Serialize(slot_id);
	
	uint16 opcode = 0;
	APPLAYER* outapp = NULL;
	BulkItemPacket_Struct* itempacket = NULL;
	
	// Construct packet
	opcode = OP_ItemPacket;
	outapp = new APPLAYER(opcode, packet.length()+1);
	itempacket = (BulkItemPacket_Struct*)outapp->pBuffer;
	memcpy(itempacket->SerializedItem, packet.c_str(), packet.length());

#if EQDEBUG >= 9
		DumpPacket(outapp);
#endif

	return outapp;
}

void Client::RemoveData() {
	eqnc->RemoveData();
}

void Client::BulkSendMerchantInventory(int merchant_id, int16 npcid) {
	const Item_Struct* handyitem = NULL;
	int32 numItemSlots=80;  //The max number of items passed in the transaction.
	const Item_Struct *item;
	std::list<MerchantList> merlist = zone->merchanttable[merchant_id];
	std::list<MerchantList>::const_iterator itr;
	if(merlist.size()==0){ //Attempt to load the data, it might have been missed if someone spawned the merchant after the zone was loaded
		zone->LoadNewMerchantData(merchant_id);
		merlist = zone->merchanttable[merchant_id];
		if(merlist.size()==0)
			return;
	}
	std::list<TempMerchantList> tmp_merlist = zone->tmpmerchanttable[npcid];
	std::list<TempMerchantList>::iterator tmp_itr;

	int i=1;
	int8 handychance = 0;
	for(itr = merlist.begin();itr != merlist.end() && i<numItemSlots;itr++){
		MerchantList ml = *itr;
		handychance = MakeRandomInt(0, merlist.size() + tmp_merlist.size() - 1 );
		
		item=database.GetItem(ml.item);
		if (item) {
			if(handychance==0)
				handyitem=item;
			else
				handychance--;
			int charges=1;
			if(item->ItemClass==ItemTypeCommon)
				charges=item->Common.MaxCharges;
			ItemInst* inst = ItemInst::Create(item,charges);
			if (inst) {
				inst->SetPrice(item->Cost*127/100);
				inst->SetMerchantSlot(ml.slot);
				if(charges > 0)
					inst->SetCharges(charges);
				else
					inst->SetCharges(1);
				SendItemPacket(ml.slot-1, inst, ItemPacketMerchant);
				safe_delete(inst);
			}
		}
		i++;
	}
	std::list<TempMerchantList> origtmp_merlist = zone->tmpmerchanttable[npcid];
	tmp_merlist.clear();
	for(tmp_itr = origtmp_merlist.begin();tmp_itr != origtmp_merlist.end() && i<numItemSlots;tmp_itr++){
		TempMerchantList ml = *tmp_itr;
		item=database.GetItem(ml.item);
		ml.slot=i;
		if (item) {
			if(handychance==0)
				handyitem=item;
			else
				handychance--;
			int charges=1;
			if(item->ItemClass==ItemTypeCommon && ml.charges <= item->Common.MaxCharges)
				charges=ml.charges;
			else
				charges = item->Common.MaxCharges;
			ItemInst* inst = ItemInst::Create(item,charges);
			if (inst) {
				inst->SetPrice(item->Cost*127/100);
				inst->SetMerchantSlot(ml.slot);
				if(charges > 0)
					inst->SetCharges(charges);
				else
					inst->SetCharges(1);
				SendItemPacket(ml.slot-1, inst, ItemPacketMerchant);
				safe_delete(inst);
			}
		}
		tmp_merlist.push_back(ml);
		i++;
	}
	//this resets the slot
	zone->tmpmerchanttable[npcid] = tmp_merlist;
	Mob* merch = entity_list.GetMobByNpcTypeID(npcid);
	if(merch != NULL && handyitem){
		char handy_id[8]={0};
		int greeting=rand()%5;
		int greet_id=0;
		switch(greeting){
			case 1:
				greet_id=MERCHANT_GREETING;
				break;
			case 2:
				greet_id=MERCHANT_HANDY_ITEM1;
				break;
			case 3:
				greet_id=MERCHANT_HANDY_ITEM2;
				break;
			case 4:
				greet_id=MERCHANT_HANDY_ITEM3;
				break;
			default:
				greet_id=MERCHANT_HANDY_ITEM4;
        }
		sprintf(handy_id,"%i",greet_id);
		char merchantname[64]={0};
		strncpy(merchantname,merch->GetName(),strlen(merch->GetName())-2);
		if(greet_id!=MERCHANT_GREETING){
			Message_StringID(10,GENERIC_STRINGID_SAY,merchantname,handy_id,this->GetName(),handyitem->Name);
		
        }
        else
			Message_StringID(10,GENERIC_STRINGID_SAY,merchantname,handy_id,this->GetName());
		
		merch->CastToNPC()->FaceTarget(this->CastToMob());
        }
		
//		safe_delete_array(cpi);
}

int8 Client::WithCustomer(){
	if(this->withcustomer)
		return 0;
	else{
		this->withcustomer=true;
		return 1;
	}
}
void Client::OPRezzAnswer(const APPLAYER* app) {
	if (!pendingrezzexp)
		return;
	const Resurrect_Struct* ra = (const Resurrect_Struct*) app->pBuffer;
	if (ra->action == 1){
		cout << "Player " << this->name << " got a " << (int16)spells[ra->spellid].base[0] << "% Rezz" << endl;
		this->BuffFadeAll();
		SetMana(0);
		SetHP(GetMaxHP()/5);
		APPLAYER* outapp = app->Copy();
		outapp->opcode = OP_RezzComplete;
		worldserver.RezzPlayer(outapp,0,OP_RezzComplete);
		cout << "pe: " << pendingrezzexp << endl;
		SetEXP(((int)(GetEXP()+((float)((pendingrezzexp/100)*spells[ra->spellid].base[0])))),GetAAXP(),true);
		pendingrezzexp = 0;
		if (strcmp(ra->zone,zone->GetShortName()) != 0){
			SetZoneSummonCoords(ra->x,ra->y,ra->z);
		}
		this->FastQueuePacket(&outapp);
	}
}

void Client::OPTGB(const APPLAYER *app)
{
	if(!app) return;
	if(!app->pBuffer) return;

	int32 tgb_flag = *(int32 *)app->pBuffer;
	if(tgb_flag == 2)
		Message_StringID(0, TGB() ? TGB_ON : TGB_OFF);
	else
		tgb = tgb_flag;
}

void Client::OPMemorizeSpell(const APPLAYER* app)
{
	if(app->size != sizeof(MemorizeSpell_Struct))
	{
		LogFile->write(EQEMuLog::Error,"Wrong size on OP_MemorizeSpell. Got: %i, Expected: %i", app->size, sizeof(MemorizeSpell_Struct));
		DumpPacket(app);
		return;
	}
	
	const MemorizeSpell_Struct* memspell = (const MemorizeSpell_Struct*) app->pBuffer;
	
	if(!IsValidSpell(memspell->spell_id))
	{
		Message(13, "Unexpected error: spell id out of range");
		return;
	}

	if
	(
		GetClass() > 16 ||
		GetLevel() < spells[memspell->spell_id].classes[GetClass()-1]
	)
	{
		char val1[20]={0};
		Message_StringID(13,SPELL_LEVEL_TO_LOW,ConvertArray(spells[memspell->spell_id].classes[GetClass()-1],val1),spells[memspell->spell_id].name);
		//Message(13, "Unexpected error: Class cant use this spell at your level!");
		return;
	}

	switch(memspell->scribing)
	{
		case memSpellScribing:	{	// scribing spell to book
			ItemInst* inst = m_inv.PopItem(SLOT_CURSOR);
			
			if(inst && inst->IsType(ItemTypeCommon))
			{
				const Item_Struct* item = inst->GetItem();
				
				if(item && item->Common.SpellId == (sint32)(memspell->spell_id))
				{
					ScribeSpell(memspell->spell_id, memspell->slot);

					// Destroy scroll on cursor
					APPLAYER* outapp = new APPLAYER(OP_MoveItem, sizeof(MoveItem_Struct));
					MoveItem_Struct* spellmoveitem = (MoveItem_Struct*) outapp->pBuffer;
					spellmoveitem->from_slot = SLOT_CURSOR;
					spellmoveitem->to_slot = SLOT_INVALID;
					spellmoveitem->number_in_stack = 0;
					QueuePacket(outapp);
					safe_delete(outapp);

					DeleteItemInInventory(SLOT_CURSOR);
					
				}
				else 
					Message(0,"Scribing spell: inst exists but item does not or spell ids do not match.");
			}
			else
				Message(0,"Scribing a spell without an inst on your cursor?");
			break;

			}
		case memSpellMemorize:	{	// memming spell
			MemSpell(memspell->spell_id, memspell->slot);
			break;
		}
		case memSpellForget:	{	// unmemming spell
			UnmemSpell(memspell->slot);
			break;
		}
	}

	Save();
}

void Client::BreakInvis()
{
	if (invisible)
	{
		APPLAYER* outapp = new APPLAYER(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
		SpawnAppearance_Struct* sa_out = (SpawnAppearance_Struct*)outapp->pBuffer;
		sa_out->spawn_id = GetID();
		sa_out->type = 0x03;
		sa_out->parameter = 0;
		entity_list.QueueClients(this, outapp, true);
		safe_delete(outapp);
		invisible = false;
	}
}

void Client::OPMoveCoin(const APPLAYER* app)
{
	MoveCoin_Struct* mc = (MoveCoin_Struct*)app->pBuffer;
	int value = 0, amount_to_take = 0, amount_to_add = 0;
	sint32 *from_bucket = 0, *to_bucket = 0;
	Mob* trader = trade->With();
	
	//DumpPacket(app);

	// could just do a range, but this is clearer and explicit
	if
	(
		(
			mc->cointype1 != COINTYPE_PP &&
			mc->cointype1 != COINTYPE_GP &&
			mc->cointype1 != COINTYPE_SP &&
			mc->cointype1 != COINTYPE_CP
		) ||
		(
			mc->cointype2 != COINTYPE_PP &&
			mc->cointype2 != COINTYPE_GP &&
			mc->cointype2 != COINTYPE_SP &&
			mc->cointype2 != COINTYPE_CP
		)
	)
	{
		return;
	}

	switch(mc->from_slot)
	{
		case -1:	// destroy
		{
			// solar: I don't think you can move coin from the void, 
			// but need to check this
			break;
		}
		case 0:	// cursor
		{
			switch(mc->cointype1)
			{
				case COINTYPE_PP:
					from_bucket = &m_pp.platinum_cursor; break;
				case COINTYPE_GP:
					from_bucket = &m_pp.gold_cursor; break;
				case COINTYPE_SP:
					from_bucket = &m_pp.silver_cursor; break;
				case COINTYPE_CP:
					from_bucket = &m_pp.copper_cursor; break;
			}
			break;
		}
		case 1:	// inventory
		{
			switch(mc->cointype1)
			{
				case COINTYPE_PP:
					from_bucket = &m_pp.platinum; break;
				case COINTYPE_GP:
					from_bucket = &m_pp.gold; break;
				case COINTYPE_SP:
					from_bucket = &m_pp.silver; break;
				case COINTYPE_CP:
					from_bucket = &m_pp.copper; break;
			}
			break;
		}
		case 2:	// bank
		{
			switch(mc->cointype1)
			{
				case COINTYPE_PP:
					from_bucket = &m_pp.platinum_bank; break;
				case COINTYPE_GP:
					from_bucket = &m_pp.gold_bank; break;
				case COINTYPE_SP:
					from_bucket = &m_pp.silver_bank; break;
				case COINTYPE_CP:
					from_bucket = &m_pp.copper_bank; break;
			}
			break;
		}
		case 3:	// trade
		{
			// can't move coin from trade
			break;
		}
		case 4:	// shared bank
		{
			if(mc->cointype1 == COINTYPE_PP)	// there's only platinum here
				from_bucket = &m_pp.platinum_shared;
			break;
		}
	}

	switch(mc->to_slot)
	{
		case -1:	// destroy
		{
			// no action required
			break;
		}
		case 0:	// cursor
		{
			switch(mc->cointype2)
			{
				case COINTYPE_PP:
					to_bucket = &m_pp.platinum_cursor; break;
				case COINTYPE_GP:
					to_bucket = &m_pp.gold_cursor; break;
				case COINTYPE_SP:
					to_bucket = &m_pp.silver_cursor; break;
				case COINTYPE_CP:
					to_bucket = &m_pp.copper_cursor; break;
			}
			break;
		}
		case 1:	// inventory
		{
			switch(mc->cointype2)
			{
				case COINTYPE_PP:
					to_bucket = &m_pp.platinum; break;
				case COINTYPE_GP:
					to_bucket = &m_pp.gold; break;
				case COINTYPE_SP:
					to_bucket = &m_pp.silver; break;
				case COINTYPE_CP:
					to_bucket = &m_pp.copper; break;
			}
			break;
		}
		case 2:	// bank
		{
			switch(mc->cointype2)
			{
				case COINTYPE_PP:
					to_bucket = &m_pp.platinum_bank; break;
				case COINTYPE_GP:
					to_bucket = &m_pp.gold_bank; break;
				case COINTYPE_SP:
					to_bucket = &m_pp.silver_bank; break;
				case COINTYPE_CP:
					to_bucket = &m_pp.copper_bank; break;
			}
			break;
		}
		case 3:	// trade
		{
			if(trader)
			{
				switch(mc->cointype2)
				{
					case COINTYPE_PP:
						to_bucket = &trade->pp; break;
					case COINTYPE_GP:
						to_bucket = &trade->gp; break;
					case COINTYPE_SP:
						to_bucket = &trade->sp; break;
					case COINTYPE_CP:
						to_bucket = &trade->cp; break;
				}
			}
			break;
		}
		case 4:	// shared bank
		{
			if(mc->cointype2 == COINTYPE_PP)	// there's only platinum here
				to_bucket = &m_pp.platinum_shared;
			break;
		}
	}

	if(!from_bucket)
	{
		return;
	}

	// don't allow them to go into negatives (from our point of view)
	amount_to_take = *from_bucket < mc->amount ? *from_bucket : mc->amount;

	// solar: if you move 11 gold into a bank platinum location, the packet
	// will say 11, but the client will have 1 left on their cursor, so we have
	// to figure out the conversion ourselves

	value = amount_to_take * (int)pow(10.0, mc->cointype1);
	amount_to_add = value / (int)pow(10.0, mc->cointype2);

	// the amount we're adding could be different than what was requested, so
	// we have to adjust the amount we take as well
	value = amount_to_add * (int)pow(10.0, mc->cointype2);
	amount_to_take = value / (int)pow(10.0, mc->cointype1);

	// solar: now we should have a from_bucket, a to_bucket, an amount_to_take
	// and an amount_to_add

#ifdef SOLAR
	printf("taking %d coins, adding %d coins\n", amount_to_take, amount_to_add);
#endif

	// solar: now we actually take it from the from bucket.  if there's an error
	// with the destination slot, they lose their money
	*from_bucket -= amount_to_take;
	assert(*from_bucket >= 0);

	if(to_bucket)
	{
		if(*to_bucket + amount_to_add > *to_bucket)	// overflow check
			*to_bucket += amount_to_add;
	}

#ifdef SOLAR
	printf("from bucket = %d  ", *from_bucket);
	if(to_bucket)
		printf("to bucket = %d", *to_bucket);
	printf("\n");
#endif

	// if this is a trade move, inform the person being traded with
	if(mc->to_slot == 3 && trader && trader->IsClient())
	{
		Client* recipient = trader->CastToClient();
		recipient->Message(15, "%s adds some coins to the trade.", GetName());
		recipient->Message(15, "The total trade is: %i PP, %i GP, %i SP, %i CP",
			trade->pp, trade->gp,
			trade->sp, trade->cp
		);

		APPLAYER* outapp = new APPLAYER(OP_TradeCoins,sizeof(TradeCoin_Struct));
		TradeCoin_Struct* tcs = (TradeCoin_Struct*)outapp->pBuffer;
		tcs->trader = trader->GetID();
		tcs->slot = mc->cointype2;
		tcs->unknown5 = 0x4fD2;
		tcs->unknown7 = 0;
		tcs->amount = amount_to_add;
		recipient->QueuePacket(outapp);
		safe_delete(outapp);
	}

	Save();
}

void Client::OPGMTraining(const APPLAYER *app)
{
	int cur_skill;

	APPLAYER* outapp = app->Copy();
	GMTrainee_Struct* gmtrain = (GMTrainee_Struct*) outapp->pBuffer;

	Mob* pTrainer = entity_list.GetMob(gmtrain->npcid);

	if(!pTrainer)
		return;

	for (cur_skill = 0; cur_skill <= HIGHEST_SKILL; cur_skill++)
	{
		gmtrain->skills[cur_skill] = pTrainer->CastToMob()->MaxSkill(cur_skill);
	}
	uchar ending[]={0xE0,0xCB,0x90,0x3F,0x01
		,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9
		,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9,0xC9
		,0x88,0x49,0x00};
	memcpy(&outapp->pBuffer[outapp->size-40],ending,sizeof(ending));
	FastQueuePacket(&outapp);

	// welcome message
	if (pTrainer && pTrainer->IsNPC())
	{
		pTrainer->Say_StringID(MakeRandomInt(1204, 1207), GetCleanName());
	}
}

void Client::OPGMEndTraining(const APPLAYER *app)
{
	APPLAYER *outapp = new APPLAYER(OP_GMEndTrainingResponse, 0);
	GMTrainEnd_Struct *p = (GMTrainEnd_Struct *)app->pBuffer;

	FastQueuePacket(&outapp);

	Mob* pTrainer = entity_list.GetMob(p->npcid);

	// goodbye message
	if (pTrainer && pTrainer->IsNPC())
	{
		pTrainer->Say_StringID(MakeRandomInt(1208, 1211), GetCleanName());
	}
}

void Client::OPGMTrainSkill(const APPLAYER *app)
{
	DumpPacket(app);

	if(!m_pp.points)
		return;

	GMSkillChange_Struct* gmskill = (GMSkillChange_Struct*) app->pBuffer;
	if (gmskill->skillbank == 0x01)
	{
		// languages go here
		if (gmskill->skill_id > 25)
		{
			cout << "Wrong Training Skill (languages)" << endl;
			DumpPacket(app);
			return;
		}
		cout << "Training language: " << gmskill->skill_id << endl;
		IncreaseLanguageSkill(gmskill->skill_id);
	}
	else if (gmskill->skillbank == 0x00)
	{
		// normal skills go here
		if (gmskill->skill_id > HIGHEST_SKILL)
		{
			cout << "Wrong Training Skill (abilities)" << endl;
			DumpPacket(app);
			return;
		}

		int8 skilllevel = GetRawSkill(gmskill->skill_id);

		if ( skilllevel == 255)
		{
			// Client never gets this skill; check for gm status or fail
			return;
		}
		else if (skilllevel == 254)
		{
			// Client training new skill for the first time set the skill to level-1

			int16 t_level = database.GetTrainlevel(GetClass(), gmskill->skill_id);
			cout<<"t_level:"<<t_level<<endl;
			if (t_level == 66 || t_level == 0)
			{
				return;
			}
			//m_pp.skills[gmskill->skill_id + 1] = t_level;
			SetSkill(gmskill->skill_id, t_level);
		}
		else if (skilllevel <= 251)
		{
			// Client train a valid skill
			// FIXME If the client doesn't do the "You are more skilled than I" check we should do it here
			SetSkill(gmskill->skill_id, skilllevel + 1);
		}
		else
		{
			// Log a warning someones been hacking
			LogFile->write(EQEMuLog::Error, "OP_GMTrainSkill: failed client: %s", GetName());
			return;
		}
	}
	m_pp.points--;
}

// this is used for /summon and /corpse
void Client::OPGMSummon(const APPLAYER *app)
{
	GMSummon_Struct* gms = (GMSummon_Struct*) app->pBuffer;
	Mob* st = entity_list.GetMob(gms->charname);

	if(st && st->IsCorpse())
	{
		st->CastToCorpse()->Summon(this, false);
	}
	else
	{
		if(admin < 80)
		{
			return;
		}
		if(st)
		{
			Message(0, "Local: Summoning %s to %i, %i, %i", gms->charname, gms->x, gms->y, gms->z);
			if (st->IsClient() && (st->CastToClient()->GetAnon() != 1 || this->Admin() >= st->CastToClient()->Admin()))
				st->CastToClient()->MovePC((char*) 0, gms->x, gms->y, gms->z, 2, true);
			else
				st->GMMove(this->GetX(), this->GetY(), this->GetZ(),this->GetHeading());
		}
		else
		{
			int8 tmp = gms->charname[strlen(gms->charname)-1];
			if (!worldserver.Connected())
			{
				Message(0, "Error: World server disconnected");
			}
			else if (tmp < '0' || tmp > '9') // dont send to world if it's not a player's name
			{
				ServerPacket* pack = new ServerPacket;
				pack->opcode = ServerOP_ZonePlayer;
				pack->size = sizeof(ServerZonePlayer_Struct);
				pack->pBuffer = new uchar[pack->size];
				memset(pack->pBuffer, 0, pack->size);
				ServerZonePlayer_Struct* szp = (ServerZonePlayer_Struct*) pack->pBuffer;
				strcpy(szp->adminname, this->GetName());
				szp->adminrank = this->Admin();
				strcpy(szp->name, gms->charname);
				strcpy(szp->zone, zone->GetShortName());
				szp->x_pos = gms->x;
				szp->y_pos = gms->y;
				szp->z_pos = gms->z;
				szp->ignorerestrictions = 2;
				worldserver.SendPacket(pack);
				safe_delete(pack);
			}
		}
	}
}

void Client::OPCombatAbility(const APPLAYER *app) {
	if(!target)
		return;
	if(!IsAttackAllowed(target))
		return;

	CombatAbility_Struct* ca_atk = (CombatAbility_Struct*) app->pBuffer;
	if ((ca_atk->m_atk == 100) && (ca_atk->m_type==10)) {    // SLAM - Bash without a shield equipped
		DoAnim(animTailRake);
		float chance = (level+GetSkill(BASH)+GetSTR())/5;
		sint32 dmg = 0;
		if(chance<20)
			chance=20;
		else if(chance>90)
			chance=90;

		//this formula is just a hack, its not perfect by any means
		if((rand()%100)<chance)//success figure damage
			dmg = ((((level/10)*(rand()%7))+GetSkill(BASH)*5+GetSTR())/100)*(rand()%10);
		target->Damage(this, dmg, 0xffff, BASH);
		CheckIncreaseSkill(BASH);
		
		/* using CheckIncreaseSkill now
		if (GetClass()==WARRIOR&&(GetRace()==BARBARIAN||GetRace()==TROLL||GetRace()==OGRE)) { // large race warriors only *
			float wisebonus =  (m_pp.WIS > 200) ? 20 + ((m_pp.WIS - 200) * 0.05) : m_pp.WIS * 0.1;
			if (((55-(GetSkill(BASH)*0.240))+wisebonus > MakeRandomFloat(0, 100))&& (GetSkill(BASH)<(m_pp.level+1)*5))
					this->SetSkill(BASH,GetRawSkill(BASH)+1);
		}*/
		return;
	}
	
	//throwing weapons
	if ((ca_atk->m_atk == 11)&&(ca_atk->m_type == 51)) {
		ThrowingAttack(target);
		return;
	}
	
	//ranged attack (archery)
	if ((ca_atk->m_atk == 11)&&(ca_atk->m_type==7)) {
		RangedAttack(target);
		return;
	}
	
	float multiple=(GetLevel()/5);
	multiple++;
	switch(GetClass())
	{
	case WARRIOR:
		if (target!=this) {
			float dmg=((((GetSkill(KICK) + GetSTR() + GetLevel())/90)*multiple)+10) * ( MakeRandomFloat(0, 1) );
			if(target->IsClient())
				dmg*=.76;
			else{
				CheckIncreaseSkill(KICK);
				dmg*=1.2f;//small increase for warriors
			}
			target->Damage(this, (int32)dmg, 0xffff, 0x1e);
			DoAnim(animKick);
		}
		break;
	case RANGER:
	case BEASTLORD:
		if (target!=this) {
			float dmg=((((GetSkill(KICK) + GetSTR() + GetLevel())/250)*multiple)+5) * ( MakeRandomFloat(0, 1) );
			if(target->IsClient())
				dmg*=.67f;
			else
				CheckIncreaseSkill(KICK);
			target->Damage(this, (int32)dmg, 0xffff, 0x1e);
			DoAnim(animKick);
		}
		break;
	case PALADIN:
	case SHADOWKNIGHT:
		break;
	case MONK:
		CheckIncreaseSkill(ca_atk->m_type);
		MonkSpecialAttack(target->CastToMob(), ca_atk->m_type);
		break;
	case ROGUE:
		if (ca_atk->m_atk != 100) {
			break;
		}
		uint8 aa_item = GetAA(aaChaoticStab);// Chaotic backstab TODO make it do min damage
		if (target && BehindMob(target, GetX(), GetY())) // Player is behind target
		{
			// solar - chance to assassinate
			// TODO: it's set to 40% chance, should be a formula involving DEX
			float chance=0;
			if(
				level >= 60 && // player is 60 or higher
				target->GetLevel() <= 45 && // mob 45 or under
				!target->CastToNPC()->IsEngaged() && // not aggro
				target->GetHP()<=32000 &&
				(chance = MakeRandomFloat(0, 100)) < 40 // chance
				&& target->IsNPC()
				) {
				//char temp[100];
				//snprintf(temp, 100, "%s ASSASSINATES their victim!!", this->GetName());
				//entity_list.MessageClose(this, 0, 200, 10, temp);
				entity_list.MessageClose_StringID(this, false, 200, 10, ASSASSINATES, GetName());
				CheckIncreaseSkill(BACKSTAB);
				RogueAssassinate(target);
			}
			else {
				RogueBackstab(target, m_inv.GetItem(SLOT_PRIMARY), GetSkill(BACKSTAB));
				if ((level > 54) && (target != 0)) {
					float DoubleAttackProbability = (GetSkill(DOUBLE_ATTACK) + GetLevel()) / 500.0f; // 62.4 max
					// Check for double attack with main hand assuming maxed DA Skill (MS)
					float random = MakeRandomFloat(0, 1);
					
					if(random < DoubleAttackProbability)		// Max 62.4 % chance of DA
						if(target && target->GetHP() > 0)
							RogueBackstab(target, m_inv.GetItem(SLOT_PRIMARY), GetSkill(BACKSTAB));
				}
				CheckIncreaseSkill(BACKSTAB);
			}
		}
		else if(aa_item>0) {
			RogueBackstab(target, m_inv.GetItem(SLOT_PRIMARY), GetSkill(BACKSTAB));
			if ((level > 54) && (target != 0)) {
				float DoubleAttackProbability = (GetSkill(DOUBLE_ATTACK) + GetLevel()) / 500.0f; // 62.4 max
				CheckIncreaseSkill(BACKSTAB);
				// Check for double attack with main hand assuming maxed DA Skill (MS)
				float random = MakeRandomFloat(0, 1);
				if(random < DoubleAttackProbability)		// Max 62.4 % chance of DA
					if(target && target->GetHP() > 0)
						RogueBackstab(target, m_inv.GetItem(SLOT_PRIMARY), GetSkill(BACKSTAB));
			}
		}
		else {	// Player is in front of target
			Attack(target, 13);
			if ((level > 54) && (target != 0)) {
				float DoubleAttackProbability = (GetSkill(DOUBLE_ATTACK) + GetLevel()) / 500.0f; // 62.4 max
				
				// Check for double attack with main hand assuming maxed DA Skill (MS)
				float random = MakeRandomFloat(0, 1);
				if(random < DoubleAttackProbability)		// Max 62.4 % chance of DA
					if(target && target->GetHP() > 0)
						Attack(target, 13);
			}
		}
		break;
	}
}

void Client::DoHPRegen() {
	sint32 normal_regen = LevelRegen();
	sint32 item_regen = itembonuses.HPRegen;
	sint32 spell_regen = spellbonuses.HPRegen;
	sint32 total_regen = normal_regen + item_regen + spell_regen;
	SetHP(GetHP() + total_regen);
	SendHPUpdate();
}

void Client::DoManaRegen() {
	if (GetMana() >= max_mana)
		return;
	int32 level=GetLevel();
	int32 regen = 0;
	if (IsSitting()) {		//this should be changed so we dont med while camping, etc...
		int32 med = GetSkill(MEDITATE);
		if(med > 0) {
			medding = true;
			regen = (((GetSkill(MEDITATE)/10)+(level-(level/4)))/4)+4;
			regen += spellbonuses.ManaRegen + itembonuses.ManaRegen;
			CheckIncreaseSkill(MEDITATE);
		}
		else
			regen = 2+spellbonuses.ManaRegen+itembonuses.ManaRegen+(level/5);
	}
	else {
		medding = false;
		regen = 2+spellbonuses.ManaRegen+itembonuses.ManaRegen+(level/5);
	}
	
	SetMana(GetMana() + regen);
	SendManaUpdatePacket();
}



