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
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <iostream>
using namespace std;

#ifdef WIN32
#include <process.h>
#else
#include <pthread.h>
#include "../common/unix.h"
#endif

#include "net.h"
#include "masterentity.h"
#include "worldserver.h"
#include "PlayerCorpse.h"
#include "../common/packet_dump.h"
#include "../common/packet_functions.h"
#include "petitions.h"
#include "spdat.h"

#ifdef WIN32
#define snprintf	_snprintf
#define vsnprintf	_vsnprintf
#define strncasecmp	_strnicmp
#define strcasecmp	_stricmp
#endif

extern Zone* zone;
extern volatile bool ZoneLoaded;
extern WorldServer worldserver;
extern NetConnection net;
extern GuildRanks_Struct guilds[512];
extern int32 numclients;
#ifndef NEW_LoadSPDat
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif
extern bool spells_loaded;
extern PetitionList petition_list;

extern char  errorname[32];
extern int16 adverrornum;

#ifdef GUILDWARS
#include "../GuildWars/GuildWars.h"
extern GuildLocationList location_list;
extern GuildWars guildwars;
#endif

Entity::Entity() {
	id = 0;
	pDBAsyncWorkID = 0;
}

Entity::~Entity() {
	dbasync->CancelWork(pDBAsyncWorkID);
}

void Entity::SetID(int16 set_id) {
	id = set_id;
}

Client* Entity::CastToClient() {
	if(this==0x00){
		cout << "CastToClient error" << endl;
		DebugBreak();
		return 0;
	}
#ifdef _EQDEBUG
	if(!IsClient()) {
		cout << "CastToClient error" << endl;
		DebugBreak();
		return 0;
	}
#endif
	return static_cast<Client*>(this);
}

NPC* Entity::CastToNPC() {
#ifdef _EQDEBUG
	if(!IsNPC()) {	
		cout << "CastToNPC error" << endl;
		DebugBreak();
		return 0;
	}
#endif
	return static_cast<NPC*>(this);
}

Mob* Entity::CastToMob() {
#ifdef _EQDEBUG
	if(!IsMob()) {	
		cout << "CastToMob error" << endl;
		DebugBreak();
		return 0;
	}
#endif
	return static_cast<Mob*>(this);
}

Corpse* Entity::CastToCorpse() {
#ifdef _EQDEBUG
	if(!IsCorpse()) {	
		cout << "CastToCorpse error" << endl;
		DebugBreak();
		return 0;
	}
#endif
	return static_cast<Corpse*>(this);
}
Object* Entity::CastToObject() {
#ifdef _EQDEBUG
	if(!IsObject()) {	
		cout << "CastToObject error" << endl;
		DebugBreak();
		return 0;
	}
#endif
	return static_cast<Object*>(this);
}

Group* Entity::CastToGroup() {
#ifdef _EQDEBUG
	if(!IsGroup()) {	
		cout << "CastToGroup error" << endl;
		DebugBreak();
		return 0;
	}
#endif
	return static_cast<Group*>(this);
}

Doors* Entity::CastToDoors() {
return static_cast<Doors*>(this);
}

Beacon* Entity::CastToBeacon() {
	return static_cast<Beacon*>(this);
}

bool EntityList::CanAddHateForMob(Mob *p) {
    LinkedListIterator<NPC*> iterator(npc_list);
    int count = 0;

    iterator.Reset();
    while( iterator.MoreElements())
    {
        NPC *npc=iterator.GetData();
        if (npc->IsOnHatelist(p))
            count++;
        // no need to continue if we already hit the limit
        if (count > 3)
            return false;
        iterator.Advance();
    }

    if (count <= 2)
        return true;
    return false;
}

void EntityList::AddClient(Client* client) {
	client->SetID(GetFreeID());
	client_list.Insert(client);
	mob_list.Insert(client);
	if(!client_list.dont_delete)
		client_list.dont_delete=true;
}
void EntityList::GroupProcess() {
	LinkedListIterator<Group*> iterator(group_list);
	iterator.Reset();
	int32 count=0;
	while(iterator.MoreElements())
	{
		count++;
		if(!iterator.GetData()->Process()){
			iterator.RemoveCurrent();
		}
		else
			iterator.Advance();
	}
	if(count==0)
		net.group_timer->Disable();//No groups in list, disable until one is added
}
void EntityList::DoorProcess() {
	LinkedListIterator<Doors*> iterator(door_list);
	iterator.Reset();
	int32 count=0;
	while(iterator.MoreElements())
	{
		count++;
		if(!iterator.GetData()->Process()){
			iterator.RemoveCurrent();
		}
		else
			iterator.Advance();
	}
	if(count==0)
		net.door_timer->Disable();//No doors in list, disable until one is added
}
void EntityList::ObjectProcess() {
	LinkedListIterator<Object*> iterator(object_list);
	iterator.Reset();
	int32 count=0;
	while(iterator.MoreElements())
	{
		count++;
		if(!iterator.GetData()->Process()){
			iterator.RemoveCurrent();
		}
		else
			iterator.Advance();
	}
	if(count==0)
		net.object_timer->Disable();//No objects in list, disable until one is added
}
void EntityList::CorpseProcess() {
	LinkedListIterator<Corpse*> iterator(corpse_list);
	iterator.Reset();
	int32 count=0;
	while(iterator.MoreElements())
	{
		count++;
		if(!iterator.GetData()->Process()){
			iterator.RemoveCurrent();
		}
		else
			iterator.Advance();
	}
	if(count==0)
		net.corpse_timer->Disable();//No corpses in list, disable until one is added
}
void EntityList::MobProcess() {
	LinkedListIterator<Mob*> iterator(mob_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
 		if(!iterator.GetData()->Process()){
			Mob* mob=iterator.GetData();
			if(mob->IsNPC())
				entity_list.RemoveNPC(mob->CastToNPC()->GetID());
			else{
#ifdef WIN32
					struct in_addr	in;
					in.s_addr = mob->CastToClient()->GetIP();
					cout << "Dropping client: Process=false, ip=" << inet_ntoa(in) << ", port=" << mob->CastToClient()->GetPort() << endl;
#endif
					zone->StartShutdownTimer();
					entity_list.RemoveClient(mob->GetID());
			}
			iterator.RemoveCurrent();
		}
		else
			iterator.Advance();
	}
}

void EntityList::BeaconProcess()
{
	LinkedListIterator<Beacon *> iterator(beacon_list);
	int count;

	for(iterator.Reset(), count = 0; iterator.MoreElements(); count++)
	{
		if(!iterator.GetData()->Process())
			iterator.RemoveCurrent();
		else
			iterator.Advance();
	}
}


void EntityList::AddGroup(Group* group) {
	group->SetID(GetFreeID());
	group_list.Insert(group);
	if(!net.group_timer->Enabled())
		net.group_timer->Start();
}

void EntityList::GuildItemAward(int32 guilddbid, int16 itemid)
{
if(itemid != 0)
{
	LinkedListIterator<Client*> iterator(client_list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GuildDBID() == guilddbid)
		{
			iterator.GetData()->SummonItem(itemid);
		}
		iterator.Advance();
	}
}
}
void EntityList::AddCorpse(Corpse* corpse, int32 in_id) {
	if (corpse == 0)
		return;
	
	if (in_id == 0xFFFFFFFF)
		corpse->SetID(GetFreeID());
	else
		corpse->SetID(in_id);
	corpse->CalcCorpseName();
	corpse_list.Insert(corpse);
	if(!net.corpse_timer->Enabled())
		net.corpse_timer->Start();
}

void EntityList::AddNPC(NPC* npc, bool SendSpawnPacket, bool dontqueue) {
	npc->SetID(GetFreeID());
	
	if (SendSpawnPacket) {
		if (dontqueue) { // aka, SEND IT NOW BITCH!
			APPLAYER* app = new APPLAYER;
			npc->CreateSpawnPacket(app,npc);
			//app->Deflate();
			QueueClients(npc, app);
			safe_delete(app);
			parse->Event(EVENT_SPAWN, npc->GetNPCTypeID(), 0, npc->CastToMob(), npc->CastToMob());

		}
		else {
			NewSpawn_Struct* ns = new NewSpawn_Struct;
			memset(ns, 0, sizeof(NewSpawn_Struct));
			npc->FillSpawnStruct(ns, 0);	// Not working on player newspawns, so it's safe to use a ForWho of 0
			AddToSpawnQueue(npc->GetID(), &ns);
			safe_delete(ns);
			parse->Event(EVENT_SPAWN, npc->GetNPCTypeID(), 0, npc->CastToMob(), 0);
		}
	}
	
	npc_list.Insert(npc);
	if(!npc_list.dont_delete)
		npc_list.dont_delete=true;
	mob_list.Insert(npc);
};
void EntityList::AddObject(Object* obj, bool SendSpawnPacket) {
	obj->SetID(GetFreeID()); 
	if (SendSpawnPacket) {
		APPLAYER app;
		obj->CreateSpawnPacket(&app);
		#if (EQDEBUG >= 5)
			DumpPacket(&app);
		#endif
		QueueClients(0, &app,false);
	}
	object_list.Insert(obj);
	if(!net.object_timer->Enabled())
		net.object_timer->Start();
};

void EntityList::AddDoor(Doors* door) {
	door->SetEntityID(GetFreeID());
	door_list.Insert(door);
	if(!net.door_timer->Enabled())
		net.door_timer->Start();
}

