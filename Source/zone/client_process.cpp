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

int Client::HandlePacket(const APPLAYER *app)
{
	bool ret = true;
	
	if (app->opcode == OP_AckPacket) {
    	return true;
	}
	
	#if EQDEBUG >= 9
		cout << "Received 0x" << hex << setw(4) << setfill('0') << app->opcode << ", size=" << dec << app->size << endl;
	#endif
	
	#ifdef SOLAR
		if(0 && app->opcode != OP_ClientUpdate)
		{
			LogFile->write(EQEMuLog::Debug,"HandlePacket() OPCODE debug enabled client %s", GetName());
			cerr << "OPCODE: " << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << app->size << endl;
			DumpPacket(app);
		}
	#endif

	switch(client_state)
	{
		case CLIENT_CONNECTING:
		{
			this->IsOnBoat=false;
			switch(app->opcode){
				case OP_SetDataRate:
				{
					// Set client datarate
					if (app->size != sizeof(float)) {
						LogFile->write(EQEMuLog::Error,"Wrong size on OP_SetDatarate. Got: %i, Expected: %i", app->size, sizeof(float));
						break;
					}
					LogFile->write(EQEMuLog::Debug, "HandlePacket() OP_SetDataRate request : %f",  *(float*) app->pBuffer);
					float tmpDR = *(float*) app->pBuffer;
					if (tmpDR <= 0.0f) {
						LogFile->write(EQEMuLog::Error,"HandlePacket() OP_SetDataRate INVALID request : %f <= 0", tmpDR);
						LogFile->write(EQEMuLog::Normal,"WARNING: Setting datarate for client to 5.0 expect a client lock up =(");
						tmpDR = 5.0f;
					}
					if (tmpDR > 25.0f)
						tmpDR = 25.0f;
					eqnc->SetDataRate(tmpDR);
					break;
				}
				case OP_SendTributes:
					break;
				case OP_ZoneEntry: {
					// Quagmire - Antighost code
					// tmp var is so the search doesnt find this object
					char tmp[64] = {0};
					snprintf(tmp, 64, "%s", (char*)&app->pBuffer[4]);
					Client* client = entity_list.GetClientByName(tmp);
					if (!zone->GetAuth(ip, tmp, &WID, &account_id, &character_id, &admin, lskey, &tellsoff)) {
						LogFile->write(EQEMuLog::Error, "GetAuth() returned false kicking client");
						if (client != 0)
						{
							client->Save();
							client->Kick();
						}
						ret = false; // TODO: Can we tell the client to get lost in a good way
						break;
					}
					
					strcpy(name, tmp);
					if (client != 0) {
						struct in_addr ghost_addr;
						ghost_addr.s_addr = eqnc->GetrIP();
						
						LogFile->write(EQEMuLog::Error,"Ghosting client: Account ID:%i Name:%s Character:%s IP:%s",
											client->AccountID(), client->AccountName(), client->GetName(), inet_ntoa(ghost_addr));
						client->Save();
						client->Disconnect();
					}
					
					char* query = 0;
					uint32_breakdown workpt;
					workpt.b4() = DBA_b4_Entity;
					workpt.w2_3() = GetID();
					workpt.b1() = DBA_b1_Entity_Client_InfoForLogin;
					DBAsyncWork* dbaw = new DBAsyncWork(MTdbafq, workpt, DBAsync::Read);
					dbaw->AddQuery(1, &query, MakeAnyLenString(&query, "SELECT status,name,lsaccount_id,gmspeed,revoked FROM account WHERE id=%i", account_id));
					dbaw->AddQuery(2, &query, MakeAnyLenString(&query, "SELECT id,profile,zonename,x,y,z,guild,guildrank FROM character_ WHERE id=%i", character_id));
					dbaw->AddQuery(3, &query, MakeAnyLenString(&query, "SELECT faction_id,current_value FROM faction_values WHERE char_id = %i", character_id));
					if (!(pDBAsyncWorkID = dbasync->AddWork(&dbaw))) {
						safe_delete(dbaw);
						LogFile->write(EQEMuLog::Error,"dbasync->AddWork() returned false, client crash");
						ret = false;
						break;
					}
					break;
				}
				case OP_SetServerFilter: {
					SetServerFilter_Struct* filter=(SetServerFilter_Struct*)app->pBuffer;
					ServerFilter(filter);
					break;
				}
				case OP_SendAATable: {
					for(int a=0; a < MAX_PP_AA_ARRAY; a++){
						aa[a] = &m_pp.aa_array[a];
						int32 id = aa[a]->AA;
						if(aa[a]->value>1)
							aa_points[(id - aa[a]->value +1)] = aa[a]->value;
						else
							aa_points[id] = aa[a]->value;
					}
					SendAAList();
					break;
				}
				case 0x037f:{
					APPLAYER* outapp = new APPLAYER(0x0380, sizeof(int32));
					QueuePacket(outapp);
					safe_delete(outapp);
					break;
				}
				case OP_ReqClientSpawn: {
					
					//Send Tribute info
					//SendTribute();

					APPLAYER* outapp = new APPLAYER;
					
					// Send Zone Doors
					if (entity_list.MakeDoorSpawnPacket(outapp)) {
						//outapp->Deflate();
						QueuePacket(outapp);
					}
					safe_delete(outapp);
					
					// Send Zone Objects
					entity_list.SendZoneObjects(this);
					
					// Send Zone Points
					if (zone->numzonepoints > 0) {
						int32 zpsize = sizeof(ZonePoints) + ((zone->numzonepoints+1) * sizeof(ZonePoint_Entry));
						APPLAYER* outapp = new APPLAYER(OP_SendZonepoints,zpsize);
						ZonePoints* zp = (ZonePoints*)outapp->pBuffer;
						memset(zp, 0, zpsize);
						LinkedListIterator<ZonePoint*> iterator(zone->zone_point_list);
						iterator.Reset();
						zp->count = zone->numzonepoints;
						int32 count = 0;
						
						while(iterator.MoreElements())
						{
							ZonePoint* data = iterator.GetData();
							zp->zpe[count].iterator = data->number;
							zp->zpe[count].x = data->target_y;
							zp->zpe[count].y = data->target_x; //Backwards to convert to eqlives standard..
							zp->zpe[count].z = data->target_z;
							zp->zpe[count].heading=data->target_heading;
							zp->zpe[count].zoneid = database.GetZoneID((const char*)data->target_zone);
							iterator.Advance();
							count++;
						}
						//outapp->Deflate();
						QueuePacket(outapp);
						safe_delete(outapp);
					}
					
					// Tell client they can continue we're done
					outapp = new APPLAYER(OP_SendExpZonein, 0);
					QueuePacket(outapp);
					safe_delete(outapp);

					if(strncasecmp(zone->GetShortName(),"bazaar",6)==0)
						SendBazaarWelcome();
					if(GetAdventureID()>0){
						AdventureInfo ai=database.GetAdventureInfo(GetAdventureID());
						if(zone->GetZoneID() == ai.zoneid) {
							SendAdventureRequestData(entity_list.GetGroupByClient(this),false,true);
						}
						else if(zone->GetZoneID() == ai.zonedungeonid && 
							database.GetLDoNDungeon(zone->GetZoneID()) == true ){
							SendAdventureRequestData(entity_list.GetGroupByClient(this),true,false);
						}
						else
							SendAdventureRequestData(NULL,false,false,true);
					}
					break;
				}
				case OP_SendExpZonein: {
					//////////////////////////////////////////////////////
					// Spawn Appearance Packet
					APPLAYER* outapp = new APPLAYER(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
					SpawnAppearance_Struct* sa = (SpawnAppearance_Struct*)outapp->pBuffer;
					sa->type = AT_SpawnID;			// Is 0x10 used to set the player id?
					sa->parameter = GetID();	// Four bytes for this parameter...
					outapp->priority = 6;
					QueuePacket(outapp);
					safe_delete(outapp);
					
					// Inform the world about the client
					outapp = new APPLAYER();
					
					CreateSpawnPacket(outapp);
					outapp->priority = 6;
					entity_list.QueueClients(this, outapp, true);
					safe_delete(outapp);
					
					if(GuildDBID()!=0 && GuildDBID()!=0xFFFFFFFF)
						SendGuildMembers(GuildDBID());
					// Send exp packets
					outapp = new APPLAYER(OP_ExpUpdate, sizeof(ExpUpdate_Struct));
					ExpUpdate_Struct* eu = (ExpUpdate_Struct*)outapp->pBuffer;
					int32 tmpxp1 = GetEXPForLevel(GetLevel()+1);
					int32 tmpxp2 = GetEXPForLevel(GetLevel());

					// Quag: crash bug fix... Divide by zero when tmpxp1 and 2 equalled each other, most likely the error case from GetEXPForLevel() (invalid class, etc)
					if (tmpxp1 != tmpxp2 && tmpxp1 != 0xFFFFFFFF && tmpxp2 != 0xFFFFFFFF) {
						double tmpxp = (double) ( (double) m_pp.exp-tmpxp2 ) / ( (double) tmpxp1-tmpxp2 );
						eu->exp = (uint32)(330.0f * tmpxp);
						outapp->priority = 6;
						QueuePacket(outapp);
					}
					safe_delete(outapp);

					if(GetLevel() >= 51)
						SendAATimers();

					outapp = new APPLAYER(OP_SendExpZonein, 0);
					QueuePacket(outapp);
					safe_delete(outapp);

					outapp = new APPLAYER(OP_RaidInvite, sizeof(ZoneInSendName_Struct));
					ZoneInSendName_Struct* zonesendname=(ZoneInSendName_Struct*)outapp->pBuffer;
					strcpy(zonesendname->name,m_pp.name);
					strcpy(zonesendname->name2,m_pp.name);
					zonesendname->unknown0=0x0A;
					outapp->Deflate();

					QueuePacket(outapp);
					safe_delete(outapp);
					outapp = new APPLAYER(OP_ZoneInSendName2, sizeof(ZoneInSendName_Struct2));
					ZoneInSendName_Struct2* zonesendname2=(ZoneInSendName_Struct2*)outapp->pBuffer;
					strcpy(zonesendname2->name,m_pp.name);
					outapp->Deflate();
					QueuePacket(outapp);
					safe_delete(outapp);
					break;
				}
				case OP_ZoneComplete: {
					APPLAYER* outapp = new APPLAYER(0x0347, 0);
					QueuePacket(outapp);
					safe_delete(outapp);
					CompleteConnect();
					break;
				}
				case OP_ReqNewZone: {
					APPLAYER* outapp;
					
					/////////////////////////////////////
					// New Zone Packet
					outapp = new APPLAYER(OP_NewZone, sizeof(NewZone_Struct));
					NewZone_Struct* nz = (NewZone_Struct*)outapp->pBuffer;
					memcpy(outapp->pBuffer, &zone->newzone_data, sizeof(NewZone_Struct));
					strcpy(nz->char_name, m_pp.name);
					QueuePacket(outapp);
					safe_delete(outapp);
					break;
				}
				case OP_SpawnAppearance:
					break;
				case OP_WearChange: {
					if(app->size==9 && app->pBuffer[8]==6)
						SendHPUpdate();
					break;
				}
				case OP_ClientError: {
					// Client reporting error to server
					ClientError_Struct* error = (ClientError_Struct*)app->pBuffer;
					LogFile->write(EQEMuLog::Error, "Client error: %s", error->character_name);
					LogFile->write(EQEMuLog::Error, "Error message: %s", error->message);
					Message(13, error->message);
#if (EQDEBUG>=5)
				    DumpPacket(app);
#endif
					break;
				}
				case OP_ApproveZone: {
					ApproveZone_Struct* azone =(ApproveZone_Struct*)app->pBuffer;
					azone->approve=1;
					QueuePacket(app);
					break;
				}
				case OP_TGB: {
					OPTGB(app);
					break;
				}
				default:{
				LogFile->write(EQEMuLog::Error, "HandlePacket() Opcode error: Unexpected packet during CLIENT_CONNECTING: opcode: 0x%04x, size: %i", app->opcode, app->size);
#if EQDEBUG >= 9
					cout << "Unexpected packet during CLIENT_CONNECTING: OpCode: 0x" << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << app->size << endl;
					DumpPacket(app);
#endif
				}
			}
			break;
		}
		case CLIENT_CONNECTED:
			{
				adverrorinfo = app->opcode;
				#if EQDEBUG >= 9
					LogFile->write(EQEMuLog::Debug,"HandlePacket() OPCODE debug enabled about to process");
					cerr << "OPCODE: " << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << app->size << endl;
					DumpPacket(app);
				#endif
				
				switch(app->opcode)
				{
				case OP_ClientUpdate: {
					if (IsAIControlled())
						break;
					
					if (app->size != sizeof(PlayerPositionUpdateClient_Struct)) {
						LogFile->write(EQEMuLog::Error, "OP size error: OP_ClientUpdate expected:%i got:%i", sizeof(PlayerPositionUpdateClient_Struct), app->size);
						break;
					}

					PlayerPositionUpdateClient_Struct* ppu = (PlayerPositionUpdateClient_Struct*)app->pBuffer;

					// solar: a very low chance to improve at sense heading, since
					// the client doesn't send an opcode for the key anymore, ever
					// since they made it useless on live
					//
					// this checking of heading/x_pos is cheap, and the result is
					// subtle, but you'll notice your sense heading improve as you
					// travel around
					if(
						( (heading != ppu->heading) && !((int)heading % 3) ) ||	// turning
						( (x_pos != ppu->x_pos) && !((int)x_pos % 6) )					// moving
					)
					{
						CheckIncreaseSkill(SENSE_HEADING, -20);
					}

					// Update internal state
					delta_y			= ppu->delta_y;
					delta_x			= ppu->delta_x;
					delta_z			= ppu->delta_z;
					delta_heading	= ppu->delta_heading;
					heading			= EQ19toFloat(ppu->heading);

					if(IsTracking && ((x_pos!=ppu->x_pos) || (y_pos!=ppu->y_pos))){
						if(MakeRandomFloat(0, 100) < 70)//should be good
							CheckIncreaseSkill(TRACKING,-10);
					}
					y_pos			= ppu->y_pos;
					x_pos			= ppu->x_pos;
					z_pos			= ppu->z_pos;
					animation		= ppu->animation;
#ifdef GUILDWARS
					if(animation > 65 && admin<80 && CheckCheat()){
						if(cheater || cheatcount>0){
							Message(15,"Cheater log updated...yup your busted,its not nice to cheat.");
							char descript[50]={0};
							sprintf(descript,"%s: %i","Player using a movement of",ppu->animation);
							database.logevents(this->AccountName(),this->AccountID(),admin,this->GetName(),"none","Movement speed cheat",descript,15);
							Damage(this,200,0,4,false);
							if(cheater==false){
								worldserver.SendEmoteMessage(0,0,0,13,"<Cheater Locator> We have found a cheater.  %s (Acct: %s) was just caught hacking, please show them what we think of hackers...",this->GetName(),this->AccountName());
								cheater=true;
							}
							cheatcount=0;
						}
						else
							cheatcount++;
					}
					else
						cheatcount=0;
					cheat_x=x_pos;
					cheat_y=y_pos;
#endif
						//printf("animation: %i\n",ppu->animation);
					// Outgoing client packet
					if (ppu->y_pos != y_pos || ppu->x_pos != x_pos || ppu->heading != heading || ppu->animation != animation)
 					{
 						APPLAYER* outapp = new APPLAYER(OP_ClientUpdate, sizeof(PlayerPositionUpdateServer_Struct));
 						PlayerPositionUpdateServer_Struct* ppu = (PlayerPositionUpdateServer_Struct*)outapp->pBuffer;
 						MakeSpawnUpdate(ppu);
 						if (gmhideme)
 							entity_list.QueueClientsStatus(this,outapp,true,Admin(),250);
 						else
 						entity_list.QueueCloseClients(this,outapp,true,300);
 						safe_delete(outapp);
 					}
					break;
				}
				case OP_AutoAttack: {
					if (app->size == 4)	{
						if (app->pBuffer[0] == 0) {
							auto_attack = false;
							if (IsAIControlled())
								break;
							attack_timer.Disable();
							attack_dw_timer.Disable();
							SetAttackTimer();
						}
						else if (app->pBuffer[0] == 1) {
							auto_attack = true;
							if (IsAIControlled())
								break;
							attack_timer.Enable();
							attack_dw_timer.Enable();
							SetAttackTimer();
						}
					}
					else {
						LogFile->write(EQEMuLog::Error, "OP size error: OP_AutoAttack expected:4 got:%i", app->size);
					}
					break;
				}
				case OP_AutoAttack2: { // Why 2?
					break;
				}
				case OP_Consent:{
					if(app->size<64){
						char* c_name = (char*)app->pBuffer;
						Client* client = entity_list.GetClientByName(c_name);
						if(client && client!=this){
							consent_list.push_back(client);
							APPLAYER* outapp = new APPLAYER(OP_ConsentResponse, sizeof(ConsentResponse_Struct));
							ConsentResponse_Struct* crs = (ConsentResponse_Struct*)outapp->pBuffer;
							strcpy(crs->grantname,client->GetName());
							strcpy(crs->ownername,GetName());
							crs->permission=1;
							strcpy(crs->zonename,"all zones");
							client->QueuePacket(outapp);
							QueuePacket(outapp);
							safe_delete(outapp);
						}
						else if(client==this)
							Message_StringID(0,CONSENT_YOURSELF);
						else
							Message_StringID(0,CONSENT_INVALID_NAME);
					}
					break;
				}
				case OP_Deny:{
					if(app->size<64){
						char* c_name = (char*)app->pBuffer;
						Client* client = entity_list.GetClientByName(c_name);
						if(client){
							consent_list.remove(client);
							APPLAYER* outapp = new APPLAYER(OP_ConsentResponse, sizeof(ConsentResponse_Struct));
							ConsentResponse_Struct* crs = (ConsentResponse_Struct*)outapp->pBuffer;
							strcpy(crs->grantname,client->GetName());
							strcpy(crs->ownername,GetName());
							crs->permission=0;
							strcpy(crs->zonename,"all zones");
							client->QueuePacket(outapp);
							QueuePacket(outapp);
							safe_delete(outapp);
						}
						else
							Message_StringID(0,TARGET_NOT_FOUND);
					}
					break;
				}
				case OP_TargetMouse:		// mouse targetting a person
				case OP_TargetCommand: {	// /target user
					if (app->size != sizeof(ClientTarget_Struct)) {
						LogFile->write(EQEMuLog::Error, "OP size error: OP_TargetMouse expected:%i got:%i", sizeof(ClientTarget_Struct), app->size);
						break;
					}
					if(target)
						target->IsTargeted(false);

					// Locate and cache new target
					ClientTarget_Struct* ct=(ClientTarget_Struct*)app->pBuffer;
					pClientSideTarget = ct->new_target;
					if (!IsAIControlled())
						target = entity_list.GetMob(ct->new_target);
					
					// For /target, send reject or success packet
					if (app->opcode == OP_TargetCommand) {
						if (target && !target->CastToMob()->IsInvisible(this) && Dist(*target) <= TARGETING_RANGE) {
							QueuePacket(app);
							APPLAYER hp_app;
							target->IsTargeted(true);
							target->CreateHPPacket(&hp_app);
							QueuePacket(&hp_app, false);
						}
						else {
							APPLAYER* outapp = new APPLAYER(OP_TargetReject, sizeof(TargetReject_Struct));
							outapp->pBuffer[0] = 0x2f;
							outapp->pBuffer[1] = 0x01;
							outapp->pBuffer[4] = 0x0d;
							if(target)
								target->IsTargeted(false);
							QueuePacket(outapp);
							safe_delete(outapp);
						}
					}
					else{
						if(target)
							target->IsTargeted(true);
					}
					break;
				}
				case OP_Shielding: {
					if (shield_target)
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
					}
					Shielding_Struct* shield = (Shielding_Struct*)app->pBuffer;
					shield_target = entity_list.GetMob(shield->target_id);
					bool ack = false;
					ItemInst* inst = GetInv().GetItem(14);
					if (!shield_target)
						break;
					if (inst)
					{
						const Item_Struct* shield = inst->GetItem();
						if (shield && shield->Common.ItemUse == ItemUseShield)
						{
							for (int x = 0; x < 2; x++)
							{
								if (shield_target->shielder[x].shielder_id == 0)
								{
									entity_list.MessageClose(this,false,100,0,"%s uses their shield to guard %s.",GetName(),shield_target->GetName());
									shield_target->shielder[x].shielder_id = GetID();
									int shieldbonus = shield->Common.AC*2;
									switch (GetAA(197))
									{
										case 1:
											shieldbonus = shieldbonus * 115 / 100;
											break;
										case 2:
											shieldbonus = shieldbonus * 125 / 100;
											break;
										case 3:
											shieldbonus = shieldbonus * 150 / 100;
											break;
									}
									shield_target->shielder[x].shielder_bonus = shieldbonus;
									shield_timer.Start();
									ack = true;
									break;
								}
							}
						}
						else
						{
							Message(0,"You must have a shield equipped to shield a target!");
							shield_target = 0;
							break;
						}
					}
					else
					{
						Message(0,"You must have a shield equipped to shield a target!");
						shield_target = 0;
						break;
					}
					if (!ack)
					{
						Message(0,"No more than two warriors may shield the same being.");
						shield_target = 0;
						break;
					}
					break;
				}
				case OP_Jump: {
					// neotokyo: here we could reduce fatigue, if we knew how
					/*
					m_pp.fatigue += 10;
					if(m_pp.fatigue > 100)
						m_pp.fatigue = 100;
					*/
					break;
				}
				case OP_AdventureInfoRequest:{
					SendAdventureInfoRequest(app);
					break;
				}
				case OP_AdventureRequest:
					SendAdventureRequest();
					break;
				case OP_LDoNButton:{
					bool* p=(bool*)app->pBuffer;
					if(*p == true ) {
						Group* group=entity_list.GetGroupByClient(this);
						SendAdventureRequestData(group);
					}
					else
						SetAdventureID(0);
					break;
				}
				case OP_LeaveAdventure:{
					uchar lol[4]={0x3F,0x2A,0x00,0x00};
					APPLAYER* outapp=new APPLAYER(0x02e4,4);
					uchar* x=(uchar*)outapp->pBuffer;
					memcpy(x,lol,4);
					QueuePacket(outapp);
					safe_delete(outapp);
					//Cofruben: lol2 for message,lol for leave confirmation.
					uchar lol2[12]={0x0F,0x14,0x00,0x00,0x0D,0x00,0x00,0x00,0x01,0x00,0x00,0x00};
					APPLAYER* outapp2=new APPLAYER(0x01d7,12);
					uchar* xx=(uchar*) outapp2->pBuffer;
					memcpy(xx,lol2,12);
					QueuePacket(outapp2);
					safe_delete(outapp2);
					SendAdventureFinish(0,0);
					break;

				}
				case OP_Consume: {
					if (app->size != sizeof(Consume_Struct))
					{
						LogFile->write(EQEMuLog::Error, "OP size error: OP_Consume expected:%i got:%i", sizeof(Consume_Struct), app->size);
						break;
					}
					Consume_Struct* pcs = (Consume_Struct*)app->pBuffer;
					
					const Item_Struct* eat_item = GetInv().GetItem(pcs->slot)->GetItem();
					if (pcs->type == 0x01) {
#if EQDEBUG >= 1
						LogFile->write(EQEMuLog::Debug, "Eating from slot:%i", (int)pcs->slot);
#endif
						m_pp.hunger_level += eat_item->Common.CastTime*30; //roughly 1 item per 10 minutes
						GetInv().DeleteItem(pcs->slot,1);
					}
					else if (pcs->type == 0x02) {
#if EQDEBUG >= 1
						LogFile->write(EQEMuLog::Debug, "Drinking from slot:%i", (int)pcs->slot);
#endif
						// 6000 is the max. value
						//m_pp.thirst_level += 1000;
						m_pp.thirst_level += eat_item->Common.CastTime*30; //roughly 1 item per 10 minutes
						GetInv().DeleteItem(pcs->slot,1);
					}
					else {
						LogFile->write(EQEMuLog::Error, "OP_Consume: unknown type, type:%i", (int)pcs->type);
						break;
					}
					if (m_pp.hunger_level > 6000)
						m_pp.hunger_level = 6000;
					if (m_pp.thirst_level > 6000)
						m_pp.thirst_level = 6000;
					APPLAYER *outapp;
					outapp = new APPLAYER(OP_Stamina, sizeof(Stamina_Struct));
					Stamina_Struct* sta = (Stamina_Struct*)outapp->pBuffer;
					sta->food = m_pp.hunger_level;
					sta->water = m_pp.thirst_level;

					QueuePacket(outapp);
					safe_delete(outapp);
					break;
				}
				case OP_AdventureMerchantRequest: {
					// Packet contains entity id
					char msg[16000];
					memset(msg,0,16000);
					int8 count = 0;
					EntityId_Struct* eid = (EntityId_Struct*)app->pBuffer;
					int32 merchantid = 0;
					//DumpPacket(app);

					Mob* tmp = entity_list.GetMob(eid->entity_id);
					if (tmp != 0)
					{
						merchantid=tmp->CastToNPC()->MerchantType;
						tmp->CastToNPC()->FaceTarget(this->CastToMob());
					}
					else
						break;

					const Item_Struct *item = 0;
					std::list<MerchantList> merlist = zone->merchanttable[merchantid];
					std::list<MerchantList>::const_iterator itr;
					for(itr = merlist.begin();itr != merlist.end() && count<80;itr++){
						MerchantList ml = *itr;
						item = database.GetItem(ml.item);
						if(item)
						{
							sprintf(msg,"%s^%s,%i,%i,%i,0,1,32767,32767",msg,item->Name,item->ItemNumber,item->Common.ldonpointcost,item->Common.ldonpointtheme);
							count++;
						}
					}
					//Count
					//^Item Name,Item ID,Cost in Points,Theme (0=none),0,1,32767,32767
					APPLAYER* outapp = new APPLAYER(OP_AdventureMerchantResponse,strlen(msg)+2);
					outapp->pBuffer[0] = count;
					strncpy((char*)&outapp->pBuffer[1],msg,strlen(msg));
					//DumpPacket(outapp);
					//outapp->Deflate();
					QueuePacket(outapp);
					safe_delete(outapp);
					break;
					}
				case OP_AdventureMerchantPurchase: {
					Adventure_Purchase_Struct* aps = (Adventure_Purchase_Struct*)app->pBuffer;
/*
					Get item apc->itemid (can check NPC if thats necessary), ldon point theme check only if theme is not 0 (I am not sure what 1-5 are though for themes)
					if(ldon_available_points >= item ldonpointcost)
					{
					give item (67 00 00 00 for the packettype using opcode 0x02c5)
					ldon_available_points -= ldonpointcost;
					}
*/
					int32 merchantid = 0;
					Mob* tmp = entity_list.GetMob(aps->npcid);
					if (tmp != 0)
						merchantid = tmp->CastToNPC()->MerchantType;
					else
						break;

					const Item_Struct* item = 0;
					std::list<MerchantList> merlist = zone->merchanttable[merchantid];
					std::list<MerchantList>::const_iterator itr;

					for(itr = merlist.begin();itr != merlist.end();itr++){
						MerchantList ml = *itr;
					    item = database.GetItem(ml.item);
						if(item && item->ItemNumber == aps->itemid) //This check to make sure that the item is actually on the NPC, people attempt to inject packets to get items summoned...
							break;
						else if(item && item->ItemNumber != aps->itemid)
							item = 0;
					}
					if (!item) {
						Message(13, "Error: The item you purchased does not exist!");
						break;
					}
					if(m_pp.ldon_available_points >= item->Common.ldonpointcost)
					{
						sint8 charges=1;
							charges=item->Common.MaxCharges;
						ItemInst* inst = ItemInst::Create(item,charges);
						if (inst) {
							sint16 openslot = m_inv.FindFreeSlot(false,true, item->Size);
							if(openslot == SLOT_INVALID)
								break;
							sint32 requiredpts = (sint32)item->Common.ldonpointcost*-1;
							if(!UpdateLDoNPoints(requiredpts,item->Common.ldonpointtheme))
								break;
							if(charges > 0)
								inst->SetCharges(charges);
							else
								inst->SetCharges(1);
							SendItemPacket(openslot, inst, ItemPacketTrade);
							PutItemInInventory(openslot,*inst);
							safe_delete(inst);
						}
					}
					//DumpPacket(app);
					break;
					}
				case OP_ConsiderCorpse: {
					if (app->size == sizeof(Consider_Struct))
					{
						Consider_Struct* conin = (Consider_Struct*)app->pBuffer;
						Corpse* tcorpse = entity_list.GetCorpseByID(conin->targetid);
						if (tcorpse && tcorpse->IsNPCCorpse()) {
							Message(10, "This corpse regards you sadly. Noone asked what i wanted my tombstone to say!");
							int32 min; int32 sec; int32 ttime;
							if ((ttime = tcorpse->GetDecayTime()) != 0) {
								sec = (ttime/1000)%60; // Total seconds
								min = (ttime/60000)%60; // Total seconds / 60 drop .00
								//Message(10,  "This corpse will decay in %i minutes, %i seconds.", min, sec);
								char val1[20]={0};
								char val2[20]={0};
								Message_StringID(10,CORPSE_DECAY1,ConvertArray(min,val1),ConvertArray(sec,val2));
							}
							else {
								Message_StringID(10,CORPSE_DECAY_NOW);
								//Message(10,  "This corpse is waiting to expire.");
							}
						}
						else if (tcorpse && tcorpse->IsPlayerCorpse()) {
							Message(10, "This corpse glares at you threateningly! I told you that wasn't going to work");
							int32 min; int32 sec; int32 ttime;
							if ((ttime = tcorpse->GetDecayTime()) != 0) {
								sec = (ttime/1000)%60; // Total seconds
								min = (ttime/60000)%60; // Total seconds / 60 drop .00
								//Message(10,  "This corpse will decay in %i minutes, %i seconds.", min, sec);
								char val1[20]={0};
								char val2[20]={0};
								Message_StringID(10,CORPSE_DECAY1,ConvertArray(min,val1),ConvertArray(sec,val2));
							}
							else {
								//Message(10,  "This corpse is waiting to expire.");
								Message_StringID(10,CORPSE_DECAY_NOW);
							}
						}
					}
					else {
						LogFile->write(EQEMuLog::Debug, "Size mismatch in Consider corpse expected %i got %i", sizeof(Consider_Struct), app->size);
					}
					break;
				}
				case OP_Consider: {
					Consider_Struct* conin = (Consider_Struct*)app->pBuffer;
					Mob* tmob = entity_list.GetMob(conin->targetid);
					if (tmob == 0)
						break;
					APPLAYER* outapp = new APPLAYER(OP_Consider, sizeof(Consider_Struct));
					Consider_Struct* con = (Consider_Struct*)outapp->pBuffer;
					con->playerid = GetID();
					con->targetid = conin->targetid;
					if(tmob->IsNPC())
						con->faction = GetFactionLevel(character_id, tmob->GetNPCTypeID(), race, class_, deity,(tmob->IsNPC()) ? tmob->CastToNPC()->GetPrimaryFaction():0, tmob); // rembrant, Dec. 20, 2001; TODO: Send the players proper deity
					else
						con->faction = 1;
					con->level = GetLevelCon(tmob->GetLevel());
					if(zone->IsPVPZone()) {
						if (!tmob->IsNPC() )
							con->pvpcon = tmob->CastToClient()->GetPVP();
					}

					// Mongrel: If we're feigned show NPC as indifferent 
					if (tmob->IsNPC()) 
					{ 
						if (GetFeigned()) 
							con->faction = FACTION_INDIFFERENT; 
					} 

					QueuePacket(outapp);
					safe_delete(outapp);
					break;
				}
				case OP_Begging:{
					if(GetSkill(BEGGING)>0){
					int ran=MakeRandomInt(0,100);
					int chancetoattack=0;
					if(this->GetLevel() > this->GetTarget()->GetLevel())
						chancetoattack=MakeRandomInt(0,15);
					else
						chancetoattack=MakeRandomInt(((this->GetTarget()->GetLevel() - this->GetLevel())*10)-5,((this->GetTarget()->GetLevel() - this->GetLevel())*10));
					if(chancetoattack<0)
						chancetoattack=-chancetoattack;
					if(ran<chancetoattack){
						this->GetTarget()->Attack(this);
						break;
					}
					float chancetobeg=((float)(GetSkill(BEGGING)/700.0f) + 0.15f) * 100;

					if(ran<chancetobeg)
					{
						APPLAYER* outapp = new APPLAYER(OP_MoneyOnCorpse, sizeof(moneyOnCorpseStruct)); 
						moneyOnCorpseStruct* d = (moneyOnCorpseStruct*) outapp->pBuffer; 
						d->copper=MakeRandomInt(1,3);
						d->silver=MakeRandomInt(1,1);
						d->platinum=0;
						d->gold=0;
						d->response      = 1; 
						d->unknown1      = 0x5a; 
						d->unknown2      = 0x40; 
						d->unknown3      = 0; 
						AddMoneyToPP(d->copper, d->silver, d->gold, d->platinum,true); 
						QueuePacket(outapp);
						safe_delete(outapp);
						Message(0,"Begging success!.Received %i silver and %i copper!",d->silver,d->copper);
					}
					else
						Message(0,"Your attempt to beg was not succesful.");
					}
					break;
				}
				case OP_TestBuff: {
#ifdef GUILDWARS
					int32 ClassItemTable[16][10] = {
											0,0,0,0,0,0,0,0,0,0,
						/*Warrior*/			20301, 20302, 20303, 20304, 20305, 20306, 20298, 0, 0, 0,
						/*Cleric*/			20308, 20309, 20310, 20311, 20312, 20313, 20314, 20330, 0, 0,
						/*Paladin*/			20315, 20316, 20317, 20318, 20319, 20320, 20321, 20329, 0, 0,
						/*Ranger*/			27509, 27510, 27511, 27512, 27513, 27514, 27515, 27532, 0, 0,
						/*ShadowKnight*/	4968, 4969, 4970, 4971, 4972, 4973, 4974, 4975, 5121, 0,
						/*Druid*/			27516, 27517, 27518, 27519, 27520, 27521, 27522, 27531, 0, 0,
						/*Monk*/			9879, 9880, 9881, 9882, 9883, 9884, 9885, 0, 0, 0,
						/*Bard*/			27523, 27524, 27525, 27526, 27527, 27528, 27529, 27533, 0, 0,
						/*Rogue*/			24412, 24414, 24417, 24418, 24419, 24420, 24421, 27530, 0, 0,
						/*Shaman*/			51069, 51070, 51071, 51072, 51073, 51074, 51075, 51095, 0, 0,
						/*Necromancer*/		20322, 20324, 20325, 20326, 20327, 20328, 20332, 19911, 19913, 20323,
						/*Wizard*/			20322, 20324, 20325, 20326, 20327, 20328, 20332, 19911, 19913, 20323,
						/*Magician*/		20322, 20324, 20325, 20326, 20327, 20328, 20332, 19911, 19913, 20323,
						/*Enchanter*/		20322, 20324, 20325, 20326, 20327, 20328, 20332, 19911, 19913, 20323
					};

					if(GetClass() > 14)
						break;

						for(int i=0;i<10;i++)
						{
						sint16 slot = m_inv.FindFreeSlot(false,false);
						if(slot == SLOT_INVALID)
						slot = m_inv.FindFreeSlot(false,true);

						if(ClassItemTable[GetClass()][i] != 0 && (slot != SLOT_INVALID) 
							&& (m_inv.HasItem(ClassItemTable[GetClass()][i], 1, invWhereWorn|invWherePersonal) == SLOT_INVALID))
						{
						const Item_Struct* item = database.GetItem(ClassItemTable[GetClass()][i]);
						ItemInst* inst = ItemInst::Create(item, 1);

						if(item)
						{
						SendItemPacket(slot, inst, ItemPacketTrade);
						PutItemInInventory(slot,*inst,true);
						}
						safe_delete(inst);
						}
						}
#endif
					break;
				}
				case OP_Surname: {
					Surname_Struct* surname = (Surname_Struct*) app->pBuffer;
					char *c = surname->lastname;
					int found_bad_char = 0;

					// solar: fix name so first letter is capital, the rest is lowercase
					// and check for illegal chars too
					for(c = surname->lastname; *c; c++)
					{
						if(!isalpha(*c))
						{
							found_bad_char = 1;
							break;
						}
						if(c == surname->lastname)	// first letter
							*c = toupper(*c);
						else
							*c = tolower(*c);
					}

					if(!strcasecmp(surname->lastname,"ld") || !strcasecmp(surname->lastname,"afk"))
						found_bad_char = 1;
					
					if (found_bad_char) {
						Message(13, "Surnames may only contain alphabet characters.");
						break;
					}
					else if (strlen(surname->lastname)>=20) {
						Message_StringID(10,STRING_SURNAME_TOO_LONG);
						break;
					}
					ChangeLastName(surname->lastname);
					
					APPLAYER* outapp = new APPLAYER(OP_GMLastName, sizeof(GMLastName_Struct));
					GMLastName_Struct* lnc = (GMLastName_Struct*) outapp->pBuffer;
					strcpy(lnc->name, surname->name);
					strcpy(lnc->lastname, surname->lastname);
					strcpy(lnc->gmname, "SurnameOP");
					lnc->unknown[0] = 1;
					lnc->unknown[1] = 1;
					entity_list.QueueClients(this, outapp, false);
					safe_delete(outapp);
					
					outapp = app->Copy();
					surname = (Surname_Struct*) outapp->pBuffer;
					surname->unknown0064=1;
					//outapp->Deflate();
					FastQueuePacket(&outapp);
					break;
				}
				case OP_YellForHelp: {
					entity_list.QueueCloseClients(this,app, true, 100.0);
					break;
				}
				case OP_Assist: {
					if (app->size != sizeof(EntityId_Struct)) {
						LogFile->write(EQEMuLog::Debug, "Size mismatch in OP_Assist expected %i got %i", sizeof(EntityId_Struct), app->size);
						break;
					}

					EntityId_Struct* eid = (EntityId_Struct*)app->pBuffer;
					Entity* entity = entity_list.GetID(eid->entity_id);
					
					APPLAYER* outapp = app->Copy();
					eid = (EntityId_Struct*)outapp->pBuffer;
					eid->entity_id = GetID();
					if(entity && entity->IsMob())
					{
						Mob *assistee = entity->CastToMob();
						if(!assistee->IsInvisible(this) && assistee->GetTarget())
						{
							Mob *new_target = assistee->GetTarget();
							if
							(
								new_target &&
								!new_target->IsInvisible(this) &&
								Dist(*assistee) <= TARGETING_RANGE &&
								Dist(*new_target) <= TARGETING_RANGE
							)
							{
								eid->entity_id = new_target->GetID();
							}
						}
					}
					
					FastQueuePacket(&outapp);
					break;
				}
				case OP_GMTraining: {
					if (app->size != sizeof(GMTrainee_Struct)) {
						LogFile->write(EQEMuLog::Debug, "Size mismatch in OP_GMTraining expected %i got %i", sizeof(GMTrainee_Struct), app->size);
						DumpPacket(app);
						break;
					}
					OPGMTraining(app);
					break;
				}
				case OP_GMEndTraining: {
					if (app->size != sizeof(GMTrainEnd_Struct)) {
						LogFile->write(EQEMuLog::Debug, "Size mismatch in OP_GMEndTraining expected %i got %i", sizeof(GMTrainEnd_Struct), app->size);
						DumpPacket(app);
						break;
					}
					OPGMEndTraining(app);
					break;
				}
				case OP_GMTrainSkill: {
					if (app->size != sizeof(GMSkillChange_Struct)) {
						LogFile->write(EQEMuLog::Debug, "Size mismatch in OP_GMTrainSkill expected %i got %i", sizeof(GMSkillChange_Struct), app->size);
						DumpPacket(app);
						break;
					}
					OPGMTrainSkill(app);
					break;
				}
				case OP_DuelResponse: {
					if(app->size != sizeof(DuelResponse_Struct))
						break;
					DuelResponse_Struct* ds = (DuelResponse_Struct*) app->pBuffer;
					Entity* entity = entity_list.GetID(ds->target_id);
					Entity* initiator = entity_list.GetID(ds->entity_id);
					if(!entity->IsClient() || !initiator->IsClient())
						break;
					
					entity->CastToClient()->SetDuelTarget(0);
					entity->CastToClient()->SetDueling(false);
					initiator->CastToClient()->SetDuelTarget(0);
					initiator->CastToClient()->SetDueling(false);
					if(GetID() == initiator->GetID())
						entity->CastToClient()->Message_StringID(10,DUEL_DECLINE,initiator->GetName());
					else
						initiator->CastToClient()->Message_StringID(10,DUEL_DECLINE,entity->GetName());
					break;
				}
				case OP_DuelResponse2: {
					if(app->size != sizeof(Duel_Struct))
						break;
					
					Duel_Struct* ds = (Duel_Struct*) app->pBuffer;
					Entity* entity = entity_list.GetID(ds->duel_target);
					Entity* initiator = entity_list.GetID(ds->duel_initiator);
					
					if (entity && initiator && entity == this && initiator->IsClient()) {
						APPLAYER* outapp = new APPLAYER(OP_RequestDuel, sizeof(Duel_Struct));
						Duel_Struct* ds2 = (Duel_Struct*) outapp->pBuffer;
						
						ds2->duel_initiator = entity->GetID();
						ds2->duel_target = entity->GetID();
						initiator->CastToClient()->QueuePacket(outapp);
						
						outapp->opcode = OP_DuelResponse2;
						ds2->duel_initiator = initiator->GetID();

						initiator->CastToClient()->QueuePacket(outapp);
						
						QueuePacket(outapp);
						SetDueling(true);
						initiator->CastToClient()->SetDueling(true);
						SetDuelTarget(ds->duel_initiator);
						safe_delete(outapp);

						if (IsCasting())
							InterruptSpell();
						if (initiator->CastToClient()->IsCasting())
							initiator->CastToClient()->InterruptSpell();
					}
					break;
				}
				case OP_RequestDuel: {
					if(app->size != sizeof(Duel_Struct))
						break;
					
					APPLAYER* outapp = app->Copy();
					Duel_Struct* ds = (Duel_Struct*) outapp->pBuffer;
					int32 duel = ds->duel_initiator;
					ds->duel_initiator = ds->duel_target;
					ds->duel_target = duel;
					Entity* entity = entity_list.GetID(ds->duel_target);
					if(GetID() != ds->duel_target && entity->IsClient() && (entity->CastToClient()->IsDueling() && entity->CastToClient()->GetDuelTarget() != 0)) {
						Message_StringID(10,DUEL_CONSIDERING,entity->GetName());
						break;
					}
					if(IsDueling()) {
						Message_StringID(10,DUEL_INPROGRESS);
						break;
					}
					
					if(GetID() != ds->duel_target && entity->IsClient() && GetDuelTarget() == 0 && !IsDueling() && !entity->CastToClient()->IsDueling() && entity->CastToClient()->GetDuelTarget() == 0) {
						SetDuelTarget(ds->duel_target);
						entity->CastToClient()->SetDuelTarget(GetID());
						ds->duel_target = ds->duel_initiator;
						entity->CastToClient()->FastQueuePacket(&outapp);
						entity->CastToClient()->SetDueling(false);
						SetDueling(false);
					}
					else
						safe_delete(outapp);
					break;
				}
				case OP_SpawnAppearance: {
					if (app->size != sizeof(SpawnAppearance_Struct)) {
						cout << "Wrong size on OP_SpawnAppearance. Got: " << app->size << ", Expected: " << sizeof(SpawnAppearance_Struct) << endl;
						break;
					}
					SpawnAppearance_Struct* sa = (SpawnAppearance_Struct*)app->pBuffer;
					
					if(sa->spawn_id != GetID())
						break;

					if (sa->type == AT_Invis) { 
						this->invisible = (sa->parameter == 1);
						entity_list.QueueClients(this, app, true);
						break;
					}
					else if (sa->type == AT_Anim) {
						if (IsAIControlled())
							break;
						if (sa->parameter == ANIM_STAND) {
							SetAppearance(0);
							playeraction = 0;
							SetFeigned(false);
							BindWound(this, false, true);
							camp_timer.Disable();
						}
						else if (sa->parameter == ANIM_SIT) {
							SetAppearance(1);
							playeraction = 1;
							if(!UseBardSpellLogic())
								InterruptSpell();
							SetFeigned(false);
							BindWound(this, false, true);
						}
						else if (sa->parameter == ANIM_CROUCH) {
							if(!UseBardSpellLogic())
								InterruptSpell();
							SetAppearance(2);
							playeraction = 2;
							SetFeigned(false);
							StopSong();
						}
						else if (sa->parameter == ANIM_DEATH) { // feign death too
							SetAppearance(3);
							playeraction = 3;
							InterruptSpell();
							StopSong();
						}
						else if (sa->parameter == ANIM_LOOT) {
							SetAppearance(4);
							playeraction = 4;
							SetFeigned(false);
						}

						// @merth: This is from old code
						// I have no clue what it's for
						/*
						else if (sa->parameter == 0x05) {
							// Illusion
							cout << "Illusion packet recv'd:" << endl;
							DumpPacket(app);
						}
						*/
						else {
							cerr << "Client " << name << " unknown apperance " << (int)sa->parameter << endl;
							break;
						}
						
						entity_list.QueueClients(this, app, true);
					}
					else if (sa->type == AT_Anon) {
						// For Anon/Roleplay
						if (sa->parameter == 1) { // Anon
							m_pp.anon = 1;
						}
						else if ((sa->parameter == 2) || (sa->parameter == 3)) { // This is Roleplay, or anon+rp
							m_pp.anon = 2;
						}
						else if (sa->parameter == 0) { // This is Non-Anon
							m_pp.anon = 0;
						}
						else {
							cerr << "Client " << name << " unknown Anon/Roleplay Switch " << (int)sa->parameter << endl;
							break;
						}
#ifdef GUILDWARS
						if(Admin() == 0)
						{
							m_pp.anon = 0;
							sa->parameter = 0;
						}
#endif
						entity_list.QueueClients(this, app, true);
						UpdateWho();
					}
					else if ((sa->type == AT_HP) && (dead == 0)) {
						break;
					}
					else if (sa->type == AT_AFK) {
						this->AFK = (sa->parameter == 1);
						entity_list.QueueClients(this, app, true);
					}
					//Father Nitwit:
					else if (sa->type == AT_Split) {
						auto_split = (sa->parameter == 1);
					}
					else if (sa->type == AT_Sneak) {
						this->sneaking = (sa->parameter == 1);
						entity_list.QueueClients(this, app, true);
					}
					else if (sa->type == AT_Size)
					{
						entity_list.QueueClients(this, app, false);
					}
					else if (sa->type == AT_Light)	// client emitting light (lightstone, shiny shield)
					{
						entity_list.QueueClients(this, app, false);
					}
					else if (sa->type == AT_Levitate)
					{
						// don't do anything with this, we tell the client when it's
						// levitating, not the other way around
					}
					else {
						cout << "Unknown SpawnAppearance type: 0x" << hex << setw(4) << setfill('0') << sa->type << dec
							<< " value: 0x" << hex << setw(8) << setfill('0') << sa->parameter << dec << endl;
					}
					break;
				}
				case OP_BazaarInspect:{
					if (app->size != sizeof(BazaarInspect_Struct)) {
						LogFile->write(EQEMuLog::Error, "Invalid size for BazaarInspect_Struct: Expected %i, Got %i",
							sizeof(BazaarInspect_Struct), app->size);
						break;
					}
					
					BazaarInspect_Struct* bis = (BazaarInspect_Struct*)app->pBuffer;
					const Item_Struct* item = database.GetItem(bis->item_id);
				
					if (!item) {
						Message(13, "Error: This item does not exist!");
						break;
					}
					
					ItemInst* inst = ItemInst::Create(item);
					if (inst) {
						SendItemPacket(0, inst, ItemPacketViewLink);
						safe_delete(inst);
					}
					
					break;
				}
				case OP_Death: {
					if(app->size != sizeof(Death_Struct))
						break;
					
					Death_Struct* ds = (Death_Struct*)app->pBuffer;
					
					if(GetHP() > 0)
						break;
					
					Mob* killer = entity_list.GetMob(ds->killer_id);
					Death(killer, ds->damage, ds->spell_id, ds->attack_skill);
					break;
				}
				case OP_MoveCoin: {
					if(app->size != sizeof(MoveCoin_Struct)){
						LogFile->write(EQEMuLog::Error, "Wrong size on OP_MoveCoin.  Got: %i, Expected: %i", app->size, sizeof(MoveCoin_Struct));
						DumpPacket(app);
						break;
					}
					OPMoveCoin(app);
					break;
				}
				case OP_ItemLinkClick: {
					if(app->size != sizeof(ItemViewRequest_Struct)){
						LogFile->write(EQEMuLog::Error, "Wrong size on OP_ItemLinkClick.  Got: %i, Expected: %i", app->size, sizeof(ItemViewRequest_Struct));
						DumpPacket(app);
						break;
					}
					DumpPacket(app);
					ItemViewRequest_Struct* ivrs = (ItemViewRequest_Struct*)app->pBuffer;
					
					const Item_Struct* item = database.GetItem(ivrs->item_id);
					if (!item) {
						Message(13, "Error: The item for the link you have clicked on does not exist!");
						break;
					}
					
					ItemInst* inst = ItemInst::Create(item);
					if (inst) {
						SendItemPacket(0, inst, ItemPacketViewLink);
						safe_delete(inst);
					}
					break;
				}
				case OP_MoveItem: {
					if(this->CharacterID()==0)
						break;
					if (app->size != sizeof(MoveItem_Struct)) {
						LogFile->write(EQEMuLog::Error, "Wrong size: OP_MoveItem, size=%i, expected %i", app->size, sizeof(MoveItem_Struct));
						break;
					}
					
					MoveItem_Struct* mi = (MoveItem_Struct*)app->pBuffer;
					SwapItem(mi);
					break;
				}
				case OP_Camp: {
					//LogFile->write(EQEMuLog::Debug, "%s sent a camp packet.", GetName());
					if(GetAdventureID()>0)DeleteCharInAdventure(CharacterID(),GetAdventureID());
					Save();
					LeaveGroup();
					if (GetGM()) {
						Disconnect();
					}
					camp_timer.Start(30000);
					break;
				}
				case OP_Logout: {
					//LogFile->write(EQEMuLog::Debug, "%s sent a logout packet.", GetName());
					Save();
					Disconnect();
					break;
				}
#if 0	//solar: this isn't used anymore, the client doesn't send a packet
				case OP_SenseHeading: {
					if (rand()%100 <= 15 && (GetSkill(SENSE_HEADING) < 200) && (GetSkill(SENSE_HEADING) < this->GetLevel()*5+5)) {
						this->SetSkill(SENSE_HEADING, GetRawSkill(SENSE_HEADING) + 1);
					}
					break;
				}
#endif
				case OP_FeignDeath: {
					if(GetClass() != MONK)
						break;
					if(!p_timers.Expired(pTimerFeignDeath, false)) {
						Message(13,"Ability recovery time not yet met.");
						break;
					}
					int reuse = FeignDeathReuseTime;
					switch (GetAA(aaRapidFeign))
					{
						case 1:
							reuse = 9;
							break;
						case 2:
							reuse = 7;
							break;
						case 3:
							reuse = 5;
							break;
					}
					p_timers.Start(pTimerFeignDeath, reuse-1);
					
					//BreakInvis();
					
					int16 primfeign = GetSkill(FEIGN_DEATH);
					int16 secfeign = GetSkill(FEIGN_DEATH);
					if (primfeign > 100) {
						primfeign = 100;
						secfeign = secfeign - 100;
						secfeign = secfeign / 2;
					}
					else
						secfeign = 0;
					
					int16 totalfeign = primfeign + secfeign;
					if (MakeRandomFloat(0, 160) > totalfeign) {
						SetFeigned(false);
						entity_list.MessageClose_StringID(this, false, 200, 10, STRING_FEIGNFAILED, GetName());
					}
					else {
						SetFeigned(true);
					}
					//what is this doing? why is it doing this and CheckIncreaseSkill??
					if ((uint16)MakeRandomInt(0, 300) > GetSkill(FEIGN_DEATH) && MakeRandomFloat(0, 4) == 1 && GetSkill(FEIGN_DEATH) < 200 && GetSkill(FEIGN_DEATH) < (uint16)(GetLevel()*5+5) ) {
						SetSkill(FEIGN_DEATH, GetRawSkill(FEIGN_DEATH) + 1);
					}

					CheckIncreaseSkill(FEIGN_DEATH);
					break;
				}
				case OP_Sneak: {
					if(GetSkill(SNEAK) < 1) {
						break; //You cannot sneak if you do not have sneak
					}
					
					if(!p_timers.Expired(pTimerSneak, false)) {
						Message(13,"Ability recovery time not yet met.");
						break;
					}
					p_timers.Start(pTimerSneak, SneakReuseTime-1);
					
					bool was = sneaking;
					if (sneaking){
						sneaking = false;
					}
					else {
						CheckIncreaseSkill(SNEAK,15);
					}
					float hidechance = ((GetSkill(SNEAK)/300.0f) + .25) * 100;
					float random = MakeRandomFloat(0, 100);
					if(!was && random < hidechance) {
						sneaking = true;
					}
					APPLAYER* outapp = new APPLAYER(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
					SpawnAppearance_Struct* sa_out = (SpawnAppearance_Struct*)outapp->pBuffer;
					sa_out->spawn_id = GetID();
					sa_out->type = 0x0F;
					sa_out->parameter = sneaking;
					QueuePacket(outapp);
					safe_delete(outapp);
					if(GetClass() == ROGUE){
						if (sneaking){
							outapp = new APPLAYER(0x0202,12);
							uint8 rawData0[12] = { 0x5B, 0x01, 0x00, 0x00, 0x0E, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
							memcpy(outapp->pBuffer,rawData0,12);
							QueuePacket(outapp);
							safe_delete(outapp)
						}
						else {
							outapp = new APPLAYER(0x0202,12);
							uint8 rawData0[12] = { 0x5C, 0x01, 0x00, 0x00, 0x0E, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
							memcpy(outapp->pBuffer,rawData0,12);
							QueuePacket(outapp);
							safe_delete(outapp)
						}
					}
					break;
				}
				case OP_Hide: {
					if(GetSkill(HIDE) < 1) {
						break; //You cannot hide if you do not have hide
					}
					
					if(!p_timers.Expired(pTimerHide, false)) {
						Message(13,"Ability recovery time not yet met.");
						break;
					}
					int reuse = HideReuseTime - GetAA(209);
					p_timers.Start(pTimerHide, reuse-1);
					
					float hidechance = ((GetSkill(HIDE)/300.0f) + .25) * 100;
					float random = MakeRandomFloat(0, 100);
					CheckIncreaseSkill(HIDE,15);					
					if (random < hidechance) {
						APPLAYER* outapp = new APPLAYER(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
						SpawnAppearance_Struct* sa_out = (SpawnAppearance_Struct*)outapp->pBuffer;
						sa_out->spawn_id = GetID();
						sa_out->type = 0x03;
						this->invisible = true;
						sa_out->parameter = 1;
						entity_list.QueueClients(this, outapp, true);
						safe_delete(outapp);
						invisible = true;
					}
					if(GetClass() == ROGUE){
						if (!auto_attack && entity_list.Fighting(this)) {
							if (MakeRandomInt(0, 300) < (int)GetSkill(HIDE)) {
								Message(0,"You momentarily duck out of combat.");
								entity_list.RemoveFromHateLists(this,true);
							} else {
								Message(0,"Your attempts to duck out of combat fail.");
							}
						}
						if (invisible){
							APPLAYER* outapp = new APPLAYER(0x0202,12);
							uint8 rawData0[12] = { 0x5A, 0x01, 0x00, 0x00, 0x0E, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
							memcpy(outapp->pBuffer,rawData0,12);
							QueuePacket(outapp);
							safe_delete(outapp)
						}
						else {
							APPLAYER* outapp = new APPLAYER(0x0202,12);
							uint8 rawData0[12] = { 0x59, 0x01, 0x00, 0x00, 0x0E, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
							memcpy(outapp->pBuffer,rawData0,12);
							QueuePacket(outapp);
							safe_delete(outapp)
						}
					}
					break;
				}
				case OP_ChannelMessage: {
					ChannelMessage_Struct* cm=(ChannelMessage_Struct*)app->pBuffer;
					
					if (app->size < sizeof(ChannelMessage_Struct)) {
						cout << "Wrong size " << app->size << ", should be " << sizeof(ChannelMessage_Struct) << "+ on 0x" << hex << setfill('0') << setw(4) << app->opcode << dec << endl;
						break;
					}
					if (IsAIControlled()) {
						Message(13, "You try to speak but cant move your mouth!");
						break;
					}
					
					ChannelMessageReceived(cm->chan_num, cm->language, &cm->message[0], cm->targetname);
					break;
				}
				case OP_WearChange: {
					if (app->size != sizeof(WearChange_Struct)) {
						cout << "Wrong size: OP_WearChange, size=" << app->size << ", expected " << sizeof(WearChange_Struct) << endl;
						//DumpPacket(app);
						break;
					}

					WearChange_Struct* wc=(WearChange_Struct*)app->pBuffer;
					//printf("Wearchange:\n");
					//DumpPacket(app);
					if(wc->spawn_id != GetID())
						break;

					// we could maybe ignore this and just send our own from moveitem
					entity_list.QueueClients(this, app, true);
					break;
				}
				case OP_ZoneChange: {
					zoning = true;
					if (app->size != sizeof(ZoneChange_Struct)) {
						cout << "Wrong size: OP_ZoneChange, size=" << app->size << ", expected " << sizeof(ZoneChange_Struct) << endl;
						break;
					}
					
					entity_list.ClearFeignAggro(this);
					
#if EQDEBUG >= 5
					LogFile->write(EQEMuLog::Debug, "Zone request from %s", GetName());
					DumpPacket(app);
#endif
					ZoneChange_Struct* zc=(ZoneChange_Struct*)app->pBuffer;
#ifdef GUILDWARS
					if(zc->zoneID == 186)	// hateplaneb
						zc->zoneID = 76;		// hateplane
					if(Admin() == 0 && (zone->GetZoneID() == 183 || zone->GetZoneID() == 184))
						break;
#endif
					strcpy(zc->char_name, GetName()); // Stops packets from being inserted to make other clients zone					
					
					float tarx = -1, tary = -1, tarz = -1,tarheading=999;
					sint16 minstatus = 0;
					int8 minlevel = 0;
					sint8 myerror=ZONE_ERROR_NOTREADY;
					char target_zone[32] = {0};
					if (zc->zoneID != 0 && dead)
					{
#ifdef GUILDWARS
					if(animation > 65 && admin<80 && CheckCheat()){
						if(cheater || cheatcount>0){
							Message(15,"Cheater log updated...yup your busted,its not nice to cheat.");
							char descript[50]={0};
							sprintf(descript,"%s: %i","Death zone cheat");
							database.logevents(this->AccountName(),this->AccountID(),admin,this->GetName(),"none","Death zone cheat",descript,15);
							if(cheater==false){
								worldserver.SendEmoteMessage(0,0,0,13,"<Cheater Locator> We have found a cheater.  %s (Acct: %s) was just caught hacking, please show them what we think of hackers...",this->GetName(),this->AccountName());
								cheater=true;
							}
							cheatcount=0;
						}
						else
							cheatcount++;
					}
#endif
					}

					if (zc->zoneID == 0)
					{
						if(strlen(zonesummon_name)==0)//Player Died
							strcpy(target_zone, zone->GetShortName());
						else
							strcpy(target_zone, zonesummon_name);
					}
					else if (database.GetZoneName(zc->zoneID))
							strcpy(target_zone, database.GetZoneName(zc->zoneID));
					else
						target_zone[0] = 0;

					
					// this both loads the safe points and does a sanity check on zone name
					if (!database.GetSafePoints(target_zone, &tarx, &tary, &tarz, &minstatus, &minlevel))
					{
						target_zone[0] = 0;
					}
					int8 tmpzonesummon_ignorerestrictions = zonesummon_ignorerestrictions;
					if (zonesummon_ignorerestrictions)
					{
						minstatus = 0;
						minlevel = 0;
					}
					zonesummon_ignorerestrictions = 0;
					
					ZonePoint* zone_point = zone->GetClosestZonePoint(x_pos, y_pos, z_pos, zc->zoneID);
		
// vesuvias - zone processing fix
					//zone debugging
					ZonePoint* zp = zone_point;										

					tarx=zonesummon_x;
					tary=zonesummon_y;
					tarz=zonesummon_z;
					if(zp && zc->zoneID==0)
					{
					strcpy(target_zone,zp->target_zone);
					zc->zoneID = database.GetZoneID(target_zone);
					tarheading = zp->target_heading;
					}
					
					// -1, -1, -1 = code for zone safe point
					if ((x_pos == -1 && y_pos == -1 && (z_pos == -1 || z_pos == -10)) ||
						(zonesummon_x == -1 && zonesummon_y == -1 && (zonesummon_z == -1 || zonesummon_z == -10))) {
						cout << "Zoning to safe coords: " << target_zone << " (" << database.GetZoneID(target_zone) << ")" << endl;
						tarx=database.GetSafePoint(target_zone, "x");
						tary=database.GetSafePoint(target_zone, "y");
						tarz=database.GetSafePoint(target_zone, "z");
						zonesummon_x = -2;
						zonesummon_y = -2;
						zonesummon_z = -2;
					}
					// -3 -3 -3 = bind point
// vesuvias - zone processing fix
					else if (zonesummon_x == -3 && zonesummon_y == -3 && (zonesummon_z == -3 || zonesummon_z == -30)) {						
						if (database.GetZoneName(m_pp.bind_zone_id)){
							//zoneing to bind point
							strcpy(target_zone, database.GetZoneName(m_pp.bind_zone_id));
							tarx = m_pp.bind_x[0];
							tary = m_pp.bind_y[0];
							tarz = m_pp.bind_z[0];
							
						} //else bind point isn't set and we will zone to the zone safe point

						zonesummon_x = -2;
						zonesummon_y = -2;
						zonesummon_z = -2;
						minstatus = 0;
						minlevel = 0;
					}
					else if (zone_point != 0) {
						if(zone_point->target_x==999999)
							tarx=GetX();
						else
							tarx = zone_point->target_x;
						if(zone_point->target_y==999999)
							tary=GetY();
						else
							tary = zone_point->target_y;
						if(zone_point->target_z==999999)
							tarz=GetZ();
						else
							tarz = zone_point->target_z;
						tarheading = zone_point->target_heading;
						//strcpy(target_zone,zone_point->target_zone);
					}

					// if not -2 -2 -2, zone to these coords. -2, -2, -2 = not a zonesummon zonerequest
					else if (!(zonesummon_x == -2 && zonesummon_y == -2 && (zonesummon_z == -2 || zonesummon_z == -20))) {
						tarx = zonesummon_x;
						tary = zonesummon_y;
						tarz = zonesummon_z;
						zonesummon_x = -2;
						zonesummon_y = -2;
						zonesummon_z = -2;
					}
					else {
						cout << "WARNING: No target coords for this zone in DB found" << endl;
						cout << "Zoning to safe coords: " << target_zone << " (" << database.GetZoneID(target_zone) << ")" << ", x=" << tarx << ", y=" << tary << ", z=" << tarz << endl;
// vesuvias - zone processing fix
						//tarx=-1;
						//tary=-1;
						//tarz=-1;

						zonesummon_x = -2;
						zonesummon_y = -2;
						zonesummon_z = -2;
					}
					
#ifdef GUILDWARS
// image/solar: GW hack to forze dead clients into nexus on death
					if(dead && !IsBecomeNPC() && Admin() == 0)
					{
					printf("player %s appears to be dead, zoning to nexus\n", GetName());
					m_pp.zone_id = 152;
					database.MoveCharacterToZone(CharacterID(),database.GetZoneName(m_pp.zone_id));
					tarx = 10;
					tary = 10;
					tarz = -30;
					zonesummon_x = -2;
					zonesummon_y = -2;
					zonesummon_z = -2;
					strcpy(target_zone,"nexus");
					}
#endif
					if (admin < minstatus || GetLevel() < minlevel)
						myerror = ZONE_ERROR_NOEXPERIENCE;

					bool RAZone = true;
#ifdef RAIDADDICTS
					if (!raidaddicts.ZoneInCheck(zc->zoneID, this)) {
						myerror = ZONE_ERROR_NOEXPERIENCE;
						RAZone = false;
					}
#endif
					if(GetAdventureID()>0){
						AdventureInfo ai=database.GetAdventureInfo(GetAdventureID());
						if(zc->zoneID != ai.zonedungeonid && database.GetLDoNDungeon(zc->zoneID) == true){
							Message(0,"You are not allowed to enter this dungeon!");
							strcpy(target_zone,zone->GetShortName());
							tarx = GetX();
							tary = GetY();
							tarz = GetZ();
							zonesummon_x = 0;
							zonesummon_y = 0;
							zonesummon_z = 0;
						}
					}
					else if(database.GetLDoNDungeon(zc->zoneID) == true){
							Message(0,"You are not allowed to enter this dungeon!");
							strcpy(target_zone,zone->GetShortName());
							tarx = GetX();
							tary = GetY();
							tarz = GetZ();
							zonesummon_x = 0;
							zonesummon_y = 0;
							zonesummon_z = 0;
					}
					APPLAYER* outapp = NULL;
					if (target_zone[0] != 0 && admin >= minstatus && GetLevel() >= minlevel && RAZone) {
						LogFile->write(EQEMuLog::Status, "Zoning '%s' to: %s (%i) x=%f, y=%f, z=%f",
							m_pp.name, target_zone, database.GetZoneID(target_zone),
							tarx, tary, tarz);
						
						UpdateWho(1);
						
						x_pos = tarx; // Hmm, these coordinates will now be saved when ~client is called
						y_pos = tary;
						z_pos = tarz;
						if(tarheading!=999)
							m_pp.heading=tarheading;
						else
							m_pp.heading=heading;
						m_pp.zone_id = database.GetZoneID(target_zone);
						
						Save();
						
						if (m_pp.zone_id == zone->GetZoneID()) {
							// No need to ask worldserver if we're zoning to ourselves (most
							// likely to a bind point), also fixes a bug since the default response was failure
							APPLAYER* outapp = new APPLAYER(OP_ZoneChange,sizeof(ZoneChange_Struct));
							ZoneChange_Struct* zc2 = (ZoneChange_Struct*) outapp->pBuffer;
							strcpy(zc2->char_name, GetName());
							zc2->zoneID = m_pp.zone_id;
							zc2->success = 1;
							outapp->Deflate();
							outapp->priority = 6;
							QueuePacket(outapp);
							safe_delete(outapp);
							zone->StartShutdownTimer(AUTHENTICATION_TIMEOUT * 1000);
						}
						else {
						// vesuvias - zoneing to another zone so we need to the let the world server
						//handle things with the client for a while
							ServerPacket* pack = new ServerPacket(ServerOP_ZoneToZoneRequest, sizeof(ZoneToZone_Struct));
							ZoneToZone_Struct* ztz = (ZoneToZone_Struct*) pack->pBuffer;
							ztz->response = 0;
							ztz->current_zone_id = zone->GetZoneID();
							ztz->requested_zone_id = database.GetZoneID(target_zone);
							ztz->admin = admin;
							ztz->ignorerestrictions = tmpzonesummon_ignorerestrictions;
							strcpy(ztz->name, GetName());
							ztz->guild_id = GuildDBID();
							worldserver.SendPacket(pack);
							safe_delete(pack);
						}
					}
					else {
// vesuvias - zone processing fix
						LogFile->write(EQEMuLog::Error, "Zone %i is not available because target wasn't found or character insufficent level", zc->zoneID);
						
						outapp = new APPLAYER(OP_ZoneChange, sizeof(ZoneChange_Struct));
						ZoneChange_Struct *zc2 = (ZoneChange_Struct*)outapp->pBuffer;
						strcpy(zc2->char_name, zc->char_name);
						zc2->zoneID = zc->zoneID;
						zc2->success = myerror;
						outapp->priority = 6;
						QueuePacket(outapp);
						safe_delete(outapp);
						
						int8 stuffdata[16] = {0xE6, 0x02, 0x10, 0x00, 0x00, 0x00, 0x68, 0x42, 0x00, 0x00, 0xDB, 0xC3, 0xFA, 0xFE, 0x00, 0xC2};
						outapp = new APPLAYER(0x2120, sizeof(stuffdata));
						memcpy(outapp->pBuffer, stuffdata, sizeof(stuffdata));
						outapp->priority = 6;
						QueuePacket(outapp);
						safe_delete(outapp);
					}
					break;
				}
				case OP_DeleteSpawn: {
					// The client will send this with his id when he zones, maybe when he disconnects too?
					RemoveData(); // Flushing the queue of packet data to allow for proper zoning -Kasai
					
					APPLAYER* outapp = new APPLAYER(OP_DeleteSpawn, sizeof(EntityId_Struct));
					EntityId_Struct* eid = (EntityId_Struct*)outapp->pBuffer;
					eid->entity_id = GetID();
					
					entity_list.QueueClients(this, outapp, false);
					safe_delete(outapp);
					
					hate_list.RemoveEnt(this->CastToMob());

					Disconnect();
					break;
				}
				case OP_SaveOnZoneReq:
				case OP_Save: {
					// The payload is 192 bytes - Not sure what is contained in payload
					Save();
					break;
				}
				case OP_WhoAllRequest: {
					if (app->size != sizeof(Who_All_Struct)) {
						cout << "Wrong size on OP_WhoAll. Got: " << app->size << ", Expected: " << sizeof(Who_All_Struct) << endl;
						//DumpPacket(app);
						break;
					}
					Who_All_Struct* whoall = (Who_All_Struct*) app->pBuffer;
					WhoAll(whoall);
					break;
				}
				case OP_GMZoneRequest: {
					if (app->size != sizeof(GMZoneRequest_Struct)) {
						cout << "Wrong size on OP_GMZoneRequest. Got: " << app->size << ", Expected: " << sizeof(GMZoneRequest_Struct) << endl;
						break;
					}
					if (this->Admin() < 80) {
						Message(13, "Your account has been reported for hacking.");
						database.SetHackerFlag(this->account_name, this->name, "/zone");
						break;
					}
					
					GMZoneRequest_Struct* gmzr = (GMZoneRequest_Struct*)app->pBuffer;
					float tarx = -1, tary = -1, tarz = -1;
					
					sint16 minstatus = 0;
					int8 minlevel = 0;
					char tarzone[32];
					if (gmzr->zone_id == 0)
						strcpy(tarzone, zonesummon_name);
					else if (database.GetZoneName(gmzr->zone_id))
						strcpy(tarzone, database.GetZoneName(gmzr->zone_id));
					else
						tarzone[0] = 0;
					
					// this both loads the safe points and does a sanity check on zone name
					if (!database.GetSafePoints(tarzone, &tarx, &tary, &tarz, &minstatus, &minlevel)) {
						tarzone[0] = 0;
					}
					
					APPLAYER* outapp = new APPLAYER(OP_GMZoneRequest, sizeof(GMZoneRequest_Struct));
					GMZoneRequest_Struct* gmzr2 = (GMZoneRequest_Struct*) outapp->pBuffer;
					strcpy(gmzr2->charname, this->GetName());
					gmzr2->zone_id = gmzr->zone_id;
					gmzr2->x = tarx;
					gmzr2->y = tary;
					gmzr2->z = tarz;
					// Next line stolen from ZoneChange as well... - This gives us a nicer message than the normal "zone is down" message...
					if (tarzone[0] != 0 && admin >= minstatus && GetLevel() >= minlevel)
						gmzr2->success = 1;
					else {
						cout << "GetZoneSafeCoords failed. zoneid = " << gmzr->zone_id << "; czone = " << zone->GetZoneID() << endl;
						gmzr2->success = 0;
					}
					
					QueuePacket(outapp);
					safe_delete(outapp);
					break;
				}
				case OP_GMZoneRequest2: {
					int32 zonereq = (int32)*app->pBuffer;
					if(zonereq == zone->GetZoneID())
						this->MovePC(zonereq, zone->safe_x(), zone->safe_y(), zone->safe_z(), 0, false);
					else
						this->MovePC(zonereq, -1, -1, -1, 0, false);
					break;
				}
				case OP_EndLootRequest: {
					if (app->size != sizeof(int32)) {
						cout << "Wrong size: OP_EndLootRequest, size=" << app->size << ", expected " << sizeof(int32) << endl;
						break;
					}
					
					Entity* entity = entity_list.GetID(*((int16*)app->pBuffer));
					if (entity == 0) {
						//DumpPacket(app);
						Message(13, "Error: OP_EndLootRequest: Corpse not found (ent = 0)");
						Corpse::SendLootReqErrorPacket(this);
						break;
					}
					else if (!entity->IsCorpse()) {
						Message(13, "Error: OP_EndLootRequest: Corpse not found (!entity->IsCorpse())");
						Corpse::SendLootReqErrorPacket(this);
						break;
					}
					else {
						entity->CastToCorpse()->EndLoot(this, app);
					}
					break;
				}
				case OP_LootRequest: {
					if (app->size != sizeof(int32)) {
						cout << "Wrong size: OP_LootRequest, size=" << app->size << ", expected " << sizeof(int32) << endl;
						break;
					}
					
					Entity* ent = entity_list.GetID(*((int32*)app->pBuffer));
					if (ent == 0) {
						Message(13, "Error: OP_LootRequest: Corpse not found (ent = 0)");
						Corpse::SendLootReqErrorPacket(this);
						break;
					}
					if (ent->IsCorpse()) {
						ent->CastToCorpse()->MakeLootRequestPackets(this, app);
						break;
					}
					else {
						cout << "npc == 0 LOOTING FOOKED3" << endl;
						Message(13, "Error: OP_LootRequest: Corpse not a corpse?");
						Corpse::SendLootReqErrorPacket(this);
					}
					break;
				}
				case OP_Dye:{
					if(app->size!=sizeof(DyeStruct))
						printf("Wrong size of DyeStruct, Got: %i, Expected: %i\n",app->size,sizeof(DyeStruct));
					else{
						DyeStruct* dye = (DyeStruct*)app->pBuffer;
						DyeArmor(dye);
					}
					break;
				}
				case OP_LootItem: {
					if (app->size != sizeof(LootingItem_Struct)) {
						LogFile->write(EQEMuLog::Error, "Wrong size: OP_LootItem, size=%i, expected %i", app->size, sizeof(LootingItem_Struct));
						break;
					}
					/*
					** Disgrace:
					**	fixed the looting code so that it sends the correct opcodes
					**	and now correctly removes the looted item the player selected
					**	as well as gives the player the proper item.
					**	Also fixed a few UI lock ups that would occur.
					*/
					
					APPLAYER* outapp = 0;
					Entity* entity = entity_list.GetID(*((int16*)app->pBuffer));
					if (entity == 0) {
						Message(13, "Error: OP_LootItem: Corpse not found (ent = 0)");
						outapp = new APPLAYER(OP_LootComplete, 0);
						QueuePacket(outapp);
						safe_delete(outapp);
						break;
					}
					
					if (entity->IsCorpse()) {
						entity->CastToCorpse()->LootItem(this, app);
						break;
					}
					else {
						Message(13, "Error: Corpse not found! (!ent->IsCorpse())");
						Corpse::SendEndLootErrorPacket(this);
					}
					
					break;
				}
				case OP_GuildDelete:{
					if(GuildRank() != 2 || GuildDBID() == 0)
						Message(0,"You are not a guild leader or not in a guild.");
					else{
						if (!database.DeleteGuild(guilddbid))
							Message(0, "Guild delete failed.");
						else {
							int32 tmpeq = database.GetGuildEQID(guilddbid);
							if (tmpeq != GUILD_NONE) {
								ServerPacket* pack = new ServerPacket(ServerOP_RefreshGuild,5);
								memcpy(pack->pBuffer, &tmpeq, 4);
								pack->pBuffer[4] = 1;
								worldserver.SendPacket(pack);
								safe_delete(pack);
							}
							Message(0, "Guild successfully deleted.");
						}
					}
					break;
				}
				case OP_GuildPublicNote:{
					GuildUpdate_PublicNote* gpn=(GuildUpdate_PublicNote*)app->pBuffer;
					database.SetPublicNote(guilddbid,gpn->target,gpn->note);
					break;
				}
				case OP_GetGuildMOTD:
				case OP_GuildMOTD: {
					char tmp[600]={0};
					if (app->size !=sizeof(GuildMOTD_Struct)) {
						// client calls for a motd on login even if they arent in a guild
						printf("Error: app size of %i != size of GuildMOTD_Struct of %i\n",app->size,sizeof(GuildMOTD_Struct));
						break;
					}
					GuildMOTD_Struct* gmotd=(GuildMOTD_Struct*)app->pBuffer;
					if ((GuildRank()>0) && (strlen(gmotd->motd)>0)) {
						sprintf(tmp,"%s - %s",gmotd->name,gmotd->motd);
						if (!database.SetGuildMOTD(guilddbid, tmp)) {
							Message(0, "Motd update failed.");
						}
						worldserver.SendEmoteMessage(0, guilddbid, MT_Guild, "Guild MOTD: %s", tmp);
					}
					else {
						strcpy(tmp,database.GetGuildMOTD(guilddbid));
						if (strlen(tmp) != 0)
							Message_StringID(MT_Guild,GENERIC_STRING,tmp);
							//Message(MT_Guild, "Guild MOTD: %s", tmp);
						//Message(MT_Guild, "Guild MOTD Disabled for now");
					}
					
					break;
				}
				case OP_GuildPeace: {
					break;
				}
				case OP_GuildWar: {
					break;
				}
				case OP_GuildLeader: {
					if (app->size <= 1)
						break;
					app->pBuffer[app->size-1] = 0;
					GuildMakeLeader* gml=(GuildMakeLeader*)app->pBuffer;
					if (guilddbid == 0)
						Message(0, "Error: You arent in a guild!");
					else if (GuildRank()!=2)
						Message(0, "Error: You arent the guild leader!");
					else if (!worldserver.Connected())
						Message(0, "Error: World server disconnected");
					else {
						Client* newleader=entity_list.GetClientByName(gml->target);
						if(newleader){
							if(database.SetGuildLeader(guilddbid,newleader->AccountID())){
								newleader->GuildChangeRank(GuildEQID(),newleader->GuildRank(),2);
								newleader->SetGuild(guilddbid,2);
								GuildChangeRank(GuildEQID(),2,1);
								SetGuild(guilddbid,1);
								Message(0,"Successfully Transfered Leadership to %s.",gml->target);
								newleader->Message(15,"%s has transfered the guild leadership into your hands.",GetName());
							}
							else
								Message(0,"Could not change leadership at this time.");
						}
						else
							Message(0,"Failed to change leader, could not find target.");
					}
					break;
				}
				case OP_GuildDemote:{
					if(app->size==sizeof(GuildDemoteStruct)){
						GuildDemoteStruct* demote = (GuildDemoteStruct*)app->pBuffer;
						Client* client=entity_list.GetClientByName(demote->target);
						if(client){
							client->GuildChangeRank(GuildEQID(),1,0);
							client->SetGuild(guilddbid,0);
						}
						else{
							GuildChangeRank(demote->target,GuildEQID(),1,0);
							database.SetGuild(demote->target,guilddbid,0);
						}
						Message(0,"Successfully demoted %s.",demote->target);
					}
					break;
				}
				case OP_GuildInvite: {
					if (app->size != sizeof(GuildCommand_Struct)) {
						cout << "Wrong size: OP_GuildInvite, size=" << app->size << ", expected " << sizeof(GuildCommand_Struct) << endl;
						break;
					}
					if (guilddbid == 0)
						Message(0, "Error: You arent in a guild!");
					else if (GuildRank()==0)
						Message(0, "You dont have permission to invite.");
					else if (!worldserver.Connected())
						Message(0, "Error: World server disconnected");
					else {
						#ifdef GUILDWARS
							if (database.NumberInGuild(guilddbid)>MAXMEMBERS){
								Message(15,"Your Guild has reached its Guildwars size limit.  You cannot invite any more people.");
								break;
							}
						#endif
						
						GuildCommand_Struct* gc = (GuildCommand_Struct*) app->pBuffer;
						Client* client=entity_list.GetClientByName(gc->othername);
						if(client){
							if(client->GuildDBID() != 0 && (client->GuildDBID() != GuildDBID()))
							{
								Message(0,"Player is in a guild.");
								break;
							}
							if(gc->guildeqid==0)
								gc->guildeqid=GuildEQID();
							client->QueuePacket(app);
						}
						else
							Message(0,"You must be in the same zone as the person you are inviting.");
					}
					break;
				}
				case OP_GuildRemove: {
					if (app->size != sizeof(GuildCommand_Struct)) {
						cout << "Wrong size: OP_GuildRemove, size=" << app->size << ", expected " << sizeof(GuildCommand_Struct) << endl;
						break;
					}
					GuildCommand_Struct* gc = (GuildCommand_Struct*) app->pBuffer;
					if (guilddbid == 0)
						Message(0, "Error: You arent in a guild!");
					else if (GuildRank()==0 && strcasecmp(gc->othername,GetName()))
						Message(0, "You dont have permission to remove.");
					else if (!worldserver.Connected())
						Message(0, "Error: World server disconnected");
					else {
						Client* client=entity_list.GetClientByName(gc->othername);
						if(client && (client->GuildDBID() != GuildDBID()))
						{
							Message(0,"You aren't in the same guild, what do you think you are doing?");
							break;
						}
						if(database.SetGuild(gc->othername,0,0) || client){
							APPLAYER* outapp = new APPLAYER(OP_GuildManageRemove,sizeof(GuildManageRemove_Struct));
							GuildManageRemove_Struct* gm=(GuildManageRemove_Struct*)outapp->pBuffer;
							gm->guildeqid=GuildEQID();
							strcpy(gm->member,gc->othername);
							if(client)
								client->SetGuild(0,0);
							Message(0,"%s successfully removed from your guild.",gc->othername);
							entity_list.QueueClientsGuild(this,outapp,false,GuildEQID());
							safe_delete(outapp);
						}
						else
							Message(0,"Unable to remove %s from your guild.",gc->othername);
					}
					break;
				}
				case OP_GuildInviteAccept: {
					if (app->size != sizeof(GuildInviteAccept_Struct)) {
						cout << "Wrong size: OP_GuildInviteAccept, size=" << app->size << ", expected " << sizeof(GuildJoin_Struct) << endl;
						break;
					}
					GuildInviteAccept_Struct* gj = (GuildInviteAccept_Struct*) app->pBuffer;
					if (gj->response == 5 || gj->response == 4) {
						worldserver.SendEmoteMessage(gj->inviter, 0, 0, "%s has declined to join the guild.", this->GetName());
					}
					else{
						//int32 tmpeq = gj->guildeqid;
						if (guilddbid != 0 && gj->response==GuildRank())
							Message(0, "Error: You're already in a guild!");
						else if (!worldserver.Connected())
							Message(0, "Error: World server disconnected");
						else {
							if(gj->guildeqid==GuildEQID()){//already in guild, need to promote or demote
								/*APPLAYER* outapp = new APPLAYER(OP_GuildManageRemove,sizeof(GuildManageRemove_Struct));
								GuildManageRemove_Struct* gm=(GuildManageRemove_Struct*)outapp->pBuffer;
								gm->guildeqid=GuildEQID();
								strcpy(gm->member,GetName());
								entity_list.QueueClientsGuild(this,outapp,false,GuildEQID());
								safe_delete(outapp);*/
								if(gj->response<2){
									GuildChangeRank(GuildEQID(),0,1);
									SetGuild(guilddbid,1);
								}
								else{
									GuildChangeRank(GuildEQID(),1,0);
									SetGuild(guilddbid,0);
								}
								break;
							}
							GuildJoin_Struct* gj2= new GuildJoin_Struct;
							gj2->class_=GetClass();
							gj2->guildid=gj->guildeqid;
							gj2->level=GetLevel();
							strcpy(gj2->name,GetName());
							gj2->rank=gj->response;
							gj2->zoneid=zone->GetZoneID();
							if(gj->response>1)
								SetGuild(guilds[gj->guildeqid].databaseID,0);
							else
								SetGuild(guilds[gj->guildeqid].databaseID,gj->response);
							worldserver.SendGuildJoin(gj2);
							SendGuildMembers(guilds[gj->guildeqid].databaseID);
							safe_delete(gj2);
						}
					}
					break;
				}
				case OP_ManaChange: {
					if(app->size == 0) {
						// i think thats the sign to stop the songs
						InterruptSpell(SONG_ENDS, 0x121);
						break;
					}
					else	// solar: i don't think the client sends proper manachanges
					{			// with a length, just the 0 len ones for stopping songs
						//ManaChange_Struct* p = (ManaChange_Struct*)app->pBuffer;
						printf("OP_ManaChange from client:\n");
						DumpPacket(app);
					}
					break;
				}
				case OP_MemorizeSpell: {
					OPMemorizeSpell(app);
					break;
				}
				case OP_SwapSpell: {
					if (app->size != sizeof(SwapSpell_Struct)) {
						cout << "Wrong size on OP_SwapSpell. Got: " << app->size << ", Expected: " << sizeof(SwapSpell_Struct) << endl;
						break;
					}
					const SwapSpell_Struct* swapspell = (const SwapSpell_Struct*) app->pBuffer;
					int swapspelltemp;

					if(swapspell->from_slot < 0 || swapspell->from_slot > MAX_PP_SPELLBOOK || swapspell->to_slot < 0 || swapspell->to_slot > MAX_PP_SPELLBOOK)
						break;
					
					swapspelltemp = m_pp.spell_book[swapspell->from_slot];
					m_pp.spell_book[swapspell->from_slot] = m_pp.spell_book[swapspell->to_slot];
					m_pp.spell_book[swapspell->to_slot] = swapspelltemp;
					
					QueuePacket(app);
					break;
				}
				case OP_CastSpell: {
					if (app->size != sizeof(CastSpell_Struct)) {
						cout << "Wrong size: OP_CastSpell, size=" << app->size << ", expected " << sizeof(CastSpell_Struct) << endl;
						break;
					}
					if (IsAIControlled()) {
						this->Message_StringID(13,NOT_IN_CONTROL);
						//Message(13, "You cant cast right now, you arent in control of yourself!");
						break;
					}
					
					CastSpell_Struct* castspell = (CastSpell_Struct*)app->pBuffer;

#ifdef _EQDEBUG
						LogFile->write(EQEMuLog::Debug, "cs_unknown2: %u %i", (uint8)castspell->cs_unknown[0], castspell->cs_unknown[0]);
						LogFile->write(EQEMuLog::Debug, "cs_unknown2: %u %i", (uint8)castspell->cs_unknown[1], castspell->cs_unknown[1]);
						LogFile->write(EQEMuLog::Debug, "cs_unknown2: %u %i", (uint8)castspell->cs_unknown[2], castspell->cs_unknown[2]);
						LogFile->write(EQEMuLog::Debug, "cs_unknown2: %u %i", (uint8)castspell->cs_unknown[3], castspell->cs_unknown[3]);
						LogFile->write(EQEMuLog::Debug, "cs_unknown2: 32 %p %u", &castspell->cs_unknown, *(uint32*) castspell->cs_unknown );
						LogFile->write(EQEMuLog::Debug, "cs_unknown2: 32 %p %i", &castspell->cs_unknown, *(int32*) castspell->cs_unknown );
						LogFile->write(EQEMuLog::Debug, "cs_unknown2: 16 %p %u %u", &castspell->cs_unknown, *(uint16*) castspell->cs_unknown, *(uint16*) castspell->cs_unknown+sizeof(uint16) );
						LogFile->write(EQEMuLog::Debug, "cs_unknown2: 16 %p %i %i", &castspell->cs_unknown, *(int16*) castspell->cs_unknown, *(int16*) castspell->cs_unknown+sizeof(int16) );
#endif
LogFile->write(EQEMuLog::Debug, "OP CastSpell: slot=%d, spell=%d, target=%d", castspell->slot, castspell->spell_id, castspell->target_id);

					if (castspell->slot == 10)	// this means item
					{
						if (castspell->inventoryslot < 30)	// sanity check
						{
							const ItemInst* inst = m_inv[castspell->inventoryslot]; //@merth: slot values are sint16, need to check packet on this field
							//bool cancast = true;
							if (inst && inst->IsType(ItemTypeCommon))
							{
								const Item_Struct* item = inst->GetItem();
								if(item->Common.SpellId != (sint32)castspell->spell_id)
								{
									InterruptSpell(castspell->spell_id);	//CHEATER!!
									break;
								}
								DeleteItemInInventory(castspell->inventoryslot,1,true);
								if ((item->Common.EffectType == 1) || (item->Common.EffectType == 3) || (item->Common.EffectType == 4) || (item->Common.EffectType == 5))
								{
									CastSpell(item->Common.SpellId, castspell->target_id, castspell->slot, item->Common.CastTime, 0, 0, castspell->inventoryslot);
								}
								else
								{
									Message(0, "Error: unknown item->Common.EffectType (0x%02x)", item->Common.EffectType);
								}
							}
							else
							{
								Message(0, "Error: item not found for inventory slot #%i", castspell->inventoryslot);
								InterruptSpell(castspell->spell_id);
							}
						}
						else
						{
							Message(0, "Error: castspell->inventoryslot >= 30 (0x%04x)", castspell->inventoryslot);
							InterruptSpell(castspell->spell_id);
						}
					}
					else	// ability, or regular memmed spell
					{
						int16 spell_to_cast = 0;
						
						//current client seems to send LH in slot 8 now...
						if(castspell->slot == ABILITY_SPELL_SLOT &&
							castspell->spell_id == SPELL_LAY_ON_HANDS && GetClass() == PALADIN) {
							if(!p_timers.Expired(pTimerLayHands)) {
								Message(13,"Ability recovery time not yet met.");
								break;
							}
							spell_to_cast = SPELL_LAY_ON_HANDS;
							p_timers.Start(pTimerLayHands, LayOnHandsReuseTime);
							//database.UpdateAATimers(CharacterID(),LayOnHandsReuseTime,0, 87);//72 minutes
							AbilityTimer=true;
							
						} else if(castspell->slot == ABILITY_SPELL_SLOT &&
							(castspell->spell_id == SPELL_HARM_TOUCH
								|| castspell->spell_id == SPELL_HARM_TOUCH2
							) && GetClass() == SHADOWKNIGHT) {
							
							if(!p_timers.Expired(pTimerHarmTouch)) {
								Message(13,"Ability recovery time not yet met.");
								break;
							}
							
							if(GetLevel() < 40)
								spell_to_cast = SPELL_HARM_TOUCH;
							else
								spell_to_cast = SPELL_HARM_TOUCH2;
							p_timers.Start(pTimerHarmTouch, HarmTouchReuseTime);
							//database.UpdateAATimers(CharacterID(),HarmTouchReuseTime,0, 89);//72 minutes
							AbilityTimer=true;
						}
						
						//handle disciplines
						if(castspell->slot == DISCIPLINE_SPELL_SLOT) {
							if(!UseDiscipline(castspell->spell_id, castspell->target_id)) {
								printf("Unknown ability being used by %s, spell being cast is: %i\n",GetName(),castspell->spell_id);
								InterruptSpell(castspell->spell_id);
							}
							break;
						}
						
						if(castspell->slot < MAX_PP_MEMSPELL)
						{
							spell_to_cast = m_pp.mem_spells[castspell->slot];
							if(spell_to_cast != castspell->spell_id)
							{
								InterruptSpell(castspell->spell_id); //CHEATER!!!
								break;
							}
						}
						/*
						these are coming through with slot 8 now...
						else if(castspell->slot == 9)	//discipline, LoH, HT, etc
						{
							if(GetClass() == PALADIN && castspell->spell_id == SPELL_LAY_ON_HANDS)
							{
								spell_to_cast = SPELL_LAY_ON_HANDS;
								p_timers.Start(pTimerLayHands, LayOnHandsReuseTime);
								CastSpell(spell_to_cast, castspell->target_id, castspell->slot);
							}
							else if(GetClass() == SHADOWKNIGHT
								&& (castspell->spell_id == SPELL_HARM_TOUCH || castspell->spell_id == SPELL_HARM_TOUCH2))
							{
								if(GetLevel() < 40)
									spell_to_cast = SPELL_HARM_TOUCH;
								else
									spell_to_cast = SPELL_HARM_TOUCH2;
								p_timers.Start(pTimerHarmTouch, HarmTouchReuseTime);
							}
							else*/
							//try disciplines
						AbilityTimer=true;
						
						CastSpell(spell_to_cast, castspell->target_id, castspell->slot);
					}
					break;
				}
				case OP_ConsumeAmmo: {
					if (app->size != sizeof(CombatAbility_Struct)) {
						cout << "Wrong size on OP_ConsumeAmmo. Got: " << app->size << ", Expected: " << sizeof(CombatAbility_Struct) << endl;
						break;
					}
					
					// @merth: Need to figure out which slot to delete from .. or maybe m_id is the slot?
					CombatAbility_Struct* ca_atk = (CombatAbility_Struct*) app->pBuffer;
 					const ItemInst *inst = GetInv().GetItem(ca_atk->m_id);
					if (inst && inst->GetItem()->Common.ItemUse == ItemUseAlcohol) {
						//TODO: grant alcohol bonuses..?
						CheckIncreaseSkill(ALCOHOL_TOLERANCE,200);
					}
					DeleteItemInInventory(ca_atk->m_id, 1);
					
					break;
				}
				case OP_CombatAbility: {
					if (app->size != sizeof(CombatAbility_Struct)) {
						cout << "Wrong size on OP_CombatAbility. Got: " << app->size << ", Expected: " << sizeof(CombatAbility_Struct) << endl;
						break;
					}		
					OPCombatAbility(app);			
					break;
				}
				case OP_Taunt: {
					if (app->size != sizeof(ClientTarget_Struct)) {
						cout << "Wrong size on OP_Taunt. Got: " << app->size << ", Expected: "<< sizeof(ClientTarget_Struct) << endl;
						break;
					}
					
					if(!p_timers.Expired(pTimerTaunt, false)) {
								Message(13,"Ability recovery time not yet met.");
								break;
					}
					p_timers.Start(pTimerTaunt, TauntReuseTime-1);
					
					if(!GetTarget()->IsNPC())
						break;
					
					Taunt(GetTarget()->CastToNPC(), false);
					break;
				}
				case OP_InstillDoubt: {
					//FIXME: Struct is wrong as of 2/25/04 --Shawn319
					/*if(app->size != sizeof(Instill_Doubt_Struct))
					{
						cout << "Wrong size on OP_InstillDoubt. Got: " << app->size << ", Expected: " << sizeof(Instill_Doubt_Struct) << endl;
						break;
					}
					//Fear Spell not yet implemented
					Instill_Doubt_Struct* iatk = (Instill_Doubt_Struct*) app->pBuffer;
					if (iatk->i_atk == 0x2E) {
						Message_StringID(4,NOT_SCARING);
						//Message(4, "You\'re not scaring anyone.");
					}*/
					
					if (!target || !(target->IsNPC() || target->IsClient()) || !CombatRange(target))
						break;
					
					if(!p_timers.Expired(pTimerInstillDoubt, false)) {
								Message(13,"Ability recovery time not yet met.");
								break;
					}
					p_timers.Start(pTimerInstillDoubt, InstillDoubtReuseTime-1);
					CheckIncreaseSkill(INTIMIDATION);
						break;
					if ((rand()%100 + GetSkill(INTIMIDATION) + GetCHA()/2) >= (uint32)(target->GetLevel()*4 + target->GetWIS()/2)) {
						//cast fear on them... should prolly be a different spell
						//and should be un-resistable.
						SpellOnTarget(229, target);
						//is there a success message?
					} else {
						Message_StringID(4,NOT_SCARING);
						//Idea from WR:
						/* if (target->IsNPC() && MakeRandomInt(0,99) < 10 ) {
							entity_list.MessageClose(target, false, 50, MT_Rampage, "%s lashes out in anger!",target->GetName());
							//should we actually do this? and the range is completely made up, unconfirmed
							entity_list.AEAttack(target, 50);
						}*/
					}
					break;
				}
				case OP_RezzAnswer: {
					OPRezzAnswer(app);
					break;
				}
				case OP_GMSummon: { // and /corpse
					if (app->size != sizeof(GMSummon_Struct)) {
						cout << "Wrong size on OP_GMSummon. Got: " << app->size << ", Expected: " << sizeof(GMSummon_Struct) << endl;
						break;
					}
					OPGMSummon(app);
					break;
				}
				case OP_TradeRequest: {
					// Client requesting a trade session from an npc/client
					// Trade session not started until OP_TradeRequestAck is sent
					TradeRequest_Struct* msg = (TradeRequest_Struct*) app->pBuffer;
					trade->Start(msg->to_mob_id);
					
					BreakInvis();
					
					// Pass trade request on to recipient
					Mob* with = trade->With();
					if (with && with->IsClient()) {
						with->CastToClient()->QueuePacket(app);
					}
					else if (with) {
						//npcs always accept
						APPLAYER* outapp = new APPLAYER(OP_TradeRequestAck, sizeof(TradeRequest_Struct));
						TradeRequest_Struct* acc = (TradeRequest_Struct*) outapp->pBuffer;
						acc->from_mob_id = msg->to_mob_id;
						acc->to_mob_id = msg->from_mob_id;
						FastQueuePacket(&outapp);
						safe_delete(outapp);
					}
					break;
				}
				case OP_TradeRequestAck: {
					// Trade request recipient is acknowledging they are able to trade
					// After this, the trade session has officially started
					//TradeRequest_Struct* msg = (TradeRequest_Struct*) app->pBuffer;
					
					// Send ack on to trade initiator if client
					Mob* with = trade->With();
					if (with->IsClient()) {
						with->CastToClient()->QueuePacket(app);
					}
					break;
				}
				case OP_CancelTrade: {
					Mob* with = trade->With();
					if (with && with->IsClient()) {
						CancelTrade_Struct* msg = (CancelTrade_Struct*) app->pBuffer;
						
						// Forward cancel packet to other client
						msg->fromid = with->GetID();
						//msg->action = 1;
	
						with->CastToClient()->QueuePacket(app);
						
						// Put trade items/cash back into inventory
						FinishTrade(this);
						trade->Reset();
					}
					else if(with){
						CancelTrade_Struct* msg = (CancelTrade_Struct*) app->pBuffer;
						msg->fromid = with->GetID();
						QueuePacket(app);
						FinishTrade(this);
						trade->Reset();
					}
					break;
				}
				case OP_TradeAcceptClick: {
					Mob* with = trade->With();
					trade->state = TradeAccepted;
					if (with && with->IsClient()) {
						// Have both accepted?
						Client* other = with->CastToClient();
						other->QueuePacket(app);
						
						if (other->trade->state == trade->state) {
							other->trade->state = TradeCompleting;
							trade->state = TradeCompleting;
							
							if (CheckTradeLoreConflict(other) || other->CheckTradeLoreConflict(this))
							{
								Message_StringID(13,104);
								other->Message_StringID(13,104);
								this->FinishTrade(this);
								other->FinishTrade(other);
								other->trade->Reset();
								trade->Reset();
							} else {
								// Audit trade to database for both trade streams
								other->trade->LogTrade();
								trade->LogTrade();
							
								// Perform actual trade
								this->FinishTrade(other);
								other->FinishTrade(this);
								other->trade->Reset();
								trade->Reset();
							}
							// All done
							APPLAYER* outapp = new APPLAYER(OP_FinishTrade, 0);
							other->QueuePacket(outapp);
							this->FastQueuePacket(&outapp);
						}
					}
					else if(with){
						APPLAYER* outapp = new APPLAYER(OP_FinishTrade,0);
						QueuePacket(outapp);
						safe_delete(outapp);
						FinishTrade(with->CastToNPC());						
					}
					
					break;
				}
				case OP_BoardBoat: {
					char *boatname;
					this->IsOnBoat=true;
					boatname = new char[app->size-4];
					memset(boatname, 0, app->size-4);
					memcpy(boatname, app->pBuffer, app->size-4);
					printf("%s has gotten on the boat %s\n",GetName(),boatname);
					Mob* boat = entity_list.GetMob(boatname);
					if (boat){
						boat->CastToNPC()->passengers=true;
					}
					safe_delete(boatname);
					break;
				}
				case OP_LeaveBoat: {
					this->IsOnBoat=false;
					break;
				}
				case OP_RandomReq: {
					const RandomReq_Struct* rndq = (const RandomReq_Struct*) app->pBuffer;
					uint32 randLow=rndq->low > rndq->high?rndq->high:rndq->low;
					uint32 randHigh=rndq->low > rndq->high?rndq->low:rndq->high;
					uint32 randResult;

					if(randLow==0 && randHigh==0)
					{	// defaults
						randLow=0;
						randHigh=100;
					}
					randResult=MakeRandomInt(randLow, randHigh);

					APPLAYER* outapp = new APPLAYER(OP_RandomReply, sizeof(RandomReply_Struct));
					RandomReply_Struct* rr = (RandomReply_Struct*)outapp->pBuffer;
					rr->low=randLow;
					rr->high=randHigh;
					rr->result=randResult;
					strcpy(rr->name, GetName());
					entity_list.QueueCloseClients(this, outapp, false, 400);
					safe_delete(outapp);
					break;
				}
				case OP_Buff: {
					if (app->size != sizeof(SpellBuffFade_Struct))
					{
						LogFile->write(EQEMuLog::Error, "Size mismatch in OP_Buff. expected %i got %i", sizeof(SpellBuffFade_Struct), app->size);
						DumpPacket(app);
						break;
					}
					
					SpellBuffFade_Struct* sbf = (SpellBuffFade_Struct*) app->pBuffer;
					if(sbf->spellid == 0xFFFF)
						QueuePacket(app);
					else
						BuffFadeBySpellID(sbf->spellid);

					break;
				}
				case OP_GMHideMe: {
					int reqlevel = database.CommandRequirement("!gm");
					reqlevel = reqlevel == 255 ? 80 : reqlevel;
					if(this->Admin() < reqlevel) {
						Message(13, "Your account has been reported for hacking.");
						database.SetHackerFlag(this->account_name, this->name, "/hideme");
						break;
					}
					SpawnAppearance_Struct* sa = (SpawnAppearance_Struct*)app->pBuffer;
					SetHideMe(sa->parameter == 1);
					break;
				}

				case OP_GMNameChange: {
					const GMName_Struct* gmn = (const GMName_Struct *)app->pBuffer;
					if(this->Admin() < 100){
						Message(13, "Your account has been reported for hacking.");
						database.SetHackerFlag(this->account_name, this->name, "/name");
						break;
					}
					Client* client = entity_list.GetClientByName(gmn->oldname);
					LogFile->write(EQEMuLog::Status, "GM(%s) changeing players name. Old:%s New:%s", GetName(), gmn->oldname, gmn->newname);
					bool usedname = database.CheckUsedName((const char*) gmn->newname);
					if(client==0) {
						Message(13, "%s not found for name change. Operation failed!", gmn->oldname);
						break;
					}
					if((strlen(gmn->newname) > 63) || (strlen(gmn->newname) == 0)) {
						Message(13, "Invalid number of characters in new name (%s).", gmn->newname);
						break;
					}
					if (!usedname) {
						Message(13, "%s is already in use.  Operation failed!", gmn->newname);
						break;

					}
					database.UpdateName(gmn->oldname, gmn->newname);
					strcpy(client->name, gmn->newname);
					client->Save();
					
					if(gmn->badname==1) {
						database.AddToNameFilter(gmn->oldname);
					}
					APPLAYER* outapp = app->Copy();
					GMName_Struct* gmn2 = (GMName_Struct*) outapp->pBuffer;
					gmn2->unknown[0] = 1;
					gmn2->unknown[1] = 1;
					gmn2->unknown[2] = 1;
					entity_list.QueueClients(this, outapp, false);
					safe_delete(outapp);
					UpdateWho();
					break;
				}
				case OP_GMKill: {
					if(this->Admin() < 100) {
						Message(13, "Your account has been reported for hacking.");
						database.SetHackerFlag(this->account_name, this->name, "/kill");
						break;
					}
					GMKill_Struct* gmk = (GMKill_Struct *)app->pBuffer;
					Mob* obj = entity_list.GetMob(gmk->name);
					Client* client = entity_list.GetClientByName(gmk->name);
					if(obj!=0) {
						if(client!=0) {
							entity_list.QueueClients(this,app);
						}
						else {
							obj->Kill();
						}
					}
					else {
						if (!worldserver.Connected())
							Message(0, "Error: World server disconnected");
						else {
							ServerPacket* pack = new ServerPacket;
							pack->opcode = ServerOP_KillPlayer;
							pack->size = sizeof(ServerKillPlayer_Struct);
							pack->pBuffer = new uchar[pack->size];
							ServerKillPlayer_Struct* skp = (ServerKillPlayer_Struct*) pack->pBuffer;
							strcpy(skp->gmname, gmk->gmname);
							strcpy(skp->target, gmk->name);
							skp->admin = this->Admin();
							worldserver.SendPacket(pack);
							safe_delete(pack);
						}
					}
					break;
				}
				case OP_GMLastName: {
					if (app->size != sizeof(GMLastName_Struct)) {
						cout << "Wrong size on OP_GMLastName. Got: " << app->size << ", Expected: " << sizeof(GMLastName_Struct) << endl;
						break;
					}
					GMLastName_Struct* gmln = (GMLastName_Struct*) app->pBuffer;
					if (strlen(gmln->lastname) >= 64) {
						Message(13, "/LastName: New last name too long. (max=63)");
					}
					else {
						Client* client = entity_list.GetClientByName(gmln->name);
						if (client == 0) {
							Message(13, "/LastName: %s not found", gmln->name);
						}
						else {
							if (this->Admin() < 80) {
								Message(13, "Your account has been reported for hacking.");
								database.SetHackerFlag(client->account_name, client->name, "/lastname");
								break;
							}
							else

								client->ChangeLastName(gmln->lastname);
						}
						gmln->unknown[0] = 1;
						gmln->unknown[1] = 1;
						gmln->unknown[2] = 1;
						gmln->unknown[3] = 1;
						entity_list.QueueClients(this, app, false);
					}
					break;
				}
				case OP_GMToggle: {
					if (app->size != 68) {
						cout << "Wrong size on OP_GMToggle. Got: " << app->size << ", Expected: " << 36 << endl;
						break;
					}
					if (this->Admin() < 80) {
						Message(13, "Your account has been reported for hacking.");
						database.SetHackerFlag(this->account_name, this->name, "/toggle");
						break;
					}
					if (app->pBuffer[64] == 0) {
						this->Message_StringID(0,TOGGLE_OFF);
						//Message(0, "Turning tells OFF");
						tellsoff = true;
					}
					else if (app->pBuffer[64] == 1) {
						//Message(0, "Turning tells ON");
						this->Message_StringID(0,TOGGLE_ON);
						tellsoff = false;
					}
					else {
						Message(0, "Unkown value in /toggle packet");
					}
					UpdateWho();
					break;
				}
				case OP_LFGCommand: {
					if (app->size != sizeof(LFG_Struct)) {
						cout << "Wrong size on OP_LFGCommand. Got: " << app->size << ", Expected: " << sizeof(LFG_Struct) << endl;
						DumpPacket(app);
						break;
					}
					
					// Process incoming packet
					LFG_Struct* lfg = (LFG_Struct*) app->pBuffer;
					if (lfg->value == 1) {
						LFG = true;
					}
					else if (lfg->value == 0) {
						LFG = false;
					}
					else
						Message(0, "Error: unknown LFG value");
					UpdateWho();
					
					// Issue outgoing packet to notify other clients
					APPLAYER* outapp = new APPLAYER(OP_LFGAppearance, sizeof(LFG_Appearance_Struct));
					LFG_Appearance_Struct* lfga = (LFG_Appearance_Struct*)outapp->pBuffer;
					lfga->spawn_id = this->GetID();
					lfga->lfg = (uint8)LFG;
					
					entity_list.QueueClients(this, outapp, true);
					safe_delete(outapp);
					break;
				}
				case OP_GMGoto: {
					if (app->size != sizeof(GMSummon_Struct)) {
						cout << "Wrong size on OP_GMGoto. Got: " << app->size << ", Expected: " << sizeof(GMSummon_Struct) << endl;
						break;
					}
					if (this->Admin() < 80) {
						Message(13, "Your account has been reported for hacking.");
						database.SetHackerFlag(this->account_name, this->name, "/goto");
						break;
					}
					GMSummon_Struct* gmg = (GMSummon_Struct*) app->pBuffer;
					Mob* gt = entity_list.GetMob(gmg->charname);
					if (gt != 0) {
						this->MovePC((char*) 0, gt->GetX(), gt->GetY(), gt->GetZ());
					}
					else if (!worldserver.Connected())
						Message(0, "Error: World server disconnected.");
					else {
						ServerPacket* pack = new ServerPacket;
						pack->opcode = ServerOP_GMGoto;
						pack->size = sizeof(ServerGMGoto_Struct);
						pack->pBuffer = new uchar[pack->size];
						memset(pack->pBuffer, 0, pack->size);
						ServerGMGoto_Struct* wsgmg = (ServerGMGoto_Struct*) pack->pBuffer;
						strcpy(wsgmg->myname, this->GetName());
						strcpy(wsgmg->gotoname, gmg->charname);
						wsgmg->admin = admin;
						worldserver.SendPacket(pack);
						safe_delete(pack);
					}
					break;
				}
				case OP_TraderShop:{
					TraderClick_Struct* tcs= (TraderClick_Struct*)app->pBuffer;
					if(app->size==sizeof(TraderClick_Struct)){
						APPLAYER* outapp = new APPLAYER(OP_TraderShop, sizeof(TraderClick_Struct));
						TraderClick_Struct* outtcs=(TraderClick_Struct*)outapp->pBuffer;
						Client* tmp = entity_list.GetClientByID(tcs->traderid);
						if (tmp)
							outtcs->approval=tmp->WithCustomer();
						else
							break;
						outtcs->traderid=tcs->traderid;
						QueuePacket(outapp);
						if(outtcs->approval)
							this->BulkSendTraderInventory(tmp->CharacterID());
						safe_delete(outapp);
					}
					break;
				}
				case OP_ShopRequest: {
					// this works
					Merchant_Click_Struct* mc=(Merchant_Click_Struct*)app->pBuffer;
					if (app->size != sizeof(Merchant_Click_Struct))
						break;
					// Send back opcode OP_ShopRequest - tells client to open merchant window.
					//APPLAYER* outapp = new APPLAYER(OP_ShopRequest, sizeof(Merchant_Click_Struct));
					//Merchant_Click_Struct* mco=(Merchant_Click_Struct*)outapp->pBuffer;

					int merchantid=0;
					Mob* tmp = entity_list.GetMob(mc->npcid);
					if (tmp != 0)
						merchantid=tmp->CastToNPC()->MerchantType;
					else
						break;

					int action = 1;
					if(merchantid == 0)
					{
						APPLAYER* outapp = new APPLAYER(OP_ShopRequest, sizeof(Merchant_Click_Struct));
						Merchant_Click_Struct* mco=(Merchant_Click_Struct*)outapp->pBuffer;
						mco->npcid = mc->npcid;
						mco->playerid = 0;
						mco->unknown[0] = 0;
						mco->unknown[1] = 0x00;
						mco->unknown[2] = 0x00;
						mco->unknown[3] = 0x00;
						mco->unknown[4] = 0xE0;
						mco->unknown[5] = 0xCB;
						mco->unknown[6] = 0x90;
						mco->unknown[7] = 0x3F;
						QueuePacket(outapp);
						safe_delete(outapp);
						break;
					}
					if(tmp->IsEngaged()){
						this->Message_StringID(0,MERCHANT_BUSY);
						action = 0;
					}
					if (GetFeigned() || IsInvisible())
					{
						Message(0,"You cannot use a merchant right now.");
						action = 0;
					}
					int factionlvl = GetFactionLevel(CharacterID(), tmp->CastToNPC()->GetNPCTypeID(), GetRace(), GetClass(), GetDeity(), tmp->CastToNPC()->GetPrimaryFaction(), tmp);
					if(factionlvl >= 6 && factionlvl != 9)
					{
						Message(0,"I will not deal with one such as you!");
						action = 0;
					}
					if (tmp->Charmed())
					{
						action = 0;
					}

					APPLAYER* outapp = new APPLAYER(OP_ShopRequest, sizeof(Merchant_Click_Struct));
					Merchant_Click_Struct* mco=(Merchant_Click_Struct*)outapp->pBuffer;

					mco->npcid = mc->npcid;
					mco->playerid = 0;
					mco->unknown[0] = action; // Merchant command 0x01 = open
					mco->unknown[1] = 0x00;
					mco->unknown[2] = 0x00;
					mco->unknown[3] = 0x00;
					mco->unknown[4] = 0xE0;
					mco->unknown[5] = 0xCB; // 32
					mco->unknown[6] = 0x90; // 139
					mco->unknown[7] = 0x3F; // 63

					outapp->priority = 6;
					QueuePacket(outapp);
					safe_delete(outapp);

					if (action == 1)
						BulkSendMerchantInventory(merchantid,tmp->GetNPCTypeID());
					
					break;
				}
				case OP_Bazaar: {
 					if (app->size==sizeof(BazaarSearch_Struct)) {
						BazaarSearch_Struct* bss= (BazaarSearch_Struct*)app->pBuffer;
						this->SendBazaarResults(bss->traderid,bss->class_,bss->race,bss->stat,bss->slot,bss->type,bss->name,bss->minprice,bss->maxprice);
					}
					else if (app->size==sizeof(BazaarWelcome_Struct)) {
						BazaarWelcome_Struct* bws = (BazaarWelcome_Struct*)app->pBuffer;
						if (bws->beginning.action==9)
							SendBazaarWelcome();
					}
					else
						LogFile->write(EQEMuLog::Error, "Malformed BazaarSearch_Struct packet received, ignoring...\n");
					break;
				}
				case OP_ShopPlayerBuy: {
					Merchant_Sell_Struct* mp=(Merchant_Sell_Struct*)app->pBuffer;
#if EQDEBUG >= 5
						LogFile->write(EQEMuLog::Debug, "%s, purchase item..", GetName());
						DumpPacket(app);
#endif
					
					int merchantid;
					bool tmpmer_used = false;
					Mob* tmp = entity_list.GetMob(mp->npcid);
					if (tmp != 0)
						merchantid=tmp->CastToNPC()->MerchantType;
					else
						break;
					uint32 item_id = 0;
					std::list<MerchantList> merlist = zone->merchanttable[merchantid];
					std::list<MerchantList>::const_iterator itr;
					int findslot = mp->itemslot - 84;
					for(itr = merlist.begin();itr != merlist.end();itr++){
						MerchantList ml = *itr;
						if(findslot == ml.slot){
							item_id = ml.item;
							break;
						}
					}
					const Item_Struct* item = NULL;
					int32 prevcharges = 0;
					if (item_id == 0) { //check to see if its on the temporary table
						std::list<TempMerchantList> tmp_merlist = zone->tmpmerchanttable[tmp->GetNPCTypeID()];
						std::list<TempMerchantList>::const_iterator tmp_itr;
						TempMerchantList ml;
						for(tmp_itr = tmp_merlist.begin();tmp_itr != tmp_merlist.end();tmp_itr++){
							ml = *tmp_itr;
							if(findslot == ml.slot){
								item_id = ml.item;
								tmpmer_used = true;
								prevcharges = ml.charges;
								break;
							}
						}
					} 
					item = database.GetItem(item_id);
					if (!item){
						//error finding item, client didnt get the update packet for whatever reason, roleplay a tad
						Message(15,"%s tells you 'Sorry, that item is for display purposes only.' as they take the item off the shelf.",tmp->GetCleanName());						
						APPLAYER* delitempacket = new APPLAYER(OP_ShopDelItem, sizeof(Merchant_DelItem_Struct));
						Merchant_DelItem_Struct* delitem = (Merchant_DelItem_Struct*)delitempacket->pBuffer;
						delitem->itemslot = mp->itemslot;
						delitem->npcid = mp->npcid;
						delitem->playerid = mp->playerid;
						entity_list.QueueCloseClients(tmp,delitempacket); //que for anyone that could be using the merchant so they see the update
						safe_delete(delitempacket);
						break;
					}
					if (CheckLoreConflict(item))
					{
						Message(15,"You can only have one of a lore item.");
						break;
					}
					
					APPLAYER* outapp = new APPLAYER(OP_ShopPlayerBuy, sizeof(Merchant_Sell_Struct));
					Merchant_Sell_Struct* mpo=(Merchant_Sell_Struct*)outapp->pBuffer;
					mpo->quantity = mp->quantity;
					mpo->playerid = mp->playerid;
					mpo->npcid = mp->npcid;
					mpo->itemslot=mp->itemslot;
					
					sint16 freeslotid=0;
					freeslotid = m_inv.FindFreeSlot(false, true, item->Size);
					
					//make sure we are not completely full...
					if(freeslotid == SLOT_CURSOR) {
						if(m_inv.GetItem(SLOT_CURSOR) != NULL) {
							Message(13, "You do not have room for any more items.");
							safe_delete(outapp);
							break;
						}
					}
					
					if(freeslotid == SLOT_INVALID || !TakeMoneyFromPP(mpo->price))
					{
						safe_delete(outapp);
						break;
					}

 					string packet;
					if(tmpmer_used && (mp->quantity > prevcharges))
						mp->quantity = prevcharges;
					else if(mp->quantity==1 && item->Common.MaxCharges>0 && item->Common.MaxCharges<255)
						mp->quantity=item->Common.MaxCharges;
					
					ItemInst* inst = ItemInst::Create(item, mp->quantity);
					if (inst) {
						mpo->price = (item->Cost*127/100)*mp->quantity;
						PutItemInInventory(freeslotid, *inst);
						SendItemPacket(freeslotid, inst, ItemPacketTrade);
					}
					else {
						LogFile->write(EQEMuLog::Error, "OP_ShopPlayerBuy: item->ItemClass Unknown! Type: %i", item->ItemClass);
					}

					QueuePacket(outapp);
					if(inst && tmpmer_used){
						sint32 new_charges = prevcharges - mp->quantity;
						zone->SaveTempItem(merchantid, tmp->GetNPCTypeID(),item_id,new_charges);
						if(new_charges<=0){
							APPLAYER* delitempacket = new APPLAYER(OP_ShopDelItem, sizeof(Merchant_DelItem_Struct));
							Merchant_DelItem_Struct* delitem = (Merchant_DelItem_Struct*)delitempacket->pBuffer;
							delitem->itemslot = mp->itemslot;
							delitem->npcid = mp->npcid;
							delitem->playerid = mp->playerid;
							delitempacket->priority = 6;
							entity_list.QueueClients(tmp,delitempacket); //que for anyone that could be using the merchant so they see the update
							safe_delete(delitempacket);
						}
					}
					safe_delete(inst);
					safe_delete(outapp);
					
					if (zone->merchantvar!=0){
						if (zone->merchantvar==7){
								LogMerchant(this,tmp,mpo,item,true);
						}
						else if ((admin>=10) && (admin<20)){
							if ((zone->merchantvar<8) && (zone->merchantvar>5))
								LogMerchant(this,tmp,mpo,item,true);
						}
						else if (admin<=20){
							if ((zone->merchantvar<8) && (zone->merchantvar>4))
								LogMerchant(this,tmp,mpo,item,true);
						}
						else if (admin<=80){
							if ((zone->merchantvar<8) && (zone->merchantvar>3))
								LogMerchant(this,tmp,mpo,item,true);
						}
						else if (admin<=100){
							if ((zone->merchantvar<9) && (zone->merchantvar>2))
								LogMerchant(this,tmp,mpo,item,true);
						}
						else if (admin<=150){
							if (((zone->merchantvar<8) && (zone->merchantvar>1)) || (zone->merchantvar==9))
								LogMerchant(this,tmp,mpo,item,true);
						}
						else if (admin<=255){
							if ((zone->merchantvar<8) && (zone->merchantvar>0))
								LogMerchant(this,tmp,mpo,item,true);	
						}
					}
					break;
				}
				case OP_ShopPlayerSell: {
					Merchant_Purchase_Struct* mp=(Merchant_Purchase_Struct*)app->pBuffer;
					Mob* vendor = entity_list.GetMob(mp->npcid);
					int32 price=0;
					int32 itemid = GetItemIDAt(mp->itemslot);
					if(itemid == 0)
						break;
					const Item_Struct* item = database.GetItem(itemid);
					ItemInst* inst = GetInv().GetItem(mp->itemslot);
					if(!inst){
						Message(13,"You seemed to have misplaced that item..");
						break;
					}
					if(mp->quantity > 1 && (sint16)mp->quantity > inst->GetCharges())
						break;

					if (item){
						price=(int)((item->Cost*mp->quantity)*.884);
						AddMoneyToPP(price,false);
						if (zone->merchantvar!=0){
							if (zone->merchantvar==7) {
								LogMerchant(this,vendor,mp,item,false);
							}
							else if ((admin>=10) && (admin<20)) {
								if ((zone->merchantvar<8) && (zone->merchantvar>5))
									LogMerchant(this,vendor,mp,item,false);
							}
							else if (admin<=20) {
								if ((zone->merchantvar<8) && (zone->merchantvar>4))
									LogMerchant(this,vendor,mp,item,false);
							}
							else if (admin<=80) {
								if ((zone->merchantvar<8) && (zone->merchantvar>3))
									LogMerchant(this,vendor,mp,item,false);
							}
							else if (admin<=100) {
								if ((zone->merchantvar<9) && (zone->merchantvar>2))
									LogMerchant(this,vendor,mp,item,false);
							}
							else if (admin<=150) {
								if (((zone->merchantvar<8) && (zone->merchantvar>1)) || (zone->merchantvar==9))
									LogMerchant(this,vendor,mp,item,false);
							}
							else if (admin<=255) {
								if ((zone->merchantvar<8) && (zone->merchantvar>0))
									LogMerchant(this,vendor,mp,item,false);	
							}
						}
					}
					else
						Message(0, "Error #1, item == 0");
					
	
					if (item && inst->IsStackable())
					{
						unsigned int i_quan = inst->GetCharges();
						if (mp->quantity > i_quan)
							mp->quantity = i_quan;
					}
					else
					{
						mp->quantity = 1;
					}
					int freeslot = 0;
					int charges = 0;
					if(inst->IsStackable())
						charges = mp->quantity;
					else
						charges = inst->GetCharges();
					if((freeslot = zone->SaveTempItem(vendor->CastToNPC()->MerchantType, vendor->GetNPCTypeID(),itemid,charges,true)) > 0){
						ItemInst* inst2 = inst->Clone();
						inst2->SetPrice(item->Cost*127/100);
						inst2->SetUnknown5(freeslot+84);
						if(inst2->IsStackable())
							inst2->SetCharges(mp->quantity);
						SendItemPacket(freeslot-1, inst2, ItemPacketMerchant);
						safe_delete(inst2);
					}

					// Now remove the item from the player, this happens irrguardless of outcome
					if (!inst->IsStackable())
						this->DeleteItemInInventory(mp->itemslot,0,false);
					else
						this->DeleteItemInInventory(mp->itemslot,mp->quantity,false);
					
					APPLAYER* outapp = new APPLAYER(OP_ShopPlayerSell, sizeof(Merchant_Purchase_Struct));
					Merchant_Purchase_Struct* mco=(Merchant_Purchase_Struct*)outapp->pBuffer;
					mco->npcid = vendor->GetID();
					mco->itemslot=mp->itemslot;
					mco->quantity=mp->quantity;
					mco->price=price;
					QueuePacket(outapp);
					safe_delete(outapp);
					Save();	
					break;
				}
				case OP_ShopEnd: {
					APPLAYER* outapp = new APPLAYER(OP_ShopEndConfirm, 2);
					outapp->pBuffer[0] = 0x0a;
					outapp->pBuffer[1] = 0x66;
					QueuePacket(outapp);
					safe_delete(outapp);
					Save();
					break;
				}
				case OP_CloseContainer: { // Player is closing tradeskill container
					if (app->size != sizeof(CloseContainer_Struct)) {
						LogFile->write(EQEMuLog::Error, "Invalid size on CloseContainer_Struct: Expected %i, Got %i",
							sizeof(CloseContainer_Struct), app->size);
						break;
					}

					SetTradeskillObject(NULL);
					
					ClickObjectAck_Struct* oos = (ClickObjectAck_Struct*)app->pBuffer;
					Entity* entity = entity_list.GetID(oos->drop_id);
					if (entity && entity->IsObject()) {
						Object* object = entity->CastToObject();
						object->Close();
					}
					break;
				}
				case OP_ClickObject: { // Player clicked on zone object (forge, item on ground, etc)
					if (app->size != sizeof(ClickObject_Struct)) {
						LogFile->write(EQEMuLog::Error, "Invalid size on ClickObject_Struct: Expected %i, Got %i",
							sizeof(ClickObject_Struct), app->size);
						break;
					}
					
					ClickObject_Struct* click_object = (ClickObject_Struct*)app->pBuffer;
					Entity* entity = entity_list.GetID(click_object->drop_id);
					if (entity && entity->IsObject()) {
						Object* object = entity->CastToObject();
						object->HandleClick(this, click_object);
					}
					break;
				}
				case OP_RecipesFavorite: {
					if (app->size != sizeof(TradeskillFavorites_Struct)) {
						LogFile->write(EQEMuLog::Error, "Invalid size for TradeskillFavorites_Struct: Expected: %i, Got: %i",
							sizeof(TradeskillFavorites_Struct), app->size);
						break;
					}
					
					TradeskillFavorites_Struct* tsf = (TradeskillFavorites_Struct*)app->pBuffer;
					
					uint32 tskill = Object::TypeToSkill(tsf->object_type);
					if(tskill == 0) {
						LogFile->write(EQEMuLog::Error, "Unknown container type for OP_RecipesFavorite: %d\n", tsf->object_type);
						break;
					}
					
    				char *query = 0;
					char buf[1100];	//gotta be big enough for 100 IDs
					
					bool first = true;
					uint8 r;
					char *pos = buf;
					
					//Assumes item IDs are <10 characters long
					for(r = 0; r < 100; r++) {
						if(tsf->favorite_recipes[r] == 0)
							break;	//assume the first 0 is the end...
						
						if(first) {
							pos += snprintf(pos, 10, "%lu", tsf->favorite_recipes[r]);
							first = false;
						} else {
							pos += snprintf(pos, 10, ",%lu", tsf->favorite_recipes[r]);
						}
					}
					
					if(first)	//no favorites....
						break;
					
					//To be a good kid, I should move this SQL somewhere else...
					//but im lazy right now, so it stays here
					uint32 qlen = 0;
					qlen = MakeAnyLenString(&query, "SELECT tr.id,tr.name,tr.trivial,SUM(tre.componentcount) "
						" FROM tradeskill_recipe AS tr "
						" LEFT JOIN tradeskill_recipe_entries AS tre ON tr.id=tre.recipe_id "
						" WHERE tr.id IN (%s) AND tradeskill=%lu "
						" GROUP BY tr.id LIMIT 100 ", buf, tskill);
					
					TradeskillSearchResults(query, qlen, tsf->object_type, tsf->some_id);
					
					safe_delete_array(query);
					break;
				}
				case OP_RecipesSearch: {
					if (app->size != sizeof(RecipesSearch_Struct)) {
						LogFile->write(EQEMuLog::Error, "Invalid size for RecipesSearch_Struct: Expected: %i, Got: %i",
							sizeof(RecipesSearch_Struct), app->size);
						break;
					}
					
					RecipesSearch_Struct* rss = (RecipesSearch_Struct*)app->pBuffer;
					rss->query[55] = '\0';	//just to be sure.
					
					
					uint32 tskill = Object::TypeToSkill(rss->object_type);
					if(tskill == 0) {
						LogFile->write(EQEMuLog::Error, "Unknown container type for OP_RecipesSearch: %d\n", rss->object_type);
						break;
					}
					
    				char *query = 0;
					char searchclause[140];	//2X rss->query + SQL crap
					
					//omit the rlike clause if query is empty
					if(rss->query[0] != 0) {
						char buf[120];	//larger than 2X rss->query
						database.DoEscapeString(buf, rss->query, strlen(rss->query));
						
						snprintf(searchclause, 139, "name rlike '%s' AND", buf);
					} else {
						searchclause[0] = '\0';
					}
					uint32 qlen = 0;
					
					//arbitrary limit of 200 recipes, makes sense to me.
					qlen = MakeAnyLenString(&query, "SELECT tr.id,tr.name,tr.trivial,SUM(tre.componentcount) "
						" FROM tradeskill_recipe AS tr "
						" LEFT JOIN tradeskill_recipe_entries AS tre ON tr.id=tre.recipe_id "
						" WHERE %s tr.trivial >= %u AND tr.trivial <= %u AND tradeskill=%lu "
						" GROUP BY tr.id LIMIT 200 ", searchclause, rss->mintrivial, rss->maxtrivial, tskill);
					
					TradeskillSearchResults(query, qlen, rss->object_type, rss->some_id);
					
					safe_delete_array(query);
					break;
				}
				case OP_RecipeDetails: {
					if(app->size != sizeof(unsigned long)) {
						LogFile->write(EQEMuLog::Error, "Invalid size for RecipeDetails Request: Expected: %i, Got: %i",
							sizeof(unsigned long), app->size);
						break;
					}
					unsigned long *recipe_id = (unsigned long *) app->pBuffer;
					
					SendTradeskillDetails(*recipe_id);
					
					break;
				}
				case OP_RecipeAutoCombine: {
					if (app->size != sizeof(RecipeAutoCombine_Struct)) {
						LogFile->write(EQEMuLog::Error, "Invalid size for RecipeAutoCombine_Struct: Expected: %i, Got: %i",
							sizeof(RecipeAutoCombine_Struct), app->size);
						break;
					}
					
					RecipeAutoCombine_Struct* rac = (RecipeAutoCombine_Struct*)app->pBuffer;
					
					Object::HandleAutoCombine(this, rac);
					break;
				}
				case OP_TradeSkillCombine: {
					if (app->size != sizeof(NewCombine_Struct)) {
						LogFile->write(EQEMuLog::Error, "Invalid size for NewCombine_Struct: Expected: %i, Got: %i",
							sizeof(NewCombine_Struct), app->size);
						break;
					}
					/*if (m_tradeskill_object == NULL) {
						Message(13, "Error: Server is not aware of the tradeskill container you are attempting to use");
						break;
					}*/
					
					//fixed this to work for non-world objects
					
					// Delegate to tradeskill object to perform combine
					NewCombine_Struct* in_combine = (NewCombine_Struct*)app->pBuffer;
					Object::HandleCombine(this, in_combine, m_tradeskill_object);
					break;
				}
				
				case OP_ClickDoor: {
                                        ClickDoor_Struct* cd = (ClickDoor_Struct*)app->pBuffer;
                                        Doors* currentdoor = entity_list.FindDoor(cd->doorid);
                                        if(!currentdoor)
                                        {
                                        	Message(0,"Unable to find door, please notify a GM (DoorID: %i).",cd->doorid);
                                                break;
                                        }

					#ifdef GUILDWARS
                                        if (IsSettingGuildDoor){
                                                        if (database.SetGuildDoor(cd->doorid,SetGuildDoorID,zone->GetShortName())){
                                                                cout << SetGuildDoorID << endl;
                                                                if (SetGuildDoorID){
                                                                        Message(0,"This is now a guilddoor of '%s'",guilds[SetGuildDoorID].name);
                                                                        currentdoor->SetGuildID(SetGuildDoorID);
                                                                }
                                                                else{
                                                                                        Message(0,"Guildoor deleted");
                                                                                        currentdoor->SetGuildID(0);
                                                                }
                                                        }
                                                        else
                                                                Message(0,"Failed to edit guilddoor!");
                                                        IsSettingGuildDoor = false;
                                                        break;
                                                }
                                                if (cd->doorid >= 128){
                                                        if(!database.CheckGuildDoor(cd->doorid,GuildEQID(),zone->GetShortName())){
                                                                // heh hope grammer on this is right lol
                                                                this->Message(0,"A magic barrier protects this hall against intruders!");

                                                                break;
                                                        }
                                                        else
                                                                this->Message(0,"The magic barrier disappears and you open the door!");
                                        }
					#endif
                        		currentdoor->HandleClick(this);
					break;
				}
				case OP_CreateObject: { // User dropping item from cursor to ground
					DropItem(SLOT_CURSOR);
					break;
				}
				case OP_FaceChange: { // When client changes their face & stuff
					// Notify other clients in zone
					entity_list.QueueClients(this, app, false);
					
					FaceChange_Struct* fc = (FaceChange_Struct*)app->pBuffer;
					m_pp.haircolor	= fc->haircolor;
					m_pp.beardcolor	= fc->beardcolor;
					m_pp.eyecolor1	= fc->eyecolor1;
					m_pp.eyecolor2	= fc->eyecolor2;
					m_pp.hairstyle	= fc->hairstyle;
					m_pp.face		= fc->face;
// vesuvias - appearence fix
					m_pp.beard		= fc->beard;					

					
					Save();
					Message_StringID(13,FACE_ACCEPTED);
					//Message(13, "Facial features updated.");
					break;
				}
				case OP_GroupInvite:
				case OP_GroupInvite2: {
					if(this->GetTarget() != 0 && this->GetTarget()->IsClient()) {
						this->GetTarget()->CastToClient()->QueuePacket(app);
						break;
					}
					/*if(this->GetTarget() != 0 && this->GetTarget()->IsNPC() && this->GetTarget()->CastToNPC()->IsInteractive()) {
						if(!this->GetTarget()->CastToNPC()->IsGrouped()) {
							APPLAYER* outapp = new APPLAYER(OP_GroupUpdate,sizeof(GroupUpdate_Struct));
							GroupUpdate_Struct* gu = (GroupUpdate_Struct*) outapp->pBuffer;
							gu->action = 9;
							strcpy(gu->membername,GetName());
							strcpy(gu->yourname,GetTarget()->CastToNPC()->GetName());
							FastQueuePacket(&outapp);
							if (!isgrouped){
								Group* ng = new Group(this);
								entity_list.AddGroup(ng);
							}
							entity_list.GetGroupByClient(this->CastToClient())->AddMember(GetTarget());
							this->GetTarget()->CastToNPC()->TakenAction(22,this->CastToMob());
						}
						else {
							    LogFile->write(EQEMuLog::Debug, "IPC: %s already grouped.", this->GetTarget()->GetName());
						}
					}*/
					break;
				}
				case OP_GroupAcknowledge:
					break;
				case OP_GroupCancelInvite: {
					GroupGeneric_Struct* gf = (GroupGeneric_Struct*) app->pBuffer;
					Mob* inviter = entity_list.GetClientByName(gf->name1);
					
					if(inviter != NULL && inviter->IsClient())
						inviter->CastToClient()->QueuePacket(app);

					database.SetGroupID(GetName(), 0);
					break;
				}
				case OP_GroupFollow:
				case OP_GroupFollow2: {
					GroupGeneric_Struct* gf = (GroupGeneric_Struct*) app->pBuffer;
					Mob* inviter = entity_list.GetClientByName(gf->name1);
					
					if(inviter != NULL && inviter->IsClient()) {
						isgrouped = true;
						strcpy(gf->name1,inviter->GetName());
						strcpy(gf->name2,this->GetName());
						
						Group* group = entity_list.GetGroupByClient(inviter->CastToClient());
						
						if(!group){
							//Make new group
							group = new Group(inviter);
							if(!group)
								break;
							entity_list.AddGroup(group);
							
							if(group->GetID() == 0) {
								Message(13, "Unable to get new group id. Cannot create group.");
								inviter->Message(13, "Unable to get new group id. Cannot create group.");
								break;
							}
							
							//now we have a group id, can set inviter's id
							database.SetGroupID(inviter->GetName(), group->GetID());
							
							//Invite the inviter into the group first.....dont ask
							APPLAYER* outapp=new APPLAYER(OP_GroupUpdate,sizeof(GroupJoin_Struct));
							GroupJoin_Struct* outgj=(GroupJoin_Struct*)outapp->pBuffer;
							strcpy(outgj->membername, inviter->GetName());
							strcpy(outgj->yourname, inviter->GetName());
							outgj->action = 9;
							inviter->CastToClient()->QueuePacket(outapp);
							safe_delete(outapp);
						}
						if(!group)
							break;
						
						inviter->CastToClient()->QueuePacket(app);//notify inviter the client accepted
						
						if(!group->AddMember(this))
							break;
						group->SendUpdate(7,this);
						group->SendHPPackets(this);
						
					}
					break;
				}
				case OP_GroupDisband: {
					printf("Member Disband Request\n");
					
					GroupGeneric_Struct* gd = (GroupGeneric_Struct*) app->pBuffer;
					Group* group = GetGroup();
					
					if(!group)
						break;
					
					if((group->IsLeader(this) && target == 0) || (group->GroupCount()<3)) {
						group->DisbandGroup();
					} else {
						group->DelMember(entity_list.GetMob(gd->name2),false);
					}
					break;
				}
				case OP_GroupDelete: {
					printf("Group Delete Request\n");
					Group* group = GetGroup();
					if (group)
						group->DisbandGroup();
					break;
				}
				case OP_GMEmoteZone: {
					if(this->Admin() < 80) {
						Message(13, "Your account has been reported for hacking.");
						database.SetHackerFlag(this->account_name, this->name, "/emote");
						break;
					}
					GMEmoteZone_Struct* gmez = (GMEmoteZone_Struct*)app->pBuffer;
					char* newmessage=0;
					if(strstr(gmez->text,"^")==0)
						entity_list.Message(0, 15, gmez->text);
					else{
						for(newmessage = strtok((char*)gmez->text,"^");newmessage!=NULL;newmessage=strtok(NULL, "^"))
							entity_list.Message(0, 15, newmessage);
					}
					break;
				}
				case OP_InspectRequest: {
					Inspect_Struct* ins = (Inspect_Struct*) app->pBuffer;
					Mob* tmp = entity_list.GetMob(ins->TargetID);
					if(tmp != 0 && tmp->IsClient())
						tmp->CastToClient()->QueuePacket(app); // Send request to target

					break;
				}
				case OP_InspectAnswer: {
					//Cofruben: Fills the app sent from client.
					Inspect_Struct* ins = (Inspect_Struct*) app->pBuffer;
					APPLAYER* outapp = app->Copy();
					InspectResponse_Struct* insr = (InspectResponse_Struct*) outapp->pBuffer;
					Mob* tmp = entity_list.GetMob(ins->TargetID);
					const Item_Struct* item = NULL;
					for (sint16 L=0; L<=21; L++) {
						const ItemInst* inst = GetInv().GetItem(L);
						item = (inst) ? inst->GetItem() : NULL;
						if(item>0){
							strcpy(insr->itemnames[L],item->Name);
							insr->itemicons[L]=item->IconNumber;
						}
						else
							insr->itemicons[L]=0xFFFFFFFF;	
					}
					if(tmp != 0 && tmp->IsClient())
						tmp->CastToClient()->QueuePacket(outapp); // Send answer to requester
					break;
				}
#if 0	// solar: i dont think there's an op for this now, and we check this
			// when the client is sitting
				case OP_Medding: {
					if (app->pBuffer[0])
						medding = true;
					else
						medding = false;
					break;
				}
#endif
				case OP_DeleteSpell: {
					if(app->size != sizeof(DeleteSpell_Struct))
						break;
				
					APPLAYER* outapp = app->Copy();
					DeleteSpell_Struct* dss = (DeleteSpell_Struct*) outapp->pBuffer;
					
					if(dss->spell_slot < 0 || dss->spell_slot > MAX_PP_SPELLBOOK)
						break;
					
					if(m_pp.spell_book[dss->spell_slot] != SPELLBOOK_UNKNOWN) {
						m_pp.spell_book[dss->spell_slot] = SPELLBOOK_UNKNOWN;
						dss->success = 1;
					}
					else
						dss->success = 0;
					
					FastQueuePacket(&outapp);
					break;
				}
				case OP_PetitionBug:{
					if(app->size!=sizeof(PetitionBug_Struct))
						printf("Wrong size of BugStruct! Expected: %i, Got: %i\n",sizeof(PetitionBug_Struct),app->size);
					else{
						PetitionBug_Struct* bug=(PetitionBug_Struct*)app->pBuffer;
						database.UpdateBug(bug);
					}
					break;
				}
				case OP_Bug:{
					if(app->size!=sizeof(BugStruct))
						printf("Wrong size of BugStruct!\n");
					else{
						BugStruct* bug=(BugStruct*)app->pBuffer;
						database.UpdateBug(bug);
					}
					break;
				}
				case OP_Petition: {
					if (app->size <= 1)
						break;
					if (!worldserver.Connected())
						Message(0, "Error: World server disconnected");
					/*else if(petition_list.FindPetitionByAccountName(this->AccountName()))
						{
						Message(0,"You already have a petition in queue, you cannot petition again until this one has been responded to or you have deleted the petition.");
						break;
						}*/
					else
					{
						if(petition_list.FindPetitionByAccountName(AccountName()))
						{
						Message(0,"You already have a petition in the queue, you must wait for it to be answered or use /deletepetition to delete it.");
						break;
						}
						Petition* pet = new Petition;
						pet->SetAName(this->AccountName());
						pet->SetClass(this->GetClass());
						pet->SetLevel(this->GetLevel());
						pet->SetCName(this->GetName());
						pet->SetRace(this->GetRace());
						pet->SetLastGM("");
						pet->SetCName(this->GetName());
						pet->SetPetitionText((char*) app->pBuffer);
						pet->SetZone(zone->GetZoneID());
						pet->SetUrgency(0);
						petition_list.AddPetition(pet);						
						database.InsertPetitionToDB(pet);						petition_list.UpdateGMQueue();
						petition_list.UpdateZoneListQueue();
						worldserver.SendEmoteMessage(0, 0, 80, 15, "%s has made a petition. #%i", GetName(), pet->GetID());
					}
					break;
				}
				case OP_PetitionCheckIn: {
					Petition_Struct* inpet = (Petition_Struct*) app->pBuffer;

					Petition* pet = petition_list.GetPetitionByID(inpet->petnumber);
					//if (inpet->urgency != pet->GetUrgency())
						pet->SetUrgency(inpet->urgency);
					pet->SetLastGM(this->GetName());
					pet->SetGMText(inpet->gmtext);

					pet->SetCheckedOut(false);
					petition_list.UpdatePetition(pet);
					petition_list.UpdateGMQueue();
					petition_list.UpdateZoneListQueue();
					break;
				}
				case OP_PetitionResolve:
				case OP_PetitionDelete: {
					APPLAYER* outapp = new APPLAYER(OP_PetitionUpdate,sizeof(PetitionUpdate_Struct));
					PetitionUpdate_Struct* pet = (PetitionUpdate_Struct*) outapp->pBuffer;
					pet->petnumber = *((int*) app->pBuffer);
					pet->color = 0x00;
					pet->status = 0xFFFFFFFF;
					pet->senttime = 0;
					strcpy(pet->accountid, "");
					strcpy(pet->gmsenttoo, "");
					pet->quetotal = petition_list.GetMaxPetitionID();
					strcpy(pet->charname, "");
					FastQueuePacket(&outapp);
					
					if (petition_list.DeletePetition(pet->petnumber) == -1)
						cout << "Something is borked with: " << pet->petnumber << endl;
					petition_list.ClearPetitions();
					petition_list.UpdateGMQueue();
					petition_list.ReadDatabase();
					petition_list.UpdateZoneListQueue();
					break;
				}
				case OP_PetCommands: {
					char val1[20]={0};		
					PetCommand_Struct* pet = (PetCommand_Struct*) app->pBuffer;
					Mob* mypet = this->GetPet();
					if(!mypet) break;
					if(mypet == 0)
                    {
                        // try to get a familiar if we dont have a real pet
                        mypet = this->GetFamiliar();
                        if (mypet == NULL)
							break;
                    }
					
                    if(GetClass() == ENCHANTER && mypet->GetPetType() != 0xFF)
						break;
					
                    // just let the command "/pet get lost" work for familiars
                    if(mypet == GetFamiliar() && pet->command != PET_GETLOST)
						break;
					switch(pet->command)
					{
					case PET_ATTACK: {
						if (!target)
							break;
						if (target->IsMezzed()) {
							Message_StringID(10, CANNOT_WAKE, mypet->GetCleanName(), target->GetCleanName());
							break;
						}
						if (mypet->GetHateTop()==0 && target != this && DistNoZ(*target) <= 100) {
							zone->AddAggroMob();
							mypet->AddToHateList(target, 1);
							Message_StringID(10, PET_ATTACKING, mypet->GetCleanName(), target->GetCleanName());
						}
						break;
					}
					case PET_BACKOFF: {
						mypet->Say_StringID(PET_CALMING);
						mypet->WhipeHateList();
						break;
					}
					case PET_HEALTHREPORT: {
						Message_StringID(10, PET_REPORT_HP, ConvertArrayF(mypet->GetHPRatio(), val1));
						//Message(10,"%s tells you, 'I have %d percent of my hit points left.'",mypet->GetName(),(int8)mypet->GetHPRatio());
						break;
					}
					case PET_GETLOST: {
						if (mypet->Charmed())
							break;
						if (mypet->GetPetType() == 0xFF || !mypet->IsNPC()) {
							// eqlive ignores this command
							// we could just remove the charm
							// and continue
							mypet->BuffFadeByEffect(SE_Charm);
							break;
						} else {
							SetPet(NULL);
						}
						if (mypet == GetFamiliar()) {
							SetFamiliarID(0);
						}
						mypet->Say_StringID(PET_GETLOST_STRING);
						mypet->CastToNPC()->Depop();
						break;
					}
					case PET_LEADER: {
						mypet->Say_StringID(PET_LEADERIS);
						break;
					}
					case PET_GUARDHERE: {
						mypet->Say_StringID(PET_GUARDINGLIFE);
						mypet->SetPetOrder(SPO_Guard);
						mypet->SaveGuardSpot();
						break;
					}
					case PET_FOLLOWME: {
						mypet->Say_StringID(PET_FOLLOWING);
						mypet->SetPetOrder(SPO_Follow);
						mypet->SendAppearancePacket(AT_Anim, ANIM_STAND);
						break;
					}
					case PET_TAUNT: {
						Message(0,"%s says, 'Now taunting foes, Master!",mypet->GetName());
						mypet->CastToNPC()->SetTaunting(true);
						break;
					}
					case PET_NOTAUNT: {
						Message(0,"%s says, 'No longer taunting foes, Master!",mypet->GetName());
						mypet->CastToNPC()->SetTaunting(false);
						break;
					}
					case PET_GUARDME: {
						mypet->Say_StringID(PET_GUARDME_STRING);
						mypet->SetPetOrder(SPO_Follow);
						mypet->SendAppearancePacket(AT_Anim, ANIM_STAND);
						break;
					}
					case PET_SITDOWN: {
						mypet->Say_StringID(PET_SIT_STRING);
						mypet->SetPetOrder(SPO_Sit);
						mypet->SetRunAnimSpeed(0);
						if(!mypet->UseBardSpellLogic())	// solar: maybe we can have a bard pet
							mypet->InterruptSpell(); //Baron-Sprite: No cast 4 u. // neotokyo: i guess the pet should start casting
						mypet->SendAppearancePacket(AT_Anim, ANIM_SIT);
						break;
					}
					case PET_STANDUP: {
						mypet->Say_StringID(PET_SIT_STRING);
						mypet->SetPetOrder(SPO_Follow);
						mypet->SendAppearancePacket(AT_Anim, ANIM_STAND);
						break;
					}
					case PET_SLUMBER: {
						mypet->Say_StringID(PET_SIT_STRING);
						mypet->SetPetOrder(SPO_Sit);
						mypet->SetRunAnimSpeed(0);
						if(!mypet->UseBardSpellLogic())	// solar: maybe we can have a bard pet
							mypet->InterruptSpell(); //Baron-Sprite: No cast 4 u. // neotokyo: i guess the pet should start casting
						mypet->SendAppearancePacket(AT_Anim, ANIM_DEATH);
						break;
					}
					default: {
						printf("Client attempted to use a unknown pet command:\n");
						break;
					}
					}
					break;
				}
				case OP_PetitionUnCheckout:{
					if (app->size != sizeof(int32)) {
						cout << "Wrong size: OP_PetitionUnCheckout, size=" << app->size << ", expected " << sizeof(int32) << endl;
						break;
					}
					if (!worldserver.Connected())
						Message(0, "Error: World server disconnected");
					else {
						int32 getpetnum = *((int32*) app->pBuffer);
						Petition* getpet = petition_list.GetPetitionByID(getpetnum);
						if (getpet != 0) {
							getpet->SetCheckedOut(false);
							petition_list.UpdatePetition(getpet);
							petition_list.UpdateGMQueue();
							petition_list.UpdateZoneListQueue();
						}
					}
					break;
				}
				case OP_PetitionQue: {
#ifdef _EQDEBUG
						printf("%s looking at petitions..\n",this->GetName());
#endif
					break;
				}
				case OP_PDeletePetition:{
					if(petition_list.DeletePetitionByCharName((char*)app->pBuffer))
						Message_StringID(0,PETITION_DELETED);	
					else
						Message_StringID(0,PETITION_NO_DELETE);	
					break;
				}
				case OP_PetitionCheckout: {
					if (app->size != sizeof(int32)) {
						cout << "Wrong size: OP_PetitionCheckout, size=" << app->size << ", expected " << sizeof(int32) << endl;
						break;
					}
					if (!worldserver.Connected())
						Message(0, "Error: World server disconnected");
					else {
						int32 getpetnum = *((int32*) app->pBuffer);
						Petition* getpet = petition_list.GetPetitionByID(getpetnum);
						if (getpet != 0) {
							getpet->AddCheckout();
							getpet->SetCheckedOut(true);
							getpet->SendPetitionToPlayer(this->CastToClient());
							petition_list.UpdatePetition(getpet);
							petition_list.UpdateGMQueue();
							petition_list.UpdateZoneListQueue();
						}
					}
					break;
				}
				case OP_PetitionRefresh: {
					// This is When Client Asks for Petition Again and Again...
					// break is here because it floods the zones and causes lag if it
					// Were to actually do something:P  We update on our own schedule now.
					break;
				}
				case OP_ReadBook: {
					BookRequest_Struct* book = (BookRequest_Struct*) app->pBuffer;
					ReadBook(book);
					break;
				}
				case OP_Emote: {
					// Calculate new packet dimensions
					Emote_Struct* in	= (Emote_Struct*)app->pBuffer;
					const char* name	= GetName();
					uint32 len_name		= strlen(name);
					uint32 len_msg		= strlen(in->message);
					uint32 len_packet	= sizeof(in->unknown01) + len_name
										+ strlen(in->message) + 1;
					
					// Construct outgoing packet
					APPLAYER* outapp = new APPLAYER(OP_Emote, len_packet);
					Emote_Struct* out = (Emote_Struct*)outapp->pBuffer;
					out->unknown01 = in->unknown01;
					memcpy(out->message, name, len_name);
					memcpy(&out->message[len_name], in->message, len_msg);
					
					//cout << "######### Outgoing emote packet" << endl;
					//DumpPacket(outapp);
					
					/*
					if (target && target->IsClient()) {
						entity_list.QueueCloseClients(this, outapp, false, 100, target);
						
						cptr = outapp->pBuffer + 2;
						
                        // not sure if live does this or not.  thought it was a nice feature, but would take a lot to
						// clean up grammatical and other errors.  Maybe with a regex parser...
						replacestr((char *)cptr, target->GetName(), "you");
						replacestr((char *)cptr, " he", " you");
						replacestr((char *)cptr, " she", " you");
						replacestr((char *)cptr, " him", " you");
						replacestr((char *)cptr, " her", " you");
						target->CastToClient()->QueuePacket(outapp);
						
					}
					else
					*/
					entity_list.QueueCloseClients(this, outapp, true, 100,0,true,FILTER_SOCIALS);
					
					safe_delete(outapp);
					break;
				}
				case OP_EmoteAnim: {
					// Someone animation an emote (i.e., waving arm to say hello)
					entity_list.QueueCloseClients(this, app, true);
					break;
				}
				case OP_SetServerFilter: {
					SetServerFilter_Struct* filter=(SetServerFilter_Struct*)app->pBuffer;
					ServerFilter(filter);
					break;
				}
				case OP_GMDelCorpse: {
					if(this->Admin() < 100) {
						Message(13, "Your account has been reported for hacking.");
						database.SetHackerFlag(this->account_name, this->name, "/delcorpse");
						break;
					}
					GMDelCorpse_Struct* dc = (GMDelCorpse_Struct *)app->pBuffer;
					Mob* corpse = entity_list.GetMob(dc->corpsename);
					if(corpse==0) {
						break;
					}
					if(corpse->IsCorpse() != true) {
						break;
					}
					corpse->CastToCorpse()->Delete();
					cout << name << " deleted corpse " << dc->corpsename << endl;
					Message(13, "Corpse %s deleted.", dc->corpsename);
					break;
				}
				case OP_GMKick: {
					if(this->Admin() < 150) {
						Message(13, "Your account has been reported for hacking.");
						database.SetHackerFlag(this->account_name, this->name, "/kick");
						break;
					}
					GMKick_Struct* gmk = (GMKick_Struct *)app->pBuffer;

					Client* client = entity_list.GetClientByName(gmk->name);
					if(client==0) {
						if (!worldserver.Connected())
							Message(0, "Error: World server disconnected");
						else {
							ServerPacket* pack = new ServerPacket;
							pack->opcode = ServerOP_KickPlayer;
							pack->size = sizeof(ServerKickPlayer_Struct);
							pack->pBuffer = new uchar[pack->size];
							ServerKickPlayer_Struct* skp = (ServerKickPlayer_Struct*) pack->pBuffer;
							strcpy(skp->adminname, gmk->gmname);
							strcpy(skp->name, gmk->name);
							skp->adminrank = this->Admin();
							worldserver.SendPacket(pack);
							safe_delete(pack);
						}
					}
					else {
						entity_list.QueueClients(this,app);
						//client->Kick();
					}
					break;
				}
				case OP_GMServers: {
					if (!worldserver.Connected())
						Message(0, "Error: World server disconnected");
					else {
						ServerPacket* pack = new ServerPacket;
						pack->size = strlen(this->GetName())+2;
						pack->pBuffer = new uchar[pack->size];
						memset(pack->pBuffer, 0, pack->size);
						pack->opcode = ServerOP_ZoneStatus;
						memset(pack->pBuffer, (int8) admin, 1);
						strcpy((char *) &pack->pBuffer[1], this->GetName());
						worldserver.SendPacket(pack);
						safe_delete(pack);
					}
					break;
				}
				case OP_Illusion: {
					Illusion_Struct* bnpc = (Illusion_Struct*)app->pBuffer;
					// @merth: these need to be implemented
					/*
					texture		= bnpc->texture;
					helmtexture	= bnpc->helmtexture;
					luclinface	= bnpc->luclinface;
					*/
					race		= bnpc->race;
					size		= 0;
					
					entity_list.QueueClients(this,app);
					break;
				}
				case OP_GMBecomeNPC: {
					if(this->Admin() < 80) {
						Message(13, "Your account has been reported for hacking.");
						database.SetHackerFlag(this->account_name, this->name, "/becomenpc");
						break;
					}
					//entity_list.QueueClients(this, app, false);
					BecomeNPC_Struct* bnpc = (BecomeNPC_Struct*)app->pBuffer;
					
					Mob* cli = (Mob*) entity_list.GetMob(bnpc->id);
					if(cli==0)
						break;
					
					if(cli->IsClient())
						cli->CastToClient()->QueuePacket(app);
					cli->SendAppearancePacket(AT_NPCName, 1, true);
					cli->CastToClient()->SetBecomeNPC(true);
					cli->CastToClient()->SetBecomeNPCLevel(bnpc->maxlevel);
					cli->Message_StringID(0,TOGGLE_OFF);
					cli->CastToClient()->tellsoff = true;
					//TODO: Make this toggle a BecomeNPC flag so that it gets updated when people zone in as well; Make combat work with this.
					break;
				}
				case OP_Fishing: {
					if(!p_timers.Expired(pTimerFishing, false)) {
						Message(13,"Ability recovery time not yet met.");
						break;
					}
					p_timers.Start(pTimerFishing, FishingReuseTime-1);
					
					fishing_timer.Start();
					break;
				}
				// Changes made based on Bobs work on foraging.  Now can set items in the forage database table to 
				// forage for.
				case OP_Forage:	{
					// @merth: This needs to be redone with new item classes
					
					if(!p_timers.Expired(pTimerForaging, false)) {
						Message(13,"Ability recovery time not yet met.");
						break;
					}
					p_timers.Start(pTimerForaging, ForagingReuseTime-1);
					
					ForageItem();
					
					break;
				}
				case OP_Mend: {
					if(GetClass() != MONK)
						break;
					
					if(!p_timers.Expired(pTimerMend, false)) {
						Message(13,"Ability recovery time not yet met.");
						break;
					}
					p_timers.Start(pTimerMend, MendReuseTime-1);
					
					int num = 25 + 5*GetAA(aaCriticalMend) + 5*GetAA(aaMendingoftheTranquil);
					int mendhp = (int) GetMaxHP() * num / 100;
					uint32 noadvance = MakeRandomInt(0, 200);
					int currenthp = GetHP();
					if (MakeRandomInt(0, 100) <= (int)GetSkill(MEND)) {
						SetHP(GetHP() + mendhp);
						SendHPUpdate();
						Message_StringID(4,MEND_SUCCESS);
						//Message(4, "You mend your wounds and heal some damage");
					}
					else if (noadvance > 175) {
						if(currenthp > mendhp) {
							SetHP(GetHP() - mendhp);
							SendHPUpdate();
							//Message(4, "You fail to mend your wounds and damage yourself!");
							Message_StringID(4,MEND_WORSEN);
						} else {
							SetHP(1);
							SendHPUpdate();
							//Message(4, "You fail to mend your wounds and damage yourself!");
							Message_StringID(4,MEND_WORSEN);
						}
					}
					else	{
						//Message(4, "You fail to mend your wounds");
						Message_StringID(4,MEND_FAIL);
					}
					
					if(GetSkill(MEND) < noadvance)
						CheckIncreaseSkill(MEND);
					//if ((GetSkill(MEND) < noadvance) && (MakeRandomFloat(0, 100) < 35) && (GetSkill(MEND) < 101))
					//	this->SetSkill(MEND,GetRawSkill(MEND)+1);
					break;
				}
				case OP_EnvDamage: {
					EnvDamage2_Struct* ed = (EnvDamage2_Struct*)app->pBuffer;
					if(admin>=100 && GetGM()){
						Message(13, "Your GM status protects you from %i points of type %i environmental damage.", ed->damage, ed->dmgtype);
						SetHP(GetHP()-1);//needed or else the client wont acknowledge
						break;
					} else if(GetInvul()) {
						Message(13, "Your invuln status protects you from %i points of type %i environmental damage.", ed->damage, ed->dmgtype);
						SetHP(GetHP()-1);//needed or else the client wont acknowledge
						break;
					}
					
					int damage = ed->damage;
					
					if (ed->dmgtype == 252) {
						if(CanUseSkill(SAFE_FALL)) {
							int sv = GetSkill(SAFE_FALL);
							//this is a total bullshit forumla, somebody find a better one
							if(MakeRandomInt(0,240) < sv/5)
								damage = 0;
							else if(sv > 2)
								damage = damage * 3 / sv;
							
							CheckIncreaseSkill(SAFE_FALL);
						}
						
						switch(GetAA(aaAcrobatics)) {
						case 1:
							damage = damage * 95 / 100;
							break;
						case 2:
							damage = damage * 90 / 100;
							break;
						case 3:
							damage = damage * 80 / 100;
							break;
						}
					}
					
					if(damage < 0)
						damage = 31337;

					else if(zone->GetZoneID() == 183 || zone->GetZoneID() == 184)
						break;
					else
						SetHP(GetHP() - damage);
					
					if(GetHP() <= 0)
						Death(0,32000);
					SendHPUpdate();
					break;
				}
				case OP_StartTribute:{
					if(app->size!=sizeof(StartTribute_Struct))
						printf("Error in OP_StartTribute.  Expected size of: %i, but got: %i\n",sizeof(StartTribute_Struct),app->size);
					else{
						StartTribute_Struct* st = (StartTribute_Struct*)app->pBuffer;
						Mob* tribmast=entity_list.GetMob(st->npc_id);
						if(tribmast && tribmast->GetClass()==TRIBUTE_MASTER){
							st->response=1;
							QueuePacket(app);
						}
						else{
							st->response=0;
							QueuePacket(app);
						}
					}
					break;
				}
				case OP_Damage: {
					// Broadcast to other clients
					entity_list.QueueClients(this, app, false);
					break;
				}
				case OP_AAAction: {
					//DumpPacket(app);
					if(app->size!=sizeof(AA_Action)){
						printf("Error! OP_AAAction size didnt match!\n");
					break;
					}
					AA_Action* action=(AA_Action*)app->pBuffer;
					
					if(action->action == aaActionActivate)//AA Hotkey
						ActivateAA((aaID) action->ability);
					else if(action->action == aaActionBuy) {
						BuyAA(action);
					}
					else if(action->action == aaActionDisableEXP){ //Turn Off AA Exp
						m_pp.perAA = 0;
						SendAAStats();
					} else if(action->action == aaActionSetEXP) {
						m_pp.perAA = action->exp_value;
						if (m_pp.perAA<0 || m_pp.perAA>100) m_pp.perAA=0;	// stop exploit with sanity check
						// send an update
						SendAAStats();
						SendAATable();
					} else {
						printf("Unknown AA action: %lu %lu 0x%x %d\n", action->action, action->ability, action->unknown08, action->exp_value);
					}
					
					break;
				}
				case OP_TraderBuy:{
					if(app->size==sizeof(TraderBuy_Struct)){
						TraderBuy_Struct* tbs = (TraderBuy_Struct*)app->pBuffer;
						if(Client* trader=entity_list.GetClientByID(tbs->traderid)){
							BuyTraderItem(tbs,trader,app);
						}
					}
					break;
				}
				case OP_Trader:{
					if(app->size==sizeof(Trader_ShowItems_Struct)){ //Show Items
						Trader_ShowItems_Struct* sis = (Trader_ShowItems_Struct*)app->pBuffer;
						if(sis->code==2){//end trader
							this->Trader_EndTrader();
						}
						else if(sis->code==4){ //end transaction
							Client* tmp=entity_list.GetClientByID(sis->traderid);
							if(tmp)
								tmp->withcustomer=false;
						}
						else if(sis->code==11){
							this->Trader_ShowItems();
						}
					}
					else if(app->size==sizeof(ClickTrader_Struct)){
						ClickTrader_Struct* ints = (ClickTrader_Struct*)app->pBuffer;
						if(ints->code==1){
							GetItems_Struct* gis=GetTraderItems();
							for(int i=0;i<80;i++){
								if(gis->items[i]>0 && gis->items[i]<database.GetMaxItem() && database.GetItem(gis->items[i])!=0)
									database.SaveTraderItem(this->CharacterID(),gis->items[i],ints->itemcost[i],i);
								else
									break; //sony doesnt memset so assume done on first bad item
							}
							safe_delete(gis);
							this->Trader_StartTrader();
						}
						else
							LogFile->write(EQEMuLog::Error, "Unknown TraderStruct code of: %i\n", ints->code);
					}
					else{
						LogFile->write(EQEMuLog::Error, "Unknown size for OP_Trader: %i\n", app->size);
						DumpPacket(app);
						break;
					}
					
					break;
				}
				case OP_GMFind: {
					if (this->Admin() < 80) {
						Message(13, "Your account has been reported for hacking.");
						database.SetHackerFlag(this->account_name, this->name, "/find");
						break;
					}
					//Break down incoming
					GMSummon_Struct* request=(GMSummon_Struct*)app->pBuffer;
					//Create a new outgoing
					APPLAYER *outapp = new APPLAYER(OP_GMFind, sizeof(GMSummon_Struct));
					GMSummon_Struct* foundplayer=(GMSummon_Struct*)outapp->pBuffer;
					//Copy the constants
					strcpy(foundplayer->charname,request->charname);
					strcpy(foundplayer->gmname, request->gmname);
					//Check if the NPC exits intrazone...
					Mob* gt = entity_list.GetMob(request->charname);
					if (gt != 0) {
						foundplayer->success=1;
						foundplayer->x=(sint32)gt->GetX();
						foundplayer->y=(sint32)gt->GetY();

						foundplayer->z=(sint32)gt->GetZ();
						foundplayer->zoneID=zone->GetZoneID();
					}
					//Send the packet...
					FastQueuePacket(&outapp);
					break;
				}
				case OP_PickPocket: { // Pickpocket
					if (app->size != sizeof(PickPocket_Struct)){
						LogFile->write(EQEMuLog::Error, "Size mismatch for Pick Pocket packet");
						DumpPacket(app);
					}
					PickPocket_Struct* pick_in = (PickPocket_Struct*) app->pBuffer;

					//APPLAYER* outapp = new APPLAYER(OP_PickPocket, sizeof(sPickPocket_Struct));
					//sPickPocket_Struct* pick_out = (sPickPocket_Struct*) outapp->pBuffer;
					Mob* victim = entity_list.GetMob(pick_in->to);
					if (!victim)
						break;
					if (victim == this)
						Message(0,"You catch yourself red-handed.");
					else if (victim->GetOwnerID())
						Message(0,"You cannot steal from pets!");
					else if (victim->IsNPC())
						victim->CastToNPC()->PickPocket(this);
					else
						Message(0,"Stealing from clients not yet supported.");
					//safe_delete(outapp);

/*
					APPLAYER* outapp = new APPLAYER(OP_PickPocket, sizeof(sPickPocket_Struct));
					sPickPocket_Struct* pick_out = (sPickPocket_Struct*) outapp->pBuffer;
					Mob* victim = entity_list.GetMob(pick_in->to);
					if (!victim)
						break;
					if (pick_in->myskill == 0) {
						LogFile->write(EQEMuLog::Debug,
							"Client pick pocket response");
						DumpPacket(app);
						safe_delete(outapp);
						break;
					}
					uint8 success = 0;
					if (RandomTimer(1,900) >= (GetSkill(PICK_POCKETS)+GetDEX())){
						success = RandomTimer(1,5);
						if (GetSkill(PICK_POCKETS) >=200)
							success += 1;
					}
					if ( (victim->IsNPC() && victim->CastToNPC()->IsInteractive()) || (victim->IsClient()) ) {
						if (EQDEBUG>=5) LogFile->write(EQEMuLog::Debug, "PickPocket picking ipc/client");
						pick_out->to   = pick_in->from;
						pick_out->from = pick_in->to;
						if (victim->IsClient() && !success){
								victim->CastToClient()->QueuePacket(app);
						}
						else if (victim->IsNPC()
						&& victim->CastToNPC()->IsInteractive()
						&& GetPVP() 
						&& (GetPVP() == victim->CastToClient()->GetPVP())
						&& !success

						) {
						int16 hate = RandomTimer(1,503);
						int16 tmpskill = (GetSkill(PICK_POCKETS)+GetDEX());
						if( hate >= tmpskill ){
							victim->AddToHateList(this, 1, 0);
							entity_list.MessageClose(victim, true, 200, 0, "%s says, your not as quick as you thought you were thief!", victim->GetName());
						}
					}
					if(!success){
						// Failed
						QueuePacket(outapp);
					}
					else if (success) {
						pick_out->to   = pick_in->from;
						pick_out->from = pick_in->to;

						pick_out->type = success;
						pick_out->coin = (RandomTimer(1,RandomTimer(2,10)));
						switch (success){
							case 1:
								AddMoneyToPP(0,0,0,pick_out->coin, false);
								break;
							case 2:
								AddMoneyToPP(0,0,pick_out->coin,0, false);
								break;
							case 3:
								AddMoneyToPP(0,pick_out->coin,0,0, false);
								break;
							case 4:
								AddMoneyToPP(pick_out->coin,0,0,0, false);
								break;
							case 5:
							case 6: {
								// Item
								// @merth: This needs to be redone with new item classes
								
								APPLAYER* outapp2 = new APPLAYER(OP_PickPocket, sizeof(sItem_PickPocket_Struct));
								sItem_PickPocket_Struct* pick_out2 = (sItem_PickPocket_Struct*) outapp2->pBuffer;
								pick_out2->to   = pick_in->from;
								pick_out2->from = pick_in->to;
								pick_out2->type = success;
								// FIXME: This should search the npc inventory for an item of fail the Pickpocket attempt
								const Item_Struct* temp_item = 0;
								while (!temp_item) {
									temp_item = database.GetItem(RandomTimer(1001,32768));
								}
								//Inventory[30] = temp_item->ItemNumber;
								PutItemInInventory(30, temp_item);
								memcpy(&pick_out2->item, temp_item, sizeof(Item_Struct));
								pick_out2->item.CurrentEquipSlot = 30;
								QueuePacket(outapp2);
								safe_delete(outapp2);
								
								break;
							}
							default:
								LogFile->write(EQEMuLog::Error,"Unknown success in OP_PickPocket %i", __LINE__); break;
						}
						if (success!=5)
							QueuePacket(outapp);
					}
					else {
						LogFile->write(EQEMuLog::Error, "Unknown error in OP_PickPocket");
					}
				}
				else {
					if (EQDEBUG>=5)
						LogFile->write(EQEMuLog::Debug, "PickPocket picking npc");
					if (success) {
						pick_out->to   = pick_in->from;
						pick_out->from = pick_in->to;
						pick_out->type = success;
						pick_out->coin = (RandomTimer(1,RandomTimer(2,10)));
						switch (success){
							case 1:
								AddMoneyToPP(0,0,0,pick_out->coin, false);
								break;
							case 2:
								AddMoneyToPP(0,0,pick_out->coin,0, false);
								break;
							case 3:
								AddMoneyToPP(0,pick_out->coin,0,0, false);
								break;
							case 4:
								AddMoneyToPP(pick_out->coin,0,0,0, false);
								break;
							case 5:
							case 6: {
								// Item
								// @merth: This needs to be redone with new item classes
								
								APPLAYER* outapp2 = new APPLAYER(OP_PickPocket, sizeof(sItem_PickPocket_Struct));
								sItem_PickPocket_Struct* pick_out2 = (sItem_PickPocket_Struct*) outapp2->pBuffer;
								pick_out2->to   = pick_in->from;
								pick_out2->from = pick_in->to;
								pick_out2->type = success;
								// FIXME: This should search the npc inventory for an item of fail the Pickpocket attempt
								const Item_Struct* temp_item = 0;
								while (!temp_item) {
									temp_item = database.GetItem(RandomTimer(1001,32768));
								}
								memcpy(&pick_out2->item, temp_item, sizeof(Item_Struct));
								pick_out2->item.CurrentEquipSlot = 30;
								QueuePacket(outapp2);
								safe_delete(outapp2);
								break;
								
							}
							default:
								LogFile->write(EQEMuLog::Error,"Unknown success in OP_PickPocket %i", __LINE__); break;
						}
						if (success!=5)
							QueuePacket(outapp);
					}
					else {
						int16 hate = RandomTimer(1,503);
						int16 tmpskill = (GetSkill(PICK_POCKETS)+GetDEX());
						if( hate >= tmpskill ){
							victim->AddToHateList(this, 1, 0);
								entity_list.MessageClose(victim, true, 200, 0, "%s says, your not as quick as you thought you were thief!", victim->GetName());
						}
						QueuePacket(outapp);
					}
				}
					safe_delete(outapp);
					break;
				*/
				}
				case OP_Bind_Wound:{
					if (app->size != sizeof(BindWound_Struct)){
						LogFile->write(EQEMuLog::Error, "Size mismatch for Bind wound packet");
						DumpPacket(app);
					}
					BindWound_Struct* bind_in = (BindWound_Struct*) app->pBuffer;
					Mob* bindmob = entity_list.GetMob(bind_in->to);
					if (!bindmob){
					    LogFile->write(EQEMuLog::Error, "Bindwound on non-exsistant mob from %s", this->GetName());
					}
					LogFile->write(EQEMuLog::Debug, "BindWound in: to:\'%s\' from=\'%s\'", bindmob->GetName(), GetName());
					BindWound(bindmob, true);
					break;
				}
				case OP_TrackTarget:{
					// Looks like an entityid should probably do something with it.
					IsTracking=(IsTracking==false);
					break;
				}
				case OP_Track:{
					IsTracking=false;
					if(GetClass() != RANGER && GetClass() != DRUID && GetClass() != BARD)
						break;
					
					if(!p_timers.Expired(pTimerTracking, false)) {
						Message(13,"Ability recovery time not yet met.");
						break;
					}
					p_timers.Start(pTimerTracking, TrackingReuseTime-1);
					
					if( GetSkill(TRACKING)==0 )
						SetSkill(TRACKING,1);
					else
						CheckIncreaseSkill(TRACKING,15); 

					entity_list.MakeTrackPacket(this);
					break;
				}
				case OP_TrackUnknown:{
					// size 0 send right after OP_Track
					break;
				}
				/*case 0x0193: {
					// Not sure what this opcode does.  It started being sent when OP_ClientUpdate was
					// changed to pump OP_ClientUpdate back out instead of OP_MobUpdate
					// 2 bytes: 00 00
				}*/
#if 0	// solar: disabled 2/13/04
				case 0x01e7: {
					// Dunno what this opcode does but client needs an answer
					if(app->size==8){
						app->pBuffer[4]=0;
						app->pBuffer[5]=0;
						app->pBuffer[6]=0;
						app->pBuffer[7]=0;
						QueuePacket(app);
					}
					break;
				}
#endif
				case OP_ClientError: {
					ClientError_Struct* error = (ClientError_Struct*)app->pBuffer;
					LogFile->write(EQEMuLog::Error, "Client error: %s", error->character_name);
					LogFile->write(EQEMuLog::Error, "Error message:%s", error->message);
					//if (EQDEBUG>=5)
					//	DumpPacket(app);
					break;
				}
				case OP_ReloadUI:{ //put anything that needs to be resent here
					if(guilddbid>0 && guilddbid<0xFFFFFFFF)
						SendGuildMembers(guilddbid);
					break;
				}
				case OP_TGB: {
					OPTGB(app);
					break;
				}
				case OP_Split: {
					// solar: the client removes the money on its own, but we have to
					// update our state anyway, and make sure they had enough to begin
					// with.
					Split_Struct *split = (Split_Struct *)app->pBuffer;
					//Implemented by Father Nitwit
					//Per the note above, Im not exactly sure what to do on error
					//to notify the client of the error...
					if(!isgrouped) {
						Message(13, "You can not split money if your not in a group.");
						break;
					}
					Group *cgroup = GetGroup();
					if(cgroup == NULL) {
						//invalid group, not sure if we should say more...
						Message(13, "You can not split money if your not in a group.");
						break;
					}
					
					if(!TakeMoneyFromPP(split->copper + 10 * split->silver + 100 * split->gold + 1000 * split->platinum)) {
						Message(13, "You do not have enough money to do that split.");
						break;
					}
					cgroup->SplitMoney(split->copper, split->silver, split->gold, split->platinum);
					
					break;
				}
				
				case OP_SenseTraps:
				{
					if (!CanUseSkill(SENSE_TRAPS))
						break;
					
					if(!p_timers.Expired(pTimerSenseTraps, false)) {
						Message(13,"Ability recovery time not yet met.");
						break;
					}
					int reuse = SenseTrapsReuseTime;
					switch(GetAA(aaAdvTrapNegotiation)) {
						case 1:
							reuse = reuse * 90/100;
							break;
						case 2:
							reuse = reuse * 75/100;
							break;
						case 3:
							reuse = reuse * 50/100;
							break;
					}
					p_timers.Start(pTimerSenseTraps, reuse-1);
					
					Trap* trap = entity_list.FindNearbyTrap(this,100);
					
					CheckIncreaseSkill(SENSE_TRAPS);
					
					if (trap && trap->skill > 0) {
						int uskill = GetSkill(SENSE_TRAPS);
						if ((MakeRandomInt(0,99) + uskill) >= (MakeRandomInt(0,99) + trap->skill*0.75))
						{
							float xdif = trap->x - GetX();
							float ydif = trap->y - GetY();
							if (xdif == 0 && ydif == 0)
								Message(MT_Skills,"You sense a trap right under your feet!");
							else if (xdif > 10 && ydif > 10)
								Message(MT_Skills,"You sense a trap to the NorthWest.");
							else if (xdif < -10 && ydif > 10)
								Message(MT_Skills,"You sense a trap to the NorthEast.");
							else if (ydif > 10)
								Message(MT_Skills,"You sense a trap to the North.");
							else if (xdif > 10 && ydif < -10)
								Message(MT_Skills,"You sense a trap to the SouthWest.");
							else if (xdif < -10 && ydif < -10)
								Message(MT_Skills,"You sense a trap to the SouthEast.");
							else if (ydif < -10)
								Message(MT_Skills,"You sense a trap to the South.");
							else if (xdif > 10)
								Message(MT_Skills,"You sense a trap to the West.");
							else
								Message(MT_Skills,"You sense a trap to the East.");
							trap->detected = true;
							break;
						}
					}
					Message(MT_Skills,"You did not find any traps nearby.");
					break;
				}
				case OP_DisarmTraps:
				{
					if (!CanUseSkill(DISARM_TRAPS))
						break;
					
					if(!p_timers.Expired(pTimerSenseTraps, false)) {
						Message(13,"Ability recovery time not yet met.");
						break;
					}
					int reuse = SenseTrapsReuseTime;
					switch(GetAA(aaAdvTrapNegotiation)) {
						case 1:
							reuse = reuse * 90/100;
							break;
						case 2:
							reuse = reuse * 75/100;
							break;
						case 3:
							reuse = reuse * 50/100;
							break;
					}
					p_timers.Start(pTimerSenseTraps, reuse-1);
					
					Trap* trap = entity_list.FindNearbyTrap(this,40);
					if (trap && trap->detected)
					{
						int uskill = GetSkill(DISARM_TRAPS);
						if ((MakeRandomInt(0, 49) + uskill) >= (MakeRandomInt(0, 49) + trap->skill))
						{
							Message(MT_Skills,"You disarm a trap.");
							trap->disarmed = true;
							trap->respawn_timer.Start(6000000);
						}
						else
						{
							Message(MT_Skills,"You set off the trap while trying to disarm it!");
							trap->Trigger(this);
						}
						CheckIncreaseSkill(DISARM_TRAPS);
						break;
					}
					Message(MT_Skills,"You did not find any traps close enough to disarm.");
					break;
				}
				case OP_CrashDump:
				case OP_ControlBoat:
				case OP_DumpName:
				case OP_SetRunMode:
				case OP_SafeFallSuccess:
				case OP_Heartbeat:
				case OP_SafePoint:
					break;
				default: {
					cout << "Unknown opcode: 0x" << hex << setfill('0') << setw(4) << app->opcode << dec
						<< " size:" << app->size << " Client:" << GetName() << endl;
					if(app->size<1000)
						DumpPacket(app->pBuffer, app->size);
					else{
						cout << "Dump limited to 1000 characters:\n";
						DumpPacket(app->pBuffer, 1000);
					}
					break;
				}
			}
			break;
		}
		case CLIENT_KICKED:
		case DISCONNECTED:
		case CLIENT_LINKDEAD:
			break;
		default: {
			cerr << "Unknown client_state:" << (int16) client_state << endl;
			break;
		}
	}
	
	return ret;
}

void Client::DBAWComplete(int8 workpt_b1, DBAsyncWork* dbaw) {
	Entity::DBAWComplete(workpt_b1, dbaw);
	switch (workpt_b1) {
		case DBA_b1_Entity_Client_InfoForLogin: {
			if (!FinishConnState2(dbaw))
				client_state = CLIENT_ERROR;
			break;
		}
		case DBA_b1_Entity_Client_Save: {
			char errbuf[MYSQL_ERRMSG_SIZE];
			int32 affected_rows = 0;
			DBAsyncQuery* dbaq = dbaw->PopAnswer();
			if (dbaq->GetAnswer(errbuf, 0, &affected_rows) && affected_rows == 1) {
				if (dbaq->QPT())
					SaveBackup();
			}
			else {
				cout << "Async client save failed. '" << errbuf << "'" << endl;
				Message(13, "Error: Asyncronous save of your character failed.");
				if (Admin() >= 200)
					Message(13, "errbuf: %s", errbuf);
			}
			pQueuedSaveWorkID = 0;
			break;
		}
		default: {
			cout << "Error: Client::DBAWComplete(): Unknown workpt_b1" << endl;
			break;
		}
	}
}

bool Client::FinishConnState2(DBAsyncWork* dbaw) {
	uint32 pplen = 0;
	DBAsyncQuery* dbaq = 0;
	APPLAYER* outapp = 0;
	MYSQL_RES* result = 0;
	bool loaditems = 0;
	char errbuf[MYSQL_ERRMSG_SIZE];
	int i;
	
	for (i=1; i<=3; i++) {
		dbaq = dbaw->PopAnswer();
		if (!dbaq) {
			cout << "Error in FinishConnState2(): dbaq==0" << endl;
			return false;
		}
		if (!dbaq->GetAnswer(errbuf, &result)) {
			cout << "Error in FinishConnState2(): !dbaq[" << dbaq->QPT() << "]->GetAnswer(): " << errbuf << endl;
			return false;
		}
		if (dbaq->QPT() == 1) {
			database.GetAccountInfoForLogin_result(result, 0, account_name, &lsaccountid, &gmspeed, &revoked);
		}
		else if (dbaq->QPT() == 2) {
			loaditems = database.GetCharacterInfoForLogin_result(result, 0, 0, &m_pp, &m_inv, &pplen, &guilddbid, &guildrank);
		}
		else if (dbaq->QPT() == 3) {
			database.LoadFactionValues_result(result, &factionvalue_list);
		}
		else {
			cout << "Error in FinishConnState2(): dbaq->PQT() unknown" << endl;
			return false;
		}
	}
	
	char temp1[64];
	if (database.GetVariable("Max_AAXP", temp1, sizeof(temp1)-1)) {
		max_AAXP = (atoi(temp1)*16)/10;
	}
	
	//int32 aalen = database.GetPlayerAlternateAdv(account_id, name, &aa);
	//if (aalen == 0) {
	//	cout << "Client dropped: !GetPlayerAlternateAdv, name=" << name << endl;
	//	return false;
	//}
	
	
	
	////////////////////////////////////////////////////////////	// Player Profile Packet
	// Try to find the EQ ID for the guild, if doesnt exist, guild has been deleted.

	// Clear memory, but leave it in the DB (no reason not to, guild might be restored?)
	strcpy(name, m_pp.name);
	strcpy(lastname, m_pp.last_name);
	if((m_pp.x == -1 && m_pp.y == -1 && m_pp.z == -1)||(m_pp.x == -2 && m_pp.y == -2 && m_pp.z == -2)) {
		m_pp.x = zone->safe_x();
		m_pp.y = zone->safe_y();
		m_pp.z = zone->safe_z();
	}

	x_pos		= m_pp.x;
	y_pos		= m_pp.y;
	z_pos		= m_pp.z;
	heading		= m_pp.heading;
	race		= m_pp.race;
	base_race	= m_pp.race;
	class_		= m_pp.class_;
	gender		= m_pp.gender;
	base_gender	= m_pp.gender;
	level		= m_pp.level;
	deity		= m_pp.deity;//FYI: DEITY_AGNOSTIC = 396; still valid?
	haircolor	= m_pp.haircolor;
	beardcolor	= m_pp.beardcolor;
	eyecolor1	= m_pp.eyecolor1;
	eyecolor2	= m_pp.eyecolor2;
	hairstyle	= m_pp.hairstyle;
	luclinface	= m_pp.face;
// vesuvias - appearence fix
	beard		= m_pp.beard;
	
	
	//if we zone in with invalid Z, fix it.
	if (zone->map != NULL) {
		
		//for whatever reason, LineIntersectsNode is giving better results than FindBestZ
		
		NodeRef pnode;
		VERTEX me;
		me.x = GetX();
		me.y = GetY();
		me.z = GetZ() + (GetSize()==0.0?6:GetSize());
		pnode = zone->map->SeekNode( zone->map->GetRoot(), me.x, me.y );
		
		VERTEX hit;
		VERTEX below_me(me);
		below_me.z -= 500;
		if(!zone->map->LineIntersectsNode(pnode, me, below_me, &hit, NULL) || hit.z < -5000) {
#if EQDEBUG >= 5
			LogFile->write(EQEMuLog::Debug, "Player %s started below the zone trying to fix! (%.3f, %.3f, %.3f)", GetName(), me.x, me.y, me.z);
#endif
			//theres nothing below us... try to find something to stand on
			me.z += 200;	//arbitrary #
			if(zone->map->LineIntersectsNode(pnode, me, below_me, &hit, NULL)) {
				//+10 so they dont stick in the ground
				SendTo(me.x, me.y, hit.z + 10);
				m_pp.z = hit.z + 10;
			} else {
				//one more, desperate try
				me.z += 2000;
				if(zone->map->LineIntersectsNode(pnode, me, below_me, &hit, NULL)) {
				//+10 so they dont stick in the ground
					SendTo(me.x, me.y, hit.z + 10);
					m_pp.z = hit.z + 10;
				}
			}
		}
	}

	//m_pp.hunger_level = 6000;
	//m_pp.thirst_level = 6000;
	
	//aa_title	= m_pp.aa_title;
	//m_pp.timeplayed=64;
	//m_pp.birthday=1057434792;
	//m_pp.lastlogin=1057464792;

	if (m_pp.gm && admin < 80)
		m_pp.gm = 0;
	
	if (m_pp.platinum < 0 || m_pp.gold < 0 || m_pp.silver < 0 || m_pp.copper < 0 || m_pp.platinum > 1000000 || m_pp.gold > 1000000 || m_pp.silver > 1000000 || m_pp.copper > 1000000)
	{
		m_pp.platinum = 0;
		m_pp.gold = 0;
		m_pp.silver = 0;
		m_pp.copper = 0;
	}
	guildeqid = database.GetGuildEQID(guilddbid);
	if (guildeqid == GUILD_NONE) {
		guilddbid = 0;
		guildrank = GUILD_MEMBER;
		m_pp.guildid = 0xFFFFFFFF;
	}
	else
		m_pp.guildid = guildeqid;
	
	switch (race)
	{
		case OGRE:
			size = 9;break;
		case TROLL:
			size = 8;break;
		case VAHSHIR:

		case FROGLOK: //Frog
		case BARBARIAN:
			size = 7;break;
		case HUMAN:
		case HIGH_ELF:
		case ERUDITE:
		case IKSAR:
			size = 6;break;
		case HALF_ELF:
			size = 5.5;break;
		case WOOD_ELF:
		case DARK_ELF:
			size = 5;break;
		case DWARF:
			size = 4;break;
		case HALFLING:
			size = 3.5;break;
		case GNOME:
			size = 3;break;
		default:
			size = 0;break;
	}
	
	//validate skills
	for (int sk = 1; sk < MAX_PP_SKILL; sk++) {
		//int cap = GetSkillCap(sk-1);
		int cap = MaxSkill(sk-1, GetClass(), GetLevel());
		if (cap >= 254)
			m_pp.skills[sk] = cap;
	}
	
	if(GetSkill(SWIMMING) < 100)
		SetSkill(SWIMMING,100);
#ifdef GUILDWARS
	m_pp.ldon_guk_points = 0;
	m_pp.ldon_mirugal_points = 0;
	m_pp.ldon_mistmoore_points = 0;
	m_pp.ldon_rujarkian_points = 0;
	m_pp.ldon_takish_points = 0;
	m_pp.ldon_available_points = 0;
	permitflag = false;
	if(m_pp.pvp)
		m_pp.pvp = false;
	if(m_pp.anon && Admin() == 0)
		m_pp.anon = 0;
	profit = 0;
	if(GuildDBID() != 0 && GuildRank() == 2)
	{
	GuildLocation* gl = 0;
	gl = location_list.FindClosestLocationByClient(this);
	if(gl != 0 && gl->GetLocationType() == CITY && gl->GetGuildOwner() == GuildDBID())
	{
	if(gl->GetProfit() > 0)
	{
	m_pp.platinum_bank += gl->GetProfit()/1000;
	profit = gl->GetProfit()/1000;
	gl->SetProfit(0);
	database.SetLocationProfit(gl->GetLocationID(),0);
	}
	}
	}
	if(GuildDBID() != 0)
	{
	this->castpercentbonus = location_list.GetCasterAttackBonus(GuildDBID());
	this->meleepercentbonus = location_list.GetMeleeAttackBonus(GuildDBID());
	}
					sint32 availpts = database.GetAvailablePoints(CharacterID(),0);
#ifdef GWDEBUG
					printf("Available points: %i for %s\n",availpts,GetName());
#endif
					if(availpts == -9999999)
					{
#ifdef GWDEBUG
					printf("Setting up points table for %s\n",GetName());
#endif
						database.SetupPointsTable(CharacterID(),0,GetName());
						availpts = 0;
					}
					if(availpts < 0)
					availpts = 0;

					m_pp.ldon_available_points = (int32)availpts;

						guildwars.SetCurrentUsers(numclients);
						if(guildwars.GetCurrentUsers() > guildwars.GetMaxUsers())
							guildwars.SetMaxUsers(numclients);
#endif

	if (spells_loaded)
	{
		for(int z=0;z<MAX_PP_MEMSPELL;z++)
		{
			if(m_pp.mem_spells[z] >= (int32)SPDAT_RECORDS)
				UnmemSpell(z, false);
		}

		for (i = 0; i < BUFF_COUNT; i++) {
			for(int z = 0; z < BUFF_COUNT; z++) {
			// check for duplicates
				if(buffs[z].spellid != SPELL_UNKNOWN && buffs[z].spellid == m_pp.buffs[i].spellid) {
					buffs[z].spellid = SPELL_UNKNOWN;
					m_pp.buffs[i].spellid = 0xFFFFFFFF;
				}
			}
			
			if (m_pp.buffs[i].spellid <= (int32)SPDAT_RECORDS && m_pp.buffs[i].spellid != 0 && m_pp.buffs[i].duration > 0) {
				if(m_pp.buffs[i].level == 0 || m_pp.buffs[i].level > 100)
					m_pp.buffs[i].level = 1;
				buffs[i].spellid			= m_pp.buffs[i].spellid;
				buffs[i].ticsremaining		= m_pp.buffs[i].duration;
				buffs[i].casterlevel		= m_pp.buffs[i].level;
				buffs[i].casterid			= 0;
				buffs[i].durationformula	= spells[buffs[i].spellid].buffdurationformula;
				buffs[i].poisoncounters		= m_pp.buffs[i].poisoncounters;
				buffs[i].diseasecounters	= m_pp.buffs[i].diseasecounters;
			}
			else {
				buffs[i].spellid = SPELL_UNKNOWN;
				m_pp.buffs[i].spellid = 0xFFFFFFFF;
				m_pp.buffs[i].slotid = 0;
				m_pp.buffs[i].level = 0;
				m_pp.buffs[i].duration = 0;
				m_pp.buffs[i].effect = 0;

			}
		}
		for (int j1=0; j1 < BUFF_COUNT; j1++) {
			if (buffs[j1].spellid <= (int32)SPDAT_RECORDS) {
				for (int x1=0; x1 < EFFECT_COUNT; x1++) {
					switch (spells[buffs[j1].spellid].effectid[x1]) {
						case SE_Charm:
						case SE_Rune:
						case SE_Illusion:
							buffs[j1].spellid = SPELL_UNKNOWN;
							m_pp.buffs[j1].spellid = SPELLBOOK_UNKNOWN;
							m_pp.buffs[j1].slotid = 0;
							m_pp.buffs[j1].level = 0;
							m_pp.buffs[j1].duration = 0;
							m_pp.buffs[j1].effect = 0;
							x1 = EFFECT_COUNT;
							break;
						// We can't send appearance packets yet, put down at CompleteConnect
					}
				}
			}
		}
		
		//Validity check for memorized
		for (int mem = 0; mem < 8; mem++)
		{
			if (m_pp.mem_spells[mem] < 1 || m_pp.mem_spells[mem] >= (unsigned int)SPDAT_RECORDS || spells[m_pp.mem_spells[mem]].classes[GetClass()-1] < 1 || spells[m_pp.mem_spells[mem]].classes[GetClass()-1] > GetLevel())
				m_pp.mem_spells[mem] = SPELLBOOK_UNKNOWN;
		}
		for (int bk = 0; bk < MAX_PP_SPELLBOOK; bk++)
		{
			if (m_pp.spell_book[bk] < 1 || m_pp.spell_book[bk] >= (unsigned int)SPDAT_RECORDS || spells[m_pp.spell_book[bk]].classes[GetClass()-1] < 1 || spells[m_pp.spell_book[bk]].classes[GetClass()-1] > 65)
				m_pp.spell_book[bk] = SPELLBOOK_UNKNOWN;
		}
	}
	
	/*
	if(this->isgrouped) {
		Group* group;
		group = GetGroup();
		for(int z=0; z<5; z++) {
			memset(m_pp.GMembers[z],0,sizeof(group->members[z]->GetName()));
		}
	}
	else {
		for(int z=0; z<5; z++) {
			memset(m_pp.GMembers[z],0,sizeof(m_pp.GMembers[0]));
		}
	}
	*/
	
	CalcBonuses();
	CalcMaxHP();
	CalcMaxMana();
	if (m_pp.cur_hp <= 0)
		m_pp.cur_hp = GetMaxHP();
	
	SetHP(m_pp.cur_hp);
	Mob::SetMana(m_pp.mana);
	
	m_pp.zone_change_count++;
	
	int32 groupid = database.GetGroupID(GetName());
#ifdef _EQDEBUG
		printf("Loaded group id %lu from DB.\n", groupid);
#endif
	Group* group = NULL;
	if(groupid > 0){
		group = entity_list.GetGroupByID(groupid);
		if(!group) {	//nobody from our is here... start a new group
#ifdef _EQDEBUG
			printf("Nobody in group is in this zone, making new group object.");
#endif
			group = new Group(groupid);
			if(group->GetID() != 0)
				entity_list.AddGroup(group, groupid);
			else	//error loading group members...
				group = NULL;
		}	//else, somebody from our group is allready here...
		
		if(group)
			group->UpdatePlayer(this);
		else
			database.SetGroupID(GetName(), 0);	//cannot re-establish group, kill it
		
	} else {	//no group id
		//clear out the group junk in our PP
		int xy=0;
		for(xy=0;xy < MAX_GROUP_MEMBERS;xy++)
			memset(m_pp.groupMembers[xy], 0, 64);
	}

	if(m_pp.z <= zone->newzone_data.underworld) {
		m_pp.x = zone->newzone_data.safe_x;
		m_pp.y = zone->newzone_data.safe_y;
		m_pp.z = zone->newzone_data.safe_z;
	}
	if(m_pp.class_==SHADOWKNIGHT || m_pp.class_==PALADIN){
		int32 abilitynum=0;
		if(m_pp.class_==SHADOWKNIGHT)
			abilitynum = pTimerHarmTouch;
		else
			abilitynum = pTimerLayHands;
		//int32 remaining = database.GetTimerRemaining(CharacterID(),abilitynum);
		
		//returns 0 or 0xFFFFFFFF if timer is not set.
		int32 remaining = p_timers.GetRemainingTime(abilitynum);
		
		if(remaining > 0 && remaining < 15300){
			m_pp.ability_down=1;
			m_pp.ability_up=0;
			m_pp.ability_number=abilitynum;
			int8 minutes=0;
			int8 hours=0;
			if(remaining>3600){
				hours=(remaining/3600);
				remaining=remaining-(hours*3600);
			}
			if(remaining>60){
				minutes=(remaining/60);
				remaining=remaining-(minutes*60);
			}
			m_pp.ability_time_minutes=minutes;
			m_pp.ability_time_seconds=remaining;
			m_pp.ability_time_hours=hours;
			AbilityTimer=true;
		}
		else{
			m_pp.ability_down=0;
			m_pp.ability_up=1;
			m_pp.ability_number=0;
			m_pp.ability_time_minutes=0;
			m_pp.ability_time_seconds=0;
			m_pp.ability_time_hours=0;
		}
	}
	char val[20] = {0};
	if (database.GetVariable("Expansions", val, 20))
		m_pp.expansion = atoi(val);
	else
		m_pp.expansion = 0xFF;
	
	p_timers.SetCharID(CharacterID());
	if(!p_timers.Load()) {
		//report it...
	}
	if(!p_timers.Expired(pTimerDisciplineReuse)) {
		//reset this so they get the avaliable message.
		disc_timer.Start(p_timers.GetRemainingTime(pTimerDisciplineReuse)*1000);
	}
#ifdef _EQDEBUG	
	printf("Dumping inventory on load:\n");
	m_inv.dumpInventory();
#endif
	strcpy(m_pp.servername,"eqemulator");
	m_pp.air_remaining = 60; //Reset to max so they dont drown on zone in if its underwater
	if(zone->IsPVPZone())
		m_pp.pvp=1;
	CRC32::SetEQChecksum((unsigned char*)&m_pp, sizeof(PlayerProfile_Struct)-4);
	outapp = new APPLAYER(OP_PlayerProfile,sizeof(PlayerProfile_Struct));
#ifdef SOLAR
	printf("PP size: %d\n", sizeof(PlayerProfile_Struct));
#endif
	memcpy(outapp->pBuffer,&m_pp,outapp->size);
	outapp->Deflate();
	outapp->priority = 6;
	QueuePacket(outapp);
	safe_delete(outapp);

	
	
	
	////////////////////////////////////////////////////////////
	// Server Zone Entry Packet
	outapp = new APPLAYER(OP_ZoneEntry, sizeof(ServerZoneEntry_Struct));
	ServerZoneEntry_Struct* sze = (ServerZoneEntry_Struct*)outapp->pBuffer;

	FillSpawnStruct(&sze->player,CastToMob());
	sze->player.spawn.cur_hp=1;
	sze->player.spawn.npc=0;
	sze->player.spawn.unknown367[0]=0xFFFFFFFF;
	sze->player.spawn.unknown367[1]=0xFFFFFFFF;
	QueuePacket(outapp);
	safe_delete(outapp);
	
	////////////////////////////////////////////////////////////
	// Zone Spawns Packet
	entity_list.SendZoneSpawnsBulk(this);
	entity_list.SendZoneCorpsesBulk(this);
	entity_list.SendTraders(this);
	
	
	
	////////////////////////////////////////////////////////////
	// Time of Day packet
	outapp = new APPLAYER(OP_TimeOfDay, sizeof(TimeOfDay_Struct));
	TimeOfDay_Struct* tod = (TimeOfDay_Struct*)outapp->pBuffer;
	zone->zone_time.getEQTimeOfDay(time(0), tod);
	outapp->priority = 6;
	QueuePacket(outapp);
	safe_delete(outapp);
	
	uchar blah[]={0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF};
	outapp = new APPLAYER(0x02f2,sizeof(blah));
	memcpy(outapp->pBuffer,blah,sizeof(blah));
	QueuePacket(outapp);
	safe_delete(outapp);
	
	////////////////////////////////////////////////////////////
	// Character Inventory Packet
	if (loaditems) //dont load if a length error occurs
		BulkSendInventoryItems();
	
	
	//////////////////////////////////////
	// Weather Packet
	outapp = new APPLAYER(OP_Weather, 12);
	if (zone->zone_weather == 1)
		outapp->pBuffer[4] = 0x31; // Rain
	if (zone->zone_weather == 2)
	{
		outapp->pBuffer[8] = 0x01;
		outapp->pBuffer[4] = 0x02;
	}
	outapp->priority = 6;
	QueuePacket(outapp);
	safe_delete(outapp);
	SetAttackTimer();
	return true;
}

// Finish client connecting state
void Client::CompleteConnect()
{

	hpupdate_timer.Start();
	position_timer.Start();
	SetDuelTarget(0);
	SetDueling(false);
		
	UpdateWho();
//	database.UpdateTimersClientConnected(CharacterID());
	client_state = CLIENT_CONNECTED;
	if(!m_inv[SLOT_CURSOR]){
		for(int ndx=0;ndx<10;ndx++){
			if(m_inv[8000+ndx]){
				if(!m_inv[SLOT_CURSOR]){//no item has been put on the cursor from the que
					m_inv.SwapItem(8000+ndx,SLOT_CURSOR);//put the next item in line onto the cursor
					const ItemInst* inst = m_inv[SLOT_CURSOR];
					if (inst)
						SendItemPacket(SLOT_CURSOR, inst, ItemPacketSummonItem);
				}
				else{//item is on cursor now
					m_inv.SwapItem(8000+ndx,8000+ndx-1);//move items ahead in the que
					const ItemInst* inst = m_inv[8000+ndx-1];
					if (inst)
						SendItemPacket(SLOT_CURSOR, inst, ItemPacketSummonItem);
				}
				DeleteItemInInventory(8000+ndx);//delete the source item
			}
		}
	}
	else{
		for(int ndx2=0;ndx2<10;ndx2++){
			const ItemInst* inst = m_inv[8000+ndx2];
			if (inst)
				SendItemPacket(SLOT_CURSOR, inst, ItemPacketSummonItem);
		}
	}
#ifdef GUILDWARS
	guildwars.EnteringMessages(this);
#endif

#ifdef RAIDADDICTS
	raidaddicts.ZoneIn(this);
#endif

	// Sets GM Flag if needed & Sends Petition Queue
	UpdateAdmin(false);

	if(GuildEQID()>0 && GuildEQID()<0xFFFFFFFF){
		SendAppearancePacket(AT_GuildID, GuildEQID(), false);
		SendAppearancePacket(AT_GuildRank, GuildRank(), false);
	}
	for(int spellInt= 0; spellInt < MAX_PP_SPELLBOOK; spellInt++)
	{
		if (m_pp.spell_book[spellInt] < 3 || m_pp.spell_book[spellInt] > 20000)
			m_pp.spell_book[spellInt] = 0xFFFFFFFF;
	}

	for(int a=0; a < MAX_PP_AA_ARRAY; a++){
		aa[a] = &m_pp.aa_array[a];
		int32 id = aa[a]->AA;
		if(aa[a]->value>1)
			aa_points[(id - aa[a]->value +1)] = aa[a]->value;
		else
			aa_points[id] = aa[a]->value;
	}
	SendAATable();
	
	//reapply some buffs
	for (uint32 j1=0; j1 < BUFF_COUNT; j1++) {
		if (buffs[j1].spellid > (int32)SPDAT_RECORDS)
			continue;
		
		for (int x1=0; x1 < EFFECT_COUNT; x1++) {
			switch (spells[buffs[j1].spellid].effectid[x1]) {
				case SE_Illusion: {
					if (spells[buffs[j1].spellid].base[x1] == -1)
					{
						if (gender == 1)
							gender = 0;
						else if (gender == 0)
							gender = 1;
						SendIllusionPacket(GetRace(), gender, 0xFFFF, 0xFFFF);
					}
					else if (spells[buffs[j1].spellid].base[x1] == -2)
					{
						if (GetRace() == 128 || GetRace() == 130 || GetRace() <= 12)
							SendIllusionPacket(GetRace(), GetGender(), spells[buffs[j1].spellid].max[x1], spells[buffs[j1].spellid].max[x1]);
					}
					else if (spells[buffs[j1].spellid].max[x1] > 0)
					{
						SendIllusionPacket(spells[buffs[j1].spellid].base[x1], 0xFF, spells[buffs[j1].spellid].max[x1], spells[buffs[j1].spellid].max[x1]);
					}
					else
					{
						SendIllusionPacket(spells[buffs[j1].spellid].base[x1], 0xFF, 0xFFFF, 0xFFFF);
					}
					break;
				}
				case SE_SummonHorse: {
					hasmount = true;	//this was false, is that the correct thing?
					break;
				}
				case SE_Rune: {
					BuffFadeBySpellID(buffs[j1].spellid);
					//SetRune(buffs[j1].durationformula);
					//Somehow we need to toss the remaining rune value over..
							  }
				case SE_DivineAura:
					{
					invulnerable = true;
					break;
					}
				case SE_Invisibility: 
					{
					invisible = true;
					SendAppearancePacket(AT_Invis, 1);
					break;
					}
				case SE_Levitate:
					{
					SendAppearancePacket(AT_Levitate, 2);
					break;
					}
				case SE_InvisVsUndead: 
					{
					invisible_undead = true;
					break;
					} 
			}
		}
	}
	
	//Remake pet
	if (!GetPet() && m_pp.pet_id > 1 && m_pp.pet_id <= SPDAT_RECORDS)
	{
		printf("Making pet with id %d\n", m_pp.pet_id);
		fflush(stdout);
		MakePet(m_pp.pet_id, spells[m_pp.pet_id].teleport_zone);
		if (GetPet())
			GetPet()->SetHP(m_pp.pet_hp);
	}
	m_pp.pet_id = 0;
	m_pp.pet_hp = 0;
	
	client_data_loaded = true;
	for(int x=0;x<8;x++)
		SendWearChange(x);
}


bool Client::Process() {
	adverrorinfo = 1;
	bool ret = true;
	//bool throughpacket = true;
	if (Connected() || IsLD())
	{
        // try to send all packets that weren't send before
		if(!IsLD())
			SendAllPackets();
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
#ifdef ITEMCOMBINED
void Client::BulkSendInventoryItems()
{
	// Search all inventory buckets for items
	APPLAYER* outapp = new APPLAYER(OP_CharInventory,0);
	uchar* buffer = 0;
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
	
	int buffptr=0;
	for (slot_id=0; slot_id<=30; slot_id++) {
		const ItemInst* inst = m_inv[slot_id];
		if (inst){
			APPLAYER* temp = ReturnItemPacket(slot_id, inst, ItemPacketCharInventory);
			if(!temp)
				continue;
			if(buffer)
			{
				uchar* newbuffer = new uchar[buffptr+temp->size+1];
				memcpy(newbuffer,buffer,buffptr);
				memcpy(&newbuffer[buffptr],temp->pBuffer,temp->size);
				buffptr=buffptr+temp->size+1;
				safe_delete(buffer);
				buffer = newbuffer;
			}
			else
			{
				buffptr=temp->size;
				buffer = new uchar[temp->size];
				memcpy(buffer,temp->pBuffer,buffptr);
			}
			safe_delete(temp);
		}
	}
	// Bank items
	for (slot_id=2000; slot_id<=2015; slot_id++) {
		const ItemInst* inst = m_inv[slot_id];
		if (inst){
			APPLAYER* temp = ReturnItemPacket(slot_id, inst, ItemPacketCharInventory);
			if(temp)
			{
			if(buffer)
			{
				uchar* newbuffer = new uchar[buffptr+temp->size+1];
				memcpy(newbuffer,buffer,buffptr);
				memcpy(&newbuffer[buffptr],temp->pBuffer,temp->size);
				buffptr=buffptr+temp->size+1;
				safe_delete(buffer);
				buffer = newbuffer;
			}
			else
			{
				buffptr=temp->size;
				buffer = new uchar[temp->size];
				memcpy(buffer,temp->pBuffer,buffptr);
			}
			safe_delete(temp);
			}
		}
	}
	
	// Shared Bank items
	for (slot_id=2500; slot_id<=2501; slot_id++) {
		const ItemInst* inst = m_inv[slot_id];
		if (inst){
			APPLAYER* temp = ReturnItemPacket(slot_id, inst, ItemPacketCharInventory);
			if(temp)
			{
			if(buffer)
			{
				uchar* newbuffer = new uchar[buffptr+temp->size+1];
				memcpy(newbuffer,buffer,buffptr);
				memcpy(&newbuffer[buffptr],temp->pBuffer,temp->size);
				buffptr=buffptr+temp->size+1;
				safe_delete(buffer);
				buffer = newbuffer;
			}
			else
			{
				buffptr=temp->size;
				buffer = new uchar[temp->size];
				memcpy(buffer,temp->pBuffer,buffptr);
			}
			safe_delete(temp);
			}
		}
	}
	outapp->size = buffptr;
	outapp->pBuffer = buffer;
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
#else
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
#endif
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
				inst->SetUnknown5(ml.slot+84);
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
				inst->SetUnknown5(ml.slot+84);
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
		sint32 dmg=(sint32) ((level/10)  * 3  * (GetSkill(BASH) + GetSTR() + level) / (700-GetSkill(BASH)));
		
		Message(MT_Emote, "You Bash for a total of %d damage.",  dmg);
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