void EntityList::AddBeacon(Beacon *beacon)
{
	beacon->SetID(GetFreeID());
	beacon_list.Insert(beacon);
}

void EntityList::AddToSpawnQueue(int16 entityid, NewSpawn_Struct** ns) {
	SpawnQueue.Append(*ns);
	NumSpawnsOnQueue++;
	if (tsFirstSpawnOnQueue == 0xFFFFFFFF)
		tsFirstSpawnOnQueue = Timer::GetCurrentTime();
	*ns = 0; // make it so the calling function cant fuck us and delete the data =)
}

void EntityList::CheckSpawnQueue() {
	// Send the stuff if the oldest packet on the queue is older than 50ms -Quagmire
	if (tsFirstSpawnOnQueue != 0xFFFFFFFF && (Timer::GetCurrentTime() - tsFirstSpawnOnQueue) > 50) {
		if (NumSpawnsOnQueue <= 5) {
			LinkedListIterator<NewSpawn_Struct*> iterator(SpawnQueue);
			APPLAYER* outapp = 0;
			
			iterator.Reset();
			while(iterator.MoreElements()) {
				outapp = new APPLAYER;
				Mob::CreateSpawnPacket(outapp, iterator.GetData());
//				cout << "Sending spawn packet: " << iterator.GetData()->spawn.name << endl;
				//outapp->Deflate();
				QueueClients(0, outapp);
				safe_delete(outapp);
				iterator.RemoveCurrent();
			}
		}
		else {
			BulkZoneSpawnPacket* bzsp = new BulkZoneSpawnPacket(0, MAX_SPAWNS_PER_PACKET);
			LinkedListIterator<NewSpawn_Struct*> iterator(SpawnQueue);
			
			iterator.Reset();
			while(iterator.MoreElements()) {
				bzsp->AddSpawn(iterator.GetData());
				iterator.RemoveCurrent();
			}
			safe_delete(bzsp);
		}
		
		tsFirstSpawnOnQueue = 0xFFFFFFFF;
		NumSpawnsOnQueue = 0;
	}
}

Doors* EntityList::FindDoor(int8 door_id)
{
	if (door_id == 0)
		return 0;

	LinkedListIterator<Doors*> iterator(door_list);
	iterator.Reset();

	while(iterator.MoreElements())
	{
		Doors* door=iterator.GetData();
		if (door->GetDoorID() == door_id)
		{
			return door;
		}
		iterator.Advance();
	}
	return 0;
}

bool EntityList::MakeDoorSpawnPacket(APPLAYER* app)
{
	uchar packet_buffer[32767];
	int length, qty;
	Doors *door;
	LinkedListIterator<Doors*> iterator(door_list);
	Door_Struct nd;

	for
	(
		iterator.Reset(), qty = length = 0;
		iterator.MoreElements() && length + sizeof(nd) < 32767;
		iterator.Advance()
	)
	{
		door = iterator.GetData();
		if
		(
			door &&
			door->GetDoorID() != 0 &&
			!(
				door->GetDoorID() == 0xCD &&	// what's this?
				strlen(door->GetDoorName()) < 3
			)
		)
		{
			memset(&nd, 0, sizeof(nd));

			memcpy(nd.name, door->GetDoorName(), 16);
			nd.xPos = door->GetX();
			nd.yPos = door->GetY();
			nd.zPos = door->GetZ();
			nd.heading = door->GetHeading();
			nd.incline = door->GetIncline();
			nd.size = 100;	// 100 is normal size
			nd.doorId = door->GetGuildID() ? door->GetGuildID() : door->GetDoorID();				
			nd.opentype = door->GetOpenType();
			nd.state_at_spawn = door->GetInvertState() ? !door->IsDoorOpen() : door->IsDoorOpen();
			nd.invert_state = door->GetInvertState();

			switch(door->GetOpenType())
			{
				case 0x3B:	// vertical movement
					nd.door_param = door->GetLiftHeight();
					break;
				case 0x3A:	// the PoK books, maybe other zone point type things too
					nd.door_param = 0; // solar TODO: zone point should go here
					break;
				default:
					nd.door_param = 0xFFFFFFFF;
			}
			
			// append it to the packet
			memcpy(packet_buffer + length, &nd, sizeof(nd));
			qty++;
			length += sizeof(nd);
		}
	}

#if EQDEBUG >= 5
	LogFile->write(EQEMuLog::Debug, "MakeDoorPacket() packet length:%i qty:%i", length, qty);
#endif

	if (qty == 0)
		return false;

	length = qty * sizeof(nd);
	app->opcode = OP_SpawnDoor;
	app->size = length;
	app->pBuffer = new uchar[length];
	memcpy(app->pBuffer, packet_buffer, length);

	return true;	
}
Entity* EntityList::GetEntityMob(int16 id){
	LinkedListIterator<Mob*> iterator(mob_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GetID() == id)
		{
			return iterator.GetData();
		}
		iterator.Advance();
	}
	return 0;
}
Entity* EntityList::GetEntityMob(const char *name)
{
	if (name == 0)
		return 0;

	LinkedListIterator<Mob*> iterator(mob_list);
	
	for(iterator.Reset(); iterator.MoreElements(); iterator.Advance())
	{
		if (strcasecmp(iterator.GetData()->GetName(), name) == 0)
		{
			return iterator.GetData();
		}
	}
	return 0;
}
Entity* EntityList::GetEntityDoor(int16 id){
	LinkedListIterator<Doors*> iterator(door_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GetID() == id)
		{
			return iterator.GetData();
		}
		iterator.Advance();
	}
	return 0;
}
Entity* EntityList::GetEntityCorpse(int16 id){
	LinkedListIterator<Corpse*> iterator(corpse_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GetID() == id)
		{
			return iterator.GetData();
		}
		iterator.Advance();
	}
	return 0;
}
Entity* EntityList::GetEntityCorpse(const char *name)
{
	if (name == 0)
		return 0;

	LinkedListIterator<Corpse*> iterator(corpse_list);
	
	for(iterator.Reset(); iterator.MoreElements(); iterator.Advance())
	{
		if (strcasecmp(iterator.GetData()->GetName(), name) == 0)
		{
			return iterator.GetData();
		}
	}
	return 0;
}
Entity* EntityList::GetEntityObject(int16 id){
	LinkedListIterator<Object*> iterator(object_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GetID() == id)
		{
			return iterator.GetData();
		}
		iterator.Advance();
	}
	return 0;
}
Entity* EntityList::GetEntityGroup(int16 id){
	LinkedListIterator<Group*> iterator(group_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GetID() == id)
		{
			return iterator.GetData();
		}
		iterator.Advance();
	}
	return 0;
}
Entity* EntityList::GetEntityBeacon(int16 id){
	LinkedListIterator<Beacon*> iterator(beacon_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GetID() == id)
		{
			return iterator.GetData();
		}
		iterator.Advance();
	}
	return 0;
}
Entity* EntityList::GetID(int16 get_id)
{
	Entity* ent=0;
	if((ent=entity_list.GetEntityMob(get_id))!=0)
		return ent;
	else if((ent=entity_list.GetEntityDoor(get_id))!=0)
		return ent;
	else if((ent=entity_list.GetEntityCorpse(get_id))!=0)
		return ent;
	else if((ent=entity_list.GetEntityGroup(get_id))!=0)
		return ent;
	else if((ent=entity_list.GetEntityObject(get_id))!=0)
		return ent;
	else if((ent=entity_list.GetEntityBeacon(get_id))!=0)
		return ent;
	else
		return 0;
}

Mob* EntityList::GetMob(int16 get_id)
{
	Entity* ent=0;

	if (get_id == 0)
		return 0;

	if((ent=entity_list.GetEntityMob(get_id))!=0)
		return ent->CastToMob();
	else if((ent=entity_list.GetEntityCorpse(get_id))!=0)
		return ent->CastToMob();

	return 0;
}

Mob* EntityList::GetMob(const char* name)
{
	Entity* ent=0;

	if (name == 0)
		return 0;

	if((ent=entity_list.GetEntityMob(name))!=0)
		return ent->CastToMob();
	else if((ent=entity_list.GetEntityCorpse(name))!=0)
		return ent->CastToMob();

	return 0;
}

Mob* EntityList::GetMobByNpcTypeID(int32 get_id)
{
	if (get_id == 0)
		return 0;
	LinkedListIterator<Mob*> iterator(mob_list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GetNPCTypeID() == get_id)
		{
			return iterator.GetData();
		}
		iterator.Advance();
	}
	return 0;
}

int16 EntityList::GetFreeID()
{
	int16 getid=0;
	while(1)
	{
		getid++;
		if (GetID(getid) == 0)
		{
			return getid;
		}
	}
}

void EntityList::ChannelMessage(Mob* from, int8 chan_num, int8 language, const char* message, ...) {
	LinkedListIterator<Client*> iterator(client_list);
	va_list argptr;
	char buffer[4096];
	
	va_start(argptr, message);
	vsnprintf(buffer, 4096, message, argptr);
	va_end(argptr);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		Client* client = iterator.GetData();
		int8 filter=0;
		if(chan_num==3)//shout
			filter=FILTER_SHOUT;
		else if(chan_num==4) //auction
			filter=FILTER_AUCTION;
		if (chan_num != 8 || client->Dist(*from) < 200) // Only say is limited in range
		{
			if(filter==0 || (filter>0 && client->GetFilter(filter)!=0))
				client->ChannelMessageSend(from->GetName(), 0, chan_num, language, buffer);
		}
		iterator.Advance();
	}
}

void EntityList::ChannelMessageSend(Mob* to, int8 chan_num, int8 language, const char* message, ...) {
	LinkedListIterator<Client*> iterator(client_list);
	va_list argptr;
	char buffer[4096];
	va_start(argptr, message);
	vsnprintf(buffer, 4096, message, argptr);
	va_end(argptr);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		Client* client = iterator.GetData()->CastToClient();
		if (client->GetID() == to->GetID()) {
			client->ChannelMessageSend(0, 0, chan_num, language, buffer);
			break;
		}
		iterator.Advance();
	}
}

void EntityList::SendZoneSpawns(Client* client)
{
	LinkedListIterator<Mob*> iterator(mob_list);
	
	APPLAYER* app;
	iterator.Reset();
	while(iterator.MoreElements()) {
		Mob* ent = iterator.GetData();
		if (!( ent->InZone() ) || (ent->IsClient() && ent->CastToClient()->GMHideMe(client))) {
			iterator.Advance();
			continue;
		}
		app = new APPLAYER;
		iterator.GetData()->CastToMob()->CreateSpawnPacket(app); // TODO: Use zonespawns opcode instead
        client->QueuePacket(app, true, Client::CLIENT_CONNECTED);
		safe_delete(app);
		iterator.Advance();
	}	
}

void EntityList::SendZoneSpawnsBulk(Client* client)
{
	float rate = client->Connection()->GetDataRate();
	LinkedListIterator<Mob*> iterator(mob_list);
	NewSpawn_Struct ns;
	Mob *spawn;
	int32 maxspawns;

	rate = rate > 1.0 ? (rate < 10.0 ? rate : 10.0) : 1.0;
	maxspawns = (int32)rate * SPAWNS_PER_POINT_DATARATE; // FYI > 10240 entities will cause BulkZoneSpawnPacket to throw exception
	BulkZoneSpawnPacket* bzsp = new BulkZoneSpawnPacket(client, maxspawns);
	
	for(iterator.Reset(); iterator.MoreElements(); iterator.Advance())
	{
		spawn = iterator.GetData();
		if(spawn && spawn->InZone())
		{
			if(spawn->IsClient() && spawn->CastToClient()->GMHideMe(client))
				continue;
			memset(&ns, 0, sizeof(NewSpawn_Struct));
			spawn->FillSpawnStruct(&ns, client);
			bzsp->AddSpawn(&ns);
		}
	}
	safe_delete(bzsp);
}

void EntityList::SendZoneCorpses(Client* client)
{
	APPLAYER* app;
	LinkedListIterator<Corpse*> iterator(corpse_list);
	
	for(iterator.Reset(); iterator.MoreElements(); iterator.Advance())
	{
		Corpse *ent = iterator.GetData();
		app = new APPLAYER;
		ent->CreateSpawnPacket(app);
		client->QueuePacket(app, true, Client::CLIENT_CONNECTED);
		safe_delete(app);
	}	
}

void EntityList::SendZoneCorpsesBulk(Client* client) {
	float rate = client->Connection()->GetDataRate();
	LinkedListIterator<Corpse*> iterator(corpse_list);
	NewSpawn_Struct ns;
	Corpse *spawn;
	int32 maxspawns;

	rate = rate > 1.0 ? (rate < 10.0 ? rate : 10.0) : 1.0;
	maxspawns = (int32)rate * SPAWNS_PER_POINT_DATARATE; // FYI > 10240 entities will cause BulkZoneSpawnPacket to throw exception
	BulkZoneSpawnPacket* bzsp = new BulkZoneSpawnPacket(client, maxspawns);
	
	for(iterator.Reset(); iterator.MoreElements(); iterator.Advance())
	{
		spawn = iterator.GetData();
		if(spawn && spawn->InZone())
		{
			memset(&ns, 0, sizeof(NewSpawn_Struct));
			spawn->FillSpawnStruct(&ns, client);
			bzsp->AddSpawn(&ns);
		}
	}
	safe_delete(bzsp);
}

void EntityList::SendZoneObjects(Client* client)
{
	LinkedListIterator<Object*> iterator(object_list);
	APPLAYER app;
	iterator.Reset();
	while(iterator.MoreElements())
	{
		iterator.GetData()->CreateSpawnPacket(&app);
		client->QueuePacket(&app);
		iterator.Advance();
	}
}

void EntityList::Save()
{
	LinkedListIterator<Client*> iterator(client_list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		iterator.GetData()->Save();
		iterator.Advance();
	}	
}

void EntityList::ReplaceWithTarget(Mob* pOldMob, Mob*pNewTarget)
{
	if(!pNewTarget)
		return;
	LinkedListIterator<Mob*> iterator(mob_list);
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsAIControlled()) {
            // replace the old mob with the new one
			if (iterator.GetData()->RemoveFromHateList(pOldMob))
                    iterator.GetData()->AddToHateList(pNewTarget, 1, 0);
		}
		iterator.Advance();
	}
}

void EntityList::RemoveFromTargets(Mob* mob)
{
	LinkedListIterator<Mob*> iterator(mob_list);
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		iterator.GetData()->RemoveFromHateList(mob);
		iterator.Advance();
	}	
}

void EntityList::QueueClientsByTarget(Mob* sender, const APPLAYER* app, bool iSendToSender, Mob* SkipThisMob, bool ackreq) {
	LinkedListIterator<Client*> iterator(client_list);
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		if ((iSendToSender || (iterator.GetData() != sender && iterator.GetData()->GetTarget() == sender)) && iterator.GetData() != SkipThisMob) {
			iterator.GetData()->QueuePacket(app, ackreq);
		}
		iterator.Advance();
	}	
}
void EntityList::FilterQueueCloseClients(int8 filter, int8 required, Mob* sender, const APPLAYER* app, bool ignore_sender, float dist, Mob* SkipThisMob, bool ackreq){
	if(dist <= 0) {
		dist = 600;
	}

	float dist2 = dist * dist; //pow(dist, 2);
	
	LinkedListIterator<Client*> iterator(client_list);
	
	iterator.Reset();
	while(iterator.MoreElements()) {

		Client* ent = iterator.GetData();
			int8 filterval=ent->GetFilter(filter);
			if(required==0)
				required=1;
			if(filterval==required){
				if ((!ignore_sender || ent != sender) && (ent != SkipThisMob)
			  		) {
					if(ent->Connected() &&  (ent->DistNoRoot(*sender) <= dist2 || dist == 0)) {
							ent->QueuePacket(app, ackreq);
					}
				}
			}
		iterator.Advance();
	}
}
void EntityList::QueueCloseClients(Mob* sender, const APPLAYER* app, bool ignore_sender, float dist, Mob* SkipThisMob, bool ackreq,int8 filter) {
	if (sender == 0) {
		QueueClients(sender, app, ignore_sender);
		return;
	}
	if(dist <= 0) {
		dist = 600;
	}

	float dist2 = dist * dist; //pow(dist, 2);
	
	LinkedListIterator<Client*> iterator(client_list);
	
	iterator.Reset();
	while(iterator.MoreElements()) {

		Client* ent = iterator.GetData();

			if ((!ignore_sender || ent != sender) && (ent != SkipThisMob)) {
				int8 filter2=ent->GetFilter(filter);
				if(ent->Connected() && (ent->DistNoRoot(*sender) <= dist2 || dist == 0) && 
					(filter==0 || (filter2==1 || 
					(filter2==99 && entity_list.GetGroupByClient(ent)!=0 && 
					 entity_list.GetGroupByClient(ent)->IsGroupMember(sender))
					 || (filter2==98 && ent==sender)))) {
						ent->QueuePacket(app, ackreq);
				}
			}
		iterator.Advance();
	}	
}

void EntityList::QueueClients(Mob* sender, const APPLAYER* app, bool ignore_sender, bool ackreq) {
	LinkedListIterator<Client*> iterator(client_list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		Client* ent = iterator.GetData();

		if ((!ignore_sender || ent != sender))
		{
            ent->QueuePacket(app, ackreq, Client::CLIENT_CONNECTED);
		}
		iterator.Advance();
	}	
}


void EntityList::QueueClientsStatus(Mob* sender, const APPLAYER* app, bool ignore_sender, int8 minstatus, int8 maxstatus)
{
	LinkedListIterator<Client*> iterator(client_list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if ((!ignore_sender || iterator.GetData() != sender) && (iterator.GetData()->Admin() >= minstatus && iterator.GetData()->Admin() <= maxstatus))
		{
			iterator.GetData()->QueuePacket(app);
		}
		iterator.Advance();
	}	
}

// solar: causes caster to hit every mob within dist range of center with
// spell_id.
// NOTE: center is not affected, but caster is if it's in range
void EntityList::AESpell(Mob *caster, Mob *center, float dist, int16 spell_id)
{
	LinkedListIterator<Mob*> iterator(mob_list);
	Mob *curmob;
	
	for(iterator.Reset(); iterator.MoreElements(); iterator.Advance())
	{
		curmob = iterator.GetData();
		if(curmob != center && center->Dist(*curmob) <= dist)
			caster->SpellOnTarget(spell_id, curmob);
	}	
}

#if 0	// solar: this is old code
void EntityList::AESpell(Mob* caster, Mob* center, float dist, int16 spell_id, bool group)
{
	LinkedListIterator<Mob*> iterator(mob_list);
	iterator.Reset();
	while(iterator.MoreElements()) {
		Mob* mob = iterator.GetData();
		if (group){
			    // Client casting group spell with out target group buffs enabled
			    // Skip non group members
                if (   caster->IsClient()
                    && !caster->CastToClient()->TGB()
                    && GetGroupByMob(mob) != 0
                    && !GetGroupByMob(mob)->IsGroupMember(caster)
                    ) {
                    LogFile->write(EQEMuLog::Debug, "Group spell skipping %s", mob->GetName());
                        iterator.Advance();
                        continue;
                }
			    // Client casting group spell with target group buffs enabled
                else if (  caster->IsClient()
                        && caster->CastToClient()->TGB()
                        && GetGroupByMob(mob) != 0
                        && GetGroupByMob(mob)->IsGroupMember(caster)
                        ){
                        LogFile->write(EQEMuLog::Debug, "Group spell TGB on %s's Group", mob->GetName());
                        GetGroupByMob(mob)->CastGroupSpell(caster, spell_id);
                        iterator.Advance();
                        continue;
                }
                else if (  caster->IsClient()
                        && caster->CastToClient()->TGB()
                        && GetGroupByMob(mob) == 0
                        && mob == center
                        ){
                        LogFile->write(EQEMuLog::Debug, "Group spell TGB on %s", mob->GetName());
                        caster->SpellOnTarget(spell_id, mob);
                        return;
                }
		}
		if (
			mob->DistNoZ(*center) <= dist
			&& !(mob->IsClient() && mob->CastToClient()->GMHideMe())
			&& !mob->IsCorpse()
			) {
			//cout << "AE Spell Hit: t=" << iterator.GetData()->GetName() << ", d=" << iterator.GetData()->CastToMob()->DistNoRoot(center) << ", x=" << iterator.GetData()->CastToMob()->GetX() << ", y=" << iterator.GetData()->CastToMob()->GetY() << endl;
			if (caster == mob) {
				// Caster gets the first hit, already handled in spells.cpp
			}
		#ifdef IPC
			else if(caster->IsNPC() && !caster->CastToNPC()->IsInteractive()) {
		#else
			else if(caster->IsNPC()) {
        #endif
        	// Npc
				if (caster->IsAttackAllowed(mob) && spells[spell_id].targettype != ST_AEBard) {
				//    printf("NPC Spell casted on %s\n", mob->GetName());
					caster->SpellOnTarget(spell_id, mob);
				}
				else if (mob->IsAIControlled() && spells[spell_id].targettype == ST_AEBard) {
				//    printf("NPC mgb/aebard spell casted on %s\n", mob->GetName());
					caster->SpellOnTarget(spell_id, mob);
				}
				else {
				//    printf("NPC AE, fall thru. spell_id:%i, Target type:%x\n", spell_id, spells[spell_id].targettype);
				}
			}
		#ifdef IPC
            else if(caster->IsNPC() && caster->CastToNPC()->IsInteractive()) {
			  	// Interactive npc
				if (caster->IsAttackAllowed(mob) && spells[spell_id].targettype != ST_AEBard && spells[spell_id].targettype != ST_GroupTeleport) {
				//    printf("IPC Spell casted on %s\n", mob->GetName());
					caster->SpellOnTarget(spell_id, mob);
				}
				else if (!mob->IsAIControlled() && (spells[spell_id].targettype == ST_AEBard||group) && mob->CastToClient()->GetPVP() == caster->CastToClient()->GetPVP()) {
					    if (group && GetGroupByMob(mob) != GetGroupByMob(caster)) {
                                iterator.Advance();
                                continue;
                    }
				//    printf("IPC mgb/aebard spell casted on %s\n", mob->GetName());
				caster->SpellOnTarget(spell_id, mob);
				}
				else {
				//    printf("NPC AE, fall thru. spell_id:%i, Target type:%x\n", spell_id, spells[spell_id].targettype);
				}
			}
		#endif
            else if (caster->IsClient() && !(caster->CastToClient()->IsBecomeNPC())) {
				// Client
				if (caster->IsAttackAllowed(mob) && spells[spell_id].targettype != ST_AEBard){
				//    printf("Client Spell casted on %s\n", mob->GetName());
					caster->SpellOnTarget(spell_id, mob);
				}
				else if(spells[spell_id].targettype == ST_GroupTeleport && mob->IsClient() && mob->isgrouped && caster->isgrouped && entity_list.GetGroupByMob(caster))
				{
					Group* caster_group = entity_list.GetGroupByMob(caster);
                    if(caster_group != 0 && caster_group->IsGroupMember(mob))
					    caster->SpellOnTarget(spell_id,mob);
				}
				else if (mob->IsClient() && (spells[spell_id].targettype == ST_AEBard||group) && mob->CastToClient()->GetPVP() == caster->CastToClient()->GetPVP()) {
					if (group && GetGroupByMob(mob) != GetGroupByMob(caster)) {
                                iterator.Advance();
                                continue;
                    }
					else if (mob->IsClient() && spells[spell_id].targettype == ST_AEBard && mob->CastToClient()->GetPVP() == caster->CastToClient()->GetPVP())
						caster->SpellOnTarget(spell_id, mob);
				#ifdef IPC
                    else if (mob->IsNPC() && mob->CastToNPC()->IsInteractive()) {
					    if (group && GetGroupByMob(mob) != GetGroupByMob(caster))
					            continue;
					    caster->SpellOnTarget(spell_id, mob);
					}
			    #endif
				}
			}
			else if (caster->IsClient()) {
				// Client BecomeNPC
				caster->SpellOnTarget(spell_id, mob);
			}
		}
		iterator.Advance();
	}	
}
#endif	// solar: old code

Client* EntityList::GetClientByName(const char *checkname) {
	LinkedListIterator<Client*> iterator(client_list); 
	
	iterator.Reset(); 
	while(iterator.MoreElements()) 
	{ 
		if (strcasecmp(iterator.GetData()->GetName(), checkname) == 0) {
			return iterator.GetData();
		}
		iterator.Advance(); 
	} 
	return 0; 
}

Client* EntityList::GetClientByCharID(int32 iCharID) {
	LinkedListIterator<Client*> iterator(client_list); 
	
	iterator.Reset(); 
	while(iterator.MoreElements()) { 
		if (iterator.GetData()->CharacterID() == iCharID) {

			return iterator.GetData();
		}
		iterator.Advance(); 
	} 
	return 0; 
}

Client* EntityList::GetClientByWID(int32 iWID) {
	LinkedListIterator<Client*> iterator(client_list); 
	
	iterator.Reset(); 
	while(iterator.MoreElements()) {  
		if (iterator.GetData()->GetWID() == iWID) {
			return iterator.GetData();
		} 
		iterator.Advance(); 
	} 
	return 0; 
}

Corpse*	EntityList::GetCorpseByOwner(Client* client){
	LinkedListIterator<Corpse*> iterator(corpse_list); 
	
	iterator.Reset(); 
	while(iterator.MoreElements()) 
	{ 
		if (iterator.GetData()->IsPlayerCorpse()) 
		{ 
			if (strcasecmp(iterator.GetData()->GetOwnerName(), client->GetName()) == 0) {
				return iterator.GetData();
			}
		} 
		iterator.Advance(); 
	} 
	return 0; 
}
Corpse* EntityList::GetCorpseByID(int16 id){
	LinkedListIterator<Corpse*> iterator(corpse_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->id == id) {
			return iterator.GetData();
		}
		iterator.Advance();
	}
	return 0;
}

Group* EntityList::GetGroupByMob(Mob* mob) 
{ 
	LinkedListIterator<Group*> iterator(group_list); 
	
	iterator.Reset(); 
	while(iterator.MoreElements()) 
	{ 
		if (iterator.GetData()->IsGroupMember(mob)) {
			return iterator.GetData();
		}
		iterator.Advance(); 
	} 
	return 0; 
}
Group* EntityList::GetGroupByLeaderName(char* leader){
	LinkedListIterator<Group*> iterator(group_list); 
	
	iterator.Reset(); 
	while(iterator.MoreElements()) 
	{ 
		if (!strcmp(iterator.GetData()->GetLeaderName(),leader)) {
			return iterator.GetData();
		}
		iterator.Advance(); 
	}
	return 0;
}
Group* EntityList::GetGroupByID(int32 group_id){
	LinkedListIterator<Group*> iterator(group_list); 
	
	iterator.Reset(); 
	while(iterator.MoreElements()) 
	{ 
		if (iterator.GetData()->GetID()==group_id) {
			return iterator.GetData();
		}
		iterator.Advance(); 
	}
	return 0;
}
Group* EntityList::GetGroupByClient(Client* client) 
{ 
	LinkedListIterator<Group*> iterator(group_list); 
	
	iterator.Reset(); 
	while(iterator.MoreElements()) 
	{ 
		if (iterator.GetData()->IsGroupMember(client->CastToMob())) {
			return iterator.GetData();
		}
		iterator.Advance(); 
	} 
	return 0; 
} 

Client* EntityList::GetClientByAccID(int32 accid) 
{ 
	LinkedListIterator<Client*> iterator(client_list); 
	
	iterator.Reset(); 
	while(iterator.MoreElements()) 
	{ 
		if (iterator.GetData()->AccountID() == accid) {
			return iterator.GetData();
		}
		iterator.Advance(); 
	} 
	return 0; 
} 
Client* EntityList::GetClientByID(int16 id) { 
	LinkedListIterator<Client*> iterator(client_list); 
	
	iterator.Reset(); 
	while(iterator.MoreElements()) 
	{ 
		if (iterator.GetData()) 
		{ 
			if (iterator.GetData()->GetID() == id) {
				return iterator.GetData();
			}
		} 
		iterator.Advance(); 
	} 
	return 0; 
} 

void EntityList::ChannelMessageFromWorld(const char* from, const char* to, int8 chan_num, int32 guilddbid, int8 language, const char* message, ...) {
	va_list argptr;
	char buffer[4096];
	
	va_start(argptr, message);
	vsnprintf(buffer, 4096, message, argptr);
	va_end(argptr);
	
	LinkedListIterator<Client*> iterator(client_list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		Client* client = iterator.GetData();
		if (chan_num != 0 || (client->GuildDBID() == guilddbid && client->GuildEQID() != 0xFFFFFF && guilds[client->GuildEQID()].rank[client->GuildRank()].heargu)){
			if((chan_num!=0 && chan_num!=5) || 
				(chan_num==0 && client->GetFilter(FILTER_GUILDSAY)!=0) ||
				(chan_num==5 && client->GetFilter(FILTER_OOC)!=0))//ooc
				client->ChannelMessageSend(from, to, chan_num, language, buffer);
		}
		iterator.Advance();
	}
}

void EntityList::Message(int32 to_guilddbid, int32 type, const char* message, ...) {
	va_list argptr;
	char buffer[4096];
	
	va_start(argptr, message);
	vsnprintf(buffer, 4096, message, argptr);
	va_end(argptr);
	
	LinkedListIterator<Client*> iterator(client_list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		Client* client = iterator.GetData()->CastToClient();
		if (to_guilddbid == 0 || client->GuildDBID() == to_guilddbid)
			client->Message(type, buffer);
		iterator.Advance();
	}
}
void EntityList::QueueClientsGuild(Mob* sender, const APPLAYER* app, bool ignore_sender, int32 guildeqid){
	LinkedListIterator<Client*> iterator(client_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		Client* client = iterator.GetData()->CastToClient();
		if (client->GuildEQID() == guildeqid)
			client->QueuePacket(app);
		iterator.Advance();
	}
}
void EntityList::SendGuildJoin(GuildJoin_Struct* gj){
	LinkedListIterator<Client*> iterator(client_list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		Client* client = iterator.GetData()->CastToClient();
		if (client->GuildEQID() == gj->guildid)
			client->SendGuildJoin(gj);
		iterator.Advance();
	}
}
void EntityList::MessageStatus(int32 to_guilddbid, int to_minstatus, int32 type, const char* message, ...) {
	va_list argptr;
	char buffer[4096];
	
	va_start(argptr, message);
	vsnprintf(buffer, 4096, message, argptr);
	va_end(argptr);
	
	LinkedListIterator<Client*> iterator(client_list);
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		Client* client = iterator.GetData()->CastToClient();
		if ((to_guilddbid == 0 || client->GuildDBID() == to_guilddbid) && client->Admin() >= to_minstatus)
			client->Message(type, buffer);
		iterator.Advance();
	}
}

// works much like MessageClose, but with formatted strings
void EntityList::MessageClose_StringID(Mob *sender, bool skipsender, float dist, int32 type, int32 string_id, const char* message1,const char* message2,const char* message3,const char* message4,const char* message5,const char* message6,const char* message7,const char* message8,const char* message9)
{
	Client *c;
	LinkedListIterator<Client*> iterator(client_list);
	float dist2 = dist * dist;
	
	
	for(iterator.Reset(); iterator.MoreElements(); iterator.Advance())
	{
		c = iterator.GetData();
		if(c && c->DistNoRoot(*sender) <= dist2 && (!skipsender || c != sender))
			c->Message_StringID(type, string_id, message1, message2, message3, message4, message5, message6, message7, message8, message9);
	}
}

void EntityList::Message_StringID(Mob *sender, bool skipsender, int32 type, int32 string_id, const char* message1,const char* message2,const char* message3,const char* message4,const char* message5,const char* message6,const char* message7,const char* message8,const char* message9)
{
	Client *c;
	LinkedListIterator<Client*> iterator(client_list);
	
	
	for(iterator.Reset(); iterator.MoreElements(); iterator.Advance())
	{
		c = iterator.GetData();
		if(c && (!skipsender || c != sender))
			c->Message_StringID(type, string_id, message1, message2, message3, message4, message5, message6, message7, message8, message9);
	}
}

void EntityList::MessageClose(Mob* sender, bool skipsender, float dist, int32 type, const char* message, ...) {
	va_list argptr;
	char buffer[4096];
	
	va_start(argptr, message);
	vsnprintf(buffer, 4095, message, argptr);
	va_end(argptr);
	
	float dist2 = dist * dist;
	
	LinkedListIterator<Client*> iterator(client_list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->DistNoRoot(*sender) <= dist2 && (!skipsender || iterator.GetData() != sender)) {
			iterator.GetData()->Message(type, buffer);
		}
		iterator.Advance();
	}
}

#if 0	// solar: old code, see Mob functions: Say, Say_StringID, Shout, Emote
void EntityList::NPCMessage(Mob* sender, bool skipsender, float dist, int32 type, const char* message, ...) { 
   va_list argptr; 
   char buffer[4096]; 
   char *findzero; 
    int  stripzero; 
   va_start(argptr, message); 
   vsnprintf(buffer, 4095, message, argptr); 
   va_end(argptr); 
    findzero = strstr( buffer, "0" ); 
    stripzero = (int)(findzero - buffer + 2); 
   if (stripzero > 2 && stripzero<4096) //Incase its not an npc, you dont want to crash the zone 
      strncpy(buffer + stripzero," ",1); 
   float dist2 = dist * dist; 
   char *tmp = new char[strlen(buffer)];
   memset(tmp,0x0,sizeof(tmp));
   LinkedListIterator<Client*> iterator(client_list); 
   if (dist2==0) {
      iterator.Reset(); 
      while(iterator.MoreElements()) 
      {  
		Client* client = iterator.GetData()->CastToClient(); 
		client->Message(type, buffer); 
		iterator.Advance(); 
      } 
   } 
   else { 
      iterator.Reset(); 
      while(iterator.MoreElements()) 
      { 
         if (iterator.GetData()->DistNoRoot(*sender) <= dist2 && (!skipsender || iterator.GetData() != sender)) { 
            iterator.GetData()->Message(type, buffer); 
         } 
         iterator.Advance(); 
      } 
   }

   if (sender->GetTarget() && sender->GetTarget()->IsNPC() && buffer)
   {
	   strcpy(tmp,strstr(buffer,"says"));
	   tmp[strlen(tmp) - 1] = '\0';
	   while (*tmp)
	   {
    	   tmp++;
		   if (*tmp == '\'') { tmp++; break; }
	   }
	   if (tmp) parse->Event(1, sender->GetTarget()->GetNPCTypeID(), tmp, sender->GetTarget(), sender);
   }

} 
#endif

void EntityList::RemoveAllMobs(){
	LinkedListIterator<Mob*> iterator(mob_list);
	iterator.Reset();
	while(iterator.MoreElements())
		iterator.RemoveCurrent();
}
void EntityList::RemoveAllClients(){
	LinkedListIterator<Client*> iterator(client_list);
	iterator.Reset();
	while(iterator.MoreElements())
		iterator.RemoveCurrent(false);
}
void EntityList::RemoveAllNPCs(){
	LinkedListIterator<NPC*> iterator(npc_list);
	iterator.Reset();
	while(iterator.MoreElements())
		iterator.RemoveCurrent(false);
}
void EntityList::RemoveAllGroups(){
	LinkedListIterator<Group*> iterator(group_list);
	iterator.Reset();
	while(iterator.MoreElements())
		iterator.RemoveCurrent();
}
void EntityList::RemoveAllDoors(){
	LinkedListIterator<Doors*> iterator(door_list);
	iterator.Reset();
	while(iterator.MoreElements())
		iterator.RemoveCurrent();
}
void EntityList::RemoveAllCorpses(){
	LinkedListIterator<Corpse*> iterator(corpse_list);
	iterator.Reset();
	while(iterator.MoreElements())
		iterator.RemoveCurrent();
}
void EntityList::RemoveAllObjects(){
	LinkedListIterator<Object*> iterator(object_list);
	iterator.Reset();
	while(iterator.MoreElements())
		iterator.RemoveCurrent();
}
bool EntityList::RemoveMob(int16 delete_id){
	if(delete_id==0)
		return true;
	LinkedListIterator<Mob*> iterator(mob_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if(iterator.GetData()->GetID()==delete_id){
			if(iterator.GetData()->IsNPC())
				entity_list.RemoveNPC(delete_id);
			else if(iterator.GetData()->IsClient())
				entity_list.RemoveClient(delete_id);
			iterator.RemoveCurrent();
			return true;
		}
		iterator.Advance();
	}
	return false;
}
bool EntityList::RemoveNPC(int16 delete_id){
	LinkedListIterator<NPC*> iterator(npc_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if(iterator.GetData()->GetID()==delete_id){
			iterator.RemoveCurrent(false);//Already Deleted
			return true;
		}
		iterator.Advance();
	}
	return false;
}
bool EntityList::RemoveClient(int16 delete_id){
	LinkedListIterator<Client*> iterator(client_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if(iterator.GetData()->GetID()==delete_id){
			iterator.RemoveCurrent(false);//Already Deleted
			return true;
		}
		iterator.Advance();
	}
	return false;
}
bool EntityList::RemoveObject(int16 delete_id){
	LinkedListIterator<Object*> iterator(object_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if(iterator.GetData()->GetID()==delete_id){
			iterator.RemoveCurrent();
			return true;
		}
		iterator.Advance();
	}
	return false;
}
bool EntityList::RemoveDoor(int16 delete_id){
	LinkedListIterator<Doors*> iterator(door_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if(iterator.GetData()->GetID()==delete_id){
			iterator.RemoveCurrent();
			return true;
		}
		iterator.Advance();
	}
	return false;
}
bool EntityList::RemoveCorpse(int16 delete_id){
	LinkedListIterator<Corpse*> iterator(corpse_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if(iterator.GetData()->GetID()==delete_id){
			iterator.RemoveCurrent();
			return true;
		}
		iterator.Advance();
	}
	return false;
}
bool EntityList::RemoveGroup(int16 delete_id){
	LinkedListIterator<Group*> iterator(group_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if(iterator.GetData()->GetID()==delete_id){
			iterator.RemoveCurrent();
			return true;
		}
		iterator.Advance();
	}
	return false;
}
void EntityList::Clear()
{
	entity_list.RemoveAllClients();
	entity_list.RemoveAllNPCs();
	entity_list.RemoveAllMobs();
	entity_list.RemoveAllCorpses();
	entity_list.RemoveAllGroups();
	entity_list.RemoveAllDoors();
	entity_list.RemoveAllObjects();
	last_insert_id = 0;
}

void EntityList::UpdateWho(bool iSendFullUpdate) {
	if ((!worldserver.Connected()) || !ZoneLoaded)
		return;
	LinkedListIterator<Client*> iterator(client_list);
	int32 tmpNumUpdates = numclients + 5;
	ServerPacket* pack = 0;
	ServerClientListKeepAlive_Struct* sclka = 0;
	if (!iSendFullUpdate) {
		pack = new ServerPacket(ServerOP_ClientListKA, sizeof(ServerClientListKeepAlive_Struct) + (tmpNumUpdates * 4));
		sclka = (ServerClientListKeepAlive_Struct*) pack->pBuffer;
	}
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->InZone()) {
			if (iSendFullUpdate) {
				iterator.GetData()->UpdateWho();
			}
			else {

				if (sclka->numupdates >= tmpNumUpdates) {
					tmpNumUpdates += 10;
					int8* tmp = pack->pBuffer;
					pack->pBuffer = new int8[sizeof(ServerClientListKeepAlive_Struct) + (tmpNumUpdates * 4)];
					memset(pack->pBuffer, 0, sizeof(ServerClientListKeepAlive_Struct) + (tmpNumUpdates * 4));
					memcpy(pack->pBuffer, tmp, pack->size);
					pack->size = sizeof(ServerClientListKeepAlive_Struct) + (tmpNumUpdates * 4);
					safe_delete_array(tmp);
				}
				sclka->wid[sclka->numupdates] = iterator.GetData()->GetWID();
				sclka->numupdates++;
			}
		}
		iterator.Advance();
	}
	if (!iSendFullUpdate) {
		pack->size = sizeof(ServerClientListKeepAlive_Struct) + (sclka->numupdates * 4);
		worldserver.SendPacket(pack);
		safe_delete(pack);
	}
}

void EntityList::RemoveEntity(int16 id)
{
	if (id == 0)
		return;
	if(entity_list.RemoveMob(id))
		return;
	else if(entity_list.RemoveCorpse(id))
		return;
	else if(entity_list.RemoveDoor(id))
		return;
	else if(entity_list.RemoveGroup(id))
		return;
	else 
		entity_list.RemoveObject(id);
}

#ifdef GUILDWARS
Client* EntityList::FindRankingOfficial(int32 guild_id)
{
#ifdef GWDEBUG
	printf("FindRankingOfficial(%i)",guild_id);
#endif
Client* highestrank = 0;
	if (guild_id == 0)
		return 0;
	LinkedListIterator<Client*> iterator(client_list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GuildDBID() == guild_id)
		{
		int8 rank = iterator.GetData()->GuildRank();
#ifdef GWDEBUG
		printf("Name: %s Rank: %i\n",iterator.GetData()->GetName(),rank);
#endif
		if(highestrank == 0 && rank == 1)
			highestrank = iterator.GetData()->CastToClient();
		else if(rank == 2)
			return iterator.GetData()->CastToClient();
		}
		iterator.Advance();
	}
return highestrank;
}

Client* EntityList::FindRankingOfficialByLocation(float x,float y,float z,float dist,int32 guild_id)
{
Client* highestrank = 0;
	LinkedListIterator<Client*> iterator(client_list);
	
	iterator.Reset();
	int count = 0;
	while(iterator.MoreElements())
	{
		if(iterator.GetData()->CastToClient()->GuildDBID() == guild_id && location_list.GetDistance(x,y,z,iterator.GetData()->GetX(),iterator.GetData()->GetY(),iterator.GetData()->GetZ()) <= dist)
			count++;
		else if (highestrank == 0 && iterator.GetData()->GuildDBID() != 0 && iterator.GetData()->CastToClient()->Admin() == 0 && location_list.GetDistance(x,y,z,iterator.GetData()->GetX(),iterator.GetData()->GetY(),iterator.GetData()->GetZ()) <= dist)
		{
			highestrank = iterator.GetData();
		}
		iterator.Advance();
	}
if(!count)
return highestrank;

return 0;
}

Client* EntityList::FindEnemiesAtLocation(float x,float y,float z,float dist,int32 notguild)
{
Client* highestrank = 0;
	LinkedListIterator<Client*> iterator(client_list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GuildDBID() != 0 && iterator.GetData()->GuildDBID() != notguild && iterator.GetData()->Admin() == 0 && location_list.GetDistance(x,y,z,iterator.GetData()->GetX(),iterator.GetData()->GetY(),iterator.GetData()->GetZ()) <= dist)
		{
		return iterator.GetData();
		}
		iterator.Advance();
	}
return 0;
}

void EntityList::GuildIntervalPoints(int32 guildid,sint32 points)
{
	LinkedListIterator<Client*> iterator(client_list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->IsClient() && iterator.GetData()->GuildDBID() == guildid && iterator.GetData()->Admin() == 0)
		{
		iterator.GetData()->UpdateLDoNPoints(points,0);
		iterator.GetData()->CastToClient()->Message(0,"You have received %i points for defending a location.",points);
		}
		iterator.Advance();
	}
}
#endif

void EntityList::Process()
{
	CheckSpawnQueue();
}

void EntityList::CountNPC(int32* NPCCount, int32* NPCLootCount, int32* gmspawntype_count) {
	LinkedListIterator<NPC*> iterator(npc_list);
	*NPCCount = 0;
	*NPCLootCount = 0;
	
	iterator.Reset();
	while(iterator.MoreElements())	
	{
		(*NPCCount)++;
		(*NPCLootCount) += iterator.GetData()->CastToNPC()->CountLoot();
		if (iterator.GetData()->CastToNPC()->GetNPCTypeID() == 0)
			(*gmspawntype_count)++;
		iterator.Advance();
	}
}

void EntityList::DoZoneDump(ZSDump_Spawn2* spawn2_dump, ZSDump_NPC* npc_dump, ZSDump_NPC_Loot* npcloot_dump, NPCType* gmspawntype_dump) {
	int32 spawn2index = 0;
	int32 NPCindex = 0;
	int32 NPCLootindex = 0;
	int32 gmspawntype_index = 0;
	
	if (npc_dump != 0) {
		LinkedListIterator<NPC*> iterator(npc_list);
		NPC* npc = 0;
		iterator.Reset();
		while(iterator.MoreElements())	
		{
			npc = iterator.GetData()->CastToNPC();
			if (spawn2_dump != 0)
				npc_dump[NPCindex].spawn2_dump_index = zone->DumpSpawn2(spawn2_dump, &spawn2index, npc->respawn2);
			npc_dump[NPCindex].npctype_id = npc->GetNPCTypeID();
			npc_dump[NPCindex].cur_hp = npc->GetHP();
			if (npc->IsCorpse()) {
				if (npc->CastToCorpse()->IsLocked())
					npc_dump[NPCindex].corpse = 2;
				else
					npc_dump[NPCindex].corpse = 1;
			}
			else
				npc_dump[NPCindex].corpse = 0;
			npc_dump[NPCindex].decay_time_left = 0xFFFFFFFF;
			npc_dump[NPCindex].x = npc->GetX();
			npc_dump[NPCindex].y = npc->GetY();
			npc_dump[NPCindex].z = npc->GetZ();
			npc_dump[NPCindex].heading = npc->GetHeading();
			npc_dump[NPCindex].copper = npc->copper;
			npc_dump[NPCindex].silver = npc->silver;
			npc_dump[NPCindex].gold = npc->gold;
			npc_dump[NPCindex].platinum = npc->platinum;
			if (npcloot_dump != 0)
				npc->DumpLoot(NPCindex, npcloot_dump, &NPCLootindex);
			if (gmspawntype_dump != 0) {
				if (npc->GetNPCTypeID() == 0) {
					memcpy(&gmspawntype_dump[gmspawntype_index], npc->NPCTypedata, sizeof(NPCType));
					npc_dump[NPCindex].gmspawntype_index = gmspawntype_index;
					gmspawntype_index++;
				}
			}
			NPCindex++;
			iterator.Advance();
		}
	}
	if (spawn2_dump != 0)
		zone->DumpAllSpawn2(spawn2_dump, &spawn2index);
}

void EntityList::Depop() {
	LinkedListIterator<NPC*> iterator(npc_list);
	
	iterator.Reset();
	while(iterator.MoreElements())
	{
		iterator.GetData()->Depop();
		iterator.Advance();
	}
}

void EntityList::SendTraders(Client* client){
	LinkedListIterator<Client*> iterator(client_list);
	iterator.Reset();
	Client* trader;
	while(iterator.MoreElements()) {
		trader=iterator.GetData();
		if(trader->Trader)
			client->SendTraderPacket(trader);
		iterator.Advance();
	}
}


// Currently, a new packet is sent per entity.
// @todo: Come back and use FLAG_COMBINED to pack
// all updates into one packet.
void EntityList::SendPositionUpdates(Client* client, int32 cLastUpdate, float range, Entity* alwayssend, bool iSendEvenIfNotChanged) {
	range = range * range;
	LinkedListIterator<Mob*> iterator(mob_list);
	
	APPLAYER* outapp = 0;
	PlayerPositionUpdateServer_Struct* ppu = 0;
	Mob* mob = 0;
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (outapp == 0) {
			outapp = new APPLAYER(OP_ClientUpdate, sizeof(PlayerPositionUpdateServer_Struct));
			ppu = (PlayerPositionUpdateServer_Struct*)outapp->pBuffer;
		}
		mob = iterator.GetData()->CastToMob();
		if (!mob->IsCorpse() 
			&& (iterator.GetData() != client)
			&& (mob->IsClient() || iSendEvenIfNotChanged || (mob->LastChange() >= cLastUpdate)) 
			&& (!iterator.GetData()->IsClient() || !iterator.GetData()->CastToClient()->GMHideMe(client))
			) {
				if (range == 0 || (iterator.GetData() == alwayssend) || (mob->DistNoRootNoZ(*client) <= range)) {
					mob->MakeSpawnUpdate(ppu);
			}
		}
		if(mob && mob->IsClient())
			client->QueuePacket(outapp, false, Client::CLIENT_CONNECTED);
		safe_delete(outapp);
		outapp = 0;	
		iterator.Advance();
	}
	
	safe_delete(outapp);
}

char* EntityList::MakeNameUnique(char* name) {
	bool used[100];
	memset(used, 0, sizeof(used));
	name[61] = 0; name[62] = 0; name[63] = 0;

	LinkedListIterator<Mob*> iterator(mob_list);

	iterator.Reset();
	int len = strlen(name);
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsMob()) {
			if (strncasecmp(iterator.GetData()->CastToMob()->GetName(), name, len) == 0) {
				if (Seperator::IsNumber(&iterator.GetData()->CastToMob()->GetName()[len])) {
					used[atoi(&iterator.GetData()->CastToMob()->GetName()[len])] = true;
				}
			}
		}
		iterator.Advance();
	}
	for (int i=0; i < 100; i++) {
		if (!used[i]) {
			#ifdef WIN32
			snprintf(name, 64, "%s%02d", name, i);
			#else
			//glibc clears destination of snprintf
			//make a copy of name before snprintf--misanthropicfiend
			char temp_name[64];
			strn0cpy(temp_name, name, 64);
			snprintf(name, 64, "%s%02d", temp_name, i);
			#endif
			return name;
		}
	}
	LogFile->write(EQEMuLog::Error, "Fatal error in EntityList::MakeNameUnique: Unable to find unique name for '%s'", name);
	char tmp[64] = "!";
	strn0cpy(&tmp[1], name, sizeof(tmp) - 1);
	strcpy(name, tmp);
	return MakeNameUnique(name);
}
void EntityList::SendAATimer(int32 charid,UseAA_Struct* uaa){
	Client* client2=this->GetClientByCharID(charid);
	if(!client2){
		LogFile->write(EQEMuLog::Error, "Error in SendAATimer: Couldnt find character!");
		return;
	}
	client2->SendAATimer(uaa);
}
char* EntityList::RemoveNumbers(char* name) {
	char	tmp[64];
	memset(tmp, 0, sizeof(tmp));
	int k = 0;
	for (unsigned int i=0; i<strlen(name) && i<sizeof(tmp); i++) {
		if (name[i] < '0' || name[i] > '9')
			tmp[k++] = name[i];
	}
	strn0cpy(name, tmp, sizeof(tmp));
	return name;
}
void EntityList::ListNPCs(Client* client, const char* arg1, const char* arg2, int8 searchtype) {
	if (arg1 == 0)
		searchtype = 0;
	else if (arg2 == 0 && searchtype >= 2)
		searchtype = 0;
	LinkedListIterator<NPC*> iterator(npc_list);
	int32 x = 0;
	int32 z = 0;
	char sName[36];
	
	iterator.Reset();
	client->Message(0, "NPCs in the zone:");
	if(searchtype == 0) {
		while(iterator.MoreElements()) {
			client->Message(0, "  %5d: %s", iterator.GetData()->GetID(), iterator.GetData()->GetName());
			x++;
			z++;
			iterator.Advance();
		}
	}
	else if(searchtype == 1) {
		client->Message(0, "Searching by name method. (%s)",arg1);
		char* tmp = new char[strlen(arg1) + 1];
		strcpy(tmp, arg1);
		strupr(tmp);
		while(iterator.MoreElements()) {
			z++;
			strcpy(sName, iterator.GetData()->GetName());
			strupr(sName);
			if (strstr(sName, tmp)) {
				client->Message(0, "  %5d: %s", iterator.GetData()->GetID(), iterator.GetData()->GetName());
				x++;
			}
			iterator.Advance();
		}
		safe_delete_array(tmp);
	}
	else if(searchtype == 2) {
		client->Message(0, "Searching by number method. (%s %s)",arg1,arg2);
		while(iterator.MoreElements()) {
			z++;
			if ((iterator.GetData()->GetID() >= atoi(arg1)) && (iterator.GetData()->GetID() <= atoi(arg2)) && (atoi(arg1) <= atoi(arg2))) {
				client->Message(0, "  %5d: %s", iterator.GetData()->GetID(), iterator.GetData()->GetName());
				x++;
			}
			iterator.Advance();
		}
	}
	client->Message(0, "%d npcs listed. There is a total of %d npcs in this zone.", x, z);
}

void EntityList::ListNPCCorpses(Client* client) {
	LinkedListIterator<Corpse*> iterator(corpse_list);
	int32 x = 0;
	
	iterator.Reset();
	client->Message(0, "NPC Corpses in the zone:");
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsNPCCorpse()) {
			client->Message(0, "  %5d: %s", iterator.GetData()->GetID(), iterator.GetData()->GetName());
			x++;
		}
		iterator.Advance();
	}
	client->Message(0, "%d npc corpses listed.", x);
}

void EntityList::ListPlayerCorpses(Client* client) {
	LinkedListIterator<Corpse*> iterator(corpse_list);
	int32 x = 0;
	
	iterator.Reset();
	client->Message(0, "Player Corpses in the zone:");
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsPlayerCorpse()) {
			client->Message(0, "  %5d: %s", iterator.GetData()->GetID(), iterator.GetData()->GetName());
			x++;
		}
		iterator.Advance();
	}
	client->Message(0, "%d player corpses listed.", x);
}

// returns the number of corpses deleted. A negative number indicates an error code.
sint32 EntityList::DeleteNPCCorpses() {
	LinkedListIterator<Corpse*> iterator(corpse_list);
	sint32 x = 0;
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsNPCCorpse()) {
			iterator.GetData()->Depop();
			x++;
		}
		iterator.Advance();
	}
	return x;
}

// returns the number of corpses deleted. A negative number indicates an error code.
sint32 EntityList::DeletePlayerCorpses() {
	LinkedListIterator<Corpse*> iterator(corpse_list);
	sint32 x = 0;
	
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->IsPlayerCorpse()) {
			iterator.GetData()->CastToCorpse()->Delete();
			x++;
		}
		iterator.Advance();
	}
	return x;
}
void EntityList::SendPetitionToAdmins(){
	LinkedListIterator<Client*> iterator(client_list);
	APPLAYER* outapp = new APPLAYER(OP_PetitionUpdate,sizeof(PetitionUpdate_Struct));
	PetitionUpdate_Struct* pcus = (PetitionUpdate_Struct*) outapp->pBuffer;
	pcus->petnumber = 0;		// Petition Number
	pcus->color = 0;
	pcus->status = 0xFFFFFFFF;
	pcus->senttime = 0;
	strcpy(pcus->accountid, "");
	strcpy(pcus->gmsenttoo, "");
	pcus->quetotal=0;
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->CastToClient()->Admin() >= 80)
			iterator.GetData()->CastToClient()->QueuePacket(outapp);
		iterator.Advance();
	}
	safe_delete(outapp);
}
void EntityList::SendPetitionToAdmins(Petition* pet) {
	LinkedListIterator<Client*> iterator(client_list);
	
	APPLAYER* outapp = new APPLAYER(OP_PetitionUpdate,sizeof(PetitionUpdate_Struct));
	PetitionUpdate_Struct* pcus = (PetitionUpdate_Struct*) outapp->pBuffer;
	pcus->petnumber = pet->GetID();		// Petition Number
	if (pet->CheckedOut()) {
		pcus->color = 0x00;
		pcus->status = 0xFFFFFFFF;
		pcus->senttime = pet->GetSentTime();
		strcpy(pcus->accountid, "");
		strcpy(pcus->gmsenttoo, "");
	}
	else {
		pcus->color = pet->GetUrgency();	// 0x00 = green, 0x01 = yellow, 0x02 = red
		pcus->status = pet->GetSentTime();
		pcus->senttime = pet->GetSentTime();			// 4 has to be 0x1F
		strcpy(pcus->accountid, pet->GetAccountName());
		strcpy(pcus->charname, pet->GetCharName());
	}
	pcus->quetotal = petition_list.GetTotalPetitions();
	iterator.Reset();
	while(iterator.MoreElements()) {
		if (iterator.GetData()->CastToClient()->Admin() >= 80) {
			if (pet->CheckedOut())
				strcpy(pcus->gmsenttoo, "");
			else
				strcpy(pcus->gmsenttoo, iterator.GetData()->CastToClient()->GetName());
			iterator.GetData()->CastToClient()->QueuePacket(outapp);
		}
		iterator.Advance();
	}
	safe_delete(outapp);
}

void EntityList::ClearClientPetitionQueue() {
	APPLAYER* outapp = new APPLAYER(OP_PetitionUpdate,sizeof(PetitionUpdate_Struct));
	PetitionUpdate_Struct* pet = (PetitionUpdate_Struct*) outapp->pBuffer;
	pet->color = 0x00;
	pet->status = 0xFFFFFFFF;
	pet->senttime = 0;
	strcpy(pet->accountid, "");
	strcpy(pet->gmsenttoo, "");
	strcpy(pet->charname, "");
	pet->quetotal = petition_list.GetTotalPetitions();
	LinkedListIterator<Client*> iterator(client_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->CastToClient()->Admin() >= 100) {
			int x = 0;
			for (x=0;x<64;x++) {
				pet->petnumber = x;
				iterator.GetData()->CastToClient()->QueuePacket(outapp);
			}
		}
		iterator.Advance();
	}
	safe_delete(outapp);
	return;
}

void EntityList::WriteEntityIDs() {
	LinkedListIterator<Mob*> iterator(mob_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		cout << "ID: " << iterator.GetData()->GetID() << "  Name: " << iterator.GetData()->GetName() << endl;
		iterator.Advance();
	}
}

BulkZoneSpawnPacket::BulkZoneSpawnPacket(Client* iSendTo, int32 iMaxSpawnsPerPacket) {
	data = 0;
	pSendTo = iSendTo;
	pMaxSpawnsPerPacket = iMaxSpawnsPerPacket;
#ifdef _EQDEBUG
	if (pMaxSpawnsPerPacket <= 0 || pMaxSpawnsPerPacket > MAX_SPAWNS_PER_PACKET) {
		// ok, this *cant* be right =p
		ThrowError("Error in BulkZoneSpawnPacket::BulkZoneSpawnPacket(): pMaxSpawnsPerPacket outside range that makes sense");
	}
#endif
}

BulkZoneSpawnPacket::~BulkZoneSpawnPacket() {
	SendBuffer();
	safe_delete_array(data)
}

bool BulkZoneSpawnPacket::AddSpawn(NewSpawn_Struct* ns) {
	if (!data) {
		data = new NewSpawn_Struct[pMaxSpawnsPerPacket];
		memset(data, 0, sizeof(NewSpawn_Struct) * pMaxSpawnsPerPacket);
		index = 0;
	}
	memcpy(&data[index], ns, sizeof(NewSpawn_Struct));
	index++;
	if (index >= pMaxSpawnsPerPacket) {
		SendBuffer();
		return true;
	}
	return false;
}

void BulkZoneSpawnPacket::SendBuffer() {
	if (!data)
		return;
	int32 tmpBufSize = (sizeof(NewSpawn_Struct) * pMaxSpawnsPerPacket) + 50;
	APPLAYER* outapp = new APPLAYER(OP_ZoneSpawns, tmpBufSize);
	outapp->size = DeflatePacket((int8*) data, index * sizeof(NewSpawn_Struct), outapp->pBuffer, tmpBufSize);
	outapp->opcode |=FLAG_COMPRESSED;
	//EncryptZoneSpawnPacket(outapp);
	//DumpPacket(outapp);
	if (pSendTo)
		pSendTo->QueuePacket(outapp);
	else
		entity_list.QueueClients(0, outapp);
	safe_delete(outapp);
	memset(data, 0, sizeof(NewSpawn_Struct) * pMaxSpawnsPerPacket);
	index = 0;
}

void EntityList::DoubleAggro(Mob* who)
{
	LinkedListIterator<NPC*> iterator(npc_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->CheckAggro(who))
		  iterator.GetData()->SetHate(who,iterator.GetData()->CastToNPC()->GetHateAmount(who),iterator.GetData()->CastToNPC()->GetHateAmount(who)*2);
		iterator.Advance();
	}
}

void EntityList::HalveAggro(Mob* who)
{
	LinkedListIterator<NPC*> iterator(npc_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->CastToNPC()->CheckAggro(who))
		  iterator.GetData()->CastToNPC()->SetHate(who,iterator.GetData()->CastToNPC()->GetHateAmount(who)/2);
		iterator.Advance();
	}
}

void EntityList::ClearFeignAggro(Mob* targ)
{
	LinkedListIterator<NPC*> iterator(npc_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->CastToNPC()->CheckAggro(targ))
		{
			iterator.GetData()->CastToNPC()->RemoveFromHateList(targ);
			if (iterator.GetData()->CastToMob()->GetLevel() >= 35)
				iterator.GetData()->CastToNPC()->SetFeignMemory(targ->CastToMob()->GetName());
		}
		iterator.Advance();
	}
}


// Signal Quest command function
void EntityList::SignalMobsByNPCID(int32 snpc)
{
	LinkedListIterator<Mob*> iterator(mob_list);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData()->GetNPCTypeID() == snpc)
		{
			iterator.GetData()->signaled=true;
		}
		iterator.Advance();
	}
}


bool EntityList::MakeTrackPacket(Client* client){
	int32 distance = 0;
	if(client->GetClass() == DRUID)
		distance = (client->GetSkill(TRACKING)*7);
	else if(client->GetClass() == RANGER)
		distance = (client->GetSkill(TRACKING)*12);
	else if(client->GetClass() == BARD)
		distance = (client->GetSkill(TRACKING)*10); 
	if(distance <= 0)
		return false;
	if(distance<300)
		distance=300;
	int8 spe=100;
	
	bool ret = false;
	
	uchar* buffer1 = new uchar[sizeof(Track_Struct)];
	Track_Struct* track_ent = (Track_Struct*) buffer1;
	
	uchar* buffer2 = new uchar[sizeof(Track_Struct)*spe];
	Tracking_Struct* track_array = (Tracking_Struct*) buffer2;
	memset(track_array, 0, sizeof(Track_Struct)*spe);
	int array_counter = 0;
	
	LinkedListIterator<Mob*> iterator(mob_list);
	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData() && (iterator.GetData()->DistNoZ(*client)<=distance))
		{
			memset(track_ent, 0, sizeof(Track_Struct));
			Mob* cur_entity = iterator.GetData();
			track_ent->entityid = cur_entity->GetID();
			track_ent->x=(int16)cur_entity->GetX();
			track_ent->y=(int16)cur_entity->GetY();
			track_ent->z=(int16)cur_entity->GetZ();
			memcpy(&track_array->Entrys[array_counter], track_ent, sizeof(Track_Struct));
			array_counter++;
			if (array_counter >= (spe)){
				APPLAYER* outapp = new APPLAYER(OP_Track,sizeof(Track_Struct)*spe);
				memcpy(outapp->pBuffer, track_array,sizeof(Track_Struct)*spe);
				outapp->priority = 6;
				client->QueuePacket(outapp);
				safe_delete(outapp);
				ret = true;
				break;
			}
		}
		iterator.Advance();
	}
	if ((array_counter!=0) && (ret==false)) {
		APPLAYER* outapp = new APPLAYER(OP_Track,sizeof(Track_Struct)*(array_counter));
		memcpy(outapp->pBuffer, track_array,sizeof(Track_Struct)*(array_counter));
		outapp->priority = 6;
		client->QueuePacket(outapp);
		safe_delete(outapp);
		ret = true;
	}
	
	safe_delete_array(buffer1);
	safe_delete_array(buffer2);
	
	return ret;
}
